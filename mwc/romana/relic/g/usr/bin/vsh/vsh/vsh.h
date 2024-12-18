/*
 *      HEADER file for vsh
 *
 *      Copyright (c) 1990-93 by Udo Munk
 */

#include <sys/types.h>
#ifndef POSDIR
#include <sys/dir.h>
#else
#include <dirent.h>
#endif
#include <sys/stat.h>

/*
 *	system dependent defines
 */
#ifdef AIX
#define SIZE_OF_DIR	sizeof(struct dirent)
#define FILE_NAME_LEN	_D_NAME_MAX
#endif
#if defined(SYSV4) || defined(SCO32) || defined(COHERENT)
#define SIZE_OF_DIR	(sizeof(struct dirent) + MAXNAMLEN)
#define FILE_NAME_LEN	MAXNAMLEN
#endif

/*
 *	define several maximum sizes
 */
#define MAXFILES        1000            /* number of files/directory */
#define MAXCMD          40              /* number of entrys in command history */
#define MAXTMP          256             /* size of the working buffers */
#define DSTKSIZE        10              /* size of directory stack */
#define MAXEXCMD        40              /* size of configurable commands */
#define MAXACT		40		/* number of configurable file actions */
#define MAXACTSIZE	41		/* size of file actions */

/*
 *	defines for used filenames
 */
#define VSHFILE		".vsh"		/* dot file to store environment */
#define VSHTFILE	".vsh$"		/* temporary file */
#define VSHLOCK         "/tmp/vshlock"  /* lock file */
#define UXPASSWD        "/etc/passwd"   /* password file */

/*
 *	defines for used commands
 */
#define CMD_SH		"/bin/sh"	/* default shell */
#define CMD_EDITOR	"vi"		/* default editor */
#ifndef COHERENT
#define CMD_PRINT	"lp"		/* default print spooler */
#define CMD_VIEW	"pg"		/* default file viewer */
#else
#define CMD_PRINT	"lpr -B"
#define CMD_VIEW	"more"
#endif
#define CMD_MAN		"man"		/* UNXI command: show manual entry */
#define CMD_MKDIR	"/bin/mkdir"	/* UNIX command: create directory */
#define CMD_RMDIR	"/bin/rmdir"	/* UNIX command: remove directory */
#define CMD_RM		"/bin/rm"	/* UNIX command: remove file */
#define OPT_RM		"-rf"		/* option rm: remove all recursive */
#define CMD_MV		"/bin/mv"	/* UNIX command: move file */
#define CMD_CP		"/bin/cp"	/* UNIX command: copy file */
#define CMD_FILE	"file"		/* UNIX command: guess type of file */
#define	CMD_FIND	"find"		/* UNIX command: find files */
#ifndef COHERENT
#define OPT_FIND	"-depth -print"	/* option find: search recursive */
#else
#define OPT_FIND	"-print"
#endif
#define CMD_CPIO	"cpio"		/* UNIX command: cpio */
#define OPT_CPIO	"-pdlm"		/* option cpio: pass files to dir */

/*
 *	predefined default file actions
 */
#define ACTION1		"[Mm]akefile:make"
#define ACTION2		"*.mk:make -f %F"
#define ACTION3		"*.sh:sh %F"
#define ACTION4		"*.c:cc -c -O %F"
#define ACTION5		"*.sc:sc %F"
#ifndef COHERENT
#define ACTION6		"*.a:ar tv %F | pg"
#else
#define ACTION6		"*.a:ar tv %F | more"
#endif
#ifndef COHERENT
#define ACTION7		"*.[1-9]:nroff -man %F | pg"
#else
#define ACTION7		"*.[1-9]:nroff -man %F | more"
#endif
#define ACTION8		"*.tar.F:fcat %F | tar xvf -"
#ifndef COHERENT
#define ACTION9		"*.F:fcat %F | pg"
#else
#define ACTION9		"*.F:fcat %F | more"
#endif
#define ACTION10	"*.tar.Z:zcat %F | tar xvf -"
#ifndef COHERENT
#define ACTION11	"*.Z:zcat %F | pg"
#else
#define ACTION11	"*.Z:zcat %F | more"
#endif
#define ACTION12	"*.tar.z:zcat %F | tar xvf -"
#ifndef COHERENT
#define ACTION13	"*.z:zcat %F | pg"
#else
#define ACTION13	"*.z:zcat %F | more"
#endif

/*
 *	error numbers for functions sf_error() and tf_error()
 */
#define ERR_MV		1		/* move */
#define ERR_RM		2		/* delete */
#define ERR_CP		3		/* copy */
#define ERR_REN		4		/* rename */
#define ERR_CHANGE	5		/* touch, chmod, chown */
#define ERR_MKDIR	6		/* mkdir */

/*
 *	just for systems where a min macro isn't already defined
 */
#ifndef min
#define min(a,b) ((a) < (b)) ? (a) : (b)
#endif

/*
 *      structure for the list of files
 */
struct filelist {
#ifndef POSDIR
	struct direct *f_dir;           /* directory entry */
#else
	struct dirent *f_dir;
#endif
	struct stat *f_stat;            /* file status */
	int f_mflag;                    /* file tagg flag */
};

/*
 *	some macros to get the file type from a file of the file list
 */
#define is_file(i)      ((files[i].f_stat->st_mode & S_IFMT) == S_IFREG)
#define is_dir(i)       ((files[i].f_stat->st_mode & S_IFMT) == S_IFDIR)

/*
 *	structure of a directory stack entry
 */
struct dstack {
	char *dname;			/* name of directory */
	int  afile;			/* number of file where cursor is on */
};
