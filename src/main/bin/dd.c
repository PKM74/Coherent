/*
 * DD - file conversion and copying
 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#ifndef	TYPES_H
#include <sys/types.h>
#include <sys/mdata.h>
#endif

/* Conversions */
#define	DD_ASC	01		/* EBCDIC->ASCII */
#define	DD_EBC	02		/* ASCII->EBCDIC */
#define	DD_IBM	04		/* ASCII-> special print EBCDIC */
#define	DD_LCA	010		/* alphabetics to lower case */
#define	DD_UCA	020		/* alphabetics to upper case */
#define	DD_SWAB	040		/* Swap byte pairs */
#define	DD_NERR	0100		/* Ignore errors */
#define	DD_SYNC	0200		/* Pad input records to `ibs' */
#define	DD_COPY	(DD_ASC|DD_EBC|DD_IBM)	/* Requires copy */

#define	BSIZE	BUFSIZ
#define	streq(a,b)	strcmp(a,b)==0

/*
 * Conversion table from EBCDIC to ASCII.
 * This is from Nov. 1968, CACM (p 788).
 * It includes conversions of some controls in
 * EBCDIC up to characters that are >0200 which
 * is somewhat non-standard.
 */
char	etoatab[256] = {
	0000 /*NUL*/,	0001 /*SOH*/,	0002 /*STX*/,	0003 /*ETX*/,
	0234 /*K28*/,	0011 /*HT */,	0206 /*K6 */,	0177 /*DEL*/,
	0227 /*K23*/,	0215 /*K13*/,	0216 /*K14*/,	0013 /*VT */,
	0014 /*FF */,	0015 /*CR */,	0016 /*SO */,	0017 /*SI */,
	0020 /*DLE*/,	0021/*DC1*/,	0022 /*DC2*/,	0023 /*DC3*/,
	0235 /*K29*/,	0205 /*K5 */,	0010 /*BS */,	0207 /*K7 */,
	0030 /*CAN*/,	0031 /*EM */,	0222 /*K18*/,	0217 /*K15*/,
	0034 /*FS */,	0035 /*GS */,	0036 /*RS */,	0037 /*US */,
	0200 /*K0 */,	0201 /*K1 */,	0202 /*K2 */,	0203 /*K3 */,
	0204 /*K4 */,	0012 /*LF */,	0027 /*ETB*/,	0033 /*ESC*/,
	0210 /*K8 */,	0211 /*K9 */,	0212 /*K10*/,	0213 /*K11*/,
	0214 /*K12*/,	0005 /*ENQ*/,	0006 /*ACK*/,	0007 /*BEL*/,
	0220 /*K16*/,	0221 /*K17*/,	0026 /*SYN*/,	0223 /*K19*/,
	0224 /*K20*/,	0225 /*K21*/,	0226 /*K22*/,	0004 /*EOT*/,
	0230 /*K24*/,	0231 /*K25*/,	0232 /*K26*/,	0233 /*K27*/,
	0024 /*DC4*/,	0025 /*NAK*/,	0236 /*K30*/,	0032 /*SUB*/,
	0040 /*   */,	0240 /*N0 */,	0241 /*N1 */,	0242 /*N2 */,
	0243 /*N3 */,	0244 /*N4 */,	0245 /*N5 */,	0246 /*N6 */,
	0247 /*N7 */,	0250 /*N8 */,	0133 /* [ */,	0056 /* . */,
	0074 /* < */,	0050 /* ( */,	0053 /* + */,	0041 /* ! */,
	0046 /* & */,	0251 /*N9 */,	0252 /*N10*/,	0253 /*N11*/,
	0254 /*N12*/,	0255 /*N13*/,	0256 /*N14*/,	0257 /*N15*/,
	0260 /*N16*/,	0261 /*N17*/,	0135 /* ] */,	0044 /* $ */,
	0052 /* * */,	0051 /* ) */,	0073 /* ; */,	0136 /* ^ */,
	0055 /* - */,	0057 /* / */,	0262 /*N18*/,	0263 /*N19*/,
	0264 /*N20*/,	0265 /*N21*/,	0266 /*N22*/,	0267 /*N23*/,
	0270 /*N24*/,	0271 /*N25*/,	0174 /* | */,	0054 /* , */,
	0045 /* % */,	0137 /* _ */,	0076 /* > */,	0077 /* ? */,
	0272 /*N26*/,	0273 /*N27*/,	0274 /*N28*/,	0275 /*N29*/,
	0276 /*N30*/,	0277 /*N31*/,	0300 /*N32*/,	0301 /*N33*/,
	0302 /*N34*/,	0140 /* ` */,	0072 /* : */,	0043 /* # */,
	0100 /* @ */,	0047 /* ' */,	0075 /* = */,	0042 /* " */,
	0303 /*N35*/,	0141 /* a */,	0142 /* b */,	0143 /* c */,
	0144 /* d */,	0145 /* e */,	0146 /* f */,	0147 /* g */,
	0150 /* h */,	0151 /* i */,	0304 /*N36*/,	0305 /*N37*/,
	0306 /*N38*/,	0307 /*N39*/,	0310 /*N40*/,	0311 /*N41*/,
	0312 /*N42*/,	0152 /* j */,	0153 /* k */,	0154 /* l */,
	0155 /* m */,	0156 /* n */,	0157 /* o */,	0160 /* p */,
	0161 /* q */,	0162 /* r */,	0313 /*N43*/,	0314 /*N44*/,
	0315 /*N45*/,	0316 /*N46*/,	0317 /*N47*/,	0320 /*N48*/,
	0321 /*N49*/,	0176 /* ~ */,	0163 /* s */,	0164 /* t */,
	0165 /* u */,	0166 /* v */,	0167 /* w */,	0170 /* x */,
	0171 /* y */,	0172 /* z */,	0322 /*N50*/,	0323 /*N51*/,
	0324 /*N52*/,	0325 /*N53*/,	0326 /*N54*/,	0327 /*N55*/,
	0330 /*N56*/,	0331 /*N57*/,	0332 /*N58*/,	0333 /*N59*/,
	0334 /*N60*/,	0335 /*N61*/,	0336 /*N62*/,	0337 /*N63*/,
	0340 /*G0 */,	0341 /*G1 */,	0342 /*G2 */,	0343 /*G3 */,
	0344 /*G4 */,	0345 /*G5 */,	0346 /*G6 */,	0347 /*G7 */,
	0173 /* { */,	0101 /* A */,	0102 /* B */,	0103 /* C */,
	0104 /* D */,	0105 /* E */,	0106 /* F */,	0107 /* G */,
	0110 /* H */,	0111 /* I */,	0350 /*G8 */,	0351 /*G9 */,
	0352 /*G10*/,	0353 /*G11*/,	0354 /*G12*/,	0355 /*G13*/,
	0175 /* } */,	0112 /* J */,	0113 /* K */,	0114 /* L */,
	0115 /* M */,	0116 /* N */,	0117 /* O */,	0120 /* P */,
	0121 /* Q */,	0122 /* R */,	0356 /*G14*/,	0357 /*G15*/,
	0360 /*G16*/,	0361 /*G17*/,	0362 /*G18*/,	0363 /*G19*/,
	0134 /* \ */,	0237 /*K31*/,	0123 /* S */,	0124 /* T */,
	0125 /* U */,	0126 /* V */,	0127 /* W */,	0130 /* X */,
	0131 /* Y */,	0132 /* Z */,	0364 /*G20*/,	0365 /*G21*/,
	0366 /*G22*/,	0367 /*G23*/,	0370 /*G24*/,	0371 /*G25*/,
	0060 /* 0 */,	0061 /* 1 */,	0062 /* 2 */,	0063 /* 3 */,
	0064 /* 4 */,	0065 /* 5 */,	0066 /* 6 */,	0067 /* 7 */,
	0070 /* 8 */,	0071 /* 9 */,	0372 /*G26*/,	0373 /*G27*/,
	0374 /*G28*/,	0375 /*G29*/,	0376 /*G30*/,	0377 /*EO */,
};

