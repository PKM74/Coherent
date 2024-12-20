#ifndef	BUILDOBJ_H
#define	BUILDOBJ_H

/*
 * This file declares structures and function prototypes for some routines
 * useful for building up objects a piece at a time.
 */

/*
 *-IMPORTS:
 *	<sys/compat.h>
 *		EXTERN_C_BEGIN
 *		EXTERN_C_END
 *		VOID
 *		PROTO ()
 *	<stddef.h>
 *		size_t
 */

#include <sys/compat.h>
#include <stddef.h>

/*
 * The following code can be used as the basis of a system for building up
 * objects such as symbol-table entries incrementally. It's kind of like the
 * GNU obstack system (and clearly inspired by it), but it's not free
 * software. It's also a little more clearly laid out, and easier to
 * understand and modify, and slower.
 */

#ifndef	BUILD_T
#define	BUILD_T
typedef struct builder		build_t;
#endif


/*
 * Return values from build functions.
 */

enum {
	BUILD_CORRUPT = -10,
	BUILD_STACK_EMPTY = -9,
	BUILD_BAD_NESTING = -8,
	BUILD_NOT_LAST = -7,
	BUILD_SIZE_OVERFLOW = -6,
	BUILD_NO_MEMORY = -5,
	BUILD_NO_OBJECT = -4,
	BUILD_OBJECT_BEGUN = -3,
	BUILD_NULL_BASE = -2,
	BUILD_NULL_HEAP = -1,
	BUILD_OK
};

/*
 * Special value for "init" parameter in functions below that causes newly
 * allocated memory to be cleared.
 */

#define	INIT_ZERO	((VOID *) 1)


EXTERN_C_BEGIN

build_t	      *	builder_alloc	PROTO ((size_t _chunksize, size_t _align));
void		builder_free	PROTO ((build_t * _heap));

VOID	      *	build_malloc	PROTO ((build_t * _heap, size_t _size));

int		build_begin	PROTO ((build_t * _heap, size_t _size,
					  VOID * _init));
int		build_add	PROTO ((build_t * _heap, size_t _size,
					  VOID * _init));
int		build_addchar	PROTO ((build_t * _heap, char _ch));
VOID	      *	build_end	PROTO ((build_t * _heap, size_t * _size));

int		build_release	PROTO ((build_t * _heap, VOID * _base));

CONST char    *	build_error	PROTO ((int _errcode));

int		build_push	PROTO ((build_t * _heap));
int		build_pop	PROTO ((build_t * _heap));

EXTERN_C_END

#endif	/* ! defined (BUILDOBJ_H) */
