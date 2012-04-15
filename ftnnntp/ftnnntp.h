#ifndef	_FTNNNTP_H
#define	_FTNNNTP_H

/* $Id: ftnnntp.h */

int usercharset;

void send_nntp(const char *, ...);

#ifndef USE_NEWSGATE

int  get_nntp(char *, int);
void nntp(void);

#endif

#endif
