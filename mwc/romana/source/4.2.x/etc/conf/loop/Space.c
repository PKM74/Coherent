/* Generated from Space.spc on Thu Aug  4 18:49:49 1994 CDT */
#define	_DDI_DKI	1
#define	_SYSV4		1

/*
 * Configurable information for the loop-around driver.
 */

#include "conf.h"
#include "loop.h"

struct loop	loop_loop [LOOP_COUNT];
int		loop_cnt = LOOP_COUNT;
