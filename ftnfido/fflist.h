#ifndef _FFLIST_H_
#define _FFLIST_H

/* $Id: fflist.h,v 1.3 2007/10/15 12:48:11 mbse Exp $ */

typedef struct _ff_list {		/* Filefind array		   */
	struct _ff_list	*next;
	char		from[36];	/* From username		   */
	char		subject[72];	/* Search parameters		   */
	unsigned short	zone;		/* Original zone		   */
	unsigned short	net;		/* Original net			   */
	unsigned short	node;		/* Original node		   */
	unsigned short	point;		/* Original point		   */
	char		msgid[81];	/* Original msgid		   */
	unsigned int	msgnr;		/* Original message number	   */
	unsigned 	done : 1;	/* True if processed with success  */
} ff_list;



typedef struct _rf_list {		/* Reply filenames array	   */
	struct _rf_list	*next;
	char		filename[81];	/* Filename found		   */
	unsigned int	area;		/* BBS file area number		   */
} rf_list;


void tidy_fflist(ff_list **);
void fill_fflist(ff_list **);
void tidy_rflist(rf_list **);
void fill_rflist(rf_list **, char *, unsigned int);


#endif
