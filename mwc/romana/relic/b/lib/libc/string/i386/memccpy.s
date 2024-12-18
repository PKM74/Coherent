//////////
/ libc/string/i386/memccpy.s
/ i386 C string library.
/ Not in ANSI C standard.
//////////

////////
/ char *
/ memccpy(char *dest, char *src, int c, size_t n)
/
/ Copy characters from src to dest,
/ stopping after the first occurrence of character c has been copied
/ or after n characters have been copied.
/ Return a pointer to the character after the copy of c in dest,
/ or NULL if c is not found in the first n characters of src.
/////////

dest	.equ	12
src	.equ	dest+4
c	.equ	src+4
n	.equ	c+4

	.globl	memccpy

memccpy:
	push	%esi
	push	%edi

	movb	%al, c(%esp)		/ c to AL
	movl	%edx, n(%esp)		/ n to EDX
	movl	%edi, src(%esp)		/ src to EDI
	movl	%esi, %edi		/ save src in ESI
	movl	%ecx, %edx		/ count to ECX
	jecxz	?copy			/ n is 0, skip looking for c, return NULL
	cld
	repne
	scasb				/ Find first c in src
	jne	?copy			/ Not found, just copy and return NULL
	subl	%edx, %ecx		/ Adjust count for copy
	movl	%ecx, %edi		/ pointer past c in src
	subl	%ecx, %esi		/ offset to char after c in src
	add	%ecx, dest(%esp)	/ pointer to char after c in dest

	/ The return value is in ECX, the count in EDX.
?copy:
	movl	%eax, %ecx		/ Return value to EAX
	movl	%esi, %edi
	movl	%edi, dest(%esp)
	movl	%ecx, %edx		/ Count to ECX
	rep
	movsb				/ Copy

	pop	%edi
	pop	%esi
	ret

/ end of libc/string/i386/memccpy.s
