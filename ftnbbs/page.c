/*****************************************************************************
 *
 * $Id: page.c,v 1.19 2007/02/17 12:14:26 mbse Exp $
 * Purpose ...............: Sysop Paging
 * Todo ..................: Implement new config settings.
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
#include "dispfile.h"
#include "input.h"
#include "chat.h"
#include "page.h"
#include "timeout.h"
#include "mail.h"
#include "language.h"
#include "term.h"
#include "ttyio.h"


extern    pid_t           mypid;


/*
 * Function to Page Sysop
 */
void Page_Sysop(char *String)
{
    int		i;
    char	*Reason, temp[81];
    static char buf[128];

    clear();

    if (SYSOP == TRUE) {
	/*
	 * Forbid the sysop to page, paging himself causes troubles on the
	 * chatserver, the sysop can only use mbmon.
	 */
	Syslog('+', "The Sysop attempted to page the Sysop");
	pout(LIGHTRED, BLACK, (char *)"The Sysop cannot page the Sysop!");
	Enter(1);
	Pause();
	return;
    }

    Reason = calloc(81, sizeof(char));

    /* MBSE BBS Chat */
    poutCenter(LIGHTRED, BLACK, (char *) Language(151));

    if (CFG.iAskReason) {
	locate(4, 0);
	/* Enter a short reason for chat */
	poutCenter(GREEN, BLACK, (char *)Language(28));

	locate(6,0);
	colour(BLUE, BLACK);
	PUTCHAR(213);
	for (i = 0; i < 78; i++) 
	    PUTCHAR(205);
	PUTCHAR(184);
	Enter(1);

	PUTCHAR(' ');
	colour(LIGHTGRAY, BLACK);
	for (i = 0; i < 78; i++) 
	    PUTCHAR(250);
	Enter(1);

	colour(BLUE, BLACK);
	PUTCHAR(212);
	for (i = 0; i < 78; i++) 
	    PUTCHAR(205);
	PUTCHAR(190);
	Enter(1);

	locate(7, 2);
	colour(LIGHTGRAY, BLACK);
	temp[0] = '\0';
	GetPageStr(temp, 76);

	if ((strcmp(temp, "")) == 0)
	    return;

	Syslog('+', "Chat Reason: %s", temp);
	strcpy(Reason, temp);
    } else {
	snprintf(Reason, 81, "User want's to chat");
    }

    CFG.iMaxPageTimes--;

    if (CFG.iMaxPageTimes <= 0) {
	if (!DisplayFile((char *)"maxpage")) {
	    /* If i is FALSE display hard coded message */
	    Enter(1);
	    /* You have paged the Sysop the maximum times allowed. */
	    pout(WHITE, BLACK, (char *) Language(154));
	    Enter(2);
	}

	Syslog('!', "Attempted to page Sysop, above maximum page limit.");
	Pause();
	free(Reason);
	return;
    }
	
    locate(14, ((80 - strlen(String) ) / 2 - 2));
    pout(WHITE, BLACK, (char *)"[");
    pout(LIGHTGRAY, BLACK, String);
    pout(WHITE, BLACK, (char *)"]");

    locate(16, ((80 - CFG.iPageLength) / 2 - 2));
    pout(WHITE, BLACK, (char *)"[");
    colour(BLUE, BLACK);
    for (i = 0; i < CFG.iPageLength; i++)
	PUTCHAR(176);
    pout(WHITE, BLACK, (char *)"]");

    locate(16, ((80 - CFG.iPageLength) / 2 - 2) + 1);

    snprintf(buf, 128, "CPAG:2,%d,%s;", mypid, clencode(Reason));
    if (socket_send(buf)) {
	Syslog('+', "Failed to send message to mbtask");
	free(Reason);
	return;
    }
    strcpy(buf, socket_receive());

    /*
     * Check if sysop is busy
     */
    if (strcmp(buf, "100:1,1;") == 0) {
	/* The SysOp is currently speaking to somebody else */
	pout(LIGHTMAGENTA, BLACK, (char *) Language(152));
	/* Try paging him again in a few minutes ... */
	pout(LIGHTGREEN, BLACK, (char *) Language(153));
	Enter(2);
	Syslog('+', "SysOp was busy chatting with another user");
	Pause();
	free(Reason);
	return;
    }

    /*
     * Check if sysop is not available
     */
    if (strcmp(buf, "100:1,2;") == 0) {
	Syslog('+', "Sysop is not available for chat");
    }

    /*
     * Check for other errors
     */
    if (strcmp(buf, "100:1,3;") == 0) {
	pout(LIGHTRED, BLACK, (char *)"Internal system error, the sysop is informed");
	Enter(2);
	Syslog('!', "Got error on page sysop command");
	Pause();
	free(Reason);
	return;
    }

    if (strcmp(buf, "100:0;") == 0) {
	/*
	 * Page accpeted, wait until sysop responds
	 */
	colour(LIGHTBLUE, BLACK);
	for (i = 0; i < CFG.iPageLength; i++) {
	    PUTCHAR(219);
	    sleep(1);

	    snprintf(buf, 128, "CISC:1,%d", mypid);
	    if (socket_send(buf) == 0) {
		strcpy(buf, socket_receive());
		if (strcmp(buf, "100:1,1;") == 0) {
		    /*
		     * First cancel page request
		     */
		    snprintf(buf, 128, "CCAN:1,%d;", mypid);
		    socket_send(buf);
		    socket_receive();
		    Syslog('+', "Sysop responded to paging request");
		    Chat(exitinfo.Name, (char *)"#sysop");
		    free(Reason);
		    return;
		}
	    }
	}

	/*
	 * Cancel page request
	 */
	snprintf(buf, 128, "CCAN:1,%d;", mypid);
	socket_send(buf);
	strcpy(buf, socket_receive());
    }

    PageReason();
    Enter(3);
    Pause();
    if (strlen(Reason))
	SysopComment(Reason);
    else
	SysopComment((char *)"Failed chat");

    free(Reason);
    Pause();
}



