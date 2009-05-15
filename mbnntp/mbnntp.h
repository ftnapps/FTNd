#ifndef	_MBNNTP_H
#define	_MBNNTP_H

/* $Id: mbnntp.h,v 1.5 2005/01/14 19:52:13 mbse Exp $ */

int usercharset;

void send_nntp(const char *, ...);

#ifndef USE_NEWSGATE

int  get_nntp(char *, int);
void nntp(void);

#endif

#endif
