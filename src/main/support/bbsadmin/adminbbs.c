#include "contents.h"
#include <stdio.h>
#include <curses.h>

char selection[15];
char filenames [MAXRECORDS][15];
char workfile[15];
char workstring[80];
char getfiles[26][115];
char open_mode;
int place[MAXRECORDS];
int limit, screen_num;

void bubble();	/* this is a bubble sort */
void show_files(); /* this should display the filenames on a curses screen */
int lite(); /* inverse/normal video display of a filename */
int rfile(); /* read records from a given file */
void write_win(); /*does the actual work of writing filenames to a window */
void display_form(); /* for for displaying selected filename */
void display_record(); /* display selected filename */
void menu(); /* menu printed at bottom of screen */
void del_rec(); /* this will be used to delete records */
void add_rec(); /* this will be used to add records */
void getstring(); /* this will be called by add_rec to get input */
void build_uucp(); /* this will build multiple uucp requests */

struct entry{
		char filename [15];
		char filesize [10];
		char date[7];
		char description [78];
		char requires [60];
		char notes [78];
		char pathname [60];
		int noparts;
	    }


main(argc, argv)
int argc;
char *argv[];
{

int x;


	if(argc < 2)
	{
		printf("Usage: mwcbbs [adr] filename\n");
		printf("[a] add\n[d] download\n[r] remove\n");
		exit(1);
	}


	if(strlen(argv[2]) == 0)
	{
		printf("filename not specified. Please enter a filename: ");
		gets (workfile);
	}
	else

		strcpy(workfile, argv[2]);

	if(argv[1][0] != 'a')
		if(argv[1][0] != 'd')
			if(argv[1][0] != 'r')
				{
				printf("Option %c not recognized!\n",argv[1][0]);
				exit(1);
				}

	open_mode = argv[1][0];

	screen_num = 0;

	x = rfile();
	show_files  (x);

}




/*	show_files()
 *	This function will display the filenames read to a curses
 *	screen.
*/


void show_files(EOF_FLAG)
int EOF_FLAG;

