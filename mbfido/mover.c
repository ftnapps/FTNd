/*****************************************************************************
 *
 * File ..................: mbfido/mover.c
 * Purpose ...............: Bad file mover
 * Last modification date : 02-Nov-1999
 *
 *****************************************************************************
 * Copyright (C) 1997-1999
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "tic.h"
#include "mover.h"



void mover(char *srcdir, char *fn)
{
	char	*From, *To;

	From = calloc(128, sizeof(char));
	To   = calloc(128, sizeof(char));

	sprintf(From, "%s%s", srcdir, fn);
	sprintf(To,   "%s/%s", CFG.badtic, fn);
	Syslog('!', "Moving %s to %s", From, To);

	if (mkdirs(To)) {
		if (file_mv(From, To) != 0)
			WriteError("$Failed to move %s to %s", From, To);
	}

	free(From);
	free(To);
}



/*
 * Move the file and .tic file to the bad directory
 */
void MoveBad()
{
	mover(TIC.Inbound, TIC.TicName);
	mover(TIC.FilePath, TIC.TicIn.OrgName);
}


