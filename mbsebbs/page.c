/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Sysop Paging
 * Todo ..................: Implement new config settings.
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "dispfile.h"
#include "input.h"
#include "chat.h"
#include "page.h"
#include "timeout.h"
#include "mail.h"
#include "language.h"


/*
 * Function to Page Sysop
 */
void Page_Sysop(char *String)
{
	int	i;
	FILE	*pWritingDevice, *pBusy;
	int	iOpenDevice = FALSE; /* Flag to check if you can write to CFG.sChatDevice */
	char	*Reason;
	char	temp[81];

	Reason = calloc(81, sizeof(char));

	clear();
	colour(12, 0);
	/* MBSE BBS Chat */
	Center((char *) Language(151));

	if (CFG.iAskReason) {
		locate(6, 0);
		colour(1, 0);
		printf("%c", 213);
		for(i = 0; i < 78; i++) 
			printf("%c", 205);
		printf("%c ", 184);

		colour(7, 0);
		for(i = 0; i < 78; i++) 
			printf("%c", 250);
		printf("\n");

		colour(1, 0);
		printf("%c", 212);
		for(i = 0; i < 78; i++) 
			printf("%c", 205);
		printf("%c\n", 190);

		locate(7, 2);

		colour(7, 0);
		fflush(stdout);
		GetPageStr(temp, 76);

		colour(1, 0);
		printf("%c", 212);
		for(i = 0; i < 78; i++) 
			printf("%c", 205);
		printf("%c\n", 190);

		if((strcmp(temp, "")) == 0)
			return;

		Syslog('+', "Chat Reason: %s", temp);
		strcpy(Reason, temp);
	}

	if (access("/tmp/.BusyChatting", F_OK) == 0) {
		if((pBusy = fopen("/tmp/.BusyChatting", "r")) == NULL)
			WriteError("Unable to open BusyChatting file", pTTY);
		else {
			fscanf(pBusy, "%10s", temp); 
			fclose(pBusy);
		}
		colour(13, 0);
		printf("%s%s\n", (char *) Language(152), temp);
		pout(10, 0, (char *) Language(153));
		Enter(2);
		Syslog('+', "SysOp was busy chatting to user on %s", temp);
		Pause();
		free(Reason);
		return;
	}

	CFG.iMaxPageTimes--;

	if(CFG.iMaxPageTimes <= 0) {
		if (!DisplayFile((char *)"maxpage")) {
			/* If i is FALSE display hard coded message */
			Enter(1);
			pout(15, 0, (char *) Language(154));
			Enter(2);
		}

		Syslog('!', "Attempted to page Sysop, above maximum page limit.");
		Pause();
	} else {
		locate(14, ((80 - strlen(String) ) / 2 - 2));
		pout(15, 0, (char *)"[");
		pout(7, 0, String);
		pout(15, 0, (char *)"]");

		locate(16, ((80 - CFG.iPageLength) / 2 - 2));
		pout(15, 0, (char *)"[");
		colour(1, 0);
		for(i = 0; i < CFG.iPageLength; i++)
			printf("%c", 176);
		pout(15, 0, (char *)"]");

		locate(16, ((80 - CFG.iPageLength) / 2 - 2) + 1);

		if((pWritingDevice = fopen(CFG.sChatDevice, "w")) != NULL) {
			fprintf(pWritingDevice, "\n\n\n%s is trying to page you on %s.\n", \
				usrconfig.sUserName, pTTY);

			fprintf(pWritingDevice, "Type: '%s/bin/schat %s' to talk to him, you have", \
				getenv("MBSE_ROOT"), pTTY);
			fprintf(pWritingDevice, "\n%d seconds to respond!\n\n", CFG.iPageLength);
				fprintf(pWritingDevice, "%s\n", temp);
			iOpenDevice = TRUE;
		}

		colour(9, 0);
		for(i = 0; i < CFG.iPageLength; i++) {
			printf("\x07");
			/* If there weren't any problems opening the writing device */
			if(iOpenDevice)
				fprintf(pWritingDevice, "\x07");
			printf("%c", 219);
			fflush(pWritingDevice);
			fflush(stdout);
			sleep(1);
			if(access("/tmp/chatdev", R_OK) == 0) {
				fclose(pWritingDevice);
				Chat();
				free(Reason);
				return;
			}
		}

		fclose(pWritingDevice);
		PageReason();
		printf("\n\n\n");
		if (strlen(Reason))
			SysopComment(Reason);
		else
			SysopComment((char *)"Failed chat");
	}

	free(Reason);
 	Pause();
}



/* 
 * Function gets string from user for Pager Function
 */
void GetPageStr(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0; 
	int		iPos = 0;

	if ((ttyfd = open ("/dev/tty", O_RDWR)) < 0) {
		perror("open 6");
		return;
	}
	Setraw();

	strcpy(sStr, "");

	alarm_on();
	while (ch != 13) {
		ch = Readkey();

		if (((ch == 8) || (ch == KEY_DEL) || (ch == 127)) && (iPos > 0)) {
			printf("\b%c\b", 250);
			fflush(stdout);
			sStr[--iPos]='\0';
		}

		if (ch > 31 && ch < 127) {
			if (iPos <= iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
				fflush(stdout);
			} else
				ch=13;
		}
	}

	Unsetraw();
	close(ttyfd);
	printf("\n");
}



/*
 * Function gets page reason from a file in the txtfiles directory
 * randomly generates a number and prints the string to the screen
 */
void PageReason()
{
	FILE	*Page;
	int	iLoop = FALSE, id, i, j = 0;
	int	Lines = 0, Count = 0, iFoundString = FALSE;
	char	*String;
	char	*temp;

	temp = calloc(128, sizeof(char));
	String = calloc(81, sizeof(char));

	sprintf(temp, "%s/page.asc", CFG.bbs_txtfiles);
	if(( Page = fopen(temp, "r")) != NULL) {

		while (( fgets(String, 80 ,Page)) != NULL)
			Lines++;

		rewind(Page);

		Count = Lines;
		id = getpid();
		do {
			i = rand();
			j = i % id;
			if ((j <= Count) && (j != 0))
				iLoop = 1;
		} while (!iLoop);

		Lines = 0;

		while (( fgets(String,80,Page)) != NULL) {
			if(Lines == j) {
				Striplf(String);
				locate(18, ((78 - strlen(String) ) / 2));
				pout(15, 0, (char *)"[");
				pout(9, 0, String);
				pout(15, 0, (char *)"]");
				iFoundString = TRUE;
			}

			Lines++; /* Increment Lines until correct line is found */
		}
	} /* End of Else */

	if(!iFoundString) {
		/* Sysop currently is not available ... please leave a comment */
		sprintf(String, "%s", (char *) Language(155));
		locate(18, ((78 - strlen(String) ) / 2));
		pout(15, 0, (char *)"[");
		pout(9, 0, String);
		pout(15, 0, (char *)"]");
	}

	free(temp);
	free(String);
}

