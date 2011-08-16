#ifndef _TASKCHAT_H
#define	_TASKCHAT_H

/* $Id: taskchat.h,v 1.10 2006/02/13 19:26:31 mbse Exp $ */

void chat_msg(char *, char *, char *);
void system_shout(const char *, ...);
void chat_init(void);
void chat_cleanuser(pid_t);
void chat_connect_r(char *, char *);
void chat_close_r(char *, char *);
void chat_put_r(char *, char *);
void chat_get_r(char *, char *);
void chat_checksysop_r(char *, char *);

#endif