{
char arrow;
int prevcol =1;
int prevrow =0;			/* prevcol = column before arrow	  */
int newrow =0;			/* prevrow = row before arrow	 	  */
int newcol =1;  		/* newrow = row after arrow		  */
int counter = 0;		/* newcol = column after arrow		  */
				
				
WINDOW *win1, *win2;		
				
	initscr();		
	noecho();
	raw();

	/* allocate memory for window. print message on failure. */

	if((win1=newwin(20, 79, 0,0)) == NULL)
	{
		printf("\007Window Memroy allocation for win1 failed!\n");
		exit(1);
	}

	if((win2=newwin(20, 79, 0,0)) == NULL)
	{
		printf("\007Window Memroy allocation for win2 failed!\n");
		exit(1);
	}

	write_win(win1);
	menu();


/*  highlite a filename. This is accomplished by going to a designated
 * row and column, as determined by the row and counter nested loops.
 * The innermost loop gets the character found, copies the retrieved 
 * character into a string and deletes the character from the screen.
 * When the filename has been deleted from the screen, it is reprinted
 * to the screen with highliting turned on. Padding for spaces must be
 * accounted for since deleting chars shifts everything on the line one
 * space to the left.
*/

	/* print the first file in inverse video */

	lite (win1, prevrow, prevcol, 1);

	do
	{

	/* now we need to get a key (preferably an arrow) */



	      arrow = getch();   /* This stupid code should allow to use arrows keys   */
	      if (arrow == 27)  /* that looks more frendly than hjkl. Vlad 8/15/91    */
		{
	         getch();
	         arrow = getch(); /* When an arrow key is pressed, an escape  */
	         if (arrow == 68) /* sequence is returned. The value '27'     */
	            arrow = 'h';  /* begins the sequence and the relevant     */
	         if (arrow == 67) /* values needed end the sequence. The      */
        	    arrow = 'l';  /* middle value is not needed, so it is     */
         	 if (arrow == 66) /* skipped over with a getch() statement    */
         	   arrow = 'j';
	         if (arrow == 65)
        	    arrow = 'k';            	
      	         }


/* each movement case in the following switch...case will test to see if the
 * new position returns a space. If a space is returned, then we will hit
 * an empty field, which we don't want to do. If we hit an empty space,
 * then don't move the cursor.
*/

		switch(arrow)
		{
			case 'h':	/* move left */
				newcol = prevcol - 15;
				if (newcol < 1)
					newcol = 61;
				if (' ' == mvwinch(win1,newrow,newcol))
					newcol = 1;
				break;


			case 'l':	/* move right */
				newcol = prevcol + 15;
				if (newcol > 61)
					newcol = 1;
				if (' ' == mvwinch(win1,newrow,newcol))
					newcol = 1;
				break;


			case 'j':	/* move down */
				newrow = prevrow + 1;
				if (newrow == 20)
					newrow = 0;
				if (' ' == mvwinch(win1,newrow,newcol))
					newrow = 0;
				break;


			case 'k':	/* move up */
				newrow = prevrow -1;
				if (newrow == -1)
					newrow = 19;
				if (' ' == mvwinch(win1,newrow,newcol))
					newrow = 0;
				break;


			case 'p':	
				screen_num --;
				newrow = 0;
				newcol = 1;
				if (screen_num < 0)
					screen_num = 0;
				else
				{
					EOF_FLAG = rfile();
					write_win(win1);
				}
				break;
	

			case 'n':	
				newrow = 0;
				newcol = 1;
				if (EOF_FLAG == -1)
					break;
				else
				{
					screen_num ++;
					EOF_FLAG = rfile();
					write_win(win1);
				}
				break;	


			case 'q':
				break;

			case 'Q':
				break;

			case 'S':
			case 's':
				wclear(win1);
				wrefresh(win1);
				wclear(win2);
				display_form(win2);
				if (open_mode == 'a')
					add_rec(win2, prevrow, prevcol, screen_num);
				else
					display_record(win2,prevrow, prevcol, screen_num);
				clear();
				menu();
				if( open_mode == 'a' || open_mode == 'r')
					{
					wmove (win2,0,0);
					waddstr(win2,"Reading file. Please wait...");
					wrefresh(win2);	
					rfile();
					}
				write_win(win1);
				wclear(win2);
				wrefresh(win2);
				refresh();
				wrefresh(win1);
				break;

			default:
				newcol = prevcol;
				newrow = prevrow;
				break;

		}

	/* print previous file highlited in normal video */
	lite (win1, prevrow, prevcol, 0);	

	/* print new filename selection in inverse video */

	lite (win1, newrow, newcol, 1);

	/* set our previous coordinates so that we can go back
	 * and unlite a field when the next directin is given.
	*/

		prevrow = newrow;
		prevcol = newcol;

	
	}
	while (arrow != 'q');

	echo();
	noraw();
	endwin();
}
		

		

/* this routine will take a pointer to a window and row/col
 * coordinates and print a filename found at the coordinates
 * in INVERSE video.
*/

int lite(win1, row, col, liteflag)

WINDOW *win1;
int row, col, liteflag;

{

int x,y;
char litestring[15];

	wmove(win1, row,col);

	/* get existing filename char by char, deleting each
	 * char as it is read into a string variable.
	*/
	
	for(x=0; x<15; x++)
	{
		litestring[x] = winch(win1);
	/* if we hit a space, we've hit the end of the filename */
		if(litestring[x] == ' ')
			break;
		wdelch(win1);
	}

	litestring[x] = '\0';


	/* repad the spaces to keep the remaning filenames
	 * on this row in their proper columns.
	*/

	for(y=0;y<x;y++)
		winsch(win1,' ');

	if (liteflag == 1)
		wstandout(win1);

	waddstr(win1, litestring);
	wmove(win1, row, col);

	if (liteflag == 1)
		wstandend(win1);


	/* copy filename to a string to be displayed on stdscr.
	 * as a filename is highlited, it is also displayed on
	 * stdscr. 'selection' will be used in case the user
	 * hits return to locate the filename's entry in the
	 * Contents file.
	*/

	strcpy(selection, litestring);
	move (23,0);
	printw("Selected filename is: %14s", selection);

	refresh();
	wrefresh(win1);

	return(0);

}
		

		


