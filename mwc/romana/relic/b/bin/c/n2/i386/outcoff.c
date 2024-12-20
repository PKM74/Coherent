/*
 * n2/i386/outcoff.c
 * C compiler COFF output writer.
 * The first pass writes to a temp file.
 * After the first pass, the compiler knows the sizes of the internal segments.
 * The compiler then maps the internal segments to the actual output segments.
 * The second pass reads the temp file and writes the actual output bits.
 * i386.
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <coff.h>
#include "cc2.h"

#define	CC_ID	"COHERENT i386 cc "	/* .comment contains CC_ID VERSMWC */

#define	NTXT	256			/* Text buffer size		*/
#define	NREL	256			/* Relocation buffer size	*/

#define	RUP(n)	(((n)+3)&(~3))		/* Round n up to dword boundary	*/

/* SYM flag tests. */
#define	is_bss(sp)	((sp)->s_seg==SBSS)
#define	is_def(sp)	(((sp)->s_flag&S_DEF)!=0)
#define	is_global(sp)	(((sp)->s_flag&S_GBL)!=0)
#define	is_label(sp)	(((sp)->s_flag&S_LABNO)!=0)

/* Debugging output. */
#if	DEBUG
#define	dbprintf(args)	printf args
#else
#define	dbprintf(args)
#endif

/*
 * COFF output.
 * .text is section 1, .data is section 2, .bss is section 3.
 * Section 4 is .comment, containing a translator ID and version number;
 * this currently gets written only if debug.
 * The structure of the COFF output file is presently as follows:
 *	file header
 *	[optional header: absent]
 *	section headers 1 through 3 [4 if debug]
 *	data for sections 1 through 3 [4 if debug]
 *	relocs for sections 1 through 3
 *	line numbers for section 1 [absent if not debug]
 *	symbol table [seg names, then symbols]
 *	string table [absent if no long names]
 */

/*
 * The definitions below are indices for the header definitions in scn_hdr[].
 * If not debug, they are also the symbol indices for .text, .data, .bss.
 * Add 1 to covert to COFF section number.
 */
#define	C_TEXT_SEG	0			/* .text section index	*/
#define	C_DATA_SEG	1			/* .data section index	*/
#define	C_BSS_SEG	2			/* .bss section index	*/
#define	C_CMT_SEG	3			/* .comment section index */
#define	C_NSECS		4			/* Number of sections	*/

/* Symbol indices for .text, .data and .bss if debug. */
/* Symbol 0 is .file with 1 aux entry, each section also has 1 aux entry. */
#define	C_TEXT_DB	2
#define	C_DATA_DB	4
#define	C_BSS_DB	6

/*
 * Debug information.
 * Each DBSYM represents one symbol entry or aux entry
 * in the object symbol table.
 * The DBSYM contains the proto entry itself, plus
 * enough additional information for the second pass to correct the
 * actual entry by adjusting long names and adding segment bases.
 */
typedef	struct	dbsym	{
	struct	dbsym	*db_next;	/* Link, must be first	*/
	int		db_ref;		/* Index number		*/
	union	{
		SYMENT	db_use;		/* Symbol entry		*/
		AUXENT	db_uae;		/* Aux entry		*/
	} db_u;
	unsigned short	db_level;	/* Lexical level	*/
	char		db_seg;		/* C segment		*/
	char		db_flags;	/* Flags		*/
	char		db_name[];	/* NUL or name if > 8	*/
} DBSYM;
#define	db_se	db_u.db_use
#define	db_ae	db_u.db_uae
#define	DB_AUX		0x01		/* AUXENT, not SYMENT	*/
#define	DB_LNAME	0x02		/* Name is in db_name	*/
#define	is_aux(dp)	(((dp)->db_flags & DB_AUX) != 0)
#define	is_lname(dp)	(((dp)->db_flags & DB_LNAME) != 0)

/* The output writer generates a LNNUM for each line number entry. */
typedef	struct	lnnum	{
	struct	lnnum	*ln_next;	/* Link			*/
	DBSYM		*ln_dp;		/* Symbol needing value	*/
	AUXENT		*ln_ap;		/* Function aux entry	*/
	unsigned short	ln_lnnum;	/* Line number, 0 for fn */
	unsigned short	ln_val;		/* Refnum or fn syminx	*/
	ADDRESS		ln_addr;	/* Address		*/
	char		ln_flag;	/* LN_FUNC or LN_LINE	*/
} LNNUM;
#define	LN_FUNC	0
#define	LN_LINE	1

/* Linked list of symbols, used for .bb and tag lists. */
typedef	struct	symlist	{
	struct	symlist	*sl_next;	/* Link			*/
	DBSYM		*sl_dp;		/* Symbol pointer	*/
} SYMLIST;

/* Tag patch list for forward tag references. */
typedef	struct	tagfix	{
	struct	tagfix	*tf_next;	/* Link			*/
	SYMLIST		*tf_sp;		/* DBSYMs to fix	*/
	char		tf_name[];	/* Name			*/
} TAGFIX;

extern	DBSYM	*new_db();
extern	DBSYM	*new_sym();
extern	AUXENT	*new_aux();
extern	LNNUM	*new_ln();

/*
 * Scratch file.
 */
#define	TTYPE	0x07			/* Type mask			*/
#define	TPCR	0x08			/* PC rel flag, or'ed in	*/
#define	TSYM	0x10			/* Symbol based flag, or'ed in	*/

#define	TEND	0x00			/* End marker			*/
#define	TENTER	0x01			/* Enter new segment		*/
#define	TBYTE	0x02			/* Byte data			*/
#define	TWORD	0x03			/* Word data			*/
#define	TLONG	0x04			/* Dword data			*/

/*
 * Output writer globals.
 */
SYMLIST	*blockp;			/* Open .bb block list	*/
FILEHDR	coff_hdr;			/* Header buffer	*/
DBSYM	*db_list;			/* Debug symbol list	*/
DBSYM	**db_lastp = &db_list;		/* Last db_list pointer	*/
int	dot_sec;			/* Current COFF section	*/
DBSYM	*fn_dp;				/* Current fn entry	*/
int	level;				/* Lexical level	*/
LNNUM	*ln_list;			/* Line number list	*/
LNNUM	**ln_lastp = &ln_list;		/* Last ln_list pointer	*/
int	refnum = -1;			/* Debug item refnum	*/
char	rel[NREL];			/* Relocation buffer	*/
ADDRESS	reldot[C_NSECS];		/* Relocation offsets	*/
char	*relp;				/* Relocation pointer	*/
SCNHDR	scn_hdr[C_NSECS] = {		/* Section headers	*/
	{ _TEXT,	0L, 0L, 0L, 0L, 0L, 0L, 0, 0, STYP_TEXT },
	{ _DATA,	0L, 0L, 0L, 0L, 0L, 0L, 0, 0, STYP_DATA },
	{ _BSS,		0L, 0L, 0L, 0L, 0L, 0L, 0, 0, STYP_BSS },
	{ _COMMENT,	0L, 0L, 0L, 0L, 0L, 0L, 0, 0, STYP_INFO }
};
long	str_seek;			/* Seek to string table	*/
long	str_size;			/* Size of string table	*/
char	symindex[C_NSECS];		/* Section symbol indices */
SYMLIST	*tag_list;			/* Defined tags		*/
TAGFIX	*tag_fix;			/* Forward tag refs	*/
char	txt[NTXT];			/* Text buffer		*/
ADDRESS	txtdot;				/* Text offset		*/
char	*txtp;				/* Text pointer		*/

