/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Scan for virusses
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "virscan.h"


int VirScan(void)
{
    char    *temp, *cmd = NULL;
    FILE    *fp;
    int	    rc = FALSE;

    temp = calloc(PATH_MAX, sizeof(char));
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
				(char *)"/dev/null" , (char *)"/dev/null") != virscan.error) {
                    Syslog('!', "Virus found by %s", virscan.comment);
		    rc = TRUE;
                }
                free(cmd);
		Nopper();
            }
        }
        fclose(fp);
    }

    return rc;
}


