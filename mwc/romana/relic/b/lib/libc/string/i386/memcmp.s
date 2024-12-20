//////////
/ libc/string/i386/memcmp.s
/ i386 C string library.
/ ANSI 4.11.4.1.
//////////

//////////
/ int
/ memcmp(void *String1, void *String2, size_t Count)
/
/ Compare Count bytes of String2 and String1.
/ Return -1 for <, 0 for ==, 1 for >.
//////////

String1	.equ	12
String2	.equ	String1+4
Count	.equ	String2+4

	.globl	memcmp

memcmp:
	push	%esi
	push	%edi

	subl	%eax, %eax		/ Result 0 to EAX, set ZF in case ECX==0
	movl	%ecx, Count(%esp)	/ Count to ECX
	movl	%esi, String2(%esp)	/ String2 address to ESI
	movl	%edi, String1(%esp)	/ String1 address to EDI
	cld
	repe
	cmpsb
	jz	?done			/ String1 == String2, return 0
	ja	?less
	incl	%eax			/ String1 > String2, return 1
	jmp	?done

?less:
	decl	%eax			/ String1 < String2, return -1

?done:
	pop	%edi
	pop	%esi
	ret

/ end of libc/string/i386/memcmp.s
