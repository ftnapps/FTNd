/*****************************************************************************
 *
 * File ..................: bbs/exitinfo.c
 * Purpose ...............: Exitinfo functions
 * Last modification date : 26-Oct-2001
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
#include "funcs.h"
#include "funcs4.h"
#include "language.h"
#include "oneline.h"
#include "misc.h"
#include "bye.h"
#include "timeout.h"
#include "timecheck.h"
#include "exitinfo.h"


extern int  LC_Download, LC_Upload, LC_Read, LC_Chat, LC_Olr, LC_Door;


/*
 * Copy usersrecord into ~/tmp/.bbs-exitinfo.tty
 */
void InitExitinfo()
{
	FILE	*pUsrConfig, *pExitinfo;
	char	*temp;
	long	offset;

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));

	if ((pUsrConfig = fopen(temp,"r+b")) == NULL) {
		WriteError("$Can't open %s for writing", temp);
		free(temp);
		return;
	}

	fread(&usrconfighdr, sizeof(usrconfighdr), 1, pUsrConfig);
	offset = usrconfighdr.hdrsize + (grecno * usrconfighdr.recsize);
	if(fseek(pUsrConfig, offset, 0) != 0) {
		WriteError("$Can't move pointer in %s", temp);
		free(temp);
		Good_Bye(1); 
	}

	fread(&usrconfig, usrconfighdr.recsize, 1, pUsrConfig);

	exitinfo = usrconfig;
	fclose(pUsrConfig);

	sprintf(temp, "%s/tmp/.bbs-exitinfo.%s", getenv("MBSE_ROOT"), pTTY);
	mkdirs(temp);
	if ((pExitinfo = fopen(temp, "w+b")) == NULL)
		WriteError("$Can't open %s for writing", temp);
	else {
		fwrite(&exitinfo, sizeof(exitinfo), 1, pExitinfo);
		fclose(pExitinfo);
	}
	free(temp);
}



/*
 * Function will re-read users file in memory, so the latest information
 * is available to other functions
 */
void ReadExitinfo()
{
	FILE *pExitinfo;
	char *temp;

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/tmp/.bbs-exitinfo.%s", getenv("MBSE_ROOT"), pTTY);
	mkdirs(temp);
	if(( pExitinfo = fopen(temp,"r+b")) == NULL)
		InitExitinfo();
	else {
		fflush(stdin);
		fread(&exitinfo, sizeof(exitinfo), 1, pExitinfo);
		fclose(pExitinfo);
	}
	free(temp);
}



/*
 * Function will rewrite userinfo from memory, so the latest information
 * is available to other functions
 */
void WriteExitinfo()
{
	FILE *pExitinfo;
	char *temp;

	temp = calloc(PATH_MAX, sizeof(char));

	sprintf(temp, "%s/tmp/.bbs-exitinfo.%s", getenv("MBSE_ROOT"), pTTY);
	if(( pExitinfo = fopen(temp,"w+b")) == NULL)
		WriteError("$WriteExitinfo() failed");
	else {
		fwrite(&exitinfo, sizeof(exitinfo), 1, pExitinfo);
		fclose(pExitinfo);
	}
	free(temp);
}



/*
 * Function to display what users are currently On-Line and what they
 * are busy doing
 */
void WhosOn(char *OpData)
{
	FILE	*pExitinfo;
	DIR	*Directory;
	char	*Heading, *Underline, *temp, *tmp, *device;
	struct	dirent *Dir;
	int	i, x;

	Underline = calloc(81, sizeof(char));
	Heading   = calloc(81, sizeof(char));
	temp      = calloc(PATH_MAX, sizeof(char));
	tmp       = calloc(PATH_MAX, sizeof(char));

	WhosDoingWhat(WHOSON);

	clear();

	Enter(1);
	colour(15, 0);
	sprintf(Heading, "%s%s", (char *) Language(414), CFG.bbs_name);
	Center(Heading);
	x = strlen(Heading);

	for(i = 0; i < x; i++)
                sprintf(Underline, "%s%c", Underline, exitinfo.GraphMode ? 196 : 45);

	colour(12, 0);
	Center(Underline);

	printf("\n");
		
	pout(10, 0, (char *) Language(415));
	Enter(1);

	colour(2, 0);
	fLine(79);

	sprintf(tmp, "%s/tmp", getenv("MBSE_ROOT"));
	if ((Directory = opendir(tmp)) != NULL)
       		while ((Dir = readdir( Directory )) != NULL)
			if((strstr(Dir->d_name, ".bbs-exitinfo.")) != NULL) {
				sprintf(temp, "%s/%s", tmp, Dir->d_name);
				if(( pExitinfo = fopen(temp, "rb")) != NULL) {
					fread(&exitinfo, sizeof(exitinfo), 1, pExitinfo);

					colour(11, 0);
					if((strcmp(OpData, "/H")) == 0) {
						if((strcmp(exitinfo.sHandle, "") != 0 && *(exitinfo.sHandle) != ' '))
							printf("%-30s", exitinfo.sHandle);
						else
							printf("%-30s", exitinfo.sUserName);
					} else
						printf("%-30s", exitinfo.sUserName);

					colour(9, 0);
					if((device = strstr(Dir->d_name, "tty")) != NULL)
						printf("%-9s", device);
					else
						printf("%-9s", "None");

					colour(15, 0);

					/* Browseng */
					if(exitinfo.iStatus == BROWSING)
						printf("%-15s", (char *) Language(418));

					/* Downloading */
					else if(exitinfo.iStatus == DOWNLOAD)
						printf("%-15s", (char *) Language(419));

					/* Uploading */
					else if(exitinfo.iStatus == UPLOAD)
						printf("%-15s", (char *) Language(420));

					/* Msg Section */
					else if(exitinfo.iStatus == READ_POST)
						printf("%-15s", (char *) Language(421));

					/* External Door */
					else if(exitinfo.iStatus == DOOR)
						printf("%-15s", (char *) Language(422));

					/* Chatting */
					else if(exitinfo.iStatus == SYSOPCHAT)
						printf("%-15s", (char *) Language(423));

					/* Listing Files */
					else if(exitinfo.iStatus == FILELIST)
						printf("%-15s", (char *) Language(424));

					/* Banking Door */
					else if(exitinfo.iStatus == TIMEBANK)
						printf("%-15s", (char *) Language(426));

					/* Safe Door */
					else if(exitinfo.iStatus == SAFE)
						printf("%-15s", (char *) Language(427));

					/* WhosOn List */
					else if(exitinfo.iStatus == WHOSON)
						printf("%-15s", (char *) Language(428));

					/* Idle */
					else
						printf("%s", (char *) Language(429));

					colour(12, 0);
					printf("%-25s\n", exitinfo.sLocation);

					fclose(pExitinfo);
				}
			}
      	closedir(Directory);

	ReadExitinfo();

	colour(2, 0);
	fLine(79);

	free(tmp);
	free(temp);
	free(Underline);
	free(Heading);

	printf("\n");
}



