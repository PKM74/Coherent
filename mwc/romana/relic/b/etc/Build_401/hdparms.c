/*
 * File:	hdparms.c
 *
 * Last Hacked:	06/25/92
 *
 * Purpose:	display and modify hard drive parameters
 *
 *	Called by "mkdev" command or usable as stand-alone.
 *	Non-build version expects driver in /tmp/drv, which is where mkdev
 *	leaves it.
 *
 * Usage:	hdparms [-bfrs] devname ...
 *
 * Options:
 *	-b	Use special processing when invoked from /etc/build
 *	-f	Future Domain SCSI
 *	-r	Specified device controls root partition (implies -b)
 *	-s	Seagate SCSI
 *
 * $Log:	hdparms.c,v $
 * Revision 1.1  92/08/14  08:28:40  bin
 * Initial revision
 * 
 * Revision 1.4  92/01/17  11:31:11  bin
 * another hal update... looks like the final 321 ship version
 * 
 * Revision 1.2  91/06/28  07:29:47  bin
 * updated by hal
 * 
 * Revision 1.4  91/06/27  13:38:07  hal
 * Steve-style printf call for long messages.
 * Drop calculated default parameters.
 * 
 * Revision 1.3  91/06/27  13:21:24  hal
 * Fix exit value if parameters are NOT changed.
 * Use "success" not "exitval".
 * 
 * Revision 1.2  91/06/03  04:32:58  hal
 * Patch drv_parm_ table for ss driver.
 * 
 * Revision 1.1  91/06/02  13:26:01  hal
 * Initial revision
 * 
 */

/*
 * Includes.
 */
#include <stdio.h>
#include <sys/fdisk.h>
#include <sys/hdioctl.h>
#include <sys/stat.h>
#include "build0.h"

/*
 * Definitions.
 *	Constants.
 *	Macros with argument lists.
 *	Typedefs.
 *	Enums.
 */
#define	USAGEMSG	"Usage:\t/etc/hdparms [ -bfrs ] device...\n"
#define OPENMODE	2	/* Default open mode: read/write. */
#define BUFLEN		40
#define DEV_SCSI_ID(dev)	((dev >> 4) & 0x0007)
#define	VERSION		"V2.0"		/* version number */

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long ulong;

typedef	struct {			/* "ss" kludge coutesy of Hal */
	uint	ncyl;			/* # of cylinders */
	uchar	nhead;			/* # of heads */
	uchar	nspt;			/* # of sectors/track */
} drv_parm_type;

/*
 * Functions.
 *	Import Functions.
 *	Export Functions.
 *	Local Functions.
 */
#if DEFAULT_PARMS
static void cam_parms();
static void fd_parms();
#endif
static void getuint();
static int hdparms();

/*
 * Global Data.
 *	Import Variables.
 *	Export Variables.
 *	Local Variables.
 */
static int bflag;	/* 1 for call via /etc/build */
static int fflag;	/* 1 for Future Domain SCSI */
static int rflag;	/* 1 for rootdev */
static int sflag;	/* 1 for Seagate SCSI */

/*
 * main()
 */
main(argc, argv)
int argc;
char *argv[];
{
	uchar *s;
	int success = 1;

	argv0 = argv[0];
	usagemsg = USAGEMSG;
	if (argc > 1 && argv[1][0] == '-') {
		for (s = &argv[1][1]; *s; ++s) {
			switch(*s) {
			case 'b':
				++bflag;
				break;
			case 'f':
				++fflag;
				break;
			case 'r':
				++rflag;
				++bflag;
				break;
			case 's':
				++sflag;
				break;
			default:
				usage();
			}
		}
		--argc;
		++argv;
	}

	while (--argc > 0) {
		success &= hdparms(argv[1]);
		++argv;
	}

	/*
	 * Exit with nonzero value if any call to hdparms() failed.
	 */
	exit(success == 0);
}

/*
 * hdparms()
 *
 * Return 0 if error occurred, else 1.
 */
