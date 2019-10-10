/*********************************************************************
*                         COPYRIGHT NOTICE                           *
**********************************************************************
*        This software is copyright (C) 1982 by Pavel Curtis         *
*                                                                    *
*        Permission is granted to reproduce and distribute           *
*        this file by any means so long as no fee is charged         *
*        above a nominal handling fee and so long as this            *
*        notice is always included in the copies.                    *
*                                                                    *
*        Other rights are reserved except as explicitly granted      *
*        by written permission of the author.                        *
*                Pavel Curtis                                        *
*                Computer Science Dept.                              *
*                405 Upson Hall                                      *
*                Cornell University                                  *
*                Ithaca, NY 14853                                    *
*                                                                    *
*                Ph- (607) 256-4934                                  *
*                                                                    *
*                Pavel.Cornell@Udel-Relay   (ARPAnet)                *
*                decvax!cornell!pavel       (UUCPnet)                *
*********************************************************************/

/*
 *	comp_parse.c -- The high-level (ha!) parts of the compiler,
 *			that is, the routines which drive the scanner,
 *			etc.
 *
 *   $Log:	comp_parse.c,v $
 * Revision 1.11  93/04/12  14:13:03  bin
 * Udo: third color update
 * 
 * Revision 1.5  92/06/02  12:04:31  bin
 * *** empty log message ***
 * 
 * Revision 1.2  92/04/13  14:36:22  bin
 * update by vlad
 * 
 * Revision 3.2  91/07/28  13:59:10  munk
 * Made all the large arrays static
 *
 * Revision 3.1  84/12/13  11:19:32  john
 * Revisions by Mark Horton
 * 
 * Revision 2.1  82/10/25  14:45:43  pavel
 * Added Copyright Notice
 * 
 * Revision 2.0  82/10/24  15:16:39  pavel
 * Beta-one Test Release
 * 
 * Revision 1.3  82/08/23  22:29:39  pavel
 * The REAL Alpha-one Release Version
 * 
 * Revision 1.2  82/08/19  19:09:53  pavel
 * Alpha Test Release One
 * 
 * Revision 1.1  82/08/12  18:37:12  pavel
 * Initial revision
 * 
 *
 */

#ifdef RCSHDR
static char RCSid[] =
	"$Header: /src386/usr/lib/ncurses/RCS/comp_parse.c,v 1.11 93/04/12 14:13:03 bin Exp Locker: bin $";
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include "compiler.h"
#include "term.h"
#include "object.h"

char	*string_table;
int	next_free;	/* next free character in string_table */
int	table_size = 0; /* current string_table size */
short	term_names;	/* string table offset - current terminal */
int	part2 = 0;	/* set to allow old compiled defns to be used */
int	complete = 0;	/* 1 if entry done with no forward uses */

struct use_item
{
	long	offset;
	struct use_item	*fptr, *bptr;
};

struct use_header
{
	struct use_item	*head, *tail;
};

struct use_header	use_list = {NULL, NULL};
int			use_count = 0;

/*
 *  The use_list is a doubly-linked list with NULLs terminating the lists:
 *
 *	   use_item    use_item    use_item
 *	  ---------   ---------   ---------
 *	  |       |   |       |   |       |   offset
 *        |-------|   |-------|   |-------|
 *	  |   ----+-->|   ----+-->|  NULL |   fptr
 *	  |-------|   |-------|   |-------|
 *	  |  NULL |<--+----   |<--+----   |   bptr
 *	  ---------   ---------   ---------
 *	  ^                       ^
 *	  |  ------------------   |
 *	  |  |       |        |   |
 *	  +--+----   |    ----+---+
 *	     |       |        |
 *	     ------------------
 *	       head     tail
 *	          use_list
 *
 */


/*
 *	compile()
 *
 *	Main loop of the compiler.
 *
 *	get_token()
 *	if curr_token != NAMES
 *	    err_abort()
 *	while (not at end of file)
 *	    do an entry
 *
 */

