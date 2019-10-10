/* taylor.c: This file opens the sys file and looks for the specific system
 *	     name supplied by the user. A 1 is returned for success and 0
 *	     for failure. Id the sys file is empty, the program will exit.
 */

#include <stdio.h>
#include <fcntl.h>

#define SYSFILE "/usr/lib/uucp/sys"

check_sys_file(sysname)
char * sysname;			/* system name to look for */
{
	int x = 0;
	FILE * configfile;

	if ( (configfile = fopen(SYSFILE,"r")) == NULL){
		printf("Error opening %s.\n",SYSFILE);
		exit(1);
	}


	/* file exists, but is empty */
	x = read_entries(configfile, sysname);

	if(x == -1){
		fclose(configfile);
		printf("File /usr/lib/uucp/sys is empty.\n");
		exit(1);
	}

	fclose(configfile);
	return(x);

}


/* read the information from the specified file. We want to keep track of the
 * port, system or dialer name we have read, the line we began reading it 
 * on and the total number of entries read;
 */


read_entries(configfile, sysname)
FILE * configfile;		/* pointer to our configuration file */
char * sysname;			/* name of system we are looking for */
{

	int x = 0;			/* counter of lines read */

	char buffer[256];
	char lookfor[8];

	strcpy(lookfor,"system");

	while(fgets(buffer,sizeof(buffer) -1, configfile) != NULL){
		x++;

		/* skip commented lines */

		if(buffer[0] == '#')
			continue;

		/* found first line of a configuration entry, copy name and
		 * the line number we found it on.
		 */

		if(strstr(buffer,lookfor) && strstr(buffer, sysname))
			return(1);
	}

	if(x == 0)
		return(-1);

	return(0);
}