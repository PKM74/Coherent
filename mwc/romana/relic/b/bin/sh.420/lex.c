/*
 * sh/lex.c
 * Bourne shell.
 * Lexical analysis.
 */

#include "sh.h"
#include "y.tab.h"

/*
 * Local externals.
 */
int	lastget = '\0';		/* Pushed back character */
int	eolflag = 0;		/* End of line */

/*
 * For processing here documents.
 */
char	*hereeof = NULL;	/* Here document EOF mark */
int	herefd;			/* Here document fd */
char	*heretmp;		/* Here document tempfile name */
int	hereqflag;		/* Here document quoted */

/*
 * Keyword table.
 */
typedef	struct	key {
	int	k_hash;			/* Hash */
	int	k_lexv;			/* Lexical value */
	char	*k_name;		/* Keyword name */
} KEY;

/*
 * Keyword table.
 */
KEY keytab[] ={
	0,	_CASE,	"case",
	0,	_DO,	"do",
	0,	_DONE,	"done",
	0,	_ELIF,	"elif",
	0,	_ELSE,	"else",
	0,	_ESAC,	"esac",
	0,	_FI,	"fi",
	0,	_FOR,	"for",
	0,	_IF,	"if",
	0,	_IN,	"in",
	0,	_RET,	"return",
	0,	_THEN,	"then",
	0,	_UNTIL,	"until",
	0,	_WHILE,	"while",
	0,	_OBRAC,	"{",
	0,	_CBRAC, "}"
};
#define	NKEYS	(sizeof(keytab) / sizeof(keytab[0]))
	
/*
 * Get the next lexical token.
 */
yylex()
{
	register int c;
	register KEY *kp;
	int hash;

	if (keytab[0].k_hash == 0)
		for (kp = & keytab [0] ; kp < & keytab [NKEYS] ; kp ++)
			kp->k_hash = ihash (kp->k_name);
again:
	while ((c = getn ()) == ' ' || c == '\t')
		/* DO NOTHING */ ;
	strp = strt;
	if (c == '#' && readflag == 0) {
		/*
		 * Ignore a '#'-delimited comment line.
		 * Lines which begin with a ':' token are lexed as usual;
		 * the built-in function s_colon() executes (i.e. ignores)
		 * lines starting with ':', while other ':' tokens get passed.
		 * The built-in "read" does not ignore comment lines.
		 */
		do
			c = getn ();
		while (c > 0 && c != '\n');
		return c;
	} else if (class (c, MDIGI)) {
		* strp ++ = c;
		c = getn ();
		if (c == '>' || c == '<') {
			* strp ++ = c;
			return lexiors (c);
		}
		ungetn (c);
		return lexname ();
	}
	if (! class (c, MNAME)) {
		if (c >= 0)
			ungetn (c);
		if ((c = lexname ()) == 0)
			goto again;
		else if (c < 0)
			return c;

		hash = ihash (strt);

		if (keyflag) {
			for (kp = keytab; kp < & keytab [NKEYS] ; kp ++)
				if (hash == kp->k_hash &&
				    strcmp (strt, kp->k_name) == 0) {
					return kp->k_lexv;
				}
		}

		return c;
	}
	* strp ++ = c;
	* strp = '\0';

	switch (c) {
	case ';':
		return isnext (c, _DSEMI);

	case '>':
		return lexiors (c);

	case '<':
		return lexiors (c);

	case '&':
		return isnext (c, _ANDF);

	case '|':
#ifdef NAMEPIPE
		if ( ! isnext (')', 0))
			return _NCLOSE;
#endif
		return isnext (c, _ORF);

#ifdef NAMEPIPE
	case '(':
		return isnext ('|', _NOPEN);
#else
	case '(':
		return isnext (')', _PARENS);
#endif

	default:
		if (hereeof != NULL) {
			/* Read here document. */
			for (;;) {
				strp = strt;
				if ((c = collect ('\n', NO_ERRORS)) < 0)
					break;
				* strp = '\0';
				if (strcmp (strt, hereeof) == 0)
					break;
				if (herefd < 0)
					continue;
				if (! hereqflag && strp > strt + 1 &&
				    strp [-2] == '\\')
					* (strp -= 2) = '\0';
				if (! hereqflag && * strt == '\\' &&
				    strcmp (hereeof, strt + 1) == 0) {
					write (herefd, strt + 1,
					       strp - strt - 1);
				} else
					write (herefd, strt, strp - strt);
			}
			close (herefd);
			remember_temp (heretmp);
			hereeof = NULL;
			return '\n';
		}

		return c;
	}
}

