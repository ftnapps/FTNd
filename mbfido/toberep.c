/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Add a file to the To-Be-Reported database 
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
    int			rc, Found = FALSE;

    fname = calloc(PATH_MAX, sizeof(char));
    sprintf(fname, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
    if ((tbr = fopen(fname, "r+")) == NULL) {
	if ((tbr = fopen(fname, "a+")) == NULL) {
	    WriteError("$Can't create %s", fname);
	    free(fname);
	    return FALSE;
	} else {
	    Syslog('f', "Created new %s", fname);
	}
    }
    free(fname);

    fseek(tbr, 0, SEEK_SET);
    while (fread(&Temp, sizeof(Temp), 1, tbr) == 1) {

	if (strcmp(Temp.Name, report.Name) == 0) {
	    Syslog('f', "Add_ToBeRep found record with the same name");
	    if (strlen(report.Echo) && (strcmp(Temp.Echo, report.Echo) == 0)) {
		Syslog('f', "Add_ToBeRep this is the same tic area !!!");
		/*
		 * If it's a later received file, update the record
		 */
		if (report.Fdate > Temp.Fdate) {
		    Syslog('f', "Add_ToBeRep this file is newer, update record at position %d", ftell(tbr));
		    rc = fseek(tbr, - sizeof(Temp), SEEK_CUR);
		    Syslog('f', "fseek rc=%d, size=%d", rc, sizeof(Temp));
		    Syslog('f', "Position before update is now %d", ftell(tbr));
		    rc = fwrite(&report, sizeof(Temp), 1, tbr);
		    Syslog('f', "Written %d, position after update is now %d", rc, ftell(tbr));
		    fclose(tbr);
		    return TRUE;
		}
		Syslog('f', "Add_ToBeRep this file is older, discard record");
		fclose(tbr);
		return TRUE;
	    }
	}
	if ((strcmp(Temp.Name, report.Name) == 0) && (Temp.Fdate == report.Fdate)) {
	    Syslog('f', "Add_ToBeRep record with same filename, but other area");
	    Found = TRUE;
	}
    }

    if (Found) {
	Syslog('!', "File %s already in toberep.data", report.Name);
	fclose(tbr);
	return FALSE;
    }

    /*
     * Append record
     */
    Syslog('f', "Add_ToBeRep append record at position %ld", ftell(tbr));
    fwrite(&report, sizeof(report), 1, tbr);
    fclose(tbr);
    return TRUE;
}



