/*****************************************************************************
 *
 * File ..................: mbaff/fflist.c
 * Purpose ...............: Announce new files and FileFind
 * Last modification date : 27-Nov-1999
 *
 *****************************************************************************
 * Copyright (C) 1997-1999
 *   
 * Michiel Broek		FIDO:		2:2801/16
 * Beekmansbos 10		Internet:	mbroek@ux123.pttnwb.nl
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/clcomm.h"
#include "../lib/msg.h"
#include "fflist.h"


/*
 * Tidy the filefind array
 */
void tidy_fflist(ff_list ** fdp)
{
	ff_list	*tmp, *old;

	for (tmp = *fdp; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}
	*fdp = NULL;
}



/*
 * Add a search record to the array
 */
void fill_fflist(ff_list **fdp)
{
	char	*b;
	ff_list	*tmp, *ta;

	b = calloc(44, sizeof(char));
	sprintf(b, "%s~", Msg.FromAddress);

	/*
	 *  Add a new record
	 */
	tmp = (ff_list *)malloc(sizeof(ff_list));
	tmp->next = NULL;
	sprintf(tmp->from, "%s", Msg.From);
	sprintf(tmp->subject, "%s", Msg.Subject);
	if (strchr(b, '.') == NULL) {
		tmp->zone = atoi(strtok(b, ":"));
		tmp->net  = atoi(strtok(NULL, "/"));
		tmp->node  = atoi(strtok(NULL, "~"));
	} else {
		tmp->zone = atoi(strtok(b, ":"));
		tmp->net  = atoi(strtok(NULL, "/"));
		tmp->node  = atoi(strtok(NULL, "."));
		tmp->point = atoi(strtok(NULL, "~"));
	}
	sprintf(tmp->msgid, "%s", Msg.Msgid);
	tmp->msgnr = Msg.Id;
	tmp->done  = FALSE;

	/*
	 *  New record goes at the end.
	 */
	if (*fdp == NULL)
		*fdp = tmp;
	else
		for (ta = *fdp; ta; ta = ta->next)
			if (ta->next == NULL) {
				ta->next = (ff_list *)tmp;
				break;
			}

	free(b);
}



/*
 * Tidy the reply files array
 */
void tidy_rflist(rf_list ** fdp)
{
	rf_list	*tmp, *old;

	for (tmp = *fdp; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}
	*fdp = NULL;
}



/*
 * Add a reply file to the array
 */
void fill_rflist(rf_list **fdp, char *fname, unsigned long area)
{
	rf_list	*tmp, *ta;

	/*
	 *  Add a new record
	 */
	tmp = (rf_list *)malloc(sizeof(rf_list));
	tmp->next = NULL;
	sprintf(tmp->filename, "%s", fname);
	tmp->area = area;

	/*
	 *  New record goes at the end.
	 */
	if (*fdp == NULL)
		*fdp = tmp;
	else
		for (ta = *fdp; ta; ta = ta->next)
			if (ta->next == NULL) {
				ta->next = (rf_list *)tmp;
				break;
			}
}