/*
 * Map C compiler internal segment into a COFF output section.
 * The SSTRN entry gets patched by outinit() if not -VPSTR.
 */
char	sec_index[] = {
	C_TEXT_SEG,				/* SCODE */
	C_TEXT_SEG,				/* SLINK */
	C_TEXT_SEG,				/* SPURE */
	C_TEXT_SEG,	/* possibly patched */	/* SSTRN */
	C_DATA_SEG,				/* SDATA */
	C_BSS_SEG				/* SBSS  */
};

/*
 * Map C debug scalar types to COFF types.
 * Indexed by type as defined in h/ops.h.
 * COFF has no direct representation for "void" type,
 * it uses <DT_FCN T_NULL> for a void function.
 */
char	coff_type[] = {
	T_NULL,				/* DT_NONE	*/
	T_CHAR,				/* DT_CHAR	*/
	T_UCHAR,			/* DT_UCHAR	*/
	T_SHORT,			/* DT_SHORT	*/
	T_USHORT,			/* DT_USHORT	*/
	T_INT,				/* DT_INT	*/
	T_UINT,				/* DT_UINT	*/
	T_LONG,				/* DT_LONG	*/
	T_ULONG,			/* DT_ULONG	*/
	T_FLOAT,			/* DT_FLOAT	*/
	T_DOUBLE,			/* DT_DOUBLE	*/
	T_NULL,				/* DT_VOID	*/
	T_STRUCT,			/* DT_STRUCT	*/
	T_UNION,			/* DT_UNION	*/
	T_ENUM,				/* DT_ENUM	*/
	DT_PTR,				/* DD_PTR	*/
	DT_FCN,				/* DD_FUNC	*/
	DT_ARY				/* DD_ARRAY	*/
};

/*
 * Map C debug storage class to COFF storage class.
 * Indexed by class-DC_SEX, as defined in h/ops.h.
 * DC_CALL is defined in ops.h but not used anywhere.
 * DC_LINE items generate COFF line number items,
 * they have no COFF storage class.
 */
char	coff_class[] = {
	C_STAT,				/* DC_SEX	*/
	C_EXT,				/* DC_GDEF	*/
	C_EXT,				/* DC_GREF	*/
	C_TPDEF,			/* DC_TYPE	*/
	C_STRTAG,			/* DC_STAG	*/
	C_UNTAG,			/* DC_UTAG	*/
	C_ENTAG,			/* DC_ETAG	*/
	C_FILE,				/* DC_FILE	*/
	C_NULL,				/* DC_LINE	*/
	C_LABEL,			/* DC_LAB	*/
	C_AUTO,				/* DC_AUTO	*/
	C_ARG,				/* DC_PAUTO	*/
	C_REG,				/* DC_REG	*/
	C_REGPARM,			/* DC_PREG	*/
	C_STAT,				/* DC_SIN	*/
	C_MOE,				/* DC_MOE	*/
	C_NULL,		/* unused */	/* DC_CALL	*/
	C_MOS,				/* DC_MOS	*/
	C_MOU				/* DC_MOU	*/
};

/*
 * Map C debug storage class to COFF section number.
 * Indexed by class-DC_SEX, as defined in h/ops.h.
 * DC_CALL is defined in ops.h but not used anywhere.
 * DC_LINE items generate COFF line number items, they have no COFF section.
 */
#define	N_ANY	127	/* maps to .code, .text or .data using sec_index[] */
char	coff_sec[] = {
	N_ANY,				/* DC_SEX	*/
	N_ANY,				/* DC_GDEF	*/
	N_UNDEF,			/* DC_GREF	*/
	N_DEBUG,			/* DC_TYPE	*/
	N_DEBUG,			/* DC_STAG	*/
	N_DEBUG,			/* DC_UTAG	*/
	N_DEBUG,			/* DC_ETAG	*/
	N_DEBUG,			/* DC_FILE	*/
	N_DEBUG,			/* DC_LINE	*/
	N_ANY,				/* DC_LAB	*/
	N_ABS,				/* DC_AUTO	*/
	N_ABS,				/* DC_PAUTO	*/
	N_ABS,				/* DC_REG	*/
	N_ABS,				/* DC_PREG	*/
	N_ANY,				/* DC_SIN	*/
	N_ABS,				/* DC_MOE	*/
	N_ANY,		/* unused */	/* DC_CALL	*/
	N_ABS,				/* DC_MOS	*/
	N_ABS				/* DC_MOU	*/
};

/* First pass routines. */
/*
 * Initialize the code writer.
 */
outinit()
{
#if	MONOLITHIC
	/* Reinitialize the coff_hdr. */
	memset(&coff_hdr, 0, sizeof coff_hdr);
	dot_sec = 0;
	memset(reldot, 0, sizeof reldot);
	memset(scn_hdr, 0, sizeof scn_hdr);
	strcpy(scn_hdr[C_TEXT_SEG].s_name, _TEXT);
	scn_hdr[C_TEXT_SEG].s_flags = STYP_TEXT;
	strcpy(scn_hdr[C_DATA_SEG].s_name, _DATA);
	scn_hdr[C_DATA_SEG].s_flags = STYP_DATA;
	strcpy(scn_hdr[C_BSS_SEG].s_name, _BSS);
	scn_hdr[C_BSS_SEG].s_flags = STYP_BSS;
	strcpy(scn_hdr[C_CMT_SEG].s_name, _COMMENT);
	scn_hdr[C_CMT_SEG].s_flags = STYP_INFO;

	/* Reinitialize other globals. */
	level = 0;
	db_lastp = &db_list;
	ln_lastp = &ln_list;
	refnum = -1;
	tag_fix = tag_list = NULL;
#endif
	if (notvariant(VPSTR))
		sec_index[SSTRN] = C_DATA_SEG;
}

/*
 * Output an absolute byte.
 */
