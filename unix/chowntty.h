/* $Id: chowntty.h,v 1.1 2002/01/05 13:57:10 mbroek Exp $ */

#ifndef _CHOWNTTY_H
#define _CHOWNTTY_H


int  is_my_tty(const char *);
void chown_tty(const char *, const struct passwd *);


#endif
