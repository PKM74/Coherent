/* $Header: /usr/src/sys/ldrv/RCS/ldswap.c,v 1.1 88/03/24 16:30:50 src Exp $ */
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
 * Coherent.
 * Swapper.
 *
 * $Log:	/usr/src/sys/ldrv/RCS/ldswap.c,v $
 * Revision 1.1	88/03/24  16:30:50	src
 * Initial revision
 * 
 * 87/11/05	Allan Cornish		/usr/src/sys/ldrv/ldswap.c
 * New seg struct now used to allow extended addressing.
 *
 * 87/10/26	Allan Cornish		/usr/src/sys/ldrv/ldswap.c
 * Modified to support loadable drivers - temporary modification.
 * Now requires SIGKILL signal in order to terminate.
 *
 * 87/01/05	Allan Cornish		/usr/src/sys/ker/swap.c
 * Swap() now waits for all processes to be swapped in before exit on signal.
 */
#include <sys/coherent.h>
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/seg.h>
#include <sys/uproc.h>
#include <sys/buf.h>

/*
 * Functions.
 */
SEG	*xmalloc();
SEG	*xdalloc();
void	Kwakeup();
void	Ktimeout();

main()
{
	register SEG *sp;
	register PROC *pp1;
	register PROC *pp2;
	register PROC *pp3;
	register unsigned s;
	register unsigned n;
	register unsigned t;
	register unsigned v;
	register unsigned m;
	register int i;
	static unsigned ltimer;

	if (sexflag != 0)
		uexit(1);
	sexflag++;

	while (1) {
		lock( pnxgate );
		t = (utimer - ltimer) / NSUTICK;
		v = t * SVCLOCK;
		ltimer += t * NSUTICK;

		/*
		 * Search for process to swap into memory.
		 */
		pp2 = NULL;
		m   = 0;
		s   = 0;
		for (pp1 = procq.p_nback; pp1 != &procq; pp1 = pp1->p_nback) {

			/*
			 * Process resides in memory.
			 */
			if ( (pp1->p_flags & PFCORE) != 0 ) {
				pp1->p_sval >>= t;
				pp1->p_ival  -= t;
				if (pp1->p_ival < -30000)
					pp1->p_ival = -30000;
				continue;
			}

			/*
			 * Update swap value - high values swapped in first.
			 */
			addu( pp1->p_sval, v );
			s = 1;

			/*
			 * Process is not runnable.
			 */
			if ( pp1->p_state != PSRUN )
				continue;

			/*
			 * Calculate disk usage in Kbytes.
			 */
			s = 0;
			for ( i = 0; i < NUSEG+1; i++ )
				if ( (sp = pp1->p_segp[i]) != NULL )
					if ( (sp->s_flags & SFCORE) == 0 )
						s += sp->s_size / 1024;
			if ( s == 0 )
				s = 1;

			/*
			 * Compute importance:
			 *
			 *	swap value + response value
			 *	---------------------------
			 *	  Kbytes to be swapped in
			 */
			n = (pp1->p_sval + pp1->p_rval) / s;

			/*
			 * More important.
			 */
			if ( n > m ) {
				m = n;
				pp2 = pp1;
			}
		}
		unlock( pnxgate );

		/*
		 * No runnable processes swapped out.
		 */
		if ( pp2 == NULL ) {
			/*
			 * No processes swapped out, and KILL signal received.
			 */
			if ( (s == 0) && (SELF->p_ssig & 0x0100) )
				break;
			goto con;
		}

#ifndef	NOMONITOR
		if (swmflag)
			printf("Swapin(%p, %d)\n", pp2, pp2->p_pid);
#endif
	xxx:
		/*
		 * Try to swap process into memory.
		 */
		while ( (testcore(pp2) == 0) || (proccore(pp2) != 0) ) {

			/*
			 * Swap process out.
			 */
			procdisk(pp2);
			i   = 32767;
			pp3 = NULL;

			/*
			 * Search for process to swap out.
			 */
			lock( pnxgate );
			for (pp1=procq.p_nforw; pp1!=&procq; pp1=pp1->p_nforw){

				if ( pp1->p_flags & (PFSWIO|PFLOCK|PFKERN) )
					continue;

				/*
				 * Process is not totally memory resident.
				 */
				if ( (pp1->p_flags&PFCORE) == 0 ) {
					/*
					 * Swap segments out to disk.
					 */
					if ( procdisk(pp1) != 0 ) {
						unlock( pnxgate );
						goto xxx;
					}
					continue;
				}

				/*
				 * Process too important to swap out.
				 */
				if ((pp1->p_ival > -64) && (pp1->p_sval != 0))
					continue;

				/*
				 * Less important.
				 */
				if ( pp1->p_ival < i ) {
					i = pp1->p_ival;
					pp3 = pp1;
				}
			}
			unlock( pnxgate );

			/*
			 * No processes to swap out.
			 */
			if ( pp3 == NULL ) {
#ifndef NOMONITOR
				if (swmflag)
					printf("No one to swap out\n");
#endif
				break;
			}

			/*
			 * Process is too important to swap out.
			 */
			if ( i > 0 ) {
#ifndef NOMONITOR
				if (swmflag)
					printf("Dispatch(%p, %d)\n",
						pp3, pp3->p_pid);
#endif
				pp3->p_flags |= PFDISP;
				break;
			}
#ifndef NOMONITOR
			if (swmflag)
				printf("Swapout(%p, %d)\n", pp3, pp3->p_pid);
#endif
			/*
			 * Swap process out to disk.
			 */
			procdisk( pp3 );
		}

#ifndef NOMONITOR
		if (swmflag)
			printf("Swapdone\n");
#endif
	con:
		kcall( Ktimeout, &stimer, NSRTICK, Kwakeup, (char *)&stimer );
		sleep( (char *)&stimer, CVSWAP, IVSWAP, SVSWAP );
	}
	sexflag--;
	uexit( 1 );
}

