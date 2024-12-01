/////////
/ 
/ Subtraction.
/ Similar (at least in spirit) to addition.
/ Some of the very strange cases are not required,
/ since subtraction is not as reorderable as addition.
/ Special stuff for LARGE model pointers.
/
/////////

SUB:
%	PEFFECT|PRVALUE|PSREL|P_SLT
	WORD		ANYR	ANYR	*	TEMP
		TREG		WORD
		1|MMX		*
%	PLVALUE|P_SLT
	WORD		ANYL	ANYL	*	TEMP
		TREG		WORD
		1|MMX		*
			[ZDEC]	[R]
		[IFR]	[REL0]	[LAB]

#ifndef ONLYSMALL
%	PEFFECT|PRVALUE|P_SLT
	LPTX		ANYR	ANYR	*	TEMP
		TREG		LPTX
		1|MMX		*
%	PLVALUE|P_SLT
	LPTX		ANYL	ANYL	*	TEMP
		TREG		LPTX
		1|MMX		*
			[ZDEC]	[LO R]
#endif

/////////
/
/ A strange optimization for the i8086.
/ It is just as fast as the subtract, and it is smaller.
/ Thanks to Bob McNamara at Digital.
/
/////////

%	PEFFECT|PRVALUE|PSREL|P_SLT
	WORD		ANYR	ANYR	*	TEMP
		TREG		WORD
		2|MMX		*
%	PLVALUE|P_SLT
	WORD		ANYL	ANYL	*	TEMP
		TREG		WORD
		2|MMX		*
			[ZDEC]	[R]
			[ZDEC]	[R]
		[IFR]	[REL0]	[LAB]

#ifndef ONLYSMALL
%	PEFFECT|PRVALUE|P_SLT
	LPTX		ANYR	ANYR	*	TEMP
		TREG		LPTX
		2|MMX		*
%	PLVALUE|P_SLT
	LPTX		ANYL	ANYL	*	TEMP
		TREG		LPTX
		2|MMX		*
			[ZDEC]	[LO R]
			[ZDEC]	[LO R]
#endif

/////////
/
/ Old fashoned subtraction.
/
/////////

%	PEFFECT|PRVALUE|PSREL|P_SLT
	WORD		ANYR	ANYR	*	TEMP
		TREG		WORD
		ADR|IMM		WORD
%	PLVALUE|P_SLT
	WORD		ANYL	ANYL	*	TEMP
		TREG		WORD
		ADR|IMM		WORD
			[ZSUB]	[R],[AR]
		[IFR]	[REL0]	[LAB]

#ifndef ONLYSMALL
%	PEFFECT|PRVALUE|P_SLT
	LPTX		ANYR	ANYR	*	TEMP
		TREG		LPTX
		ADR|IMM		WORD
%	PLVALUE|P_SLT
	LPTX		ANYL	ANYL	*	TEMP
		TREG		LPTX
		ADR|IMM		WORD
			[ZSUB]	[LO R],[AR]
#endif

%	PEFFECT|PRVALUE|PGE|PLT|P_SLT
	LONG		DXAX	DXAX	*	DXAX
		TREG		LONG
		ADR|IMM		LONG
			[ZSUB]	[LO R],[LO AR]
			[ZSBB]	[HI R],[HI AR]
		[IFR]	[REL1]	[LAB]

#ifndef ONLYSMALL
%	PEFFECT|PRVALUE|P_SLT
	LPTX		ANYR	ANYR	*	TEMP
		TREG		LPTX
		ADR|IMM		LONG
%	PLVALUE|P_SLT
	LPTX		ANYL	ANYL	*	TEMP
		TREG		LPTX
		ADR|IMM		LONG
			[ZSUB]	[LO R],[LO AR]
#endif

/////////
/
/ LARGE model pointer-pointer yields 1 word of result.
/ Only RVALUE contexts need to be done.
/
/////////

#ifndef ONLYSMALL
%	PRVALUE
	WORD		ANYR	*	*	TEMP
		DIR|IMM|MMX	LPTX
		DIR|IMM|MMX	LPTX
			[ZMOV]	[R],[LO AL]
			[ZSUB]	[R],[LO AR]

%	PRVALUE
	WORD		DXAX	DXAX	*	AX
		TREG		LPTX
		ADR|IMM		LPTX
			[ZSUB]	[LO R],[LO AR]
#endif

////////
/
/ Floating point, using the numeric data coprocessor (8087).
/
////////

#ifdef NDPDEF
%	PRVALUE|P_SLT
	FF32|FF64	FPAC	FPAC	*	FPAC
		TREG		FF64
		ADR		FS16
			[ZFSUBI] [AR]

%	PRVALUE|P_SLT
	FF32|FF64	FPAC	FPAC	*	FPAC
		TREG		FF64
		ADR		FS32
			[ZFSUBL] [AR]

%	PRVALUE|P_SLT
	FF32|FF64	FPAC	FPAC	*	FPAC
		TREG		FF64
		ADR		FF32
			[ZFSUBF] [AR]

%	PRVALUE|P_SLT
	FF32|FF64	FPAC	FPAC	*	FPAC
		TREG		FF64
		ADR		FF64
			[ZFSUBD] [AR]
#endif
