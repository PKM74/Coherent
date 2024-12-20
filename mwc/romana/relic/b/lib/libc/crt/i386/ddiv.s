//////////
/ libc/crt/i386/ddiv.s
/ i386 C runtime library.
/ IEEE software floating point support.
//////////

//////////
/ double _ddiv(double d)
/ Return d / %edx:eax in %edx:%eax.
/
/ double _drdiv(double d)
/ Return %edx:eax / d in %edx:%eax.
/
/ The hard part of floating point division is computing the result mantissa,
/ i.e. the quotient of the numerator mantissa by the denominator mantissa;
/ all are 53-bit quantities.
/ We shift the denominator mantissa to make it as large as possible,
/ then use i386 "divl" (64-bit by 32-bit unsigned integer divide)
/ twice to find the two quotient dwords efficiently.
/
/ This does not handle denormals, though it could without much trouble.
//////////

d	.equ	8
BIAS	.equ	1023
EXPMASK	.equ	0x7FF00000
MANMASK	.equ	0x000FFFFF
SGNMASK	.equ	0x80000000
HIDDEN	.equ	0x00100000

	.globl	_ddiv
	.globl	_drdiv

_ddiv:
	xchgl	%edx, 8(%esp)
	xchgl	%eax, 4(%esp)		/ exchange arg order
/	jmp	_drdiv			/ and fall through to divide

/ Numerator is in EDX:EAX, call it A = hiA:loA.
/ Denominator is on stack, call it B = hiB:loB.
/ Compute the quotient A/B, call it Q = hiQ:loQ.
_drdiv:
	push	%ebp
	movl	%ebp, %esp
	push	%esi
	push	%edi
	push	%ebx
	push	%ecx

	movl	%esi, d+4(%ebp)
	movl	%edi, d(%ebp)		/ B to ESI:EDI
					/ now done with EBP as index register

	/ Compute result sign.
	movl	%ebx, %edx
	xorl	%ebx, %esi
	andl	%ebx, $SGNMASK
	push	%ebx			/ save result sign bit

	/ Check for special cases +-0.0, +-infinity, NaN on each side.
	movl	%ecx, %esi
	andl	%ecx, $EXPMASK
	movl	%ebx, %edx
	andl	%ebx, $EXPMASK
	jz	?lhszero		/ A is 0.0; ignore denormal
	cmpl	%ebx, $EXPMASK
	jz	?lhsmax			/ A is +-infinity or NaN
	orl	%ecx, %ecx
	jz	?inf			/ normal/0.0, return +-infinity; ignore denormal
	cmpl	%ecx, $EXPMASK
	jz	?rhsmax			/ normal/+-infinity or normal/NaN

	/ Compute probable result exponent in EBX.
	/ It might get incremented below (so 0 might become 1).
	shrl	%ebx, $20		/ A biased exp in EBX
	shrl	%ecx, $20		/ B biased exp in ECX
	subl	%ecx, $BIAS-1		/ unbiased, fudged to get right result
	subl	%ebx, %ecx		/ form biased result exponent
	jl	?zero			/ underflow, return 0.0
	cmpl	%ebx, $EXPMASK>>20
	jge	?inf			/ overflow, return +-infinity

	/ Extract the mantissas.
	/ Shift the denominator mantissa to range 2^63 <= B < 2^64.
	andl	%edx, $MANMASK
	orl	%edx, $HIDDEN		/ extract A mantissa, restore hidden bit
	andl	%esi, $MANMASK
	orl	%esi, $HIDDEN		/ extract B mantissa, restore hidden bit
	shld	%esi, %edi, $11
	shll	%edi, $11		/ shift left, B bit 31 is now 1
	jnz	?hard			/ loB is nonzero

	/ Division is easy when the lo divisor dword is zero:
	/ perform a 96-bit by 32-bit divide of hiA:loA:0 by hiB to get
	/ a 64-bit quotient and a 32-bit remainder,
	/ then zero-extend the remainder to 64 bits.
	/ hiQ = q1 = A/hiB and r1 = A%hiB, then loQ = q2 = r1:0/hiB.
	/ This is a special case for efficiency; division by any double with
	/ up to 32 mantissa bits, notably any int or unsigned, is quite fast.
	divl	%esi			/ q1 = A/hiB to EAX, r1 = A%hiB to EDX
	movl	%ebp, %eax		/ save q1
	subl	%eax, %eax		/ r1:0 in EDX:EAX
	divl	%esi			/ q2 = r1:0/hiB to EAX, r to EDX
	xchgl	%ebp, %edx		/ result quotient in EDX:EAX
	subl	%ecx, %ecx		/ r:0 in EBP:ECX

	/ The quotient is in EDX:EAX, the remainder is in EBP:ECX.
	/ Round up the quotient when remainder >= B / 2, i.e. 2*r >= B.
