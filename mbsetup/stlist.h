#ifndef _STLIST_H
#define _STLIST_H

/* $Id$ */

typedef struct _st_list {
	struct _st_list	*next;
	char		string[81];
	int		pos;
} st_list;


void tidy_stlist(st_list **);
void fill_stlist(st_list **, char *, int);
void sort_stlist(st_list **);


#endif

