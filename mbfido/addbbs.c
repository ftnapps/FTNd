/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Add TIC file to the BBS
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
#include "../lib/users.h"
#include "../lib/mbsedb.h"
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
    struct FILE_record	    frec;
    int			    rc, i, Found = FALSE, Keep = 0, DidDelete = FALSE;
    char		    temp1[PATH_MAX], temp2[PATH_MAX], *fname, *lname, *p;
    fd_list		    *fdl = NULL;
#ifdef	USE_EXPERIMENT
    struct _fdbarea	    *fdb_area = NULL;
#else
    struct FILE_recordhdr   frechdr;
    int			    Insert, Done = FALSE;
    char		    fdbname[PATH_MAX], fdbtemp[PATH_MAX];
    FILE		    *fp, *fdt;
#endif

    /*
     * First check for an existing record with the same filename,
     * if it exists, update the record and we are ready. This will
     * prevent for example allfiles.zip to get a new record everytime
     * and thus the download counters will be reset after a new update.
     */
#ifdef	USE_EXPERIMENT
    if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30))) {
	while (fread(&frec, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
#else
    sprintf(fdbname, "%s/fdb/file%ld.data", getenv("MBSE_ROOT"), tic.FileArea);
    if ((fp = fopen(fdbname, "r+")) != NULL) {
	fread(&frechdr, sizeof(frechdr), 1, fp);
	while (fread(&frec, frechdr.recsize, 1, fp) == 1) {
#endif
	    if (strcmp(frec.Name, TIC.NewFile) == 0) {
		sprintf(temp1, "%s/%s", TIC.Inbound, TIC.NewFile);
		sprintf(temp2, "%s/%s", TIC.BBSpath, TIC.NewFile);
		mkdirs(temp2, 0755);
		if ((rc = file_cp(temp1, temp2))) {
		    WriteError("Copy to %s failed: %s", temp2, strerror(rc));
#ifdef	USE_EXPERIMENT
		    mbsedb_CloseFDB(fdb_area);
#else
		    fclose(fp);
#endif
		    return FALSE;
		}
		chmod(temp2, 0644);
		strncpy(frec.TicArea, TIC.TicIn.Area, sizeof(frec.TicArea) -1);
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
#ifdef	USE_EXPERIMENT
		if (mbsedb_LockFDB(fdb_area, 30)) {
		    fseek(fdb_area->fp, 0 - sizeof(frec), SEEK_CUR);
		    fwrite(&frec, sizeof(frec), 1, fdb_area->fp);
		    mbsedb_UnlockFDB(fdb_area);
		}
		mbsedb_CloseFDB(fdb_area);
#else
		fseek(fp, 0 - sizeof(frec), SEEK_CUR);
		fwrite(&frec, sizeof(frec), 1, fp);
		fclose(fp);
#endif
		tic_imp++;
		if ((i = file_rm(temp1)))
		    WriteError("file_rm(%s): %s", temp1, strerror(i));
		return TRUE;
	    }
	}
#ifdef	USE_EXPERIMENT
	mbsedb_CloseFDB(fdb_area);
#else
	fclose(fp);
#endif
    }


    /*
     * Create filedatabase record.
     */
    memset(&frec, 0, sizeof(frec));
    strncpy(frec.Name, TIC.NewFile, sizeof(frec.Name) -1);
    if (strlen(TIC.NewFullName)) {
	strncpy(frec.LName, TIC.NewFullName, sizeof(frec.LName) -1);
    } else {
	/*
	 * No LFN, fake it with a lowercase copy of the 8.3 filename.
	 */
	strncpy(frec.LName, TIC.NewFile, sizeof(frec.LName) -1);
	for (i = 0; i < strlen(frec.LName); i++)
	    frec.LName[i] = tolower(frec.LName[i]);
    }
    strncpy(frec.TicArea, TIC.TicIn.Area, 20);
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
    if (strlen(TIC.TicIn.Magic)) {
	strncpy(frec.Magic, TIC.TicIn.Magic, sizeof(frec.Magic) -1);
    }

    sprintf(temp1, "%s/%s", TIC.Inbound, TIC.NewFile);
    sprintf(temp2, "%s/%s", TIC.BBSpath, frec.Name);
    mkdirs(temp2, 0755);

    if ((rc = file_cp(temp1, temp2))) {
	WriteError("Copy to %s failed: %s", temp2, strerror(rc));
	return FALSE;
    }
    chmod(temp2, 0644);

    /*
     * If LFN = 8.3 name and is DOS 8.3 format, change the LFN to lowercase.
     */
    if (strcmp(frec.Name, frec.LName) == 0) {
	p = frec.LName;
	while (*p) {
	    if (islower(*p))
		Found = TRUE;
	    p++;
	}
	if (!Found) {
	    /*
	     * All uppercase, change to lowercase.
	     */
	    tl(frec.LName);
	    Syslog('f', "Converted LFN to lowercase: \"%s\"", frec.LName);
	}
    }
    Found = FALSE;
    lname = calloc(PATH_MAX, sizeof(char));
    sprintf(lname, "%s/%s", TIC.BBSpath, frec.LName);
    if (symlink(temp2, lname)) {
	WriteError("$Create link %s to %s failed", temp2, lname);
    }
    free(lname);

#ifdef	USE_EXPERIMENT
    if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30)) == NULL)
	return FALSE;
    mbsedb_InsertFDB(fdb_area, frec, area.AddAlpha);
    mbsedb_CloseFDB(fdb_area);
