/*****************************************************************************
 *
 * Purpose ...............: Who's online functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
    char	    buf[128], *Heading, *isdoing, *location, *device;
    char	    *fullname, *temp, msg[81], wstr[128];;
    int		    x, Start = TRUE;
    FILE	    *fp;
    struct userhdr  ushdr;
    struct userrec  us;

    Heading   = calloc(81, sizeof(char));

    WhosDoingWhat(WHOSON, NULL);

    if (utf8)
	chartran_init((char *)"CP437", (char *)"UTF-8", 'B');

    strcpy(wstr, clear_str());
    strncat(wstr, (char *)"\r\n", 127);
    strncat(wstr, colour_str(WHITE, BLACK), 127);
    /* Callers On-Line to */
    snprintf(Heading, 81, "%s%s", (char *) Language(414), CFG.bbs_name);
    strncat(wstr, Center_str(Heading), 127);
    PUTSTR(chartran(wstr));

    x = strlen(Heading);
    strcpy(wstr, colour_str(LIGHTRED, BLACK));
    strncat(wstr, Center_str(hLine_str(x)), 127);
    PUTSTR(chartran(wstr));

    /* Name                          Device   Status         Location */
    strcpy(wstr, pout_str(LIGHTGREEN, BLACK, (char *) Language(415)));
    strncat(wstr, (char *)"\r\n", 127);
    PUTSTR(chartran(wstr));

    strcpy(wstr, colour_str(GREEN, BLACK));
    strncat(wstr, fLine_str(79), 127);
    PUTSTR(chartran(wstr));

    while (TRUE) {
	if (Start)
		snprintf(buf, 128, "GMON:1,1;");
	else
		snprintf(buf, 128, "GMON:1,0;");
	Start = FALSE;
	if (socket_send(buf) == 0) {
	strcpy(buf, socket_receive());
	    if (strncmp(buf, "100:0;", 6) == 0)
		break;  /* No more data */
	    if (strstr(buf, "mbsebbs")) {
		/*
		 * We are only interested in copies of the mbsebbs program
		 */
		strtok(buf, ",");
		strtok(NULL, ",");
		device   = xstrcpy(strtok(NULL, ","));
		fullname = xstrcpy(cldecode(strtok(NULL, ",")));

		if (((strcasecmp(OpData, "/H")) == 0) || (strlen(OpData) == 0)) {
		    /*
		     * The mbtask daemon has only the users Unix names, we
		     * want the handle or real name instead.
		     */
		    temp = calloc(PATH_MAX, sizeof(char));
		    snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
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
		snprintf(msg, 81, "%-30s", fullname);
		pout(LIGHTCYAN, BLACK, msg);
		free(fullname);

		snprintf(msg, 81, "%-9s", device);
		pout(LIGHTBLUE, BLACK, msg);
		free(device);

		strtok(NULL, ",");
		location = xstrcpy(cldecode(strtok(NULL, ",")));
		isdoing  = xstrcpy(cldecode(strtok(NULL, ",")));

		if (strstr(isdoing, "Browsing"))
		    /* Browseng */
		    snprintf(msg, 81, "%-15s", (char *) Language(418));
		else if (strstr(isdoing, "Downloading"))
		    /* Downloading */
		    snprintf(msg, 81, "%-15s", (char *) Language(419));
		else if (strstr(isdoing, "Uploading"))
		    /* Uploading */
		    snprintf(msg, 81, "%-15s", (char *) Language(420));
		else if (strstr(isdoing, "Read"))
		    /* Msg Section */
		    snprintf(msg, 81, "%-15s", (char *) Language(421));
		else if (strstr(isdoing, "External"))
		    /* External Door */
		    snprintf(msg, 81, "%-15s", (char *) Language(422));
		else if (strstr(isdoing, "Chat"))
		    /* Chatting */
		    snprintf(msg, 81, "%-15s", (char *) Language(423));
		else if (strstr(isdoing, "Files"))
		    /* Listing Files */
		    snprintf(msg, 81, "%-15s", (char *) Language(424));
		else if (strstr(isdoing, "Time"))
		    /* Banking Door */
		    snprintf(msg, 81, "%-15s", (char *) Language(426));
		else if (strstr(isdoing, "Safe"))
		    /* Safe Door */
		    snprintf(msg, 81, "%-15s", (char *) Language(427));
		else if (strstr(isdoing, "Whoson"))
		    /* WhosOn List */
		    snprintf(msg, 81, "%-15s", (char *) Language(428));
		else if (strstr(isdoing, "Offline"))
		    /* Offline Reader */
		    snprintf(msg, 81, "%-15s", (char *) Language(429));
		else {
		    /* 
		     * This is default when nothing matches, with doors this
		     * will show the name of the door.
		     */
		    if (strlen(isdoing) > 15)
			isdoing[15] = '\0';
		    snprintf(msg, 81, "%-15s", isdoing);
		}
		pout(WHITE, BLACK, msg);

		snprintf(msg, 81, "%-25s", location);
		pout(LIGHTRED, BLACK, msg);
		Enter(1);
		free(location);
		free(isdoing);
	    }
	}
    }

    strcpy(wstr, colour_str(GREEN, BLACK));
    strncat(wstr, fLine_str(79), 127);
    PUTSTR(chartran(wstr));

    chartran_close();
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
    char	    *User, *String, *temp, *from, *too, *msg;
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
	snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "rb")) != NULL) {
	    fread(&ushdr, sizeof(ushdr), 1, fp);
	    Syslog('-', "Using translate");

	    while (fread(&us, ushdr.recsize, 1, fp) == 1) {
		if ((strcasecmp(OpData, "/H") == 0) && strlen(us.sHandle) && (strcasecmp(User, us.sHandle) == 0)) {
		    snprintf(User, 36, "%s", us.Name);
		    break;
		} else if ((strlen(OpData) == 0) && (strcasecmp(User, us.sUserName) == 0)) {
		    snprintf(User, 36, "%s", us.Name);
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
	    from = xstrcpy(clencode(exitinfo.sHandle));
	else if (strcasecmp(OpData, "/U") == 0)
	    from = xstrcpy(clencode(exitinfo.Name));
	else
	    from = xstrcpy(clencode(exitinfo.sUserName));
	too = xstrcpy(clencode(User));
	msg = xstrcpy(clencode(String));
	snprintf(buf, 128, "CSPM:3,%s,%s,%s;", from, too, msg);
	free(from);
	free(too);
	free(msg);

	if (socket_send(buf) == 0) {
	    strcpy(buf, socket_receive());

	    if (strncmp(buf, "100:1,3;", 8) == 0) {
		Enter(1);
		/* Sorry, there is no user on */
		snprintf(temp, PATH_MAX, "%s %s", (char *) Language(431), User);
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
		snprintf(temp, PATH_MAX, "%s %s", User, (char *) Language(432));
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


