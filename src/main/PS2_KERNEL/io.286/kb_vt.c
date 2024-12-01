/* (-lgl
 * 	COHERENT Driver Kit Version 1.1.0
 * 	Copyright (c) 1982, 1990 by Mark Williams Company.
 * 	All rights reserved. May not be copied without permission.
 -lgl) */
/*
 * Keyboard/display driver.
 * Coherent, IBM PC/XT/AT.
 */

/*
 *	virtual terminal additions Copyright 1991, 1992 Todd Fleming
 *	6/91, 1/92
 *
 *	Todd Fleming
 *	1991 Mountainside Drive
 *	Blacksburg, VA 24060
 */

/* Notes on virtual terminal additions:
 * 
 * Console minor dev # = 0, virtual terminal minor #'s = 1 - 10
 * User switches terminals w/CTRL + F1-10
 *
 *    Console is parasitic. Its output is always directed to the current
 * onscreen terminal and it steels its input from the current terminal.
 *
 *   NEVER allow the console to be enabled via /etc/ttys while virtual terminals
 * are enabled. It is only used in single-user mode and for system messages.
 * (When in single-user mode, the kernal automatically uses the console
 * even though it is disabled in /etc/ttys)
 *
 *    Each virtual terminal has its own resources for the screen buffer,
 * toggle key state (num, caps, and scroll lock), alternate keypad state,
 * keyboard lock state, and ESC-string parsing state. Except for input, 
 * all virtual terminals are fully functional at all times. The 
 * virtual-terminal switching is transparent to the kernal and user programs.
 *
 *    Loadable key tables are not supported. (This feature was unavailable in 
 * the origional source code for kb). Loadable function keys are supported.
 */

#include <sys/coherent.h>
#include <sys/i8086.h>
#include <sys/con.h>
#include <sys/devices.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/tty.h>
#include <signal.h>
#include <sys/sched.h>
#include <sys/silo.h>

#define NVIRTERMS 4			/* Number of virtual terminals */
					/* (Does not include console) */

#define	SPC	0376			/* Special encoding */
#define XXX	0377			/* Non-character */
#define	KBDATA	0x60			/* Keyboard data */
#define	KBCTRL	0x61			/* Keyboard control */
#define	KBFLAG	0x80			/* Keyboard reset flag */
#define	LEDCMD	0xED			/* status indicator command */
#define	KBACK	0xFA			/* status indicator acknowledge */
#define	EXTENDED1 0xE1			/* extended key seq initiator */

#define	KEYUP	0x80			/* Key up change */
#define	KEYSC	0x7F			/* Key scan code mask */
#define	LSHIFT	0x2A-1			/* Left shift key */
#define LSHIFTA 0x2B-1			/* Alternate left-shift key */
#define	RSHIFT	0x36-1			/* Right shift key */
#define	CTRL	0x1D-1			/* Control key */
/*-- #define	CAPLOCK	0x1D-1	--*/		/* Control key */
#define	ALT	0x38-1			/* Alt key */
#define	CAPLOCK	0x3A-1			/* Caps lock key */
/*-- #define	CTRL	0x3A-1	--*/		/* Caps lock key */
#define	NUMLOCK	0x45-1			/* Numeric lock key */
#define	DELETE	0x53-1			/* Del, as in CTRL-ALT-DEL */
#define BACKSP	0x0E-1			/* Back space */
#define SCRLOCK	0x46-1			/* Scroll lock */

/* Shift flags */
#define	SRS	0x01			/* Right shift key on */
#define	SLS	0x02			/* Left shift key on */
#define CTS	0x04			/* Ctrl key on */
#define ALS	0x08			/* Alt key on */
#define CPLS	0x10			/* Caps lock on */
#define NMLS	0x20			/* Num lock on */
#define AKPS	0x40			/* Alternate keypad shift */
#define SHFT	0x80			/* Shift key flag */

/* Function key information */
#define	NFKEY	20			/* Number of settable functions */
#define	NFCHAR	150			/* Number of characters settable */
#define	NFBUF	(NFKEY*2+NFCHAR+1)	/* Size of buffer */

/*
 * Functions.
 */
