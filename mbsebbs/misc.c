/*****************************************************************************
 *
 * File ..................: bbs/misc.c
 * Purpose ...............: Misc functions
 * Last modification date : 28-Jun-2001
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
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "funcs.h"
#include "funcs4.h"
#include "language.h"
#include "misc.h"
#include "timeout.h"
#include "exitinfo.h"


extern pid_t	mypid;		/* Pid of this program			   */


int MoreFile(char *filename)
{
	char	Buf[80];
	static	FILE *fptr;
	int	lines;
	int	input;
	int	ignore = FALSE;
	int	maxlines;

	maxlines = lines = exitinfo.iScreenLen - 2;

	if ((fptr =  fopen(filename,"r")) == NULL) {
		printf("%s%s\n", (char *) Language(72), filename);
		return(0);
	}

	printf("\n");

	while (fgets(Buf,80,fptr) != NULL) {
		if ( (lines != 0) || (ignore) ) {
			lines--;
			printf("%s",Buf);
		}

		if (strlen(Buf) == 0) {
			fclose(fptr);
			return(0);
		}
		if (lines == 0) {
			fflush(stdin);
			/* More (Y/n/=) */
			printf(" %sY\x08", (char *) Language(61));
			fflush(stdout);
			alarm_on();
			input = toupper(getchar());

			if ((input == Keystroke(61, 0)) || (input == '\r'))
				lines = maxlines;

			if (input == Keystroke(61, 1)) {
				fclose(fptr);
				return(0);
			}

			if (input == Keystroke(61, 2))
				ignore = TRUE;
			else
				lines  = maxlines;
		}
		printf("\n\n");
		Pause();
	}
	fclose(fptr);
	return 1;
}



int GetLastUser()
{
	FILE	*pCallerLog;
	char	*sDataFile;

	sDataFile = calloc(PATH_MAX, sizeof(char));
	sprintf(sDataFile, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));

	if((pCallerLog = fopen(sDataFile, "r+")) == NULL)
		WriteError("GetLastUser: Can't open file: %s", sDataFile);
	else {
		fread(&SYSINFO, sizeof(SYSINFO), 1, pCallerLog);

		/* Get lastcaller in memory */
		strcpy(LastCaller, SYSINFO.LastCaller);

		/* Set next lastcaller (this user) */
		if(!usrconfig.Hidden)
			strcpy(SYSINFO.LastCaller,exitinfo.sUserName);

		SYSINFO.SystemCalls++;
		switch(ttyinfo.type) {
			case POTS:
				SYSINFO.Pots++;
				break;

			case ISDN:
				SYSINFO.ISDN++;
				break;

			case NETWORK:
				SYSINFO.Network++;
				break;

			case LOCAL:
				SYSINFO.Local++;
				break;
		}

		rewind(pCallerLog);
		fwrite(&SYSINFO, sizeof(SYSINFO), 1, pCallerLog);

		fclose(pCallerLog);
	}
	free(sDataFile);
	return 1;
}



int ChkFiles()
{
	FILE	*pCallerLog, *pUsersFile;
	char	*sDataFile;
	time_t	Now;
	char	*temp;

	sDataFile = calloc(PATH_MAX, sizeof(char));
	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(sDataFile, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));

	/*
	 * Check if users.data exists, if not create a new one.
	 */
	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if((pUsersFile = fopen(temp,"rb")) == NULL) {
		if((pUsersFile = fopen(temp,"wb")) == NULL) {
			WriteError("$Can't create %s", temp);
			ExitClient(1); 
		} else {
			usrconfighdr.hdrsize = sizeof(usrconfighdr);
			usrconfighdr.recsize = sizeof(usrconfig);
			fwrite(&usrconfighdr, sizeof(usrconfighdr), 1, pUsersFile);
			fclose(pUsersFile);
		}
	}

	/*
	 * Check if sysinfo.data exists, if not, create a new one.
	 */
	if((pCallerLog = fopen(sDataFile, "rb")) == NULL) {
		if((pCallerLog = fopen(sDataFile, "wb")) == NULL)
			WriteError("$ChkFiles: Can't create %s", sDataFile);
		else {
			memset((char *)&SYSINFO, 0, sizeof(SYSINFO));
			time(&Now);
			SYSINFO.StartDate = Now;

			rewind(pCallerLog);
			fwrite(&SYSINFO, sizeof(SYSINFO), 1, pCallerLog);
			fclose(pCallerLog);
		}
	}
	free(temp);
	free(sDataFile);
	return 1;
}



void DisplayLogo()
{
	FILE	*pLogo;
	char	*sString, *temp;

	temp = calloc(PATH_MAX, sizeof(char));
	sString = calloc(81, sizeof(char));

	sprintf(temp, "%s/%s", CFG.bbs_txtfiles, CFG.welcome_logo);
	if((pLogo = fopen(temp,"rb")) == NULL)
		WriteError("$DisplayLogo: Can't open %s", temp);
	else {
		while( fgets(sString,80,pLogo) != NULL)
			printf("%s", sString);
		fclose(pLogo);
	}
	free(sString);
	free(temp);
}



/*
 * Update a variable in the exitinfo file.
 */
void Setup(char *Option, char *variable)
{
	ReadExitinfo();
	strcpy(Option, variable);
	WriteExitinfo();
}



