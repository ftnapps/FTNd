#ifndef _TASKIBC_H
#define	_TASKIBC_H

/* $Id$ */

#ifdef	USE_EXPERIMENT


/*
 * Linked list of neighbour chatservers
 */
typedef struct _ncs_list {
    struct _ncs_list	*next;
    char		server[64];	    /* Server address		*/
    char		myname[64];	    /* My server address	*/
    char		passwd[16];	    /* Server password		*/
    int			state;		    /* Connection state		*/
    time_t		action;		    /* Time for next action	*/
    time_t		last;		    /* Last received message	*/
    int			version;	    /* Protocol version of peer	*/
    unsigned		remove	    : 1;    /* If entry must be removed	*/
    unsigned		compress    : 1;    /* User link compression	*/
    struct sockaddr_in	servaddr_in;	    /* Peer socketaddress	*/
    int			socket;		    /* Peer socket		*/
    unsigned long	token;		    /* Server token		*/
} ncs_list;


void send_all(char *);
void *ibc_thread(void *);

#endif

#endif