int	isrint();
int	istime();
void	isbatch();
int	mmstart();
int	isopen();
int	isclose();
int	isread();
int	mmwrite();
int	isioctl();
void	mmwatch();
int	isload();
int	isuload();
int	ispoll();
int	nulldev();
int	nonedev();

/* 
 * virtual terminal stuff
 */

extern int mmcurvirterm;		/* current on-screen terminal */
extern int mmvirtermsinitialized;	/* initialization flag */
extern mminitvirterm();			/* starter function */
extern mmchangevirterm();		/* mm's half of this function */
kbchangeterm();				/* kb's part */
unsigned kbAKPShold[NVIRTERMS+1];	/* Alternate keypad states */
unsigned kbNUMLCKhold[NVIRTERMS+1];	/* NumLock states */
unsigned kbCAPLCKhold[NVIRTERMS+1];	/* CapLock states */

/*
 * Configuration table.
 */
CON iscon ={
	DFCHR|DFPOL,			/* Flags */
	KB_MAJOR,			/* Major index */
	isopen,				/* Open */
	isclose,			/* Close */
	nulldev,			/* Block */
	isread,				/* Read */
	mmwrite,			/* Write */
	isioctl,			/* Ioctl */
	nulldev,			/* Powerfail */
	mmwatch,			/* Timeout */
	isload,				/* Load */
	isuload,			/* Unload */
	ispoll				/* Poll */
};

/*
 * Flag indicating turbo machine.
 */
int isturbo = 0;

/*
 * Terminal structures.
 * istty[0]= console
 * 1-10    = virtual terminals
 */
TTY	istty[NVIRTERMS+1] = {
	{{0}, {0}, 0,  mmstart, NULL, 0, 0},
	{{0}, {0}, 1,  mmstart, NULL, 0, 0},
	{{0}, {0}, 2,  mmstart, NULL, 0, 0},
	{{0}, {0}, 3,  mmstart, NULL, 0, 0},
	{{0}, {0}, 4,  mmstart, NULL, 0, 0},
#if 0
	{{0}, {0}, 5,  mmstart, NULL, 0, 0},
	{{0}, {0}, 6,  mmstart, NULL, 0, 0},
	{{0}, {0}, 7,  mmstart, NULL, 0, 0},
	{{0}, {0}, 8,  mmstart, NULL, 0, 0},
	{{0}, {0}, 9,  mmstart, NULL, 0, 0},
	{{0}, {0}, 10, mmstart, NULL, 0, 0}
#endif
};
silo_t	in_silo[NVIRTERMS+1];

/*
 * State variables.
 */
int		islock[NVIRTERMS+1];	/* Keyboard locked flag */
int		isbusy;			/* Raw input conversion busy */
static	char	shift;			/* Overall shift state */
static	char	scroll[NVIRTERMS+1];	/* Scroll lock state */
static  char	lshift = LSHIFT;	/* Left shift alternate state */
static	char	isfbuf[NFBUF];		/* Function key values */
static	char	*isfval[NFKEY];		/* Function key string pointers */
static	int	ledcmd;			/* LED update command flag */
static	int	extended;		/* extended key scan count */

/*
 * Tables for converting key code to ASCII.
 * lmaptab specifies unshifted conversion,
 * umaptab specifies shifted conversion,
 * smaptab specifies the shift states which are active.
 * An entry of XXX says the key is dead.
 * An entry of SPC requires further processing.
 *
 * Key codes:
 *	ESC .. <- == 1 .. 14
 *	-> .. \n == 15 .. 28
 *	CTRL .. ` == 29 .. 41
 *	^Shift .. PrtSc == 42 .. 55
 * 	ALT .. CapsLock == 56 .. 58
 *	F1 .. F10 == 59 .. 68
 *	NumLock .. Del == 69 .. 83
 */