void GetLastCallers()
{
	FILE	*pGLC;
	char	*sFileName;
	char	sFileDate[9];
	char	sDate[9];
	struct	stat statfile;

	/*
	 * First check if we passed midnight, in that case we
	 * create a fresh file.
	 */
	sFileName = calloc(PATH_MAX, sizeof(char));
	sprintf(sFileName,"%s/etc/lastcall.data", getenv("MBSE_ROOT"));
	stat(sFileName, &statfile);

	sprintf(sFileDate,"%s", StrDateDMY(statfile.st_mtime));
	sprintf(sDate,"%s", (char *) GetDateDMY());

	if ((strcmp(sDate,sFileDate)) != 0) {
		unlink(sFileName);
		Syslog('+', "Erased old lastcall.data");
	}

	/*
	 * Check if file exists, if not create the file and
	 * write the fileheader.
	 */
	if ((pGLC = fopen(sFileName, "r")) == NULL) {
		if ((pGLC = fopen(sFileName, "w")) != NULL) {
			LCALLhdr.hdrsize = sizeof(LCALLhdr);
			LCALLhdr.recsize = sizeof(LCALL);
			fwrite(&LCALLhdr, sizeof(LCALLhdr), 1, pGLC);
			Syslog('+', "Created new lastcall.data");
		}
		fclose(pGLC);
	}

	if(( pGLC = fopen(sFileName,"a+")) == NULL) {
		WriteError("$Can't open %s", sFileName);
		return;
	} else {
		ReadExitinfo();
		memset(&LCALL, 0, sizeof(LCALL));
		sprintf(LCALL.UserName,"%s", exitinfo.sUserName);
		sprintf(LCALL.Handle,"%s", exitinfo.sHandle);
		sprintf(LCALL.TimeOn,"%s", (char *) GetLocalHM());
		sprintf(LCALL.Device,"%s", pTTY);
		LCALL.SecLevel = exitinfo.Security.level;
		LCALL.Calls  = exitinfo.iTotalCalls;
		sprintf(LCALL.Speed, "%s", ttyinfo.speed);

		/* If true then set hidden so it doesn't display in lastcallers function */
		LCALL.Hidden = exitinfo.Hidden;

		sprintf(LCALL.Location,"%s", exitinfo.sLocation);

		rewind(pGLC); /* ???????????? */
		fwrite(&LCALL, sizeof(LCALL), 1, pGLC);
		fclose(pGLC);
 	}
	free(sFileName);
}



/* Gets Date for GetLastCallers(), returns DD:Mmm */
char *GLCdate()
{
	static char	GLcdate[15];

	time(&Time_Now);
	l_date = localtime(&Time_Now);
	sprintf(GLcdate,"%02d-", l_date->tm_mday);

	strcat(GLcdate,GetMonth(l_date->tm_mon+1));
	return(GLcdate);
}



/*
 * Display last callers screen.
 */
void LastCallers(char *OpData)
{
	FILE	*pLC;
	int	LineCount = 5;
	int	count = 0;
	char	*sFileName;
	char	*Heading;
	char	*Underline;
	int	i, x;
	struct	lastcallers lcall;
	struct	lastcallershdr lcallhdr;

	sFileName = calloc(PATH_MAX, sizeof(char));
	Heading   = calloc(81, sizeof(char));
	Underline = calloc(81, sizeof(char));

	clear();

	sprintf(sFileName,"%s/etc/lastcall.data", getenv("MBSE_ROOT"));
	if((pLC = fopen(sFileName,"r")) == NULL) 
		WriteError("$LastCallers: Can't open %s", sFileName);
	else {
		fread(&lcallhdr, sizeof(lcallhdr), 1, pLC);
		colour(15, 0);
		/* Todays callers to */
		sprintf(Heading, "%s%s", (char *) Language(84), CFG.bbs_name);
		Center(Heading);

		x = strlen(Heading);

		for(i = 0; i < x; i++)
       			sprintf(Underline, "%s%c", Underline, exitinfo.GraphMode ? 196 : 45);

		colour(12, 0);
		Center(Underline);

		printf("\n");

		/* #  User Name               Device  timeOn  Calls Location */
		pout(10, 0, (char *) Language(85));
		Enter(1);

		colour(2, 0);
		fLine(79);
		
		while (fread(&lcall, lcallhdr.recsize, 1, pLC) == 1) {
			if(!lcall.Hidden) {
				count++;

				colour(15,0);
				printf("%-5d", count);

				colour(11, 0);
				if((strcmp(OpData, "/H")) == 0) {
					if((strcmp(lcall.Handle, "") != 0 && *(lcall.Handle) != ' '))
						printf("%-20s", lcall.Handle);
					else
						printf("%-20s", lcall.UserName);
				} else
					printf("%-20s", lcall.UserName);

				colour(9, 0);
				printf("%-8s", lcall.Device);

				colour(13, 0);
				printf("%-8s", lcall.TimeOn);

				colour(14, 0);
				printf("%-7d", lcall.Calls);

				colour(12, 0);
				printf("%-32s\n", lcall.Location);

				LineCount++;
				if (LineCount == exitinfo.iScreenLen) {
					Pause();
					LineCount = 0;
				}
			} /* End of check if user is sysop */
		}

		colour(2, 0);
		fLine(79);

		fclose(pLC);
		printf("\n");
		Pause();
	}
	free(sFileName);
	free(Heading);
	free(Underline);
}



/*
 * Check for a personal message, this will go via mbsed. If there
 * is a message, it will be displayed, else nothing happens.
 */
void Check_PM(void)
{
	static char	buf[128];
	char		resp[128];

	sprintf(buf, "CIPM:1,%d;", mypid);
	if (socket_send(buf) == 0) {
		strcpy(buf, socket_receive());
		if (strncmp(buf, "100:0;", 6) == 0)
			return;

		strcpy(resp, strtok(buf, ":"));
		strcpy(resp, strtok(NULL, ","));
		colour(CYAN, BLACK);
		/* ** Message ** from */
		printf("\n\n\007%s %s:\n", (char *)Language(434), strtok(NULL, ","));
		printf("%s\n", strtok(NULL, ";"));
		Pause();
	}
}



