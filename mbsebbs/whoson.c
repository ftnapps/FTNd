/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Who's online functions
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
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "input.h"
#include "language.h"
#include "exitinfo.h"
#include "whoson.h"


extern int  LC_Download, LC_Upload, LC_Read, LC_Chat, LC_Olr, LC_Door;



/*
 * Function to display what users are currently On-Line and what they
 * are busy doing
 */
void WhosOn(char *OpData)
{
    char	    buf[128], *Heading, *Underline, *cnt, *isdoing, *location, *device;
    char	    *fullname, *temp;
    int		    i, x, Start = TRUE;
    FILE	    *fp;
    struct userhdr  ushdr;
    struct userrec  us;

    Underline = calloc(81, sizeof(char));
    Heading   = calloc(81, sizeof(char));

    WhosDoingWhat(WHOSON);

    clear();
    Enter(1);
    colour(WHITE, BLACK);
    /* Callers On-Line to */
    sprintf(Heading, "%s%s", (char *) Language(414), CFG.bbs_name);
    Center(Heading);
    x = strlen(Heading);

    for(i = 0; i < x; i++)
	sprintf(Underline, "%s%c", Underline, exitinfo.GraphMode ? 196 : 45);
    colour(LIGHTRED, BLACK);
    Center(Underline);
    printf("\n");

    /* Name                          Device   Status         Location */
    pout(LIGHTGREEN, BLACK, (char *) Language(415));
    Enter(1);
    colour(GREEN, BLACK);
    fLine(79);

    while (TRUE) {
	if (Start)
		sprintf(buf, "GMON:1,1;");
	else
		sprintf(buf, "GMON:1,0;");
	Start = FALSE;
	if (socket_send(buf) == 0) {
	strcpy(buf, socket_receive());
	    if (strncmp(buf, "100:0;", 6) == 0)
		break;  /* No more data */
	    if (strstr(buf, "mbsebbs")) {
		/*
		 * We are only interested in copies of the mbsebbs program
		 */
		cnt = strtok(buf, ",");
		strtok(NULL, ",");
		device   = xstrcpy(strtok(NULL, ","));
		fullname = xstrcpy(strtok(NULL, ","));

		if (((strcasecmp(OpData, "/H")) == 0) || (strcasecmp(OpData, "/U") == 0)) {
		    /*
		     * The mbtask daemon has only the users real names, we
		     * want the handle or unix name instead.
		     */
		    temp = calloc(PATH_MAX, sizeof(char));
		    sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
		    if ((fp = fopen(temp,"rb")) != NULL) {
			fread(&ushdr, sizeof(ushdr), 1, fp);

			while (fread(&us, ushdr.recsize, 1, fp) == 1) {
			    if (strcmp(fullname, us.sUserName) == 0) {
				if ((strcasecmp(OpData, "/H") == 0) && strlen(us.sHandle)) {
				    free(fullname);
				    fullname = xstrcpy(us.sHandle);
				} else if (strcasecmp(OpData, "/U") == 0) {
				    free(fullname);
				    fullname = xstrcpy(us.Name);
				}
				break;
			    }
			}
			fclose(fp);
		    }
		    free(temp);
		}
		colour(LIGHTCYAN, BLACK);
		printf("%-30s", fullname);
		free(fullname);

		colour(LIGHTBLUE, BLACK);
		printf("%-9s", device);
		free(device);
		strtok(NULL, ",");
		location = xstrcpy(strtok(NULL, ","));
		isdoing  = xstrcpy(strtok(NULL, ","));

		colour(WHITE, BLACK);
		if (strstr(isdoing, "Browsing"))
		    /* Browseng */
		    printf("%-15s", (char *) Language(418));
		else if (strstr(isdoing, "Downloading"))
		    /* Downloading */
		    printf("%-15s", (char *) Language(419));
		else if (strstr(isdoing, "Uploading"))
		    /* Uploading */
		    printf("%-15s", (char *) Language(420));
		else if (strstr(isdoing, "Read"))
		    /* Msg Section */
		    printf("%-15s", (char *) Language(421));
		else if (strstr(isdoing, "External"))
		    /* External Door */
		    printf("%-15s", (char *) Language(422));
		else if (strstr(isdoing, "Chat"))
		    /* Chatting */
		    printf("%-15s", (char *) Language(423));
		else if (strstr(isdoing, "Files"))
		    /* Listing Files */
		    printf("%-15s", (char *) Language(424));
		else if (strstr(isdoing, "Time"))
		    /* Banking Door */
		    printf("%-15s", (char *) Language(426));
		else if (strstr(isdoing, "Safe"))
		    /* Safe Door */
		    printf("%-15s", (char *) Language(427));
		else if (strstr(isdoing, "Whoson"))
		    /* WhosOn List */
		    printf("%-15s", (char *) Language(428));
		else if (strstr(isdoing, "Offline"))
		    /* Idle */
		    printf("%-15s", (char *) Language(429));
		else
		    printf("System error   ");

		colour(LIGHTRED, BLACK);
		printf("%-25s\n", location);
		free(location);
		free(isdoing);
	    }
	}
    }

    colour(GREEN, BLACK);
    fLine(79);

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
	case BROWSING:	strcpy(temp, "Browsing Menus");
			break;

	case DOWNLOAD:	strcpy(temp, "Downloading");
			LC_Download = TRUE;
			break;

	case UPLOAD:	strcpy(temp, "Uploading");
			LC_Upload = TRUE;
			break;

	case READ_POST: strcpy(temp, "Read/post Messages");
			LC_Read = TRUE;
			break;

	case DOOR:	strcpy(temp, "External Door");
			LC_Door = TRUE;
			break;	

	case SYSOPCHAT: strcpy(temp, "Sysop Chat");
			LC_Chat = TRUE;
			break;

	case FILELIST:	strcpy(temp, "List Files");
			break;

	case TIMEBANK:	strcpy(temp, "Time Bank");
			LC_Door = TRUE;
			break;

	case SAFE:	strcpy(temp, "Safe Cracker");
			LC_Door = TRUE;
			break;

	case WHOSON:	strcpy(temp, "View Whoson List");
			break;

	case OLR:	strcpy(temp, "Offline Reader");
			LC_Olr = TRUE;
			break;
    }
    IsDoing(temp);
	
    free(temp);
}