static unsigned char lmaptab[] ={
	     '\33',  '1',  '2',  '3',  '4',  '5',  '6',		/* 1 - 7 */
	 '7',  '8',  '9',  '0',  '-',  '=', '\b', '\t',		/* 8 - 15 */
	 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',		/* 16 - 23 */
	 'o',  'p',  '[',  ']', '\r',  XXX,  'a',  's',		/* 24 - 31 */
	 'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',		/* 32 - 39 */
	 '\'', '`',  XXX,  '\\',  'z',  'x',  'c',  'v',	/* 40 - 47 */
	 'b',  'n',  'm',  ',',  '.',  '/',  XXX,  '*',		/* 48 - 55 */
	 XXX,  ' ',  XXX,  SPC,  SPC,  SPC,  SPC,  SPC,		/* 56 - 63 */
	 SPC,  SPC,  SPC,  SPC,  SPC,  SPC,  SPC,  SPC,		/* 64 - 71 */
	 SPC,  SPC,  '-',  SPC,  SPC,  SPC,  '+',  SPC,		/* 72 - 79 */
	 SPC,  SPC,  SPC,  SPC					/* 80 - 83 */
};

static unsigned char umaptab[] ={
	     '\33',  '!',  '@',  '#',  '$',  '%',  '^',		/* 1 - 7 */
	 '&',  '*',  '(',  ')',  '_',  '+', '\b', SPC,		/* 8 - 15 */
	 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',		/* 16 - 23 */
	 'O',  'P',  '{',  '}', '\r',  XXX,  'A',  'S',		/* 24 - 31 */
	 'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',		/* 32 - 39 */
	 '"',  '~',  XXX,  '|',  'Z',  'X',  'C',  'V',		/* 40 - 47 */
	 'B',  'N',  'M',  '<',  '>',  '?',  XXX,  '*',		/* 48 - 55 */
	 XXX,  ' ',  XXX,  SPC,  SPC,  SPC,  SPC,  SPC,		/* 56 - 63 */
	 SPC,  SPC,  SPC,  SPC,  SPC,  SPC,  SPC,  SPC,		/* 64 - 71 */
	 SPC,  SPC,  '-',  SPC,  SPC,  SPC,  '+',  SPC,		/* 72 - 79 */
	 SPC,  SPC,  SPC,  SPC					/* 80 - 83 */
};

#define SS0	0			/* No shift */
#define SS1	(SLS|SRS|CTS)		/* Shift, Ctrl */
#define SES	(SLS|SRS)		/* Shift */
#define LET	(SLS|SRS|CPLS|CTS)	/* Shift, Caps, Ctrl */
#define KEY	(SLS|SRS|NMLS|AKPS)	/* Shift, Num, Alt keypad */

static unsigned char smaptab[] ={
	       SS0,  SES,  SS1,  SES,  SES,  SES,  SS1,		/* 1 - 7 */
	 SES,  SES,  SES,  SES,  SS1,  SES,  CTS,  SES,		/* 8 - 15 */
	 LET,  LET,  LET,  LET,  LET,  LET,  LET,  LET,		/* 16 - 23 */
	 LET,  LET,  SS1,  SS1,  CTS, SHFT,  LET,  LET,		/* 24 - 31 */
	 LET,  LET,  LET,  LET,  LET,  LET,  LET,  SES,		/* 32 - 39 */
	 SES,  SS1, SHFT,  SS1,  LET,  LET,  LET,  LET,		/* 40 - 47 */
	 LET,  LET,  LET,  SES,  SES,  SES, SHFT,  SES,		/* 48 - 55 */
	SHFT,  SS1, SHFT,  SS0,  SS0,  SS0,  SS0,  SS0,		/* 56 - 63 */
	 SS0,  SS0,  SS0,  SS0,  SS0, SHFT,  KEY,  KEY,		/* 64 - 71 */
	 KEY,  KEY,  SS0,  KEY,  KEY,  KEY,  SS0,  KEY,		/* 72 - 79 */
	 KEY,  KEY,  KEY,  KEY					/* 80 - 83 */
};

/*
 * Load entry point.
 *  Do reset the keyboard because it gets terribly munged
 *  if you type during the boot.
 */
