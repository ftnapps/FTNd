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


#ifdef	USE_EXPERIMENT

int	fdbislocked = 0;		    /* Should database be locked	*/
long	currentarea = -1;		    /* Current file area		*/


/*
 *  Open files database Area number, with path. Do some checks and abort
 *  if they fail.
 */
FILE *mbsedb_OpenFDB(long Area, char *Path, int ulTimeout)
{
    char    *temp;
    FILE    *fp;
    int     Tries = 0;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/fdb/file%ld.data", getenv("MBSE_ROOT"), Area);

    /*
     * Open the file database, if it's locked, just wait.
     */
    while (((fp = fopen(temp, "r+")) == NULL) && ((errno == EACCES) || (errno == EAGAIN))) {
	if (++Tries >= (ulTimeout * 4)) {
	    WriteError("Can't open file area %ld, timeout", Area);
	    free(temp);
	    return NULL;
	}
	msleep(250);
	Syslog('f', "Open file area %ld, try %d", Area, Tries);
    }
    if (fp == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return NULL;
    }
    fread(&fdbhdr, sizeof(fdbhdr), 1, fp);

    /*
     * Fix attributes if needed
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
        WriteError("Files database header in area %d is corrupt (%d:%d)", Area, fdbhdr.hdrsize, fdbhdr.recsize);
	fclose(fp);
	return NULL;
    }

    fseek(fp, 0, SEEK_END);
    if ((ftell(fp) - fdbhdr.hdrsize) % fdbhdr.recsize) {
	WriteError("Files database area %ld is corrupt, unalligned records", Area);
	fclose(fp);
	return NULL;
    }

    /*
     * Point to the first record
     */
    fseek(fp, fdbhdr.hdrsize, SEEK_SET);
    fdbislocked = 0;
    currentarea = Area;
    return fp;
}



/*
 *  Close current open file area
 */
int mbsedb_CloseFDB(FILE *fp)
{
    if (currentarea == -1) {
	WriteError("Can't close already closed file area");
	return FALSE;
    }

    if (fdbislocked) {
	/*
	 * Unlock first
	 */
    }
    currentarea = -1;
    fclose(fp);
    return TRUE;
}


// mbsedb_LockFDB
// mbsedb_UnlockFDB
// mbsedb_SearchFileFDB
// mbsedb_UpdateFileFDB
// mbsedb_InsertFileFDB
// mbsedb_DeleteFileFDB
// mbsedb_SortFDB


#endif
