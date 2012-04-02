/*****************************************************************************
 *
 * $Id: grlist.c,v 1.6 2005/08/28 14:52:14 mbse Exp $
 * Purpose ...............: Announce new files and FileFind
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "grlist.h"




/*
 * Tidy the groupnames array
 */
void tidy_grlist(gr_list ** fdp)
{
	gr_list	*tmp, *old;

	for (tmp = *fdp; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}
	*fdp = NULL;
}



/*
 * Add a group to the array
 */
void fill_grlist(gr_list **fdp, char *groupname, char *echoname)
{
	gr_list	*tmp;

	/*
	 *  Count files in group if the group already exists.
	 */
	if (*fdp != NULL) {
		for (tmp = *fdp; tmp; tmp = tmp->next)
			if ((strcmp(groupname, tmp->group) == 0) &&
			    (strcmp(echoname,  tmp->echo) == 0)) {
				tmp->count++;
				return;
			}
	}

	/*
	 *  Add a new group
	 */
	tmp = (gr_list *)malloc(sizeof(gr_list));
	tmp->next = *fdp;
	snprintf(tmp->group, 13, "%s", groupname);
	snprintf(tmp->echo,  21, "%s", echoname);
	tmp->count = 1;
	*fdp = tmp;
}



int compgroup(gr_list **, gr_list **);

/*
 *  Sort the array of groups
 */
void sort_grlist(gr_list **fdp)
{
	gr_list	*ta, **vector;
	size_t	n = 0, i;

	if (*fdp == NULL)
		return;

	for (ta = *fdp; ta; ta = ta->next)
		n++;

	vector = (gr_list **)malloc(n * sizeof(gr_list *));

	i = 0;
	for (ta = *fdp; ta; ta = ta->next) {
		vector[i++] = ta;
	}

	qsort(vector, n, sizeof(gr_list*), (int(*)(const void*, const void*))compgroup);

	(*fdp) = vector[0];
	i = 1;

	for (ta = *fdp; ta; ta = ta->next) {

		if (i < n)
			ta->next = vector[i++];
		else
			ta->next = NULL;
	}

	free(vector);
	return;
}



int compgroup(gr_list **fdp1, gr_list **fdp2)
{
	int	rc;

	rc = strcmp((*fdp1)->group, (*fdp2)->group);
	if (rc != 0)
		return rc;
	return strcmp((*fdp1)->echo,  (*fdp2)->echo);
}



