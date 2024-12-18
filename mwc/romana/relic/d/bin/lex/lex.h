/*
 * lex.h
 */
#include <sys/mdata.h>
#include <stdio.h>
#include <ctype.h>
#include "lextype.h"

#ifdef MSDOS
#define OUTFILE "lex_yy.c"
#else
#define	OUTFILE	"lex.yy.c"
#endif

/*
 * maximum size of automaton
 */
#define	ARRSZ	1000

/*
 * number of chars in a quantum of stringspace
 */
#define	NCBLK	16

/*
 * macros for manipulating bit-packed character classes
 */
#define	classindex(n)	((n)/NBCHAR*(MAXUCHAR+1))
#define	classbit(n)	(1<<((n)&(NBCHAR-1)))

/*
 * check for octal digit
 */
#define	isoctl(c)	('0'<=(c)&&(c)<='7')

/*
 * symbol storage
 */
struct	def {
	struct	def *d_next;
	char	*d_name;
	int	d_data;
};

/*
 * types of input lines in the specification
 */
enum	{
	LN_DFLT,LN_LSPC,LN_CTXT,LN_SCON,
	LN_LCOM,LN_RCOM,LN_DLIM,LN_OPTN,LN_EOFL
};

/*
 * external declarations
 */
extern	int	nxt;
extern	int	yylval;
extern	int	ltype;
extern	int	inquotes;
extern	int	indefs;
extern	int	actn;
extern	int	clas;
extern	int	nfa[ARRSZ][2];
extern	struct	def *defstart;
extern	struct	def *ctxstart;
extern	struct	def *scnstart;
extern	unsigned char *classptr;
extern	FILE	*filein;
extern	FILE	*fileout;
extern	char	opnerr[];
extern	char	outmem[];
extern	char	noactn[];
extern	char	illchr[];
extern	char	illnln[];
extern	char	illrng[];
extern	char	illoct[];
extern	char	unddef[];
extern	char	undctx[];
extern	char	undstc[];
extern	char	illstc[];
extern	char	regsyn[];
extern	char	rulsyn[];
extern	char	actsyn[];
extern	char	unmopr[];
extern	char	reperr[];
extern	char	eoferr[];
extern	char	*alloc();
extern	char	*ralloc();
extern	char	*getident();
extern	char	*malloc();
extern	char	*realloc();
