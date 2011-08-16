/*****************************************************************************
 *
 * $Id: timestats.c,v 1.11 2007/03/01 15:17:16 mbse Exp $
 * Purpose ...............: Time Statistics
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
#include "timestats.h"
#include "funcs.h"
#include "language.h"
#include "input.h"
#include "exitinfo.h"
#include "term.h"
#include "ttyio.h"


void TimeStats()
{
    char    Logdate[21], msg[81];

    Time_Now = time(NULL);
    l_date = localtime(&Time_Now);
    snprintf(Logdate, 21, "%02d-%s %02d:%02d:%02d", l_date->tm_mday, GetMonth(l_date->tm_mon+1),
                        l_date->tm_hour, l_date->tm_min, l_date->tm_sec);

    clear();
    ReadExitinfo();

    Enter(1);
    /* TIME STATISTICS for */
    snprintf(msg, 81, "%s%s ", (char *) Language(134), exitinfo.sUserName);
    pout(WHITE, BLACK, msg);
    /* on */
    snprintf(msg, 81, "%s %s", (char *) Language(135), Logdate);
    poutCR(WHITE, BLACK, msg);

    colour(LIGHTRED, BLACK);
    if (utf8)
	chartran_init((char *)"CP437", (char *)"UTF-8", 'B');
    PUTSTR(chartran(fLine_str(75)));
    chartran_close();

    Enter(1);
	
    /* Current Time */
    snprintf(msg, 81, "%s %s", (char *) Language(136), (char *) GetLocalHMS());
    poutCR(LIGHTGREEN, BLACK, msg);

    /* Current Date */
    snprintf(msg, 81, "%s %s", (char *) Language(137), (char *) GLCdateyy());
    poutCR(LIGHTGREEN, BLACK, msg);
    Enter(1);

    /* Connect time */
    snprintf(msg, 81, "%s %d %s", (char *) Language(138), exitinfo.iConnectTime, (char *) Language(471));
    poutCR(LIGHTGREEN, BLACK, msg);
    
    /* Time used today */
    snprintf(msg, 81, "%s %d %s", (char *) Language(139), exitinfo.iTimeUsed, (char *) Language(471));
    poutCR(LIGHTGREEN, BLACK, msg);
    
    /* Time remaining today */
    snprintf(msg, 81, "%s %d %s", (char *) Language(140), exitinfo.iTimeLeft, (char *) Language(471));
    poutCR(LIGHTGREEN, BLACK, msg);
    
    /* Daily time limit */
    snprintf(msg, 81, "%s %d %s", (char *) Language(141), exitinfo.iTimeUsed + exitinfo.iTimeLeft, (char *) Language(471));
    poutCR(LIGHTGREEN, BLACK, msg);
    
    Enter(1);
    Pause();
}


