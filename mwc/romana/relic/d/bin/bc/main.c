#include <stdio.h>
#include <path.h>
#include "bc.h"

#define LIBNAME "lib.b"

main(argc, argv)
register int	argc;
register char	*argv[];
{
	register FILE	*fp;

	init();
	++argv;
	if (--argc > 0 && **argv == '-')
		if (strcmp(*argv, "-l") == 0
		 || strcmp(*argv, "-L") == 0) {
			static char lib[] = LIBNAME;
			register char *lp;
			extern char *getenv();
			lp = (lp=getenv("LIBPATH")) ? lp : DEFLIBPATH;
			if ((lp = path(lp, lib, AREAD)) == 0
			 || (fp = fopen(lp, "r")) == 0)
				die("Can't find %s", lib);
			scan(fp, lib, 1);
			--argc;
			++argv;
		} else
			usage();
	while (--argc >= 0) {
		fp = fopen(*argv, "r");
		if (fp == NULL)
			die("Can't open %s", *argv);
		scan(fp, *argv, 1);
		++argv;
	}
	scan(stdin, NULL, 0);
	return (0);
}

scan(fp, fnm, lno)
FILE	*fp;
char	*fnm;
int	lno;
{
	infile = fp;
	infnam = fnm;
	inline = lno;
	yyparse();
	fclose(fp);
}

die(str)
char	*str;
{
	fprintf(stderr, "bc: %r\n", &str);
	exit(1);
}

usage()
{
	fprintf(stderr, "Usage: bc [-l] file ... file\n");
	exit(1);
}