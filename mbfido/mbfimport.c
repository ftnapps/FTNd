/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Import files with files.bbs
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "virscan.h"
#include "mbfutil.h"
#include "mbfimport.h"



extern int	do_quiet;		/* Supress screen output	    */


void ImportFiles(int Area)
{
    char		*sAreas, *pwd, *temp, *temp2, *String, *token, *dest, *unarc;
    FILE		*pAreas, *fbbs;
    int			Append = FALSE, Files = 0, i, j = 0, k = 0, x, Doit;
    int			Imported = 0, Errors = 0;
    struct FILERecord   fdb;
    struct stat		statfile;

    Syslog('-', "Import(%d)", Area);

    if (!do_quiet)
	colour(CYAN, BLACK);

    sAreas = calloc(PATH_MAX, sizeof(char));

    sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("$Can't open %s", sAreas);
	if (!do_quiet)
	    printf("Can't open %s\n", sAreas);
	die(0);
    }

    fread(&areahdr, sizeof(areahdr), 1, pAreas);
    if (fseek(pAreas, ((Area - 1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET)) {
        WriteError("$Can't seek record %d in %s", Area, sAreas);
        if (!do_quiet)
            printf("Can't seek record %d in %s\n", Area, sAreas);
        fclose(pAreas);
        free(sAreas);
        die(0);
    }

    if (fread(&area, areahdr.recsize, 1, pAreas) != 1) {
        WriteError("$Can't read record %d in %s", Area, sAreas);
        if (!do_quiet)
            printf("Can't read record %d in %s\n", Area, sAreas);
        fclose(pAreas);
        free(sAreas);
        die(0);
    }

    fclose(pAreas);
    free(sAreas);

    if (area.Available && !area.CDrom) {
        temp   = calloc(PATH_MAX, sizeof(char));
	temp2  = calloc(PATH_MAX, sizeof(char));
        pwd    = calloc(PATH_MAX, sizeof(char));
	String = calloc(256, sizeof(char));
	dest   = calloc(PATH_MAX, sizeof(char));

        getcwd(pwd, PATH_MAX);
	if (CheckFDB(Area, area.Path))
	    die(0);

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
		    die(0);
		}
	    }
	}
	
	while (fgets(String, 255, fbbs) != NULL) {

	    if ((String[0] != ' ') && (String[0] != 't')) {
		/*
		 * New file entry, check if there has been a file that is not yet saved.
		 */
		if (Append) {
		    Doit = TRUE;
		    if ((unarc = unpacker(temp)) == NULL) {
			Syslog('+', "Unknown archive format %s", temp);
			sprintf(temp2, "%s/tmp/arc/%s", getenv("MBSE_ROOT"), fdb.LName);
			mkdirs(temp2);
			if (file_cp(temp, temp2)) {
			    WriteError("Can't copy file to %s", temp2);
			    Doit = FALSE;
			} else {
			    if (!do_quiet) {
				printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
				fflush(stdout);
			    }
			    if (VirScan()) {
				Doit = FALSE;
			    }
			}
		    } else {
			if (!do_quiet) {
			    printf("Unpacking \b\b\b\b\b\b\b\b\b\b");
			    fflush(stdout);
			}
			if (UnpackFile(temp)) {
			    if (!do_quiet) {
				printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
				fflush(stdout);
			    }
			    if (VirScan()) {
				Doit = FALSE;
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
			if (AddFile(fdb, Area, dest, temp)) {
			    Imported++;
			} else
			    Errors++;
		    } else {
			Errors++;
		    }
		    Append = FALSE;
		}

		/*
		 * Check diskspace
		 */
		if (!diskfree(CFG.freespace))
		    die(101);

		Files++;
		memset(&fdb, 0, sizeof(fdb));

		token = strtok(String, " \t");
		strcpy(fdb.Name, token);
		strcpy(fdb.LName, tl(token));
		Syslog('f', "File: %s (%s)", fdb.Name, fdb.LName);

		if (!do_quiet) {
		    printf("\rImport file: %s ", fdb.Name);
		    printf("Checking  \b\b\b\b\b\b\b\b\b\b");
		    fflush(stdout);
		}

		IsDoing("Import %s", fdb.Name);

		token = strtok(NULL, "\0");
		i = strlen(token);
		j = k = 0;
		for (x = 0; x < i; x++) {
		    if ((token[x] == '\n') || (token[x] == '\r'))
			token[x] = '\0';
		}

		Doit = FALSE;
		for (x = 0; x < i; x++) {
		    if (!Doit) {
			if (isalnum(token[x]))
			    Doit = TRUE;
		    }
		    if (Doit) {
			if (k > 42) {
			    if (token[x] == ' ') {
				fdb.Desc[j][k] = '\0';
				j++;
				k = 0;
			    } else {
				if (k == 49) {
				    fdb.Desc[j][k] = '\0';
				    k = 0;
				    j++;
				}
				fdb.Desc[j][k] = token[x];
				k++;
			    }
			} else {
			    fdb.Desc[j][k] = token[x];
			    k++;
			}
		    }
		}

		sprintf(temp,"%s/%s", pwd, fdb.LName);
		if (stat(temp,&statfile) != 0) {
		    sprintf(temp,"%s/%s", pwd, fdb.Name);
		    if (stat(temp,&statfile) != 0) {
			WriteError("Cannot locate file on disk! Skipping... -> %s\n",temp);
			Append = FALSE;
		    }
		}
		sprintf(dest, "%s/%s", area.Path, fdb.LName);
		Append = TRUE;
		fdb.Size = statfile.st_size;
		fdb.FileDate = statfile.st_mtime;
		fdb.Crc32 = file_crc(temp, FALSE);
		strcpy(fdb.Uploader, CFG.sysop_name);
		fdb.UploadDate = time(NULL);
	    } else {
		/*
		 * Add multiple description lines
		 */
		token = strtok(String, "\0");
		i = strlen(token);
		j++;
		k = 0;
		Doit = FALSE;
		for (x = 0; x < i; x++) {
		    if ((token[x] == '\n') || (token[x] == '\r'))
			token[x] = '\0';
		}
		for (x = 0; x < i; x++) {
		    if (Doit) {
			if (k > 42) {
			    if (token[x] == ' ') {
				fdb.Desc[j][k] = '\0';
				j++;
				k = 0;
			    } else {
				if (k == 49) {
				    fdb.Desc[j][k] = '\0';
				    k = 0;
				    j++;
				}
				fdb.Desc[j][k] = token[x];
				k++;
			    }
			} else {
			    fdb.Desc[j][k] = token[x];
			    k++;
			}
		    } else {
			/*
			 * Skip until + or | is found
			 */
			if ((token[x] == '+') || (token[x] == '|'))
			    Doit = TRUE;
		    }
		}
	    }
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
		mkdirs(temp2);
		if (file_cp(temp, temp2)) {
		    WriteError("Can't copy file to %s", temp2);
		    Doit = FALSE;
		} else {
		    if (!do_quiet) {
			printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }
		    if (VirScan()) {
			Doit = FALSE;
		    }
		}
	    } else {
		if (!do_quiet) {
		    printf("Unpacking \b\b\b\b\b\b\b\b\b\b");
		    fflush(stdout);
		}
		if (UnpackFile(temp)) {
		    if (!do_quiet) {
			printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		    }
		    if (VirScan()) {
			Doit = FALSE;
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
		if (AddFile(fdb, Area, dest, temp))
		    Imported++;
		else
		    Errors++;
	    } else {
		Errors++;
	    }
	}

	free(dest);
	free(String);
	free(pwd);
	free(temp2);
	free(temp);
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

