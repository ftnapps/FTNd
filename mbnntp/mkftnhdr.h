#ifndef _MKFTNHDR_H
#define	_MKFTNHDR_H

/* $Id$ */

#ifndef	USE_NEWSGATE

int ftnmsgid(char *,char **,unsigned long *,char *);
ftnmsg *mkftnhdr(rfcmsg *, int, faddr *);

#endif

#endif