int rfile()

{
struct entry record;
FILE *infp;
int EOF_FLAG;

	/* open file, abort on error */

	if ((infp = fopen(workfile,"r")) == NULL)
		{
		printf("\007ERROR opening file for input!\n");
		exit(1);
		}

	/* open a file then go to a an offset calculated by a 
	 * value passed from the calling program. Once we hit
	 * the max number of records that can be held by a screen,
	 * terminate the read and set a flag to show that we did
	 * not hit EOF. If we did hit EOF, terminate the loop
	 * and set a flag to show that we did hit EOF.
	*/

	/* start reading from a specified point in the file */
	fseek(infp, (sizeof (struct entry) * (screen_num * 100)), 0l);

	limit = 0;
	while((fread(&record, sizeof(struct entry),1, infp)) != 0)
		{
		strcpy(filenames[limit], record.filename);
		place[limit] = limit;
		limit++;
		if (limit == 100)
			break;
		}

	/* if x made it to 100, then we did NOT hit EOF */

if ( (limit == 100) && (fread (&record,sizeof(struct entry),1,infp)) !=  0)
		EOF_FLAG = 0;
	else
		EOF_FLAG = -1;

	fclose(infp);
	return (EOF_FLAG);

}



/*
 * writewin();
 * this routine does the actual work of writing filenames to a window
*/

void write_win(win1)

WINDOW *win1;

{

int r,c;	/* these are our rows and columns */
int counter = 0;	/* this counts the number of files written */



	/* clear the window */
	wclear(win1);

	/* the following loop will write the filenames to the window.
	 * 15 characters per screen field are allowed, since a filename 
	 * can only be 14 chars in length. This will leave at least one 
	 * space between filenames.
	*/

		for (r = 0; r < 20; r++)
		{
	
			/* if we've run out of files, terminate loop */

			if (counter == limit)
				break;

			/* increment column by 15 positions. This
			 * will cause the filenames to line up
			 * on the window.
			*/
	
			for (c = 1; c < 75; c+= 15)
			{
			wmove(win1, r, c);
			waddstr(win1, filenames[counter]);

		/* increment counter. When it equals the number of
		 * records, passed as 'limit', the loop should
		 * terminate.
		*/

			counter ++;
			if (counter == limit)
				break;
			}
		}


}

/* this function will draw a template for the selected record */

void display_form(win2)
WINDOW *win2;


{
int x;

	clear();
	wclear(win2);

	/* print field labels */

	wmove (win2, NAMELOCATE );
	waddstr (win2,"Filename:");

	wmove (win2, DESCLOCATE);
	waddstr(win2,"Description:");

	wmove(win2, DATELOCATE );
	waddstr(win2,"Date added/modified:");

	wmove(win2, SIZELOCATE);
	waddstr(win2,"File size:");

	if(open_mode != 'd')
		{
		wmove(win2, PARTLOCATE);;
		waddstr(win2,"Number of parts to download:");
		}

	wmove(win2, REQLOCATE );
	waddstr(win2,"Requires these other files:");

	wmove(win2, NOTELOCATE);
	waddstr(win2,"Other notes:");

	if(open_mode != 'd')
		{
		wmove(win2, PATHLOCATE);
		waddstr(win2, "Path to download file... Must give complete path including filename.");
		wmove (win2, PATHLOCATE2);
		waddstr(win2,"If a file is split into several parts, enter the first filename part");
		wmove (win2, PATHLOCATE3);
		waddstr(win2,"leaving the LAST character off of the filename:");
		}


/* highlite available fields */

	wstandout(win2);

	wmove (win2, NAMEHI);
	waddstr(win2,"              ");

	wmove (win2,DESCHI);
	for (x=1;x<79;x++)
		waddstr(win2," ");

	wmove(win2, DATEHI);
	waddstr(win2,"      ");

	wmove(win2, SIZEHI);
	waddstr(win2,"          ");

	if (open_mode != 'd')
		{
		wmove(win2,PARTHI);
		waddstr(win2,"  ");
		}

	wmove (win2,REQHI);
	for (x=1;x<61;x++)
		waddstr(win2," ");

	wmove (win2,NOTEHI);
	for (x=1;x<69;x++)
		waddstr(win2," ");

	if(open_mode != 'd')
		{
		wmove (win2, PATHHI);
		waddstr(win2,PATHNAME);
			for (x=23;x<61;x++)
				waddstr(win2," ");
		}

	wstandend(win2);
	refresh();
	wrefresh(win2);



}

