/*****************************************************************************
 *
 * File ..................: grlist.c
 * Purpose ...............: Group Listing utils
 * Last modification date : 27-Nov-2000
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h"
#include "grlist.h"
#include "ledit.h"


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
void fill_grlist(gr_list **fdp, char *groupname)
{
	gr_list	*tmp;

	tmp = (gr_list *)malloc(sizeof(gr_list));
	tmp->next = *fdp;
	sprintf(tmp->group, "%s", groupname);
	tmp->tagged = FALSE;
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
	return strcmp((*fdp1)->group, (*fdp2)->group);
}



int E_Group(gr_list **fdp, char *title)
{
	int	n = 0, i, j, x, y, rc = FALSE;
	gr_list	*tmp;

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw(5, 5, (char *)"%s", title);
	set_color(CYAN, BLACK);

	for (tmp = *fdp; tmp; tmp = tmp->next)
		n++;

	for (;;) {
		set_color(CYAN, BLACK);
		y = 7;
		x = 5;
		j = 0;

		for (tmp = *fdp; tmp; tmp = tmp->next) {
			j++;
			if (tmp->tagged)
				mvprintw(y, x, (char *)"%2d. + %s", j, tmp->group);
			else
				mvprintw(y, x, (char *)"%2d.   %s", j, tmp->group);
			y++;
			if (y == 18) {
				y = 7;
				x += 20;
			}
		}

		i = select_tag(n);

		if (i == 0) {
			clr_index();
			return rc;
		}

		if ((i >= 1) && (i <= n)) {
			j = 0;
			rc = TRUE;
			for (tmp = *fdp; tmp; tmp = tmp->next) {
				j++;
				if (j == i) {
					if (tmp->tagged)
						tmp->tagged = FALSE;
					else
						tmp->tagged = TRUE;
				}
			}
		}
	}
}