compile()
{
	static char		line[1024];
	int			token_type;
	struct use_item	*ptr;
	int			old_use_count;

	token_type = get_token();

	if (token_type != NAMES)
	    err_abort("File does not start with terminal names in column one");
	
	while (token_type != EOF)
	    token_type = do_entry((struct use_item *) 0);

	DEBUG(2, "Starting handling of forward USE's\n", "");

	for (part2=0; part2<2; part2++) {
	    old_use_count = -1;
	DEBUG(2, "\n\nPART %d\n\n", part2);
	    while (use_list.head != NULL  &&  old_use_count != use_count)
	    {
		old_use_count = use_count;
		for (ptr = use_list.tail; ptr != NULL; ptr = ptr->bptr)
		{
		    fseek(stdin, ptr->offset, 0);
		    reset_input();
		    if ((token_type = get_token()) != NAMES)
			syserr_abort("Token after a seek not NAMES");
		    (void) do_entry(ptr);
		    if (complete)
			dequeue(ptr);
		}

		for (ptr = use_list.head; ptr != NULL; ptr = ptr->fptr)
		{
		    fseek(stdin, ptr->offset, 0);
		    reset_input();
		    if ((token_type = get_token()) != NAMES)
			syserr_abort("Token after a seek not NAMES");
		    (void) do_entry(ptr);
		    if (complete)
			dequeue(ptr);
		}
		
		DEBUG(2, "Finished a pass through enqueued forward USE's\n", "");
	    }
	}

	if (use_list.head != NULL)
	{
	    fprintf(stderr, "\nError in following up use-links.  Either there is\n");
	    fprintf(stderr, "a loop in the links or they reference non-existant\n");
	    fprintf(stderr, "terminals.  The following is a list of the entries\n");
	    fprintf(stderr, "involved:\n\n");

	    for (ptr = use_list.head; ptr != NULL; ptr = ptr->fptr)
	    {
		fseek(stdin, ptr->offset, 0);
		fgets(line, 1024, stdin);
		fprintf(stderr, "%s", line);
	    }

	    exit(1);
	}
}


dump_list(str)
char *str;
{
	struct use_item *ptr;
	static char line[512];

	fprintf(stderr, "dump_list %s\n", str);
	for (ptr = use_list.head; ptr != NULL; ptr = ptr->fptr)
	{
		fseek(stdin, ptr->offset, 0);
		fgets(line, 1024, stdin);
		fprintf(stderr, "ptr %x off %d bptr %x fptr %x str %s",
		ptr, ptr->offset, ptr->bptr, ptr->fptr, line);
	}
	fprintf(stderr, "\n");
}


/*
 *	int
 *	do_entry(item_ptr)
 *
 *	Compile one entry.  During the first pass, item_ptr is NULL.  In pass
 *	two, item_ptr points to the current entry in the use_list.
 *
 *	found-forward-use = FALSE
 *	re-initialise internal arrays
 *	save names in string_table
 *	get_token()
 *	while (not EOF and not NAMES)
 *	    if found-forward-use
 *		do nothing
 *	    else if 'use'
 *		if handle_use() < 0
 *		    found-forward-use = TRUE
 *          else
 *	        check for existance and type-correctness
 *	        enter cap into structure
 *	        if STRING
 *	            save string in string_table
 *	    get_token()
 *      if ! found-forward-use
 *	    clear CANCELS out of the structure
 *	    dump compiled entry into filesystem
 *
 */

int
do_entry(item_ptr)
struct use_item	*item_ptr;
{
	long					entry_offset;
	int					i;
	register int				token_type;
	register struct name_table_entry	*entry_ptr;
	int					found_forward_use = FALSE;
	static char				Booleans[BOOLCOUNT];
	static short				Numbers[NUMCOUNT],
						Strings[STRCOUNT];

	init_structure(Booleans, Numbers, Strings);
	complete = 0;
	term_names = save_str(curr_token.tk_name);
	DEBUG(2, "Starting '%s'\n", curr_token.tk_name);
	entry_offset = curr_file_pos;

	for (token_type = get_token();
		token_type != EOF  &&  token_type != NAMES;
		token_type = get_token())
	{
	    if (found_forward_use)
		/* do nothing */ ;
	    else if (strcmp(curr_token.tk_name, "use") == 0)
	    {
		if (handle_use(item_ptr, entry_offset,
					Booleans, Numbers, Strings) < 0)
		    found_forward_use = TRUE;
	    }
	    else
	    {
		entry_ptr = find_entry(curr_token.tk_name);

		if (entry_ptr == NOTFOUND) {
		    warning("Unknown Capability - '%s'",
                                                          curr_token.tk_name);
		    continue;
		}


		if (token_type != CANCEL
                                        &&  entry_ptr->nte_type != token_type)
		    warning("Wrong type used for capability '%s'",
							  curr_token.tk_name);
		switch (token_type)
		{
		    case CANCEL:
			switch (entry_ptr->nte_type)
			{
			    case BOOLEAN:
				Booleans[entry_ptr->nte_index] = -2;
				break;

			    case NUMBER:
				Numbers[entry_ptr->nte_index] = -2;
				break;

			    case STRING:
				Strings[entry_ptr->nte_index] = -2;
				break;
			}
			break;
		
		    case BOOLEAN:
			Booleans[entry_ptr->nte_index] = TRUE;
			break;
		    
		    case NUMBER:
			Numbers[entry_ptr->nte_index] =
                                                      curr_token.tk_valnumber;
			break;

		    case STRING:
			Strings[entry_ptr->nte_index] =
                                            save_str(curr_token.tk_valstring);
			break;

		    default:
			warning("Unknown token type");
			panic_mode(',');
			continue;
		}
	    } /* end else cur_token.name != "use" */

	} /* endwhile (not EOF and not NAMES) */

