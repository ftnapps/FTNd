/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Check filebase
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
#include "mbfutil.h"
#include "mbfcheck.h"



extern int	do_quiet;		/* Suppress screen output	    */
extern int	do_pack;		/* Pack filebase		    */



/*
 *  Check file database integrity, all files in the file database must
 *  exist in real, the size and date/time must match, the files crc is
 *  checked, and if anything is wrong, the file database is updated.
 *  If the file is missing the entry is marked as deleted. With the
 *  pack option that record will be removed.
 *  After these checks, de database is checked for missing records, if
 *  there are files on disk but not in the directory these files are 
 *  deleted. System files (beginning with a dot) are left alone and
 *  the files 'files.bbs', 'files.bak', '00index', 'header' 'readme' 
 *  and 'index.html' too.
 *
 *  Remarks:  Maybe if the crc check fails, and the date and time are
 *            ok, the file is damaged and must be made unavailable.
 */
void Check(void)
{
    FILE		*pAreas, *pFile;
    int			i, j, iAreas, iAreasNew = 0, Fix, inArea, iTotal = 0, iErrors =  0, rc;
    char		*sAreas, *fAreas, *newdir, *temp, *mname, *tname;
    DIR			*dp;
    struct dirent	*de;
    int			Found, Update;
    char		fn[PATH_MAX];
    struct stat		stb;
    struct passwd	*pw;
    struct group	*gr;
    struct FILEIndex	idx;

    sAreas = calloc(PATH_MAX, sizeof(char));
    fAreas = calloc(PATH_MAX, sizeof(char));
    newdir = calloc(PATH_MAX, sizeof(char));
    temp   = calloc(PATH_MAX, sizeof(char));
    mname  = calloc(PATH_MAX, sizeof(char));

    if (!do_quiet) {
	colour(3, 0);
	printf("Checking file database...\n");
    }

    sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("Can't open %s", sAreas);
	die(MBERR_INIT_ERROR);
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    fseek(pAreas, 0, SEEK_END);
    iAreas = (ftell(pAreas) - areahdr.hdrsize) / areahdr.recsize;

    for (i = 1; i <= iAreas; i++) {

	fseek(pAreas, ((i-1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
	fread(&area, areahdr.recsize, 1, pAreas);

	if (area.Available) {

	    IsDoing("Check area %d", i);

	    if (!do_quiet) {
		printf("\r%4d => %-44s    \b\b\b\b", i, area.Name);
		fflush(stdout);
	    }

	    /*
	     * Check if download directory exists, 
	     * if not, create the directory.
	     */
	    if (access(area.Path, R_OK) == -1) {
		Syslog('!', "No dir: %s", area.Path);
		sprintf(newdir, "%s/foobar", area.Path);
		mkdirs(newdir, 0775);
	    }

	    if (stat(area.Path, &stb) == 0) {
		/*
		 * Very extended directory check
		 */
		Fix = FALSE;
		if ((stb.st_mode & S_IRUSR) == 0) {
		    Fix = TRUE;
		    WriteError("No owner read access in %s, mode is %04o", area.Path, stb.st_mode & 0x1ff);
		}
		if ((stb.st_mode & S_IWUSR) == 0) {
		    Fix = TRUE;
		    WriteError("No owner write access in %s, mode is %04o", area.Path, stb.st_mode & 0x1ff);
		}
		if ((stb.st_mode & S_IRGRP) == 0) {
		    Fix = TRUE;
		    WriteError("No group read access in %s, mode is %04o", area.Path, stb.st_mode & 0x1ff);
		}
		if ((stb.st_mode & S_IWGRP) == 0) {
		    Fix = TRUE;
		    WriteError("No group write access in %s, mode is %04o", area.Path, stb.st_mode & 0x1ff);
		}
		if ((stb.st_mode & S_IROTH) == 0) {
		    Fix = TRUE;
		    WriteError("No others read access in %s, mode is %04o", area.Path, stb.st_mode & 0x1ff);
		}
		if (Fix) {
		    iErrors++;
		    if (chmod(area.Path, 0775))
			WriteError("Could not set mode to 0775");
		    else
			Syslog('+', "Corrected directory mode to 0775");
		}
		Fix = FALSE;
		pw = getpwuid(stb.st_uid);
		if (strcmp(pw->pw_name, (char *)"mbse")) {
		    WriteError("Directory %s not owned by user mbse", area.Path);
		    Fix = TRUE;
		}
		gr = getgrgid(stb.st_gid);
		if (strcmp(gr->gr_name, (char *)"bbs")) {
		    WriteError("Directory %s not owned by group bbs", area.Path);
		    Fix = TRUE;
		}
		if (Fix) {
		    iErrors++;
		    pw = getpwnam((char *)"mbse");
		    gr = getgrnam((char *)"bbs");
		    if (chown(area.Path, pw->pw_gid, gr->gr_gid))
			WriteError("Could not set owner to mbse.bbs");
		    else
			Syslog('+', "Corrected directory owner to mbse.bbs");
		}
	    } else {
		WriteError("Can't stat %s", area.Path);
	    }

	    sprintf(fAreas, "%s/fdb/file%d.data", getenv("MBSE_ROOT"), i);

	    /*
	     * Open the file database, if it doesn't exist,
	     * create an empty one.
	     */
	    if ((pFile = fopen(fAreas, "r+")) == NULL) {
		Syslog('!', "Creating new %s", fAreas);
		if ((pFile = fopen(fAreas, "a+")) == NULL) {
		    WriteError("$Can't create %s", fAreas);
		    die(MBERR_GENERAL);
		}
		fdbhdr.hdrsize = sizeof(fdbhdr);
		fdbhdr.recsize = sizeof(fdb);
		fwrite(&fdbhdr, sizeof(fdbhdr), 1, pFile);
	    } else {
		fread(&fdbhdr, sizeof(fdbhdr), 1, pFile);
	    }

	    /*
	     * We don't do any upgrade, so the header must be correct.
	     */
	    if (fdbhdr.hdrsize != sizeof(fdbhdr)) {
		Syslog('+', "fAreas hdrsize is corrupt: %d", fdbhdr.hdrsize);
		return;
	    }
	    if (fdbhdr.recsize != sizeof(fdb)) {
		Syslog('+', "fAreas recordsize is corrupt: %d, expected %d", fdbhdr.recsize, sizeof(fdbhdr));
		return;
	    }

	    /*
	     * Now start checking the files in the filedatabase
	     * against the contents of the directory.
	     */
	    inArea = 0;
	    while (fread(&fdb, fdbhdr.recsize, 1, pFile) == 1) {

		iTotal++;
		inArea++;
		sprintf(newdir, "%s/%s", area.Path, fdb.LName);
		sprintf(mname,  "%s/%s", area.Path, fdb.Name);

		if (file_exist(newdir, R_OK) && file_exist(mname, R_OK)) {
		    Syslog('+', "File %s area %d not on disk.", newdir, i);
		    if (!fdb.NoKill) {
			fdb.Deleted = TRUE;
			do_pack = TRUE;
		    }
		    iErrors++;
		    fseek(pFile, - fdbhdr.recsize, SEEK_CUR);
		    fwrite(&fdb, fdbhdr.recsize, 1, pFile);
		} else {
		    /*
		     * File exists, now check the file.
		     */
		    Marker();
		    Update = FALSE;

		    strcpy(temp, fdb.LName);
		    name_mangle(temp);
		    sprintf(mname, "%s/%s", area.Path, temp);
		    if (strcmp(fdb.Name, temp))  {
			Syslog('!', "Converted %s to %s", fdb.Name, temp);
			tname = calloc(PATH_MAX, sizeof(char));
			sprintf(tname, "%s/%s", area.Path, fdb.Name);
			rename(tname, mname);
			sprintf(tname, "%s/%s", area.Path, fdb.LName);
			unlink(tname);
			symlink(mname, tname);
			free(tname);
			strncpy(fdb.Name, temp, 12);
			iErrors++;
			Update = TRUE;
		    }

		    /*
		     * If 8.3 and LFN are the same, try to rename the LFN to lowercase.
		     */
		    if (strcmp(fdb.Name, fdb.LName) == 0) {
			/*
			 * 8.3 and LFN are the same.
			 */
			tname = calloc(PATH_MAX, sizeof(char));
			sprintf(tname, "%s/%s", area.Path, fdb.LName);
			for (j = 0; j < strlen(fdb.LName); j++)
			    fdb.LName[j] = tolower(fdb.LName[j]);
			sprintf(newdir, "%s/%s", area.Path, fdb.LName);
			if (strcmp(tname, newdir)) {
			    Syslog('+', "Rename LFN from %s to %s", fdb.Name, fdb.LName);
			    rename(tname, newdir);
			    Update = TRUE;
			}
			free(tname);
		    }

		    /*
		     * At this point we may have (depending on the upgrade level)
                     * a real file with a long name or a real file with a short name
		     * or both. One of them may also be a symbolic link or not exist
		     * at all. Whatever it was, make it good.
		     */
		    if ((lstat(newdir, &stb) == 0) && ((stb.st_mode & S_IFLNK) != S_IFLNK)) {
			/*
			 * Long filename is a regular file and not a symbolic link.
			 */
			if (lstat(mname, &stb) == 0) {
			    /*
			     * 8.3 name exists, is it a real file?
			     */
			    if ((stb.st_mode & S_IFLNK) != S_IFLNK) {
				unlink(newdir);
				symlink(mname, newdir);
				Syslog('+', "%s changed into symbolic link", newdir);
				iErrors++;
			    } else {
				/*
				 * 8.3 is a symbolic link.
				 */
				unlink(mname);
				rename(newdir, mname);
				symlink(mname, newdir);
				Syslog('+', "%s changed to real file", mname);
				iErrors++;
			    }
			} else {
			    /*
			     * No 8.3 name on disk.
			     */
			    rename(newdir, mname);
			    symlink(mname, newdir);
			    Syslog('+', "%s changed to real file and created symbolic link", mname);
			    iErrors++;
			}
		    } else if ((lstat(mname, &stb) == 0) && ((stb.st_mode & S_IFLNK) != S_IFLNK)) {
			/*
			 * Short filename is a real file.
			 */
			if (lstat(newdir, &stb) == 0) {
			    /*
			     * LFN exists, is it a real file?
			     */
			    if ((stb.st_mode & S_IFLNK) != S_IFLNK) {
			    	/*
			    	 * LFN is a real filename too.
			    	 */
			    	unlink(newdir);
			    	symlink(mname, newdir);
			    	Syslog('+', "%s changed into symbolic link", newdir);
			    	iErrors++;
			    }
			} else {
			    /*
			     * No LFN, create symbolic link
			     */
			    symlink(mname, newdir);
			    Syslog('+', "%s created symbolic link", newdir);
			    iErrors++;
			}
		    } else {
			/*
			 * Weird, could not happen
			 */
			Syslog('!', "Weird problem, %s is no regular file", newdir);
		    }

		    /*
		     * It could be that there is a thumbnail made of the LFN.
		     */
		    tname = calloc(PATH_MAX, sizeof(char));
		    sprintf(tname, "%s/.%s", area.Path, fdb.LName);
		    if (file_exist(tname, R_OK) == 0) {
			Syslog('+', "Removing thumbnail %s", tname);
			iErrors++;
			unlink(tname);
		    }
		    free(tname);


		    if (file_time(newdir) != fdb.FileDate) {
			Syslog('!', "Date mismatch area %d file %s", i, fdb.LName);
			fdb.FileDate = file_time(newdir);
			iErrors++;
			Update = TRUE;
		    }
		    if (file_size(newdir) != fdb.Size) {
			Syslog('!', "Size mismatch area %d file %s", i, fdb.LName);
			fdb.Size = file_size(newdir);
			iErrors++;
			Update = TRUE;
		    }
		    if (file_crc(newdir, CFG.slow_util && do_quiet) != fdb.Crc32) {
			Syslog('!', "CRC error area %d, file %s", i, fdb.LName);
			fdb.Crc32 = file_crc(newdir, CFG.slow_util && do_quiet);
			iErrors++;
			Update = TRUE;
		    }
		    Marker();
		    if (Update) {
			fseek(pFile, - fdbhdr.recsize, SEEK_CUR);
			fwrite(&fdb, fdbhdr.recsize, 1, pFile);
		    }
		}
		if (strlen(fdb.Magic)) {
		    rc = magic_check(fdb.Magic, fdb.Name);
		    if (rc == -1) {
			Syslog('+', "Area %d magic alias %s file %s is invalid", i, fdb.Magic, fdb.Name);
			memset(&fdb.Magic, 0, sizeof(fdb.Magic));
			fseek(pFile, - fdbhdr.recsize, SEEK_CUR);
			fwrite(&fdb, fdbhdr.recsize, 1, pFile);
			iErrors++;
		    }
		}
	    }
	    if (inArea == 0)
		Syslog('+', "Warning: area %d (%s) is empty", i, area.Name);

	    /*
	     * Check files in the directory against the database.
	     * This test is skipped for CD-rom.
	     */
	    if (!area.CDrom) {
		if ((dp = opendir(area.Path)) != NULL) {
		    while ((de = readdir(dp)) != NULL) {
			if (de->d_name[0] != '.') {
			    Marker();
			    Found = FALSE;
			    fseek(pFile, fdbhdr.hdrsize, SEEK_SET);
			    while (fread(&fdb, fdbhdr.recsize, 1, pFile) == 1) {
				if ((strcmp(fdb.LName, de->d_name) == 0) || (strcmp(fdb.Name, de->d_name) == 0)) {
				    if (!Found) {
					Found = TRUE;
				    } else {
					/*
					 * Record has been found before, so this must be
					 * a double record.
					 */
					Syslog('!', "Double file record area %d file %s", i, fdb.LName);
					iErrors++;
					fdb.Double = TRUE;
					do_pack = TRUE;
					fseek(pFile, - fdbhdr.recsize, SEEK_CUR);
					fwrite(&fdb, fdbhdr.recsize, 1, pFile);
				    }
				}
			    }
			    if ((!Found) && (strncmp(de->d_name, "files.bbs", 9)) &&
				(strncmp(de->d_name, "files.bak", 9)) &&
				(strncmp(de->d_name, "00index", 7)) &&
				(strncmp(de->d_name, "header", 6)) &&
				(strncmp(de->d_name, "index", 5)) &&
				(strncmp(de->d_name, "readme", 6))) {
				sprintf(fn, "%s/%s", area.Path, de->d_name);
				if (stat(fn, &stb) == 0)
				    if (S_ISREG(stb.st_mode)) {
					if (unlink(fn) == 0) {
					    Syslog('!', "%s not in fdb, deleted from disk", fn);
					    iErrors++;
					} else {
					    WriteError("$%s not in fdb, cannot delete", fn);
					}
				    }
			    }
			}
		    }
		    closedir(dp);
		} else {
		    WriteError("Can't open %s", area.Path);
		}
	    }

	    fclose(pFile);
	    chmod(fAreas, 0660);
	    iAreasNew++;

	} else {
		    
	    if (strlen(area.Name) == 0) {
		sprintf(fAreas, "%s/fdb/file%d.data", getenv("MBSE_ROOT"), i);
		if (unlink(fAreas) == 0) {
		    Syslog('+', "Removed obsolete %s", fAreas);
		}
	    }

	} /* if area.Available */
    }

    fclose(pAreas);

    if (!do_quiet) {
	printf("\rChecking magic alias names ...                                    \r");
	fflush(stdout);
    }

    if (!strlen(CFG.req_magic)) {
	WriteError("No magic filename path configured");
    } else {
	if ((dp = opendir(CFG.req_magic)) == NULL) {
	    WriteError("$Can't open directory %s", CFG.req_magic);
	} else {
	    while ((de = readdir(dp))) {
		if (de->d_name[0] != '.') {
		    sprintf(temp, "%s/%s", CFG.req_magic, de->d_name);
		    if (file_exist(temp, X_OK) == 0) {
			Syslog('f', "%s is executable", temp);
		    } else if (file_exist(temp, R_OK) == 0) {
			if ((pFile = fopen(temp, "r"))) {
			    fgets(mname, PATH_MAX -1, pFile);
			    fclose(pFile);
			    Striplf(mname);
			    sprintf(newdir, "%s/etc/request.index", getenv("MBSE_ROOT"));
			    Found = FALSE;
			    if ((pFile = fopen(newdir, "r"))) {
				while (fread(&idx, sizeof(idx), 1, pFile)) {
				    if ((strcmp(idx.Name, mname) == 0) || (strcmp(idx.LName, mname) == 0)) {
					Found = TRUE;
					break;
				    }
				}
				fclose(pFile);
			    }
			    if (!Found) {
				Syslog('+', "Error: magic alias %s (%s) is invalid, removed", de->d_name, mname);
				iErrors++;
				unlink(temp);
			    }
			}
		    } else {
			Syslog('f', "%s cannot be", temp);
		    }
		}
	    }
	    closedir(dp);
	}
    }

    if (!do_quiet) {
	printf("\r                                                                  \r");
	fflush(stdout);
    }

    free(mname);
    free(temp);
    free(newdir);
    free(sAreas);
    free(fAreas);

    Syslog('+', "Check Areas [%5d] Files [%5d]  Errors [%5d]", iAreasNew, iTotal, iErrors);
}