isload()
{
	register int i;

	/*
	 * Reset keyboard if NOT an XT turbo.
	 */
	if ( ! isturbo ) {
		outb(KBCTRL, 0x0C);		/* Clock low */
		for (i = 10582; --i >= 0; );	/* For 20ms */
		outb(KBCTRL, 0xCC);		/* Clock high */
		for (i = 0; --i != 0; )
			;
		i = inb(KBDATA);
		outb(KBCTRL, 0xCC);			/* Clear keyboard */
		outb(KBCTRL, 0x4D);			/* Enable keyboard */
	}

	/*
	 * Enable mmwatch() invocation every second.
	 */
	drvl[KB_MAJOR].d_time = 1;

	/*
	 * Seize keyboard interrupt.
	 */
	setivec(1, isrint);

	/*
	 * Initiailize video display.
	 */
	for(i=0;i<=NVIRTERMS;i++)	/* initialize console, virterms */
	 mmstart( &istty[i] );
}

/*
 * Unload entry point.
 */
isuload()
{
	clrivec(1);
}

/*
 * Default function key strings (terminated by -1 [\377])
 */
static char *deffuncs[] = {
	"\33[1x\377",	/* F1 */
	"\33[2x\377",	/* F2 */
	"\33[3x\377",	/* F3 */
	"\33[4x\377", 	/* F4 */
	"\33[5x\377",	/* F5 */
	"\33[6x\377",	/* F6 */
	"\33[7x\377",	/* F7 */
	"\33[8x\377",	/* F8 */
	"\33[9x\377",	/* F9 */
	"\33[0x\377",	/* F10 - historical value */
	"\33[1y\377",	/* F11 */
	"\33[2y\377",	/* F12 */
	"\33[3y\377",	/* F13 */
	"\33[4y\377", 	/* F14 */
	"\33[5y\377",	/* F15 */
	"\33[6y\377",	/* F16 */
	"\33[7y\377",	/* F17 */
	"\33[8y\377",	/* F18 */
	"\33[9y\377",	/* F19 */
	"\33[0y\377"	/* F20 */
};

/*
 * Open routine.
 */
isopen(dev)
dev_t dev;
{
	register int s;

	if (minor(dev) > NVIRTERMS) {
		u.u_error = ENXIO;
		return;
	}

	if ((istty[minor(dev)].t_flags&T_EXCL)!=0 && super()==0) {
		u.u_error = ENODEV;
		return;
	}
	ttsetgrp(&istty[minor(dev)], dev);

	s = sphi();
	if ((istty[minor(dev)].t_open)++ == 0)
	{  initkeys();	 /* init function keys */
	   istty[minor(dev)].t_flags = T_CARR;  /* indicate "carrier" */
	   ttopen(&istty[minor(dev)]);
	}
	spl(s);
	updleds();			/* update keyboard status LEDS */
}

/* Init function keys */
initkeys()
{	register int i;
	register char *cp1, *cp2;

	for (i=0; i<NFKEY; i++)
	    isfval[i] = 0;	    /* clear function key buffer */
	cp2 = isfbuf;	      	    /* pointer to key buffer */   
	for (i=0; i<NFKEY; i++)
	{  isfval[i] = cp2;	    /* save pointer to key string */
	   cp1 = deffuncs[i];       /* get init string pointer */
	   while ((*cp2++ = *cp1++) != -1)  /* copy key data */
	     if (cp2 >= &isfbuf[NFBUF-3])   /* overflow? */
	        return;
	}
}

/*
 * Close a tty.
 */
isclose(dev)
{
	register int s;

	s = sphi();
	if (--(istty[minor(dev)].t_open) == 0)
	{	s = sphi();
		ttclose(&istty[minor(dev)]);
	}
	spl(s);
}

/*
 * Read routine.
 */
isread(dev, iop)
dev_t dev;
IO *iop;
{
 dev_t usedev=dev;
	if(minor(usedev)==0 && istty[mmcurvirterm].t_open)
						/* Route input from current */
	 usedev=makedev(2,mmcurvirterm);	/* virtual terminal to console (TBF) */
	ttread(&istty[minor(usedev)], iop, 0);
	if (istty[minor(usedev)].t_oq.cq_cc)
		mmtime(&istty[minor(usedev)]);
}

/*
 * Ioctl routine.
 */
isioctl(dev, com, vec)
dev_t dev;
struct sgttyb *vec;
{
	register int s;

