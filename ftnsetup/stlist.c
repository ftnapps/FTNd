/*****************************************************************************
 *
 * stlist.c
 * Purpose ...............: String sorting for databases.
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2012   Robert James Clay <jame@rocasa.us>
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
#include "../lib/ftndlib.h"
#include "stlist.h"


/*
 * Tidy the strings array
 */
void tidy_stlist(st_list ** fdp)
{
	st_list	*tmp, *old;

	for (tmp = *fdp; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}

	*fdp = NULL;
}



/*
 * Add a string to the array
 */
void fill_stlist(st_list **fdp, char *stringname, int pos)
{
	st_list	*tmp;

	tmp = (st_list *)malloc(sizeof(st_list));
	tmp->next = *fdp;
	snprintf(tmp->string, 81, "%s", stringname);
	tmp->pos = pos;
	*fdp = tmp;
}



int compstring(st_list **, st_list **);

/*
 *  Sort the array of strings
 */
void sort_stlist(st_list **fdp)
{
	st_list	*ta, **vector;
	size_t	n = 0, i;

	if (*fdp == NULL)
		return;

	for (ta = *fdp; ta; ta = ta->next)
		n++;

	vector = (st_list **)malloc(n * sizeof(st_list *));

	i = 0;
	for (ta = *fdp; ta; ta = ta->next)
		vector[i++] = ta;

	qsort(vector, n, sizeof(st_list*), (int(*)(const void*, const void*))compstring);

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



int compstring(st_list **fdp1, st_list **fdp2)
{
	return strcmp((*fdp1)->string, (*fdp2)->string);
}