/* following function prints the menu at the bottom of stdscr */

void menu()

{
/* print a menu of options to stdscr. They will appear at the bottom of
 * the screen with the first letter highlited as an indication to
 * the user that pressing the highlited key will result in the indicated
 * action.
*/

	if(open_mode =='r')
		{
		move (21,0);
		printw("Select file to delete");
		refresh();
		}

	if(open_mode == 'a')
		{
		move (21,0);
		printw("select file that will FOLLOW");
		move (22,0);
		printw("the file to be added.");
		refresh();
		}

	if(open_mode == 'd')
		{
		move (21,0);
		printw("Select file to download.");
		refresh();
		}

	move(21, 50);
	standout();
	printw("Options:");
	move(22,46);
	printw("n");
	move(23,46);
	printw("p");
	move(22,60);
	printw("q");
	move(23,60);
	printw("s");
	standend();
	move(22,47);
	addstr("ext page");
	move(23,47);
	addstr("rev. page");
	move(22,61);
	addstr("uit");
	move(23,61);
	addstr("elect file");


}


/* this function will open the file and read the appropriate record,
 * then display it on the window in the approp. positions.
*/

void display_record (win2, row, col, screen_num)
WINDOW *win2;
int row, col, screen_num;

{
WINDOW *win3;
struct entry record;
char choice;
FILE *infp;
int x;

	if ((infp = fopen(workfile,"r")) == NULL)
	{
		printf("\007ERROR opening file for input!\n");
		exit(1);
	}

	fseek(infp,REC_FORMULA, 0l);
	fread(&record, sizeof (struct entry),1,infp);
	fclose (infp);

	wstandout(win2);

	wmove(win2,NAMEHI);
	waddstr(win2,record.filename);

	wmove(win2,DESCHI);
	waddstr(win2,record.description);

	wmove(win2,DATEHI);
	wprintw(win2,"%.6s",record.date);

	wmove(win2,SIZEHI);
	waddstr(win2,record.filesize);

	if(open_mode != 'd')
		{
		wmove(win2,PARTHI);
		wprintw(win2,"%.2d",record.noparts);
		}

	wmove(win2,REQHI);
	waddstr(win2,record.requires);

	wmove(win2,NOTEHI);
	waddstr(win2,record.notes);

	if(open_mode != 'd')
		{
		wmove(win2,PATHHI);
		waddstr(win2,record.pathname);
		}

	wrefresh(win2);
	wstandend(win2);


/* allocate another window as a message area */

	if ( (win3=newwin(2,40,20,0)) == NULL)
		{
		printf("Memory allocation for win3 failed!\n");
		exit(1);
		}

/* the following tests for 'd' for delete from command line. If the user
 * used a 'd' on the command line, make sure that he/she really wants to
 * delete the record, then call the del_reocrd function.
*/
	if(open_mode == 'r')
		{
		wmove (win3,0,0);
		waddstr(win3,"Do you wish to delete this record?");
		wmove(win3,1,0);
		waddstr(win3, "[y] yes; any other key to abort.");
		wrefresh(win3);

		choice = '\0';
		while (choice == '\0')
			choice = wgetch(win3);

		if ((choice == 'y') || (choice == 'Y'))
			{
			wclear(win3);
			wmove(win3,0,0);
			waddstr(win3,"working... please wait");
			wrefresh(win3);
			del_rec(win3, row, col, screen_num);
			}
		
		wclear(win3);
		wmove(win3,0,0);
		waddstr(win3,"Press RETURN to continue...");
		wrefresh(win3);
		while(13 != wgetch(win3))
			;
		wclear(win3);
		wrefresh(win3);
		delwin(win3);
		}

	/* we are in download mode, so prompt for the next data screen */
	else
		{
		wclear(win3);
		wmove(win3,0,0);
		waddstr(win3,"Press <RETURN> for next screen.");
		wrefresh(win3);	
		while(13 != wgetch(win3))
			;
		wclear(win2);
		wmove(win2,1,0);
		waddstr(win2,"Size of file is: ");
		wmove(win2,1,40);
		waddstr(win2,"Number of parts to download: ");
		wstandout(win2);
		wmove(win2,1,18);
		waddstr(win2,record.filesize);
		wmove(win2,1,69);
		record.noparts = (record.noparts == 0) ? 1 : record.noparts;
		wprintw(win2,"%d",record.noparts);
		wstandend(win2);
		wmove(win2,3,5);
		wprintw(win2,"The following commands will be needed to download ");
		wstandout(win2);
		waddstr(win2,record.filename);
		wstandend(win2);

	/* if there is more than one part to download, call a function to generate
	 * the multiple uucp requests necessary to grab each piece from mwcbbs.
	*/

		if (record.noparts >1)
			{
			build_uucp(record);
			for (x=0;x<record.noparts;x++)
				{

	/* limit ourselves to 10 displayed commands */
				if (x==10)
					{
					wmove(win2,16,0);
					wprintw(win2,"There are %d more parts to download which do not appear on ths screen.", (record.noparts - x));
					break;
					}

				wmove(win2,5+x,0);
				waddstr(win2,getfiles[x]);
				}
			}
		else
			{
			strcpy(getfiles[0],HOST);
			strcat(getfiles[0],record.pathname);
			strcat(getfiles[0],RECEIVER);
			wmove(win2,5,0);
			waddstr(win2,getfiles);
			wrefresh(win2);
			}

		wclear(win3);
		wmove(win3,0,0);
		waddstr(win3,"Do you wish to download this file?");
		wmove(win3,1,0);
		waddstr(win3,"[y] yes or any other key to abort.");
		wrefresh(win2);
		wrefresh(win3);
		choice = '\0';
		while(choice == '\0')
			choice = wgetch(win3);

		if(choice == 'y' || choice == 'Y')
			{	
			wclear(win3);
			wmove(win3,0,0);
			waddstr(win3,"Processing requests...");
			wrefresh(win3);
			for(x = 0; x < record.noparts; x++)
				system(getfiles[x]);
			wclear(win3);
			wmove(win3,0,0);
			waddstr(win3,"Press <RETURN> to continue.");
			wrefresh(win3);
			while(13 != wgetch(win3))
				;
			}
		}
}


