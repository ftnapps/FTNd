/*****************************************************************************
 *
 * $Id: falists.c,v 1.8 2004/02/21 14:24:04 mbroek Exp $
 * Purpose ...............: SEEN-BY and PATH lists
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
#include "mbselib.h"



void tidy_falist(fa_list **fap)
{
	fa_list	*tmp,*old;

	for (tmp = *fap; tmp; tmp = old) {
		old = tmp->next;
		tidy_faddr(tmp->addr);
		free(tmp);
	}
	*fap = NULL;
}



int in_list(faddr *addr, fa_list **fap, int fourd)
{
	fa_list	*tmp;

	/*
	 * No seenby check for points
	 */
	if (addr->point) {
		return 0;
	}

	for (tmp = *fap; tmp; tmp = tmp->next)
		if ((tmp->addr->net == addr->net) &&
		    ((!fourd) || (fourd && (tmp->addr->zone == addr->zone))) &&
		    ((!fourd) || (fourd && (tmp->addr->point == addr->point))) &&
		    (tmp->addr->node == addr->node)) {
			return 1;
		}

	return 0;
}



void fill_list(fa_list **fap, char *str, fa_list **omit)
{
	fa_list			*tmp;
	faddr			*ta;
	static unsigned int	oldzone, oldnet;
	char			*buf, *p, *q, *r;
	int			allowskip = 1;

	if ((str == NULL) || (*str == '\0'))
		return;

	buf = xstrcpy(str);
	r = buf + strlen(buf);

	for (p = strtok(buf," \t\n"), q = p + strlen(p) + 1;
	     p;
	     p = (q < r) ? strtok(q, " \t\n"):NULL, q = p ? p + strlen(p) + 1:r)
	if ((ta = parsefnode(p))) {
		if (ta->zone == 0)
			ta->zone = oldzone;
		else
			oldzone = ta->zone;
		if (ta->net == 0) 
			ta->net = oldnet;
		else 
			oldnet = ta->net;
		if (allowskip && omit && *omit && (metric(ta,(*omit)->addr) == 0)) {
			tmp = *omit;
			*omit = (*omit)->next;
			tmp->next = NULL;
			tidy_falist(&tmp);
		} else {
			allowskip = 0;
			tmp = (fa_list *)malloc(sizeof(fa_list));
			tmp->next = *fap;
			tmp->addr = ta;
			*fap = tmp;
		}
	}

	free(buf);
	return;
}



void fill_path(fa_list **fap, char *str)
{
	fa_list			**tmp;
	faddr			*ta;
	static unsigned int	oldnet;
	char			*buf, *p, *q, *r;

	if ((str == NULL) || (*str == '\0')) 
		return;
	buf = xstrcpy(str);
	for (tmp = fap; *tmp; tmp = &((*tmp)->next)); /*nothing*/ 
	r = buf + strlen(buf);

	for (p = strtok(buf, " \t\n"), q = p + strlen(p) + 1;
	     p;
	     p = (q < r) ? strtok(q, " \t\n") : NULL, q = p ? p + strlen(p) + 1 : r)
	if ((ta = parsefnode(p))) {
		if (ta->net == 0) 
			ta->net=oldnet;
		else 
			oldnet=ta->net;
		*tmp = (fa_list *)malloc(sizeof(fa_list));
		(*tmp)->next = NULL;
		(*tmp)->addr = ta;
		tmp = &((*tmp)->next);
	}
	free(buf);
	return;
}




int compaddr(fa_list **,fa_list **);



void uniq_list(fa_list **fap)
{
	fa_list *ta, *tan;

	if (*fap == NULL)
		return;
	for (ta = *fap; ta; ta = ta->next) {
		while ((tan = ta->next) && (compaddr(&ta, &tan) == 0)) {
			ta->next = tan->next;
			tidy_faddr(tan->addr);
			free(tan);
		}
		ta->next = tan;
	}
}



void sort_list(fa_list **fap)
{
	fa_list *ta, **vector;
	size_t	n = 0, i;

	if (*fap == NULL)
		return;

	for (ta = *fap; ta; ta = ta->next) 
		n++;
	vector = (fa_list **)malloc(n * sizeof(fa_list *));
	i = 0;

	for (ta = *fap; ta; ta = ta->next) {
		vector[i++] = ta;
	}
	qsort(vector,n,sizeof(faddr*),
		(int(*)(const void*,const void*))compaddr);

	(*fap) = vector[0];
	i = 1;

	for (ta = *fap; ta; ta = ta->next) {
		while ((i < n) && (compaddr(&ta,&(vector[i])) == 0)) {
			tidy_faddr((vector[i])->addr);
			free(vector[i]);
			i++;
		}
		if (i < n) 
			ta->next=vector[i++];
		else 
			ta->next=NULL;
	}

	free(vector);
	return;
}



int compaddr(fa_list **fap1, fa_list **fap2)
{
	if ((*fap1)->addr->net != (*fap2)->addr->net)
		return ((*fap1)->addr->net - (*fap2)->addr->net);
	else
		return ((*fap1)->addr->node - (*fap2)->addr->node);
}



