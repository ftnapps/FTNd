#ifndef _MSGFLAGS_H
#define	_MSGFLAGS_H

/* $Id: msgflags.h,v 1.1 2002/06/30 12:48:46 mbroek Exp $ */

int     flag_on(char *,char *);
int     flagset(char *);
char    *compose_flags(int,char *);
char    *strip_flags(char *);
int     flag_on(char *,char *);

#endif