#else
    sprintf(fdbtemp, "%s/fdb/file%ld.temp", getenv("MBSE_ROOT"), tic.FileArea);
    if ((fp = fopen(fdbname, "r+")) == NULL) {
	Syslog('+', "Fdb %s doesn't exist, creating", fdbname);
	if ((fp = fopen(fdbname, "a+")) == NULL) {
	    WriteError("$Can't create %s", fdbname);
	    return FALSE;
	}
	frechdr.hdrsize = sizeof(frechdr);
	frechdr.recsize = sizeof(frec);
	fwrite(&frechdr, sizeof(frechdr), 1, fp);
	chmod(fdbname, 0660);
    } else {
	fread(&frechdr, sizeof(frechdr), 1, fp);
    }

    /*
     * If there are no files in this area, we append the first
     * one and leave immediatly, keepnum and replace have no
     * use at this point.
     */
    fseek(fp, 0, SEEK_END);
    if (ftell(fp) == frechdr.hdrsize) {
	fwrite(&frec, sizeof(frec), 1, fp);
	fclose(fp);
	file_rm(temp1);
	tic_imp++;
	return TRUE;
    }

    /*
     * There are already files in the area. We must now see at
     * which position to insert the new file, replace or
     * remove the old entry.
     */
    fseek(fp, frechdr.hdrsize, SEEK_SET);

    Insert = 0;
    do {
	if (fread(&fdb, frechdr.recsize, 1, fp) != 1)
	    Done = TRUE;
	if (!Done) {
	    if (strcmp(frec.LName, fdb.LName) == 0) {
		Found = TRUE;
		Insert++;
	    } else if (strcmp(frec.LName, fdb.LName) < 0)
		Found = TRUE;
	    else
		Insert++;
	}
    } while ((!Found) && (!Done));

    if (Found) {
	if ((fdt = fopen(fdbtemp, "a+")) == NULL) {
	    WriteError("$Can't create %s", fdbtemp);
	    fclose(fp);
	    return FALSE;
	}
	fwrite(&frechdr, frechdr.hdrsize, 1, fdt);

	fseek(fp, frechdr.hdrsize, SEEK_SET);
	/*
	 * Copy entries till the insert point.
	 */
	for (i = 0; i < Insert; i++) {
	    fread(&fdb, frechdr.recsize, 1, fp);

	    /*
	     * If we see a magic that is the new magic, remove
	     * the old one.
	     */
	    if (strlen(TIC.TicIn.Magic) && (strcmp(fdb.Magic, TIC.TicIn.Magic) == 0)) {
		Syslog('f', "addbbs(): remove magic from %s (%s)", fdb.Name, fdb.LName);
		memset(&fdb.Magic, 0, sizeof(fdb.Magic));
	    }

	    /*
	     * Check if we are importing a file with the same
	     * name, if so, don't copy the original database
	     * record. The file is also overwritten.
	     */
	    if (strcmp(fdb.LName, frec.LName) != 0)
		fwrite(&fdb, frechdr.recsize, 1, fdt);
	}

	if (area.AddAlpha) {
	    /*
	     * Insert the new entry
	     */
	    fwrite(&frec, frechdr.recsize, 1, fdt);
	}

	/*
	 * Append the rest of the entries.
	 */
	while (fread(&fdb, frechdr.recsize, 1, fp) == 1) {

	    /*
	     * If we see a magic that is the new magic, remove
	     * the old one.
	     */
	    if (strlen(TIC.TicIn.Magic) && (strcmp(fdb.Magic, TIC.TicIn.Magic) == 0)) {
		Syslog('f', "addbbs(): remove magic from %s (%s)", fdb.Name, fdb.LName);
		memset(&fdb.Magic, 0, sizeof(fdb.Magic));
	    }

	    /*
	     * Check if we find a file with the same name,
	     * then we skip the record what was origionaly
	     * in the database record.
	     */
	    if (strcmp(fdb.LName, frec.LName) != 0)
		fwrite(&fdb, frechdr.recsize, 1, fdt);
	}

	if (!area.AddAlpha) {
	    /*
	     * Append the new entry
	     */
	    fwrite(&frec, frechdr.recsize, 1, fdt);
	}
	fclose(fdt);
	fclose(fp);

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
	fseek(fp, 0, SEEK_END);
	fwrite(&frec, frechdr.recsize, 1, fp);
	fclose(fp);
    }