	if (found_forward_use)
	    return(token_type);

	for (i=0; i < BOOLCOUNT; i++)
	{
	    if (Booleans[i] == -2)
		Booleans[i] = FALSE;
	}

	for (i=0; i < NUMCOUNT; i++)
	{
	    if (Numbers[i] == -2)
		Numbers[i] = -1;
	}

	for (i=0; i < STRCOUNT; i++)
	{
	    if (Strings[i] == -2)
		Strings[i] = -1;
	}

	dump_structure(term_names, Booleans, Numbers, Strings);

	complete = 1;
	return(token_type);
}


/*
 *	enqueue(offset)
 *
 *      Put a record of the given offset onto the use-list.
 *
 */

enqueue(offset)
long	offset;
{
	struct use_item	*item;
	char *malloc();

	item = (struct use_item *) malloc(sizeof(struct use_item));

	if (item == NULL)
	    syserr_abort("Not enough memory for use_list element");

	item->offset = offset;

	if (use_list.head != NULL)
	{
	    item->bptr = use_list.tail;
	    use_list.tail->fptr = item;
	    item->fptr = NULL;
	    use_list.tail = item;
	}
	else
	{
	    use_list.tail = use_list.head = item;
	    item->fptr = item->bptr = NULL;
	}

	use_count ++;
}


/*
 *	dequeue(ptr)
 *
 *	remove the pointed-to item from the use_list
 *
 */

dequeue(ptr)
struct use_item	*ptr;
{
	if (ptr->fptr == NULL)
	    use_list.tail = ptr->bptr;
	else
	    (ptr->fptr)->bptr = ptr->bptr;

	if (ptr->bptr == NULL)
	    use_list.head = ptr->fptr;
	else
	    (ptr->bptr)->fptr = ptr->fptr;
	
	use_count --;
}


/*
 *	dump_structure()
 *
 *	Save the compiled version of a description in the filesystem.
 *
 *	make a copy of the name-list
 *	break it up into first-name and all-but-last-name
 *	creat(first-name)
 *	write object information to first-name
 *	close(first-name)
 *      for each name in all-but-last-name
 *	    link to first-name
 *
 */

