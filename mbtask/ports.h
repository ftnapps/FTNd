/* $Id$ */

#ifndef _PORTS_H
#define	_PORTS_H



/*
 * Linked list of available dialout ports.
 */
typedef struct _pp_list {
    struct _pp_list *next;
    char	    tty[7];	/* tty name of the port		*/
    unsigned long   mflags;	/* Analogue modem flags		*/
    unsigned long   dflags;	/* ISDN flags			*/
    int		    locked;	/* If port is locked		*/
    long	    locktime;	/* Time it is locked		*/
} pp_list;


struct _ttyinfohdr  ttyinfohdr;	/* TTY lines			*/
struct _ttyinfo	    ttyinfo;


void load_ports(void);		/* Initialize portlist		*/
void check_ports(void);		/* Check status of all ports	*/


#endif
