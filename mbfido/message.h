#ifndef	_MESSAGE_H
#define _MESSAGE_H


void StatAdd(statcnt *, unsigned long);
int  needputrfc(rfcmsg*);
char *viadate(void);
int  putmessage(rfcmsg *, ftnmsg *, FILE *, faddr *, char,fa_list **, int, int);


#endif