/*
 * Function will update users file and and update exitinfo.iStatus
 */
void WhosDoingWhat(int iStatus)
{
	char *temp;

	temp = calloc(PATH_MAX, sizeof(char));

	ReadExitinfo();
	exitinfo.iStatus = iStatus;
	WriteExitinfo();

	switch(iStatus) {
		case BROWSING: 
			strcpy(temp, "Browsing Menus");
			break;

		case DOWNLOAD:
			strcpy(temp, "Downloading");
			LC_Download = TRUE;
			break;

		case UPLOAD:
			strcpy(temp, "Uploading");
			LC_Upload = TRUE;
			break;

		case READ_POST:
			strcpy(temp, "Read/post Messages");
			LC_Read = TRUE;
			break;

		case DOOR:
			strcpy(temp, "External Door");
			LC_Door = TRUE;
			break;	

		case SYSOPCHAT:
			strcpy(temp, "Sysop Chat");
			LC_Chat = TRUE;
			break;

		case FILELIST:
			strcpy(temp, "List Files");
			break;

		case TIMEBANK:
			strcpy(temp, "Time Bank");
			LC_Door = TRUE;
			break;

		case SAFE:
			strcpy(temp, "Safe Cracker");
			LC_Door = TRUE;
			break;

		case WHOSON:
			strcpy(temp, "View Whoson List");
			break;

		case OLR:
			strcpy(temp, "Offline Reader");
			LC_Olr = TRUE;
			break;
	}
	IsDoing(temp);
	
	free(temp);
}



/*
 * Function will allow a user to send a on-line message to another user
 * It will prompt the user for the username. The message is sent thru
 * mbsed, from the resonse message we can see if we succeeded.
 */
void SendOnlineMsg(char *OpData)
{
	static char	buf[128];
	char 		*User, *String;

	User    = calloc(36, sizeof(char));    
	String  = calloc(77, sizeof(char));      
	WhosOn(OpData);

	/* Please enter username to send message to: */
	pout(3, 0, (char *) Language(430));
	colour(CFG.InputColourF, CFG.InputColourB);
	fflush(stdout);
	GetstrC(User, 35);
	if (!strcmp(User, "")) {
		free(User);
		free(String);
		return;
	}

	/* Please enter message to send (Max 76 Characters) */
	pout(10, 0, (char *)Language(433));
	pout(10, 0, (char *)"\n> ");
	colour(CFG.InputColourF, CFG.InputColourB);
	fflush(stdout);
	GetstrC(String, 76);

	if ((strcmp(String, "")) != 0) {
		buf[0] = '\0';
		sprintf(buf, "CSPM:3,%s,%s,%s;", strcmp(OpData, "/H") != 0 ? exitinfo.sUserName : \
			strcmp(exitinfo.sHandle, "") == 0 ? exitinfo.sUserName : \
			exitinfo.sHandle, User, String);

		if (socket_send(buf) == 0) {
			strcpy(buf, socket_receive());

			if (strncmp(buf, "100:1,3;", 8) == 0) {
				/* Sorry, there is no user on */
				printf("\n%s %s\n\n", (char *) Language(431), User);
			}
			if (strncmp(buf, "100:1,2;", 8) == 0) {
				printf("\nNo more room in users message buffer\n\n");
			}
			if (strncmp(buf, "100:1,1;", 8) == 0) {
				colour(12, 0);
				/* doesn't wish to be disturbed */
				printf("\n%s %s\n", User, (char *) Language(432));
			}
			if (strncmp(buf, "100:0;", 6) == 0) {
				printf("Message Sent!\n");
				Syslog('+', "Online msg to %s: \"%s\"", User, String);
			}
		}
	}

	free(User);
	free(String);
	Pause();
}


