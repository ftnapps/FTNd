/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Add TIC file to the BBS
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "tic.h"
#include "fsort.h"
#include "addbbs.h"


extern	int	tic_imp;


/*
 *  Add file to the BBS file database and place it in the download
 *  directory. If it is replacing a file, a file with a matching name
 *  will be deleted. If there is a limit on the number of files with
 *  the  same name pattern, the oldest files will be deleted. The
 *  files database will be packed if necessary. All modifications are
 *  done on temp files first.
 */
int Add_BBS()
{
    struct FILERecord	frec;
    int			rc, i, Insert, Done = FALSE, Found = FALSE;
    char		fdbname[PATH_MAX], fdbtemp[PATH_MAX];
    char		temp1[PATH_MAX], temp2[PATH_MAX], *fname, *lname;
    FILE		*fdb, *fdt;
    int			Keep = 0, DidDelete = FALSE;
    fd_list		*fdl = NULL;

    /*
     * First create 8.3 filename
     */
    sprintf(temp1, "%s", TIC.NewName);
    name_mangle(temp1);

    /*
     * First check for an existing record with the same filename,
     * if it exists, update the record and we are ready. This will
     * prevent for example allfiles.zip to get a new record everytime
     * and thus the download counters will be reset after a new update.
     */
    sprintf(fdbname, "%s/fdb/fdb%ld.data", getenv("MBSE_ROOT"), tic.FileArea);
    if ((fdb = fopen(fdbname, "r+")) != NULL) {
	while (fread(&frec, sizeof(frec), 1, fdb) == 1) {
	    if ((strcmp(frec.Name, temp1) == 0) && (strcmp(frec.LName, TIC.NewName) == 0)) {
		sprintf(temp1, "%s/%s", TIC.Inbound, TIC.NewName);
		sprintf(temp2, "%s/%s", TIC.BBSpath, TIC.NewName);
		mkdirs(temp2, 0755);
		if ((rc = file_cp(temp1, temp2))) {
		    WriteError("Copy to %s failed: %s", temp2, strerror(rc));
		    fclose(fdb);
		    return FALSE;
		}
		chmod(temp2, 0644);
		frec.TicAreaCRC =  StringCRC32(TIC.TicIn.Area);
		frec.Size = TIC.FileSize;
		frec.Crc32 = TIC.Crc_Int;
		frec.Announced = TRUE;
		frec.FileDate = TIC.FileDate;
		frec.UploadDate = time(NULL);
		for (i = 0; i <= TIC.File_Id_Ct; i++) {
		    strcpy(frec.Desc[i], TIC.File_Id[i]);
		    if (i == 24)
			break;
		}
		if (strlen(TIC.TicIn.Magic))
		    sprintf(frec.Desc[i], "Magic Request: %s", TIC.TicIn.Magic);
		fseek(fdb, 0 - sizeof(frec), SEEK_CUR);
		fwrite(&frec, sizeof(frec), 1, fdb);
		fclose(fdb);
		tic_imp++;
		if ((i = file_rm(temp1)))
		    WriteError("file_rm(%s): %s", temp1, strerror(i));
		return TRUE;
	    }
	}
	fclose(fdb);
    }

    /*
     * Create filedatabase record.
     */
    memset(&frec, 0, sizeof(frec));
    strcpy(frec.Name, temp1);
    strcpy(frec.LName, TIC.NewName);
    frec.TicAreaCRC =  StringCRC32(TIC.TicIn.Area);
    frec.Size = TIC.FileSize;
    frec.Crc32 = TIC.Crc_Int;
    frec.Announced = TRUE;
    sprintf(frec.Uploader, "Filemgr");
    frec.UploadDate = time(NULL);
    frec.FileDate = TIC.FileDate;
    for (i = 0; i <= TIC.File_Id_Ct; i++) {
	strcpy(frec.Desc[i], TIC.File_Id[i]);
	if (i == 24)
	    break;
    }
    if (strlen(TIC.TicIn.Magic))
	sprintf(frec.Desc[i], "Magic Request: %s", TIC.TicIn.Magic);

    sprintf(temp1, "%s/%s", TIC.Inbound, TIC.NewName);
    sprintf(temp2, "%s/%s", TIC.BBSpath, TIC.NewName);
    mkdirs(temp2, 0755);

    if ((rc = file_cp(temp1, temp2))) {
	WriteError("Copy to %s failed: %s", temp2, strerror(rc));
	return FALSE;
    }
    chmod(temp2, 0644);
    lname = calloc(PATH_MAX, sizeof(char));
    sprintf(lname, "%s/%s", TIC.BBSpath, frec.Name);
    if (link(temp2, lname)) {
	WriteError("$Create link %s to %s failed", lname, temp2);
    }
    free(lname);

    sprintf(fdbtemp, "%s/fdb/fdb%ld.temp", getenv("MBSE_ROOT"), tic.FileArea);

    if ((fdb = fopen(fdbname, "r+")) == NULL) {
	Syslog('+', "Fdb %s doesn't exist, creating", fdbname);
	if ((fdb = fopen(fdbname, "a+")) == NULL) {
	    WriteError("$Can't create %s", fdbname);
	    return FALSE;
	}
	chmod(fdbname, 0660);
    }

    /*
     * If there are no files in this area, we append the first
     * one and leave immediatly, keepnum and replace have no
     * use at this point.
     */
    fseek(fdb, 0, SEEK_END);
    if (ftell(fdb) == 0) {
	fwrite(&frec, sizeof(frec), 1, fdb);
	fclose(fdb);
	file_rm(temp1);
	tic_imp++;
	return TRUE;
    }

    /*
     * There are already files in the area. We must now see at
     * which position to insert the new file, replace or
     * remove the old entry.
     */
    fseek(fdb, 0, SEEK_SET);

    Insert = 0;
    do {
	if (fread(&file, sizeof(file), 1, fdb) != 1)
	    Done = TRUE;
	if (!Done) {
	    if (strcmp(frec.LName, file.LName) == 0) {
		Found = TRUE;
		Insert++;
	    } else if (strcmp(frec.LName, file.LName) < 0)
		Found = TRUE;
	    else
		Insert++;
	}
    } while ((!Found) && (!Done));

    if (Found) {
	if ((fdt = fopen(fdbtemp, "a+")) == NULL) {
	    WriteError("$Can't create %s", fdbtemp);
	    fclose(fdb);
	    return FALSE;
	}

	fseek(fdb, 0, SEEK_SET);
	/*
	 * Copy entries till the insert point.
	 */
	for (i = 0; i < Insert; i++) {
	    fread(&file, sizeof(file), 1, fdb);
	    /*
	     * Check if we are importing a file with the same
	     * name, if so, don't copy the original database
	     * record. The file is also overwritten.
	     */
	    if (strcmp(file.LName, frec.LName) != 0)
		fwrite(&file, sizeof(file), 1, fdt);
	}

	if (area.AddAlpha) {
	    /*
	     * Insert the new entry
	     */
	    fwrite(&frec, sizeof(frec), 1, fdt);
	}

	/*
	 * Append the rest of the entries.
	 */
	while (fread(&file, sizeof(file), 1, fdb) == 1) {
	    /*
	     * Check if we find a file with the same name,
	     * then we skip the record what was origionaly
	     * in the database record.
	     */
	    if (strcmp(file.LName, frec.LName) != 0)
		fwrite(&file, sizeof(file), 1, fdt);
	}

	if (!area.AddAlpha) {
	    /*
	     * Append the new entry
	     */
	    fwrite(&frec, sizeof(frec), 1, fdt);
	}
	fclose(fdt);
	fclose(fdb);

	/*
	 * Now make the changes for real.
	 */
	if (unlink(fdbname) == 0) {
	    rename(fdbtemp, fdbname);
	} else {
	    WriteError("$Can't unlink %s", fdbname);
	    unlink(fdbtemp);
	    return FALSE;
	}

    } else {
	/*
	 * Append the new entry
	 */
	fseek(fdb, 0, SEEK_END);
	fwrite(&frec, sizeof(frec), 1, fdb);
	fclose(fdb);
    }

    /*
     * Delete file from the inbound
     */
    if ((i = file_rm(temp1)))
	WriteError("file_rm(%s): %s", temp1, strerror(i));

    /*
     * Handle the replace option.
     */
    if ((strlen(TIC.TicIn.Replace)) && (tic.Replace)) {
	Syslog('f', "Must Replace: %s", TIC.TicIn.Replace);

	if ((fdb = fopen(fdbname, "r+")) != NULL) {

	    while (fread(&file, sizeof(file), 1, fdb) == 1) {
		if (strlen(file.LName) == strlen(TIC.NewName)) {
		    if (strcasecmp(file.LName, TIC.NewName) != 0) {
			Found = TRUE;
			for (i = 0; i < strlen(TIC.NewName); i++) {
			    if ((TIC.TicIn.Replace[i] != '?') && (toupper(TIC.TicIn.Replace[i]) != toupper(file.LName[i])))
				Found = FALSE;
			}
			if (Found) {
			    Syslog('+', "Replace: Deleting: %s", file.LName);
			    file.Deleted = TRUE;
			    fseek(fdb, - sizeof(file), SEEK_CUR);
			    fwrite(&file, sizeof(file), 1, fdb);
			    DidDelete = TRUE;
			}
		    }
		}
	    }
	    fclose(fdb);
	}
    }

    /*
     * Handle the Keep number of files option
     */
    if (TIC.KeepNum) {
	if ((fdb = fopen(fdbname, "r")) != NULL) {

	    while (fread(&file, sizeof(file), 1, fdb) == 1) {

		if ((strlen(file.LName) == strlen(TIC.NewName)) && (!file.Deleted)) {
		    Found = TRUE;

		    for (i = 0; i < strlen(file.LName); i++) {
			if ((TIC.NewName[i] < '0') || (TIC.NewName[i] > '9')) {
			    if (TIC.NewName[i] != file.LName[i]) {
				Found = FALSE;
			    }
			}
		    }
		    if (Found) {
			Keep++;
			fill_fdlist(&fdl, file.LName, file.UploadDate);
		    }
		}
	    }
	    fclose(fdb);
	}

	/*
	 * If there are files to delete, mark them.
	 */
	if (Keep > TIC.KeepNum) {
	    sort_fdlist(&fdl);

	    if ((fdb = fopen(fdbname, "r+")) != NULL) {
		for (i = 0; i < (Keep - TIC.KeepNum); i++) {
		    fname = pull_fdlist(&fdl);
		    fseek(fdb, 0, SEEK_SET);

		    while (fread(&file, sizeof(file), 1, fdb) == 1) {
			if (strcmp(file.LName, fname) == 0) {
			    Syslog('+', "Keep %d files, deleting: %s", TIC.KeepNum, file.LName);
			    file.Deleted = TRUE;
			    fseek(fdb, - sizeof(file), SEEK_CUR);
			    fwrite(&file, sizeof(file), 1, fdb);
			    DidDelete = TRUE;
			}
		    }
		}
		fclose(fdb);
	    }
	}
	tidy_fdlist(&fdl);
    }

    /*
     *  Now realy delete the marked files and clean the file
     *  database.
     */
    if (DidDelete) {
	if ((fdb = fopen(fdbname, "r")) != NULL) {
	    if ((fdt = fopen(fdbtemp, "a+")) != NULL) {
		while (fread(&file, sizeof(file), 1, fdb) == 1)
		    if (!file.Deleted)
			fwrite(&file, sizeof(file), 1, fdt);
		    else {
			sprintf(temp2, "%s/%s", area.Path, file.LName);
			if (unlink(temp2) != 0)
			    WriteError("$Can't unlink file %s", temp2);
			sprintf(temp2, "%s/%s", area.Path, file.Name);
			if (unlink(temp2) != 0)
			    WriteError("$Can't unlink file %s", temp2);
		    }
		fclose(fdb);
		fclose(fdt);
		if (unlink(fdbname) == 0) {
		    rename(fdbtemp, fdbname);
		} else {
		    WriteError("$Can't unlink %s", fdbname);
		    unlink(fdbtemp);
		}
	    } else {
		fclose(fdb);
	    }
	    DidDelete = FALSE;
	}
    }

    tic_imp++;
    return TRUE;
}


