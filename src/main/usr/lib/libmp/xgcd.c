#include "mprec.h"


/*
 *	Xgcd sets the value of the mint pointed to by "g" to the greatest
 *	common divisor of the mints pointed to by "a" and "b".
 *	It sets the mints "r" and "s" so that the following are true:
 *		g = r * a + s * b
 *	Note that the only restriction as to the distinctness
 *	of the arguments is that "r", "s" and "g" are all different.
 */

void
xgcd(a, b, r, s, g)
mint *a, *b, *r, *s, *g;
{
	register mint *temp;
	mint	*r1, *s1, *t, *u;
	mint	al, bl, quot, t1;

	/* make local copyies of arguments */
	minit(&al);
	mcopy(a, &al);
	a = &al;
	minit(&bl);
	mcopy(b, &bl);
	b = &bl;

	/*
	 * The following matrix equation will always hold:
	 *
	 *	r1 s1	     original a     a
	 *	t  u  times  original b  =  b
	 */

	r1 = itom(1);
	s1 = itom(0);
	t  = itom(0);
	u  = itom(1);

	/* calculate gcd by Euclidean algorithm */
	minit(&quot);
	minit(&t1);
	while (!zerop(b)) {
		/*
		 * Perform following matrix assignment:
		 *
		 *	a  r1 s1	0      1	 a  r1 s1
		 *	b  t  u	=	1  -quot  times  b  t  u
		 *
		 * (where quot = a/b).  This is done by the factorization:
		 *
		 *	0      1     0  1	  1  -quot
		 *	1  -quot  =  1  0  times  0      1
		 */

		mdiv(a, b, &quot, a);
		mult(&quot, t, &t1);
		msub(r1, &t1, r1);
		mult(&quot, u, &t1);
		msub(s1, &t1, s1);
		temp = r1;
		r1 = t;
		t = temp;
		temp = s1;
		s1 = u;
		u = temp;
		temp = a;
		a = b;
		b = temp;
	}

	/* set results */
	mpfree(g->val);
	*g = *a;
	mpfree(r->val);
	*r = *r1;
	mpfree(s->val);
	*s = *s1;

	/* throw away garbage */
	mpfree(quot.val);
	mpfree(t1.val);
	mpfree(b->val);
	mpfree(r1);
	mpfree(s1);
	mintfr(t);
	mintfr(u);
}