	switch(com) {
	case TIOCSETF:
	case TIOCGETF:
		isfunction(com, (char *)vec);
		return;
	case TIOCSHIFT:   /* switch left-SHIFT and "\" */
		lshift = LSHIFTA;    /* alternate values */
		lmaptab[41] = '\\';
		lmaptab[42] = XXX;
		umaptab[41] = '|';
		umaptab[42] = XXX;
		smaptab[41] = SS1;
		smaptab[42] = SHFT;
		return;
	case TIOCCSHIFT:  /* normal (default) left-SHIFT and "\" */
		lshift = LSHIFT;     /* normal values */
		lmaptab[41] = XXX;
		lmaptab[42] = '\\';
		umaptab[41] = XXX;
		umaptab[42] = '|';
		smaptab[41] = SHFT;
		smaptab[42] = SS1;
		return;
	}
	s = sphi();
	ttioctl(&istty[minor(dev)], com, vec);
	spl(s);
}

/*
 * Set and receive the function keys.
 */
isfunction(c, v)
int c;
char *v;
{
	register char *cp;
	register int i;

	if (c == TIOCGETF) {
		for (cp = isfbuf; cp < &isfbuf[NFBUF]; cp++)
		    putubd(v++, *cp);
	} else {
		for (i=0; i<NFKEY; i++)		/* zap current settings */
			isfval[i] = 0;
		cp = isfbuf;			/* pointer to key buffer */
		for (i=0; i<NFKEY; i++) {
			isfval[i] = cp;	        /* save pointer to key string */
			while ((*cp++ = getubd(v++)) != -1)  /* copy key data */
				if (cp >= &isfbuf[NFBUF-3])  /* overflow? */
					return;
		}
	}
}


/*
 * Poll routine.
 */
ispoll( dev, ev, msec )
dev_t dev;
int ev;
int msec;
{
	if(minor(dev)==0 && istty[mmcurvirterm].t_open)
					/* Route input from current */
	 dev=makedev(2,mmcurvirterm);	/* virtual terminal to console (TBF) */

	/*
	 * Priority polls not supported.
	 */
	ev &= ~POLLPRI;

	/*
	 * Input poll failure.
	 */
	if ( (ev & POLLIN) && (istty[minor(dev)].t_iq.cq_cc == 0) ) {

		if ( msec != 0 )
			pollopen( &istty[minor(dev)].t_ipolls );

		/*
		 * Second look AFTER enabling monitor, avoiding interrupt race.
		 */
		if ( istty[minor(dev)].t_iq.cq_cc == 0 )
			ev &= ~POLLIN;
	}

	return ev;
}

/*
 * Receive interrupt.
 */