outab(b) int b;
{
	bput(TBYTE);
	bput(b);
	++dot;
}

/*
 * Output an absolute word.
 */
outaw(w) int w;
{
	bput(TWORD);
	iput((ival_t)w);
	dot += 2;
}

/*
 * Output an absolute dword.
 */
outal(i) ival_t i;
{
	bput(TLONG);
	iput(i);
	dot += 4;
}

/*
 * Output a full byte.
 */
outxb(sp, b, pcrf) register SYM	*sp; ADDRESS b; int pcrf;
{
	register int	opcode;

	opcode = TBYTE;
	if (sp != NULL) {
		opcode |= TSYM;
		scn_hdr[dot_sec].s_nreloc++;
	}
	if (pcrf)
		opcode |= TPCR;
	bput(opcode);
	bput(b);
	if (sp != NULL)
		pput(sp);
	++dot;
}

/*
 * Output a full word.
 */
outxw(sp, w, pcrf) register SYM *sp; ADDRESS w; int pcrf;
{
	register int	opcode;

	opcode = TWORD;
	if (sp != NULL) {
		opcode |= TSYM;
		scn_hdr[dot_sec].s_nreloc++;
	}
	if (pcrf)
		opcode |= TPCR;
	bput(opcode);
	iput((ival_t)w);
	if (sp != NULL)
		pput(sp);
	dot += 2;
}

/*
 * Output a full dword.
 */
outxl(sp, i, pcrf) register SYM *sp; ADDRESS i; int pcrf;
{
	register int	opcode;

	opcode = TLONG;
	if (sp != NULL) {
		opcode |= TSYM;
		scn_hdr[dot_sec].s_nreloc++;
	}
	if (pcrf)
		opcode |= TPCR;
	bput(opcode);
	iput(i);
	if (sp != NULL)
		pput(sp);
	dot += 4;
}

/*
 * Output a segment switch.
 */
outseg(s) register int s;
{
	bput(TENTER);
	bput(s);
	dot_sec = sec_index[s];
}

/*
 * Output n zero bytes.
 * If flag is true, generate NOPs instead.
 */
outnzb(n, flag) register sizeof_t n; int flag;
{
	register int i, val;

	val = (flag) ? 0x90 : 0;	/* xchg %eax, %eax or 0 */
	for (i = 1; i <= n; ++i) {
		bput(TBYTE);
		bput(val);
	}
	dot += n;
}

/*
 * Process a dlabel record.
 * Do the work on pass 1 so that the number of symbols in the table is known.
 * 'i' is the level of indentation, 'class' is the initial storage class.
 * A lot of grunge.
 */
