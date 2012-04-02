#ifndef _STLIST_H
#define _STLIST_H

/* $Id: stlist.h,v 1.2 2005/10/11 20:49:49 mbse Exp $ */

typedef struct _st_list {
	struct _st_list	*next;
	char		string[81];
	int		pos;
} st_list;


void tidy_stlist(st_list **);
void fill_stlist(st_list **, char *, int);
void sort_stlist(st_list **);


#endif

