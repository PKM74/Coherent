/*
 * AC
 * Login connect-time accounting.
 */

#include <stdio.h>
#include <ctype.h>
#include <utmp.h>
#include <time.h>

#define	DAYSEC	(24*60*60L)		/* Seconds in a day */
#define	HOUR	(60*60)			/* Seconds in hour */

char	*wtmpf = "/usr/adm/wtmp";
char	*months[] = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December",
};

char	*days[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday",
	"Thursday", "Friday", "Saturday"
};
char	**plist;			/* List of do-only people */

typedef	struct TERM {
	struct TERM	*t_next;
	char	t_line[8];
	char	t_name[DIRSIZ];
	time_t	t_time;
}	TERM;

TERM	*terminals;

typedef	struct PEOPLE {
	struct PEOPLE	*p_next;
	char	p_name[DIRSIZ];
	time_t	p_time;
}	PEOPLE;

PEOPLE	*people;

time_t	lasttime;		/* Last time read from file */
time_t	runtotal;		/* Running total used by ac -d */
time_t	midnight;		/* Time of next midnight */

int	dflag;			/* Daily version (midnight-midnight) */
int	pflag;			/* Print totals by people */
int	badflag;		/* Possible bad file format */

main(argc, argv)
char *argv[];
{
	register char *ap;

	while (argc>1 && *argv[1]=='-') {
		for (ap = &argv[1][1]; *ap != '\0'; ap++)
			switch (*ap) {
			case 'd':
				dflag = 1;
				break;

			case 'p':
				pflag = 1;
				break;


			case 'w':
				if (argc < 2)
					usage();
				wtmpf = argv[2];
				argv++;
				argc--;
				break;

			default:
				usage();
			}
		argv++;
		argc--;
	}
	plist = argv+1;
	readwtmp();
	print(0);
	if (badflag)
		acerr("possible bad file format");
	exit(0);
}

/*
 * Read through the wtmp file keeping track of
 * each individual user.  At the end either print total
 * or by individual people.
 */
readwtmp()
{
	struct utmp ut;
	register struct tm *tmp;
	register TERM *tp;
	register FILE *fp;
	time_t tdelta;

	if ((fp = fopen(wtmpf, "r")) == NULL) {
		fprintf(stderr, "ac: cannot open %s\n", wtmpf);
		exit(1);
	}
	while (fread(&ut, sizeof ut, 1, fp) == 1) {
		if (dflag && midnight==0) {
			tmp = localtime(&ut.ut_time);
			midnight = ut.ut_time - tmp->tm_sec
			    - tmp->tm_min*60 - tmp->tm_hour*60*60L + DAYSEC;
		} else while (dflag && lasttime>=midnight) {
			atmidnight();
			print(1);
			midnight += DAYSEC;
		}
		if (ut.ut_line[1] == '\0')
			switch (ut.ut_line[0]) {
			case '~':		/* Reboot */
				lasttime = ut.ut_time;
				logout(NULL);
				continue;

			case '|':		/* Old time */
				tdelta = ut.ut_time;
				continue;

			case '}':		/* New time */
				tdelta = ut.ut_time-tdelta;
				lasttime = ut.ut_time;
				for (tp = terminals; tp!=NULL; tp = tp->t_next)
					tp->t_time += tdelta;
				continue;
			}
		if (ut.ut_time < lasttime) {
			if (lasttime-ut.ut_time > 60) {
				badflag++;
				continue;
			}
			ut.ut_time = lasttime;
		} else
			lasttime = ut.ut_time;
		logout(&ut);
		if (ut.ut_name[0] != '\0')
			login(&ut);
	}
	fclose(fp);
	time(&lasttime);
	logout(NULL);
}

/*
 * Mark a user as logged out.
 * The user is given by the tty-name
 * found in the utmp structure pointer.
 * If this pointer is NULL, log all users
 * out (e.g. at reboot and end of file).
 */
logout(utp)
register struct utmp *utp;
{
	register TERM *tp, *ptp;

loop:
	if (terminals == NULL)
		return;
	ptp = NULL;
	for (tp=terminals; tp != NULL; ptp=tp, tp=tp->t_next)
		if (utp==NULL || strncmp(tp->t_line, utp->ut_line, 8)==0) {
			if (ptp == NULL)
				terminals = tp->t_next; else
				ptp->t_next = tp->t_next;
			tp->t_time = (utp==NULL ? lasttime : utp->ut_time)
			    - tp->t_time;
			enter(tp);
			free((char *)tp);
			if (utp != NULL)
				return;
			goto loop;
		}
}

