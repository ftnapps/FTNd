/*****************************************************************************
 *
 * $id$
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "portsel.h"


extern char *name, *phone, *flags;


/*
 *  Tidy the portlist array
 */
void tidy_pplist(pp_list ** fdp)
{
	pp_list	*tmp, *old;
	int	i;

	Syslog('D', "tidy_pplist");

	for (tmp = *fdp; tmp; tmp = old) {
		old = tmp->next;
		for (i = 0; i < MAXUFLAGS; i++)
			if (tmp->uflags[i])
				free(tmp->uflags[i]);
		free(tmp);
	}
	*fdp = NULL;
}



/*
 *  Add a port to the array
 */
void fill_pplist(pp_list **fdp, pp_list *new)
{
	pp_list	*tmp, *ta;
	int	i;

	tmp = (pp_list *)malloc(sizeof(pp_list));
	tmp->next = NULL;
	sprintf(tmp->tty, "%s", new->tty);
	tmp->mflags = new->mflags;
	tmp->dflags = new->dflags;
	tmp->iflags = new->iflags;
	moflags(new->mflags);
	diflags(new->dflags);
	ipflags(new->iflags);
	tmp->match = TRUE;
	for (i = 0; i < MAXUFLAGS; i++)
		tmp->uflags[i] = xstrcpy(new->uflags[i]);

	if (*fdp == NULL)
		*fdp = tmp;
	else {
		for (ta = *fdp; ta; ta = ta->next)
			if (ta->next == NULL) {
				ta->next = (pp_list *)tmp;
				break;
			}
	}
}



/*
 *  Make a list of available ports to use for dialout. The ports
 *  are selected on the available modem flags and the remote node
 *  modem type.
 */
int make_portlist(node *nlent, pp_list **tmp)
{
	char		*temp, *p, *q;
	FILE		*fp;
	int		count = 0, j, ixflag, stdflag;
	pp_list		new;

	tidy_pplist(tmp);
	temp = calloc(PATH_MAX, sizeof(char));

	sprintf(temp, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r")) == NULL) {
		WriteError("$Can't open %s", temp);
		free(temp);
		return 0;
	}

	Syslog('d', "Building portlist...");
	fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, fp);

	while (fread(&ttyinfo, ttyinfohdr.recsize, 1, fp) == 1) {

		if (((ttyinfo.type == POTS) || (ttyinfo.type == ISDN)) && (ttyinfo.available) && (ttyinfo.callout)) {
			memset(&new, 0, sizeof(new));
			sprintf(new.tty, "%s", ttyinfo.tty);
			stdflag = TRUE;
			ixflag = 0;
			q = xstrcpy(ttyinfo.flags);
			for (p = q; p; p = q) {
				if ((q = strchr(p, ',')))
					*q++ = '\0';
				if ((strncasecmp(p, "U", 1) == 0) && (strlen(p) == 1)) {
					stdflag = FALSE;
				} else {
					for (j = 0; fkey[j].key; j++)
						if (strcasecmp(p, fkey[j].key) == 0)
							new.mflags |= fkey[j].flag;
					for (j = 0; dkey[j].key; j++)
						if (strcasecmp(p, dkey[j].key) == 0)
							new.dflags |= dkey[j].flag;
					if (!stdflag) {
						if (ixflag < MAXUFLAGS) {
							new.uflags[ixflag++] = p;
						}
					}
				}
			}
			Syslog('d', "flags: nodelist     port %s", new.tty);
			Syslog('d', "modem  %08lx %08lx", nlent->mflags, new.mflags);
			Syslog('d', "ISDN   %08lx %08lx", nlent->dflags, new.dflags);

			if ((nlent->mflags & new.mflags) || (nlent->dflags & new.dflags)) {
				fill_pplist(tmp, &new);
				count++;
				Syslog('d', "make_portlist add %s", ttyinfo.tty);
			}
		}
	}

	fclose(fp);
	free(temp);

	/* FIXME: sort ports on priority and remove ports with low speed. */

	Syslog('d', "make_portlist %d ports", count);
	return count;
}



int load_port(char *tty)
{
	char	*temp;
	FILE	*fp;

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));

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
	sprintf(temp, "%s/etc/modem.data", getenv("MBSE_ROOT"));

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


