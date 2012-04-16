/* calllist.h */

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
    unsigned int    moflags;			/* Modem flags		*/
    unsigned int    diflags;			/* ISDN flags		*/
    unsigned int    ipflags;			/* TCP/IP flags		*/
} tocall;


int check_calllist(void);

#endif

