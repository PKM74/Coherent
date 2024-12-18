/*
 * Kernel portion of typewriter structure.
 */
#ifndef	 __SYS_KTTY_H__
#define	 __SYS_KTTY_H__

#include <common/feature.h>
#include <kernel/timeout.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/clist.h>
#include <sgtty.h>
#if	_I386
#include <termio.h>
#endif

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
#if	_I386
	struct	termio t_termio;
#endif
	int	t_group;	/* Process group */
	int	t_escape;	/* Pending escape count */
	event_t t_ipolls;	/* List of input polls enabled on device */
	event_t t_opolls;	/* List of output polls enabled on device */
	TIM	t_rawtim;	/* Raw timing struct */
	int	t_cs_sel;	/* 0 for resident drivers, CS for loadable */
#if	_I386
	TIM	t_vtime;	/* VTIME timing struct */
	TIM	t_sbrk;		/* TCSBRK timing struct */
#endif
} TTY;

/*
 * Test macros for various conditions related to the terminal settings; the
 * tests are related to conditions at the granularity of the System V modes,
 * so for Coherent-286 the conditions are synthesized from the old V7 modes.
 */

#define	_IS_BREAK_CHAR(tp,c)	((tp)->t_tchars.t_brkc == (c))

/*
 * The following are not part of S5 sgtty.
 */

#define	_IS_RAW_INPUT_MODE(tp)	(((tp)->t_sgttyb.sg_flags & RAWIN) != 0)
#define	_IS_CRT_MODE(tp)	(((tp)->t_sgttyb.sg_flags & CRT) != 0)

#if	_I386

#define	_IS_EOF_CHAR(tp,c)	((tp)->t_termio.c_cc [VEOF] == (c))
#define	_IS_ERASE_CHAR(tp,c)	((tp)->t_termio.c_cc [VERASE] == (c))
#define	_IS_INTERRUPT_CHAR(tp,c) \
				((tp)->t_termio.c_cc [VINTR] == (c))
#define	_IS_KILL_CHAR(tp,c)	((tp)->t_termio.c_cc [VKILL] == (c))
#define	_IS_QUIT_CHAR(tp,c)	((tp)->t_termio.c_cc [VQUIT] == (c))
#define	_IS_START_CHAR(tp,c)	(CSTART == (c))
#define	_IS_STOP_CHAR(tp,c)	(CSTOP == (c))

#define	_IS_CANON_MODE(tp)	(((tp)->t_termio.c_lflag & ICANON) != 0)
#define	_IS_ECHO_MODE(tp)	(((tp)->t_termio.c_lflag & ECHO) != 0)
#define _IS_ICRNL_MODE(tp)	(((tp)->t_termio.c_iflag & ICRNL) != 0)
#define _IS_IGNCR_MODE(tp)	(((tp)->t_termio.c_iflag & IGNCR) != 0)
#define	_IS_ISIG_MODE(tp)	(((tp)->t_termio.c_lflag & ISIG) != 0)
#define	_IS_ISTRIP_MODE(tp)	(((tp)->t_termio.c_iflag & ISTRIP) != 0)
#define _IS_IXON_MODE(tp)	(((tp)->t_termio.c_iflag & IXON) != 0)
#define _IS_IXANY_MODE(tp)	(((tp)->t_termio.c_iflag & IXANY) != 0)
#define _IS_OCRNL_MODE(tp)	(((tp)->t_termio.c_oflag & OCRNL) != 0)
#define _IS_ONLCR_MODE(tp)	(((tp)->t_termio.c_oflag & ONLCR) != 0)
#define	_IS_RAW_OUT_MODE(tp)	(((tp)->t_termio.c_oflag & OPOST) == 0)
#define	_IS_TANDEM_MODE(tp)	(((tp)->t_termio.c_iflag & IXOFF) != 0)
#define	_IS_XTABS_MODE(tp)	(((tp)->t_termio.c_oflag & TABDLY) == TAB3)

#else	/* if ! _I386 */

#define	_IS_EOF_CHAR(tp,c)	((tp)->t_tchars.t_eofc == (c))
#define	_IS_INTRRUPT_CHAR(tp,c)	((tp)->t_tchars.t_intrc == (c))
#define	_IS_QUIT_CHAR(tp,c)	((tp)->t_tchars.t_quitc == (c))
#define	_IS_START_CHAR(tp,c)	((tp)->t_tchars.t_startc == (c))
#define	_IS_STOP_CHAR(tp,c)	((tp)->t_tchars.t_stopc == (c))
#define	_IS_ERASE_CHAR(tp,c)	(((tp)->t_sgttyb.sg_erase == (c))
#define	_IS_KILL_CHAR(tp,c)	(((tp)->t_sgttyb.sg_kill == (c))

#define	_IS_CANON_MODE(tp)	((tp)->t_sgttyb.sg_flags & \
				 (RAWIN | CBREAK)) == 0)
#define	_IS_ECHO_MODE(tp)	(((tp)->t_sgttyb.sg_flags & ECHO) != 0)
#define _IS_ICRNL_MODE(tp)	(((tp)->t_sgttyb.sg_flags & CRMOD) != 0)
#define _IS_IGNCR_MODE(tp)	0
#define	_IS_ISIG_MODE(tp)	(! _IS_RAW_INPUT_MODE (tp))
#define	_IS_ISTRIP_MODE(tp)	(! _IS_RAW_INPUT_MODE (tp))
#define _IS_IXON_MODE(tp)	(! _IS_RAW_INPUT_MODE (tp))
#define _IS_OCRNL_MODE(tp)	0
#define _IS_ONLCR_MODE(tp)	(((tp)->t_sgttyb.sg_flags & CRMOD) != 0)
#define	_IS_RAW_OUTPUT_MODE(tp)	(((tp)->t_sgttyb.sg_flags & RAWOUT) != 0)
#define	_IS_TANDEM_MODE(tp)	(((tp)->t_sgttyb.sg_flags & TANDEM) != 0)
#define	_IS_XTABS_MODE(tp)	(((tp)->t_sgttyb.sg_flags & XTABS) != 0)

#endif	/* ! _I386 */

#endif	/* ! defined (__SYS_KTTY_H__) */