#endif

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

#ifdef	USE_EXPERIMENT
	if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30))) {
	    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
#else
	if ((fp = fopen(fdbname, "r+")) != NULL) {
	    fread(&fdbhdr, sizeof(fdbhdr), 1, fp);
	    while (fread(&fdb, fdbhdr.recsize, 1, fp) == 1) {
#endif
		if (strlen(fdb.LName) == strlen(frec.LName)) {
		    // FIXME: Search must be based on a reg_exp search
		    if (strcasecmp(fdb.LName, frec.LName) != 0) {
			Found = TRUE;
			for (i = 0; i < strlen(frec.LName); i++) {
			    if ((TIC.TicIn.Replace[i] != '?') && (toupper(TIC.TicIn.Replace[i]) != toupper(fdb.LName[i])))
				Found = FALSE;
			}
			if (Found) {
			    Syslog('+', "Replace: Deleting: %s", fdb.LName);
			    fdb.Deleted = TRUE;
#ifdef	USE_EXPERIMENT
			    if (mbsedb_LockFDB(fdb_area, 30)) {
				fseek(fdb_area->fp , - fdbhdr.recsize, SEEK_CUR);
				fwrite(&fdb, fdbhdr.recsize, 1, fdb_area->fp);
				mbsedb_UnlockFDB(fdb_area);
			    }
#else
			    fseek(fp , - fdbhdr.recsize, SEEK_CUR);
			    fwrite(&fdb, fdbhdr.recsize, 1, fp);
#endif
			    DidDelete = TRUE;
			}
		    }
		}
	    }
#ifdef	USE_EXPERIMENT
	    mbsedb_CloseFDB(fdb_area);
#else
	    fclose(fp);
#endif
	}
    }

    /*
     * Handle the Keep number of files option
     */
    if (TIC.KeepNum) {
#ifdef	USE_EXPERIMENT
	if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30))) {
	    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
#else
	if ((fp = fopen(fdbname, "r")) != NULL) {
	    fread(&fdbhdr, sizeof(fdbhdr), 1, fp);
	    while (fread(&fdb, fdbhdr.recsize, 1, fp) == 1) {
#endif
		if ((strlen(fdb.LName) == strlen(frec.LName)) && (!fdb.Deleted)) {
		    Found = TRUE;

		    for (i = 0; i < strlen(fdb.LName); i++) {
			if ((frec.LName[i] < '0') || (frec.LName[i] > '9')) {
			    if (frec.LName[i] != fdb.LName[i]) {
				Found = FALSE;
			    }
			}
		    }
		    if (Found) {
			Keep++;
			fill_fdlist(&fdl, fdb.LName, fdb.UploadDate);
		    }
		}
	    }
#ifdef	USE_EXPERIMENT
	    mbsedb_CloseFDB(fdb_area);
#else
	    fclose(fp);
#endif
	}

	/*
	 * If there are files to delete, mark them.
	 */
	if (Keep > TIC.KeepNum) {
	    sort_fdlist(&fdl);

#ifdef	USE_EXPERIMENT
	    if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30))) {
#else
	    if ((fp = fopen(fdbname, "r+")) != NULL) {
		fread(&fdbhdr, sizeof(fdbhdr), 1, fp);
#endif
		for (i = 0; i < (Keep - TIC.KeepNum); i++) {
		    fname = pull_fdlist(&fdl);
#ifdef	USE_EXPERIMENT
		    fseek(fdb_area->fp, fdbhdr.hdrsize, SEEK_SET);
		    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
#else
		    fseek(fp, fdbhdr.hdrsize, SEEK_SET);
		    while (fread(&fdb, fdbhdr.recsize, 1, fp) == 1) {
#endif
			if (strcmp(fdb.LName, fname) == 0) {
			    Syslog('+', "Keep %d files, deleting: %s", TIC.KeepNum, fdb.LName);
			    fdb.Deleted = TRUE;
#ifdef	USE_EXPERIMENT
			    if (mbsedb_LockFDB(fdb_area, 30)) {
				fseek(fdb_area->fp , - fdbhdr.recsize, SEEK_CUR);
				fwrite(&fdb, fdbhdr.recsize, 1, fdb_area->fp);
				mbsedb_UnlockFDB(fdb_area);
			    }
#else
			    fseek(fp , - fdbhdr.recsize, SEEK_CUR);
			    fwrite(&fdb, fdbhdr.recsize, 1, fp);
#endif
			    DidDelete = TRUE;
			}
		    }
		}
#ifdef	USE_EXPERIMENT
		mbsedb_CloseFDB(fdb_area);
#else
		fclose(fp);
#endif
    		}
	}
	tidy_fdlist(&fdl);
    }

    /*
     *  Now realy delete the marked files and clean the file
     *  database.
     */
    if (DidDelete) {
#ifdef	USE_EXPERIMENT
	if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30))) {
		while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1)
