#ifndef	_MBNNTP_H
#define	_MBNNTP_H

/* $Id$ */

void send_nntp(const char *, ...);

#ifndef USE_NEWSGATE

int  get_nntp(char *, int);
void nntp(void);

#endif

#endif