isrint()
{
	register int	c;
	register int	s;
	register int	r;
	int	savests;
	int	update_leds = 0;

	/*
	 * Schedule raw input handler if not already active.
	 */
	if ( isbusy == 0 ) {
		if(istty[mmcurvirterm].t_open)	/* only for opened term (TBF) */
		 defer( isbatch, &istty[mmcurvirterm] );
		else				/* use console */
		 defer( isbatch, &istty[0] );
		isbusy = 1;
	}

	/*
	 * Pull character from the data
	 * port. Pulse the KBFLAG in the control
	 * port to reset the data buffer.
	 */
	r = inb(KBDATA) & 0xFF;
	c = inb(KBCTRL);
	outb(KBCTRL, c|KBFLAG);
	outb(KBCTRL, c);
#if	KBDEBUG
	printf("kbd: %d\n", r);			/* print scan code/direction */
#endif
	if (ledcmd) {
		/* ledcmd = 0; */
		if (r == KBACK) {		/* output to status LEDS */
			ledcmd = 0;
			c = scroll[mmcurvirterm] & 1;
			if (shift & NMLS)
				c |= 2;
			if (shift & CPLS)
				c |= 4;
			outb(KBDATA, c);
			return;
		}
		/*return;*/
	}
	if (extended > 0) {			/* if multi-character seq, */
		--extended;			/* ... ignore this char */
		return;
	}
	if (r == EXTENDED1) {			/* ignore extended sequences */
		extended = 5;
		return;
	}
	if (r == 0xFF) {
		return;	/* Overrun */
	}
	c = (r & KEYSC) - 1;
	/*
	 * Check for reset.
	 */
	if ((r&KEYUP) == 0 && c == DELETE && (shift&(CTS|ALS)) == (CTS|ALS))
		boot();

	/*
	 * Track "shift" keys.
	 */
	s = smaptab[c];
	if (s&SHFT) {
		if (r&KEYUP) {			/* "shift" released */
			if (c == RSHIFT)
				shift &= ~SRS;
			else if (c == lshift)
				shift &= ~SLS;
			else if (c == CTRL)
				shift &= ~CTS;
			else if (c == ALT)
				shift &= ~ALS;
		} else {			/* "shift" pressed */
			if (c == lshift)
				shift |= SLS;
			else if (c == RSHIFT)
				shift |= SRS;
			else if (c == CTRL)
				shift |= CTS;
			else if (c == ALT)
				shift |= ALS;
			else if (c == CAPLOCK) {
				shift ^= CPLS;	/* toggle cap lock */
				kbCAPLCKhold[mmcurvirterm]=shift&CPLS; /* TBF */
				updleds();
			} else if (c == NUMLOCK) {
				shift ^= NMLS;	/* toggle num lock */
				kbNUMLCKhold[mmcurvirterm]=shift&NMLS; /* TBF */
				updleds();
			}
		}
		return;
	}

	/*
	 * No other key up codes of interest.
	 */
	if (r&KEYUP) {
		return;
	}

        /*
         * If CTRL-F? switch virtual terminals	(TBF)
	 *
         */

	if(c>=58 && c-58<NVIRTERMS && shift&CTS && !(shift&(SRS|SLS|ALS)))
	 defer(kbchangeterm,(char *)(c-58+1));	
	 
      	/*
	 * If the tty is not open the character is
	 * just tossed away. XXXXXXXXXX
	 * routed to console if opened (TBF)
	 */

	if (istty[mmcurvirterm].t_open == 0 && istty[0].t_open==0) {
          return;
	}

	/*
	 * Map character, based on the
	 * current state of the shift, control,
	 * meta and lock flags.
	 */
	if (shift & CTS) {
		if (s == CTS)			/* Map Ctrl (BS | NL) */
			c = (c == BACKSP) ? 0x7F : 0x0A;  
		else if (s==SS1 || s==LET)	/* Normal Ctrl map */
			c = umaptab[c]&0x1F;	/* Clear bits 5-6 */
		else {
			return;			/* Ignore this char */
		}
	} else if (s &= shift) {
		if (shift & SES) {		 /* if shift on */
			if (s & (CPLS|NMLS))     /* if caps/num lock */
				c = lmaptab[c];  /* use unshifted */
			else
				c = umaptab[c];	 /* use shifted */
		} else {			 /* if shift not on */
			if (s & (CPLS|NMLS))     /* if caps/num lock */
				c = umaptab[c];	 /* use shifted */
			else
				c = lmaptab[c];	 /* use unshifted */
		}
	} else					 
		c = lmaptab[c];			 /* use unshifted */

	/*
	 * Act on character.
	 */
	if (c == XXX) {
		return;				 /* char to ignore */
	}

	if (c != SPC) {			 /* not special char? */
		if (shift & ALS)	 /* ALT (meta bit)? */
			c |= 0x80;	 /* set meta */
#if	KBDEBUG
	printf("%x %c ", c, c&0xff);
#endif
		isin(c);		 /* send the char */
	} else {
		update_leds += isspecial(r);	 /* special chars */
	}
	if (update_leds) {
		savests = sphi();
		outb(KBDATA, LEDCMD);
		ledcmd = 1;
		spl(savests);
	}
}

/*
 * Handle special input sequences.
 * The character passed is the key number.
 *
 * The keypad is translated by the following table,
 * the first entry is the normal sequence, the second the shifted,
 * and the third the alternate keypad sequence.
 */
static char *keypad[][3] = {
	{ "\33[H",  "7", "\33?w" },	/* 71 */
	{ "\33[A",  "8", "\33?x" },	/* 72 */
	{ "\33[V",  "9", "\33?y" },	/* 73 */
	{ "\33[D",  "4", "\33?t" },	/* 75 */
	{ "\0337",  "5", "\33?u" },	/* 76 */
	{ "\33[C",  "6", "\33?v" },	/* 77 */
	{ "\33[24H","1", "\33?q" },	/* 79 */
	{ "\33[B",  "2", "\33?r" },	/* 80 */
	{ "\33[U",  "3", "\33?s" },	/* 81 */
	{ "\33[@",  "0", "\33?p" },	/* 82 */
	{ "\33[P", ".",  "\33?n" }	/* 83 */
};