/*
 * Log the times recorded for the terminals by
 * midnight of the current day.
 */
atmidnight()
{
	register TERM *tp;

	for (tp = terminals; tp != NULL; tp = tp->t_next) {
		if (tp->t_time < midnight) {
			tp->t_time = midnight-tp->t_time;
			enter(tp);
			tp->t_time = midnight;
		}
	}
}

/*
 * Mark the user found on this terminal
 * as logged in.
 */
login(utp)
register struct utmp *utp;
{
	register TERM *tp;
	register char *np;

	for (np = utp->ut_name; *np != '\0'; np++)
		if (!isascii(*np) || !isprint(*np)) {
			badflag++;
			lasttime = 0;
			return;
		}
	for (np = utp->ut_line; *np != '\0'; np++)
		if (!isascii(*np) || (!isalpha(*np) && !isdigit(*np))) {
			badflag++;
			lasttime = 0;
			return;
		}
	if (*plist != NULL) {
		register char **plp;

		for (plp = plist; *plp != NULL; plp++)
			if (**plp == *utp->ut_name
			     && strncmp(*plp, utp->ut_name, DIRSIZ)==0)
				break;
		if (*plp == NULL)
			return;
	}
	if ((tp = (TERM*)malloc(sizeof(TERM)))==NULL)
		acerr("Out of memory for terminals");
	tp->t_time = utp->ut_time;
	strncpy(tp->t_name, utp->ut_name, DIRSIZ);
	strncpy(tp->t_line, utp->ut_line, 8);
	tp->t_next = terminals;
	terminals = tp;
}

/*
 * Enter a terminal node into the people table.
 */
enter(tp)
register TERM *tp;
{
	register PEOPLE *pp;

	if (tp->t_name[0] == '\0')
		return;
	for (pp = people; pp != NULL; pp = pp->p_next)
		if (strncmp(tp->t_name, pp->p_name, DIRSIZ) == 0) {
			pp->p_time += tp->t_time;
			return;
		}
	if ((pp = (PEOPLE *)malloc(sizeof(PEOPLE))) == NULL)
		acerr("Out of memory for people");
	pp->p_next = people;
	people = pp;
	pp->p_time = tp->t_time;
	strncpy(pp->p_name, tp->t_name, DIRSIZ);
}

/*
 * Print the results, depending on the flags.
 * The `flag' says whether print is called at the
 * end (0) or for each midnight-midnight period (1).
 */
print(flag)
int flag;
{
	register struct tm *tmp;
	register PEOPLE *pp;
	time_t total = 0;

	if (dflag) {
		time_t endofday;

		endofday = midnight-2*HOUR;
		tmp = localtime(&endofday);
		printf("%s %s %d:\n", days[tmp->tm_wday], months[tmp->tm_mon],
		    tmp->tm_mday);
	}
	for (pp = people; pp != NULL; pp = pp->p_next) {
		if (pp->p_time > 0) {
			total += pp->p_time;
			if (pflag)
				prtime(pp->p_name, pp->p_time);
		}
		if (flag)
			pp->p_time = 0;
	}
	runtotal += total;
	prtime("Total:", total);
	if (dflag) {
		printf("\n");
		if (flag == 0) {
			dflag = 0;
			prtime("Total:", runtotal);
		}
	}
}

/*
 * Print a time entry with a name.
 */
prtime(name, time)
char *name;
time_t time;
{
	if (dflag)
		printf("\t");
	printf("%-*.*s", DIRSIZ, DIRSIZ, name);
	time = (time+30)/60;
	printf("%3ld:%02ld\n", time/60, time%60);
}

/*
 * Error reporting.
 */
/* VARARGS */
acerr(x)
{
	fprintf(stderr, "ac: %r\n", &x);
	exit(1);
}

usage()
{
	fprintf(stderr, "Usage: ac [-w wtmpfile] [-d] [-p] [username ...]\n");
	exit(1);
}
