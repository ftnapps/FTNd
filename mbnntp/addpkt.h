#ifndef	_ADDPKT_H
#define	_ADDPKT_H

/* $Id$ */

FILE *OpenPkt(fidoaddr, fidoaddr, char *);
int  AddMsgHdr(FILE *, faddr *, faddr *, int, int, time_t, char *, char *, char *);

#endif
