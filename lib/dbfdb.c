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



/*
 *  Open files database Area number, with path. Do some checks and abort
 *  if they fail.
 */
struct _fdbarea *mbsedb_OpenFDB(long Area, int Timeout)
{
    char	    *temp;
    struct _fdbarea *fdb_area = NULL;
    int		    Tries = 0;
    FILE	    *fp;

    Syslog('f', "OpenFDB area %ld, timeout %d", Area, Timeout);

    temp = calloc(PATH_MAX, sizeof(char));
    fdb_area = malloc(sizeof(struct _fdbarea));	    /* Will be freed by CloseFDB */

    sprintf(temp, "%s/fdb/file%ld.data", getenv("MBSE_ROOT"), Area);

    /*
     * Open the file database, if it's locked, just wait.
     */
    while (((fp = fopen(temp, "r+")) == NULL) && ((errno == EACCES) || (errno == EAGAIN))) {
	if (++Tries >= (Timeout * 4)) {
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
    free(temp);

    if ((fdbhdr.hdrsize != sizeof(fdbhdr)) || (fdbhdr.recsize != sizeof(fdb))) {
        WriteError("Files database header in area %d is corrupt (%d:%d)", Area, fdbhdr.hdrsize, fdbhdr.recsize);
	fclose(fdb_area->fp);
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
    fdb_area->fp = fp;
    fdb_area->locked = 0;
    fdb_area->area = Area;
    Syslog('f', "OpenFDB success");
    return fdb_area;
}



/*
 *  Close current open file area
 */
int mbsedb_CloseFDB(struct _fdbarea *fdb_area)
{
    Syslog('f', "CloseFDB %ld", fdb_area->area);
    if (fdb_area->locked) {
	/*
	 * Unlock first
	 */
	mbsedb_UnlockFDB(fdb_area);
    }
    fclose(fdb_area->fp);

    free(fdb_area);
    return TRUE;
}



int mbsedb_LockFDB(struct _fdbarea *fdb_area, int Timeout)
{
    int		    rc, Tries = 0;
    struct flock    fl;
    
    Syslog('f', "LockFDB %ld", fdb_area->area);
    fl.l_type	= F_WRLCK;
    fl.l_whence	= SEEK_SET;
    fl.l_start	= 0L;
    fl.l_len	= 1L;
    fl.l_pid	= getpid();

    while ((rc = fcntl(fileno(fdb_area->fp), F_SETLK, &fl)) && ((errno == EACCES) || (errno == EAGAIN))) {
	if (++Tries >= (Timeout * 4)) {
	    fcntl(fileno(fdb_area->fp), F_GETLK, &fl);
	    WriteError("FDB %ld is locked by pid %d", fdb_area->area, fl.l_pid);
	    return FALSE;
	}
	msleep(250);
	Syslog('f', "FDB lock attempt %d", Tries);
    }

    if (rc) {
	WriteError("$FDB %ld lock error", fdb_area->area);
	return FALSE;
    }

    Syslog('f', "LockFDB success");
    fdb_area->locked = 1;
    return TRUE;
}



int mbsedb_UnlockFDB(struct _fdbarea *fdb_area)
{
    struct flock    fl;

    Syslog('f', "UnlockFDB %ld", fdb_area->area);
    fl.l_type   = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0L;
    fl.l_len    = 1L;
    fl.l_pid    = getpid();

    if (fcntl(fileno(fdb_area->fp), F_SETLK, &fl)) {
	WriteError("$Can't unlock FDB area %ld", fdb_area->area);
    }

    fdb_area->locked = 0;
    return TRUE;
}


// mbsedb_SearchFileFDB
// mbsedb_UpdateFileFDB
// mbsedb_InsertFileFDB
// mbsedb_DeleteFileFDB
// mbsedb_SortFDB


#endif
