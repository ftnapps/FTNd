/*****************************************************************************
 *
 * $Id: magic.c,v 1.3 2007/03/03 14:28:40 mbse Exp $
 * Purpose ...............: Magic filename handling
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
 * Update magic alias with new filename.
 */
void magic_update(char *Alias, char *FileName)
{
    char    *path;
    FILE    *fp;

    if (!strlen(CFG.req_magic)) {
	WriteError("No magic filename path configured");
	return;
    }

    path = xstrcpy(CFG.req_magic);
    path = xstrcat(path, (char *)"/");
    path = xstrcat(path, Alias);

    if ((fp = fopen(path, "w")) == NULL) {
	WriteError("$Can't create %s", path);
	free(path);
	return;
    }
    fprintf(fp, "%s\n", FileName);
    fclose(fp);
    chmod(path, 0644);
    free(path);
}



/*
 * Check if magic filename is valid.
 */
int magic_check(char *Alias, char *FileName)
{
    char    *path;
    FILE    *fp;
    int	    rc = -1;

    if (!strlen(CFG.req_magic)) {
	WriteError("magic_check(): no magic filename path configured");
	return -1;
    }

    path = xstrcpy(CFG.req_magic);
    path = xstrcat(path, (char *)"/");
    path = xstrcat(path, Alias);

    if ((fp = fopen(path, "r")) == NULL) {
	WriteError("$No magic alias %s", path);
	free(path);
	return -1;
    }
    free(path);

    path = calloc(PATH_MAX, sizeof(char));
    fgets(path, PATH_MAX -1, fp);
    fclose(fp);
    Striplf(path);
    if (strcmp(path, FileName) == 0)
	rc = 0;
    free(path);

    return rc;
}


