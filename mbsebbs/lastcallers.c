/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Display Last Callers
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "input.h"
#include "language.h"
#include "lastcallers.h"



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

    sprintf(sFileName,"%s/etc/lastcall.data", getenv("MBSE_ROOT"));
    if ((pLC = fopen(sFileName,"r")) == NULL) 
	WriteError("$LastCallers: Can't open %s", sFileName);
    else {
	fread(&lcallhdr, sizeof(lcallhdr), 1, pLC);
	colour(WHITE, BLACK);
	/* Todays callers to */
	sprintf(Heading, "%s%s", (char *) Language(84), CFG.bbs_name);
	Center(Heading);

	x = strlen(Heading);

	for(i = 0; i < x; i++)
	    sprintf(Underline, "%s%c", Underline, exitinfo.GraphMode ? 196 : 45);

	colour(LIGHTRED, BLACK);
	Center(Underline);

	printf("\n");

	/* #  User Name               Device  timeOn  Calls Location */
	pout(LIGHTGREEN, BLACK, (char *) Language(85));
	Enter(1);

	colour(GREEN, BLACK);
	fLine(79);
		
	while (fread(&lcall, lcallhdr.recsize, 1, pLC) == 1) {
	    if (!lcall.Hidden) {
		count++;

		colour(WHITE, BLACK);
		printf("%-5d", count);

		colour(LIGHTCYAN, BLACK);
		if ((strcasecmp(OpData, "/H")) == 0) {
		    if ((strcmp(lcall.Handle, "") != 0 && *(lcall.Handle) != ' '))
			printf("%-20s", lcall.Handle);
		    else
			printf("%-20s", lcall.UserName);
		} else if (strcasecmp(OpData, "/U") == 0) {
		    printf("%-20s", lcall.Name);
		} else {
		    printf("%-20s", lcall.UserName);
		}

		colour(LIGHTBLUE, BLACK);
		printf("%-8s", lcall.Device);

		colour(LIGHTMAGENTA, BLACK);
		printf("%-8s", lcall.TimeOn);

		colour(YELLOW, BLACK);
		printf("%-7d", lcall.Calls);

		colour(LIGHTRED, BLACK);
		printf("%-32s\n", lcall.Location);

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
	printf("\n");
	Pause();
    }
    free(sFileName);
    free(Heading);
    free(Underline);
}


