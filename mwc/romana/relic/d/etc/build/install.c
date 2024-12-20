/*
 * install.c
 * 6/7/91
 * Install COHERENT disks on a system.
 * This is the back end of the initial COHERENT installation procedure;
 * the first part is in build.c.
 * Without the -b option, it installs an update to an existing COH system.
 * Uses common routines in build0.o: cc install.c build0.c
 * Usage: install [ -bdv ] id device ndisks
 * Options:
 *	-b	Build: special processing for build, part 2.
 *	-d	Debug, echo commands without executing
 *	-v	Verbose
 */

#include <stdio.h>
#include "build0.h"

#define	VERSION		"1.12"
#define	USAGE		"Usage: /etc/install [ -bdv ] id device ndisks\n"

/* Forward. */
void	config();
void	done();
void	install();
int	newdisk();
void	newusr();
void	setbaud();

/* Globals. */
int	bflag;				/* build flag		*/
char	*device;			/* special device name	*/
char	*id;				/* disk id		*/
int	ndisks;				/* number of disks	*/

/*
 * Baud rate table for serial port initialization.
 * The index in the table gives the baud rate value from <sgtty.h>.
 */
#define	MAXBAUD	17
int	baudrate[MAXBAUD+1] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2000,
	2400, 3600, 4800, 7200, 9600, 19200
};

main(argc, argv) int argc; char *argv[];
{
	register char *s;
	register int i;

	argv0 = argv[0];
	abortmsg = 1;
	usagemsg = USAGE;
	if (argc > 1 && argv[1][0] == '-') {
		for (s = &argv[1][1]; *s; ++s) {
			switch(*s) {
			case 'b':	++bflag;	break;
			case 'd':	++dflag;	break;
			case 'v':	++vflag;	break;
			case 'V':
				fprintf(stderr, "%s: V%s\n", argv0, VERSION);
				break;
			default:	usage();	break;
			}
		}
		--argc;
		++argv;
	}
	if (argc != 4)
		usage();
	id = argv[1];
	device = argv[2];
	ndisks = atoi(argv[3]);

	/* Add line to /etc/install.log. */
	sprintf(cmd, "/bin/echo /etc/install: %s %s %s >>/etc/install.log",
		argv[1], argv[2], argv[3]);
	sys(cmd, S_NONFATAL);
	sys("/bin/date >>/etc/install.log", S_NONFATAL);

	/* Remove old ids and postfile if present. */
	if (bflag)	/* Leave disk 1 marker on /. */
		sprintf(cmd, "/bin/rm -f /%s.[023456789]* /conf/%s.post", id, id);
	else
		sprintf(cmd, "/bin/rm -f /%s.* /conf/%s.post", id, id);
	sys(cmd, S_IGNORE);
	if (bflag)
		sys("/etc/mount.all", S_NONFATAL);
	cls(0);

#if	0
	/* Execute prefile.  Not required for the moment. */
	sprintf(cmd, "/conf/%s.pre", id);
	if (exists(cmd)) {
		cls(0);
		sys(cmd, S_NONFATAL);
	}
#endif

	/*
	 * Install disks.
	 * Disk numbers are 2 to ndisks for build, 1 to ndisks otherwise.
	 */
	for (i = ((bflag) ? 2 : 1); i <= ndisks; ++i)
		install(i);
	if (bflag) {
		newusr();
		config();
		done();
	}

	/* Delete ids and execute postfile if present. */
	sprintf(cmd, "/bin/rm -f /%s.*", id);
	sys(cmd, S_NONFATAL);
	sprintf(cmd, "/conf/%s.post", id);
	if (exists(cmd)) {
		cls(0);
		sys(cmd, S_NONFATAL);
	}
	sys("/bin/echo /etc/install: success >>/etc/install.log", S_NONFATAL);
	sys("/bin/date >>/etc/install.log", S_NONFATAL);
	sys("/bin/echo >>/etc/install.log", S_NONFATAL);
	if (bflag)
		sys("/etc/umount.all", S_NONFATAL);
	cls(0);
	printf("You have completed the installation procedure successfully.\n");
	printf("Don't forget to remove the last diskette from the disk drive.\n");
	sync();
	exit(0);
}

