/* $Id$ */

#ifndef _OUTSTAT_H
#define	_OUTSTAT_H

typedef enum {CM_NONE, CM_INET, CM_ISDN, CM_POTS, MBFIDO, MBINDEX, MBFILE, MBINIT} CMODE;


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
    unsigned long   olflags;    /* Nodelist online flags        */
    unsigned long   moflags;    /* Nodelist modem flags         */
    unsigned long   diflags;    /* Nodelist ISDN flags          */
    unsigned long   ipflags;    /* Nodelist TCP/IP flags        */
    int             t1;         /* First Txx flag               */
    int             t2;         /* Second Txx flag              */
    int             callmode;   /* Call method                  */
} _alist_l;


/*
 * Linked list of available dialout ports.
 */
typedef struct _pp_list {
    struct _pp_list *next;
    char	    tty[7];	/* tty name of the port		*/
    unsigned long   mflags;	/* Analogue modem flags		*/
    unsigned long   dflags;	/* ISDN flags			*/
    int		    locked;	/* If port is locked		*/
} pp_list;


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
#define F_CALL   0x0100



struct _ttyinfohdr  ttyinfohdr;	/* TTY lines			*/
struct _ttyinfo	    ttyinfo;


int  each(faddr *, char, int, char *);
char *callstatus(int);
char *callmode(int);
int  outstat(void);
void load_ports(void);

#endif