/*
 * See if the given process may fit in core.
 */
testcore( pp )
register PROC *pp;
{
	register SEG *sp;
	register fsize_t s;
	register paddr_t s1;
	register paddr_t s2;
	register int i;

	/*
	 * Find largest segment in process.
	 */
	s = 0;
	for ( i = 0; i < NUSEG+1; i++ ) {

		if ( (sp = pp->p_segp[i]) == NULL )
			continue;

		/*
		 * Segment is memory resident.
		 */
		if ( (sp->s_flags & SFCORE) != 0 )
			continue;

		/*
		 * Largest segment so far.
		 */
		if ( sp->s_size > s )
			s = sp->s_size;
	}

	/*
	 * See if largest segment will fit in memory.
	 */
	s1 = corebot;
	sp = &segmq;
	do {
		/*
		 * Advance to next memory segment.
		 */
		sp = sp->s_forw;
		s2 = sp->s_paddr;

		/*
		 * It fits!
		 */
		if ( s2 - s1 >= s )
			return (1);

		/*
		 * Compute start of next hole.
		 */
		s1 = sp->s_paddr + sp->s_size;

	} while ( sp != &segmq );

	return( 0 );
}

/*
 * Swap all segments associated with a particular process into core.
 * The number of segments still swapped out is returned.
 */
proccore( pp )
register PROC *pp;
{
	register SEG *sp;
	register int i;
	register int n;
	register int f;

	f = pp->p_flags & PFSWAP;

	/*
	 * Try to swap in all user segments and the auxiliary segment.
	 */
	for ( n = 0, i = 0; i < NUSEG+1; i++ ) {

		if ( (sp = pp->p_segp[i]) == NULL )
			continue;

		/*
		 * Process was swapped out.
		 */
		if ( f != 0 )
			sp->s_lrefc++;

		/*
		 * Segment is disk resident - try to swap it in.
		 */
		if ( (sp->s_flags & SFCORE) == 0 )
			if ( segcore(sp) == 0 )
				n++;
	}

	/*
	 * No segments left on disk - mark process as being memory resident.
	 */
	if ( n == 0 )
		pp->p_flags |= PFCORE;

	/*
	 * Mark process as no longer being disk resident.
	 */
	pp->p_flags &= ~PFSWAP;

	return( n );
}

