#ifndef _MKFTNHDR_H
#define	_MKFTNHDR_H

/* $Id: mkftnhdr.h,v 1.3 2005/10/11 20:49:48 mbse Exp $ */

#ifndef	USE_NEWSGATE

int ftnmsgid(char *,char **,unsigned int *,char *);
ftnmsg *mkftnhdr(rfcmsg *, int, faddr *);

#endif

#endif