/*
 * System-specific configuration.
 */
void
config()
{
	register int port, i, polled, parallel;
	char device[6+1];		/* e.g. "com1pr" */
	char rdevice[7+1];

	cls(1);
	if (yes_no("Does your computer system have a modem")) {
		printf(
"You must specify which asychronous serial line your modem will use.\n"
"See the article \"com\" in the COHERENT documentation for details.\n"
			);
		port = get_int(1, 4, "Enter 1 to 4 for COM1 through COM4:");
		i = (port > 2) ? port - 2 : port;	/* 1 or 2 */
		printf(
"If your computer system uses both ports COM%d and COM%d,\n"
"one must be run in polled mode rather than interrupt-driven.\n",
			i, i+2);
		polled = yes_no("Do you want to run COM%d in polled mode", port);
		sprintf(cmd, "/bin/ln -f /dev/com%d%sr /dev/modem",
			port, (polled) ? "p" : "");
		if (sys(cmd, S_NONFATAL) == 0)
			printf("/dev/modem is now linked to /dev/com%d%sr.\n",
				port, (polled) ? "p" : "");
		printf("\n");
	}
	if (yes_no("Does your computer system have a printer")) {
		printf(
"Your printer is connected to your computer system either through a\n"
"parallel port or through a serial port; most printers are connected\n"
"through parallel port LPT1.\n"
			);
again:
		parallel = yes_no("Is your printer connected through a parallel port");
		if (parallel) {
			port = get_int(1, 3, "Enter 1, 2 or 3 for port LPT1, LPT2 or LPT3:");
			sprintf(device, "lpt%d", port);
		} else {
			port = get_int(1, 4, "Enter 1 to 4 for COM1 through COM4:");
			i = (port > 2) ? port - 2 : port;
			printf(
"If your computer system uses both ports COM%d and COM%d,\n"
"one must be run in polled mode rather than interrupt-driven.\n",
				i, i+2);
			polled = yes_no("Do you want to run COM%d in polled mode", port);
			sprintf(device, "com%d%sl", port, polled);
			printf("By default, COM%d runs at 9600 baud.\n", port);
			if (yes_no("Does your device use a different baud rate"))
				setbaud(port);
		}
		printf(
"Even if you know which port your printer uses under your existing operating\n"
"system, under COHERENT the port name may be different. For this reason,\n"
"we strongly recommend that you test your printer configuration.\n"
"If you test your printer configuration and see no output on your printer,\n"
"you can try a different configuration until you find the one which works.\n"
			);
		if (yes_no("Do you want to test whether your printer configuration is correct")) {
			/* The command below is backgrounded in case it hangs. */
			printf("Testing /dev/%s: process ", device);
			fflush(stdout);
			sprintf(cmd,
"/bin/echo -n 'This is printing on device /dev/%s.\r\n\014' >/dev/%s&",
				device, device);	/* 014 is formfeed */
			sys(cmd, S_IGNORE);
			if (!yes_no("\nDid output appear on your printer")) {
				printf(
"Now try again, specifying a different port for your printer.\n"
					);
				goto again;
			}
		}
		sprintf(cmd, "/bin/ln -f /dev/%s /dev/lp", device);
		if (sys(cmd, S_NONFATAL) == 0)
			printf("/dev/lp is now linked to /dev/%s.\n", device);
		if (yes_no("Is your printer an HP LaserJet compatible laser printer")) {
			sprintf(cmd, "/bin/ln -f /dev/%s /dev/hp", device);
			if (sys(cmd, S_NONFATAL) == 0)
				printf("/dev/hp is now linked to /dev/%s.\n", device);
			sprintf(rdevice, "%s%s", (parallel) ? "r" : "", device);
			sprintf(cmd, "/bin/ln -f /dev/%s /dev/rhp", rdevice);
			if (sys(cmd, S_NONFATAL) == 0)
				printf("/dev/rhp is now linked to /dev/%s.\n", rdevice);
		}
		printf("\n");
	}
}

