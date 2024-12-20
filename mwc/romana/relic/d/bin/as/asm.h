#include <stdio.h>
#include <setjmp.h>
#include "asmch.h"
#ifdef	LADDR
#include "n.out.h"
#else
#include <l.out.h>
#endif

/* Basic */
#define	HUGE	1000		/* A huge number */
#define NERR	10		/* Errors per line */
#define NINPUT	128		/* Input buffer size */
#define NCODE	128		/* Listing code buffer size */
#define NTIT	64		/* Title buffer size */
#define	NHASH	64		/* Buckets in hash table */
#define	HMASK	077		/* Hash mask */
#define	NLPP	60		/* Lines per page */

/* Listing */
#define NLIST	0		/* No listing */
#define SLIST	1		/* Source only */
#define ALIST	2		/* Address only */
#define	BLIST	3		/* Bytes */
#define	WLIST	4		/* Words */

#define	dot	(&sym[0])	/* Dot, current loc */

/*
 * Symbol.
 * The names `s_ref' and `s_base'
 * don't get init. in `pst.c'.
 * Good thing: you cannot initialise
 * a union.
 */
struct	sym
{
	struct	sym	*s_sp;		/* Hash link */
	char		s_id[NCPLN];	/* Name */
	unsigned char	s_kind;		/* Symbol kind (S_) */
	unsigned char	s_flag;		/* Symbol flags */
	int		s_type;		/* Expression type */
	address		s_addr;		/* Address */
	int		s_ref;		/* Ref. number */
	int		s_total;	/* Hash total */
	union	{
		struct sym *s_sp;
		struct loc *s_lp;
		int s_segn;
	} s_base;		/* Base */
};

/* Flags */
#define	S_GBL	01		/* Global */
#define	S_ASG	02		/* Assigned */
#define	S_MDF	04		/* Mult. def */
#define	S_END	010		/* End mark */
#define	S_SYMT	020		/* For l.out symbol table */

/* Kinds */
#define	S_NEW	0		/* New name */
#define	S_USER	1		/* User name */
#define	S_LOC	2		/* Loc. counter */
#define	S_SEG	3		/* Seg. name */
#define	S_BYTE	4		/* .byte */
#define	S_WORD	5		/* .word */
#define	S_SDEF	6		/* .segdef */
#define	S_ASCII	7		/* .ascii */
#define	S_COMM	8		/* .comm */
#define	S_LDEF	9		/* .locdef */
#define	S_GLOBL	10		/* .globl */
#define	S_PAGE	11		/* .page */
#define	S_TITLE	12		/* .title */
#define	S_BLK	13		/* .blk[bwl] */

/*
 * Location counter.
 */
struct	loc
{
	struct	loc *l_lp;	/* Link */
	int	l_seg;		/* Seg. no. */
	address	l_break;	/* Size */
	address	l_offset;	/* Offset in area */
	address	l_fuzz;		/* Fuzz */
};

/*
 * Temp. symbols.
 */
struct	tsym
{
	struct	tsym *t_fp;	/* Link to next */
	struct	loc *t_lp;	/* Location counter */
	address	t_addr;		/* Address */
};

struct	tsymp
{
	struct	tsym *tp_fp;	/* Symbol for `n'f */
	struct	tsym *tp_bp;	/* Symbol for `n'b */
	struct	tsym *tp_lfp;	/* First symbol */
	struct	tsym *tp_llp;	/* Last symbol */
};

/*
 * Globals.
 */
extern	address	absexpr();
extern	struct sym *lookup();
extern	int	inbss;
extern  jmp_buf env;
extern	char	*new();
extern	struct	sym sym[];
extern	int	line;
extern	int	page;
extern	int	lop;
extern	int	pass;
extern	int	lflag;
extern	int	gflag;
extern	int	eflag;
extern	int	xflag;
extern	address	laddr;
extern	address	fuzz;
extern	int	lmode;
extern	struct	sym *symhash[NHASH];
extern	int	nloc;
extern	int	nerr;
extern  struct  loc *loc[];
extern  struct  loc *defloc;
extern	char	*ep;
extern	char	eb[NERR];
extern	char	*ip;
extern	char	ib[NINPUT];
extern	char	*cp;
extern	char	cb[NCODE];
extern	char	tb[NTIT];
extern	struct	tsymp tsymp[10];
extern	char	*ofn;
extern	char	*ifn;
extern	FILE	*ofp;
extern	FILE	*sfp;
extern FILE 	*efp;
extern	char	ctype[];

/*
 * Character types.
 * Letters are < 0.
 * Letters and digits are <= 0.
 * This speeds up getid.
 */
#define	LETTER	(-1)
#define	DIGIT	0
#define	BINOP	1
#define ETC	2
#define	ILL	3
#define	SPACE	4

/*
 * Expression.
 */
struct	expr
{
	char	e_mode;		/* Address mode */
	char	e_type;		/* Type */
	address	e_addr;		/* Address */
	union	{
		struct loc  *e_lp;
		struct sym  *e_sp;
		int e_segn;
	} e_base;		/* Rel. base */
};

/* Types */
#define	E_ACON	0		/* A constant */
#define	E_ASEG	1		/* An absolute segment */
#define	E_SYM	2		/* Symbol base */
#define	E_DIR	3		/* Direct address */
#define	E_AREG	4		/* Reg */
#define	E_SEG	5		/* Relocatable segment */