/*
 * Conversion table from ASCII to standard
 * EBCDIC (CACM).
 */
char	atoetab[128] = {
	0x00 /*NUL*/,	0x01 /*SOH*/,	0x02 /*STX*/,	0x03 /*ETX*/,
	0x37 /*EOT*/,	0x2D /*ENQ*/,	0x2E /*ACK*/,	0x2F /*BEL*/,
	0x16 /*BS */,	0x05 /*HT */,	0x25 /*LF */,	0x0B /*VT */,
	0x0C /*FF */,	0x0D /*CR */,	0x0E /*SO */,	0x0F /*SI */,
	0x10 /*DLE*/,	0x11 /*DC1*/,	0x12 /*DC2*/,	0x13 /*DC3*/,
	0x3C /*DC4*/,	0x3D /*NAK*/,	0x32 /*SYN*/,	0x26 /*ETB*/,
	0x18 /*CAN*/,	0x19 /*EM */,	0x3F /*SUB*/,	0x27 /*ESC*/,
	0x1C /*FS */,	0x1D /*GS */,	0x1E /*RS */,	0x1F /*US */,
	0x40 /*   */,	0x4F /* ! */,	0x7F /* " */,	0x7B /* # */,
	0x5B /* $ */,	0x6C /* % */,	0x50 /* & */,	0x7D /* ' */,
	0x4D /* ( */,	0x5D /* ) */,	0x5C /* * */,	0x4E /* + */,
	0x6B /* , */,	0x60 /* - */,	0x4B /* . */,	0x61 /* / */,
	0xF0 /* 0 */,	0xF1 /* 1 */,	0xF2 /* 2 */,	0xF3 /* 3 */,
	0xF4 /* 4 */,	0xF5 /* 5 */,	0xF6 /* 6 */,	0xF7 /* 7 */,
	0xF8 /* 8 */,	0xF9 /* 9 */,	0x7A /* : */,	0x5E /* ; */,
	0x4C /* < */,	0x7E /* = */,	0x6E /* > */,	0x6F /* ? */,
	0x7C /* @ */,	0xC1 /* A */,	0xC2 /* B */,	0xC3 /* C */,
	0xC4 /* D */,	0xC5 /* E */,	0xC6 /* F */,	0xC7 /* G */,
	0xC8 /* H */,	0xC9 /* I */,	0xD1 /* J */,	0xD2 /* K */,
	0xD3 /* L */,	0xD4 /* M */,	0xD5 /* N */,	0xD6 /* O */,
	0xD7 /* P */,	0xD8 /* Q */,	0xD9 /* R */,	0xE2 /* S */,
	0xE3 /* T */,	0xE4 /* U */,	0xE5 /* V */,	0xE6 /* W */,
	0xE7 /* X */,	0xE8 /* Y */,	0xE9 /* Z */,	0x4A /* [ */,
	0xE0 /* \ */,	0x5A /* ] */,	0x5F /* ^ */,	0x6D /* _ */,
	0x79 /* ` */,	0x81 /* a */,	0x82 /* b */,	0x83 /* c */,
	0x84 /* d */,	0x85 /* e */,	0x86 /* f */,	0x87 /* g */,
	0x88 /* h */,	0x89 /* i */,	0x91 /* j */,	0x92 /* k */,
	0x93 /* l */,	0x94 /* m */,	0x95 /* n */,	0x96 /* o */,
	0x97 /* p */,	0x98 /* q */,	0x99 /* r */,	0xA2 /* s */,
	0xA3 /* t */,	0xA4 /* u */,	0xA5 /* v */,	0xA6 /* w */,
	0xA7 /* x */,	0xA8 /* y */,	0xA9 /* z */,	0xC0 /* { */,
	0x6A /* | */,	0xD0 /* } */,	0xA1 /* ~ */,	0x07 /*DEL*/,
};