dump_structure(term_names, Booleans, Numbers, Strings)
short	term_names;
char	Booleans[];
short	Numbers[];
short	Strings[];
{
	char		*strcpy();
	struct stat	statbuf;
	FILE		*fp;
	static char	name_list[1024];
	register char	*first_name, *other_names;
	register char	*ptr;
	static char	filename[50];
	static char	linkname[50];
	extern char check_only;

	strcpy(name_list, term_names + string_table);
	DEBUG(7, "Name list = '%s'\n", name_list);

	first_name = name_list;

	ptr = &name_list[strlen(name_list) - 1];
	other_names = ptr + 1;

	while (ptr > name_list  &&  *ptr != '|')
	    ptr--;

	if (ptr != name_list)
	{
	    *ptr = '\0';

	    for (ptr = name_list; *ptr != '\0'  &&  *ptr != '|'; ptr++)
		;
	    
	    if (*ptr == '\0')
		other_names = ptr;
	    else
	    {
		*ptr = '\0';
		other_names = ptr + 1;
	    }
	}

	if (check_only) {
		DEBUG(1, "Checked %s\n", first_name);
		return;
	}

	DEBUG(7, "First name = '%s'\n", first_name);
	DEBUG(7, "Other names = '%s'\n", other_names);

	if (strlen(first_name) > 100)
	    warning("'%s': terminal name too long.", first_name);

	check_name(first_name);

	sprintf(filename, "%c/%s", first_name[0], first_name);

	if (stat(filename, &statbuf) >= 0  &&  statbuf.st_mtime >= start_time)
	{
	    warning("'%s' defined in more than one entry.", first_name);
	    fprintf(stderr, "Entry being used is '%s'.\n",
			    (unsigned) term_names + string_table);
	}

	unlink(filename);
	fp = fopen(filename, "w");
	if (fp == NULL)
	{
	    perror(filename);
	    syserr_abort("Can't open %s/%s\n", destination, filename);
	}
	DEBUG(1, "Created %s\n", filename);

	if (write_object(fp, term_names, Booleans, Numbers, Strings) < 0)
	{
	    syserr_abort("Error in writing %s/%s", destination, filename);
	}
	fclose(fp);

	while (*other_names != '\0')
	{
	    ptr = other_names++;
	    while (*other_names != '|'  &&  *other_names != '\0')
		other_names++;

	    if (*other_names != '\0')
		*(other_names++) = '\0';

	    if (strlen(ptr) > 100)
	    {
		warning("'%s': terminal name too long.", ptr);
		continue;
	    }

	    sprintf(linkname, "%c/%s", ptr[0], ptr);

	    if (strcmp(filename, linkname) == 0)
	    {
		warning("Terminal name '%s' synonym for itself", first_name);
	    }
	    else if (stat(linkname, &statbuf) >= 0  &&
						statbuf.st_mtime >= start_time)
	    {
		warning("'%s' defined in more than one entry.", ptr);
		fprintf(stderr, "Entry being used is '%s'.\n",
			    (unsigned) term_names + string_table);
	    }
	    else
	    {
		unlink(linkname);
		if (link(filename, linkname) < 0)
		    syserr_abort("Can't link %s to %s", filename, linkname);
		DEBUG(1, "Linked %s\n", linkname);
	    }
	}
}


/*
 *	int
 *	write_object(fp, term_names, Booleans, Numbers, Strings)
 *
 *	Write out the compiled entry to the given file.
 *	Return 0 if OK or -1 if not.
 *
 */

#define swap(x)		(((x >> 8) & 0377) + 256 * (x & 0377))

#define might_swap(x)	(must_swap()  ?  swap(x)  :  (x))


int
write_object(fp, term_names, Booleans, Numbers, Strings)
FILE	*fp;
short	term_names;
char	Booleans[];
short	Numbers[];
short	Strings[];
{
    	struct header	header;
	char		*namelist;
	short		namelen;
	char		zero = '\0';
	int		i;

	namelist = term_names + string_table;
	namelen = strlen(namelist) + 1;

	if (must_swap())
	{
	    header.magic = swap(MAGIC);
	    header.name_size = swap(namelen);
	    header.bool_count = swap(BOOLCOUNT);
	    header.num_count = swap(NUMCOUNT);
	    header.str_count = swap(STRCOUNT);
	    header.str_size = swap(next_free);
	}
	else
	{
	    header.magic = MAGIC;
	    header.name_size = namelen;
	    header.bool_count = BOOLCOUNT;
	    header.num_count = NUMCOUNT;
	    header.str_count = STRCOUNT;
	    header.str_size = next_free;
	}

	if (fwrite(&header, sizeof(header), 1, fp) != 1
		||  fwrite(namelist, sizeof(char), namelen, fp) != namelen
		||  fwrite(Booleans, sizeof(char), BOOLCOUNT, fp) != BOOLCOUNT)
	    return(-1);
	
	if ((namelen+BOOLCOUNT) % 2 != 0  &&  fwrite(&zero, sizeof(char), 1, fp) != 1)
	    return(-1);

	if (must_swap())
	{
	    for (i=0; i < NUMCOUNT; i++)
		Numbers[i] = swap(Numbers[i]);
	    for (i=0; i < STRCOUNT; i++)
		Strings[i] = swap(Strings[i]);
	}

	if (fwrite((char *) Numbers, sizeof(short), NUMCOUNT, fp) != NUMCOUNT
	       ||  fwrite((char *) Strings, sizeof(short), STRCOUNT, fp) != STRCOUNT
	       ||  fwrite((char *) string_table, sizeof(char), next_free, fp)
								  != next_free)
	    return(-1);

	return(0);
}


/*
 *	check_name(name)
 *
 *	Generate an error message if given name does not begin with a
 *	digit or letter (should be lower-case letter, but SV likes capital)
 *	Vlad 3-11-92
 */

