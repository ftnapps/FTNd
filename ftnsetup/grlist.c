/*****************************************************************************
 *
 * $Id: grlist.c,v 1.11 2005/08/29 19:43:25 mbse Exp $
 * Purpose ...............: Group Listing utils
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
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
	snprintf(tmp->group, 13, "%s", groupname);
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
    int	    o = 0, n = 0, i, j, x, y, rc = FALSE, All = FALSE;
    gr_list *tmp;

    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw(5, 6, (char *)"%s", title);
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
	    if ((j >= (o + 1)) && (j < (o + 41))) {
		if (tmp->tagged)
		    mbse_mvprintw(y, x, (char *)"%2d. + %s", j, tmp->group);
		else
		    mbse_mvprintw(y, x, (char *)"%2d.   %s", j, tmp->group);
		y++;
		if (y == 17) {
		    y = 7;
		    x += 20;
		}
	    }
	}

	i = select_tag(n);

	switch (i) {
	    case 0: clr_index();
		    return rc;
		    break;
	    case -2:if ((o - 40) >= 0) {
			clr_index();
		        set_color(WHITE, BLACK);
		        mbse_mvprintw(5, 5, (char *)"%s", title);
		        set_color(CYAN, BLACK);
		        o -= 40;
		    }
		    break;
	    case -1:if ((o + 40) < n) {
		        clr_index();
			set_color(WHITE, BLACK);
		        mbse_mvprintw(5, 5, (char *)"%s", title);
		        set_color(CYAN, BLACK);
		        o += 40;
		    }
		    break;
	    case -3:All = !All;
		    for (tmp = *fdp; tmp; tmp = tmp->next)
		        tmp->tagged = All;
		    rc = TRUE;
		    break;
	    default:if ((i >= 1) && (i <= n)) {
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
		        if (o != ((i -1) / 40) * 40) {
			    /*
			     * If a group is selected outside the visible
			     * range, change the groupview.
			     */
			    o = ((i -1) / 40) * 40;
			    clr_index();
			    set_color(WHITE, BLACK);
			    mbse_mvprintw(5, 5, (char *)"%s", title);
			    set_color(CYAN, BLACK);
		        }
		    }
		    break;
	}
    }
}


