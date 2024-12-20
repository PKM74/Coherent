//////////
/ libc/string/i386/strlen.s
/ i386 C string library.
/ ANSI 4.11.6.3.
//////////

//////////
/ size_t
/ strlen(char *String)
/
/ Find length of String.
//////////

String	.equ	8

	.globl	strlen

strlen:
	push	%edi

	movl	%edi, String(%esp)	/ String address to EDI
	movl	%ecx, $-1		/ Max count to ECX
	subb	%al, %al		/ NUL to AL
	cld
	repne
	scasb				/ Scan to NUL
	movl	%eax, $-2
	subl	%eax, %ecx		/ Return length in EAX

	pop	%edi
	ret

/ end of libc/string/i386/strlen.s