isspecial(c)
int c;
{
	register char *cp;
	register int s;
	int	update_leds = 0;

	cp = 0;

	switch (c) {
	case 15:					/* cursor back tab */
		cp = "\033[Z";
		break;
	case 59: case 60: case 61: case 62: case 63:	/* Function keys */
	case 64: case 65: case 66: case 67: case 68:
		/* offset to function string */
		if(shift&CTS)
			cp="";			/* disable CTRL-F? (TBF) */
		else
		{
			if ( shift & ALS )
				cp = isfval[c-49];
			else
				cp = isfval[c-59];
		}
		break;

	case 70:		/* Scroll Lock -- stop/start output */
	{
		static char cbuf[2];

		cp = &cbuf[0];  /* working buffer */
		if (!(istty[mmcurvirterm].t_sgttyb.sg_flags&RAWIN)) {	/* not if in RAW mode */
			++update_leds;
			if (istty[mmcurvirterm].t_flags & T_STOP) {	/* output stopped? */
			   cbuf[0] = istty[mmcurvirterm].t_tchars.t_startc;  /* start it */
			   scroll[mmcurvirterm] = 0;
			} else {
			   cbuf[0] = istty[mmcurvirterm].t_tchars.t_stopc;   /* stop output */
			   scroll[mmcurvirterm] = 1;
			}
		}
		break;
	}

	case 79:		/* 1/End */
	case 80:		/* 2/DOWN */
	case 81:		/* 3/PgDn */
	case 82:		/* 0/Ins */
	case 83:		/* ./Del */
		--c;		/* adjust code */
	case 75:		/* 4/LEFT */
	case 76:		/* 5 */
	case 77:		/* 6/RIGHT */
		--c;		/* adjust code */
	case 71:		/* 7/Home/Clear */
	case 72:		/* 8/UP */
	case 73:		/* 9/PgUp */
		s = 0;			/* start off with normal keypad */
		if (shift&NMLS)		/* num lock? */
			s = 1;		/* set shift pad */
		if (shift&SES)		/* shift? */
			s ^= 1;		/* toggle shift pad */
		if (shift&AKPS)		/* alternate pad? */
			s = 2;		/* set alternate pad */		
		cp = keypad[c-71][s];   /* get keypad value */
		break;
	}
	if (cp)					/* send string */
		while ((*cp != 0) && (*cp != -1))
			isin( *cp++ & 0377 );
	return update_leds;
}

/**
 *
 * void
 * ismmfunc( c,VTN )	-- process keyboard related output escape sequences
 * char c;
 * unsigned VTN; -- virterm #  0=console, 1-NVIRTERMS=virterm 
 */
void
ismmfunc(c,VTN)
register int c;
unsigned VTN;
{ 
	switch (c) {
	case 't':	/* Enter numlock */
		kbNUMLCKhold[VTN]=NMLS;
		if(VTN==mmcurvirterm)
		 shift |= NMLS;
		updleds();			/* update LED status */
		break;
	case 'u':	/* Leave numlock */
		kbNUMLCKhold[VTN]=0;
		if(VTN==mmcurvirterm)
		 shift &= ~NMLS;
		updleds();			/* update LED status */
		break;
	case '=':	/* Enter alternate keypad */
		kbAKPShold[VTN]=AKPS;
		if(VTN==mmcurvirterm)
			shift |= AKPS;
		break;
	case '>':	/* Exit alternate keypad */
		kbAKPShold[VTN]=0;
		if(VTN==mmcurvirterm) 
			shift &= ~AKPS;
		break;
	case 'c':	/* Reset terminal */
		islock[VTN] = 0;
		kbAKPShold[VTN]=0;
		kbNUMLCKhold[VTN]=0;
		kbCAPLCKhold[VTN]=0;
		scroll[VTN]=0;
		if(VTN==mmcurvirterm)
		 shift  = 0;
		if(istty[VTN].t_flags & T_STOP)		/* output stopped? */
		 ttin(&istty[VTN],			/* re-start output */
		      istty[VTN].t_tchars.t_startc);
		initkeys();
		updleds();			/* update LED status */
		break;
	}
}