outdlab(i, class) int i; register int class;
{
	register int line, type, n;
	int csec, ctype, cclass, tagn;
	ival_t size, value, cvalue, array_size, last_size, sue_size;
	int seg, width, derived, dims, suppress;
	static int enumtype = DT_INT;
	unsigned short dim[DIMNUM];
	SYM *sp;
	DBSYM *dp, *tagdp;
	AUXENT *ap, *endap, *arrayap;

	++refnum;					/* compiler db refnum */

	/* Initialize locals. */
	seg = -1;
	suppress = tagn = ctype = derived = dims = 0;
	last_size = array_size = sue_size = (ival_t)0;
	ap = endap = arrayap = tagdp = sp = NULL;

	/*
	 * Map C segment and storage class to COFF section and storage class.
	 * The COFF info may get adjusted later.
	 */
	if (class >= DC_SEX && class <= DC_MOU) {
		csec = coff_sec[class - DC_SEX];
		cclass = coff_class[class - DC_SEX];
		if (cclass == C_NULL && class != DC_LINE)
			cbotch("unrecognized class=%d", class);
	} else
		cbotch("unrecognized class=%d", class);
	dbprintf(("outdlab(%d, %d)\t", i, class));
	dbprintf(("#%d\t", refnum));
	dbprintf(("cclass=%d\t", cclass));

	/* Get line number. */
	line = iget();
	dbprintf(("line=%d\t", line));

	/* Get value. */
	if (class < DC_AUTO)
		value = 0;		/* Name string only */
	else if (class < DC_MOS)
		value = iget();		/* Value */
	else {				/* DC_MOS or DC_MOU */
		width = bget();		/* Width */
		n = bget();		/* Offset */
		value = iget();		/* Value */
		if (width != 0) {
			value = value * 8 + n;		/* bitfield offset */
			cclass = C_FIELD;		/* and class */
		}
	}
	cvalue = value;			/* COFF value, may be modified below */
	if (class == DC_REG || class == DC_PREG) {
		/* Map compiler register to machine register number. */
		switch(value) {
		case EBX:	cvalue = MEBX;		break;
		case EDI:	cvalue = MEDI;		break;
		case ESI:	cvalue = MESI;		break;
		default:	cbotch("unexpected register %d", value);
		}
	}

	/* Get name. */
	sget(id, NCSYMB);
	dbprintf(("id=%s\t", id));

	if (class == DC_SEX || class == DC_GDEF || class == DC_GREF) {

		/* Check for global symbol table entry. */
		sp = glookup(id, 0);
		if (sp == NULL)
			cbotch("global %s not found", id);
		seg = sp->s_seg;
		/* Grab section and value; cvalue gets patched later for commons. */
		if (is_def(sp)) {
			csec = sec_index[seg] + 1;
			value = cvalue = sp->s_value;
		} else {
			/*
			 * Extern: seg != SANY changed to seg -1;
			 * common: seg SANY.
			 */
			csec = N_UNDEF;
			if (seg != SANY)
				seg = -1;
		}
		/* Suppress redefinitions of globals. */
		for (dp = db_list; dp != NULL; dp = dp->db_next) {
			if (!is_aux(dp)
			 && dp->db_level == 0
			 && db_strcmp(dp, id) == 0) {
				++suppress;
#if	0
				if (is_def(sp)) {
					/* Symbol may be defined since previous ref. */
					dp->db_se.n_value = cvalue;
					dp->db_se.n_scnum = csec;
				}
#endif
				break;
			}
		}
	} else if (class == DC_SIN) {
		/* Find location of static internal items. */
		sp = llookup(value, 0);
		if (sp == NULL)
			cbotch("local %d not found", value);
		seg = sp->s_seg;
		csec = sec_index[seg] + 1;
		value = cvalue = sp->s_value;
	} else if (class == DC_LAB)
		csec = C_TEXT_SEG + 1;

	/* Get type. */
	for (;;) {
		type = bget();				/* type */
		if (class == DC_MOE)
			type = enumtype;		/* fudge DT_NONE */
		dbprintf(("type=%d ", type));

		if (type < DC_SEX) {
			size = iget();			/* size */
			if (csec == N_UNDEF && seg != SANY)
				cvalue = size;		/* cvalue means size for commons */
			if (last_size != 0) {
				/* Compute array dim from last/current size. */
				if (dims <= DIMNUM)
					dim[dims-1] = (unsigned short)(last_size/size);
				last_size = 0;
			}
			dbprintf(("size=%d\t", size));

			/* Compute COFF type. */
			if (type <= DT_ENUM)
				ctype |= coff_type[type];	/* basic COFF type */
			else if (type <= DD_ARRAY) {
				/* Derived types DD_PTR, DD_FUNC, DD_ARRAY. */
				ctype |= (coff_type[type] << N_BTSHFT) << (derived++ * N_TSHIFT);
				if (type == DD_ARRAY
				 && (csec != N_UNDEF || seg != SANY)) {
					/* Reconstruct array dim from current/next size. */
					if (dims++ == 0) {
						array_size = size;
						for (n = 0; n < DIMNUM; n++)
							dim[n] = 0;
					}
					last_size = size;
				}
			}
			dbprintf(("ctype=%x\t", ctype));

			if (type < DT_STRUCT) {
				/*
				 * Simple types.
				 * An aux entry is needed for fields, functions,
				 * nested autos and arrays.
				 */
				if (class == DC_LINE || class == DC_LAB) {
					dbprintf(("\n"));
					db_line(line);
					return;
				} else if (class == DC_FILE) {
					dbprintf(("\n"));
					db_file();
					return;
				}
				if (suppress)
					break;
				if (csec == N_UNDEF && dims > 0)
					cvalue = array_size;
				dp = new_sym(id, cvalue, csec, ctype, cclass);
				dp->db_seg = seg;
				if (cclass == C_FIELD) {
					/* Bitfield. */
					ap = new_aux();
					ap->ae_size = width;
				} else if (ISFCN(ctype) && csec != N_UNDEF) {
					/* Non-extern function. */
					ap = new_aux();
					db_func(dp);
				} else if (cclass == C_AUTO && level > 1) {
					/* Auto in nested block. */
					if (blockp == NULL)
						cbotch("no blockp");
					ap = new_aux();
					ap->ae_lnno = blockp->sl_dp->db_ae.ae_lnno;
				}
				if (dims != 0) {
					/* Array. */
					if (ap == NULL)
						ap = new_aux();
					arrayap = ap;
					arrayap->ae_size = array_size;
				}
				break;
			} else if (type <= DT_ENUM) {
				/*
				 * DT_STRUCT, DT_UNION, DT_ENUM types.
				 * An aux entry is always generated, although in
				 * rare cases (unresolved tags) it remains empty.
				 */
				sue_size = size;
				if (suppress)
					ap = &(dp->db_ae);
				else {
					if (csec == N_UNDEF && dims > 0)
						cvalue = array_size;
					dp = new_sym(id, cvalue, csec, ctype, cclass);
					ap = new_aux();
					ap->ae_size = size;
				}
				tagn = dp->db_ref;		/* tag index */
				tagdp = dp->db_next;		/* tag aux dp */
				if (ISTAG(cclass)) {
					/* Tag definition. */
					tag_def(dp);
					endap = ap;
					ap->ae_size = size;
					if (cclass == C_ENTAG)
						enumtype = (size == 1) ? DT_UCHAR : DT_INT;
				} else if (ISFCN(ctype))
					db_func(dp);
				if (dims != 0) {
					arrayap = ap;
					arrayap->ae_size = array_size;
				}
				continue;	/* to process tag, member list */
			} else if (type == DX_MEMBS) {
				/* Process member list recursively. */
				++level;
				for ( ; size > 0; --size) {
					dbprintf(("\n"));
					/*
					 * Leave refnum unchanged to keep
					 * it in sync with the outdloc()
					 * reference numbers, for local structs.
					 * It gets bumped in outdlab().
					 */
					--refnum;
					outdlab(i+1, bget());	/* overwrites id */
				}
				--level;
				dp = new_sym(".eos", sue_size, N_ABS, T_NULL, C_EOS);
				ap = new_aux();
				ap->ae_size = sue_size;
				ap->ae_tagndx = tagn;
				break;
			} else if (type == DX_NAME) {
				/* Named type. */
				sget(id, NCSYMB);		/* overwrites id */
				tag_ref(id, tagdp);
				dbprintf(("tag=%s", id));
				break;
			}
		} else
			cbotch("unrecognized type %d", type);
	}
	if (endap != NULL)
		endap->ae_endndx = coff_hdr.f_nsyms;	/* patch end index */
	if (arrayap != NULL)
		for (n = 0; n < DIMNUM; n++)
			arrayap->ae_dimen[n] = dim[n];	/* patch array dims */
	if (dims > DIMNUM)
		cwarn("line %d: array type too complex, debug information lost", line);
	if (derived > 6)
		cwarn("line %d: type too complex, debug information lost", line);
	dbprintf(("\n"));
}

/*
 * Handle function debug information.
 * This generates the .bf when the function name is seen
 * rather than when the '{' is seen,
 * because the .bb symbol must precede the function parameter symbols.
 * The function size and endindex are patched when the .ef gets generated.
 */
db_func(dp) register DBSYM *dp;
{
	register LNNUM *lp;

	lp = new_ln(LN_FUNC);
	lp->ln_val = dp->db_ref;
	fn_dp = dp;
	dp = new_sym(".bf", (ival_t)0, C_TEXT_SEG+1, T_NULL, C_FCN);
	new_aux();
}

/*
 * Process DC_LINE ('{', ';' and '}') and DC_LAB items.
 * Each generates a line number entry, some generate additional symbol entries.
 * The screwy line numbers for .bb/.eb are as in Unix, might be wrong.
 */
