/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Files database functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
 *   
 * Michiel Broek		FIDO:	2:280/2802
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
#include "mbselib.h"
#include "users.h"
#include "mbsedb.h"

#ifdef USE_EXPERIMEMT


/*
 * Add file to the BBS. The file must in the current directory which may not be
 * the destination directory.
 * The f_db record already has all needed information.
 * AllowReplace should be set by mbfido on ticfiles import to allow updates of
 * files like allfiles.zip
 */
int AddFile(struct FILE_record f_db, int Area, char *DestPath, char *FromPath, char *LinkPath, int AllowReplace)
{
    char    *temp1, *temp2;
    FILE    *fp1, *fp2;
    int     i, rc, Insert, Done = FALSE, Found = FALSE;

    if (AllowReplace) {
	/*
	 * First check for an existing record with the same filename,
	 * if it exists, update the record and we are ready. This will
	 * prevent for example allfiles.zip to get a new record everytime
	 * and thus the download counters will be reset after a new update.
	 */
	sprintf(fdbname, "%s/fdb/file%ld.data", getenv("MBSE_ROOT"), tic.FileArea);
	if ((fp = fopen(fdbname, "r+")) != NULL) {
	    fread(&frechdr, sizeof(frechdr), 1, fp);
	    while (fread(&frec, frechdr.recsize, 1, fp) == 1) {
		if (strcmp(frec.Name, TIC.NewFile) == 0) {
		    sprintf(temp1, "%s/%s", TIC.Inbound, TIC.NewFile);
		    sprintf(temp2, "%s/%s", TIC.BBSpath, TIC.NewFile);
		    mkdirs(temp2, 0755);
		    if ((rc = file_cp(temp1, temp2))) {
			WriteError("Copy to %s failed: %s", temp2, strerror(rc));
			fclose(fp);
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
		    fseek(fp, 0 - sizeof(frec), SEEK_CUR);
		    fwrite(&frec, sizeof(frec), 1, fp);
		    fclose(fp);
		    tic_imp++;
		    if ((i = file_rm(temp1)))
			WriteError("file_rm(%s): %s", temp1, strerror(i));
		    return TRUE;
		}
	    }
	    fclose(fp);
	}
    }


    /*
     * Copy file to the final destination and make a hard link with the
     * 8.3 filename to the long filename.
     */
    mkdirs(DestPath, 0775);

    if (file_exist(DestPath, F_OK) == 0) {
        WriteError("File %s already exists in area %d", f_db.Name, Area);
        if (!do_quiet)
            printf("\nFile %s already exists in area %d\n", f_db.Name, Area);
        return FALSE;
    }

    if ((rc = file_cp(FromPath, DestPath))) {
        WriteError("Can't copy file in place");
        if (!do_quiet)
            printf("\nCan't copy file to %s, %s\n", DestPath, strerror(rc));
        return FALSE;
    }
    chmod(DestPath, 0644);
    if (LinkPath) {
        if ((rc = symlink(DestPath, LinkPath))) {
            WriteError("Can't create symbolic link %s", LinkPath);
            if (!do_quiet)
                printf("\nCan't create symbolic link %s, %s\n", LinkPath, strerror(rc));
            unlink(DestPath);
            return FALSE;
        }
    }

    temp1 = calloc(PATH_MAX, sizeof(char));
    temp2 = calloc(PATH_MAX, sizeof(char));
    sprintf(temp1, "%s/fdb/file%d.data", getenv("MBSE_ROOT"), Area);
    sprintf(temp2, "%s/fdb/file%d.temp", getenv("MBSE_ROOT"), Area);

    fp1 = fopen(temp1, "r+");
    fread(&fdbhdr, sizeof(fdbhdr.hdrsize), 1, fp1);
    fseek(fp1, 0, SEEK_END);
    if (ftell(fp1) == fdbhdr.hdrsize) {
        /*
         * No records yet
         */
        fwrite(&f_db, fdbhdr.recsize, 1, fp1);
        fclose(fp1);
    } else {
        /*
         * Files are already there. Find the right spot.
         */
        fseek(fp1, fdbhdr.hdrsize, SEEK_SET);

        Insert = 0;
        do {
            if (fread(&fdb, fdbhdr.recsize, 1, fp1) != 1)
                Done = TRUE;
            if (!Done) {
                if (strcmp(f_db.LName, fdb.LName) == 0) {
                    Found = TRUE;
                    Insert++;
                } else {
                    if (strcmp(f_db.LName, fdb.LName) < 0)
                        Found = TRUE;
                    else
                        Insert++;
                }
            }
        } while ((!Found) && (!Done));

        if (Found) {
            if ((fp2 = fopen(temp2, "a+")) == NULL) {
                WriteError("Can't create %s", temp2);
                return FALSE;
            }
            fwrite(&fdbhdr, fdbhdr.hdrsize, 1, fp2);
            fseek(fp1, fdbhdr.hdrsize, SEEK_SET);

            /*
             * Copy until the insert point
             */
            for (i = 0; i < Insert; i++) {
                fread(&fdb, fdbhdr.recsize, 1, fp1);
                /*
                 * If we are importing a file with the same name,
                 * skip the original record and put the new one in place.
                 */
                if (strcmp(fdb.LName, f_db.LName) != 0)
                    fwrite(&fdb, fdbhdr.recsize, 1, fp2);
            }

            if (area.AddAlpha)
                fwrite(&f_db, fdbhdr.recsize, 1, fp2);

            /*
             * Append the rest of the records
             */
            while (fread(&fdb, fdbhdr.recsize, 1, fp1) == 1) {
                if (strcmp(fdb.LName, f_db.LName) != 0)
                    fwrite(&fdb, fdbhdr.recsize, 1, fp2);
            }
            if (!area.AddAlpha)
                fwrite(&f_db, fdbhdr.recsize, 1, fp2);
            fclose(fp1);
            fclose(fp2);

            if (unlink(temp1) == 0) {
                rename(temp2, temp1);
                chmod(temp1, 0660);
            } else {
                WriteError("$Can't unlink %s", temp1);
                unlink(temp2);
                return FALSE;
            }
        } else { /* if (Found) */
            /*
             * Append file record
             */
            fseek(fp1, 0, SEEK_END);
            fwrite(&f_db, fdbhdr.recsize, 1, fp1);
            fclose(fp1);
        }
    }

    free(temp1);
    free(temp2);
    return TRUE;
}



/*
 * Check files database, create if it doesn't exist.
 */
int mbsedb_CheckFDB(int Area, char *Path)
{
    char    *temp;
    FILE    *fp;
    int     rc = FALSE;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/fdb/file%d.data", getenv("MBSE_ROOT"), Area);

    /*
     * Open the file database, create new one if it doesn't excist.
     */
    if ((fp = fopen(temp, "r+")) == NULL) {
        Syslog('!', "Creating new %s", temp);
        if ((fp = fopen(temp, "a+")) == NULL) {
            WriteError("$Can't create %s", temp);
            rc = TRUE;
        } else {
            fdbhdr.hdrsize = sizeof(fdbhdr);
            fdbhdr.recsize = sizeof(fdb);
            fwrite(&fdbhdr, sizeof(fdbhdr), 1, fp);
            fclose(fp);
        }
    } else {
        fread(&fdbhdr, sizeof(fdbhdr), 1, fp);
        fclose(fp);
    }

    /*
     * Set the right attributes
     */
    chmod(temp, 0660);

    /*
     * Now check the download directory
     */
    if (access(Path, W_OK) == -1) {
        sprintf(temp, "%s/foobar", Path);
        if (mkdirs(temp, 0755))
            Syslog('+', "Created directory %s", Path);
    }

    free(temp);

    if ((fdbhdr.hdrsize != sizeof(fdbhdr)) || (fdbhdr.recsize != sizeof(fdb))) {
	WriteError("Files database header in area %d is corrupt", Area);
	rc = TRUE;
    }

    return rc;
}


#endif