/* 
 * Function gets string from user for Pager Function
 */
void GetPageStr(char *sStr, int iMaxlen)
{
    unsigned char   ch = 0; 
    int		    iPos = 0;

    strcpy(sStr, "");

    alarm_on();
    while (ch != 13) {
	ch = Readkey();

	if (((ch == 8) || (ch == KEY_DEL) || (ch == 127)) && (iPos > 0)) {
	    PUTCHAR('\b');
	    PUTCHAR(250);
	    PUTCHAR('\b');
	    sStr[--iPos]='\0';
	}

	if (ch > 31 && ch < 127) {
	    if (iPos <= iMaxlen) {
		iPos++;
		snprintf(sStr + strlen(sStr), 5, "%c", ch);
		PUTCHAR(ch);
		fflush(stdout);
	    } else
		ch=13;
	}
    }

    Enter(1);
}



/*
 * Function gets page reason from a file in the txtfiles directory
 * randomly generates a number and prints the string to the screen
 */
void PageReason()
{
    FILE    *Page;
    int	    iLoop = FALSE, id, i, j = 0;
    int	    Lines = 0, Count = 0, iFoundString = FALSE;
    char    *String;
    char    *temp;

    temp = calloc(PATH_MAX, sizeof(char));
    String = calloc(81, sizeof(char));

    snprintf(temp, PATH_MAX, "%s/share/int/txtfiles/%s/page.asc", getenv("MBSE_ROOT"), CFG.deflang);
    if ((Page = fopen(temp, "r")) != NULL) {

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
	    if (Lines == j) {
		Striplf(String);
		locate(18, ((78 - strlen(String) ) / 2));
		pout(WHITE, BLACK, (char *)"[");
		pout(LIGHTBLUE, BLACK, String);
		pout(WHITE, BLACK, (char *)"]");
		iFoundString = TRUE;
	    }

	    Lines++; /* Increment Lines until correct line is found */
	}
    } /* End of Else */

    if (!iFoundString) {
	/* Sysop currently is not available ... please leave a comment */
	snprintf(String, 81, "%s", (char *) Language(155));
	locate(18, ((78 - strlen(String) ) / 2));
	pout(WHITE, BLACK, (char *)"[");
	pout(LIGHTBLUE, BLACK, String);
	pout(WHITE, BLACK, (char *)"]");
    }

    free(temp);
    free(String);
}