isnext(c, t1)
register int c;
{
	register int c2;

	if ((c2 = getn ()) == c) {
		* strp ++ = c2;
		* strp = '\0';
		return t1;
	}
	ungetn (c2);
	return strp [-1];
}

/*
 * Read stuff delimited by (possibly nested) '{' '}' pairs.
 */

void getcurlies () {
	int		c;
	int		quote = 0;

	for (;;) {
		if ((c = getn ()) < 0 || c == '\n')
			emisschar ();

		if (strp >= strt + STRSIZE)
			etoolong ("in getcurlies ()");

		switch (* strp ++ = c) {
		case '}':
			if (! quote)
				return;
			continue;

		case '"':
			quote ^= 1;
			continue;

		case '\'':
			if ((c = collect ('\'', NO_BACKSLASH)) != '\'')
				break;
			continue;

		case '\\':
			if ((c = getn ()) < 0) {
				syntax ();
				break;
			}
			if (c == '\n') {
				strp --;
				continue;
			}
			* strp ++ = c;
			continue;

		case '$':
			c = getn ();
			if (c == '{') {
				* strp ++ = c;
				getcurlies ();
			} else
				ungetn(c);
			continue;

		case '`':
			if ((c = collect ('`', BACKSLASH_END)) != '`')
				break;
			continue;
		}
	}
}


/*
 * Scan a single argument.
 *	Return 0 if it's an escaped newline, EOF if EOF is found,
 *	or _NAME or _ASGN if any part of an argument is found.
 */
lexname()
{
	int q, asgn;
	register int c, m;
	register char *cp;

	q = 0;
	asgn = 0;
	m = MNQUO;
	cp = strp;
	for (;;) {
		c = getn ();
		if (asgn == 0)
			asgn = class (c, MBVAR) ? 1 : -1;
		else if (asgn == 1)
			asgn = class (c, MRVAR) ? 1 : (c == '=' ? 2 : -1);
		if (cp >= strt + STRSIZE)
			etoolong ("in lexname ()");
		else
			* cp++ = c;
		if (! class (c, m))
			continue;
		switch (c) {
		case '"':
			m = (q ^= 1) ? MDQUO : MNQUO;
			continue;

		case '\'':
			strp = cp;
			c = collect ('\'', NO_BACKSLASH);
			cp = strp;
			if (c != '\'')
				break;
			continue;

		case '\\':
			if ((c = getn ()) < 0) {
				syntax ();
				break;
			}
			if (c == '\n') {
				ungetn ((c = getn ()) < 0 ? '\n' : c);
				if (-- cp == strp)
					return 0;
				continue;
			}
			* cp ++ = c;
			continue;

		case '$':
			c = getn ();
			if (c == '{') {
				* cp ++ = c;
				strp = cp;
				getcurlies ();
				cp = strp;
			} else
				ungetn (c);
			continue;

		case '`':
			strp = cp;
			c = collect ('`', BACKSLASH_END);
			cp = strp;
			if (c != '`')
				break;
			continue;

		case '\n':
			if (q)
				continue;
			break;
		}
		break;
	}

#if 0
	if (c < 0)
		return c;
#endif

	if (q) {
		emisschar ('"');
		* cp = '\0';
	} else
		* -- cp = '\0';

	if (c >= 0)
		ungetn (c);
	strp = cp;

#ifdef VERBOSE
	if (vflag)
	prints("\t<%d> <%s> %s\n", getpid(), (asgn==2 ? "ASGN" : "NAME"), strt);
#endif
	if (errflag)
		return _NULL;
	else if (asgn == 2)
		return _ASGN;
	else
		return _NAME;
}

/*
 * Lex an io redirection string, including the file name if any.
 *	Called with one '>' or '<' in buffer, optionally preceded by
 *	a digit.
 */
