#ifndef _MSGFLAGS_H
#define	_MSGFLAGS_H

/* msgflags.h */

int     flag_on(char *,char *);
int     flagset(char *);
char    *compose_flags(int,char *);
char    *strip_flags(char *);
int     flag_on(char *,char *);

#endif