check_name(name)
char	*name;
{
	if (!isalnum(name[0]))
	{
	    fprintf(stderr, "tic: Line %d: Illegal terminal name - '%s'\n",
							    curr_line, name);
	    fprintf(stderr,
			"Terminal names must start with letter or digit\n");
	    exit(1);
	}
}


/*
 *	int
 *	save_str(string)
 *
 *	copy string into next free part of string_table, doing a realloc()
 *	if necessary.  return offset of beginning of string from start of
 *	string_table.
 *
 */

int
save_str(string)
char	*string;
{
	char	*malloc(), *realloc(), *strcpy();
	int	old_next_free = next_free;

	if (table_size == 0)
	{
	    if ((string_table = malloc(1024)) == NULL)
		syserr_abort("Out of memory");
	    table_size = 1024;
	    DEBUG(5, "Made initial string table allocation.  Size is %d\n",
								    table_size);
	}

	while (table_size < next_free + strlen(string))
	{
	    if ((string_table = realloc(string_table, table_size + 1024))
									== NULL)
		syserr_abort("Out of memory");
	    table_size += 1024;
	    DEBUG(5, "Extended string table.  Size now %d\n", table_size);
	}

	strcpy(&string_table[next_free], string);
	DEBUG(7, "Saved string '%s' ", string);
	DEBUG(7, "at location %d\n", next_free);
	next_free += strlen(string) + 1;

	return(old_next_free);
}


/*
 *	init_structure(Booleans, Numbers, Strings)
 *
 *	Initialise the given arrays
 *	Reset the next_free counter to zero.
 *
 */

init_structure(Booleans, Numbers, Strings)
char	Booleans[];
short	Numbers[], Strings[];
{
	int	i;

	for (i=0; i < BOOLCOUNT; i++)
	    Booleans[i] = FALSE;
	
	for (i=0; i < NUMCOUNT; i++)
	    Numbers[i] = -1;

	for (i=0; i < STRCOUNT; i++)
	    Strings[i] = -1;

	next_free = 0;
}


/*
**	int
**	handle_use(item_ptr, entry_offset, Booleans, Numbers, Strings)
**
**	Merge the compiled file whose name is in cur_token.valstring
**	with the current entry.
**
**		if it's a forward use-link
**	    	    if item_ptr == NULL
**		        queue it up for later handling
**	            else
**		        ignore it (we're already going through the queue)
**	        else it's a backward use-link
**	            read in the object file for that terminal
**	            merge contents with current structure
**
**	Returned value is 0 if it was a backward link and we
**	successfully read it in, -1 if a forward link.
*/

int
handle_use(item_ptr, entry_offset, Booleans, Numbers, Strings)
long		entry_offset;
struct use_item	*item_ptr;
char		Booleans[];
short		Numbers[];
short		Strings[];
{
	struct term	use_term;
	struct stat	statbuf;
	static char	filename[50];
        int             i;

	check_name(curr_token.tk_valstring);

	sprintf(filename, "%c/%s", curr_token.tk_valstring[0],
                                                     curr_token.tk_valstring);

	if (stat(filename, &statbuf) < 0  ||  part2==0 && statbuf.st_mtime < start_time)
	{
	    DEBUG(2, "Forward USE to %s", curr_token.tk_valstring);

 	    if (item_ptr == NULL)
 	    {
 		DEBUG(2, " (enqueued)\n", "");
 		enqueue(entry_offset);
 	    }
 	    else 
 		DEBUG(2, " (skipped)\n", "");
	    
	    return(-1);
	}
	else
	{
	    DEBUG(2, "Backward USE to %s\n", curr_token.tk_valstring);
	    if (read_entry(filename, &use_term) < 0)
		syserr_abort("Error in re-reading compiled file %s", filename);

	    for (i=0; i < BOOLCOUNT; i++)
	    {
		if (Booleans[i] == FALSE  &&  use_term.Booleans[i] == TRUE)
		    Booleans[i] = TRUE;
	    }

	    for (i=0; i < NUMCOUNT; i++)
	    {
		if (Numbers[i] == -1  &&  use_term.Numbers[i] != -1)
		    Numbers[i] = use_term.Numbers[i];
	    }

	    for (i=0; i < STRCOUNT; i++)
	    {
		if (Strings[i] == -1  &&  use_term.Strings[i] != (char *) 0)
		    Strings[i] = save_str(use_term.Strings[i]);
	    }

	}
	return(0);
}