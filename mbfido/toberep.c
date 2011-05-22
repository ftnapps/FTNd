/*****************************************************************************
 *
 * Purpose ...............: Add a file to the To-Be-Reported database 
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
#include "orphans.h"
#include "tic.h"
#include "toberep.h"


/*
 *  Add a file record to the toberep database and do some checks.
 *  The function returns TRUE if the file will be announced.
 *  The newfiles announce option will later remove these records.
 */
int Add_ToBeRep(struct _filerecord report)
{
    char		*fname;
    struct _filerecord	Temp;
    FILE		*tbr;
    int			Found = FALSE;

    fname = calloc(PATH_MAX, sizeof(char));
    snprintf(fname, PATH_MAX, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
    if ((tbr = fopen(fname, "r+")) == NULL) {
	if ((tbr = fopen(fname, "a+")) == NULL) {
	    WriteError("$Can't create %s", fname);
	    free(fname);
	    return FALSE;
	}
    }
    free(fname);

    fseek(tbr, 0, SEEK_SET);
    while (fread(&Temp, sizeof(Temp), 1, tbr) == 1) {

	if (strcmp(Temp.Name, report.Name) == 0) {
	    if (strlen(report.Echo) && (strcmp(Temp.Echo, report.Echo) == 0)) {
		/*
		 * If it's a later received file, update the record
		 */
		if (report.Fdate > Temp.Fdate) {
		    fseek(tbr, - sizeof(Temp), SEEK_CUR);
		    fwrite(&report, sizeof(Temp), 1, tbr);
		    fclose(tbr);
		    return TRUE;
		}
		fclose(tbr);
		return TRUE;
	    }
	}
	if ((strcmp(Temp.Name, report.Name) == 0) && (Temp.Fdate == report.Fdate)) {
	    Found = TRUE;
	}
    }

    if (Found) {
	fclose(tbr);
	return FALSE;
    }

    /*
     * Append record
     */
    fwrite(&report, sizeof(report), 1, tbr);
    fclose(tbr);
    return TRUE;
}



