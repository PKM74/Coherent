/* (-lgl
 *	Coherent 386 release 4.2
 *	Copyright (c) 1982, 1993 by Mark Williams Company.
 *	All rights reserved. May not be copied without permission.
 *	For copying permission and licensing info, write licensing@mwc.com
 -lgl) */

#ifndef	__COMMON__XDEBUG_H__
#define	__COMMON__XDEBUG_H__

#include <common/ccompat.h>


/*
 * This file contains some simple macros for use in debugging and statistics
 * gathering code. Centralizing these definitions has no real purpose other
 * than increasing consistency for the person attempting to test code.
 *
 * Note that it would be nice if we could hook this into a source-code
 * control system's notion of version, so that in addition to file name,
 * build date, and build time we could include version number, author and
 * last change date by having macros in every .c file's beginning.
 *
 * Note that under C++, higher levels of debugging information can use the
 * constructor/destructor facilities of C++ code to create automatic
 * dependency information for files and all kinds of other useful facilities
 * if the source-code control system above can be extended for most file types.
 *
 * Because this file controls compile-time debugging detail, it is intended
 * for instances where source distribution is not desirable. The ability to
 * include multiple library versions is an invaluable aid to debugging, not
 * merely because of the extra information that may be provided in case of a
 * simply detected error, but because the addition of debugging information
 * subtly alters the characteristics of the system under test. Merely
 * including a range of different libraries can test a system for a variety
 * of the more subtle bugs that usually manifest themselves only during the
 * porting process.
 */

/*
 * We define standard names for system debugging-level macros that are
 * controlled by a single user symbol defined on the command line for the
 * compilation, _DEBUG_LEVEL. If not defined, this symbol defaults to the
 * value zero. The value of this symbol determines the level of detail
 * included in the compile-time debugging information, and/or the level of
 * run-time checking that is desired.
 *
 * This facility defines a simple linear scale of debugging detail, and is
 * intended to supplement system-dependent optional debugging facilities in
 * cases where the systems affected by a problem cannot be easily determined
 * without experimentation.
 */

#ifndef	_DEBUG_LEVEL
#define	_DEBUG_LEVEL   3
#endif


/*
 * Define a bitmap of debugging feature levels that we can combine. Note
 * that some feature sections are ordered so that the relative magnitudes
 * and order of the bits are important.
 */

#define	__FI_NAME_M__	0x0001		/* include file name */
#define	__FI_LINE_M__	0x0002		/* include file line no */
#define	__FI_DATE_M__	0x0004		/* include build date */
#define	__FI_TIME_M__	0x0008		/* include build time */
#define	__FI_USER_M__	0x0010		/* include user info */


/*
 * Turn the _DEBUG_LEVEL into a __DEBUG_PROFILE__, unless that has already
 * been overridden.
 */

#if	_DEBUG_LEVEL < 1

# define  __DEBUG_PROFILE__	0

#elif	_DEBUG_LEVEL < 2

# define  __DEBUG_PROFILE__	__FI_NAME_M__

#elif	_DEBUG_LEVEL < 3

# define  __DEBUG_PROFILE__	(__FI_NAME_M__ | __FI_LINE_M__)

#elif	_DEBUG_LEVEL < 4

# define  __DEBUG_PROFILE__	(__FI_NAME_M__ | __FI_LINE_M__ | \
				 __FI_DATE_M__)

#elif	_DEBUG_LEVEL < 5

# define  __DEBUG_PROFILE__	(__FI_NAME_M__ | __FI_LINE_M__ | \
				 __FI_DATE_M__)

#elif	_DEBUG_LEVEL < 6

# define  __DEBUG_PROFILE__	(__FI_NAME_M__ | __FI_LINE_M__ | \
				 __FI_DATE_M__ | __FI_USER_M__)

#elif	_DEBUG_LEVEL < 7

# define  __DEBUG_PROFILE__	(__FI_NAME_M__ | __FI_LINE_M__ | \
				 __FI_DATE_M__ | __FI_USER_M__)

#elif	_DEBUG_LEVEL < 8

# define  __DEBUG_PROFILE__	(__FI_NAME_M__ | __FI_LINE_M__ | \
				 __FI_DATE_M__ | __FI_TIME_M__ | \
				 __FI_USER_M__)

#else	/* anything else */

# define  __DEBUG_PROFILE__	(__FI_NAME_M__ | __FI_LINE_M__ | \
				 __FI_DATE_M__ | __FI_TIME_M__ | \
				 __FI_USER_M__)

#endif	/* switch (_DEBUG_LEVEL) */


/*
 * Before we get into the _DEBUG_LEVEL feature tests, we note that there is
 * some other information that the user might like to use to annotate the
 * __FILE_INFO__ macro, rather than just to redefine it. We allow the user
 * to define the _USER_NAME macro for the purpose of including the name of
 * the person performing the build (or other information such as the name
 * of the translator or cross-development platform being used).
 */

#ifndef	_USER_NAME

# define	_USER_NAME	no-one

#endif	/* ! defined (_USER_NAME) */

# define	__USER_NAME__	__STRINGVAL (_USER_NAME)



