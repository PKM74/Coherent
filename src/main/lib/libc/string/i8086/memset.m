//////////
/ i8086 C string library.
/ memset()
/ ANSI 4.11.6.1.
//////////

//////////
/ char *
/ memset(String, Char, Count)
/ char *String;
/ int Char, Count;
/
/ Set Count bytes of String to Char.
//////////

#include <larges.h>

String	=	LEFTARG
Char	=	String+DPL
Count	=	Char+2

	Enter(memset_)
	mov	cx, Count(bp)	/ Count to CX
	movb	al, Char(bp)	/ Char to AL
	Les	di, String(bp)	/ String address to ES:DI
	cld
	rep
	stosb			/ Copy Char to String
	mov	ax, String(bp)	/ Return the destination in AX
#if	LARGEDATA
	mov	dx, es		/ or DX:AX
#endif
	Leave
