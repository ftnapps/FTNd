/*****************************************************************************
 *
 * File ..................: mbsebbs/mbchat.c
 * Purpose ...............: Sysop chat utility.
 * Last modification date : 27-Nov-2000
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
 *   
 * Michiel Broek		FIDO:	2:2801/2802
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
#include "../lib/structs.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"


char *ttime2(void);            /* Returns current time  HH:MM     */

struct	sysconfig CFG;


int main(int argc, char **argv)
{
	FILE	*fp1, *pChatDev, *pPid, *pLog;
	FILE	*pDataFile;
	int	ch;
	int	iLetter = 0;
	short	ipid;
	char	*tty;
	char	*sStr1= (char *)"";
	char	pid[81];
	char	pid1[20];
	char	sTTY[10];
	char	*sLog= (char *)"";
	char	*Config, *FileName, *LogName;
	char	*BBSpath;

#ifdef MEMWATCH
        mwInit();
#endif

	FileName = calloc(PATH_MAX, sizeof(char));
	Config   = calloc(PATH_MAX, sizeof(char));
	LogName  = calloc(PATH_MAX, sizeof(char));

	if ((BBSpath = getenv("MBSE_ROOT")) == NULL) {
		printf("Could not get MBSE_ROOT environment variable\n");
		printf("Please set the environment variable ie:\n\n");
		printf("\"MBSE_ROOT=/usr/local/mbse;export MBSE_ROOT\"\n\n");
#ifdef MEMWATCH
		mwTerm();
#endif
		exit(1);
	}

 	sprintf(FileName, "%s/etc/config.data", BBSpath);

	if ((pDataFile = fopen(FileName, "rb")) == NULL) {
		perror("\n\nFATAL ERROR: Can't open config.data for reading!!!");
		printf("Please run mbsetup to create configuration file.\n");
		printf("Or check that your MBSE_ROOT variable is set to the BBS Path!\n");
#ifdef MEMWATCH
		mwTerm();
#endif
		exit(1);
	}
	fread(&CFG, sizeof(CFG), 1, pDataFile);
	fclose(pDataFile);
	free(Config);
	free(FileName);

	if(CFG.iAutoLog)
		sLog = calloc(56, sizeof(char));

	if(argc != 2) {
		printf("\nSCHAT: MBSE BBS %s Sysop chat facilty\n", VERSION);
		printf("       %s\n", Copyright);

		printf("\nCommand-line parameters:\n\n");

		printf("	%s <device>", *(argv));

		printf("\n");
#ifdef MEMWATCH
		mwTerm();
#endif
		exit(0);
	}

 	printf("\f");

	if (strncmp( (tty = *(argv+1)), "/dev/", 5 ) == 0 ) {
		tty+=5;
		sprintf(pid,"%s/tmp/.bbs-exitinfo.%s",BBSpath,tty);
		strcpy(sTTY,"");
	} else {
		sprintf(pid,"%s/tmp/.bbs-exitinfo.%s",BBSpath,*(argv+1));
		strcpy(sTTY,"/dev/");
	}

	strcat(sTTY, *(argv + 1));

	if(( fp1 = fopen(sTTY,"w")) == NULL)
		perror("Error");

	if(( pPid = fopen(pid,"r")) == NULL) {
		printf("\nThere is no user on %s\n", pid);
#ifdef MEMWATCH
		mwTerm();
#endif
		exit(1);
	} else {
		fgets(pid1,19,pPid);
		fclose(pPid);
	}

	ipid = atoi(pid1);

	if(( pChatDev = fopen("/tmp/chatdev","w")) == NULL)
		perror("Can't open file");
	else {
		sStr1=ttyname(1);
		fprintf(pChatDev,"%s", sStr1);
		fclose(pChatDev);
	}

	if(!CFG.iExternalChat || (strlen(CFG.sExternalChat) < 1) || \
		(access(CFG.sExternalChat, X_OK) != 0)) {
		printf("Users chatting device: %s\n", sTTY);
		printf("Wait until the user is ready");
		printf("Press ESC to exit chat\n\n");

		umask(00000);
		chmod("/tmp/chatdev", 00777);
		chmod(sStr1, 00777);

		sleep(2);

		Setraw();

		sleep(2); 

		while (1) {
			ch = getc(stdin);
			ch &= '\377';
			if (ch == '\033')
				break;
			putchar(ch);
			putc(ch, fp1);

			if(CFG.iAutoLog) {
				if(ch != '\b')
					iLetter++; /* Count the letters user presses for logging */
				sprintf(sLog, "%s%c", sLog, ch);
			}

			if (ch == '\n') {
				ch = '\r';
				putchar(ch);
				putc(ch, fp1);
			}

			if (ch == '\r') {
				ch = '\n';
				putchar(ch);
				putc(ch, fp1);
			}

			if (ch == '\b') {
				ch = ' ';
				putchar(ch);
				putc(ch, fp1);
				ch = '\b';
				putchar(ch);
				putc(ch, fp1);

				if(CFG.iAutoLog)
					sLog[--iLetter] = '\0';
			}

			/* Check if log chat is on and if so log chat to disk */
			if(CFG.iAutoLog) {
				if(iLetter >= 55 || ch == '\n') {
					iLetter = 0;
					sprintf(LogName, "%s/log/chat.log", BBSpath);
					if(( pLog = fopen(LogName, "a+")) != NULL) {
						fflush(pLog);
						fprintf(pLog, "%s [%s]: %s\n", CFG.sysop_name, ttime2(), sLog);
						fclose(pLog);
						strcpy(sLog, "");
					} else
						perror("\nCan't open chat.log");
				}
			}
		} /* while chatting */
//		fprintf(fp1, "The sysop ended the chat, press a key.\n");
	} else {
		system(CFG.sExternalChat);
		printf("\n\n");
	}

	fclose(fp1);
	sleep(2);
	Unsetraw();
	sleep(2);
	unlink("/tmp/chatdev");
	unlink("/tmp/.BusyChatting");
	fclose(fp1);
	printf("Done chatting.\n");
#ifdef MEMWATCH
	mwTerm();
#endif
	exit(0);
}



/*
 * This function returns the date for today, to test against other functions
 *                  HH:MM (HOUR-MINUTE)
 */
char *ttime2()
{
	struct	tm *l_date;	/* Structure for Date */
	time_t	Time_Now;
	static	char Ttime2[9];

	time(&Time_Now);
	l_date = localtime(&Time_Now);

 	sprintf(Ttime2, "%02d:%02d", l_date->tm_hour,l_date->tm_min);

	return(Ttime2);
}


