/*
 * Nroff/Troff.
 * Escape character definitions.
 */
#define EEOF	-1			/* End of file */
#define ENUL	0			/* Character maps onto itself */
#define EIGN	0200			/* Character is ignored */
#define EESC	0201			/* Printable version of escape char */
#define EACA	0202			/* Acute accent */
#define EGRA	0203			/* Grave accent */
#define EMIN	0204			/* Minus sign */
#define EUNP	0206			/* Unpadded space */
#define EDWS	0207			/* Digit width space */
#define EM06	0210			/* 1/6 em narrow space */
#define EM12	0211			/* 1/12 em half narrow space */
#define ENOP	0212			/* Zero width character */
#define ETLI	0213			/* Transparent line indicator */
#define ECOM	0214			/* Start of comment */
#define EARG	0215			/* Parameter character */
#define EHYP	0216			/* Optional hyphenation character */
#define ECHR	0217			/* Special character indicator */
#define ESTR	0220			/* String macro */
#define EBRA	0221			/* Bracket building function */
#define EINT	0222			/* Interrupt text processing */
#define EVNF	0223			/* 1/2 em vertical motion */
#define EFON	0224			/* Change font */
#define EHMT	0225			/* Local horizontal motion */
#define EMAR	0226			/* Mark horizontal input place */
#define EHLF	0227			/* Horizontal line drawing function */
#define EVLF	0230			/* Vertical line drawing function */
#define ENUM	0231			/* Expand number register */
#define EOVS	0232			/* Overstrike */
#define ESPR	0233			/* Break and spread output line */
#define EVRM	0234			/* Reverse 1 em vertically */
#define EPSZ	0235			/* Change pointsize */
#define EVRN	0236			/* Reverse 1 en vertically */
#define EVMT	0237			/* Local vertical motion */
#define EWID	0240			/* Width function */
#define EXLS	0241			/* Extra line spacing */
#define EZWD	0242			/* Print character with zero width */
#define EBEG	0243			/* Begin conditional input */
#define EEND	0244			/* End conditional input */
#define ECOD	0245			/* Processed text is being returned */