?remtest:
	shld	%ebp, %ecx, $1		/ 2*hiR
	jc	?roundup		/ too big
	cmpl	%ebp, %esi
	jb	?position		/ 2*r < B
	ja	?roundup		/ 2*r > B, round up
	shll	%ecx, $1		/ 2*loR
	cmpl	%ecx, %edi		/ hi(2*R) == hiB, compare lo dword
	jb	?position		/ 2*r < B

	/ Round up the quotient in EDX:EAX.
?roundup:
	addl	%eax, $1
	adcl	%edx, $0

	/ The quotient in EDX:EAX might be correctly positioned as it stands,
	/ or it might require a 1-bit right shift.
?position:
	testl	%edx, $HIDDEN<<1	/ check if shift required
	jz	?pack			/ no shift required
?rshift:
	incl	%ebx			/ bump exponent
	shrd	%eax, %edx, $1
	pushfl				/ save CF for rounding
	shrl	%edx, $1		/ shift EDX:EAX right by 1 bit
	popfl				/ restore CF
	adcl	%eax, $0
	adcl	%edx, $0		/ round if appropriate
	testl	%edx, $HIDDEN<<1	/ watch out for carry past hidden bit
	jnz	?rshift

	/ Pack result mantissa in EDX:EAX with exponent from EBX and sign from stack.
?pack:
	orl	%ebx, %ebx
	jle	?zero			/ exponent underflow, return 0.0
	cmp	%ebx, $EXPMASK>>20
	jge	?inf			/ exponent overflow, return infinity
	shll	%ebx, $20		/ position exponent
	andl	%edx, $MANMASK		/ mask off hidden bit
	orl	%edx, %ebx		/ pack mantissa and exponent
	pop	%ecx
	orl	%edx, %ecx		/ pack with sign

?done:
	pop	%ecx
	pop	%ebx
	pop	%edi
	pop	%esi
	pop	%ebp
	ret

/ Numerator is 0.0 (or denormal, ignored here).
?lhszero:
	jecxz	?NaN			/ 0/0, return NaN; ignore denormal
	cmpl	%ecx, $EXPMASK
	jnz	?zero			/ 0/normal, return 0.0
?rhsmax:
	/ Numerator is normal or 0, denominator is +-infinity or NaN.
	andl	%esi, $MANMASK
	jnz	?NaN			/ A/NaN, return NaN
	orl	%edi, %edi
	jnz	?NaN			/ A/NaN, return NaN
/	jmp	?zero			/ 0/+-infinity or normal/+-infinity, return 0.0	
					/ fall through...
/ Return +0.0.
?zero:
	pop	%edx			/ pop sign bit and ignore
	subl	%edx, %edx		/ return 0.0
?zeroeax:
	subl	%eax, %eax
	jmp	?done

/ Numerator is +-infinity or NaN.
?lhsmax:
	andl	%edx, $MANMASK
	jnz	?NaN			/ NaN/B, return NaN
	orl	%eax, %eax
	jnz	?NaN			/ NaN/B, return NaN
	cmpl	%ecx, $EXPMASK
	jz	?NaN			/ +-infinity/NaN or +-infinity/+-infinity
