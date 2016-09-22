/*****************************************************************************
 *
 * tmpwork.c
 * Purpose ...............: temp workdirectory
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "ftndlib.h"


static int  is_tmpwork = FALSE;


void clean_tmpwork(void)
{
    char    *buf, *temp, *arc;
    
    if (is_tmpwork) {
	buf  = calloc(PATH_MAX, sizeof(char));
	temp = calloc(PATH_MAX, sizeof(char));
	arc  = calloc(PATH_MAX, sizeof(char));
	getcwd(buf, PATH_MAX);
	snprintf(temp, PATH_MAX, "%s/tmp", getenv("FTND_ROOT"));
	snprintf(arc,  PATH_MAX, "-r -f arc%d", (int)getpid());
	
	if (chdir(temp) == 0) {
	    Syslog('f', "clean_tmpwork %s/arc%d", temp, (int)getpid());
	    execute_pth((char *)"rm", arc, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
	} else {
	    WriteError("$Can't chdir to %s", temp);
	}

	chdir(buf);
	free(temp);
	free(buf);
	free(arc);
	is_tmpwork = FALSE;
    }
}


int create_tmpwork(void)
{
    char    *temp;
    int	    rc = 0;

    if (! is_tmpwork) {
	temp = calloc(PATH_MAX, sizeof(char));
	snprintf(temp, PATH_MAX, "%s/tmp/arc%d/foobar", getenv("FTND_ROOT"), (int)getpid());

	if (! mkdirs(temp, 0755))
	    rc = 1;
	else
	    is_tmpwork = TRUE;

	Syslog('f', "create_tmpwork %s rc=%d", temp, rc);
	free(temp);
    }

    return rc;
}


