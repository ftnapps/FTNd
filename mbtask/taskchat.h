#ifndef _TASKCHAT_H
#define	_TASKCHAT_H

/* $Id$ */

void chat_init(void);
char *chat_connect(char *);
char *chat_close(char *);
char *chat_put(char *);
char *chat_get(char *);

#endif
