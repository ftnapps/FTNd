#ifndef _MSGFLAGS_H
#define	_MSGFLAGS_H

/* $Id: msgflags.h,v 1.2 2004/06/20 14:38:11 mbse Exp $ */

#ifndef	USE_NEWSGATE

int     flag_on(char *,char *);
int     flagset(char *);
char    *compose_flags(int,char *);
char    *strip_flags(char *);
int     flag_on(char *,char *);

#endif

#endif