void del_rec(win3,row,col,screen_num,rec_add)
WINDOW *win3;
int row,col,screen_num;
struct entry *rec_add;

{
int x;
struct entry record;
FILE *infp,*outfp;
int rec_mark = POS_FORMULA;

	if ( (infp=fopen(workfile,"r"))==NULL)
		{
		printf("Error opening file %s for input!\n",workfile);
		exit(1);
		}

	/* delete any previous temporary file that may be here */

	if (unlink(TEMPFILE)==-1);	

	if ( (outfp=fopen(TEMPFILE,"a"))==NULL)
		{
		printf("Error opening \"/tmp/mwcbbs.tmp\" for output!\n");
		exit(1);
		}


	/* we will now read each record until we hit the record number
	 * calculated by rec_mark. to delete, we will simply exit the
	 * loop when we hit the correct record, skip the record with a 
	 * read, then follow that with another read/write loop to finish
  	 * off the file.
	*/


	for(x=0;x<rec_mark;x++)
		{
		fread(&record, sizeof(struct entry),1,infp);
		wmove(win3,1,0);
		wprintw(win3,"Reading:");
		wrefresh(win3);
		fwrite(&record, sizeof(struct entry),1,outfp);
		wmove(win3,1,0);
		wprintw(win3,"Writing:");
		wrefresh(win3);
		}

