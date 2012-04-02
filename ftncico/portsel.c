/*****************************************************************************
 *
 * $Id: portsel.c,v 1.8 2005/08/28 11:34:24 mbse Exp $
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "../lib/mbselib.h"
#include "portsel.h"


extern char *name, *phone, *flags;


int load_port(char *tty)
{
	char	*temp;
	FILE	*fp;

	temp = calloc(PATH_MAX, sizeof(char));
	snprintf(temp, PATH_MAX -1, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));

	if ((fp = fopen(temp, "r")) == NULL) {
		WriteError("$Can't open %s", temp);
		free(temp);
		return FALSE;
	}

	free(temp);
	fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, fp);

	while (fread(&ttyinfo, ttyinfohdr.recsize, 1, fp) == 1) {
		if ((strcmp(ttyinfo.tty, tty) == 0) && (ttyinfo.available)) {
			fclose(fp);

			/*
			 * Override EMSI parameters.
			 */
			if (strlen(ttyinfo.phone)) {
				free(phone);
				phone = xstrcpy(ttyinfo.phone);
			}
			if (strlen(ttyinfo.flags)) {
				free(flags);
				flags = xstrcpy(ttyinfo.flags);
			}
			if (strlen(ttyinfo.name)) {
				free(name);
				name = xstrcpy(ttyinfo.name);
			}

			if ((ttyinfo.type == POTS) || (ttyinfo.type == ISDN))
				return load_modem(ttyinfo.modem);
			else
				return TRUE;
		}
	}

	fclose(fp);
	memset(&ttyinfo, 0, sizeof(ttyinfo));
	return FALSE;
}



int load_modem(char *ModemName)
{
	char		*temp;
	FILE		*fp;

	temp = calloc(PATH_MAX, sizeof(char));
	snprintf(temp, PATH_MAX -1, "%s/etc/modem.data", getenv("MBSE_ROOT"));

	if ((fp = fopen(temp, "r")) == NULL) {
		WriteError("$Can't open %s", temp);
		free(temp);
		return FALSE;
	}

	free(temp);
	fread(&modemhdr, sizeof(modemhdr), 1, fp);

	while (fread(&modem, modemhdr.recsize, 1, fp) == 1) {
		if ((strcmp(modem.modem, ModemName) == 0) && (modem.available)) {
			fclose(fp);
			return TRUE;
		}
	}

	fclose(fp);
	memset(&modem, 0, sizeof(modem));
	return FALSE;
}


