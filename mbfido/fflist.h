#ifndef _FFLIST_H_
#define _FFLIST_H


typedef struct _ff_list {		/* Filefind array		   */
	struct _ff_list	*next;
	char		from[36];	/* From username		   */
	char		subject[72];	/* Search parameters		   */
	unsigned short	zone;		/* Original zone		   */
	unsigned short	net;		/* Original net			   */
	unsigned short	node;		/* Original node		   */
	unsigned short	point;		/* Original point		   */
	char		msgid[81];	/* Original msgid		   */
	unsigned long	msgnr;		/* Original message number	   */
	unsigned 	done : 1;	/* True if processed with success  */
} ff_list;



typedef struct _rf_list {		/* Reply filenames array	   */
	struct _rf_list	*next;
	char		filename[15];	/* Filename found		   */
	unsigned long	area;		/* BBS file area number		   */
} rf_list;


void tidy_fflist(ff_list **);
void fill_fflist(ff_list **);
void tidy_rflist(rf_list **);
void fill_rflist(rf_list **, char *, unsigned long);


#endif

