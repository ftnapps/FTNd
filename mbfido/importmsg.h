#ifndef _IMPORTMSG_H
#define _IMPORTMSG_H


/*
 *  Structure for qualified systems to receive a echomail message
 */
typedef	struct	_qualify {
	struct _qualify	*next;			/* Linked list		    */
	fidoaddr	aka;			/* AKA of the linked system */
	unsigned 	inseenby	: 1;	/* System is in SEEN-BY	    */
	unsigned	send		: 1;	/* Send message to link	    */
	unsigned	orig		: 1;	/* Is originator of message */
} qualify;


int	importmsg(faddr *, faddr *, faddr *, char *, time_t, int, int, FILE *, off_t);
void	autocreate(char *, faddr *);


#endif

