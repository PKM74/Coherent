/* (-lgl
 * 	COHERENT Version 4.0
 * 	Copyright (c) 1982, 1992 by Mark Williams Company.
 * 	All rights reserved. May not be copied without permission.
 -lgl) */
/*
 * Swedish/Finnish virtual console keyboard code table.
 *
 *	See header files for definitions and constants.
 *
 *	Version: 1.1, 07/08/91
 *	Version: 1.2, 06/25/92
 *	Version: 1.3, 07/09/92
 *	Version: 1.5, 06/14/93
 *	Version: 1.6, 09/01/93
 */
#include <sys/kbscan.h>
#include <sys/kb.h>

char	tbl_name[] = "Swedish/Finnish virtual console keyboard table - V1.6";
#define A	0x80 |
#define CSI	"\x1B["
#define END	"\xFF"

KBTBL	kbtbl[] = {
/* AT                                                    Alt
 *Phys                       Ctrl           Alt   Alt    Ctrl     Alt
 *Key#  Base   Shift  Ctrl   Shift  Alt    Shift  Ctrl   Shift  Graphic Flags
 *----  ----   -----  ----   -----  ---    -----  ----   -----  ------- -----*/
{K_1,	nak,   0xAB,  none,  none,  nak,   0xAB,  none,  none,  none,	O|T  },
{K_2,	'1',   '!',   none,  none,  A'1',  A'!',  none,  none,  none,	O|T  },
{K_3,	'2',   '"',   none,  none,  A'2',  A'"',  A'@',  A nul, '@',	O|T  },
{K_4,	'3',   '#',   none,  none,  A'3',  A'#',  0x9C,  none,  0x9C,	O|T  },
{K_5,	'4',   0x0F,  none,  none,  A'4',  0x0F,  A'$',  none,  '$',	O|T  },
{K_6,	'5',   '%',   none,  none,  A'5',  A'%',  none,  none,  none,	O|T  },
{K_7,	'6',   '&',   none,  none,  A'6',  A'&',  none,  none,  none,	O|T  },
{K_8,	'7',   '/',   none,  none,  A'7',  A'/',  A'{',  none,  '{',	O|T  },
{K_9,	'8',   '(',   none,  none,  A'8',  A'(',  A'[',  none,  '[',	O|T  },
{K_10,	'9',   ')',   none,  none,  A'9',  A')',  A']',  none,  ']',	O|T  },
{K_11,	'0',   '=',   none,  none,  A'0',  A'=',  A'}',  none,  '}',	O|T  },
{K_12,	'+',   '?',   none,  none,  A'+',  A'?',  A'\\', none,  '\\',	O|T  },
{K_13,	'\'',  '`',   none,  none,  A'\'', A'`',  none,  none,  none,	O|T  },
/* key 14 undefined */
{K_15,	bs,    bs,    del,   del,   A bs,  A bs,  none,  A del, none,	O|T  },
{K_16,	f42,   f43,   none,  none,  f42,   f43,   none,  none,  none,   F|T  },
{K_17,	'q',   'Q',   dc1,   dc1,   A'q',  A'Q',  A'@',  A nul, '@',    C|T  },
{K_18,	'w',   'W',   etb,   etb,   A'w',  A'W',  none,  A etb, none,   C|T  },
{K_19,	'e',   'E',   enq,   enq,   A'e',  A'E',  none,  A enq, none,   C|T  },
{K_20,	'r',   'R',   dc2,   dc2,   A'r',  A'R',  none,  A dc2, none,   C|T  },
{K_21,	't',   'T',   dc4,   dc4,   A't',  A'T',  none,  A dc4, none,   C|T  },
{K_22,	'y',   'Y',   em,    em,    A'y',  A'Y',  none,  A em,  none,   C|T  },
{K_23,	'u',   'U',   nak,   nak,   A'u',  A'U',  none,  A nak, none,   C|T  },
{K_24,	'i',   'I',   ht,    ht,    A'i',  A'I',  none,  A ht,  none,   C|T  },
{K_25,	'o',   'O',   si,    si,    A'o',  A'O',  none,  A si,  none,   C|T  },
{K_26,	'p',   'P',   dle,   dle,   A'p',  A'P',  none,  A dle, none,   C|T  },
{K_27,	0x86,  0x8F,  none,  none,  0x86,  0x8F,  none,  none,  none,   C|T  },
{K_28,	0xB1,  '^',   none,  none,  0xB1,  A'^',  A'~',  none,  '~',	O|T  },
{K_29,	none,  none,  none,  none,  none,  none,  none,  none,  none,   O|T  },
{K_30,	caps,  caps,  caps,  caps,  caps,  caps,  caps,  caps,  caps,   S|M  },
{K_31,	'a',   'A',   soh,   soh,   A'a',  A'A',  none,  A soh, none,   C|T  },
{K_32,	's',   'S',   dc3,   dc3,   A's',  A'S',  none,  A dc3, none,   C|T  },
{K_33,	'd',   'D',   eot,   eot,   A'd',  A'D',  none,  A eot, none,   C|T  },
{K_34,	'f',   'F',   ack,   ack,   A'f',  A'F',  none,  A ack, none,   C|T  },
{K_35,	'g',   'G',   bel,   bel,   A'g',  A'G',  none,  A bel, none,   C|T  },
{K_36,	'h',   'H',   bs,    bs,    A'h',  A'H',  none,  A bs,  none,   C|T  },
{K_37,	'j',   'J',   nl,    nl,    A'j',  A'J',  none,  A nl,  none,   C|T  },
{K_38,	'k',   'K',   vt,    vt,    A'k',  A'K',  none,  A vt,  none,   C|T  },
{K_39,	'l',   'L',   ff,    ff,    A'l',  A'L',  none,  A ff,  none,   C|T  },
{K_40,	0x94,  0x99,  none,  none,  0x94,  0x99,  none,  none,  none,   C|T  },
{K_41,	0x84,  0x8E,  none,  none,  0x84,  0x8E,  none,  none,  none,   C|T  },
{K_42,	'\'',  '*',   none,  none,  A'\'', A'*',  none,  none,  none,   O|T  },
{K_43,	cr,    cr,    nl,    nl,    A cr,  A cr,  A nl,  A nl,  none,   O|T  },
{K_44,	lshift,lshift,lshift,lshift,lshift,lshift,lshift,lshift,lshift, S|MB },
{K_45,	'<',   '>',   none,  none,  A'<',  A'>',  A'|',  none,  '|',	O|T  },
{K_46,	'z',   'Z',   sub,   sub,   A'z',  A'Z',  none,  A sub, none,   C|T  },
{K_47,	'x',   'X',   can,   can,   A'x',  A'X',  none,  A can, none,   C|T  },
{K_48,	'c',   'C',   etx,   etx,   A'c',  A'C',  none,  A etx, none,   C|T  },
{K_49,	'v',   'V',   syn,   syn,   A'v',  A'V',  none,  A syn, none,   C|T  },
{K_50,	'b',   'B',   stx,   stx,   A'b',  A'B',  none,  A stx, none,   C|T  },
{K_51,	'n',   'N',   so,    so,    A'n',  A'N',  none,  A so,  none,   C|T  },
{K_52,	'm',   'M',   cr,    cr,    A'm',  A'M',  none,  A cr,  none,   C|T  },
{K_53,	',',   ';',   none,  none,  A',',  A';',  none,  none,  none,   O|T  },
{K_54,	'.',   ':',   none,  none,  A'.',  A':',  none,  none,  none,   O|T  },
{K_55,	'-',   '_',   us,    us,    A'-',  A'_',  none,  A us,  none,   O|T  },
/* key 56 undefined */
{K_57,	rshift,rshift,rshift,rshift,rshift,rshift,rshift,rshift,rshift, S|MB },
{K_58,	lctrl, lctrl, lctrl, lctrl, lctrl, lctrl, lctrl, lctrl, lctrl,  S|MB },
/* key 59 undefined */
{K_60,	lalt,  lalt,  lalt,  lalt,  lalt,  lalt,  lalt,  lalt,  lalt,   S|MB },
{K_61,	' ',   ' ',   ' ',   ' ',   A' ',  A' ',  A' ',  A' ',  none,   O|T  },
{K_62,	altgr, altgr, altgr, altgr, altgr, altgr, altgr, altgr, altgr,  S|MB },
/* key 63 undefined */
{K_64,	rctrl, rctrl, rctrl, rctrl, rctrl, rctrl, rctrl, rctrl, rctrl,  S|MB },
/* keys 65 through 74 could be functional keys on XT type keyboard */
{K_65,	f2,    f14,   f26,   f71,   vt1,   none,  none,  none,  none,   F|M  },
{K_66,	f4,    f16,   f28,   f73,   vt3,   none,  none,  none,  none,   F|M  },
{K_67,	f6,    f18,   f30,   f75,   vt5,   none,  none,  none,  none,   F|M  },
{K_68,	f8,    f20,   f65,   f77,   vt7,   none,  none,  none,  none,   F|M  },
{K_69,	f10,   f22,   f67,   f79,   vtn,   none,  none,  none,  none,   F|M  },
{K_70,	f1,    f13,   f25,   f70,   vt0,   none,  none,  none,  none,   F|M  },
{K_71,	f3,    f15,   f27,   f72,   vt2,   none,  none,  none,  none,   F|M  },
{K_72,	f5,    f17,   f29,   f74,   vt4,   none,  none,  none,  none,   F|M  },
{K_73,	f7,    f19,   f64,   f76,   vt6,   none,  none,  none,  none,   F|M  },
{K_74,	f9,    f21,   f66,   f78,   none,  none,  none,  none,  none,   F|M  },
{K_75,	f40,   f40,   f40,   f40,   f40,   f40,   f40,   f40,   f40,    F|M  },
{K_76,	f41,   f41,   f41,   f41,   f41,   f41,   reboot,f41,   f41,    F|M  },
/* keys 77 and 78 undefined */
{K_79,	f34,   f34,   f34,   f34,   f34,   f34,   f34,   f34,   f34,    F|T  },
{K_80,	f37,   f37,   f37,   f37,   f37,   f37,   f37,   f37,   f37,    F|M  },
{K_81,	f31,   f31,   f31,   f31,   f31,   f31,   f31,   f31,   f31,    F|M  },
/* key 82 undefined */
{K_83,	f38,   f38,   f38,   f38,   f38,   f38,   f38,   f38,   f38,    F|T  },
{K_84,	f32,   f32,   f32,   f32,   f32,   f32,   f32,   f32,   f32,    F|T  },
{K_85,	f39,   f39,   f39,   f39,   f39,   f39,   f39,   f39,   f39,    F|M  },
{K_86,	f33,   f33,   f33,   f33,   f33,   f33,   f33,   f33,   f33,    F|M  },
/* keys 87 and 88 undefined */
{K_89,	f36,   f36,   f36,   f36,   f36,   f36,   f36,   f36,   f36,    F|T  },
{K_90,	num,   num,   num,   num,   num,   num,   num,   num,   num,    S|M  },
{K_91,	f37,   f57,   vt7,   vt7,   f57,   f57,   f57,   f57,   f57,    F|N|M},
{K_92,	f34,   f54,   vt4,   vt4,   f54,   f54,   f54,   f54,   f54,    F|N|M},
{K_93,	f31,   f51,   vt1,   vt1,   f51,   f51,   f51,   f51,   f51,    F|N|M},
/* key 94 undefined */
{K_95,	'/',   '/',   none,  none,  none,  none,  none,  none,  none,   O|M  },
{K_96,	f38,   f58,   vt8,   vt8,   f58,   f58,   f58,   f58,   f58,    F|N|M},
{K_97,	f35,   f55,   vt5,   vt5,   f55,   f55,   f55,   f55,   f55,    F|N|M},
{K_98,	f32,   f52,   vt2,   vt2,   f52,   f52,   f52,   f52,   f52,    F|N|M},
{K_99,	f40,   f60,   vt0,   vt0,   f60,   f60,   f60,   f60,   f60,    F|N|M},
{K_100,	'*',   '*',   none,  none,  none,  none,  none,  none,  none,   O|M  },
{K_101,	f39,   f59,   vt9,   vt9,   f59,   f59,   f59,   f59,   f59,    F|N|M},
{K_102,	f36,   f56,   vt6,   vt6,   f56,   f56,   f56,   f56,   f56,    F|N|M},
{K_103,	f33,   f53,   vt3,   vt3,   f53,   f53,   f53,   f53,   f53,    F|N|M},
{K_104,	f41,   f61,   vtt,   vtt,   f61,   f61,   reboot,reboot,f61,    F|N|M},
{K_105,	f63,   f63,   vtp,   vtp,   none,  none,  none,  none,  none,   F|M  },
{K_106,	f62,   f62,   vtn,   vtn,   none,  none,  none,  none,  none,   F|M  },
/* key 107 undefined */
{K_108,	cr,    cr,    nl,    nl,    A cr,  A cr,  A nl,  A nl,  none,   O|M  },
/* key 109 undefined */
{K_110,	esc,   esc,   none,  none,  A esc, A esc, none,  none,  none,   O|M  },
/* key 111 undefined */
{K_112,	f1,    f13,   f25,   f70,   vt0,   none,  none,  none,  none,   F|M  },
{K_113,	f2,    f14,   f26,   f71,   vt1,   none,  none,  none,  none,   F|M  },
{K_114,	f3,    f15,   f27,   f72,   vt2,   none,  none,  none,  none,   F|M  },
{K_115,	f4,    f16,   f28,   f73,   vt3,   none,  none,  none,  none,   F|M  },
{K_116,	f5,    f17,   f29,   f74,   vt4,   none,  none,  none,  none,   F|M  },
{K_117,	f6,    f18,   f30,   f75,   vt5,   none,  none,  none,  none,   F|M  },
{K_118,	f7,    f19,   f64,   f76,   vt6,   none,  none,  none,  none,   F|M  },
{K_119,	f8,    f20,   f65,   f77,   vt7,   none,  none,  none,  none,   F|M  },
{K_120,	f9,    f21,   f66,   f78,   none,  none,  none,  none,  none,   F|M  },
{K_121,	f10,   f22,   f67,   f79,   vtn,   none,  none,  none,  none,   F|M  },
{K_122,	f11,   f23,   f68,   f44,   vtp,   none,  none,  none,  none,   F|M  },
{K_123,	f12,   f24,   f69,   f45,   vtt,   none,  none,  none,  none,   F|M  },
{K_124,	none,  none,  none,  none,  none,  none,  none,  none,  none,   O|M  },
{K_125,	scroll,scroll,scroll,scroll,scroll,scroll,scroll,scroll,scroll, S|M  },
{K_126,	none,  none,  none,  none,  none,  none,  none,  none,  none,   O|M  }
};

