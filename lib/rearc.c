/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: ReArc an archive.
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
        colour(12, 0);
        printf("    ReArc file %s", filename);
	fflush(stdout);
    }

    if (strchr(filename, '/') == NULL) {
	if (!do_quiet) {
	    colour(LIGHTRED, BLACK);
	    printf(" no path in filename\n");
	}
	Syslog('+', "rearc(%s, %s), no path in filename", filename, arctype);
	return -1;
    }

    if ((unarc = unpacker(filename)) == NULL) {
	if (!do_quiet) {
	    colour(LIGHTRED, BLACK);
	    printf(" unknown archive type\n");
	}
        return -1;
    }

    if (!getarchiver(unarc)) {
	if (!do_quiet) {
	    colour(LIGHTRED, BLACK);
	    printf(" no unarchiver available\n");
	}
	Syslog('+', "rearc(%s, %s), no unarchiver available", filename, arctype);
        return -1;
    }

    uncmd = xstrcpy(archiver.funarc);
    if ((uncmd == NULL) || (uncmd == "")) {
        if (!do_quiet) {
	    colour(LIGHTRED, BLACK);
	    printf(" no unarchive command available\n");
	}
	Syslog('+', "rearc(%s, %s), no unarchive command available", filename, arctype);
	return -1;
    }

    newname = calloc(PATH_MAX, sizeof(char));
    strcpy(newname, filename);
    p = strrchr(newname, '.');
    *p++;
    *p = '\0';

    if (!getarchiver(arctype)) {
	if (!do_quiet) {
	    colour(LIGHTRED, BLACK);
	    printf(" no archiver available\n");
	}
	Syslog('+', "rearc(%s, %s), no archiver available", filename, arctype);
	free(uncmd);
	free(newname);
	return -1;
    }

    if (strcmp(unarc, archiver.name) == 0) {
	if (!do_quiet) {
	    colour(LIGHTRED, BLACK);
	    printf(" already in %s format\n", arctype);
	}
	Syslog('+', "rearc(%s, %s), already in %s format", filename, arctype, arctype);
	free(uncmd);
	free(newname);
	return -1;
    }

    sprintf(p, "%s", archiver.name);
    Syslog('f', "new filename %s", newname);

    arccmd = xstrcpy(archiver.farc);
    if ((arccmd == NULL) || (arccmd == "")) {
	if (!do_quiet) {
	    colour(LIGHTRED, BLACK);
	    printf(" no archive command available\n");
	}
	Syslog('+', "rearc(%s, %s), no archive command available", filename, arctype);
	free(uncmd);
	free(newname);
	return -1;
    }

    /*
     * unarchive and archive commands are available, create a temp directory to work in.
     */
    workpath = calloc(PATH_MAX, sizeof(char));
    oldpath = calloc(PATH_MAX, sizeof(char));
    temp = calloc(PATH_MAX, sizeof(char));
    getcwd(oldpath, PATH_MAX);
    sprintf(workpath, "%s/tmp/rearc%d", getenv("MBSE_ROOT"), getpid());
    sprintf(temp, "%s/%s", workpath, filename);
    rc = mkdirs(temp, 0755) ? 0 : -1;
    if (rc == 0) {
	if ((rc = chdir(workpath)) == -1) {
	    WriteError("$Can't chdir to %s", workpath);
	}
    }

    if (!do_quiet) {
	colour(11, 0);
	printf("\rUnpacking file %s", filename);
	fflush(stdout);
    }

    /* 
     * Unarchive
     */
    if (rc == 0) {
	if ((rc = execute_str(uncmd,filename,(char *)NULL,(char*)"/dev/null",(char*)"/dev/null",(char*)"/dev/null"))) {
	    sync();
	    sleep(1);
	    WriteError("Warning: unpack %s failed, trying again after sync()", filename);
	    if ((rc = execute_str(uncmd,filename,(char *)NULL,(char*)"/dev/null",(char*)"/dev/null",(char*)"/dev/null"))) {
		WriteError("$Can't unpack %s", filename);
	    }
	}
    }

    if (!do_quiet) {
	colour(10, 0);
	printf("\r  Packing file %s", filename);
    }

    /*
     * Archive
     */
    if (rc == 0) {
        if ((rc = execute_str(arccmd,newname,(char *)".",(char*)"/dev/null",(char*)"/dev/null",(char*)"/dev/null"))) {
	    sync();
	    sleep(1);
	    WriteError("Warning: pack %s failed, trying again after sync()", newname);
	    if ((rc = execute_str(arccmd,newname,(char *)".",(char*)"/dev/null",(char*)"/dev/null",(char*)"/dev/null"))) {
		WriteError("$Can't pack %s", newname);
	    }
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
    sprintf(temp, "-rf %s", workpath);
    execute_pth((char *)"rm", temp, (char*)"/dev/null", (char*)"/dev/null", (char*)"/dev/null");
    if (rc == 0)
	sprintf(filename, "%s", newname);

    free(workpath);
    free(oldpath);
    free(temp);
    free(uncmd);
    free(arccmd);
    free(newname);
    return rc;
}


