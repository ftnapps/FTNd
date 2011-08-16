/*****************************************************************************
 *
 * $Id: lastcallers.c,v 1.13 2008/11/29 13:42:39 mbse Exp $
 * Purpose ...............: Display Last Callers
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
extern int	rows;
extern int	cols;



/*
 * Display last callers screen.
 */
void LastCallers(char *OpData)
{
    FILE		    *fp;
    int			    LineCount = 5, count = 0;
    char		    lstr[201], *sFileName, *Heading;
    struct lastcallers	    lcall;
    struct lastcallershdr   lcallhdr;

    sFileName = calloc(PATH_MAX, sizeof(char));
    Heading   = calloc(81, sizeof(char));

    if (utf8)
	chartran_init((char *)"CP437", (char *)"UTF-8", 'B');

    strcpy(lstr, clear_str());
    PUTSTR(chartran(lstr));

    snprintf(sFileName, PATH_MAX, "%s/etc/lastcall.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(sFileName,"r")) == NULL) 
	WriteError("$LastCallers: Can't open %s", sFileName);
    else {
	fread(&lcallhdr, sizeof(lcallhdr), 1, fp);

	strcpy(lstr, colour_str(WHITE, BLACK));
	/* Todays callers to */
	snprintf(Heading, 81, "%s%s", (char *) Language(84), CFG.bbs_name);
	strncat(lstr, Center_str(Heading), 200);
	PUTSTR(chartran(lstr));

	strcpy(lstr, colour_str(LIGHTRED, BLACK));
	strncat(lstr, Center_str(hLine_str(strlen(Heading))), 200);
	PUTSTR(chartran(lstr));
	Enter(1);

	/* #  User Name               Device  timeOn  Calls Location */
	strcpy(lstr, poutCR_str(LIGHTGREEN, BLACK, (char *) Language(85)));
	PUTSTR(chartran(lstr));

	strcpy(lstr, colour_str(GREEN, BLACK));
	strncat(lstr, fLine_str(cols -1), 200);
	PUTSTR(chartran(lstr));

	while (fread(&lcall, lcallhdr.recsize, 1, fp) == 1) {
	    if (!lcall.Hidden) {
		count++;

		strcpy(lstr, colour_str(WHITE, BLACK));
		snprintf(Heading, 80, "%-5d", count);
		strncat(lstr, Heading, 200);

		strncat(lstr, colour_str(LIGHTCYAN, BLACK), 200);
		if ((strcasecmp(OpData, "/H")) == 0) {
		    if ((strcmp(lcall.Handle, "") != 0 && *(lcall.Handle) != ' '))
			snprintf(Heading, 80, "%-20s", lcall.Handle);
		    else
			snprintf(Heading, 80, "%-20s", lcall.UserName);
		} else if (strcasecmp(OpData, "/U") == 0) {
		    snprintf(Heading, 80, "%-20s", lcall.Name);
		} else {
		    snprintf(Heading, 80, "%-20s", lcall.UserName);
		}
		strncat(lstr, Heading, 200);

		snprintf(Heading, 80, "%-8s", lcall.Device);
		strncat(lstr, pout_str(LIGHTBLUE, BLACK, Heading), 200);

		snprintf(Heading, 80, "%-8s", lcall.TimeOn);
		strncat(lstr, pout_str(LIGHTMAGENTA, BLACK, Heading), 200);

		snprintf(Heading, 80, "%-7d", lcall.Calls);
		strncat(lstr, pout_str(YELLOW, BLACK, Heading), 200);

		snprintf(Heading, 80, "%-32s", lcall.Location);
		strncat(lstr, pout_str(LIGHTRED, BLACK, Heading), 200);
		PUTSTR(chartran(lstr));
		Enter(1);

		LineCount++;
		if (LineCount == (rows -2)) {
		    Pause();
		    LineCount = 0;
		}

	    } /* End of check if user is hidden */
	}

	strcpy(lstr, colour_str(GREEN, BLACK));
	strncat(lstr, fLine_str(cols -1), 200);
	PUTSTR(chartran(lstr));

	fclose(fp);
	Enter(1);
	Pause();
    }

    free(sFileName);
    free(Heading);
    chartran_close();
}


