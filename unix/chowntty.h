/* $Id$ */

#ifndef _CHOWNTTY_H
#define _CHOWNTTY_H


int  is_my_tty(const char *);
void chown_tty(const char *, const struct passwd *);


#endif
