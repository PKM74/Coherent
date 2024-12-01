/*
 * Vertical hex dump subroutine
 * dumps 64 bytes per line
 */

static
clean(c)
register char c;
{
	return ((c >= ' ' && c <= '~' ) ? c : '.');
}

static unsigned int linepos = 0; /* how many non sep chars output so far */
/*
 * Put char and separators.
 */
static void
outc(c)
char c;
{
	if(!(linepos & 3))
		putchar(linepos ? ' ' : '\n');
	putchar(c);
	linepos++;
}

static
hex(c)
register char c;
{
	return(c + ((c <= 9) ? '0' : 'A' - 10));
}

xdump(p, length)
register unsigned char *p;
{
	register int tlength;
	register unsigned char *l;

	for(;(tlength = (length > 64) ? 64 : length) > 0; length -= 64) {
		for(linepos = 0, l = p; linepos < tlength; )
			outc(clean(*p++));

		for(linepos = 0, p = l; linepos < tlength; )
			outc(hex(((*p++) >> 4) & 15));

		for(linepos = 0, p = l; linepos < tlength; )
			outc(hex((*p++) & 15));
	}
	putchar('\n');
}