/*
 * Conversion table from ASCII to ibm print
 * codes (a version of EBCDIC that may be more
 * useful).
 */
char	atoietab[128] = {
	0x00 /*NUL*/,	0x01 /*SOH*/,	0x02 /*STX*/,	0x03 /*ETX*/,
	0x37 /*EOT*/,	0x2D /*ENQ*/,	0x2E /*ACK*/,	0x2F /*BEL*/,
	0x16 /*BS */,	0x05 /*HT */,	0x25 /*LF */,	0x0B /*VT */,
	0x0C /*FF */,	0x0D /*CR */,	0x0E /*SO */,	0x0F /*SI */,
	0x10 /*DLE*/,	0x11 /*DC1*/,	0x12 /*DC2*/,	0x13 /*DC3*/,
	0x3C /*DC4*/,	0x3D /*NAK*/,	0x32 /*SYN*/,	0x26 /*ETB*/,
	0x18 /*CAN*/,	0x19 /*EM */,	0x3F /*SUB*/,	0x27 /*ESC*/,
	0x1C /*FS */,	0x1D /*GS */,	0x1E /*RS */,	0x1F /*US */,
	0x40 /*   */,	0x5A /* !**/,	0x7F /* " */,	0x7B /* # */,
	0x5B /* $ */,	0x6C /* % */,	0x50 /* & */,	0x7D /* ' */,
	0x4D /* ( */,	0x5D /* ) */,	0x5C /* * */,	0x4E /* + */,
	0x6B /* , */,	0x60 /* - */,	0x4B /* . */,	0x61 /* / */,
	0xF0 /* 0 */,	0xF1 /* 1 */,	0xF2 /* 2 */,	0xF3 /* 3 */,
	0xF4 /* 4 */,	0xF5 /* 5 */,	0xF6 /* 6 */,	0xF7 /* 7 */,
	0xF8 /* 8 */,	0xF9 /* 9 */,	0x7A /* : */,	0x5E /* ; */,
	0x4C /* < */,	0x7E /* = */,	0x6E /* > */,	0x6F /* ? */,
	0x7C /* @ */,	0xC1 /* A */,	0xC2 /* B */,	0xC3 /* C */,
	0xC4 /* D */,	0xC5 /* E */,	0xC6 /* F */,	0xC7 /* G */,
	0xC8 /* H */,	0xC9 /* I */,	0xD1 /* J */,	0xD2 /* K */,
	0xD3 /* L */,	0xD4 /* M */,	0xD5 /* N */,	0xD6 /* O */,
	0xD7 /* P */,	0xD8 /* Q */,	0xD9 /* R */,	0xE2 /* S */,
	0xE3 /* T */,	0xE4 /* U */,	0xE5 /* V */,	0xE6 /* W */,
	0xE7 /* X */,	0xE8 /* Y */,	0xE9 /* Z */,	0xAD /* [**/,
	0xE0 /* \ */,	0xBD /* ]**/,	0x4A /* ^**/,	0x6D /* _ */,
	0x7D /* `**/,	0x81 /* a */,	0x82 /* b */,	0x83 /* c */,
	0x84 /* d */,	0x85 /* e */,	0x86 /* f */,	0x87 /* g */,
	0x88 /* h */,	0x89 /* i */,	0x91 /* j */,	0x92 /* k */,
	0x93 /* l */,	0x94 /* m */,	0x95 /* n */,	0x96 /* o */,
	0x97 /* p */,	0x98 /* q */,	0x99 /* r */,	0xA2 /* s */,
	0xA3 /* t */,	0xA4 /* u */,	0xA5 /* v */,	0xA6 /* w */,
	0xA7 /* x */,	0xA8 /* y */,	0xA9 /* z */,	0x8B /* {**/,
	0x4F /* |**/,	0x9B /* }**/,	0x5F /* ~**/,	0x07 /*DEL*/,
};

