/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Read *.msg messages
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
#include "msg.h"


extern int	net_msgs;


int toss_msgs(void)
{
    DIR			*dp;
    struct dirent	*de;
    int			files = 0;

    if ((dp = opendir(CFG.msgs_path)) == NULL) {
	WriteError("$Can't opendir %s", CFG.msgs_path);
	return -1;
    }

    Syslog('m', "Process *.msg in %s", CFG.msgs_path);
    IsDoing("Get *.msgs");

    while ((de = readdir(dp))) {
	if ((de->d_name[0] != '.') && (strstr(de->d_name, ".msg"))) {
	    if (IsSema((char *)"upsalarm")) {
		Syslog('+', "Detected upsalarm semafore, aborting toss");
		break;
	    }

	    Syslog('m', "Process %s", de->d_name);
	    files++;

	}
    }
    closedir(dp);

    if (files) {
	Syslog('+',"Processed %d msg messages", files);
    }

    net_msgs += files;
    return files;
}


