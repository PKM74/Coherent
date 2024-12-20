/* Generated from Space.spc on Wed Aug 10 15:43:20 1994 CDT */
/*
 * Configurable information for the patch driver.
 */
#define __KERNEL__	1

#include <sys/patch.h>
#include <sys/compat.h>
#include <sys/con.h>
#include <kernel/param.h>

/*
 * These devices need deferred startup during installation.
 */
extern CON	atcon;	/* IDE hard disk. */
extern CON	sdcon;	/* Adaptec hard disk. */
extern CON	sscon;	/* Seagate/Future Domain hard disk. */
extern CON	scsicon;	/* Hai SCSI driver. */

#define PATCHABLE_VAR(var)	{ STRING(var), &(var), sizeof(var) }
#define PATCHABLE_CON(var)	{ STRING(var), &(var) }

extern unsigned long _bar;
extern int ronflag;
extern unsigned long _entry;
extern int kb_lang;
extern unsigned int NSDRIVE;
extern unsigned int SS_INT;
extern unsigned int SS_BASE;
extern int AHA_SD_HDS;
extern int AHA_SD_SPT;
extern int AHA_SDDMA;
extern int AHA_SDIRQ;
extern int AHA_SDBASE;
extern unsigned short HAI_AHADMA;
extern unsigned short HAI_AHAINTR;
extern unsigned short HAI_AHABASE;
extern int HAI_SD_HDS;
extern int HAI_SD_SPT;
extern int HAI_TAPE;
extern int HAI_DISK;
extern int ATSREG;
extern short at_drive_ct;
extern int fl_dsk_ch_prob;

struct patchVarInternal	patchVarTable [] = {
	PATCHABLE_VAR(_bar),
	PATCHABLE_VAR(ronflag),
	PATCHABLE_VAR(_entry),
	PATCHABLE_VAR(kb_lang),
	PATCHABLE_VAR(NSDRIVE),
	PATCHABLE_VAR(SS_INT),
	PATCHABLE_VAR(SS_BASE),
	PATCHABLE_VAR(AHA_SD_HDS),
	PATCHABLE_VAR(AHA_SD_SPT),
	PATCHABLE_VAR(AHA_SDDMA),
	PATCHABLE_VAR(AHA_SDIRQ),
	PATCHABLE_VAR(AHA_SDBASE),
	PATCHABLE_VAR(HAI_AHADMA),
	PATCHABLE_VAR(HAI_AHAINTR),
	PATCHABLE_VAR(HAI_AHABASE),
	PATCHABLE_VAR(HAI_SD_HDS),
	PATCHABLE_VAR(HAI_SD_SPT),
	PATCHABLE_VAR(HAI_DISK),
	PATCHABLE_VAR(HAI_TAPE),
	PATCHABLE_VAR(ATSREG),
	PATCHABLE_VAR(at_drive_ct),
	PATCHABLE_VAR(fl_dsk_ch_prob)
};

int	patchVarCount = sizeof(patchVarTable)/sizeof(patchVarTable[0]);

struct patchConInternal	patchConTable [] = {
	PATCHABLE_CON(atcon),
	PATCHABLE_CON(sdcon),
	PATCHABLE_CON(sscon),
	PATCHABLE_CON(scsicon)
};

int	patchConCount = sizeof(patchConTable)/sizeof(patchConTable[0]);