	/* this will delete the record by using a dummy record to skip it */

	if(open_mode == 'r')
		fread(&record, sizeof(struct entry),1,infp);


	/* this will write in a new record, if invoked in 'add' mode */
	if(open_mode == 'a')
		fwrite(&rec_add, sizeof(struct entry),1,outfp);

	/* this will finish off the file. */

	while((fread(&record, sizeof(struct entry),1,infp)) != 0)
		{

		wmove(win3,1,0);
		wprintw(win3,"Reading");
		wrefresh(win3);
		fwrite(&record, sizeof(struct entry),1,outfp);
		wmove(win3,1,0);
		wprintw(win3,"Writing:");
		wrefresh(win3);
		}

/* close the files and move around as appropriate (delete the old then
 * move the new to old
*/

	fclose(infp);
	fclose(outfp);
	if(unlink(workfile) == -1)
		{
		endwin();
		printf("Could not remove old file!\n");
		printf("Updated file could be found as %s\n",TEMPFILE);
		exit(1);
		}

	if( link(TEMPFILE,workfile) == -1)
		{
		endwin();
		printf("Could not link new file!\n");
		exit(1);
		}

	if( unlink(TEMPFILE) == -1)
		{
		endwin();
		printf("Could not remove temporary file!\n");
		exit(1);
		}

}

/* this function will get the input and write it to a file */

void add_rec(win2, row, col, screen_num)
WINDOW *win2;