#else
	if ((fp = fopen(fdbname, "r")) != NULL) {
	    fread(&fdbhdr, sizeof(fdbhdr), 1, fp);
	    if ((fdt = fopen(fdbtemp, "a+")) != NULL) {
		fwrite(&fdbhdr, fdbhdr.hdrsize, 1, fdt);
		while (fread(&fdb, fdbhdr.recsize, 1, fp) == 1)
#endif
		    if (!fdb.Deleted) {
#ifndef	USE_EXPERIMENT
			fwrite(&fdb, fdbhdr.recsize, 1, fdt);
#endif
		    } else {
			sprintf(temp2, "%s/%s", area.Path, fdb.LName);
			if (unlink(temp2) != 0)
			    WriteError("$Can't unlink file %s", temp2);
			sprintf(temp2, "%s/%s", area.Path, fdb.Name);
			if (unlink(temp2) != 0)
			    WriteError("$Can't unlink file %s", temp2);
			sprintf(temp2, "%s/.%s", area.Path, fdb.Name);
			unlink(temp2); /* Thumbnail, no logging if there is an error */
		    }
#ifdef	USE_EXPERIMENT
		mbsedb_PackFDB(fdb_area);
		mbsedb_CloseFDB(fdb_area);
#else
		fclose(fp);
		fclose(fdt);
		if (unlink(fdbname) == 0) {
		    rename(fdbtemp, fdbname);
		} else {
		    WriteError("$Can't unlink %s", fdbname);
		    unlink(fdbtemp);
		}
	    } else {
		fclose(fp);
	    }
#endif
	    DidDelete = FALSE;
	}
    }

    tic_imp++;
    return TRUE;
}


