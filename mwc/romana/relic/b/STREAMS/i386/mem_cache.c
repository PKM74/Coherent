/*
 * mem_cache.c - remember correspondences between virtual and physical
 *	addresses.
 */
#include <sys/coherent.h>

#ifndef NULL
#define NULL	((char *) 0)
#endif /* NULL */

/*
 * We maintain a dynamicly allocated linked list of vaddr/paddr pairs.
 */
typedef struct cache_entry {
	struct cache_entry *next;
	struct cache_entry *prev;

	caddr_t vaddr;
	paddr_t paddr;
} CACHE_ENTRY;

static CACHE_ENTRY *find_vaddr();
static CACHE_ENTRY *find_paddr();

/*
 * cache_head is the head of the linked list of stored addresses.
 */
static CACHE_ENTRY *cache_head = NULL;

/*
 * Remember that 'vaddr' and 'paddr' correspond.
 *
 * If we run out of memory, we just do a printf().  :-(
 */
void
mem_remember(vaddr, paddr)
	caddr_t vaddr;
	paddr_t paddr;
{
	CACHE_ENTRY *entry;

	T_PIGGY( 0x40, printf("mem_remember(v:%x, p:%x)", vaddr, paddr));

	/* Be sure to overwrite any existing entry.  */
	if (NULL != (entry = find_paddr(paddr))) {
		entry->vaddr = vaddr;
		entry->paddr = paddr;
	} else { /* We need to create a new entry.  */
		if (NULL == (entry = kalloc(sizeof(CACHE_ENTRY)))) {
			printf(
			"mem_remember(v:%x, p:%x): no more kernel memory.\n",
			vaddr, paddr);
			return;
		}

		T_PIGGY( 0x40, printf(": creating %x,", entry));

		/* Store the data.  */
		entry->vaddr = vaddr;
		entry->paddr = paddr;

		/* Insert 'entry' at the head of the list.  */
		entry->prev = NULL;
		entry->next = cache_head;
		if (NULL != cache_head) { cache_head->prev = entry; }

		cache_head = entry;
	}
} /* mem_remember() */

/*
 * Forget the vaddr/paddr pair that goes with 'vaddr'.
 * Do nothing if we didn't know about vaddr.
 */
void
mem_forget(vaddr)
{
	CACHE_ENTRY *entry, *my_next, *my_prev;
	T_PIGGY( 0x40, printf("mem_forget(v:%x)", vaddr));

	if (NULL != (entry = find_vaddr(vaddr))) {
		T_PIGGY( 0x40, printf("forgetting(p:%x)", entry->paddr));

		/* Remove 'entry' from the linked list.  */
		my_next = entry->next;
		my_prev = entry->prev;

		if (NULL != my_next) { my_next->prev = my_prev; }
		if (NULL != my_prev) { my_prev->next = my_next; }

		if (entry == cache_head) { cache_head = my_next; }

		kfree(entry);
	}
} /* mem_forget() */


/*
 * Recall the vaddr that goes with 'paddr'.  Returns 0 if we don't know
 * what paddr goes with.
 */
caddr_t
mem_recall(paddr)
	paddr_t paddr;
{
	CACHE_ENTRY *entry;
	caddr_t retval;

	T_PIGGY( 0x40, printf("mem_recall(%x)=", paddr));

	if (NULL == (entry = find_paddr(paddr))) {
		retval = 0;
	} else {
		retval = entry->vaddr;
	}

	T_PIGGY( 0x40, printf("%x, ", retval));

	return retval;
} /* mem_recall() */


/*
 * Given a vaddr, 'vaddr', find the corresponding cache_entry.
 * Returns NULL if it can not find 'vaddr'.
 */
static CACHE_ENTRY *
find_vaddr(vaddr)
	register caddr_t vaddr;
{
	register CACHE_ENTRY *entry;

	/*
	 * Walk forward through the cache looking for a matching address.
	 * If we don't find it, 'entry' will be NULL, which is exactly
	 * what we want.
	 */
	for (entry = cache_head; entry != NULL; entry = entry->next) {
		if (vaddr == entry->vaddr) {
			break;
		}
	}

	return( entry );
} /* find_vaddr() */


/*
 * Given a paddr, 'paddr', find the corresponding cache_entry.
 * Returns NULL if it can not find 'paddr'.
 */
static CACHE_ENTRY *
find_paddr(paddr)
	register paddr_t paddr;
{
	register CACHE_ENTRY *entry;

	/*
	 * Walk forward through the cache looking for a matching address.
	 * If we don't find it, 'entry' will be NULL, which is exactly
	 * what we want.
	 */
	for (entry = cache_head; entry != NULL; entry = entry->next) {
		if (paddr == entry->paddr) {
			break;
		}
	}

	return( entry );
} /* find_paddr() */