db_line(line) register int line;
{
	static	int	fn_line;	/* First lnnum of fn		*/
	register LNNUM *lp;
	register DBSYM *dp;
	register AUXENT *ap;
	SYMLIST *bp;
	char *name;

	dbprintf(("db_line(%d): %c\n", line, id[0]));
	dp = NULL;
	switch(id[0]) {
	case '{':
		/* .bf got generated by db_func(), patch its value and line. */
		if (level == 0) {
			if (fn_dp == NULL)
				cfatal("incomplete function debug information");
			dp = fn_dp->db_next->db_next;		/* .bf entry */
			fn_line = dp->db_next->db_ae.ae_lnno = line;
		} else {
			/* Generate a .bb entry. */
			dp = new_sym(".bb", (ival_t)0, C_TEXT_SEG+1, T_NULL, C_BLOCK);
			ap = new_aux();
			ap->ae_lnno = line - fn_line + 2;	/* huh? */

			/* Link the .bb onto the blockp list. */
			bp = (SYMLIST *)malloc(sizeof(SYMLIST));
			if (bp == NULL)
				cnomem("db_line");
			bp->sl_next = blockp;
			bp->sl_dp = dp->db_next;
			blockp = bp;
		}
		level++;
		break;
	case ';':
		/* Generate a line number entry only. */
		break;
	case '}':
		/* Generate a .ef or .eb entry. */
		level--;
		name = (level == 0) ? ".ef" : ".eb";
		dp = new_sym(name, (ival_t)0, C_TEXT_SEG+1, T_NULL,
			(level == 0) ? C_FCN : C_BLOCK);
		ap = new_aux();
		ap->ae_lnno = (level==0) ? line - fn_line + 1
					 : line - fn_line + 2;	/* huh? */
		if (level == 0) {
			/* End of function, patch its length and end. */
			ap = &(fn_dp->db_next->db_ae);
			ap->ae_fsize = -(fn_dp->db_se.n_value);	/* -start */
			ap->ae_endndx = coff_hdr.f_nsyms;
		} else {
			/* Close nested block, popping blockp entry. */
			if ((bp = blockp) == NULL)
				cbotch("db_line");
			blockp = bp->sl_next;
			bp->sl_dp->db_ae.ae_endndx = coff_hdr.f_nsyms;
			free(bp);
		}
		break;
	default:
		/* Generate a label entry. */
		dp = new_sym(id, (ival_t)0, C_TEXT_SEG+1,
			T_NULL,		/* icc uses <DT_ARY T_INT>, dunno why */
			C_LABEL);
		break;
	}

	/* Generate a new line number item. */
	lp = new_ln(LN_LINE);
	lp->ln_dp = dp;		/* adjust value of DBSYM when addr is known */
	lp->ln_lnnum = line - fn_line + 1;
	lp->ln_val = refnum;
	if (id[0] == '}' && level == 0)
		lp->ln_ap = ap;	/* adjust fn size when addr is known */
}

/*
 * Create symbol table entries for a DC_FILE.
 */
db_file()
{
	register DBSYM *dp;
	register AUXENT *ap;
	register char *cp;

	if (refnum != 0)
		cbotch("db_file");	/* DC_FILE must be first entry */
	dp = new_sym(".file", (ival_t)0, N_DEBUG, T_NULL, C_FILE);
	ap = new_aux();			/* aux entry */
	if ((cp = strrchr(id, '/')) == NULL)	/* ignore pathname if present */
		cp = id;
	strncpy(ap->ae_fname, cp, FILNMLEN);
	coff_hdr.f_nsyms += 2 * C_NSECS;	/* section symbols with aux entries */
}

/*
 * Allocate a new DBSYM item, initialize it to 0 and link it into the list.
 * If name is non-NULL, initialize its name field.
 */
DBSYM *
new_db(name) register char *name;
{
	register DBSYM *dp;
	register int size, len;

	size = sizeof(*dp);
	if (name != NULL && (len = strlen(name)) > 8)
		size += len + 1;
	dp = (DBSYM *)malloc(size);
	if (dp == NULL)
		cnomem("new_db");
	memset(dp, 0, size);
	*db_lastp = dp;
	db_lastp = &(dp->db_next);
	if (name != NULL) {
		if (len <= 8)
			strncpy(dp->db_se.n_name, name, 8);
		else {
			strcpy(dp->db_name, name);
			dp->db_flags = DB_LNAME;
		}
	}
	dp->db_seg = -1;
	dp->db_level = level;
	dp->db_ref = coff_hdr.f_nsyms++;
	return dp;
}

/*
 * Allocate a new DBSYM item and initialize it.
 */
DBSYM *
new_sym(name, value, sec, type, class)
char *name;
ival_t value;
int sec, type, class;
{
	register DBSYM *dp;
	register SYMENT *sp;

	dp = new_db(name);
	dbprintf(("new_sym(%s) val=%ld sec=%d type=%d cl=%d @%x\n", name, value, sec,type, class, dp));
	sp = &(dp->db_se);
	sp->n_value = value;
	sp->n_scnum = sec;
	sp->n_type = type;
	sp->n_sclass = class;
	sp->n_numaux = 0;
	return dp;
}

/*
 * Allocate a new AUXENT and return a pointer to it.
 */
AUXENT *
new_aux()
{
	register DBSYM *dp;

	++(((DBSYM *)db_lastp)->db_se.n_numaux);	/* bump aux entry count */
	dp = new_db(NULL);
	dp->db_flags |= DB_AUX;
	return &(dp->db_ae);
}

/*
 * Allocate a new LNNUM item, initialize it to 0 and link it into the list.
 */
LNNUM *
new_ln(flag) int flag;
{
	register LNNUM *lp;

	lp = (LNNUM *)malloc(sizeof(*lp));
	if (lp == NULL)
		cnomem("new_ln");
	memset(lp, 0, sizeof(*lp));
	*ln_lastp = lp;
	ln_lastp = &(lp->ln_next);
	++scn_hdr[C_TEXT_SEG].s_nlnno;
	lp->ln_flag = flag;
	return lp;
}

/* Tag handling. */
/*
 * Define a tag with symbol table entry dp.
 * Previous forward references are left unresolved until tag_resolve().
 */
tag_def(dp) DBSYM *dp;
{
	register SYMLIST *sp;

	sp = (SYMLIST *)malloc(sizeof(*sp));
	if (sp == NULL)
		cnomem("tag_def");
	sp->sl_next = tag_list;
	sp->sl_dp = dp;
	tag_list = sp;
}

/*
 * Look up a tag name in the list of defined tags.
 * Return a nonnegative symbol reference number or -1 if not found.
 */
int
tag_lookup(name) register char *name;
{
	register SYMLIST *sp;

	for (sp = tag_list; sp != NULL; sp = sp->sl_next)
		if (db_strcmp(sp->sl_dp, name) == 0)
			return sp->sl_dp->db_ref;
	return -1;
}

/*
 * Reference a tag.
 * If defined, store the symbol reference number in the aux entry at dp.
 * If not, add an entry to the TAGFIX list so it gets fixed up later.
 */
