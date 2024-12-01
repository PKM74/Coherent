/*
 * strtok.c
 * ANSI 4.11.5.8.
 * Break string into tokens.
 */

#include <stddef.h>
#include <string.h>

char *
strtok(s1, s2) register char *s1; char *s2;
{
	static char *cp1;

	if (s1 != NULL)
		cp1 = s1;		/* reset static start pointer */
	s1 = cp1 + strspn(cp1, s2);	/* skip leading delimiters */
	if (*s1 == '\0')
		return(NULL);		/* no more tokens */
	cp1 = s1 + strcspn(s1, s2);	/* skip nondelimiters */
	if (*cp1 != '\0')
		*cp1++ = '\0';		/* NUL-terminate token */
	return(s1);
}
