/* $Id$ */

#ifndef _CALLLIST_H
#define	_CALLLIST_H



/*
 *  Callist
 */
typedef struct _tocall {
    fidoaddr	    addr;			/* Address to call	*/
    int		    callmode;			/* Method to use	*/
    callstat	    cst;			/* Last call status	*/
    int		    calling;			/* Is calling		*/
    pid_t	    taskpid;			/* Task pid number	*/
    unsigned long   moflags;			/* Modem flags		*/
    unsigned long   diflags;			/* ISDN flags		*/
    unsigned long   ipflags;			/* TCP/IP flags		*/
} tocall;


int check_calllist(void);

#endif

