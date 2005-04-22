#ifndef _TASKCHAT_H
#define	_TASKCHAT_H

/* $Id$ */

void system_shout(const char *, ...);
void chat_init(void);
void chat_cleanuser(pid_t);
char *chat_connect(char *);
char *chat_close(char *);
char *chat_put(char *);
char *chat_get(char *);
char *chat_checksysop(char *);

#endif