/*
 * Function will allow a user to send a on-line message to another user
 * It will prompt the user for the username. The message is sent thru
 * mbtask, from the response message we can see if we succeeded.
 * Optional data /H and /U for handles and unix names is supported.
 */
void SendOnlineMsg(char *OpData)
{
    static char	    buf[128];
    char	    *User, *String, *temp;
    FILE            *fp;
    struct userhdr  ushdr;
    struct userrec  us;

    User    = calloc(36, sizeof(char));    
    String  = calloc(77, sizeof(char));      
    WhosOn(OpData);

    /* Please enter username to send message to: */
    pout(CYAN, BLACK, (char *) Language(430));
    colour(CFG.InputColourF, CFG.InputColourB);
    fflush(stdout);
    GetstrC(User, 35);
    if (!strcmp(User, "")) {
	free(User);
	free(String);
	return;
    }

    /*
     * If we were displaying handles or unix names, then lookup the 
     * users fullname to send to mbtask.
     */
    if ((strcasecmp(OpData, "/H") == 0) || (strcasecmp(OpData, "/U") == 0)) {
	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "rb")) != NULL) {
	    fread(&ushdr, sizeof(ushdr), 1, fp);
	    Syslog('-', "Using translate");

	    while (fread(&us, ushdr.recsize, 1, fp) == 1) {
		if ((strcasecmp(OpData, "/H") == 0) && strlen(us.sHandle) && (strcasecmp(User, us.sHandle) == 0)) {
		    sprintf(User, "%s", us.sUserName);
		    break;
		} else if ((strcasecmp(OpData, "/U") == 0) && (strcasecmp(User, us.Name) == 0)) {
		    sprintf(User, "%s", us.sUserName);
		    break;
		}
	    }
	    fclose(fp);
	}
	free(temp);
    }

    /* Please enter message to send (Max 76 Characters) */
    pout(LIGHTGREEN, BLACK, (char *)Language(433));
    pout(LIGHTGREEN, BLACK, (char *)"\n> ");
    colour(CFG.InputColourF, CFG.InputColourB);
    fflush(stdout);
    GetstrC(String, 76);

    if ((strcmp(String, "")) != 0) {
	buf[0] = '\0';
	if ((strcasecmp(OpData, "/H") == 0) && strlen(exitinfo.sHandle))
	    sprintf(buf, "CSPM:3,%s,%s,%s;", exitinfo.sHandle, User, String);
	else if (strcasecmp(OpData, "/U") == 0)
	    sprintf(buf, "CSPM:3,%s,%s,%s;", exitinfo.Name, User, String);
	else
	    sprintf(buf, "CSPM:3,%s,%s,%s;", exitinfo.sUserName, User, String);

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
		colour(LIGHTRED, BLACK);
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