/*
 * Variables from options.
 */
char	*ifname;
char	*ofname;
FILE	*ifp = stdin;
FILE	*ofp = stdout;
int	ibs;
int	obs;
int	cbs;
char	*ibp;			/* Input buffer ptr. */
char	*obp;			/* Output buffer ptr. */
char	*cbp;			/* Intermediate conversion buffer (ebcdic) */
long	skip;
unsigned long	files;
fsize_t	seek;
unsigned long	count = MAXULONG; /* Limit on records */
int	conv;			/* Flags above */

long	nfri;			/* Full records in */
long	npri;			/* Partial records in */
long	nfro;			/* Full records out */
long	npro;			/* Partial records out */

char	nospace[] = "Out of memory for buffers";

int	stats();
long	number();
FILE	*openfile();

extern	int	errno;

main(argc, argv)
char *argv[];
{
	register char *op;
	register char *ov;
	register int i;
	register int difbuf = 0;
	extern char *index();

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, stats);
	for (i=1; i<argc; i++) {
		if ((ov = index(op=argv[i], '=')) == NULL)
			dderr("Missing `=' in option `%s'", op);
		*ov++ = '\0';
		if (*ov == '\0')
			dderr("Missing value in option `%s'", op);
		if (streq(op, "if"))
			ifp = openfile(ov, &ifname, 0);
		else if (streq(op, "of"))
			ofp = openfile(ov, &ofname, 1);
		else if (streq(op, "ibs")) {
			ibs = number(ov);
			difbuf++;
		} else if (streq(op, "obs")) {
			obs = number(ov);
			difbuf++;
		} else if (streq(op, "bs"))
			obs = ibs = number(ov);
		else if (streq(op, "cbs")) {
			cbs = number(ov);
			difbuf++;
		} else if (streq(op, "skip"))
			skip = number(ov);
		else if (streq(op, "files"))
			files = number(ov);
		else if (streq(op, "seek"))
			seek = number(ov);
		else if (streq(op, "count"))
			count = number(ov);
		else if (streq(op, "conv"))
			conversions(ov);
		else
			dderr("`%s' is a bad option", op);
	}
	if (ifname == NULL)
		ifname = "(stdin)";
	if (ofname == NULL)
		ofname = "(stdout)";
	if (conv & (DD_EBC|DD_IBM|DD_ASC)) {
		difbuf++;
		if ((conv&(DD_ASC|DD_EBC))==(DD_ASC|DD_EBC)
		 || (conv&(DD_ASC|DD_IBM))==(DD_ASC|DD_IBM)
		 || (conv&(DD_UCA|DD_LCA))==(DD_UCA|DD_LCA))
			dderr("Conflicting case or character set conversions");
	}
	if (ibs == 0)
		ibs = BSIZE;
	if (obs == 0)
		obs = BSIZE;
	if ((ibp = malloc(ibs)) == NULL)
		dderr(nospace);
	if (difbuf) {
		if ((obp = malloc(obs)) == NULL)
			dderr(nospace);
	} else
		obp = ibp;
	if (conv & (DD_EBC|DD_IBM|DD_ASC)) {
		if (cbs == 0)
			cbs = ibs;
		if ((cbp = malloc(cbs+1)) == NULL)
			dderr(nospace);
	}
	seek *= obs;
	stats(dd());
}

