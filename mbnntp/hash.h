#ifndef HASH_H
#define HASH_H

/* $Id: hash.h,v 1.3 2005/10/11 20:49:48 mbse Exp $ */

#ifndef	USE_NEWSGATE

void hash_update_s(unsigned int *, char *);
void hash_update_n(unsigned int *, unsigned int);

#endif

#endif
