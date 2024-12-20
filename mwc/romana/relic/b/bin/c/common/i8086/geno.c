/*
 * Output one and two operand opcodes
 * with their address fields.
 * Used in cc1 and cc2.
 */
#ifdef   vax
#include "INC$LIB:mch.h"
#include "INC$LIB:host.h"
#include "INC$LIB:ops.h"
#include "INC$LIB:var.h"
#include "INC$LIB:varmch.h"
#include "INC$LIB:opcode.h"
#include "INC$LIB:stream.h"
#else
#include "mch.h"
#include "host.h"
#include "ops.h"
#include "var.h"
#include "varmch.h"
#include "opcode.h"
#include "stream.h"
#endif
extern int *genaf();

genone(op)
{
	bput(CODE);
	bput(op);
	genaf(&op + 1);
}

gentwo(op)
{
	bput(CODE);
	bput(op);
	genaf(genaf(&op + 1));
}

int *
genaf(mp)
register int *mp;
{
	register int mode;

	iput(mode = *mp++);
	if ((mode & A_OFFS) != 0)
		iput(*mp++);
	if ((mode & A_LID) != 0)
		iput(*mp++);
	else if ((mode & A_GID) != 0)
		sput(*((char **)mp)++);
	return (mp);
}
