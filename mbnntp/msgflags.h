#ifndef _MSGFLAGS_H
#define	_MSGFLAGS_H

/* $Id$ */

#ifndef	USE_NEWSGATE

int     flag_on(char *,char *);
int     flagset(char *);
char    *compose_flags(int,char *);
char    *strip_flags(char *);
int     flag_on(char *,char *);

#endif

#endif
