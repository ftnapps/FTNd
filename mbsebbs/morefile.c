/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Display file with more
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
#include "morefile.h"
#include "timeout.h"



int MoreFile(char *filename)
{
	char	Buf[81];
	static	FILE *fptr;
	int	lines;
	int	input;
	int	ignore = FALSE;
	int	maxlines;

	maxlines = lines = exitinfo.iScreenLen - 2;

	if ((fptr =  fopen(filename,"r")) == NULL) {
		printf("%s%s\n", (char *) Language(72), filename);
		return(0);
	}

	printf("\n");

	while (fgets(Buf,80,fptr) != NULL) {
		if ((lines != 0) || (ignore)) {
			lines--;
			printf("%s",Buf);
		}

		if (strlen(Buf) == 0) {
			fclose(fptr);
			return(0);
		}
		if (lines == 0) {
			fflush(stdin);
			/* More (Y/n/=) */
			printf(" %sY\x08", (char *) Language(61));
			fflush(stdout);
			alarm_on();
			input = toupper(getchar());

			if ((input == Keystroke(61, 0)) || (input == '\r'))
				lines = maxlines;

			if (input == Keystroke(61, 1)) {
				fclose(fptr);
				return(0);
			}

			if (input == Keystroke(61, 2))
				ignore = TRUE;
			else
				lines  = maxlines;
		}
	}
	Pause();
	fclose(fptr);
	return 1;
}



