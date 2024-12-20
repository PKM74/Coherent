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
 *  $Header: /src386/usr/bin/infocmp/RCS/terminfo.h,v 1.1 92/03/13 10:21:20 bin Exp $
 *
 *	terminfo.h - those things needed for programs runnning at the
 *			terminfo level.
 *
 *  $Log:	terminfo.h,v $
 * Revision 1.1  92/03/13  10:21:20  bin
 * Initial revision
 * 
Revision 2.2  91/02/10  12:27:05  munk
Added conditional 8-bit characters for UNIX on PC's

Revision 2.1  82/10/25  14:49:59  pavel
Added Copyright Notice

Revision 2.0  82/10/24  15:18:26  pavel
Beta-one Test Release

Revision 1.4  82/08/23  22:31:21  pavel
The REAL Alpha-one Release Version

Revision 1.3  82/08/19  19:24:11  pavel
Alpha Test Release One

Revision 1.2  82/08/19  19:10:56  pavel
Alpha Test Release One

Revision 1.1  82/08/15  16:42:20  pavel
Initial revision

 *
 */

#ifndef A_STANDOUT

#include <stdio.h>
#include <sgtty.h>

/*
 * The following definition activates the handling of 8-bit characters.
 * If activated, chars may be 8 bits, but attribute A_DIM is set to
 * A_NORMAL, to get one more bit for the chars.
 * If you change the definition, curses should be recompiled!!!!
 */
#define CHAR8

#define SGTTY	struct sgttyb

    /* Video attributes */
#ifdef CHAR8
#define A_NORMAL	0000000
#define A_ATTRIBUTES	0177400
#define A_CHARTEXT	0000377

#define A_STANDOUT	0004000
#define A_UNDERLINE	0000400

#ifndef MINICURSES
#  define A_REVERSE	0001000
#  define A_BLINK	0002000
#  define A_DIM		A_NORMAL
#  define A_BOLD	0010000
#  define A_INVIS	0020000
#  define A_PROTECT	0040000
#  define A_ALTCHARSET	0100000
#endif MINICURSES
#else CHAR8
#define A_NORMAL	0000000
#define A_ATTRIBUTES	0177600
#define A_CHARTEXT	0000177

#define A_STANDOUT	0000200
#define A_UNDERLINE	0000400

#ifndef MINICURSES
#  define A_REVERSE	0001000
#  define A_BLINK	0002000
#  define A_DIM		0004000
#  define A_BOLD	0010000
#  define A_INVIS	0020000
#  define A_PROTECT	0040000
#  define A_ALTCHARSET	0100000
#endif MINICURSES
#endif CHAR8

extern char	ttytype[];
#define NAMESIZE	256
#endif