/	jmp	?inf			/ +-infinity/normal or +-infinity/0, return infinity
					/ fall through...
		
/ Return +-infinity.
?inf:
	pop	%edx			/ pop result sign bit
	orl	%edx, $EXPMASK		/ max exp, zero mantissa for infinity
	jmp	?zeroeax

/ Return NaN.
?NaN:
	pop	%edx			/ pop sign bit and ignore
	movl	%edx, $EXPMASK|MANMASK	/ max exp, nonzero mantissa for NaN
	jmp	?zeroeax

/ The hard case performs a 128-bit by 64-bit division of hiA:loA:0:0
/ by hiB:loB to get a 64-bit quotient hiQ:loQ and a 64-bit remainder hiR:loR.
/ Execute the following code twice to compute the two quotient dwords,
/ saving hiQ in EBP and using it as a termination flag.
/ Each iteration does a 96-bit by 64-bit divide to get a 32-bit quotient
/ and a 64-bit remainder.
/ Use q = A / hiB as a guess at the quotient,
/ then decrement the remainder r = A % hiB by loB*q.
/ We may need to decrement q once or twice to get the right quotient.

?hard:
	push	%ebx			/ save result exponent
	subl	%ebp, %ebp		/ clear flag for first pass
?divide:
	divl	%esi			/ q = A/hiB to EAX, r = A%hiB to EDX
	movl	%ecx, %eax		/ save q in ECX
	movl	%ebx, %edx		/ and save r in EBX
	mull	%edi			/ loB*q to EDX:EAX
	xchgl	%edx, %ebx		/ r to EDX, hi(loB*q) to EBX
	negl	%eax			/ 0-lo(loB*q) to EAX
	sbbl	%edx, %ebx		/ EDX:EAX gets r-loB*q
	jnc	?gotcha			/ q is the right quotient

	/ q is too big a guess, adjust to q' = q-1 and adjust the remainder.
	/ This gets executed at most twice (because the high bit of B is 1).
?adjust:
	decl	%ecx			/ decrement the quotient
	addl	%eax, %edi
	adcl	%edx, %esi		/ add B back to remainder
	jnc	?adjust			/ repeat if still negative

	/ The correct dword q is in ECX, the remainder r is in EDX:EAX.
?gotcha:
	orl	%ebp, %ebp
	jz	?loQ			/ repeat to find loQ
	xchgl	%ebp, %edx
	xchgl	%ecx, %eax		/ q to EDX:EAX, r to EBP:ECX
	pop	%ebx			/ restore result exponent
	jmp	?remtest

	/ Compute the lo dword of the quotient.
	/ r is in EDX:EAX, hiQ is in ECX.
?loQ:
	movl	%ebp, %ecx		/ save hiQ (nonzero) in EBP
	cmpl	%edx, %esi		/ be wary of overflow on divide
	jb	?divide			/ no overflow, proceed as above

	/ Since r is the remainder of the first division by B,
	/ it must be strictly less than B.  But it is possible that hiR = hiB,
	/ in which case the divide using "divl" would overflow.
	/ For this case, use q2 = 0xFFFFFFFF = 2^32 - 1 as the quotient guess.
	/ Compute the adjusted remainder r2 = r - q2*B as r - 2^32 * B + B.
	movl	%ecx, $0xFFFFFFFF	/ q2
	subl	%eax, %edi		/ loR - loB, must be negative
/	sbbl	%edx, %esi		/ hiR = hiB, so hi dword EDX becomes 0
	movl	%edx, %eax		/ EDX:EAX *= 2^32; EDX is now negative
	movl	%eax, %edi		/ 0 + loB
	addl	%edx, %esi		/ add back B to EDX:EAX
	jc	?gotcha			/ remainder became positive, q2 is ok
	jmp	?adjust			/ remainder stayed negative, adjust q2

/ end of libc/crt/i386/ddiv.s
