/*****************************************************************************
 *
 * $Id: mover.c,v 1.15 2005/11/19 19:43:03 mbse Exp $
 * Purpose ...............: Bad file mover
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
 *   
 * Michiel Broek		FIDO:		2:2801/16
 * Beekmansbos 10		Internet:	mbroek@ux123.pttnwb.nl
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
#include "orphans.h"
#include "tic.h"
#include "mover.h"



void mover(char *fn)
{
    char	*From, *To;
    int	rc;

    From = calloc(PATH_MAX, sizeof(char));
    To   = calloc(PATH_MAX, sizeof(char));

    snprintf(From, PATH_MAX, "%s/%s", TIC.Inbound, fn);
    snprintf(To,   PATH_MAX, "%s/%s", CFG.badtic, fn);
    Syslog('+', "Moving %s to %s", From, To);

    if (mkdirs(To, 0770)) {
	if ((rc = file_mv(From, To))) {
	    WriteError("Failed to move %s to %s: %s", From, To, strerror(rc));
	}
    } else {
	WriteError("Can't create directory for %s", To);
    }

    free(From);
    free(To);
}



/*
 * Move the file and .tic file to the bad directory
 */
void MoveBad()
{
    mover(TIC.TicName);
    mover(TIC.NewFile);
}


