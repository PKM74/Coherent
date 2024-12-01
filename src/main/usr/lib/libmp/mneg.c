#include "mprec.h"
#include <assert.h>


/*
 *	Mneg sets the mint pointed to by "b" to negative 1 times the
 *	mint pointed to by "a".  Note that "a" == "b" is permissable.
 */

void
mneg(a, b)
mint *a, *b;
{
	register char *ap, *rp;
	register unsigned count;
	mint res;
	int mifl;

	/* allocate result space */
	mifl = ispos(a);
	res.len = a->len;
	if (mifl)
		++res.len;
	res.val = (char *)mpalc(res.len);

	/* negate and copy */
	ap = a->val;
	rp = res.val;
	count = a->len;
	while (count-- > 0)
		*rp++ = NEFL - *ap++;
	if (mifl)
		*rp = NEFL;
	++*res.val;
	assert(ap == a->val + a->len);
	assert(rp == res.val + res.len - (mifl ? 1 : 0));
	norm(&res);

	/* replace old value of b with res */
	mpfree(b->val);
	*b = res;
}