tag_ref(name, dp) char *name; DBSYM *dp;
{
	register SYMLIST *sp;
	register TAGFIX *tp;
	register int n;

	dbprintf(("tag_ref(%s, %d):\n", name, dp->db_ref));
	if (dp == NULL)
		cbotch("NULL aux pointer in tag_ref");
	if ((n = tag_lookup(name)) != -1) {
		/* The tag is defined already. */
		dp->db_ae.ae_tagndx = n;		/* store index */
		return;
	}

	/*
	 * The tag is not defined.
	 * Search for an existing tag_fix entry.
	 */
	for (tp = tag_fix; tp != NULL; tp = tp->tf_next)
		if (strcmp(name, tp->tf_name) == 0)
			break;
	if (tp == NULL) {
		/* Build a new TAGFIX entry on first forward reference. */
		tp = (TAGFIX *)malloc(sizeof(*tp) + strlen(name) + 1);
		if (tp == NULL)
			cnomem("tag_ref");
		tp->tf_next = tag_fix;
		tp->tf_sp = NULL;
		strcpy(tp->tf_name, name);
		tag_fix = tp;
	}

	/* Add dp to the list of fixes. */
	sp = (SYMLIST *)malloc(sizeof(*sp));
	if (sp == NULL)
		cnomem("tag_ref");
	sp->sl_next = tp->tf_sp;
	sp->sl_dp = dp;
	tp->tf_sp = sp;
}

/*
 * Resolve forward tag references and free the tag lists.
 * Forward references which remain unresolved could be handled in several ways,
 * this just leaves the aux entry (rather than deleting it) and leaves its
 * tag member index zeroed.
 */
tag_resolve()
{
	register TAGFIX *tp;
	register SYMLIST *sp;
	int n;
	SYMLIST *sp2;

	while ((tp = tag_fix) != NULL) {
		if ((n = tag_lookup(tp->tf_name)) != -1) {
			/* Tag is defined. */
			for (sp = tp->tf_sp; sp != NULL; ) {
				sp->sl_dp->db_ae.ae_tagndx = n;
				sp2 = sp;
				sp = sp->sl_next;
				free(sp2);
			}
		} else {
			/* Tag is not defined. */
			dbprintf(("tag %s: unresolved forward references\n", tp->tf_name));
			for (sp = tp->tf_sp; sp != NULL; ) {
				/* Do nothing; there are alternatives... */
				sp2 = sp;
				sp = sp->sl_next;
				free(sp2);
			}
		}
		tag_fix = tp->tf_next;
		free(tp);
	}

	/* Free the tag list. */
	while (tag_list != NULL) {
		sp = tag_list;
		tag_list = sp->sl_next;
		free(sp);
	}
}

/*
 * Process a debug relocation record.
 * n is the debug item reference number.
 */
outdloc(n) register int n;
{
	register LNNUM *lp;
	register DBSYM *dp;
	register AUXENT *ap;

	dbprintf(("outdloc(%d): dot=%d\n", n, dot));

	/* Find matching refnum in line number list. */
	for (lp = ln_list; lp != NULL; lp = lp->ln_next) {
		if (lp->ln_flag == LN_LINE && lp->ln_val == n)
			break;
	}
	if (lp == NULL)
		cbotch("outdloc refnum=%d", n);
	if (dotseg != SCODE)
		cbotch("outdloc seg=%d", seg);
	lp->ln_addr = dot;			/* patch address */
	if ((dp = lp->ln_dp) != NULL)
		dp->db_se.n_value = dot;	/* patch symbol entry address */

	/*
	 * The function size patch below is wrong because the actual
	 * function size includes the epilog.
	 * The "dot + 2" adds in the "leave; ret" bytes,
	 * but there may be from 0 to 3 additional "pop" bytes for EBX, ESI, EDI.
	 */
	if ((ap = lp->ln_ap) != NULL)
		ap->ae_fsize += dot + 2;	/* patch function length */
}

/*
 * Finish up.
 */
outdone()
{
	if (notvariant(VASM))
		bput(TEND);
	tag_resolve();
}

