/*****************************************************************************
 *
 * File ..................: bbs/timecheck.c
 * Purpose ...............: Timecheck functions
 * Last modification date : 28-May-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
 *   
 * Michiel Broek		FIDO:		2:280/2802
 * Beekmansbos 10		Internet:	mbroek@users.sourceforge.net
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
#include "timecheck.h"
#include "funcs.h"
#include "funcs4.h"
#include "misc.h"
#include "bye.h"
#include "exitinfo.h"
#include "language.h"


/*
 * This is the users onlinetime check. Must be REWRITTEN!!
 */
void TimeCheck(void)
{
	char	temp[81];
	time_t	Now;
	int	Elapsed;

	time(&Now);

	/*
	 * Update the global string for the menu prompt
	 */
	sprintf(sUserTimeleft, "%d", iUserTimeLeft);
	ReadExitinfo();

	if (iUserTimeLeft != ((Time2Go - Now) / 60)) {

		Elapsed = iUserTimeLeft - ((Time2Go - Now) / 60);
		iUserTimeLeft -= Elapsed;
		sprintf(sUserTimeleft, "%d", iUserTimeLeft);

		sprintf(temp, "/tmp/.chat.%s", pTTY);
		/*
		 * Update users counter if not chatting
		 */
		if(!CFG.iStopChatTime || (access(temp, F_OK) != 0)) {
			exitinfo.iTimeLeft    -= Elapsed;
			exitinfo.iConnectTime += Elapsed;
			exitinfo.iTimeUsed    += Elapsed;
			WriteExitinfo();
		}
	}

	if(exitinfo.iTimeLeft <= 0) {
	 	printf("\n%s\n", (char *) Language(130));
		sleep(3);
		Syslog('!', "Users time limit exceeded ... user disconnected!");
		iExpired = TRUE;
		Good_Bye(1);
	}

	/*
	 *  Check for a personal message
	 */
	Check_PM();
}


