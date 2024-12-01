/*
 * Machine specific parts of the
 * debug routines.
 * Many are macros in cc1mch.h,
 * repeated here for documentation.
 * Both SMALL and LARGE model of
 * the Intel 8086.
 */
#ifdef   vax
#include "INC$LIB:cc1.h"
#else
#include "cc1.h"
#endif

/*
 * Print a pointer.
 * This is actually host dependent
 * and required even if TINY by other
 * diagnostic messages.
 */
psnap(p)
char *p;
{
#if RUNNING_LARGE
	struct	{
		int	offs;
		int	base;
	};

	printf("%04x:%04x", p.base, p.offs);
#else
#ifdef vax
	printf("%08x", (int) p);
#else
	printf("%04x", (int) p);
#endif
#endif
}

#if !TINY
/*
 * Print an ival_t.
 * #define isnap(x)	printf(" %d", x)
 */

/*
 * Print an lval_t.
 * #define lsnap(x)	printf(" %ld", x)
 */

/*
 * Print a dval_t.
 */
dsnap(d)
dval_t d;
{
	register int i;

	for (i=0; i<8; ++i)
		printf(" %02x", d[i]&0377);
}

/*
 * Print the t_cost field.
 * #define csnap(x)	(x!=0 ? printf(" cost=%d", x) : 0)
 */

/*
 * Print the t_flag field.
 * #define fsnap(x)	(x!=0 ? printf(" flag=%08lx", x) : 0)
 */

/*
 * Print machine dependent leaves.
 * #define mdlsnap(x)	snaptype(x, "Bad leaf");
 */

/*
 * Print machine dependent op nodes.
 * #define mdosnap(x)	snaptype(x, "Bad op");
 */

#endif
