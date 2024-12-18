/*
 * /usr/src/libc/gen/atof.c
 * C general utilities library.
 * atof()
 * ANSI 4.10.1.1.
 * Convert ASCII to double (the old fashioned way).
 * Builds significand in an unsigned long, for efficiency.
 * Does not use any knowledge about floating point representation.
 */

#if	__STDC__
#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#else
#define	_decimal_point	'.'
extern	double	_pow10();
#endif
#include <ctype.h>

/* Flag bits. */
#define	NEG	1			/* negative significand	*/
#define	DOT	2			/* decimal point seen	*/
#define	NEGEXP	4			/* negative exponent	*/
#define	BIG	8			/* significand too big for ulong */

double
atof(nptr) register char *nptr;
{
	register int		c, flag, eexp;
	register unsigned long	val;
	int			exp, vdigits;
	double			d;

	val = flag = exp = vdigits = 0;

	/* Leading white space. */
	while (isspace(c = *nptr++))
		;

	/* Optional sign. */
	switch (c) {
	case '-':
		flag |= NEG;
	case '+':
		c = *nptr++;
	}

	/* Number: sequence of decimal digits with optional '.'. */
	for (; ; c = *nptr++) {
		if (isdigit(c)) {
			c -= '0';
			if (c == 0 && (flag & DOT)) {
				/* Check for trailing zeros to avoid imprecision. */
				char *look, d;

				for (look = nptr; (d = *look++) == '0'; )
					;		/* skip a trailing zero */
				if (!isdigit(d)) {	/* ignore zeroes */
					nptr = look;
					c = d;
					break;
				}			/* else don't ignore */
			}
#if	__STDC__
			if (val > (ULONG_MAX-9) / 10) {
#else
			/* The pre-ANSI compiler gets the test above wrong. */
			if (val > 429496728L) {
#endif
				/* Significand too big for val, use d. */
				if (flag & BIG)
					d = d * _pow10(vdigits) + val;	
				else {
					d = val;
					flag |= BIG;
				}
				vdigits = 1;
				val = c;
			} else {
				++vdigits;	/* decimal digits in val */
				/* val = val * 10 + c; */
				val <<= 1;
				val = val + (val << 2) + c;
			}
			if (flag & DOT)
				--exp;
		} else if (c == _decimal_point && (flag & DOT) == 0)
			flag |= DOT;
		else
			break;
	}
	if (flag & BIG)
		d = d * _pow10(vdigits) + val;
	else
		d = val;

	/* Optional exponent: 'E' or 'e', optional sign, decimal digits. */
	if (c == 'e'  ||  c == 'E') {

		/* Optional sign. */
		switch (c = *nptr++) {
		case '-':
			flag |= NEGEXP;
		case '+':
			c = *nptr++;
		}

		/* Decimal digits. */
		for (eexp = 0; isdigit(c); c = *nptr++)
			eexp = eexp * 10 + c - '0';

		/* Adjust explicit exponent for digits read after '.'. */
		if (flag & NEGEXP)
			exp -= eexp;
		else
			exp += eexp;
	}

	/* Reconcile the significand with the exponent and sign. */
	if (exp != 0)
		d *= _pow10(exp);
	return ((flag & NEG) ? -d : d);
}

/* end of libc/gen/atof.c */
