/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Inactivity timeout functions
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
#include "../lib/msg.h"
#include "timeout.h"
#include "funcs.h"
#include "bye.h"
#include "filesub.h"
#include "language.h"
#include "term.h"


extern	int e_pid;			/* Pid of external program	     */



void die(int onsig)
{
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

		/*
		 * In case the child had the tty in raw mode, reset the tty
		 */
		execute_pth((char *)"stty", (char *)"sane", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
	}

	if (MsgBase.Locked)
		Msg_UnLock();
	if (MsgBase.Open)
		Msg_Close();

	Home();
	signal(onsig, SIG_IGN);
	if (onsig)
		if (onsig == SIGHUP) {
			Syslog('+', "Lost Carrier");
		} else if (onsig == SIGALRM) {
			Syslog('+', "User inactivity timeout");
		} else {
			if (onsig <= NSIG)
				WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
			else
				WriteError("Terminated with error %d", onsig);
		}
	else
		Syslog(' ', "Terminated by user");

	if (onsig == SIGSEGV) {
		Syslog('+', "Last msg area %s", msgs.Name);
	}

	Good_Bye(onsig);
}



void alarm_sig()
{
	colour(LIGHTRED, BLACK);
	/* Autologout: idletime reached.*/
	printf("\r\n%s\r\n", (char *) Language(410));

	Syslog('!', "Autologout: idletime reached");
	die(SIGALRM);
}



void alarm_set(int val)
{
	signal(SIGALRM, (void (*))alarm_sig);
	alarm(val);
	Syslog('S', "Alarm set for %d seconds", val);
}



void alarm_on()
{
	alarm_set(60 * CFG.idleout);
}



void alarm_off()
{
	alarm(0);
	signal(SIGALRM, SIG_IGN);
	Syslog('S', "Alarm is off");
}

