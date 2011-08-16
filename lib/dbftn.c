/*****************************************************************************
 *
 * $Id: dbftn.c,v 1.8 2005/08/28 10:03:17 mbse Exp $
 * Purpose ...............: Fidonetrecord Access
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "mbselib.h"
#include "users.h"
#include "mbsedb.h"




int InitFidonet(void)
{
	FILE	*fil;

	memset(&fidonet, 0, sizeof(fidonet));
	LoadConfig();

	snprintf(fidonet_fil, PATH_MAX -1, "%s/etc/fidonet.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(fidonet_fil, "r")) == NULL)
		return FALSE;

	fread(&fidonethdr, sizeof(fidonethdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	fidonet_cnt = (ftell(fil) - fidonethdr.hdrsize) / fidonethdr.recsize;
	fclose(fil);

	return TRUE;
}



int TestFidonet(unsigned short zone)
{
	int	i, ftnok = FALSE;

	for (i = 0; i < 6; i++) {
		if (zone == fidonet.zone[i])
			ftnok = TRUE;
	}
	return(ftnok);
}



int SearchFidonet(unsigned short zone)
{
	FILE	*fil;

	/*
	 * If current record is ok, return immediatly.
	 */
	if (TestFidonet(zone))
		return TRUE;

	if ((fil = fopen(fidonet_fil, "r")) == NULL) {
		return FALSE;
	}
	fread(&fidonethdr, sizeof(fidonethdr), 1, fil);

	while (fread(&fidonet, fidonethdr.recsize, 1, fil) == 1) {
		if (TestFidonet(zone)) {
			fclose(fil);
			return TRUE;
		}
	}
	fclose(fil);
	return FALSE;
}



char *GetFidoDomain(unsigned short zone)
{
    static char domain[9];

    memset(&domain, 0, sizeof(domain));

    if (SearchFidonet(zone) == FALSE)
	return NULL;

    strncpy(domain, fidonet.domain, 8);
    return domain;
}


