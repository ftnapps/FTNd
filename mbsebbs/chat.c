/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Sysop to user chat utility
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "chat.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "misc.h"
#include "whoson.h"



#define DEVICE "/tmp/chatdev"




void Chat(void)
{
	FILE	*pGetDev, *pLog, *pBusy, *pChat;
	int	ch;
	int	iLetter = 0;
	char	sDevice[20];
	char	*sLog = NULL;
	char	temp[81] = "";

	if(CFG.iAutoLog)
		sLog = calloc(56, sizeof(char));

	WhosDoingWhat(SYSOPCHAT);

	clear();

	if((pGetDev = fopen(DEVICE,"r")) == NULL)
			WriteError("Can't open file:", DEVICE);
	else {
		fscanf(pGetDev, "%19s", sDevice);
		fclose(pGetDev);
	}

	if(( pChat = fopen(sDevice,"w")) == NULL)
		perror("Error");

	/*
	 * Check to see if it must load a external chat program
	 * Check the length of the chat program is greater than Zero
	 * Check if user has execute permisson on the chat program
	 */
	if(!CFG.iExternalChat || (strlen(CFG.sExternalChat) < 1) || \
		(access(CFG.sExternalChat, X_OK) != 0)) {
		if ((pBusy = fopen("/tmp/.BusyChatting", "w")) == NULL)
			WriteError("Unable to open BusyChatting file", "/tmp/.BusyChatting");
		else {
			fprintf(pBusy, "%s", pTTY);
			fclose(pBusy);
		}

		sprintf(temp, "/tmp/.chat.%s", pTTY);
		if(( pBusy = fopen(temp, "w")) == NULL)
			WriteError("Unable to open chat.tty file", temp);
		else
			fclose(pBusy);

		colour(10, 0);
		printf("%s\n\r", (char *) Language(59));
		fflush(stdout);
		Setraw();

		sleep(2);

		Syslog('+', "Sysop chat started");

		fprintf(pChat, "%s is ready ...  \n\r",exitinfo.sUserName);

		while (1) {
			ch = getc(stdin);
			ch &= '\377';
			/* The next statement doesn't work, the chat will start again */
			if (ch == '\007') {
				Syslog('+', "Sysop chat ended - User exited chat");
				unlink("/tmp/chatdev");
				sprintf(temp, "/tmp/.chat.%s", pTTY);
				unlink(temp);
				unlink("/tmp/.BusyChatting");
				Unsetraw();
				break;
			}
			putchar(ch);
			putc(ch, pChat);

			if(CFG.iAutoLog) {
				if(ch != '\b')
					iLetter++; /* Count the letters user presses for logging */
				sprintf(sLog, "%s%c", sLog, ch);
			}

			if (ch == '\n') {
				ch = '\r';
				putchar(ch);
				putc(ch, pChat);
			}

			if (ch == '\r') {
				ch = '\n';
				putchar(ch);
				putc(ch, pChat);
			}

			if (ch == '\b') {
				ch = ' ';
				putchar(ch);
				putc(ch, pChat);
				ch = '\b';
				putchar(ch);
				putc(ch, pChat);

				if(CFG.iAutoLog)
					sLog[--iLetter] = '\0'; 
			}

			/* Check if log chat is on and if so log chat to disk */
			if(CFG.iAutoLog) {
				if(iLetter >= 55 || ch == '\n') {
					iLetter = 0;
					sprintf(temp, "%s/etc/%s", getenv("MBSE_ROOT"), CFG.chat_log);
					if(( pLog = fopen(temp, "a+")) != NULL) {
						fflush(pLog);
						fprintf(pLog, "%s [%s]: %s\n", exitinfo.sUserName, (char *) GetLocalHM(), sLog);
						fclose(pLog);
						strcpy(sLog, "");
					}
				}
			}

			if(access("/tmp/chatdev", R_OK) != 0) {
				colour(10, 0);
				printf("\n\n\n%s\n\n\r", (char *) Language(60));
				Syslog('+', "Sysop chat ended");
				sprintf(temp, "/tmp/.chat.%s", pTTY);
				unlink(temp);
				unlink("/tmp/.BusyChatting");
				Unsetraw();
				Pause();
				break;
			}
		}
	/* End of ExternalChat Check */
	} else {
		system(CFG.sExternalChat);
		printf("\n\n");
		Pause();
	}

	if(CFG.iAutoLog)
		free(sLog);

	fclose(pChat);
}