/*
 * Finish up.
 */
void
done()
{
	cls(1);

	/* Replace the install version of /etc/brc with the normal one. */
	sys("/bin/rm /etc/brc", S_NONFATAL);
	sys("/bin/ln -f /etc/brc.coh /etc/brc", S_NONFATAL);
}

/*
 * Install disk n.
 */
void
install(n) int n;
{
	register int i;

again:
	get_line("Insert a disk from the installation kit into the drive and hit <Enter>.",
		n);
	sprintf(cmd, "/etc/mount %s /mnt -r", device);
	if (sys(cmd, S_NONFATAL))
		goto again;
	if ((i = newdisk()) == 0) {
		sprintf(cmd, "/etc/umount %s", device);
		sys(cmd, S_NONFATAL);
		goto again;
	}
	printf("Copying disk %d.  This will take a few minutes...\n", i);
	sprintf(cmd, "/bin/cpdir -ad%s -smnt /mnt /", (vflag) ? "v" : "");
	sys(cmd, S_FATAL);
	sprintf(cmd, "/etc/umount %s", device);
	sys(cmd, S_NONFATAL);
	sprintf(cmd, "/bin/echo /etc/install: disk %d installed >>/etc/install.log",
		i);
	sys(cmd, S_NONFATAL);
}

/*
 * Check for an appropriate id on the disk on /mnt.
 * Return 0 if not found, disk number otherwise.
 */
int
newdisk()
{
	register int i;
	static int n;

	if (dflag)
		return (n >= ndisks) ? 0 : ++n;
	for (i = 1; i <= ndisks; i++) {
		sprintf(buf, "/mnt/%s.%d", id, i);
		if (!exists(buf))
			continue;			/* not disk i */
		sprintf(buf, "/%s.%d", id, i);
		if (exists(buf)) {			/* exists on root */
			printf(
				"The disk you inserted is disk %d of the kit;\n"
				"it has already been copied to the hard disk.\n"
				"Please try again.\n",
				i);
			return 0;			/* wrong disk */
		}
		return i;				/* ok */
	}
	printf(
		"The disk you inserted is not part of the kit.\n"
		"Please try again.\n"
		);
	return 0;					/* no id found */
}

/*
 * Install new users.
 */
