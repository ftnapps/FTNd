#ifndef _PORTS_H
#define	_PORTS_H

/* $Id: ports.h,v 1.4 2006/02/04 13:39:51 mbse Exp $ */


/*
 * Linked list of available dialout ports.
 */
typedef struct _pp_list {
    struct _pp_list *next;
    char	    tty[7];	/* tty name of the port		*/
    unsigned int    mflags;	/* Analogue modem flags		*/
    unsigned int    dflags;	/* ISDN flags			*/
    int		    locked;	/* If port is locked		*/
    int		    locktime;	/* Time it is locked		*/
} pp_list;


struct _ttyinfohdr  ttyinfohdr;	/* TTY lines			*/
struct _ttyinfo	    ttyinfo;


void load_ports(void);		/* Initialize portlist		*/
void unload_ports(void);	/* Deinitialize portlist	*/
void check_ports(void);		/* Check status of all ports	*/


#endif
