/*****************************************************************************
 *
 * $Id: timecheck.c,v 1.18 2005/10/17 18:02:00 mbse Exp $
 * Purpose ...............: Timecheck functions
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
#include "timecheck.h"
#include "funcs.h"
#include "bye.h"
#include "exitinfo.h"
#include "language.h"
#include "input.h"
#include "term.h"
#include "ttyio.h"


extern pid_t    mypid;		    /* Pid of this program	    */
extern int	chat_with_sysop;    /* True if chatting with sysop  */


/* 
 * Check for a personal message, this will go via mbsed. If there
 * is a message, it will be displayed, else nothing happens.
 */
void Check_PM(void);
void Check_PM(void)
{
    static char buf[200];
    char        resp[128], msg[81];

    snprintf(buf, 200, "CIPM:1,%d;", mypid);
    if (socket_send(buf) == 0) {
        strcpy(buf, socket_receive());
        if (strncmp(buf, "100:0;", 6) == 0)
            return;

        strncpy(resp, strtok(buf, ":"), 5);		/* Should be 100	*/
	strncpy(resp, strtok(NULL, ","), 3);		/* Should be 2		*/
	strncpy(resp, cldecode(strtok(NULL, ",")), 36);	/* From Name		*/

	Enter(2);
	PUTCHAR('\007');
        colour(CYAN, BLACK);
        /* ** Message ** from */
        snprintf(msg, 81, "%s %s:", (char *)Language(434), resp);
	poutCR(CYAN, BLACK, msg);
	strncpy(resp, cldecode(strtok(NULL, "\0")), 80);    /* The real message	*/
        PUTSTR(resp);
	Enter(1);
        Pause();
    }
}



/*
 * This is the users onlinetime check.
 */
void TimeCheck(void)
{
    time_t  Now;
    int	    Elapsed;

    Now = time(NULL);

    /*
     * Update the global string for the menu prompt
     */
    snprintf(sUserTimeleft, 7, "%d", iUserTimeLeft);
    ReadExitinfo();

    if (iUserTimeLeft != ((Time2Go - Now) / 60)) {

	Elapsed = iUserTimeLeft - ((Time2Go - Now) / 60);
	iUserTimeLeft -= Elapsed;
	snprintf(sUserTimeleft, 7, "%d", iUserTimeLeft);

	/*
	 * Update users counter if not chatting
	 */
	if (!CFG.iStopChatTime || (! chat_with_sysop)) {
	    exitinfo.iTimeLeft    -= Elapsed;
	    exitinfo.iConnectTime += Elapsed;
	    exitinfo.iTimeUsed    += Elapsed;
	    WriteExitinfo();
	}
    }

    if (exitinfo.iTimeLeft <= 0) {
	Enter(1);
	poutCR(YELLOW, BLACK, (char *) Language(130));
	sleep(3);
	Syslog('!', "Users time limit exceeded ... user disconnected!");
	iExpired = TRUE;
	Good_Bye(MBERR_TIMEOUT);
    }

    /*
     *  Check for a personal message
     */
    Check_PM();
}