{
struct entry new_record;	

FILE *infp;
WINDOW *win3;
int x;
char choice;


	wrefresh(win2);

	if( (win3=newwin(4,79,20,0)) == NULL )
	{
		wclear(win2);
		wmove(win2,0,0);
		wstandout(win2);
		waddstr(win2,"\007Error allocating memory for win3. Press <RETURN>");
		wrefresh(win2);
		while('\n' != getch())
			;
		exit(1);
	}

	wclear(win3);
	wmove(win3,1,1);
	waddstr(win3,"Enter filename or <RETURN> to exit.");
	wrefresh(win3);
	getstring(win2, NAMEHI);
	strcpy(new_record.filename, workstring);
	if(strlen(new_record.filename) == 0)
		return;

	for(x=strlen(workstring);x < sizeof (new_record.filename) ; x++)
		new_record.filename[x] = '\0';


	wclear(win3);
	wmove(win3,1,1);
	waddstr(win3,"Describe the uses of this package");
	wrefresh(win3);

	getstring(win2, DESCHI);
	strcpy(new_record.description, workstring);

	for(x=strlen(workstring);x < sizeof (new_record.description) ; x++)
		new_record.description[x] = '\0';



	wclear(win3);
	wmove(win3,1,1);
	waddstr(win3,"Enter the date that the file was added or last modified");
	wmove(win3,2,1);
	waddstr(win3,"Enter no more than 6 characters");
	wrefresh(win3);

	getstring(win2,DATEHI);
	strcpy(new_record.date, workstring);
	for(x=strlen(workstring);x < sizeof (new_record.date) ; x++)
		new_record.date[x] = '\0';

	wclear(win3);
	wmove(win3,1,1);
	waddstr(win3,"Enter the file size in number of bytes");
	wrefresh(win3);
	getstring(win2, SIZEHI);
	strcpy(new_record.filesize, workstring);
	for(x=strlen(workstring);x < sizeof (new_record.filesize) ; x++)
		new_record.filesize[x] = '\0';


	wclear(win3);
	wmove(win3,3,1);
	waddstr(win3,"Enter the number of parts that this file is divided into");
	wrefresh(win3);
	getstring(win2, PARTHI);
	new_record.noparts = atoi(workstring);


	/* if the number of parts is 0 (not a split file), then we can
	 * cat the filename on to the end of the pathname. We can later
	 * test the length of the pathname to determine whether or not
	 * to ask for a pathname, as would be required for a split file.
	*/


		if(new_record.noparts ==0)
		{
			strcpy(new_record.pathname, PATHNAME);
			strcat(new_record.pathname, new_record.filename);
			wmove(win2,PATHHI);
			wstandout(win2);
			waddstr(win2, new_record.pathname);
			wstandend(win2);
			wrefresh(win2);
		}


	wclear(win3);
	wmove(win3,3,1);
	waddstr(win3,"Enter the names of any support files required");
	wrefresh(win3);
	getstring(win2, REQHI);

	if(strlen(workstring) == 0 )
		{
		wmove(win2, REQHI);
		wstandout(win2);
		waddstr(win2,"none");
		wstandend(win2);
		wrefresh(win2);
		strcpy(new_record.requires, "none");
	}
	else
		strcpy(new_record.requires, workstring);

	for(x=strlen(new_record.requires);x < sizeof (new_record.requires) ; x++)
		new_record.requires[x] = '\0';

	wclear(win3);
	wmove(win3,3,1);
	waddstr(win3,"Enter any relevant notes about this file");
	wrefresh(win3);
	getstring(win2, NOTEHI);
	if(strlen(workstring) == 0)
		{
		wmove(win2,NOTEHI);
		wstandout(win2);
		waddstr(win2,"none");
		wstandend(win2);
		wrefresh(win2);
		strcpy(new_record.notes, "none");
		}
	
	else
		strcpy(new_record.notes, workstring);
	for(x=strlen(new_record.notes);x < sizeof (new_record.notes) ; x++)
		new_record.notes[x] = '\0';

	/* if there is more than one part to download, or number of
	 * parts is not zero, we need to complete the pathlist.
	*/
	
	if(new_record.noparts != 0)
	{
		wclear(win3);
		wmove(win3,3,1);
		waddstr(win3,"Complete the pathlist to this file.");
		wrefresh(win3);
		wmove(win2,PATHHI);
		wstandout(win2);
		waddstr(win2, PATHNAME);
		wstandend(win2);
		wrefresh(win2);
		getstring(win2, PATHHI2);
		strcpy(new_record.pathname, PATHNAME);
		strcat(new_record.pathname, workstring);
	}
	for(x=strlen(new_record.pathname);x < sizeof (new_record.pathname) ; x++)
		new_record.pathname[x] = '\0';


	wclear(win3);
	wmove(win3,0,0);
	waddstr(win3,"Do you wish to write the record?");
	wmove(win3,1,0);
	waddstr(win3,"[y]es or any other to abort.");
	wrefresh(win3);
	choice ='\0';
	while(choice == '\0')
		choice = wgetch(win3);

	if(choice == 'y' || choice == 'Y')
		{
		wclear(win3);
		wmove(win3,0,0);
		waddstr(win3,"Updating...");
		wrefresh(win3);
		del_rec(win3,row,col,screen_num, new_record);
		}

	delwin(win3);

}


/* this function will get a string and print to the approp field 
 * on win2. This is to prevent people from entering too many characters.
*/

void getstring (win2, row, col)

WINDOW *win2;
int row, col;

{


	noraw();
	echo();
	wmove(win2,row,col);
	wrefresh(win2);
	wgetstr(win2,workstring);
	wstandout(win2);
	wmove(win2,row,col);
	wprintw(win2,"%s",workstring);
	wstandend(win2);
	wrefresh(win2);
	noecho();
	raw();

	

}
/* this function will take the pathname and append the necessary
 * character(s) to generate the multiple requests necessary to
 * download multipart files.
*/

void build_uucp(record)
struct entry *record;

{
int x,y;

	y = strlen(record.pathname);

	for(x=0;x< record.noparts;x++)
		{
		strcpy(getfiles[x],HOST);
		record.pathname[y] = 97 + x;
		strcat(getfiles[x],record.pathname);
		strcat(getfiles[x],RECEIVER);
		}
}
