/*
 * Standard I/O Library Internals
 * Open file
 */

#include <stdio.h>

#define	CRMODE	0666	/* default access permissions on create */

FILE *
_fopen(name, type, fp, fd)
char	*name,
	*type;
register FILE	*fp;
register int	fd;
{
	extern	int	_fginit();
	extern	int	_fpinit();
	register int	mode = 1;
	int	truncate = 0,
		append = 0;
	{	register char	c = *type++;
		char	cn = 'r';

		if (c=='r') {
			mode = 0;
			cn = 'w';
		} else if (c=='w')
			truncate++;
		else if (c=='a')
			append++;
		else
			return (NULL);
		if ((c=*type)=='\0' || c=='b')
			;
		else if (c=='+' || c==cn)
			mode = 2;
		else
			return (NULL);
	}
	if (fd<0 && !truncate)
		fd = open(name, mode);
	if (fd<0 && (truncate || append)
	 && (fd=creat(name, CRMODE))>=0 && mode!=1) {
		close(fd);
		fd = open(name, mode);
	}
	if (fd<0)
		return (NULL);
	if (append)
		lseek(fd, 0L, SEEK_END);
	if (fp==NULL && (fp = (FILE *) malloc(sizeof(FILE)))==NULL) {
		close(fd);
		return (NULL);
	}
	fp->_ff = _FINUSE;
	fp->_bp = fp->_cp = fp->_dp = NULL;
	fp->_cc = 0;
	fp->_gt = &_fginit;
	fp->_pt = &_fpinit;
	fp->_fd = fd;
	return (fp);
}