/*
 * Do the actual data copying and
 * conversion operations.
 * Count down all the files.
 */
dd()
{
	register int nf;
	register int rstat = 0;
	register int ddstat;
	register int n;

	if (seek != 0)
		lseek(fileno(ofp), seek, 0);
	while (skip != 0) {
		skip--;
		if ((n = read(fileno(ifp), ibp, ibs))==0 || n==-1)
			break;
	}
	if ((nf = files) == 0)
		nf = 1;
	do {
		ddstat = cbp==NULL ? dd1() : dd2();
		rstat |= ddstat;
		if (ddstat)
			return (rstat);
	} while (--nf);
	return (rstat);
}

/*
 * Easy version of dd.
 * No ebcdic, ascii, or ibm conversions.
 */
dd1()
{
	register int n;

	while (count-- != 0) {
		if ((n = ddread(fileno(ifp), ibp, ibs)) == 0)
			break;
		if (n == -1) {
			if (conv && DD_NERR)
				continue;
			else
				return (1);
		}
		ddconv(ibp, n);
		if (ddput(ibp, n))
			return (1);
	}
	return (ddput(NULL, 0));
}

/*
 * More complicated version of dd that uses
 * the conversion intermediate buffering
 * for ebcdic/ibm or ascii conversions.
 */
