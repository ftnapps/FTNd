#ifndef _STLIST_H
#define _STLIST_H

typedef struct _st_list {
	struct _st_list	*next;
	char		string[81];
	long		pos;
} st_list;


void tidy_stlist(st_list **);
void fill_stlist(st_list **, char *, long);
void sort_stlist(st_list **);


#endif

