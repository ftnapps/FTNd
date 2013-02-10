/*****************************************************************************
 *
 * timeout.c
 * Purpose ...............: Inactivity timeout functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2007 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/ftndlib.h"
#include "../lib/ftnd.h"
#include "../lib/users.h"
#include "../lib/msg.h"
#include "timeout.h"
#include "funcs.h"
#include "bye.h"
#include "filesub.h"
#include "language.h"
#include "term.h"


extern	int e_pid;			/* Pid of external program	*/
extern	int hanged_up;			/* Hanged up status		*/


void die(int onsig)
{
    if (onsig <= NSIG)
	signal(onsig, SIG_DFL);
     
    /*
     * First check if there is a child running, if so, kill it.
     */
    if (e_pid) {
	if ((kill(e_pid, SIGTERM)) == 0)
	    Syslog('+', "SIGTERM to pid %d succeeded", e_pid);
	else {
	    if ((kill(e_pid, SIGKILL)) == 0)
		Syslog('+', "SIGKILL to pid %d succeeded", e_pid);
	    else
		WriteError("Failed to kill pid %d", e_pid);
	}
    }

    if (MsgBase.Locked)
	Msg_UnLock();
    if (MsgBase.Open)
	Msg_Close();

    Home();
    if (onsig) {
	if (onsig == SIGHUP) {
	    hanged_up = 1;
	    Syslog('+', "Lost Carrier");
	} else if (onsig == SIGPIPE) {
	    hanged_up = 1;
	    Syslog('+', "Broken Pipe");
	} else if (onsig == SIGALRM) {
	    Syslog('+', "User inactivity timeout");
	} else {
	    if (onsig <= NSIG) {
		hanged_up = 1;
		WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	    } else {
		WriteError("Terminated with error %d", onsig);
	    }
	}
    } else {
	Syslog(' ', "Terminated by user");
    }

    if (onsig == SIGSEGV) {
	Syslog('+', "Last msg area %s", msgs.Name);
    }

    Good_Bye(onsig);
}



void alarm_sig()
{
    Enter(2);
    /* Autologout: idletime reached.*/
    pout(LIGHTRED, BLACK, (char *) Language(410));
    Enter(2);

    Syslog('!', "Autologout: idletime reached");
    die(SIGALRM);
}



void alarm_set(int val)
{
    signal(SIGALRM, (void (*))alarm_sig);
    alarm(val);
}



void alarm_on()
{
    alarm_set(60 * CFG.idleout);
}



void alarm_off()
{
    alarm(0);
    signal(SIGALRM, SIG_IGN);
}

