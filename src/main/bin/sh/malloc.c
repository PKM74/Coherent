/* allocate and free routines
 * all blocks are allocated in multiples of 2
 * with the first word as the true length
 * the lo order bit is 1 if the block is free.
 * A word of zero marks the end of an arena and points
 * to the start of the next arena. Arenas point in a circle.
 */

#include <stdio.h>
#include <malloc.h>

struct mblock	*_a_scanp = NULL;	/* search starts here */

static unsigned depth = 0; 		/* depth of malloc recursion */

static char msg[] = "Bad pointer in free.\n";

/* get a new arena from sbrk and hook it to the old */

static void
newarea( len)
register unsigned len;
{
	register struct mblock *newa, *newb, *prev;

	char *passbug = BADSBRK;	/* beat large model c bug */

	static struct mblock *_a_topar = NULL;

	unsigned cbrk = (unsigned)sbrk( 0);

	if( len > 65500L)
		return;
	len += sizeof(struct mblock);
	if( len < 512)
		if( cbrk < 65536L - 511)
			len = 512;
		else
			len = -cbrk;
#if Z8001
	else if( (unsigned long)cbrk+len > 65536L)
		newarea( - cbrk - sizeof( struct mblock));
#endif
	if( passbug == (newa = sbrk(len)))
		return;

	if(_a_topar == NULL)		/* first time through */
		_a_scanp = prev = newa;

	else if(_a_topar == newa) {	/* new over old */
		--newa;
		len += sizeof(struct mblock);
		prev = newa->uval.next;

	} else {			/* discontigous arenas */
		newb = _a_topar;
		prev = (--newb)->uval.next;	/* save old ptr */
		newb->uval.next = newa;		/* old pts to new */
	}
	newa->blksize = (len - sizeof(struct mblock)) | FREE;
	newb = _a_topar = adr(newa) + len;
	(--newb)->blksize = 0;
	newb->uval.next = prev;
	return;
}

char *
malloc( size)
unsigned size;
{
	register struct mblock *ptr, *optr;
	register unsigned len, siz;

	siz = roundup(size + sizeof(unsigned), POW2);
	if(siz < size)
		return(NULL);
	optr = NULL;
	if((ptr = _a_scanp) != NULL) {
	    do {
		if(isfree(len = ptr->blksize)) { /* free block */
			if(optr != NULL) { /* consolidate free */
				len = (optr->blksize += realsize(len));
				ptr = optr;
			}
			optr = ptr;
			if(len > siz) { /* got one big enough */
				if((len -= siz)<LEASTFREE) /* close enough */
					ptr->blksize = realsize(ptr->blksize);
				else {
					ptr->blksize = siz;
					_a_scanp = siz + adr(ptr);
					_a_scanp->blksize = len;
				}
				return(ptr->uval.usera);
			}
		} else			/* used block or area ptr */
			optr = NULL;

		if(len)
			ptr = realsize(len) + adr(ptr);
		else
			ptr = ptr->uval.next;	/* new arena */
	    } while(ptr != _a_scanp);
	}

	if(optr != NULL)
		_a_scanp = optr;

	/* no room in the arena or first time */
	if(++depth >= 2) {
		depth--;
		return(NULL);
	}
	newarea(siz);
	ptr = malloc(size);
	depth--;
	return(ptr);
}

/* free a block */
free(cp)
char *cp;
{
	register struct mblock *ap;

	ap = cp - sizeof(unsigned);
	if(!ap->blksize) {
		write(2, msg, strlen(msg));
		abort();
	}
	ap->blksize |= FREE;	/* mark free even if free */
}
