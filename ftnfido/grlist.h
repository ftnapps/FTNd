#ifndef _GRLIST_H_
#define _GRLIST_H


typedef struct _gr_list {		/* Announce array		   */
	struct _gr_list	*next;
	char		group[13];	/* Group name			   */
	char		echo[21];	/* Fileecho name		   */
	int		count;		/* Number of new files		   */
} gr_list;


void	tidy_grlist(gr_list **);
void	fill_grlist(gr_list **, char *, char *);
void	sort_grlist(gr_list **);


#endif

