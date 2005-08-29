/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Display Last Callers
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "lastcallers.h"
#include "term.h"
#include "ttyio.h"


/*
 * Last caller action flags
 */
extern int	LC_Download;
extern int	LC_Upload;
extern int	LC_Read;
extern int	LC_Wrote;
extern int	LC_Chat;
extern int	LC_Olr;
extern int	LC_Door;



/*
 * Display last callers screen.
 */
void LastCallers(char *OpData)
{
    FILE		    *pLC;
    int			    LineCount = 5, count = 0, i, x;
    char		    *sFileName, *Heading, *Underline;
    struct lastcallers	    lcall;
    struct lastcallershdr   lcallhdr;

    sFileName = calloc(PATH_MAX, sizeof(char));
    Heading   = calloc(81, sizeof(char));
    Underline = calloc(81, sizeof(char));

    clear();

    snprintf(sFileName, PATH_MAX, "%s/etc/lastcall.data", getenv("MBSE_ROOT"));
    if ((pLC = fopen(sFileName,"r")) == NULL) 
	WriteError("$LastCallers: Can't open %s", sFileName);
    else {
	fread(&lcallhdr, sizeof(lcallhdr), 1, pLC);
	colour(WHITE, BLACK);
	/* Todays callers to */
	snprintf(Heading, 81, "%s%s", (char *) Language(84), CFG.bbs_name);
	Center(Heading);

	x = strlen(Heading);

	for(i = 0; i < x; i++)
	    snprintf(Underline, 81, "%s%c", Underline, exitinfo.GraphMode ? 196 : 45);

	colour(LIGHTRED, BLACK);
	Center(Underline);

	Enter(1);

	/* #  User Name               Device  timeOn  Calls Location */
	pout(LIGHTGREEN, BLACK, (char *) Language(85));
	Enter(1);

	colour(GREEN, BLACK);
	fLine(79);
		
	while (fread(&lcall, lcallhdr.recsize, 1, pLC) == 1) {
	    if (!lcall.Hidden) {
		count++;

		colour(WHITE, BLACK);
		snprintf(Heading, 81, "%-5d", count);
		PUTSTR(Heading);

		colour(LIGHTCYAN, BLACK);
		if ((strcasecmp(OpData, "/H")) == 0) {
		    if ((strcmp(lcall.Handle, "") != 0 && *(lcall.Handle) != ' '))
			snprintf(Heading, 81, "%-20s", lcall.Handle);
		    else
			snprintf(Heading, 81, "%-20s", lcall.UserName);
		} else if (strcasecmp(OpData, "/U") == 0) {
		    snprintf(Heading, 81, "%-20s", lcall.Name);
		} else {
		    snprintf(Heading, 81, "%-20s", lcall.UserName);
		}
		PUTSTR(Heading);

		snprintf(Heading, 81, "%-8s", lcall.Device);
		pout(LIGHTBLUE, BLACK, Heading);

		snprintf(Heading, 81, "%-8s", lcall.TimeOn);
		pout(LIGHTMAGENTA, BLACK, Heading);

		snprintf(Heading, 81, "%-7d", lcall.Calls);
		pout(YELLOW, BLACK, Heading);

		snprintf(Heading, 81, "%-32s", lcall.Location);
		pout(LIGHTRED, BLACK, Heading);
		Enter(1);

		LineCount++;
		if (LineCount == exitinfo.iScreenLen) {
		    Pause();
		    LineCount = 0;
		}
	    } /* End of check if user is hidden */
	}

	colour(GREEN, BLACK);
	fLine(79);

	fclose(pLC);
	Enter(1);
	Pause();
    }
    free(sFileName);
    free(Heading);
    free(Underline);
}


