/*
 * Unmount a filesystem
 */

char helpmessage[] = "\
\
umount -- unmount file system\n\
Usage:	/etc/umount special\n\
The block special file 'special' previously mounted by '/etc/mount'\n\
on 'directory' is unmounted and the previous contents of 'directory'\n\
are once again accessible.\n\
A file system can only be unmounted if all its files are closed.\n\
\
";

#include <stdio.h>
#include <mtab.h>
#include <mnttab.h>
#include <errno.h>

char	mtabf[] = "/etc/mtab";
char	mnttabf[] = "/etc/mnttab";

struct	mtab	mtab;
struct	mnttab	mnttab;
struct	mtab	zmtab;
struct	mnttab	zmnttab;
char	special[MNAMSIZ];

main(argc, argv)
char *argv[];
{
	register FILE *fp;

	if (argc != 2)
		usage();
	if (umount(argv[1]) < 0)
		merror(argv[1]);
	if ((fp = fopen(mnttabf, "r+w")) != NULL) {
		mcopy(argv[1], special);
		while (fread(&mnttab, sizeof(mnttab), 1, fp) == 1)
			if (mnttab.mt_dev[0] != '\0'
			  &&  strncmp(mnttab.mt_filsys, special, MNTNSIZ)==0) {
				fseek(fp, (long)(-sizeof(mnttab)), 1);
				if (fwrite(&zmnttab,sizeof(zmnttab),1,fp) != 1)
					merror(mnttabf);
				break;
			}
	}
	if ((fp = fopen(mtabf, "r+w")) != NULL) {
		mcopy(argv[1], special);
		while (fread(&mtab, sizeof(mtab), 1, fp) == 1)
			if (mtab.mt_name[0] != '\0'
			  &&  strncmp(mtab.mt_special, special, MNAMSIZ)==0) {
				fseek(fp, (long)(-sizeof(mtab)), 1);
				if (fwrite(&zmtab, sizeof(zmtab), 1, fp) != 1)
					merror(mtabf);
				break;
			}
	}
	return (0);
}

usage()
{
	fprintf(stderr, helpmessage);
	exit (1);
}

merror(f)
char *f;
{
	register int err;
	extern int errno;

	err = errno;
	fprintf(stderr, "umount: %r", &f);
	if (err > 0 && err < sys_nerr)
		fprintf(stderr, ": %s\n", sys_errlist[err]);
	else
		fprintf(stderr, ": unrecognized error: %d\n", err);
	exit(1);
}

/*
 * Copy special pathname (stripped of
 * leading directories) into a fixed
 * size buffer.
 */
mcopy(ms, buf)
char *ms, *buf;
{
	register char *p1, *p2;

	for (p1=p2=ms; *p1 != '\0'; )
		if (*p1++ == '/')
			p2 = p1;
	p1 = buf;
	while (*p1++ = *p2++)
		;
}
