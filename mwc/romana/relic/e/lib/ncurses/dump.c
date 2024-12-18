/*********************************************************************
*                         COPYRIGHT NOTICE                           *
**********************************************************************
*        This software is copyright (C) 1982 by Pavel Curtis         *
*                                                                    *
*        Permission is granted to reproduce and distribute           *
*        this file by any means so long as no fee is charged         *
*        above a nominal handling fee and so long as this            *
*        notice is always included in the copies.                    *
*                                                                    *
*        Other rights are reserved except as explicitly granted      *
*        by written permission of the author.                        *
*                Pavel Curtis                                        *
*                Computer Science Dept.                              *
*                405 Upson Hall                                      *
*                Cornell University                                  *
*                Ithaca, NY 14853                                    *
*                                                                    *
*                Ph- (607) 256-4934                                  *
*                                                                    *
*                Pavel.Cornell@Udel-Relay   (ARPAnet)                *
*                decvax!cornell!pavel       (UUCPnet)                *
*********************************************************************/

/*
 *	dump.c - dump the contents of a compiled terminfo file in a
 *		 human-readable format.
 *
 *  $Log:	dump.c,v $
 * Revision 1.10  93/04/12  14:13:14  bin
 * Udo: third color update
 * 
 * Revision 1.4  92/06/02  12:04:49  bin
 * *** empty log message ***
 * 
 * Revision 1.2  92/04/13  14:36:41  bin
 * update by vlad
 * 
 * Revision 2.2  91/07/28  14:03:30  munk
 * Made the large arrays static

 * Revision 2.1  82/10/25  14:46:20  pavel
 * Added Copyright Notice
 * 
 * Revision 2.0  82/10/24  15:17:29  pavel
 * Beta-one Test Release
 * 
 * Revision 1.3  82/08/23  22:30:18  pavel
 * The REAL Alpha-one Release Version
 * 
 * Revision 1.2  82/08/19  19:12:50  pavel
 * Alpha Test Release One
 * 
 * Revision 1.1  82/08/12  18:39:19  pavel
 * Initial revision
 * 
 *
 */

#ifdef RCSHDR
static char RCSid[] =
	"$Header: /src386/usr/lib/ncurses/RCS/dump.c,v 1.10 93/04/12 14:13:14 bin Exp Locker: bin $";
#endif

#include "compiler.h"
#include "term.h"

extern char *BoolNames[BOOLCOUNT], *NumNames[NUMCOUNT], *StrNames[STRCOUNT];
struct term _first_term;


main(argc, argv)
int	argc;
char	*argv[];
{
	int		i, j;
	int		cur_column;
	static char	buffer[1024];

	for (j=1; j < argc; j++)
	{
	    if (read_entry(argv[j], &_first_term) < 0)
	    {
		fprintf(stderr, "read_entry bombed on %s\n", argv[j]);
		abort();
	    }

	    printf("%s,\n", _first_term.term_names);
	    putchar('\t');
	    cur_column = 9;

	    for (i=0; i < BOOLCOUNT; i++)
	    {
		if (_first_term.Booleans[i] == TRUE)
		{
		    if (cur_column > 9
				&&  cur_column + strlen(BoolNames[i]) + 2 > 79)
		    {
			printf("\n\t");
			cur_column = 9;
		    }
		    printf("%s, ", BoolNames[i]);
		    cur_column += strlen(BoolNames[i]) + 2;
		}
	    }

	    for (i=0; i < NUMCOUNT; i++)
	    {
		if (_first_term.Numbers[i] != -1)
		{
		    if (cur_column > 9
				&&  cur_column + strlen(NumNames[i]) + 5 > 79)
		    {
			printf("\n\t");
			cur_column = 9;
		    }
		    printf("%s#%d, ", NumNames[i], _first_term.Numbers[i]);
		    cur_column += strlen(NumNames[i]) + 5;
		}
	    }

	    for (i=0; i < STRCOUNT; i++)
	    {
		if (_first_term.Strings[i])
		{
		    sprintf(buffer, "%s=%s, ", StrNames[i],
							_first_term.Strings[i]);
		    expand(buffer);
		    if (cur_column > 9  &&  cur_column + strlen(buffer) > 79)
		    {
			printf("\n\t");
			cur_column = 9;
		    }
		    printf("%s", buffer);
		    cur_column += strlen(buffer);
		}
	    }

	    putchar('\n');
	}
}


typedef unsigned char uchar;

expand(str)
uchar	*str;
{
	char		*strcpy();
    	static char	buffer[1024];
	int		bufp;
	uchar		*ptr;

	bufp = 0;
	ptr = str;
	while (*str)
	{
	    if (*str < ' ') {
		if (*str == 0x1B) {
			buffer[bufp++] = '\\';
			buffer[bufp++] = 'E';
		} else {
			buffer[bufp++] = '^';
			buffer[bufp++] = *str + '@';
		}
	    } else 
		if (*str < '\177')
			buffer[bufp++] = *str;
	    	else
	    	{
			sprintf(&buffer[bufp], "\\%03o", *str);
			bufp += 4;
	    	}

	    str++;
	}

	buffer[bufp] = '\0';
	strcpy((char *) ptr, buffer);
}