void
newusr()
{
	static int flag = 0;
	register int n, status, passwd;
	register char *s;
	char homeparent[80], user[80];

	cls(0);
	printf(
"Your COHERENT system initially allows logins by users \"root\" (superuser)\n"
"and \"bin\" (system administrator).  In addition, the password file contains\n"
"special entries for \"remacc\" (to control remote access, e.g. via modem),\n"
"\"daemon\" (the spooler), \"sys\" (to access system information), and\n"
"\"uucp\" (for communication with other COHERENT systems).\n"
"\n"
"If your system has multiple users or allows remote logins, you should assign\n"
"a password to each user.\n"
"\n"
	);
	passwd = yes_no("Do you want to assign passwords to users");
	if (passwd) {
		printf("You must enter each password twice.\n");
		if (yes_no("Do you want to assign a password for user \"root\""))
			sys("passwd root", S_NONFATAL);
		if (yes_no("Do you want to assign a remote access password"))
			sys("passwd remacc", S_NONFATAL);
		if (yes_no("Do you want to assign a password for user \"bin\""))
			sys("passwd bin", S_NONFATAL);
		if (yes_no("Do you want to assign a password for user \"uucp\""))
			sys("passwd uucp", S_NONFATAL);
	}
	printf(
"\nYou should create a login for each additional user of your system.\n"
		);
	for (n = 0; ;) {
		if (!yes_no("Do you want to create another login"))
			break;
		if (++n == 1) {
			printf(
"You must specify a login name, a full name and a shell for each user.\n"
"Joe Smith might have login name \"joe\" and full name \"Joseph H. Smith.\"\n"
"His home directory would be in \"/usr\" by default, namely \"/usr/joe\".\n"
"Do not type quotation marks around the names you enter.\n"
				);
			if (yes_no("Do you want home directories in \"/usr\"")) 
				strcpy(homeparent, "/usr");
			else {
again:
				s = get_line("Where do you want home directories?");
				if (*s != '/') {
					printf(
"Please enter a name beginning with '/', such as \"/u\".\n"
						);
					goto again;
				} else
					strcpy(homeparent, s);
			}
			if ((status = is_dir(homeparent)) == -1) {
				printf("%s is not a directory, try again.\n",
					homeparent);
				goto again;
			} else if (status == 0) {
				sprintf(cmd, "/bin/mkdir -r %s", homeparent);
				if (sys(cmd, S_NONFATAL) != 0)
					goto again;
			}
		}
		s = get_line("Login name:");
		strcpy(user, s);
		sprintf(cmd, "/etc/newusr %s ", s);
		s = get_line("Full name:");
		sprintf(&cmd[strlen(cmd)], "\"%s\" %s", s, homeparent);
		if (flag == 0) {
			++flag;		/* print only first time through */
			printf(
"COHERENT includes two different command line interpreters, or shells.\n"
"A command line interpreter is a program which reads and executes each\n"
"command which the user types.  The available command line interpreters\n"
"are the Bourne shell (/bin/sh) and the Korn shell (/usr/bin/ksh).\n"
"Use the Bourne shell if you are not sure which shell to use.\n"
"After you have finished installing COHERENT, you can change the shell\n"
"for any user by editing the password file /etc/passwd.\n"
				);
		}
		for (s = NULL; s == NULL; ) {
			if (yes_no("Do you want to user %s to use the Bourne shell (/bin/sh)",
					user))
				s = "/bin/sh";
			else if (yes_no("Do you want to user %s to use the Korn shell (/usr/bin/ksh)",
					user))
				s = "/usr/bin/ksh";
			else
				printf("You must specify either the Bourne or Korn shell.\n");
		}
		sprintf(&cmd[strlen(cmd)], " %s", s);
		sys(cmd, S_NONFATAL);
		if (passwd && yes_no("Do you want to assign a password for user \"%s\"", user)) {
			sprintf(cmd, "passwd %s", user);
			sys(cmd, S_NONFATAL);
		}
	}
	printf("\n");
}

/*
 * Set a serial port baud rate.
 * Patch the running COHERENT image and /coherent accordingly.
 */
void
setbaud(port) int port;
{
	register int i, baud;

again:
	printf(
"The COHERENT serial port driver supports the following baud rates:\n"
"	50, 75, 110, 134, 150, 200, 300, 600, 1200,\n"
"	1800, 2000, 2400, 3600, 4800, 7200, 9600, 19200\n"
"Enter the baud rate of your device (or 0 if your baud rate"
		);
	baud = get_int(0, 19200, "is not listed):");
	if (baud == 0)
		return;
	for (i = 1; i <= MAXBAUD; i++)
		if (baudrate[i] == baud)
			break;
	if (i > MAXBAUD) {
		printf("COHERENT does not support baud rate %d.\n", baud);
		goto again;
	}

	/* Patch the running COHERENT for possible subsequent test. */
	sprintf(cmd, "/conf/patch -k /coherent C%dBAUD_=%d", port, i);
	sys(cmd, S_NONFATAL);

	/* Patch /coherent for specified baudrate. */
	sprintf(cmd, "/conf/patch /coherent C%dBAUD_=%d", port, i);
	sys(cmd, S_NONFATAL);
}

/* end of install.c */

