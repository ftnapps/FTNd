#ifndef _QUALIFY_H
#define _QUALIFY_H

/* $Id$ */

/*
 *  Structure for qualified systems to receive a echomail message/tic file
 */
typedef	struct	_qualify {
	struct _qualify	*next;			/* Linked list		    */
	fidoaddr	aka;			/* AKA of the linked system */
	unsigned 	inseenby	: 1;	/* System is in SEEN-BY	    */
	unsigned	send		: 1;	/* Send message to link	    */
	unsigned	orig		: 1;	/* Is originator of message */
} qualify;



void tidy_qualify(qualify **);
void fill_qualify(qualify **, fidoaddr, int, int);


#endif
