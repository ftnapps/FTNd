/* $Id$ */

#ifndef _GETDEF_H
#define _GETDEF_H


int getdef_bool(const char *);
long getdef_long(const char *, long);
int getdef_num(const char *, int);
char *getdef_str(const char *);
void def_load (void);

#endif /* _GETDEF_H */