/*
 * Swap out all segments associated with a given process.
 */
procdisk( pp )
register PROC *pp;
{
	register SEG *sp;
	register int i;
	register int f;
	int n;

	f = pp->p_flags & PFSWAP;

	/*
	 * Mark process as no longer being memory resident BEFORE swapping.
	 */
	pp->p_flags &= ~PFCORE;

	/*
	 * Try to swap out all user segments and the auxiliary segment.
	 */
	for ( n = 0, i = 0; i < NUSEG+1; i++ ) {

		if ( (sp = pp->p_segp[i]) == NULL )
			continue;

		/*
		 * Process not already swapped out.
		 */
		if ( f == 0 )
			sp->s_lrefc--;

		/*
		 * Segment already swapped out.
		 */
		if ( (sp->s_flags & SFCORE) == 0 )
			continue;

		/*
		 * Segment no longer referenced by a memory-resident process.
		 */
		if ( (sp->s_lrefc == 0) && (segdisk(sp) != 0) )
			n++;
	}

	/*
	 * Mark process as being disk resident.
	 */
	pp->p_flags |= PFSWAP;

	return( n );
}

/*
 * Swap the given segment into core.
 * NOTE: Although swapped out, the segment may have a descriptor table entry,
 *	 and therefore have a valid s_faddr field.
 */
segcore( sp1 )
register SEG *sp1;
{
	register SEG *sp2;

	/*
	 * Lock segmentation.
	 */
	lock( seglink );

	/*
	 * Segment has been moved to memory while we waited to lock.
	 */
	if ( (sp1->s_flags & SFCORE) != 0 ) {
		unlock(seglink);
		return( 1 );
	}

	/*
	 * Allocate a memory segment sp2.
	 */
	if ((sp2 = xmalloc( sp1->s_size )) == NULL ) {
		unlock( seglink );
		return( 0 );
	}

	/*
	 * Copy the disk segment sp1 into the memory segment sp2.
	 */
	sp1->s_lrefc++;
	swapio(0, sp2->s_paddr, sp1->s_daddr, sp2->s_size );
	sp1->s_lrefc--;

	/*
	 * Remove segment sp1 from the disk queue.
	 */
	sp1->s_back->s_forw = sp1->s_forw;
	sp1->s_forw->s_back = sp1->s_back;

	/*
	 * Insert segment sp1 into memory queue replacing segment sp2.
	 */
	sp2->s_back->s_forw = sp1;
	sp1->s_back = sp2->s_back;
	sp2->s_forw->s_back = sp1;
	sp1->s_forw = sp2->s_forw;

	/*
	 * Enable access to memory segment sp1.
	 */
	sp1->s_flags |= SFCORE;
	sp1->s_paddr = sp2->s_paddr;
	vremap( sp1 );

	/*
	 * Unlock segmentation.
	 */
	unlock( seglink );

	return( 1 );
}

/*
 * Swap the given segment out onto disk.
 */
