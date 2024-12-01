/*
 * Standard stream library for the
 * C compiler and other compilers that use
 * C compiler factilities.
 * Output a byte.
 */

#include <stdio.h>
#ifdef   vax
#include "INC$LIB:mch.h"
#include "INC$LIB:stream.h"
#else
#include "mch.h"
#include "stream.h"
#endif

extern	FILE	*ofp;

bput(b)
int b;
{
#if	0
	/*
	 * Some versions VAX and i8086 stdio putc(b, ofp) return EOF (-1)
	 * for char arg 255 (because the argument gets sign-extended from
	 * char 0xFF to int 0xFFFF, and putc returns its arg on success
	 * without masking it).  The following line avoids this bug.
	 */
	b &= 0xFF;		/* avoid putc sign-extension bug */
#endif
#if	TEMPBUF
	if (ofp == NULL) {
		if (outbufp == outbufmax)
			cfatal("out of space in memory buffer");
		*outbufp++ = b;
		return;
	}
#endif
#if	MSDOS
	if (_binputc(b, ofp) == EOF)
#else
	if (putc(b, ofp) == EOF)
#endif
		cfatal("temporary file write error");
}
