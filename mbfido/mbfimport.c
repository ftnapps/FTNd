/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Import files with files.bbs
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "../lib/mberrors.h"
#include "virscan.h"
#include "mbfutil.h"
#include "mbfimport.h"



extern int	do_quiet;		/* Suppress screen output	    */
extern int	do_annon;		/* Suppress announce files	    */
extern int	do_novir;		/* Suppress virus scanning	    */


void ImportFiles(int Area)
{
    char		*pwd, *temp, *temp2, *tmpdir, *String, *token, *dest, *unarc, *lname;
    FILE		*fbbs;
    DIR			*dp;
    int			Append = FALSE, Files = 0, rc, i, line = 0, pos, x, Doit;
    int			Imported = 0, Errors = 0, Present = FALSE;
    struct FILERecord   fdb;
    struct stat		statfile;
    struct dirent	*de;

    Syslog('-', "Import(%d)", Area);

    if (!do_quiet)
	colour(CYAN, BLACK);

    if (LoadAreaRec(Area) == FALSE)
	die(MBERR_INIT_ERROR);

    if (area.Available && !area.CDrom) {
        temp   = calloc(PATH_MAX, sizeof(char));
	temp2  = calloc(PATH_MAX, sizeof(char));
        pwd    = calloc(PATH_MAX, sizeof(char));
	tmpdir = calloc(PATH_MAX, sizeof(char));
	String = calloc(4096, sizeof(char));
	dest   = calloc(PATH_MAX, sizeof(char));
	lname  = calloc(PATH_MAX, sizeof(char));

        getcwd(pwd, PATH_MAX);
	if (CheckFDB(Area, area.Path))
	    die(MBERR_GENERAL);
	sprintf(tmpdir, "%s/tmp/arc", getenv("MBSE_ROOT"));

	IsDoing("Import files");

	/*
	 * Find files.bbs
	 */
	sprintf(temp, "FILES.BBS");
	if ((fbbs = fopen(temp, "r")) == NULL) {
	    sprintf(temp, "files.bbs");
	    if ((fbbs = fopen(temp, "r")) == NULL) {
		sprintf(temp, "%s", area.FilesBbs);
		if ((fbbs = fopen(temp, "r")) == NULL) {
		    WriteError("Can't find files.bbs anywhere");
		    if (!do_quiet)
			printf("Can't find files.bbs anywhere\n");
		    die(MBERR_INIT_ERROR);
		}
	    }
	}
	
	while (fgets(String, 4095, fbbs) != NULL) {

	    if ((String[0] != ' ') && (String[0] != '\t')) {
		/*
		 * New file entry, check if there has been a file that is not yet saved.
		 */
		if (Append && Present) {
		    Doit = TRUE;
		    if ((unarc = unpacker(temp)) == NULL) {
			Syslog('+', "Unknown archive format %s", temp);
			sprintf(temp2, "%s/tmp/arc/%s", getenv("MBSE_ROOT"), fdb.Name);
			mkdirs(temp2, 0755);
			if ((rc = file_cp(temp, temp2))) {
			    WriteError("Can't copy file to %s, %s", temp2, strerror(rc));
			    if (!do_quiet)
				printf("Can't copy file to %s, %s\n", temp2, strerror(rc));
			    Doit = FALSE;
			} else {
			    if (do_novir == FALSE) {
				if (!do_quiet) {
				    printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
				    fflush(stdout);
				}
				if (VirScan(tmpdir)) {
				    Doit = FALSE;
				}
			    }
			}
		    } else {
			if (!do_quiet) {
			    printf("Unpacking \b\b\b\b\b\b\b\b\b\b");
			    fflush(stdout);
			}
			if (UnpackFile(temp)) {
			    if (do_novir == FALSE) {
				if (!do_quiet) {
				    printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
				    fflush(stdout);
				}
				if (VirScan(tmpdir)) {
				    Doit = FALSE;
				}
			    }
			} else {
			    Doit = FALSE;
			}
		    }
		    DeleteVirusWork();
		    if (Doit) {
			if (!do_quiet) {
			    printf("Adding    \b\b\b\b\b\b\b\b\b\b");
			    fflush(stdout);
			}
			if (strcmp(fdb.Name, fdb.LName)) {
			    if (AddFile(fdb, Area, dest, temp, lname)) {
				Imported++;
			    } else {
				Errors++;
			    }
			} else {
			    if (AddFile(fdb, Area, dest, temp, NULL)) {
				Imported++;
			    } else {
				Errors++;
			    }
			}
		    } else {
			Errors++;
		    }
		    Append = FALSE;
		    Present = FALSE;
		    line = 0;
		    pos = 0;
		}

		/*
		 * Check diskspace
		 */
		if (!diskfree(CFG.freespace))
		    die(MBERR_DISK_FULL);

		Files++;
		memset(&fdb, 0, sizeof(fdb));
		Present = TRUE;

		token = strtok(String, " \t");

		/*
		 * Test filename against name on disk, first normal case,
		 * then lowercase and finally uppercase.
		 */
		if ((dp = opendir(pwd)) == NULL) {
		    WriteError("$Can't open directory %s", pwd);
		    die(MBERR_INIT_ERROR);
		}
		while ((de = readdir(dp))) {
		    if (strcasecmp(de->d_name, token) == 0) {
			/*
			 * Found the right file.
			 */
			strncpy(fdb.LName, token, 80);
			break;
		    }
		}
		closedir(dp);

		if (strlen(fdb.LName) == 0) {
		    WriteError("Can't find file on disk, skipping: %s\n", token);
		    Append = FALSE;
		    Present = FALSE;
		} else {
		    /*
		     * Create DOS 8.3 filename
		     */
		    strcpy(temp2, fdb.LName);
		    name_mangle(temp2);
		    strcpy(fdb.Name, temp2);

		    if (strcmp(fdb.LName, fdb.Name) && (rename(fdb.LName, fdb.Name) == 0)) {
			Syslog('+', "Renamed %s to %s", fdb.LName, fdb.Name);
		    }

		    sprintf(temp, "%s/%s", pwd, fdb.Name);
		    stat(temp, &statfile);

		    if (do_annon)
			fdb.Announced = TRUE;
		    Syslog('f', "File: %s (%s)", fdb.Name, fdb.LName);

		    if (!do_quiet) {
			printf("\rImport file: %s ", fdb.Name);
			printf("Checking  \b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }

		    IsDoing("Import %s", fdb.Name);

		    token = strtok(NULL, "\0");
		    i = strlen(token);
		    line = pos = 0;
		    for (x = 0; x < i; x++) {
			if ((token[x] == '\n') || (token[x] == '\r'))
			    token[x] = '\0';
		    }
		    i = strlen(token);

		    Doit = FALSE;
		    for (x = 0; x < i; x++) {
			if (!Doit) {
			    if (!iscntrl(token[x]) && !isblank(token[x]))
				Doit = TRUE;
			}
			if (Doit) {
			    if (pos > 42) {
				if (token[x] == ' ') {
				    fdb.Desc[line][pos] = '\0';
				    line++;
				    pos = 0;
				} else {
				    if (pos == 49) {
					fdb.Desc[line][pos] = '\0';
					pos = 0;
					line++;
				    }
				    fdb.Desc[line][pos] = token[x];
				    pos++;
				}
			    } else {
				fdb.Desc[line][pos] = token[x];
				pos++;
			    }
			    if (line == 25)
				break;
			}
		    }

		    sprintf(dest, "%s/%s", area.Path, fdb.Name);
		    sprintf(lname, "%s/%s", area.Path, fdb.LName);
		    Append = TRUE;
		    fdb.Size = statfile.st_size;
		    fdb.FileDate = statfile.st_mtime;
		    fdb.Crc32 = file_crc(temp, FALSE);
		    strcpy(fdb.Uploader, CFG.sysop_name);
		    fdb.UploadDate = time(NULL);
		}
	    } else if (Present) {
		/*
		 * Add multiple description lines
		 */
		if (line < 25) {
		    token = strtok(String, "\0");
		    i = strlen(token);
		    line++;
		    pos = 0;
		    Doit = FALSE;
		    for (x = 0; x < i; x++) {
			if ((token[x] == '\n') || (token[x] == '\r'))
			    token[x] = '\0';
		    }
		    for (x = 0; x < i; x++) {
			if (Doit) {
			    if (pos > 42) {
				if (token[x] == ' ') {
				    fdb.Desc[line][pos] = '\0';
				    line++;
				    pos = 0;
				} else {
				    if (pos == 49) {
					fdb.Desc[line][pos] = '\0';
					pos = 0;
					line++;
				    }
				    fdb.Desc[line][pos] = token[x];
				    pos++;
				}
			    } else {
				fdb.Desc[line][pos] = token[x];
				pos++;
			    }
			    if (line == 25)
				break;
			} else {
			    /*
			     * Skip until + or | is found
			     */
			    if ((token[x] == '+') || (token[x] == '|'))
				Doit = TRUE;
			}
		    }
		}
	    } /* End if new file entry found */
	} /* End of files.bbs */
	fclose(fbbs);

	/*
	 * Flush the last file to the database
	 */
	if (Append) {
	    Doit = TRUE;
	    if ((unarc = unpacker(temp)) == NULL) {
		Syslog('+', "Unknown archive format %s", temp);
		sprintf(temp2, "%s/tmp/arc/%s", getenv("MBSE_ROOT"), fdb.LName);
		mkdirs(temp2, 0755);
		if ((rc = file_cp(temp, temp2))) {
		    WriteError("Can't copy file to %s, %s", temp2, strerror(rc));
		    Doit = FALSE;
		} else {
		    if (do_novir == FALSE) {
			if (!do_quiet) {
			    printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
			    fflush(stdout);
			}
			if (VirScan(tmpdir)) {
			    Doit = FALSE;
			}
		    }
		}
	    } else {
		if (!do_quiet) {
		    printf("Unpacking \b\b\b\b\b\b\b\b\b\b");
		    fflush(stdout);
		}
		if (UnpackFile(temp)) {
		    if (do_novir == FALSE) {
			if (!do_quiet) {
			    printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
			    fflush(stdout);
			}
			if (VirScan(tmpdir)) {
			    Doit = FALSE;
			}
		    }
		} else {
		    Doit = FALSE;
		}
	    }
	    DeleteVirusWork();
	    if (Doit) {
		if (!do_quiet) {
		    printf("Adding    \b\b\b\b\b\b\b\b\b\b");
		    fflush(stdout);
		}
		if (strcmp(fdb.Name, fdb.LName)) {
		    if (AddFile(fdb, Area, dest, temp, lname))
			Imported++;
		    else
			Errors++;
		} else {
		    if (AddFile(fdb, Area, dest, temp, NULL))
			Imported++;
		    else
			Errors++;
		}
	    } else {
		Errors++;
	    }
	}

	free(lname);
	free(dest);
	free(String);
	free(pwd);
	free(temp2);
	free(temp);
	free(tmpdir);
    } else {
	if (!area.Available) {
	    WriteError("Area not available");
	    if (!do_quiet)
		printf("Area not available\n");
	}
	if (area.CDrom) {
	    WriteError("Can't import on CD-ROM");
	    if (!do_quiet)
		printf("Can't import on CD-ROM\n");
	}
    }

    if (!do_quiet) {
        printf("\r                                                              \r");
        fflush(stdout);
    }
    Syslog('+', "Import Files [%5d] Imported [%5d] Errors [%5d]", Files, Imported, Errors);
}

