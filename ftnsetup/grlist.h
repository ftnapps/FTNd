#ifndef _GRLIST_H
#define _GRLIST_H

typedef struct _gr_list {
	struct _gr_list	*next;
	char		group[13];
	int		tagged;
} gr_list;


void tidy_grlist(gr_list **);
void fill_grlist(gr_list **, char *);
void sort_grlist(gr_list **);
int  E_Group(gr_list **, char *);


#endif

