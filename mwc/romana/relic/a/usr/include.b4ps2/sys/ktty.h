/*
 * Kernel portion of typewriter structure.
 */
#ifndef	 KTTY_H
#define	 KTTY_H
#include <sys/types.h>
#include <poll.h>
#include <sys/clist.h>
#include <sgtty.h>
#ifdef _I386
#include <termio.h>
#endif
#include <sys/timeout.h>

#define	NCIB	256		/* Input buffer */
#define	OHILIM	128		/* Output buffer hi water mark */
#define	OLOLIM	40		/* Output buffer lo water mark */
#define	IHILIM	512		/* Input buffer hi water mark */
#define	ILOLIM	40		/* Input buffer lo water mark */
#define	ITSLIM	(IHILIM-(IHILIM/4))	/* Input buffer tandem stop mark */
#define	ESC	'\\'		/* Some characters */

typedef struct tty {
	CQUEUE	t_oq;		/* Output queue */
	CQUEUE	t_iq;		/* Input queue */
	char	*t_ddp;		/* Device specific */
	int	(*t_start)();	/* Start function */
	int	(*t_param)();	/* Load parameters function */
	char	t_dispeed;	/* Default input speed */
	char	t_dospeed;	/* Default output speed */
	int	t_open;		/* Open count */
	int	t_flags;	/* Flags */
	char	t_nfill;	/* Number of fill characters */
	char	t_fillb;	/* The fill character */
	int	t_ibx;		/* Input buffer index */
	char	t_ib[NCIB];	/* Input buffer */
	int	t_hpos;		/* Horizontal position */
	int	t_opos;		/* Original horizontal position */
	struct	sgttyb t_sgttyb;/* Stty/gtty information */
	struct	tchars t_tchars;/* Tchars information */
#ifdef _I386
	struct	termio t_termio;
#endif
	int	t_group;	/* Process group */
	int	t_escape;	/* Pending escape count */
	event_t t_ipolls;	/* List of input polls enabled on device */
	event_t t_opolls;	/* List of output polls enabled on device */
	TIM	t_rawtim;	/* Raw timing struct */
	int	t_cs_sel;	/* 0 for resident drivers, CS for loadable */
} TTY;

/*
 * Test macros.
 * Assume `tp' holds a TTY pointer.
 *	  `c'  a character.
 * Be very careful if you work on the
 * tty driver that this is true.
 */
#define	ISBRK	(tp->t_tchars.t_brkc   == c)
#define	ISSTART	(tp->t_tchars.t_startc == c)
#define	ISSTOP	(tp->t_tchars.t_stopc  == c)
#define	stopc	(tp->t_tchars.t_stopc)
#define	startc	(tp->t_tchars.t_startc)

/*
 * The following are not part of S5 sgtty.
 */
#define	ISRIN	(tp->t_sgttyb.sg_flags&RAWIN)
#define	ISCRT	(tp->t_sgttyb.sg_flags&CRT)

#if _I386

#define	ISEOF	(tp->t_termio.c_cc[VEOF]   == c)
#define	ISERASE	(tp->t_termio.c_cc[VERASE] == c)
#define	ISINTR	(tp->t_termio.c_cc[VINTR]  == c)
#define	ISKILL	(tp->t_termio.c_cc[VKILL]  == c)
#define	ISQUIT	(tp->t_termio.c_cc[VQUIT]  == c)

#define	ISBBYB	((tp->t_termio.c_lflag & ICANON) == 0)
#define	ISCBRK	((tp->t_termio.c_lflag & ICANON) == 0)
#define	ISECHO	(tp->t_termio.c_lflag & ECHO)
#define ISICRNL	(tp->t_termio.c_iflag & ICRNL)
#define ISIGNCR	(tp->t_termio.c_iflag & IGNCR)
#define	ISISIG	(tp->t_termio.c_lflag & ISIG)
#define	ISISTRIP (tp->t_termio.c_iflag & ISTRIP)
#define ISIXON	(tp->t_termio.c_iflag & IXON)
#define ISONLCR	(tp->t_termio.c_iflag & ONLCR)
#define	ISROUT	((tp->t_termio.c_oflag & OPOST) == 0)
#define	ISTAND	(tp->t_termio.c_iflag & IXOFF)
#define	ISXTABS	(tp->t_termio.c_oflag & XTABS)

#else

#define	ISEOF	(tp->t_tchars.t_eofc   == c)
#define	ISINTR	(tp->t_tchars.t_intrc  == c)
#define	ISQUIT	(tp->t_tchars.t_quitc  == c)

#define	ISBBYB	(tp->t_sgttyb.sg_flags&(RAWIN|CBREAK))
#define	ISCBRK	(tp->t_sgttyb.sg_flags&CBREAK)
#define	ISECHO	(tp->t_sgttyb.sg_flags&ECHO)
#define	ISERASE	(tp->t_sgttyb.sg_erase == c)
#define ISICRNL	(tp->t_sgttyb.sg_flags&CRMOD)
#define ISIGNCR	0
#define	ISISIG	(!ISRIN)
#define	ISISTRIP (!ISRIN)
#define ISIXON	(!ISRIN)
#define	ISKILL	(tp->t_sgttyb.sg_kill  == c)
#define ISONLCR	(tp->t_sgttyb.sg_flags&CRMOD)
#define	ISROUT	(tp->t_sgttyb.sg_flags&RAWOUT)
#define	ISTAND	(tp->t_sgttyb.sg_flags&TANDEM)
#define	ISXTABS	(tp->t_sgttyb.sg_flags&XTABS)

#endif

#endif