dd2()
{
	static char *ecbp;
	register char *bp;
	register char *wcbp;
	register int nb;
	register int n;
	register int s = 0;

	ecbp = cbp+cbs;
	wcbp = cbp;
	while (count-- != 0) {
		if ((nb = ddread(fileno(ifp), ibp, ibs)) == 0)
			break;
		if (nb == -1) {
			if ((conv & DD_NERR) == 0)
				return (1);
			else {
				s = 1;
				continue;
			}
		}
		bp = ibp;
		if (conv & (DD_IBM|DD_EBC)) {
			do {
				if ((n = *wcbp++ = *bp++)=='\n' || wcbp>=ecbp) {
					if (n == '\n')
						wcbp--;
					n = ddconv(cbp, wcbp-cbp);
					if (ddput(cbp, n))
						if ((conv & DD_NERR) == 0)
							return (1);
						else
							s = 1;
					wcbp = cbp;
				}
			} while(--nb);
		} else {
			do {
				*wcbp++ = *bp++;
				if (wcbp >= ecbp) {
					n = ddconv(cbp, cbs);
					if (ddput(cbp, n))
						if ((conv & DD_NERR) == 0)
							return (1);
						else
							s = 1;
					wcbp = cbp;
				}
			} while (--nb);
		}
	}
	if ((n = wcbp-cbp) != 0) {
		n = ddconv(cbp, n);
		if (ddput(cbp, n))
			if ((conv & DD_NERR) == 0)
				return (1);
			else
				s = 1;
	}
	s |= ddput(NULL, 0);
	return (s);
}

/*
 * Do output conversions (e.g. swab)
 * and write the record.  Account
 * for partial records.
 * Return non-zero if non-repeatable error.
 */
ddwrite(bp, nb)
register char *bp;
register unsigned nb;
{
	register unsigned wnb;

	if (nb == 0)
		return (0);
	if (conv & DD_SWAB)
		swab(bp, bp, nb);
	if ((wnb = write(fileno(ofp), bp, nb)) == nb) {
		if (nb == obs)
			nfro++; else
			npro++;
	} else
		perror("dd");
	if (wnb!=nb && (conv&DD_NERR)==0)
		return (1);
	return (0);
}

/*
 * Put `n' bytes into the output buffer.
 * This is maintained internally, and
 * flushed when full.   Also, if called
 * with 0 bytes (at end) flush as well.
 */
ddput(bp, nb)
register char *bp;
register unsigned nb;
{
	static char *eobp;
	static char *cobp;
	register char *wobp;
	register int s = 0;

	if (eobp == NULL) {
		/*
		 * Initialisation.
		 */
		if (ibp == obp)
			return (ddwrite(bp, nb));
		eobp = obp + obs;
		cobp = obp;
	}
	wobp = cobp;
	if (nb == 0) {
		/*
		 * Flush at end.
		 */
		if ((nb = wobp-obp) == 0)
			return (0);
		if (ddwrite(obp, nb) != nb)
			if ((conv & DD_NERR) == 0)
				return (1);
		return (0);
	}
	do {
		*wobp++ = *bp++;
		if (wobp >= eobp) {
			if (ddwrite(obp, obs))
				if ((conv & DD_NERR) == 0)
					return (1);
				else
					s = 1;
			wobp = obp;
		}
	} while (--nb);
	cobp = wobp;
	return (s);
}

/*
 * Read in data.  increment proper counts,
 * do conv=sync and noerrors checking.
 */
ddread(fd, bp, nb)
int fd;
register char *bp;
register int nb;
{
	register int n;

	if ((n = read(fd, bp, nb)) == -1)
		perror("dd");
	else if (n != 0) {
		if (n == nb)
			nfri++; else
			npri++;
		if (conv&DD_SYNC && n<nb) {
			bp += n;
			n = nb-n;
			while (n--)
				*bp++ = 0;
			n = nb;
		}
	}
	return (n);
}

/*
 * Do other conversions.
 * Return new `nb' (useful only for ascii/ebcdic
 * conversions).
 */
ddconv(bp, nb)
char *bp;
unsigned nb;
{
	register char *cp;
	register unsigned n;
	register int c;

	if (conv & DD_ASC) {
		etoa(bp, bp, n=nb);
		for (cp=bp+n; cp>bp; )
			if (*--cp != ' ') {
				cp++;
				break;
			}
		*cp++ = '\n';
		nb = cp-bp;
	}
	if (conv & DD_UCA) {
		for (cp=bp, n=nb; n-- != 0; cp++)
			if (isascii(c=*cp) && islower(c))
				*cp = toupper(c);
	}
	if (conv & DD_LCA) {
		for (cp=bp, n=nb; n-- != 0; cp++)
			if (isascii(c=*cp) && isupper(c))
				*cp = tolower(c);
	}
	if (conv & (DD_EBC|DD_IBM)) {
		if (nb < cbs) {
			n = cbs-nb;
			cp = bp+nb;
			while (n--)
				*cp++ = ' ';
		}
		nb = n = cbs;
		if (conv & DD_IBM)
			atoie(bp, bp, n); else
			atoe(bp, bp, n);
	}
	return (nb);
}

