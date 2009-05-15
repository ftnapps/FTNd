#ifndef	_SCANNEWS_H
#define	_SCANNEWS_H

/* $Id: scannews.h,v 1.3 2005/10/11 20:49:47 mbse Exp $ */

#define MAX_MSGID_LEN 196
#define MAX_GRP_LEN 128

/*
 * Linked list for list overview.fmt
 */
typedef struct	XOVERVIEW {
	struct	XOVERVIEW *next;
	char	*header;  /* dynamically alloced */
	char	*field;
	int	fieldlen;
	int	full;
} Overview, *POverview;



/*
 * Linked list structure one for each article
 */
typedef struct LinkList {
        struct LinkList *next;
        char		msgid[MAX_MSGID_LEN];
	int		nr;
	int		isdupe;
} List, *PList;

enum { RETVAL_ERROR = -1, RETVAL_OK = 0, RETVAL_NOARTICLES, RETVAL_UNEXPECTEDANS, RETVAL_VERNR, \
       RETVAL_NOAUTH, RETVAL_EMPTYKILL, RETVAL_NOXOVER };


void ScanNews(void);


#endif