segdisk( sp1 )
register SEG *sp1;
{
	register SEG *sp2;

	/*
	 * Lock segmentation.
	 */
	lock( seglink );

	/*
	 * Verify segment sp1 did not become busy while we waited to lock.
	 * IE: raw disk i/o, or shared code fork.
	 */
	if ( sp1->s_lrefc != 0 ) {
		unlock( seglink );
		return( 0 );
	}

	/*
	 * Segment has been moved to disk while we waited to lock.
	 */
	if ( (sp1->s_flags & SFCORE) == 0 ) {
		unlock(seglink);
		return( 1 );
	}

	/*
	 * Allocate a disk segment sp2.
	 */
	if ( (sp2 = xdalloc( sp1->s_size )) == NULL ) {
		unlock( seglink );
		return( 0 );
	}

	/*
	 * Disable access to memory segment sp1.
	 */
	sp1->s_flags &= ~SFCORE;
	sp1->s_daddr = sp2->s_daddr;
	vremap( sp1 );

	/*
	 * Copy the memory segment sp1 into the disk segment sp2.
	 */
	sp1->s_lrefc++;
	swapio( 1, sp1->s_paddr, sp2->s_daddr, sp1->s_size );
	sp1->s_lrefc--;

	/*
	 * Remove segment sp1 from the memory queue.
	 */
	sp1->s_back->s_forw = sp1->s_forw;
	sp1->s_forw->s_back = sp1->s_back;

	/*
	 * Insert segment sp1 into disk queue replacing segment sp2.
	 */
	sp2->s_back->s_forw = sp1;
	sp1->s_back = sp2->s_back;
	sp2->s_forw->s_back = sp1;
	sp1->s_forw = sp2->s_forw;

	/*
	 * Unlock segmentation.
	 */
	unlock( seglink );

	return( 1 );
}

/*
 * Allocate a segment on disk that is `n' bytes long.
 * The `seglink' gate should be locked before this routine is called.
 * This routine is the same as `sdalloc' except that we can't run out of
 * alloc space to allocate the segment and we allocate in high regions.
 * NOTE: descriptor table entries are not released.
 */
SEG *
xdalloc( s )
fsize_t s;
{
	register SEG *sp1;
	register SEG *sp2;
	register daddr_t d;
	register daddr_t d1;
	register daddr_t d2;

	d  = s / BSIZE;
	d2 = swaptop;
	sp1 = &segdq;
	do {
		if ( (sp1 = sp1->s_back) != &segdq )
			d1 = sp1->s_daddr + (sp1->s_size / BSIZE);
		else
			d1 = swapbot;

		if ( d2 - d1 >= d ) {
			sp2 = &segswap;
			kclear( (char *)sp2, sizeof(SEG) );
			sp1->s_forw->s_back = sp2;
			sp2->s_forw  = sp1->s_forw;
			sp1->s_forw  = sp2;
			sp2->s_back  = sp1;
			sp2->s_urefc = 1;
			sp2->s_lrefc = 1;
			sp2->s_size  = s;
			sp2->s_daddr = d2 - d;
			return( sp2 );
		}

		d2 = sp1->s_daddr;

	} while ( sp1 != &segdq );

	return( NULL );
}

/*
 * Allocate a segment in memory that is `n' bytes long.
 * The `seglink' gate should be locked before this routine is called.
 * This routine is the same as `smalloc' except that we can't run out of
 * alloc space to allocate the segment.
 * NOTE: Do NOT remap virtual descriptor table entry.
 *	 This is a scratch entry, and the s_faddr field is not retained.
 */
SEG *
xmalloc( s )
register fsize_t s;
{
	register SEG *sp1;
	register SEG *sp2;
	register paddr_t s1;
	register paddr_t s2;

	s1  = corebot;
	sp1 = &segmq;
	do {
		if ( (sp1 = sp1->s_forw) != &segmq )
			s2 = sp1->s_paddr;
		else
			s2 = coretop;

		if ( s2 - s1 >= s ) {
			sp2 = &segswap;
			kclear( (char *)sp2, sizeof(SEG) );
			sp1->s_back->s_forw = sp2;
			sp2->s_back = sp1->s_back;
			sp1->s_back = sp2;
			sp2->s_forw = sp1;
			sp2->s_urefc = 1;
			sp2->s_lrefc = 1;
			sp2->s_size  = s;
			sp2->s_paddr = s1;
			return( sp2 );
		}

		s1 = sp1->s_paddr + sp1->s_size;

	} while ( sp1 != &segmq );

	return( NULL );
}
