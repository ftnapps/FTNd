/*****************************************************************************
 *
 * morefile.c
 * Purpose ...............: Display file with more
 *
 *****************************************************************************
 * Copyright (C) 2013-2017 Robert James Clay <jame@rocasa.us>
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 *
 * This file is part of FTNd.
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2, or (at your option) any later
 * version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with FTNd; see the file COPYING.  If not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/ftndlib.h"
#include "../lib/ftnd.h"
#include "../lib/users.h"
#include "input.h"
#include "language.h"
#include "morefile.h"
#include "timeout.h"
#include "term.h"
#include "ttyio.h"


extern int  rows;


int MoreFile(char *filename)
{
    char	Buf[81];
    static FILE	*fptr;
    int		lines, input, ignore = FALSE, maxlines;

    maxlines = lines = rows - 2;

    if ((fptr =  fopen(filename,"r")) == NULL) {
	snprintf(Buf, 81, "%s%s", (char *) Language(72), filename);
	pout(LIGHTRED, BLACK, Buf);
	Enter(2);
	return(0);
    }

    Enter(1);

    while (fgets(Buf,80,fptr) != NULL) {
	if ((lines != 0) || (ignore)) {
	    lines--;
	    PUTSTR(Buf);
	}

	if (strlen(Buf) == 0) {
	    fclose(fptr);
	    return(0);
	}
	if (lines == 0) {
	    /* More (Y/n/=) */
	    snprintf(Buf, 81, " %sY\x08", (char *) Language(61));
	    PUTSTR(Buf);
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



