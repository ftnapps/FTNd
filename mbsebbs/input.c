/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Input functions, also for some utils.
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:		2:280/2802
 * Beekmansbos 10
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
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "input.h"
#include "timeout.h"
#include "language.h"




/*
 * Get a character string with cursor position
 */
void GetstrP(char *sStr, int iMaxLen, int Position)
{
	unsigned char	ch = 0;
	int		iPos = Position;

	if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 1");
		return;
	}
	Setraw();

	alarm_on();

	while (ch != KEY_ENTER) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == KEY_BACKSPACE) || (ch == KEY_DEL) || (ch == KEY_RUBOUT)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos] = '\0';
			} else
				putchar('\007');
		}

		if (ch > 31 && ch < 127) {
			if (iPos <= iMaxLen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * Get a character string
 */
void GetstrC(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0;
	int		iPos = 0;

	fflush(stdout);
	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 6");
		return;
	}
	Setraw();

	strcpy(sStr, "");
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos] = '\0';
			} else
				putchar('\007');
		}

		if (ch > 31 && ch < 127) {
			if (iPos <= iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 *  get a string, don't allow spaces (for Unix accounts)
 */
void GetstrU(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0;
	int		iPos = 0;

	fflush(stdout);
	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 6");
		return;
	}
	Setraw();

	strcpy(sStr, "");
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos] = '\0';
			} else
				putchar('\007');
		}

		if (isalnum(ch) || (ch == '@') || (ch == '.')) {
			if (iPos <= iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * Get a phone number, only allow digits, + and - characters.
 */
void GetPhone(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0; 
	int		iPos = 0;

	fflush(stdout);

	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 5");
		return;
	}
	Setraw();

	strcpy(sStr, "");
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) { 
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos]='\0';
			} else
				putchar('\007');
		}

		if ((ch >= '0' && ch <= '9') || (ch == '-') || (ch == '+')) {
			if (iPos <= iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else 
				putchar('\007');
		}
	}

	Unsetraw();                                           
	close(ttyfd);
	printf("\n");
}



/*
 * Get a number, allow digits, spaces, minus sign, points and comma's
 */
void Getnum(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0; 
	int		iPos = 0;

	fflush(stdout);

	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 5");
		return;
	}
	Setraw();

	strcpy(sStr, "");
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos]='\0';
			} else
				putchar('\007');
		}

		if ((ch >= '0' && ch <= '9') || (ch == '-') || (ch == ' ') \
		     || (ch == ',') || (ch == '.')) {

			if (iPos <= iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * This function gets the date from the user checking the length and
 * putting two minus signs in the right places
 */
void GetDate(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0; 
	int		iPos = 0;

	fflush(stdout);

	strcpy(sStr, "");

	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 4");
		return;
	}
	Setraw();

	alarm_on();
	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0)
				printf("\b \b");
			else
				putchar('\007');

			if (iPos == 3 || iPos == 6) {
				printf("\b \b"); 
				--iPos;
			}

			sStr[--iPos]='\0';
		}

		if (ch >= '0' && ch <= '9') {
			if (iPos < iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
				if (iPos == 2 || iPos == 5) {
					printf("-");
					sprintf(sStr, "%s-", sStr);
					iPos++;
				}
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * Get a string, capitalize only if set in config.
 */
void Getname(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0; 
	int		iPos = 0, iNewPos = 0;

	fflush(stdout);

	strcpy(sStr, "");

	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 2");
		return;
	}
	Setraw();
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos]='\0';
			} else
				putchar('\007');
		}

		if (ch > 31 && (ch < 127)) {
			if (iPos < iMaxlen) {
				iPos++;
				if (iPos == 1 && CFG.iCapUserName)
					ch = toupper(ch);

				if (ch == 32) {
					iNewPos = iPos;
					iNewPos++;
				}

				if (iNewPos == iPos && CFG.iCapUserName)
					ch = toupper(ch);
				else
					if (CFG.iCapUserName)
						ch = tolower(ch);

				if (iPos == 1 && CFG.iCapUserName)
					ch = toupper(ch);

				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * Get a Fidonet style username, always capitalize.
 */
void GetnameNE(char *sStr, int iMaxlen)         
{                                              
	unsigned char	ch = 0; 
	int		iPos = 0, iNewPos = 0;

	fflush(stdout);

	strcpy(sStr, "");

	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 2");
		return;
	}
	Setraw();
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos]='\0';
			} else 
				putchar('\007');
		}

		if (ch > 31 && ch < 127) {
			if (iPos < iMaxlen) {
				iPos++;

				if (iPos == 1)
					ch = toupper(ch);

				if (ch == 32) {
					iNewPos = iPos;
					iNewPos++;
				}

				if (iNewPos == iPos)
					ch = toupper(ch);
				else
					ch = tolower(ch);

				if (iPos == 1)
					ch = toupper(ch);

				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * Open up /dev/tty to get the password from the user 
 * because this is done in raw mode, it makes life a bit
 * more difficult. 
 * This function gets a password from a user, upto Max_passlen
 */
void Getpass(char *theword)
{
        unsigned char   c = 0;
        int             counter = 0;
        char            password[Max_passlen+1];

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
}



void Pause()
{
	int	i, x;
	char	*string;

	string = malloc(81);

	/* Press (Enter) to continue: */
	sprintf(string, "\r%s", (char *) Language(375));
	colour(CFG.CRColourF, CFG.CRColourB);
	printf(string);
	
	do {
		fflush(stdout);
		fflush(stdin);
		alarm_on();
		i = Getone();
	} while ((i != '\r') && (i != '\n'));

	x = strlen(string);
	for(i = 0; i < x; i++)
		printf("\b");
	for(i = 0; i < x; i++)
		printf(" ");
	for(i = 0; i < x; i++)
		printf("\b");
	fflush(stdout);

	free(string);
}


