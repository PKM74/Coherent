/*
 * ln.c
 * Make a link.
 */

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

#define	VERSION	"2.2"
#define	USAGE	"Usage:\t%s [-f] oldfile newfile\n\t%s [-f] file ... directory\n"

/* Externals. */
extern	int	errno;
extern	char *	rindex();
extern 	char *	strcpy();
extern	int	strlen();

/* Globals. */
char		*argv0;
int		fflag;
char		namebuf[200];

/* Functions. */
char *	basename();
int	ln();
void	usage();

main(argc, argv) int argc; char *argv[];
{
	register int i, estat;
	struct	stat	sb;
	char *lname, *s;

	argv0 = *argv;
	if (argc > 1 && argv[1][0]=='-') {
		for (s = &argv[1][1]; *s; s++) {
			switch (*s) {
			case 'V':
				fprintf(stderr, "%s: V%s\n", argv0, VERSION);
				break;
			case 'f':
				++fflag;
				break;
			default:
				usage();
			}
			--argc;
			++argv;
		}
	}
	if (argc < 3)
		usage();

	lname = argv[argc-1];
	if (stat(lname, &sb)>=0 && (sb.st_mode&S_IFMT)==S_IFDIR) {
		/* The last arg is a directory. */
		strcpy(namebuf, lname);
		s = &namebuf[strlen(namebuf)];
		*s++ = '/';
		for (estat = 0, i=1; i<argc-1; i++) {
			strcpy(s, basename(argv[i]));
			estat |= ln(argv[i], namebuf);
		}
		exit(estat);
	} else if (argc > 3)
		usage();
	else
		exit(ln(argv[1], argv[2]));
}

/*
 * Do the link and check for errors.
 * Try to force the link if the '-f' is specified.
 * Return 0 on success, 1 on failure.
 */
int
ln(n1, n2) char *n1, *n2;
{
	if (link(n1, n2) == 0)
		return 0;
	if (fflag && errno==EEXIST && force(n1, n2) == 0)
		return 0;
	perror(argv0);
	return 1;
}

/*
 * The link failed, try to force it while avoiding:
 *	directory unlink
 *	unlinking source == destination
 *	unlinking destination when different device
 * Return 0 on success, -1 on failure.
 */
int
force(n1, n2) char *n1, *n2;
{
	struct stat sb1, sb2;

	if (stat(n2, &sb2) < 0)
		return -1;		/* e.g. n2 in nonexistent directory */
	if ((sb2.st_mode&S_IFMT) == S_IFDIR) {
		errno = EISDIR;
		return -1;		/* n2 is a directory */
	} else if (stat(n1, &sb1) >= 0) {
		if (sb1.st_dev != sb2.st_dev) {
			errno = EXDEV;
			return -1;	/* cross device link */
		}
		if (sb1.st_ino == sb2.st_ino)
			return 0;	/* n1 and n2 are already linked, ok */
	}
	if (unlink(n2) < 0)
		return -1;		/* cannot unlink */
	return link(n1, n2);
}

/*
 * Find the part of a name past the last '/'.
 */
char *
basename(n) register char *n;
{
	register char *s;

	if ((s = rindex(n, '/')) == NULL)
		return n;
	return ++s;
}

void
usage()
{
	fprintf(stderr, USAGE, argv0, argv0);
	exit(1);
}

/* end of ln.c */
