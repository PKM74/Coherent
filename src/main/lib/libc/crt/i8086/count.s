////////
/ Scount is called as follows:
/	mov	bx, address of m_flst
/	call	count
/ It simply increments the count field, and if it was not already
/ on the list pointed to by `_fnclst', it places it there.
/ We also compare the stack pointer to the lowest sp previously
/ encountered and save the lower.
/ Note that because of the strange calling sequence, and the need
/ for speed, the file scount.s is a hand modified version of the C
/ compiler output.  The modifications are as follows:
/	Remove the epilog and prolog.
/	Recode to use scratch registers.
/	Change routine name from scount_ to scount.
/ The C source is as follows:
/
/	#include "mon.h"
/	
/	struct m_flst	*_fnclst;
/	struct m_hdr	_mhdr;
/	
/	scount(pc, flp)
/	vaddr_t	pc;			/* actually passed in (sp) */
/	register struct m_flst	*flp;	/* actually passed in bx */
/	{
/		++flp->m_data.m_ncalls;
/		if (&pc < _mhdr.m_lowsp)
/			_mhdr.m_lowsp = &pc;
/		if (flp->m_link == NULL) {
/			flp->m_link = _fnclst;
/			_fnclst = flp;
/			flp->m_data.m_addr = pc;
/		}
/	}

	.shri

	.globl	scount

scount:
	add	0x02(bx), $0x01
	adc	0x04(bx), $0x00
	cmp	sp, _mhdr_+0x08
	jae	L2
	mov	_mhdr_+0x08, sp

L2:	cmp	0x06(bx), $0x00
	jne	L1
	mov	ax, _fnclst_
	mov	0x06(bx), ax
	mov	_fnclst_, bx
	pop	ax
	push	ax
	mov	(bx), ax

L1:	ret
