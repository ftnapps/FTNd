/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Add a file to the To-Be-Reported database 
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
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "tic.h"


#include "toberep.h"


/*
 *  Add a file whos data is in T_File to the toberep.data file.
 *  The newfiles announce option will later remove these records.
 */
void Add_ToBeRep()
{
	char	fname[128];
	struct	_filerecord Temp;
	FILE	*tbr;
	int	Found = FALSE;

	sprintf(fname, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
	if ((tbr = fopen(fname, "a+")) == NULL) {
		WriteError("$Can't create %s", fname);
		return;
	}

	fseek(tbr, 0, SEEK_SET);
	while (fread(&Temp, sizeof(Temp), 1, tbr) == 1) {
		if ((strcmp(Temp.Name, T_File.Name) == 0) &&
		    (Temp.Fdate == T_File.Fdate) &&
		    (strcmp(Temp.Echo, T_File.Echo) == 0))
			Found = TRUE;
	}

	if (Found) {
		Syslog('!', "File %s already in toberep.data", T_File.Name);
		fclose(tbr);
		return;
	}

	fwrite(&T_File, sizeof(T_File), 1, tbr);
	fclose(tbr);
}



