/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Adopt file
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
#include "mbfutil.h"
#include "mbflist.h"



extern int	do_quiet;		/* Supress screen output	    */



void AdoptFile(int Area, char *File, char *Description)
{
    FILE    *pAreas, *pFile, *fp;
    char    *sAreas, *fAreas, *temp, *temp2, *unarc, *cmd, *pwd;
    int	    IsArchive = FALSE, MustRearc = FALSE, UnPacked = FALSE;
    int	    IsVirus = FALSE, File_Id = FALSE;

    Syslog('-', "Adopt(%d, %s, %s)", Area, MBSE_SS(File), MBSE_SS(Description));

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

    if (area.Available) {
	fAreas = calloc(PATH_MAX, sizeof(char));
	temp   = calloc(PATH_MAX, sizeof(char));
	pwd    = calloc(PATH_MAX, sizeof(char));

	sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), Area);
	getcwd(pwd, PATH_MAX);
	
	/*
	 * Open the file database, create new one if it doesn't excist.
	 */
	if ((pFile = fopen(fAreas, "r+")) == NULL) {
	    Syslog('!', "Creating new %s", fAreas);
	    if ((pFile = fopen(fAreas, "a+")) == NULL) {
		WriteError("$Can't create %s", fAreas);
		die(0);
	    }
	}

	if (!do_quiet) {
	    printf("Adopt file: %s ", File);
	    printf("Unpacking \b\b\b\b\b\b\b\b\b\b");
	    fflush(stdout);
	}

	if ((unarc = unpacker(File)) == NULL)
	    Syslog('+', "Unknown archive format %s", File);
	else {
	    IsArchive = TRUE;
	    if (strlen(area.Archiver) && (strcmp(unarc, area.Archiver) == 0))
		MustRearc = TRUE;
	}

	if (IsArchive) {
	    /*
	     * Check if there is a temp directory to unpack the archive.
	     */
	    sprintf(temp, "%s/tmp/arc", getenv("MBSE_ROOT"));
	    if ((access(temp, R_OK)) != 0) {
		if (mkdir(temp, 0777)) {
		    WriteError("$Can't create %s", temp);
		    if (!do_quiet)
			printf("Can't create %s\n", temp);
		    die(0);
		}
	    }

	    /*
	     * Check for stale FILE_ID.DIZ files
	     */
	    sprintf(temp, "%s/tmp/arc/FILE_ID.DIZ", getenv("MBSE_ROOT"));
	    if (!unlink(temp))
		Syslog('+', "Removed stale %s", temp);
	    sprintf(temp, "%s/tmp/arc/file_id.diz", getenv("MBSE_ROOT"));
	    if (!unlink(temp))
		Syslog('+', "Removed stale %s", temp);

	    if (!getarchiver(unarc)) {
		WriteError("No archiver available for %s", File);
		if (!do_quiet)
		    printf("No archiver available for %s\n", File);
		    die(0);
	    }

	    cmd = xstrcpy(archiver.funarc);
	    if ((cmd == NULL) || (cmd == "")) {
		WriteError("No unarc command available");
		if (!do_quiet)
		    printf("No unarc command available\n");
		    die(0);
	    }

	    sprintf(temp, "%s/tmp/arc", getenv("MBSE_ROOT"));
	    if (chdir(temp) != 0) {
		WriteError("$Can't change to %s", temp);
		die(0);
	    }

	    sprintf(temp, "%s/%s", pwd, File);
	    if (execute(cmd, temp,  (char *)NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null") == 0) {
		UnPacked = TRUE;
	    } else {
		chdir(pwd);
		WriteError("Unpack error, file may be corrupt");
		DeleteVirusWork();
		die(0);
	    }

            if (!do_quiet) {
                printf("Virscan   \b\b\b\b\b\b\b\b\b\b");
                fflush(stdout);
            }
            sprintf(temp, "%s/etc/virscan.data", getenv("MBSE_ROOT"));

            if ((fp = fopen(temp, "r")) == NULL) {
                WriteError("No virus scanners defined");
            } else {
                fread(&virscanhdr, sizeof(virscanhdr), 1, fp);

                while (fread(&virscan, virscanhdr.recsize, 1, fp) == 1) {
                    cmd = NULL;
                    if (virscan.available) {
                        cmd = xstrcpy(virscan.scanner);
                        cmd = xstrcat(cmd, (char *)" ");
                        cmd = xstrcat(cmd, virscan.options);
                        if (execute(cmd, (char *)"*", (char *)NULL, (char *)"/dev/null", 
					(char *)"/dev/null", (char *)"/dev/null") != virscan.error) {
			    WriteError("Virus found by %s", virscan.comment);
			    IsVirus = TRUE;
                        }
                        free(cmd);
                    }
                }
                fclose(fp);

                if (IsVirus) {
                    DeleteVirusWork();
                    chdir(pwd);
		    WriteError("Virus found");
		    if (!do_quiet)
			printf("Virus found\n");
		    die(0);
                }
            }

            if (!do_quiet) {
                printf("Checking  \b\b\b\b\b\b\b\b\b\b");
                fflush(stdout);
            }

	    temp2 = calloc(PATH_MAX, sizeof(char));
            sprintf(temp, "%s/tmp/arc/FILE_ID.DIZ", getenv("MBSE_ROOT"));
            sprintf(temp2, "%s/tmp/FILE_ID.DIZ", getenv("MBSE_ROOT"));
            if (file_cp(temp, temp2) == 0) {
                File_Id = TRUE;
	    } else {
		sprintf(temp, "%s/tmp/arc/file_id.diz", getenv("MBSE_ROOT"));
		if (file_cp(temp, temp2) == 0)
		    File_Id = TRUE;
	    }
	    free(temp2);
	    if (File_Id)
		Syslog('-', "FILE_ID.DIZ found");


	    DeleteVirusWork();
	    fclose(pFile);
	}

	free(pwd);
	free(temp);
	free(fAreas);
    } else {
	WriteError("Area %d is not available", Area);
	if (!do_quiet)
	    printf("Area %d is not available\n", Area);
    }

    fclose(pAreas);
    if (!do_quiet) {
	printf("\r                                                              \r");
	fflush(stdout);
    }

    free(sAreas);
}