/* Second pass. */
copycode()
{
	register int op, i, flags;
	register SEG *segp;
	register SYM *sp;
	register long dseek;
	register ADDRESS mseek, segsize;
	int nsecs, nd, isbyte, isword, issym, ispcr;
	ival_t data;
	RELOC *rp;
	SCNHDR *shp;

	/* Assign symbol table indices for sections, different if debug. */
	flags = 0;		/* COFF header flags */
	nsecs = C_NSECS;	/* number of sections, including .comment */
	if (refnum != -1) {
		symindex[C_TEXT_SEG] = C_TEXT_DB;
		symindex[C_DATA_SEG] = C_DATA_DB;
		symindex[C_BSS_SEG] = C_BSS_DB;
	} else {
		flags |= F_LSYMS;		/* no local symbols */
		symindex[C_TEXT_SEG] = C_TEXT_SEG;
		symindex[C_DATA_SEG] = C_DATA_SEG;
		symindex[C_BSS_SEG] = C_BSS_SEG;
		--nsecs;			/* do not write .comment section */
	}

	/* Assign segment base addresses. */
	dot_sec = -1;
	dseek = sizeof(FILEHDR) + nsecs * sizeof(SCNHDR);
	mseek = 0;
	for (i = SCODE, segp = &seg[SCODE]; i <= SBSS; ++i, ++segp) {
		segsize = RUP(segp->s_dot);
		segp->s_dot = 0;
		if (sec_index[i] != dot_sec) {
			/* Begin new COFF section. */
			dot_sec = sec_index[i];
			dbprintf(("COFF section %d\n", dot_sec));
			shp = &scn_hdr[dot_sec];
			shp->s_paddr = shp->s_vaddr = mseek;
			if (i != C_BSS_SEG)
				shp->s_scnptr = dseek;
		}
		dbprintf(("segment %d: size=%d dseek=%ld mseek=%ld\n", i, segsize, dseek, mseek));
		segp->s_dseek = dseek;		/* set compiler seg disk seek */
		segp->s_mseek = mseek;		/* and memory seek */
		scn_hdr[dot_sec].s_size += segsize;	/* bump COFF seg size */
		dseek += segsize;		/* bump seek pointer */
		mseek += segsize;		/* bump memory seek */
	}

	/* Write the .comment section: translator id and version number. */
	if (refnum != -1) {
		shp = &scn_hdr[C_CMT_SEG];
		segsize = RUP(strlen(CC_ID) + strlen(VERSMWC) + 1);
		shp->s_paddr = mseek;
		shp->s_vaddr = mseek;
		shp->s_size = segsize;
		shp->s_scnptr = dseek;
		oseek(dseek);
		owrite(CC_ID, sizeof(CC_ID) - 1);	/* no NUL */
		owrite(VERSMWC, sizeof(VERSMWC));	/* with NUL */
		dseek += segsize;			/* bump seek pointer */
		mseek += segsize;			/* bump memory seek */
	}

	/* Fix reloc offsets. */
	for (shp = &scn_hdr[C_TEXT_SEG]; shp < &scn_hdr[nsecs]; ++shp) {
		if (shp->s_nreloc != 0) {
			shp->s_relptr = dseek;
			dseek += shp->s_nreloc * sizeof(RELOC);
		}
	}

	/* Write line number information, .text section only. */
	shp = &scn_hdr[C_TEXT_SEG];
	if (shp->s_nlnno != 0) {
		shp->s_lnnoptr = dseek;
		oseek(dseek);
		write_lnnums();
		dseek += scn_hdr[C_TEXT_SEG].s_nlnno * sizeof(LINENO);
	} else
		flags |= F_LNNO;		/* no line numbers */

	/* Write symbols. */
	write_symbols(dseek);

	/* Copy out code. */
	dotseg = SCODE;
	dot_sec = sec_index[dotseg];
	txtdot = dot = 0;
	txtp = &txt[0];
	relp = &rel[0];
	while ((op = bget()) != TEND) {
		if (op == TENTER) {
			notenuf();
			seg[dotseg].s_dot = dot;
			dotseg = bget();
			dot_sec = sec_index[dotseg];
			txtdot = dot = seg[dotseg].s_dot;
			continue;
		}
		enuf(4, sizeof(RELOC));		/* leave space for next op */
		if (isbyte = ((op & TTYPE) == TBYTE)) {
			nd = 1;
			data = bget();
		} else if (isword = ((op & TTYPE) == TWORD)) {
			nd = 2;
			data = iget();
		} else if ((op & TTYPE) == TLONG) {
			nd = 4;
			data = iget();
		} else
			cbotch("unrecognized op byte: %d", op);
		if (issym = ((op & TSYM) != 0)) {
			sp = pget();
			if (is_def(sp) || is_label(sp) || is_bss(sp))
				data += sp->s_value;
		}
		if (ispcr = ((op & TPCR) != 0))
			data -= dot + nd;

		/* Write text. */
		for (i = 1; i <= nd; i++) {
			*txtp++ = data;
			data >>= 8;
		}

		/* Write relocation item if required. */
		if (issym || ispcr) {
			rp = (RELOC *)relp;
			rp->r_vaddr = dot + seg[dotseg].s_mseek;
			if (issym) {
				if (is_def(sp) || is_label(sp))
					rp->r_symndx = symindex[sec_index[sp->s_seg]];
				else
					rp->r_symndx = sp->s_ref;
			} else
				rp->r_symndx = symindex[dot_sec];
			if (ispcr)
				rp->r_type = (isbyte) ? R_PCRBYTE
					   : (isword) ? R_PCRWORD : R_PCRLONG;
			else
				rp->r_type = (isbyte) ? R_DIR8
					   : (isword) ? R_DIR16 : R_DIR32;
			relp += sizeof(RELOC);
		}
		dot += nd;
	}

	/* Flush buffers. */
	notenuf();

	/* Write COFF header. */
	coff_hdr.f_magic = C_386_MAGIC;
	coff_hdr.f_nscns = nsecs;
	coff_hdr.f_timdat = time(NULL);
	coff_hdr.f_opthdr = 0;
	coff_hdr.f_flags = F_AR32WR | flags;
	oseek(0L);
	owrite((char *)&coff_hdr, sizeof(coff_hdr));

	/* Write section headers. */
	for (shp = &scn_hdr[C_TEXT_SEG]; shp < &scn_hdr[nsecs]; ++shp)
		owrite((char *)shp, sizeof(*shp));
}

/*
 * Write the COFF symbol table.
 * Spill long symbol names to the strings section.
 * If debug, there is a DBSYM entry for each symbol table entry,
 * except that the section names and their aux entries are implicit.
 * If not debug, generate COFF symbol table from the compiler symbol table.
 */
write_symbols(dseek) register long dseek;
{
	register int i;
	register SYM *sp;
	int db_flag, nsecs;
	SYMENT sym;
	AUXENT aux;
	SCNHDR *shp;

	db_flag = (refnum != -1);
	nsecs = C_NSECS;
	coff_hdr.f_symptr = dseek;		/* symbol secton base */
	oseek(dseek);				/* seek to symbol section */
	if (db_flag) {
		/* Write .file symbol and its aux entry. */
		write_dbsym();			/* .file */
		write_dbsym();			/* .file aux entry */
	} else
		coff_hdr.f_nsyms = --nsecs;	/* .text, .data, .bss symbols */

	/* Set reference numbers and adjust offsets in symbol table. */
	for (i = 0; i < NSHASH; ++i) {
		for (sp = hash2[i]; sp != NULL; sp = sp->s_fp) {
			/* Adjust symbol offsets for segment bases. */
			if (is_def(sp) || is_label(sp))
				sp->s_value += seg[sp->s_seg].s_mseek;
			/* Set reference numbers. */
			if (is_global(sp) || (!is_label(sp) && !is_def(sp))) {
				if (db_flag)
					setrefnum(sp);
				else
					sp->s_ref = coff_hdr.f_nsyms++;
			} else if (isvariant(VXSTAT) && !is_global(sp)
			        && !is_label(sp) && is_def(sp))
					sp->s_ref = coff_hdr.f_nsyms++;
		}
	}

	/* Write section name symbols. */
	for (i = C_TEXT_SEG, shp = &scn_hdr[i]; i < nsecs; ++i, ++shp) {
		memset(sym.n_name, 0, sizeof(sym.n_name));
		strcpy(sym.n_name, shp->s_name);
		sym.n_value = shp->s_vaddr;
		sym.n_scnum = i + 1;
		sym.n_type = T_NULL;			/* no type */
		sym.n_sclass = C_STAT;
		sym.n_numaux = (db_flag) ? 1 : 0;	/* aux entries */
		owrite((char *)&sym, sizeof(sym));
		if (db_flag) {
			/* Write aux entries for sections if debug. */
			memset(&aux, 0, sizeof(aux));
			aux.ae_scnlen = shp->s_size;
			aux.ae_nreloc = shp->s_nreloc;
			aux.ae_nlinno = shp->s_nlnno;
			owrite((char *)&aux, sizeof(aux));
		}
	}

	/* Write remaining symbols. */
	str_size = sizeof(long);		/* initial string section size */
	str_seek = dseek + coff_hdr.f_nsyms * sizeof(sym) + str_size;	/* first string seek */
	if (db_flag) {
		while (db_list != NULL)
			write_dbsym();
	} else {
	    /* Scan the C symbol table again and dump the appropriate symbols. */
	    for (i = 0; i < NSHASH; ++i) {
		for (sp = hash2[i]; sp != NULL; sp = sp->s_fp) {
			if (is_global(sp) || (!is_label(sp) && !is_def(sp)))
				write_sym(sp, C_EXT);
			else if (isvariant(VXSTAT) && !is_global(sp)
			        && !is_label(sp) && is_def(sp))
				write_sym(sp, C_STAT);
		}
	    }
	}
	/* Write string table size if nonempty. */
	if (str_size != sizeof(long))
		owrite((char *)&str_size, sizeof(str_size));	
}

