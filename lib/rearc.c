/*****************************************************************************
 *
 * $Id: rearc.c,v 1.7 2008/11/26 21:40:54 mbse Exp $
 * Purpose ...............: ReArc an archive.
 *
 *****************************************************************************
 * Copyright (C) 1997-2008
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


/*
 * Rearchive an archive, if successfull the filename is updated.
 * If it fails, return -1.
 */
int rearc(char *filename, char *arctype, int do_quiet)
{
    char        *p, *uncmd = NULL, *arccmd = NULL, *unarc, *newname, *workpath, *oldpath, *temp;
    int         rc = 0;

    Syslog('f', "rearc(%s, %s)", filename, arctype);

    if (!do_quiet) {
        mbse_colour(LIGHTRED, BLACK);
        printf("    ReArc file %s   ", filename);
	fflush(stdout);
    }

    if (strchr(filename, '/') == NULL) {
	if (!do_quiet) {
	    mbse_colour(LIGHTRED, BLACK);
	    printf(" no path in filename\n");
	}
	Syslog('+', "rearc(%s, %s), no path in filename", filename, arctype);
	return -1;
    }

    if ((unarc = unpacker(filename)) == NULL) {
	if (!do_quiet) {
	    mbse_colour(LIGHTRED, BLACK);
	    printf(" unknown archive type\n");
	}
        return -1;
    }

    if (!getarchiver(unarc)) {
	if (!do_quiet) {
	    mbse_colour(LIGHTRED, BLACK);
	    printf(" no unarchiver available\n");
	}
	Syslog('+', "rearc(%s, %s), no unarchiver available", filename, arctype);
        return -1;
    }

    if (strlen(archiver.funarc) == 0) {
        if (!do_quiet) {
	    mbse_colour(LIGHTRED, BLACK);
	    printf(" no unarchive command available\n");
	}
	Syslog('+', "rearc(%s, %s), no unarchive command available", filename, arctype);
	return -1;
    }
    uncmd = xstrcpy(archiver.funarc);

    newname = calloc(PATH_MAX, sizeof(char));
    strcpy(newname, filename);
    p = strrchr(newname, '.');
    *p++;
    *p = '\0';

    if (!getarchiver(arctype)) {
	if (!do_quiet) {
	    mbse_colour(LIGHTRED, BLACK);
	    printf(" no archiver available\n");
	}
	Syslog('+', "rearc(%s, %s), no archiver available", filename, arctype);
	free(uncmd);
	free(newname);
	return -1;
    }

    if (strcmp(unarc, archiver.name) == 0) {
	if (!do_quiet) {
	    mbse_colour(LIGHTRED, BLACK);
	    printf(" already in %s format\n", arctype);
	}
	Syslog('+', "rearc(%s, %s), already in %s format", filename, arctype, arctype);
	free(uncmd);
	free(newname);
	return -1;
    }

    snprintf(p, 6, "%s", archiver.name);
    Syslog('f', "new filename %s", newname);

    if (strlen(archiver.farc) == 0) {
	if (!do_quiet) {
	    mbse_colour(LIGHTRED, BLACK);
	    printf(" no archive command available\n");
	}
	Syslog('+', "rearc(%s, %s), no archive command available", filename, arctype);
	free(uncmd);
	free(newname);
	return -1;
    }
    arccmd = xstrcpy(archiver.farc);

    /*
     * unarchive and archive commands are available, create a temp directory to work in.
     */
    workpath = calloc(PATH_MAX, sizeof(char));
    oldpath = calloc(PATH_MAX, sizeof(char));
    temp = calloc(PATH_MAX, sizeof(char));
    getcwd(oldpath, PATH_MAX);
    snprintf(workpath, PATH_MAX -1, "%s/tmp/rearc%d", getenv("MBSE_ROOT"), getpid());
    snprintf(temp, PATH_MAX -1, "%s/%s", workpath, filename);
    rc = mkdirs(temp, 0755) ? 0 : -1;
    if (rc == 0) {
	if ((rc = chdir(workpath)) == -1) {
	    WriteError("$Can't chdir to %s", workpath);
	}
    }

    if (!do_quiet) {
	mbse_colour(LIGHTCYAN, BLACK);
	printf("\rUnpacking file %s   ", filename);
	fflush(stdout);
    }

    /* 
     * Unarchive
     */
    if (rc == 0) {
	if ((rc = execute_str(uncmd,filename,(char *)NULL,(char*)"/dev/null",(char*)"/dev/null",(char*)"/dev/null"))) {
	    WriteError("$Can't unpack %s", filename);
	}
    }

    if (!do_quiet) {
	mbse_colour(LIGHTGREEN, BLACK);
	printf("\r  Packing file %s   ", newname);
    }

    /*
     * Archive
     */
    if (rc == 0) {
        if ((rc = execute_str(arccmd,newname,(char *)".",(char*)"/dev/null",(char*)"/dev/null",(char*)"/dev/null"))) {
	    WriteError("$Can't pack %s", newname);
	}
    }

    if (rc == 0)
	unlink(filename);

    if ((rc = chdir(oldpath)) == -1) {
	WriteError("$Can't chdir to %s", oldpath);
    }

    /*
     * Clean and remove workdir
     */
    snprintf(temp, PATH_MAX -1, "-rf %s", workpath);
    execute_pth((char *)"rm", temp, (char*)"/dev/null", (char*)"/dev/null", (char*)"/dev/null");
    if (rc == 0)
	snprintf(filename, PATH_MAX -1, "%s", newname);

    free(workpath);
    free(oldpath);
    free(temp);
    free(uncmd);
    free(arccmd);
    free(newname);
    return rc;
}


