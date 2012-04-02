/* $Id: outstat.h,v 1.9 2005/10/11 20:49:49 mbse Exp $ */

#ifndef _OUTSTAT_H
#define	_OUTSTAT_H

typedef enum {CM_NONE, CM_INET, CM_ISDN, CM_POTS, MBFIDO, MBINDEX, MBFILE, MBINIT} CMODE;

struct  _nodeshdr   nodeshdr;   /* Header record		*/
struct  _nodes	    nodes;	/* Nodes datarecord		*/


/*
 * Linked list of nodes with mail in the outbound.
 * Updated after each scan.
 */
typedef struct _alist
{
    struct _alist   *next;	/* Next entry                   */
    fidoaddr	    addr;       /* Node address                 */
    int             flavors;    /* ORed flavors of mail/files   */
    time_t          time;       /* Date/time of mail/files      */
    off_t           size;       /* Total size of mail/files     */
    callstat        cst;        /* Last call status             */
    unsigned int    olflags;    /* Nodelist online flags        */
    unsigned int    moflags;    /* Nodelist modem flags         */
    unsigned int    diflags;    /* Nodelist ISDN flags          */
    unsigned int    ipflags;    /* Nodelist TCP/IP flags        */
    int             t1;         /* First Txx flag               */
    int             t2;         /* Second Txx flag              */
    int             callmode;   /* Call method                  */
    unsigned	    can_pots	: 1;
    unsigned	    can_ip	: 1;
    unsigned	    is_cm	: 1;
    unsigned	    is_icm	: 1;
} _alist_l;



/*
 * Bitmasks for calling status
 */
#define F_NORMAL 0x0001
#define F_CRASH  0x0002
#define F_IMM    0x0004
#define F_HOLD   0x0008
#define F_FREQ   0x0010
#define F_POLL   0x0020
#define F_ISFLO  0x0040
#define F_ISPKT  0x0080
#define	F_ISFIL  0x0100
#define F_CALL   0x0200



int  each(faddr *, char, int, char *);
char *callstatus(int);
char *callmode(int);
int  outstat(void);

#endif
