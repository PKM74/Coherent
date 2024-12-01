/*
 * The routines in this file
 * write out expression trees. They are used only
 * by the modify phase.
 */
#ifdef   vax
#include "INC$LIB:cc1.h"
#else
#include "cc1.h"
#endif

/*
 * Write out a tree.
 */
treeput(tp)
register TREE	*tp;
{
	register int	op;

	if (tp == NULL) {
		iput(NIL);
		return;
	}
	op = tp->t_op;
	iput(op);
	bput(tp->t_type);
	if (tp->t_type == BLK)
		iput(tp->t_size);
	switch (op) {

	case ICON:
		iput(tp->t_ival);
		break;

	case LCON:
		lput(tp->t_lval);
		break;

	case DCON:
		dput(tp->t_dval);
		break;

	case AID:
	case PID:
		zput(tp->t_offs);
		break;

	case LID:
		zput(tp->t_offs);
		bput(tp->t_seg);
		iput(tp->t_label);
		break;

	case GID:
		zput(tp->t_offs);
		bput(tp->t_seg);
		sput(tp->t_sp->s_id);
		break;

	case REG:
		iput(tp->t_reg);
		break;

	case FIELD:
		bput(tp->t_width);
		bput(tp->t_base);
		treeput(tp->t_lp);
		break;

	default:
		treeput(tp->t_lp);
		treeput(tp->t_rp);
	}
}