/**
 *
 * void
 * isin( c )	-- append character to raw input silo
 * char c;
 */
static
isin( c )
register int c;
{
	int cache_it = 1;
	register TTY * tp = istty + mmcurvirterm;
	register silo_t	*in_s = in_silo + mmcurvirterm;

	if(!tp->t_open)
		tp = istty;

	/*
	 * If using software incoming flow control, process and
	 * discard t_stopc and t_startc.
	 */
	if (!ISRIN) {
		if (ISSTOP) {
			if ((tp->t_flags&T_STOP) == 0)
				tp->t_flags |= T_STOP;
			cache_it = 0;
		}
		if (ISSTART) {
			tp->t_flags &= ~T_STOP;
			ttstart(tp);
			cache_it = 0;
		}
	}
	/*
	 * Cache received character.
	 */
	if (cache_it) {
#if KBDEBUG
printf("put %c ix=%d ox=%d\n", c,in_s->si_ix,in_s->si_ox);
#endif
		in_s->si_buf[ in_s->si_ix ] = c;

		if ( ++(in_s->si_ix) >= sizeof(in_s->si_buf) )
			in_s->si_ix = 0;
	} 
}

/**
 *
 * void
 * isbatch()	-- raw input conversion routine
 *
 *	Action:	Enable the video display.
 *		Canonize the raw input silo.
 *
 *	Notes:	isbatch() was scheduled as a deferred process by isrint().
 */
static void
isbatch( tp )
register TTY * tp;
{
	register int c;
	register silo_t	*in_s = in_silo + mmcurvirterm;
	static int lastc;

	/*
	 * Ensure video display is enabled.
	 */
	mm_von();

	isbusy = 0;

	/*
	 * Process all cached characters.
	 */
	while ( in_s->si_ix != in_s->si_ox ) {

		/*
		 * Get next cached char.
		 */
		c = in_s->si_buf[ in_s->si_ox ];

		if ( in_s->si_ox >= sizeof(in_s->si_buf) - 1 )
			in_s->si_ox = 0;
		else
			in_s->si_ox++;

#if KBDEBUG
printf("get %c ix=%d ox=%d\n", c,in_s->si_ix,in_s->si_ox);
#endif
		if ( (islock[mmcurvirterm] == 0) || ISINTR || ISQUIT ) {
			ttin( tp, c );
		}

		else if ( (c == 'b') && (lastc == '\033') ) {
			islock[mmcurvirterm] = 0;
			ttin( tp, lastc );
			ttin( tp, c );
		}

		else if ( (c == 'c') && (lastc == '\033') ) {
			ttin( tp, lastc );
			ttin( tp, c );
		}

		else
			putchar('\007');

		lastc = c;
	}
}

/*
 * update the keyboard status LEDS
 */
updleds()
{
	int	s;

	s = sphi();
	outb(KBDATA, LEDCMD);
	ledcmd = 1;
	spl(s);
}

/*
 * unlock the scroll in case an interrupt character is received
 */
kbunscroll()
{
	scroll[mmcurvirterm] = 0;
	updleds();
}

/*
 * Change terminals (TBF)
 * scheduled as a deferred process by isrint
 */
kbchangeterm(termno)
char *termno;
{
 unsigned intrstate=sphi();			/* Disable interrupts */

 mmchangeterm((unsigned int)termno);	

 if(kbAKPShold[mmcurvirterm])			/* set alternate keypad */
  shift|=AKPS;
 else
  shift&=~AKPS;

 if(kbCAPLCKhold[mmcurvirterm])		/* set capslock */
  shift|=CPLS;
 else
  shift&=~CPLS;

 if(kbNUMLCKhold[mmcurvirterm])		/* set numlock */
  shift|=NMLS;
 else
  shift&=~NMLS;

 updleds(); 
 spl(intrstate);				/* Restore interrupts */
}
