/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Who's online functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "input.h"
#include "language.h"
#include "exitinfo.h"
#include "whoson.h"
#include "term.h"
#include "ttyio.h"


extern int  LC_Download, LC_Upload, LC_Read, LC_Chat, LC_Olr, LC_Door;



/*
 * Function to display what users are currently On-Line and what they
 * are busy doing
 */
void WhosOn(char *OpData)
{
    char	    buf[128], *Heading, *Underline, *cnt, *isdoing, *location, *device;
    char	    *fullname, *temp, msg[81];
    int		    i, x, Start = TRUE;
    FILE	    *fp;
    struct userhdr  ushdr;
    struct userrec  us;

    Underline = calloc(81, sizeof(char));
    Heading   = calloc(81, sizeof(char));

    WhosDoingWhat(WHOSON, NULL);

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
    Enter(1);

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

		if (((strcasecmp(OpData, "/H")) == 0) || (strlen(OpData) == 0)) {
		    /*
		     * The mbtask daemon has only the users Unix names, we
		     * want the handle or real name instead.
		     */
		    temp = calloc(PATH_MAX, sizeof(char));
		    sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
		    if ((fp = fopen(temp,"rb")) != NULL) {
			fread(&ushdr, sizeof(ushdr), 1, fp);

			while (fread(&us, ushdr.recsize, 1, fp) == 1) {
			    if (strcmp(fullname, us.Name) == 0) {
				if ((strcasecmp(OpData, "/H") == 0) && strlen(us.sHandle)) {
				    free(fullname);
				    fullname = xstrcpy(us.sHandle);
				} else if (strlen(OpData) == 0) {
				    free(fullname);
				    fullname = xstrcpy(us.sUserName);
				}
				break;
			    }
			}
			fclose(fp);
		    }
		    free(temp);
		}
		sprintf(msg, "%-30s", fullname);
		pout(LIGHTCYAN, BLACK, msg);
		free(fullname);

		sprintf(msg, "%-9s", device);
		pout(LIGHTBLUE, BLACK, msg);
		free(device);

		strtok(NULL, ",");
		location = xstrcpy(strtok(NULL, ","));
		isdoing  = xstrcpy(strtok(NULL, ","));

		if (strstr(isdoing, "Browsing"))
		    /* Browseng */
		    sprintf(msg, "%-15s", (char *) Language(418));
		else if (strstr(isdoing, "Downloading"))
		    /* Downloading */
		    sprintf(msg, "%-15s", (char *) Language(419));
		else if (strstr(isdoing, "Uploading"))
		    /* Uploading */
		    sprintf(msg, "%-15s", (char *) Language(420));
		else if (strstr(isdoing, "Read"))
		    /* Msg Section */
		    sprintf(msg, "%-15s", (char *) Language(421));
		else if (strstr(isdoing, "External"))
		    /* External Door */
		    sprintf(msg, "%-15s", (char *) Language(422));
		else if (strstr(isdoing, "Chat"))
		    /* Chatting */
		    sprintf(msg, "%-15s", (char *) Language(423));
		else if (strstr(isdoing, "Files"))
		    /* Listing Files */
		    sprintf(msg, "%-15s", (char *) Language(424));
		else if (strstr(isdoing, "Time"))
		    /* Banking Door */
		    sprintf(msg, "%-15s", (char *) Language(426));
		else if (strstr(isdoing, "Safe"))
		    /* Safe Door */
		    sprintf(msg, "%-15s", (char *) Language(427));
		else if (strstr(isdoing, "Whoson"))
		    /* WhosOn List */
		    sprintf(msg, "%-15s", (char *) Language(428));
		else if (strstr(isdoing, "Offline"))
		    /* Offline Reader */
		    sprintf(msg, "%-15s", (char *) Language(429));
		else {
		    /* 
		     * This is default when nothing matches, with doors this
		     * will show the name of the door.
		     */
		    if (strlen(isdoing) > 15)
			isdoing[15] = '\0';
		    sprintf(msg, "%-15s", isdoing);
		}
		pout(WHITE, BLACK, msg);

		sprintf(msg, "%-25s", location);
		pout(LIGHTRED, BLACK, msg);
		Enter(1);
		free(location);
		free(isdoing);
	    }
	}
    }

    colour(GREEN, BLACK);
    fLine(79);

    free(Underline);
    free(Heading);
    Enter(1);
}



/*
 * Function will update users file and and update exitinfo.iStatus
 */
void WhosDoingWhat(int iStatus, char *what)
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

	case DOOR:	if (what)
			    strcpy(temp, what);
			else
			    strcpy(temp, "External Door");
			LC_Door = TRUE;
			break;	

	case SYSOPCHAT: strcpy(temp, "Sysop Chat");
			LC_Chat = TRUE;
			break;

	case FILELIST:	strcpy(temp, "List Files");
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
    GetstrC(User, 35);
    if (!strcmp(User, "")) {
	free(User);
	free(String);
	return;
    }
    temp = calloc(PATH_MAX, sizeof(char));

    /*
     * If we were displaying handles or real names, then lookup the 
     * users unix name to send to mbtask.
     */
    if ((strcasecmp(OpData, "/H") == 0) || (strlen(OpData) == 0)) {
	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "rb")) != NULL) {
	    fread(&ushdr, sizeof(ushdr), 1, fp);
	    Syslog('-', "Using translate");

	    while (fread(&us, ushdr.recsize, 1, fp) == 1) {
		if ((strcasecmp(OpData, "/H") == 0) && strlen(us.sHandle) && (strcasecmp(User, us.sHandle) == 0)) {
		    sprintf(User, "%s", us.Name);
		    break;
		} else if ((strlen(OpData) == 0) && (strcasecmp(User, us.sUserName) == 0)) {
		    sprintf(User, "%s", us.Name);
		    break;
		}
	    }
	    fclose(fp);
	}
    }

    /* Please enter message to send (Max 76 Characters) */
    pout(LIGHTGREEN, BLACK, (char *)Language(433));
    Enter(1);
    pout(LIGHTGREEN, BLACK, (char *)"> ");
    colour(CFG.InputColourF, CFG.InputColourB);
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
		Enter(1);
		/* Sorry, there is no user on */
		sprintf(temp, "%s %s", (char *) Language(431), User);
		PUTSTR(temp);
		Enter(1);
	    }
	    if (strncmp(buf, "100:1,2;", 8) == 0) {
		Enter(1);
		PUTSTR((char *)"No more room in users message buffer");
		Enter(2);
	    }
	    if (strncmp(buf, "100:1,1;", 8) == 0) {
		Enter(1);
		/* doesn't wish to be disturbed */
		sprintf(temp, "%s %s", User, (char *) Language(432));
		pout(LIGHTRED, BLACK, temp);
		Enter(1);
	    }
	    if (strncmp(buf, "100:0;", 6) == 0) {
		PUTSTR((char *)"Message Sent!");
		Enter(1);
		Syslog('+', "Online msg to %s: \"%s\"", User, String);
	    }
	}
    }

    free(temp);
    free(User);
    free(String);
    Pause();
}