/*
 * Write a symbol.
 */
write_sym(sp, class) register SYM *sp;
{
	register int len;
	SYMENT sym;

	len = strlen(sp->s_id);
	if (len <= SYMNMLEN)
		strncpy(sym.n_name, sp->s_id, SYMNMLEN);
	else {
		/* Spill long name to string table. */
		write_string(&sym, sp->s_id);
	}
	sym.n_value = sp->s_value;
	if (is_def(sp))
		sym.n_scnum = sec_index[sp->s_seg] + 1;
	else
		sym.n_scnum = N_UNDEF;
	sym.n_type = T_NULL;			/* no type */
	sym.n_sclass = class;
	sym.n_numaux = 0;			/* no aux entries */
	owrite((char *)&sym, sizeof(sym));
}

/*
 * Write a long string name to the string section.
 */
write_string(sp, id) SYMENT *sp; register char *id;
{
	register long seek;
	register int len;

	sp->n_zeroes = 0L;
	sp->n_offset = str_size;
	len = strlen(id) + 1;
	seek = ftell(ofp);
	oseek(str_seek);
	sput(id);
	str_seek += len;
	str_size += len;
	oseek(seek);
}

/*
 * Write the line number entries.
 */
write_lnnums()
{
	register LNNUM *lp;
	LINENO lineno;

	while ((lp = ln_list) != NULL) {
		lineno.l_lnno = lp->ln_lnnum;
		if (lp->ln_flag == LN_FUNC)		/* function index */
			lineno.l_addr.l_symndx = lp->ln_val;
		else if (lp->ln_flag == LN_LINE)	/* line physical address */
			lineno.l_addr.l_paddr = lp->ln_addr;
		else
			cbotch("write_lnnums");
		owrite(&lineno, sizeof lineno);
		ln_list = lp->ln_next;
		free(lp);		
	}
}

/*
 * Write a single DBSYM item to the symbol table.
 * Free the item from the list.
 */
write_dbsym()
{
	register DBSYM *dp;
	register SYMENT *sp;
	register int n;

	if ((dp = db_list) == NULL)
		cfatal("incomplete debug information");
	sp = &(dp->db_se);

	/* Adjust .data and .bss locations by section base. */
	n = sp->n_scnum - 1;
	if (dp->db_seg != -1)
		sp->n_value += seg[dp->db_seg].s_mseek;

	/* Spill long name to the string table. */
	if (is_lname(dp))
		write_string(sp, dp->db_name);
	
	/* Write the symbol entry. */
	owrite(sp, sizeof *sp);

	/* Free the entry. */
	db_list = dp->db_next;
	free(dp);
}

/*
 * Set reference number of symbol sp by searching the debug symbol list,
 * so that fixups targeting it get the right symbol number.
 * This is used only if debug.
 */
setrefnum(sp) register SYM *sp;
{
	static int ignore = -1;
	register DBSYM *dp;
	register char *name;

	/* If the user specifies -VTPROF but not -g, ignore the debug info. */
	if (ignore == -1)				/* set it first time */
		ignore = (isvariant(VTPROF) && notvariant(VLINES)
			 && notvariant(VTYPES) && notvariant(VDSYMB));
	if (ignore)
		return;

	name = sp->s_id;
	for (dp = db_list; dp != NULL; dp = dp->db_next)
		if (!is_aux(dp)				/* SYMENT not AUXENT */
		 && dp->db_level == 0			/* global */
		 && db_strcmp(dp, name) == 0)		/* matching name */
			break;				/* gotcha */
	if (dp == NULL) {
		/*
		 * Function calls generated by the compiler (e.g. "memcpy"
		 * for structure assignment) may have no associated debug item,
		 * so create an appropriate External entry as required.
		 */
		if (sp->s_seg != SANY)
			cfatal("incomplete debug information (%s)", sp->s_id);
		dp = new_sym(sp->s_id, sp->s_value, N_UNDEF, T_NULL, C_EXT);
	}
	sp->s_ref = dp->db_ref;
}

/*
 * Compare two names like strcmp() does, but the first arg is a DBSYM *.
 */
int
db_strcmp(dp, s) register DBSYM *dp; register char *s;
{
	register int status;

	if (is_lname(dp))
		return strcmp(dp->db_name, s);		/* compare long db name */
	status = strncmp(dp->db_se.n_name, s, 8);	/* compare short db name */
	if (status != 0)
		return status;				/* mismatch */
	return (strlen(s) <= 8) ? status : 1;		/* first 8 match */
}

/* Buffering and output writing routines. */
/*
 * Make sure there is enough room in the text and relocation buffers
 * for nt bytes of text and nr bytes of relocation.
 */
enuf(nt, nr) int nt, nr;
{
	if (txtp+nt > &txt[NTXT] || relp+nr > &rel[NREL])
		notenuf();
}

/*
 * Flush the text and relocation buffers.
 * Look at the segment table to figure out the exact location
 * of the data in the file, and compute the correct seek
 * address in the image by adding the base address of
 * the text record "txtdot" to that base.
 */
notenuf()
{
	register int n;

	if ((n = txtp-txt) != 0) {
		dbprintf(("notenuf: %d text bytes\n", n));
		dbprintf(("seek to txtdot=%d + seg[%d].s_dseek=%ld\n", txtdot, dotseg, seg[dotseg].s_dseek));
		oseek(txtdot + seg[dotseg].s_dseek);
		owrite(txt, n);
		if ((n = relp-rel) != 0) {
			dbprintf(("notenuf: %d reloc bytes\n", n));
			dbprintf(("seek to reldot[%d]=%d + relptr=%ld\n", dot_sec, reldot[dot_sec], scn_hdr[dot_sec].s_relptr));
			oseek(reldot[dot_sec] + scn_hdr[dot_sec].s_relptr);
			owrite(rel, n);
			reldot[dot_sec] += n;
			relp = rel;
		}
	}
	txtp = txt;
	txtdot = dot;
}

/*
 * Seek output file, checking for seek error.
 */
oseek(l) long l;
{
	if (fseek(ofp, l, SEEK_SET) == -1L)
		cfatal("seek to %ld failed", l);
}

/*
 * Write a block of n bytes from p to the output file,
 * checking for write errors.
 */
owrite(p, n) char *p; register int n;
{
	if (fwrite(p, sizeof(char), n, ofp) != n)
		cfatal("output write error");
}

/* end of n2/i386/outcoff.c */
