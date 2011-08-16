/*****************************************************************************
 *
 * $Id: morefile.c,v 1.10 2005/10/07 20:42:35 mbse Exp $
 * Purpose ...............: Display file with more
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



