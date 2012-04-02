/*****************************************************************************
 *
 * $Id: fsort.c,v 1.7 2005/08/28 15:33:23 mbse Exp $
 * Purpose ...............: File sort
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
#include "../lib/mbselib.h"
#include "fsort.h"



/*
 * Tidy the filearray
 */
void tidy_fdlist(fd_list **fdp)
{
	fd_list	*tmp, *old;

	for (tmp = *fdp; tmp; tmp = old) {
		old = tmp->next;
		free(tmp);
	}
	*fdp = NULL;
}



/*
 * Add a file on the array.
 */
void fill_fdlist(fd_list **fdp, char *filename, time_t filedate)
{
	fd_list	*tmp;

	tmp = (fd_list *)malloc(sizeof(fd_list));
	tmp->next = *fdp;
	snprintf(tmp->fname, 65, "%s", filename);
	tmp->fdate = filedate;
	*fdp = tmp;
}



int compfdate(fd_list **, fd_list **);


/*
 * Sort the array of files by filedate
 */
void sort_fdlist(fd_list **fdp)
{
	fd_list	*ta, **vector;
	size_t	n = 0, i;

	if (*fdp == NULL) {
		return;
	}

	for (ta = *fdp; ta; ta = ta->next)
		n++;

	vector = (fd_list **)malloc(n * sizeof(fd_list *));

	i = 0;
	for (ta = *fdp; ta; ta = ta->next) {
		vector[i++] = ta;
	}

	qsort(vector, n, sizeof(fd_list*), (int(*)(const void*, const void*))compfdate);

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



int compfdate(fd_list **fdp1, fd_list **fdp2)
{
	return ((*fdp1)->fdate - (*fdp2)->fdate);
}



/*
 * Return the name of the oldest file in the array
 */
char *pull_fdlist(fd_list **fdp)
{
	static char	buf[PATH_MAX];
	fd_list		*ta;

	if (*fdp == NULL)
		return NULL;

	ta = *fdp;
	memset(&buf, 0, sizeof(buf));
	snprintf(buf, PATH_MAX, "%s", ta->fname);

	if (ta->next != NULL)
		*fdp = ta->next;
	else
		*fdp = NULL;

	free(ta);
	return buf;
}


