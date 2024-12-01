/ $Header: /kernel/kersrc/io.286/ipcas.s,v 1.1 92/07/17 15:24:15 bin Exp Locker: bin $
/
/	The  information  contained herein  is a trade secret  of INETCO
/	Systems, and is confidential information.   It is provided under
/	a license agreement,  and may be copied or disclosed  only under
/	the terms of that agreement.   Any reproduction or disclosure of
/	this  material  without  the express  written  authorization  of
/	INETCO Systems or persuant to the license agreement is unlawful.
/
/	Copyright (c) 1985, 1984
/	An unpublished work by INETCO Systems, Ltd.
/	All rights reserved.
/

////////
/
/ System V Compatible Inter-Process Communication - Assembler Support
/
/ ufcopy( base, off, sel, n ) -- copy n bytes from user base to sel:off.
/ fucopy( off, sel, base, n ) -- copy n bytes from sel:off to user base.
/
/ $Log:	ipcas.s,v $
/ Revision 1.1  92/07/17  15:24:15  bin
/ Initial revision
/
/ Revision 2.1	88/09/03  13:06:24	src
/ *** empty log message ***
/ 
/ Revision 1.1	88/03/24  17:05:05	src
/ Initial revision
/ 
/
/ 85/07/19	Allan Cornish
/ Inserted code to check user address for validity.
/ Functions ufcopy and fucopy now return 0 if user address is invalid.
/ Replaced 'jnc .+2;movsb' with 'rcl cx,$1;rep movsb' to improve pipelining.
/
/ 85/07/03	Allan Cornish
/ Functions renamed: uoscopy --> ufcopy, osucopy --> fucopy (f = far).
/ Module moved from msgas.s to ipcas.s, to reflect its shared use.
/
////////

	.globl	ufcopy_
	.globl	fucopy_

////////
/
/ ufcopy( base, off, sel, n )	-- copy n bytes from user base to sel:off.
/
/	Input:	base = offset in user memory to copy from
/		off  = offset in the destination segment
/		sel  = selector to access the destination segment
/		n    = number of bytes to copy
/
/	Action:	Copy 'n' bytes of data from offset 'base' in user memory
/		to offset 'off' in the segment accessed by selector 'sel'.
/
/	Return:	Number of bytes copied, or 0 if invalid user address.
/
////////

ufcopy_:			/ ufcopy( base, off, sel, n )
	push	si		/
	push	di		/ unsigned base;
	push	bp		/ unsigned off;
	mov	bp, sp		/ saddr_t sel;
	push	ds		/ unsigned n;
	push	es		/
				/ {
	mov	ax, 8(bp)	/	Validate user address.
	dec	ax		/
	add	ax, 14(bp)	/	Wrap-around error?
	jc	fuerr		/
	cmp	ax, udl_	/	Address out of bounds error?
	ja	fuerr		/
				/
	mov	bx, uds_	/	Map DS:SI into user (source) addr
	mov	ds, bx		/
	mov	si, 8(bp)	/
	les	di, 10(bp)	/	Map ES:DI into segment (dest) addr
	mov	cx, 14(bp)	/	Transfer count
				/
	cld			/	Auto Increment
	clc			/
	rcr	cx, $1		/	Change byte count into word count
	rep			/	Transfer data words
	movsw			/
	rcl	cx, $1		/	If residual byte count
	rep			/		Transfer last data byte.
	movsb			/
				/
	mov	ax, 14(bp)	/	Return transfer count.
	pop	es		/ }
	pop	ds
	pop	bp
	pop	di
	pop	si
	ret

fuerr:	sub	ax, ax
	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	ret

////////
/
/ fucopy( off, sel, base, n )	-- copy n bytes from sel:off to user base.
/
/	Input:	off  = offset is the source segment
/		sel  = selector to access the source segment
/		base = offset in user memory to copy to
/		n    = number of bytes to copy
/
/	Action:	Copy 'n' bytes of data from offset 'off' in the segment
/		accessed by selector 'sel' to offset 'base' in user memory.
/
/	Return:	Number of bytes copied, or 0 if invalid user address.
/
////////

fucopy_:			/ fucopy( off, sel, base, n )
	push	si		/
	push	di		/ unsigned off;
	push	bp		/ saddr_t  sel;
	mov	bp, sp		/ unsigned base;
	push	ds		/ unsigned n;
	push	es		/
				/ {
	mov	ax, 12(bp)	/	Validate user address.
	dec	ax		/
	add	ax, 14(bp)	/	Wrap-around error?
	jc	fuerr		/
	cmp	ax, udl_	/	Address out of bounds error?
	ja	fuerr		/
				/
	mov	es, uds_	/	Map ES:DI into user (dest) address
	mov	di, 12(bp)	/
	lds	si, 8(bp)	/	Map DS:SI into segment (source) addr
	mov	cx, 14(bp)	/	Transfer count
				/
	cld			/	Auto Increment
	clc			/
	rcr	cx, $1		/	Change byte count into word count
	rep			/
	movsw			/	Transfer data words
	rcl	cx, $1		/	If residual byte count
	rep			/		Transfer last data byte.
	movsb			/
				/
	mov	ax, 14(bp)	/	Return transfer count.
	pop	es		/ }
	pop	ds
	pop	bp
	pop	di
	pop	si
	ret