static int hdparms(devname)
uchar *devname;
{
	int fd;
	hdparm_t parms;
	int ret = 0;
	struct stat dvstat;
	int s_id;		/* SCSI id */
	uint pl;		/* offset of device entry in drv_parm_ */
	uchar ss_patch[60];
	uchar * drv = "/drv/ss";
	uchar cmd[80];
	FILE * fp;

	if ((fd = open(devname, OPENMODE)) < 0) {
		printf("Can't open %s.\n", devname);
		goto noclose_fd;
	}

	if (sflag || fflag)
		if (stat(devname, &dvstat) != 0) {
			printf("Can't stat %s.\n", devname);
			goto close_fd;
		} else {
			s_id = DEV_SCSI_ID(dvstat.st_rdev);
			pl = sizeof(drv_parm_type) * s_id;
		}

	if (ioctl(fd, HDGETA, (char *)(&parms)) == -1)
		printf("Can't get parameters for %s.\n", devname);
	else {
		uint ncyls, nheads, nspt;
		ulong nsectors;

		ncyls = (parms.ncyl[1] << 8) | parms.ncyl[0];
		nheads = parms.nhead;
		nspt = parms.nspt;
		nsectors = (ulong)ncyls * (ulong)nheads * (ulong)nspt;

printf("\nHere are the current parameters for SCSI device %d:\n", s_id);
printf("Number of cylinders = %d\n", ncyls);
printf("Number of heads = %d\n", nheads);
printf("Number of sectors per track = %d\n", nspt);

printf(
"\nIf the values above do not agree with those used by your host adapter's BIOS"
"\nprogramming, you will not be able to boot COHERENT from this hard drive.\n");

		if (ncyls > 1024) {
printf(
"\nThis device has more than 1024 cylinders.  In order to use the entire drive"
"\nfrom COHERENT, and possibly to be compatible with other operating systems,"
"\nyou will need to enter a set of translation-mode parameters.  Enter the"
"\nparameters your BIOS uses.  You can accept the default values shown by"
"\npressing <Enter> at each prompt.\n\n");
#if DEFAULT_PARMS
			if (fflag)
				fd_parms(&ncyls, &nheads, &nspt);
			else
				cam_parms(&ncyls, &nheads, &nspt);
#endif				
		}

		if (yes_no("Do you want to modify drive parameters")) {
			getuint(&ncyls, "Number of cylinders");
			getuint(&nheads, "Number of heads");
			getuint(&nspt, "Number of sectors per track");

			parms.ncyl[1] = ncyls >> 8;
			parms.ncyl[0] = ncyls & 0xFF;
			parms.nhead = nheads;
			parms.nspt = nspt;

			if (ioctl(fd, HDSETA, (char *)(&parms)) == -1)
printf("Couldn't write new parameters for %s.\n", devname);
			else {
printf("New parameters written for %s.\n", devname);
				ret = 1;
			}

			/* Prepare to patch "ss" driver. */
			if (sflag || fflag) {
				sprintf(ss_patch,
#if _I386
					"drv_parm+%d=%d drv_parm+%d=0x%04x",
#else
					"drv_parm_+%d=%d drv_parm_+%d=0x%04x",
#endif
					pl, ncyls, pl+sizeof(int),
					(nspt<<8) + nheads);
				/*  Write PATCHFILE which is run by build. */
				if (bflag) {
					fp = fopen(PATCHFILE, "a");
#if !_I386
fprintf(fp, "/conf/patch /mnt%s %s\n", drv, ss_patch);
					if (rflag)
#endif
fprintf(fp, "/conf/patch /mnt/coherent %s\n", ss_patch);
					fclose(fp);

				} else { /* patch driver */
fprintf(cmd, "/conf/patch /tmp/drv/ss %s\n", ss_patch);
					sys(cmd, S_FATAL);
printf("Parameters patched in /tmp/drv/ss\n");
				}
			}

		} else
			ret = 1;
	}

close_fd:
	close(fd);

noclose_fd:
	return ret;
}

/*
 * getuint()
 *
 * get unsigned integer value - display prompt with default value
 */
static void getuint(np, prompt)
uint * np;
uchar * prompt;
{
	uchar buf[BUFLEN];

	printf("%s [%d]: ", prompt, *np);
	fgets(buf, BUFLEN, stdin);
	sscanf(buf, "%d", np);
}

#if DEFAULT_PARMS
/*
 * cam_parms()
 *
 * Use CAM algorithm to compute new drive parameters which will keep
 * number of cylinders under 1024.
 */
static void cam_parms(p_ncyls, p_nheads, p_nspt)
uint * p_ncyls, * p_nheads, * p_nspt;
{
	ulong capacity, ncyls, nheads, nspt;
	ulong ntracks, nsph, nspc;

	capacity = (ulong)*p_ncyls * (ulong)*p_nheads * (ulong)*p_nspt;
	if (capacity == 0L)
		goto frotz;
	ncyls = 1024L;
	nspt = 62L;
	nsph = ncyls * nspt;
	nheads = capacity / nsph;

	if (capacity % nsph) {
		nheads++;
		ntracks = ncyls * nheads;
		nspt = capacity / ntracks;

		if (capacity % ntracks) {
			nspt++;
			nspc = nheads * nspt;
			ncyls = capacity / nspc;
		}
	}

	*p_ncyls = ncyls;
	*p_nheads = nheads;
	*p_nspt = nspt;
frotz:
	return;
}

/*
 * fd_parms()
 *
 * Use Future Domain algorithm to compute new drive parameters which will
 * keep number of cylinders under 1024.
 */
static void fd_parms(p_ncyls, p_nheads, p_nspt)
uint * p_ncyls, * p_nheads, * p_nspt;
{
	ulong capacity, ncyls, nheads, nspt;
	ulong ntracks, foo, nspc;

	capacity = (ulong)*p_ncyls * (ulong)*p_nheads * (ulong)*p_nspt;
	if (capacity == 0L)
		goto frotz;
	nspt = 17;	/* first try 17 spt */
	while (1) {
		foo = capacity / 1024;
		nheads = (foo / nspt) + 1;
		nspc = nheads * nspt;
		ncyls = capacity / nspc;
		ntracks = nheads * ncyls;
		if (ntracks < 32768L)
			break;
		if (nspt == 17)
			nspt = 34;	/* after 17, try 34 spt */
		else if (nspt == 34)
			nspt = 63;	/* after 34, try 63 spt */
		else
			break;
	}

	*p_ncyls = ncyls;
	*p_nheads = nheads;
	*p_nspt = nspt;
frotz:
	return;
}
#endif