lexiors(c1)
{
	register int c;
	register char *name;
	char *iors;

	* strp ++ = c = getn ();
	if (c == '&') {
		* strp ++ = c = getn ();
		* strp = '\0';
		if (c < 0)
			return c;
		if (c != '-' && ! class (c, MDIGI))
			eredir ();
		return _IORS;
	}
	if (c == c1)
		c1 += 0200;
	else {
		* -- strp = '\0';
		ungetn (c);
	}

	/* Collect file name */
	while ((c = getn ()) == ' ' || c == '\t')
		* strp ++ = c;
	ungetn (c);
	name = strp;
	if (c == '\n') {
		eredir ();
		return _IORS;
	}

	while ((c = lexname ()) == 0)
		/* DO NOTHING */ ;

	if (c < 0)
		return c;

	if (c1 != '<' + 0200)
		return _IORS;

	/*
	 * Set up here document processing.
	 * Modified by steve 1/25/91 so that
	 * the actual processing happens at the '\n' ending the line,
	 * otherwise the common "foo <<SHAR_EOF >baz\n" does not work.
	 * This code is anything but obvious, it could doubtless be simpler.
	 */
	strp = strt;
	/* Simplify quoted here document iors from ?<<file to ?<file. */
	if ((hereqflag = strpbrk (name, "\"\\'") != NULL) != 0)
		* ++ strp = * strt;
	heretmp = name;
	name = duplstr (name, 0);
	strcpy (heretmp, shtmp ());
	iors = duplstr (strp, 0);
	heretmp += iors - strp;
	eval (name, EWORD);
	hereeof = duplstr (strcat (strt, "\n"), 0);
	if ((herefd = creat (heretmp, 0666)) < 0)
		ecantmake (heretmp);
	strcpy (strt, iors);
	return _IORS;
}

/*
 * Collect characters until the end character is found.  If 'f' is
 * CONSUME_BACKSLASH, '\' escapes the next character and newline is
 * not allowed. If 'f' is BACKSLASH_END, backslashes are retained but
 * suppress recognition of the end-character. If 'f' is NO_BACKSLASH,
 * all characters are passed through. If 'f' is NO_ERRORS, behave as
 * NO_BACKSLASH but be quiet about errors.
 */
collect(ec, f)
register int ec;
{
	register int c;
	register char *cp;

	cp = strp;
	while ((c = getn ()) != ec) {
backslashed:
		/*
		 * NIGEL: Originally, this routine complained about missing
		 * characters at EOF, but System V just implicitly closes off
		 * any strings in this situation. In order to stroke the
		 * lexer, we mutate EOF's into EOL's.
		 */
		if (c < 0) {
			if (f != NO_ERRORS) {
				c = ec;
				break;
			}
			return c;
		}
		if (c == '\n' && f == CONSUME_BACKSLASH) {
			if (f != NO_ERRORS)
				emisschar (ec);
			return c;
		}
		if (c == '\\' && f == CONSUME_BACKSLASH) {
			if ((c = getn ()) < 0) {
				syntax ();
				return c;
			}
			if (c == '\n')
				continue;
		}
		if (cp >= strt + STRSIZE)
			etoolong ("in collect ()");
		else
			* cp ++ = c;
		if (c == '\\' && f == BACKSLASH_END) {
			if ((c = getn ()) != '\n')
				goto backslashed;
			cp --;
		}
	}
	* cp ++ = ec;
	strp = cp;
	return ec;
}


/*
 * Get a character.
 */

getn()
{
	register int c;
	register int t;

	if (lastget != '\0') {
		c = lastget;
		lastget = '\0';
		return c;
	}
	switch (t = sesp->s_type) {
	case SSTR:
	case SFILE:
		yyline += eolflag;
		eolflag = 0;
		if (prpflag && sesp->s_flag) {
			if (prpflag -= 1) {
				prompt("\n");
				prpflag -= 1;
			}
			prompt (comflag ? vps1 : vps2);
			comflag = 0;
		}
		if ((c = getc (sesp->s_ifp)) == '\n') {
			if (sesp->s_flag) {
				prpflag = 1;
				yyline = 1;
			} else
				eolflag = 1;
		}
		if (vflag)
			putc(c, stderr);
		return c;
	case SARGS:
	case SARGV:
		if (sesp->s_flag)
			return EOF;
		if ((c = * sesp->s_strp ++) == '\0') {
			if (t == SARGV &&
			    (sesp->s_strp = * ++ sesp->s_argv) != NULL)
				c = ' ';
			else {
				sesp->s_flag = 1;
				c = EOF;
			}
		}
		if (vflag)
			putc (c, stderr);
		return c;
	}
}


/*
 * Unget a character.
 */

ungetn(c)
{
	lastget = c;
}