/*
 * The "standard" debugging macro __FILE_INFO__ can be used in place of the
 * more common __FILE__ definition in many cases. At different levels of
 * debugging information, the value of this symbol (which is an ANSI string)
 * includes more information. At debugging level 0, this macro expands to
 * an empty string, allowing it to be used without complex #if statements
 * in the client.
 *
 * A side issue is the way spaces are included in the output string; note
 * that individual feature tests are done in bit order of the feature
 * string, so we can do some funky tests of the magnitude of part of the
 * feature mask to see if anything we are interested in is following the
 * current bit. Sleazy, but it gives nice output.
 *
 * Because including line numbers as strings generates a lot of big strings
 * that cannot be merged, we provide __FILE_INFO__, which consists of a string
 * and an optional line number (zero if line numbers are not to be included),
 * and __FILE_INFO_STRING__, which is a single string.
 *
 * We also export the symbol __NO_FILE_INFO__ if we know that the value of
 * __FILE_INFO__ is an empty string, in order to allow users to replace a
 * null string expansion with a NULL value.
 */

#if	! defined (__FILE_INFO__) && ! defined (__NO_FILE_INFO__)

# define  __FI_M__	(__FI_NAME_M__ | __FI_DATE_M__ | __FI_TIME_M__ | \
			 __FI_USER_M__)

# if	(__DEBUG_PROFILE__ & __FI_NAME_M__) != 0
#  define	__FI_NAME__	__FILE__
# else
#  define	__FI_NAME__
# endif	/* no file name */


# if	(__DEBUG_PROFILE__ & __FI_LINE_M__) != 0
#  define	__FI_LINE_STR__	" Line # " __STRINGVAL (__LINE__)
#  define	__FI_LINE__	, __LINE__
#else
#  define	__FI_LINE_STR__
#  define	__FI_LINE__	, 0
#endif


# if	(__DEBUG_PROFILE__ & __FI_M__) > __FI_NAME_M__
#  define	__FI_NAME_S__	" "
# else
#  define	__FI_NAME_S__
# endif	/* don't need a space */


# if	(__DEBUG_PROFILE__ & __FI_LINE_M__) != 0
#  define	__FI_LINE_S__	" "
# else
#  define	__FI_LINE_S__
#endif


# if	(__DEBUG_PROFILE__ & __FI_DATE_M__) != 0
#  define	__FI_DATE__	" built on " __DATE__
# else
#  define	__FI_DATE__
# endif	/* no file build date */


# if	(__DEBUG_PROFILE__ & __FI_M__) > __FI_DATE_M__
#  define	__FI_DATE_S__	" "
# else
#  define	__FI_DATE_S__
# endif	/* don't need a space */


# if	(__DEBUG_PROFILE__ & __FI_TIME_M__) != 0
#  define	__FI_TIME__	" at " __TIME__
# else
#  define	__FI_TIME__
# endif	/* no file build time */


# if	(__DEBUG_PROFILE__ & __FI_M__) > __FI_TIME_M__
#  define	__FI_TIME_S__	" "
# else
#  define	__FI_TIME_S__
# endif	/* don't need a space */


# if	(__DEBUG_PROFILE__ & __FI_USER_M__) != 0
#  define	__FI_USER__	__USER_NAME__
# else
#  define	__FI_USER__
# endif	/* no file name */


/*
 * Define __NO_FILE_INFO__ iff the user has not requested anything to be
 * included in the __FILE_INFO__ macro.
 */

# if	(__DEBUG_PROFILE__ & __FI_M__) == 0
#  define	__NO_FILE_INFO__	1
# endif


/*
 * Note that we terminate these strings with a "" so that they ALWAYS expand
 * to a valid ISO C string of some form. If we are using a C compiler that
 * cannot merge constant strings, create a static string constant.
 */

# define	__FILE_INFO_FMT__	__FI_NAME__ __FI_NAME_S__ \
					__FI_DATE__ __FI_DATE_S__ \
					__FI_TIME__ __FI_TIME_S__ \
					__FI_USER__ ""

# if	! _SHARED_STRINGS
#  if	(__DEBUG_PROFILE__ & __FI_NAME_M__) != 0 && defined (__BASE_FILE__)
/*
 * Because we are evaluating __FILE__ here in the header, switch over to use
 * __BASE_FILE__ if the preprocessor knows about that.
 */
#   undef	__FI_NAME__
#   define	__FI_NAME__	__BASE_FILE__

static __CONST__ char * __CONST__ __file_info_fmt__ = __FILE_INFO_FMT__;
#   undef	__FILE_INFO_FMT__
#   define	__FILE_INFO_FMT__	__file_info_fmt__

#  endif /* defined (__BASE_FILE__) */
# endif	/* ! _SHARED_STRINGS */


# define	__FILE_INFO__		__FILE_INFO_FMT__ __FI_LINE__

# define	__FILE_INFO_STRING__ 	__FI_NAME__ __FI_NAME_S__ \
					__FI_LINE_STR__ __FI_LINE_S__ \
					__FI_DATE__ __FI_DATE_S__ \
					__FI_TIME__ __FI_TIME_S__ \
					__FI_USER__ ""

/*
 * Remove unneeded private preprocessor symbols.  Note that the __FI_xxx__
 * symbols cannot be removed because they are needed in the expansion of
 * __FILE_INFO__.  Becausee they begin with double-underscores, this should not
 * be a problem.
 */

# undef	__FI_M__

#endif	/* ! defined (__FILE_INFO__) && ! defined (__NO_FILE_INFO__) */

/*
 * Other handy stuff: many debugging systems do not maintain a separate
 * debugger global namespace, so definitions that have local or file static
 * scope are not necessarily visible.
 *
 * To get more control over the global namespace, structures that should
 * not normally appear visible but whose addresses we may wish to
 * reveal should be declared with the following storage class prefix.
 */

#ifndef	__LOCAL__
# ifdef	_SHOW

#  define	__LOCAL__

# else

#  define	__LOCAL__	static

# endif
#endif


#endif	/* ! defined (__COMMON__XDEBUG_H__) */