/*
 * Special and programmable function key definitions:
 *
 * Notes:
 *   1) If a key outputs a multi-byte sequence in any mode, the key
 *	must be defined as a function key (flags field == F) and all entries
 *	for the key must be function keys (i.e., f1 through f50).
 *
 *   2)	All key definition strings must be terminated by a \377 sequence.
 *	This allows the NUL ('\0') character to be embedded in function strings.
 */

unsigned char	*funkey[] = {
/* 0/reboot */	"reboot" END,		/* jump to reboot code */
/* 1 */		CSI"M" END,		/* F1 */
/* 2 */		CSI"N" END,		/* F2 */
/* 3 */		CSI"O" END,		/* F3 */
/* 4 */		CSI"P" END, 		/* F4 */
/* 5 */		CSI"Q" END,		/* F5 */
/* 6 */		CSI"R" END,		/* F6 */
/* 7 */		CSI"S" END,		/* F7 */
/* 8 */		CSI"T" END,		/* F8 */
/* 9 */		CSI"U" END,		/* F9 */
/* 10 */	CSI"V" END,		/* F10 */
/* 11 */	CSI"W" END,		/* F11 */
/* 12 */	CSI"X" END,		/* F12 */
/* 13 */	CSI"Y" END,		/* sF1 */
/* 14 */	CSI"Z" END, 		/* sF2 */
/* 15 */	CSI"a" END,		/* sF3 */
/* 16 */	CSI"b" END,		/* sF4 */
/* 17 */	CSI"c" END,		/* sF5 */
/* 18 */	CSI"d" END,		/* sF6 */
/* 19 */	CSI"e" END,		/* sF7 */
/* 20 */	CSI"f" END,		/* sF8 */
/* 21 */	CSI"g" END,		/* sF9 */
/* 22 */	CSI"h" END,		/* sF10 */
/* 23 */	CSI"i" END,		/* sF11 */
/* 24 */	CSI"j" END, 		/* sF12 */
/* 25 */	CSI"k" END,		/* cF1 */
/* 26 */	CSI"l" END,		/* cF2 */
/* 27 */	CSI"m" END,		/* cF3 */
/* 28 */	CSI"n" END,		/* cF4 */
/* 29 */	CSI"o" END,		/* cF5 */
/* 30 */	CSI"p" END,		/* cF6 */
/* 31 */	CSI"F" END,	 	/* End */
/* 32 */	CSI"B" END,		/* Down Arrow */
/* 33 */	CSI"G" END, 		/* Page Down */
/* 34 */	CSI"D" END,	 	/* Left Arrow */
/* 35 */	"" END,			/* Unshifted keypad 5 */
/* 36 */	CSI"C" END,	 	/* Right Arrow */
/* 37 */	CSI"H" END,	 	/* Home */
/* 38 */	CSI"A" END,	 	/* Up Arrow */
/* 39 */	CSI"I" END,	 	/* Page Up */
/* 40 */	CSI"L" END,	 	/* Insert */
/* 41 */        "\177" END,	 	/* Delete */
/* 42 */	"\t" END,		/* Tab */
/* 43 */	CSI"Z" END,	 	/* Back Tab */
/* 44 */	CSI"`" END,		/* c-s-F11 */
/* 45 */	CSI"{" END,		/* c-s-F12 */
/* 46 */	"F44" END,		/* unused */
/* 47 */	"F47" END,		/* unused */
/* 48 */	"F48" END,		/* unused */
/* 49 */	"F49" END,		/* unused */
/* 50 */	"F50" END,		/* unused */
/* 51 */	"1" END,	 	/* Keypad 1 */
/* 52 */	"2" END,	 	/* Keypad 2 */
/* 53 */	"3" END,	 	/* Keypad 3 */
/* 54 */	"4" END,	 	/* Keypad 4 */
/* 55 */	"5" END,	 	/* Keypad 5 */
/* 56 */	"6" END,	 	/* Keypad 6 */
/* 57 */	"7" END,	 	/* Keypad 7 */
/* 58 */	"8" END,	 	/* Keypad 8 */
/* 59 */	"9" END,	 	/* Keypad 9 */
/* 60 */	"0" END,	 	/* Keypad 0 */
/* 61 */	"." END,	 	/* Keypad . */
/* 62 */	"+" END,	 	/* Keypad + */
/* 63 */	"-" END,	 	/* Keypad - */
/* 64 */	CSI"q" END, 		/* cF7 */
/* 65 */	CSI"r" END,		/* cF8 */
/* 66 */	CSI"s" END,		/* cF9 */
/* 67 */	CSI"t" END,		/* cF10 */
/* 68 */	CSI"u" END,		/* cF11 */
/* 69 */	CSI"v" END,		/* cF12 */
/* 70 */	CSI"w" END,		/* c-s-F1 */
/* 71 */	CSI"x" END,		/* c-s-F2 */
/* 72 */	CSI"y" END,		/* c-s-F3 */
/* 73 */	CSI"z" END,		/* c-s-F4 */
/* 74 */	CSI"@" END, 		/* c-s-F5 */
/* 75 */	CSI"[" END,		/* c-s-F6 */
/* 76 */	CSI"\\" END,		/* c-s-F7 */
/* 77 */	CSI"]" END,		/* c-s-F8 */
/* 78 */	CSI"^" END,		/* c-s-F9 */
/* 79 */	CSI"_" END		/* c-s-F10 */

};

int	numfun	= sizeof(funkey) / sizeof(funkey[0]);	/* # of Fn keys */
int	numkey	= sizeof(kbtbl) / sizeof(kbtbl[0]);	/* # of actual keys */
/* end of swedish.c */
