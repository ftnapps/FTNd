/*****************************************************************************
 *
 * $Id: addbbs.c,v 1.39 2007/03/07 20:05:30 mbse Exp $
 * Purpose ...............: Add TIC file to the BBS
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "orphans.h"
#include "tic.h"
#include "fsort.h"
#include "qualify.h"
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
int Add_BBS(qualify **qal)
{
    struct FILE_record	    frec;
    int			    rc, i, Found = FALSE, Keep = 0, DidDelete = FALSE;
    char		    temp1[PATH_MAX], temp2[PATH_MAX], *fname, *lname, *p;
    fd_list		    *fdl = NULL;
    struct _fdbarea	    *fdb_area = NULL;
    qualify		    *tmpq;
    faddr		    *taka;

    /*
     * First check for an existing record with the same filename,
     * if it exists, update the record and we are ready. This will
     * prevent for example allfiles.zip to get a new record everytime
     * and thus the download counters will be reset after a new update.
     */
    if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30))) {
	while (fread(&frec, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
	    if (strcmp(frec.Name, TIC.NewFile) == 0) {
		snprintf(temp1, PATH_MAX, "%s/%s", TIC.Inbound, TIC.NewFile);
		snprintf(temp2, PATH_MAX, "%s/%s", TIC.BBSpath, TIC.NewFile);
		mkdirs(temp2, 0755);
		if ((rc = file_cp(temp1, temp2))) {
		    WriteError("Copy to %s failed: %s", temp2, strerror(rc));
		    mbsedb_CloseFDB(fdb_area);
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
		if (mbsedb_LockFDB(fdb_area, 30)) {
		    fseek(fdb_area->fp, 0 - fdbhdr.recsize, SEEK_CUR);
		    fwrite(&frec, fdbhdr.recsize, 1, fdb_area->fp);
		    mbsedb_UnlockFDB(fdb_area);
		}
		mbsedb_CloseFDB(fdb_area);
		tic_imp++;
		if ((i = file_rm(temp1)))
		    WriteError("file_rm(%s): %s", temp1, strerror(i));
		return TRUE;
	    }
	}
	mbsedb_CloseFDB(fdb_area);
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
    snprintf(frec.Uploader, 36, "Filemgr");
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

    snprintf(temp1, PATH_MAX, "%s/%s", TIC.Inbound, TIC.NewFile);
    snprintf(temp2, PATH_MAX, "%s/%s", TIC.BBSpath, frec.Name);
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
    if (getcwd(lname, PATH_MAX -1)) {
	if (chdir(TIC.BBSpath)) {
	    WriteError("$Can't chdir %s", TIC.BBSpath);
	} else {
	    // snprintf(lname, PATH_MAX, "%s/%s", TIC.BBSpath, frec.LName);
	    if (symlink(frec.Name, frec.LName)) {
		WriteError("$Create link %s to %s failed", frec.Name, frec.LName);
	    }
	}
    }
    free(lname);

    if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30)) == NULL)
	return FALSE;
    mbsedb_InsertFDB(fdb_area, frec, area.AddAlpha);
    mbsedb_CloseFDB(fdb_area);

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

	if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30))) {
	    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
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
			    if (mbsedb_LockFDB(fdb_area, 30)) {
				fseek(fdb_area->fp , - fdbhdr.recsize, SEEK_CUR);
				fwrite(&fdb, fdbhdr.recsize, 1, fdb_area->fp);
				mbsedb_UnlockFDB(fdb_area);
			    }
			    DidDelete = TRUE;
			}
		    }
		}
	    }
	    mbsedb_CloseFDB(fdb_area);
	}
    }

    /*
     * Handle the Keep number of files option
     */
    if (TIC.KeepNum) {
	if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30))) {
	    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
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
	    mbsedb_CloseFDB(fdb_area);
	}

	/*
	 * If there are files to delete, mark them.
	 */
	if (Keep > TIC.KeepNum) {
	    sort_fdlist(&fdl);

	    if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30))) {
		for (i = 0; i < (Keep - TIC.KeepNum); i++) {
		    fname = pull_fdlist(&fdl);
		    fseek(fdb_area->fp, fdbhdr.hdrsize, SEEK_SET);
		    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
			if (strcmp(fdb.LName, fname) == 0) {
			    Syslog('+', "Keep %d files, deleting: %s", TIC.KeepNum, fdb.LName);
			    fdb.Deleted = TRUE;
			    if (mbsedb_LockFDB(fdb_area, 30)) {
				fseek(fdb_area->fp , - fdbhdr.recsize, SEEK_CUR);
				fwrite(&fdb, fdbhdr.recsize, 1, fdb_area->fp);
				mbsedb_UnlockFDB(fdb_area);
			    }
			    DidDelete = TRUE;
			}
		    }
		}
		mbsedb_CloseFDB(fdb_area);
    		}
	}
	tidy_fdlist(&fdl);
    }

    /*
     *  Now realy delete the marked files and clean the file
     *  database.
     */
    if (DidDelete) {
	if ((fdb_area = mbsedb_OpenFDB(tic.FileArea, 30))) {
	    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
		if (fdb.Deleted) {
		    snprintf(temp2, PATH_MAX, "%s/%s", area.Path, fdb.LName);
		    if (unlink(temp2) != 0)
		        WriteError("$Can't unlink file %s", temp2);
		    snprintf(temp2, PATH_MAX, "%s/%s", area.Path, fdb.Name);

		    /*
		     * With the path to the 8.3 name, we can check if this file
		     * is attached for any possible downlink. We use the qualify
		     * list created by the ptic function to check connected nodes
		     * only.
		     */
		    for (tmpq = *qal; tmpq; tmpq = tmpq->next) {
			if (tmpq->send) {
			    taka = fido2faddr(tmpq->aka);
			    un_attach(taka, temp2);
			    tidy_faddr(taka);
			}
		    }

		    if (unlink(temp2) != 0)
		        WriteError("$Can't unlink file %s", temp2);
		    snprintf(temp2, PATH_MAX, "%s/.%s", area.Path, fdb.Name);
		    unlink(temp2); /* Thumbnail, no logging if there is an error */
		}
	    }
	    mbsedb_PackFDB(fdb_area);
	    mbsedb_CloseFDB(fdb_area);
	    DidDelete = FALSE;
	}
    }

    tic_imp++;
    return TRUE;
}


