/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Time Statistics
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
#include "timestats.h"
#include "funcs.h"
#include "language.h"
#include "input.h"
#include "exitinfo.h"



void TimeStats()
{
	char	Logdate[15];

        Time_Now = time(NULL);
        l_date = localtime(&Time_Now);
        sprintf(Logdate,"%02d-%s %02d:%02d:%02d", l_date->tm_mday, GetMonth(l_date->tm_mon+1),
                        l_date->tm_hour, l_date->tm_min, l_date->tm_sec);

	clear();
	ReadExitinfo();

	colour(15, 0);
	/* TIME STATISTICS for */
	printf("\n%s%s ", (char *) Language(134), exitinfo.sUserName);
	/* on */
	printf("%s %s\n", (char *) Language(135), Logdate);

	colour(12, 0);
	fLine(79);

	printf("\n");
	
	colour(10, 0);

	/* Current Time */
	printf("%s %s\n", (char *) Language(136), (char *) GetLocalHMS());

	/* Current Date */
	printf("%s %s\n\n", (char *) Language(137), (char *) GLCdateyy());

	/* Connect time */
	printf("%s %d %s\n", (char *) Language(138), exitinfo.iConnectTime, (char *) Language(471));

	/* Time used today */
	printf("%s %d %s\n", (char *) Language(139), exitinfo.iTimeUsed, (char *) Language(471));

	/* Time remaining today */
	printf("%s %d %s\n", (char *) Language(140), exitinfo.iTimeLeft, (char *) Language(471));

	/* Daily time limit */
	printf("%s %d %s\n", (char *) Language(141), exitinfo.iTimeUsed + exitinfo.iTimeLeft, (char *) Language(471));

	printf("\n");
	Pause();
}


