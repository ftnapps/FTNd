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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

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
	FILE	*pLC;
	int	LineCount = 5;
	int	count = 0;
	char	*sFileName;
	char	*Heading;
	char	*Underline;
	int	i, x;
	struct	lastcallers lcall;
	struct	lastcallershdr lcallhdr;

	sFileName = calloc(PATH_MAX, sizeof(char));
	Heading   = calloc(81, sizeof(char));
	Underline = calloc(81, sizeof(char));

	clear();

	sprintf(sFileName,"%s/etc/lastcall.data", getenv("MBSE_ROOT"));
	if((pLC = fopen(sFileName,"r")) == NULL) 
		WriteError("$LastCallers: Can't open %s", sFileName);
	else {
		fread(&lcallhdr, sizeof(lcallhdr), 1, pLC);
		colour(15, 0);
		/* Todays callers to */
		sprintf(Heading, "%s%s", (char *) Language(84), CFG.bbs_name);
		Center(Heading);

		x = strlen(Heading);

		for(i = 0; i < x; i++)
       			sprintf(Underline, "%s%c", Underline, exitinfo.GraphMode ? 196 : 45);

		colour(12, 0);
		Center(Underline);

		printf("\n");

		/* #  User Name               Device  timeOn  Calls Location */
		pout(10, 0, (char *) Language(85));
		Enter(1);

		colour(2, 0);
		fLine(79);
		
		while (fread(&lcall, lcallhdr.recsize, 1, pLC) == 1) {
			if(!lcall.Hidden) {
				count++;

				colour(15,0);
				printf("%-5d", count);

				colour(11, 0);
				if((strcmp(OpData, "/H")) == 0) {
					if((strcmp(lcall.Handle, "") != 0 && *(lcall.Handle) != ' '))
						printf("%-20s", lcall.Handle);
					else
						printf("%-20s", lcall.UserName);
				} else
					printf("%-20s", lcall.UserName);

				colour(9, 0);
				printf("%-8s", lcall.Device);

				colour(13, 0);
				printf("%-8s", lcall.TimeOn);

				colour(14, 0);
				printf("%-7d", lcall.Calls);

				colour(12, 0);
				printf("%-32s\n", lcall.Location);

				LineCount++;
				if (LineCount == exitinfo.iScreenLen) {
					Pause();
					LineCount = 0;
				}
			} /* End of check if user is sysop */
		}

		colour(2, 0);
		fLine(79);

		fclose(pLC);
		printf("\n");
		Pause();
	}
	free(sFileName);
	free(Heading);
	free(Underline);
}


