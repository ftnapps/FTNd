/*****************************************************************************
 *
 * File ..................: bbs/pwcheck.c
 * Purpose ...............: Password checking routines
 * Last modification date : 18-Oct-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
 *   
 * Michiel Broek		FIDO:		2:280/2802
 * Beekmansbos 10		Internet:	mbroek@users.sourceforge.net
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "pwcheck.h"
#include "funcs4.h"
#include "timeout.h"


/*
 * Open up /dev/tty to get the password from the user 
 * because this is done in raw mode, it makes life a bit
 * more difficult. 
 * This function gets a password from a user, upto Max_passlen
 */
int Getpass(char *theword)
{
	unsigned char	c = 0;
	int		counter = 0;
	char		password[Max_passlen+1];

	/* 
	 * Open the device that we want to read the password from, you can't use
	 * stdin as this might change in a pipe
	 */
	if ((ttyfd = open ("/dev/tty", O_RDWR)) < 0) {
		perror("open 7");
		ExitClient(1);
	}

	/* Set Raw mode so that the characters don't echo */
	Setraw();
	alarm_on();

	/* 
	 * Till the user presses ENTER or reaches the maximum length allowed
	 */
	while ((c != 13) && (counter < Max_passlen )) {

		fflush(stdout);
		c = Readkey();  /* Reads a character from the raw device */

		if (((c == 8) || (c == KEY_DEL) || (c == 127)) && (counter != 0 )) { /* If its a BACKSPACE */
			counter--;
			password[counter] = '\0';
			printf("\x008 \x008");
			continue;
		}  /* Backtrack to fix the BACKSPACE */

		if (((c == 8) || (c == KEY_DEL) || (c == 127)) && (counter == 0) ) {
			printf("\x007");
			continue;
		} /* Don't Backtrack as we are at the begining of the passwd field */

		if (isalnum(c)) {
			password[counter] = c;
			counter++;
			printf("%c", CFG.iPasswd_Char);
		}
	}
	Unsetraw();  /* Go normal */
	close(ttyfd);

	password[counter] = '\0';  /* Make sure the string has a NULL at the end*/
	strcpy(theword,password);

	return(0);
}