/*
 * Open a file on the input line.
 * `f' is 0 for read, 1 for write.
 */
FILE *
openfile(fn, fnp, f)
register char *fn;
register char **fnp;
int f;
{
	register FILE *fp;
	register int errsave;

	if (*fnp != NULL)
		dderr("more than one `%s' specified", f ? "of=" : "if=");
	*fnp = fn;
	if ((fp = fopen(fn, f?"w":"r")) == NULL) {
		errsave = errno;
		fprintf(stderr, "dd: ");
		errno = errsave;
		perror(fn);
		exit(1);
	}
	return (fp);
}

/*
 * Read a number, scaled by the
 * multipliers `k', `b', or `w'.
 */
long
number(num)
char *num;
{
	register char *s = num;
	long rn, n;
	register int base;

	rn = 1;
again:
	n = 0;
	base = 10;
	if (*s == '0') {
		base = 8;
		s++;
	}
	while (isdigit(*s))
		n = n*base + *s++ - '0';
	for (; *s!='\0'; s++)
		if (*s=='b')
			n *= BSIZE;
		else if (*s == 'w')
			n *= sizeof(int);
		else if (*s == 'k')
			n *= 1024;
		else if (*s == 'x') {
			s++;
			rn *= n;
			goto again;
		} else
			dderr("bad number `%s'", num);
	if (rn != 1)
		return (n*rn);
	return (n);
}

/*
 * Read in the conversions from
 * the string given.
 */
conversions(s)
register char *s;
{
	register char *np = s;

	for (;;) {
		for (;; np++) {
			if (*np == ',')
				*np++ = '\0';
			else if (*np == '\0')
				np = NULL;
			else
				continue;
			break;
		}
		if (streq(s, "ascii"))
			conv |= DD_ASC;
		else if (streq(s, "ebcdic"))
			conv |= DD_EBC;
		else if (streq(s, "ibm"))
			conv |= DD_IBM;
		else if (streq(s, "lcase"))
			conv |= DD_LCA;
		else if (streq(s, "ucase"))
			conv |= DD_UCA;
		else if (streq(s, "swab"))
			conv |= DD_SWAB;
		else if (streq(s, "noerror"))
			conv |= DD_NERR;
		else if (streq(s, "sync"))
			conv |= DD_SYNC;
		else
			dderr("`%s' is an illegal conversion", s);
		if (np == NULL)
			break;
		s = np;
	}
}

/*
 * Convert ascii to ebcdic.
 * This is the CACM standard one.
 */
atoe(ip, op, n)
register unsigned char *ip;
register unsigned char *op;
register unsigned n;
{
	if (n)
		do {
			if (isascii(*ip))
				*op++ = atoetab[*ip++];
			else {
				*op++ = 0;
				ip++;
			}
		} while (--n);
}

/*
 * Convert ascii to `ibm' ebcdic.
 * This is the more reasonable one
 * for printers.
 */
atoie(ip, op, n)
register unsigned char *ip;
register unsigned char *op;
register unsigned n;
{
	if (n)
		do {
			if (isascii(*ip))
				*op++ = atoietab[*ip++];
			else {
				*op++ = 0;
				ip++;
			}
		} while (--n);
}

/*
 * Convert EBCDIC to ASCII.
 * As it turns out, the mappings
 * are both the same.
 */
etoa(ip, op, n)
register unsigned char *ip;
register unsigned char *op;
register unsigned n;
{
	if (n)
		do {
			*op++ = etoatab[*ip++];
		} while (--n);
}

/*
 * Print statistics and exit.
 */
stats(s)
{
	fprintf(stderr, "%ld+%ld records in\n", nfri, npri);
	fprintf(stderr, "%ld+%ld records out\n", nfro, npro);
	exit(s);
}

usage()
{
	fprintf(stderr, "Usage: dd [option=value] ...\n");
	exit(1);
}

/* VARARGS */
dderr(x)
{
	fprintf(stderr, "dd: %r\n", &x);
	exit(1);
}
