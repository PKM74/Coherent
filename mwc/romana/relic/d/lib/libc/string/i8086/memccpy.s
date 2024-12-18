////////
/
/ #include <string.h>
/
/ char *
/ memccpy(dest, src, c, n)
/ char *dest, *src;
/
/	Action:	Copy characters from memory area src into dest, stopping
/		after  the first occurrence of  character C has  been
/		been copied, or after N characters have been copied.
/
/	Return:	A pointer to the character after the copy of C in dest,
/		or a NULL pointer  if C was not found  in the first N
/		characters of src.
////////
	.globl	memccpy_

memccpy_:				/ char *
	push	si			/ memccpy ( dest, src, c, n )
	push	di			/
	mov	bx, sp			/ char *dest, *src;
	movb	al, 10(bx)		/ register int c;		/* AX */
	mov	dx, 12(bx)		/ register unsigned n;		/* DX */
					/
	mov	di, 8(bx)		/ {	register char *cp = src; /* DI */
	mov	cx, dx			/	register cnt = n;	/* CX */
					/
	jcxz	0f			/	for (; cnt != 0; --cnt)
	cld				/ 		if (*cp++ == c)
	repne				/			break;
	scasb				/
	jne	0f			/	if (cp[-1] == c)
	sub	dx, cx			/	{	n  -= cnt;
	mov	cx, di			/
	sub	cx, 8(bx)		/		cnt = cp - src + dest;
	add	cx, 6(bx)		/	}
0:	mov	ax, cx			/	c = cnt;
	mov	si, 8(bx)		/
	mov	di, 6(bx)		/
	mov	cx, dx			/	for ( cnt = n; cnt != 0; --cnt)
	rep				/		*dest++ = *src++;
	movsb				/
	pop	di			/	return c;
	pop	si			/ }
	ret
