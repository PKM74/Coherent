/* $Header: /newbits/286_KERNEL/USRSRC/coh/RCS/elog.c,v 1.1 92/01/09 13:26:39 bin Exp Locker: bin $ */
/* (lgl-
 *	The information contained herein is a trade secret of Mark Williams
 *	Company, and  is confidential information.  It is provided  under a
 *	license agreement,  and may be  copied or disclosed  only under the
 *	terms of  that agreement.  Any  reproduction or disclosure  of this
 *	material without the express written authorization of Mark Williams
 *	Company or persuant to the license agreement is unlawful.
 *
 *	COHERENT Version 2.3.37
 *	Copyright (c) 1982, 1983, 1984.
 *	An unpublished work by Mark Williams Company, Chicago.
 *	All rights reserved.
 -lgl) */
/*
 * Coherent - event/error logging facility.
 *
 * $Log:	elog.c,v $
 * Revision 1.1  92/01/09  13:26:39  bin
 * Initial revision
 * 
 * Revision 1.1	88/03/24  16:19:48	src
 * Initial revision
 * 
 */
#include <coherent.h>

#define NEVENT 256
typedef struct {
	char	*e_event;	/* Function pointer or whatever */
	int	e_time;		/* Timer tick of event */
} EVENT;

EVENT events[NEVENT];
unsigned curevent = 0;
unsigned totevent = 0;

elog(eventp)
char *eventp;
{
	register EVENT *ep;

	totevent += 1;
	ep = &events[curevent++];
	curevent %= NEVENT;
	ep->e_event = eventp;
	ep->e_time = timer.t_time;
}
