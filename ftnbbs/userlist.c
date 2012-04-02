/*****************************************************************************
 *
 * $Id: userlist.c,v 1.12 2007/02/25 20:28:13 mbse Exp $
 * Purpose ...............: Display Userlist
 *
 *****************************************************************************
 * Copyright (C) 1997-2007 
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
#include "userlist.h"
#include "language.h"
#include "input.h"
#include "timeout.h"
#include "term.h"
#include "ttyio.h"


extern int  rows;
extern int  cols;


void UserList(char *OpData)
{                                                                        
    FILE	    *pUsrConfig;
    int		    LineCount = 2, iFoundName = FALSE, iNameCount = 0;
    char	    ustr[128], *Name, *sTemp, *User, *temp, msg[81];
    struct userhdr  uhdr;
    struct userrec  u;

    temp  = calloc(PATH_MAX, sizeof(char));
    Name  = calloc(37, sizeof(char));
    sTemp = calloc(81, sizeof(char));
    User  = calloc(81, sizeof(char));

    if (utf8)
	chartran_init((char *)"CP437", (char *)"UTF-8", 'B');

    strcpy(ustr, clear_str());
    /* User List */
    strncat(ustr, poutCR_str(WHITE, BLACK, (char *) Language(126)), 127);
    PUTSTR(chartran(ustr));
    LineCount = 1;

    snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((pUsrConfig = fopen(temp, "rb")) == NULL) {
	WriteError("UserList: Can't open file: %s", temp);
	return;
    }
    fread(&uhdr, sizeof(uhdr), 1, pUsrConfig);

    /* Enter Username search string or (Enter) for all users: */
    language(WHITE, BLACK, 127);
    colour(CFG.InputColourF, CFG.InputColourB);
    alarm_on();
    GetstrC(Name, 35);
    
    strcpy(ustr, clear_str());
    /* Name         Location                   Last On    Calls */
    strncat(ustr, poutCR_str(WHITE, BLACK, (char *) Language(128)), 127);
    PUTSTR(chartran(ustr));

    strcpy(ustr, colour_str(GREEN, BLACK));
    strncat(ustr, fLine_str(cols -1), 127);
    PUTSTR(chartran(ustr));

    strcpy(ustr, colour_str(CYAN, BLACK));
    PUTSTR(chartran(ustr));

    while (fread(&u, uhdr.recsize, 1, pUsrConfig) == 1) {
	if ((strcmp(Name,"")) != 0) {
	    if (((strcasecmp(OpData, "/H")) == 0) && strlen(u.sHandle))
		snprintf(User, 36, "%s", u.sHandle);
	    else if ((strcasecmp(OpData, "/U")) == 0)
		snprintf(User, 36, "%s", u.Name);
	    else
		snprintf(User, 36, "%s", u.sUserName);

	    if ((strstr(tl(User), tl(Name)) != NULL)) {
		if ((!u.Hidden) && (!u.Deleted)) {
		    if ((strcasecmp(OpData, "/H")) == 0) {
			if ((strcmp(u.sHandle, "") != 0 && *(u.sHandle) != ' '))
			    snprintf(msg, 81, "%-25s", u.sHandle);
			else
			    snprintf(msg, 81, "%-25s", u.sUserName);
		    } else if (strcasecmp(OpData, "/U") == 0) {
			snprintf(msg, 81, "%-25s", u.Name);
		    } else {
			snprintf(msg, 81, "%-25s", u.sUserName);
		    }
		    PUTSTR(msg);

		    snprintf(msg, 81, "%-30s%-14s%-10d", u.sLocation, StrDateDMY(u.tLastLoginDate), u.iTotalCalls);
		    PUTSTR(msg);
		    iFoundName = TRUE;
		    LineCount++;
		    iNameCount++; 
		    Enter(1);
		}
	    }
	} else if ((!u.Hidden) && (!u.Deleted) && (strlen(u.sUserName) > 0)) {
	    if ((strcmp(OpData, "/H")) == 0) {
		if ((strcasecmp(u.sHandle, "") != 0 && *(u.sHandle) != ' '))
		    snprintf(msg, 81, "%-25s", u.sHandle);
		else
		    snprintf(msg, 81, "%-25s", u.sUserName);
	    } else if (strcasecmp(OpData, "/U") == 0) {
		snprintf(msg, 81, "%-25s", u.Name);
	    } else {
		snprintf(msg, 81, "%-25s", u.sUserName);
	    }
	    PUTSTR(msg);
	    
	    snprintf(msg, 81, "%-30s%-14s%-10d", u.sLocation, StrDateDMY(u.tLastLoginDate), u.iTotalCalls);
	    PUTSTR(msg);
	    iFoundName = TRUE;
	    LineCount++;
	    iNameCount++;
	    Enter(1);
	}

	if (LineCount >= rows - 2) {
	    LineCount = 0;
	    Pause();
	    colour(CYAN, BLACK);
	}
    }

    if (!iFoundName) {
	language(CYAN, BLACK, 129);
	Enter(1);
    }

    fclose(pUsrConfig);

    strcpy(ustr, colour_str(GREEN, BLACK));
    strncat(ustr, fLine_str(cols -1), 127);
    PUTSTR(chartran(ustr));

    free(temp);
    free(Name);
    free(sTemp);
    free(User);

    chartran_close();

    Pause();
}



