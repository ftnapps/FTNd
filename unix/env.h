/* $Id: env.h,v 1.1 2002/01/05 13:57:10 mbroek Exp $ */

#ifndef _ENV_H
#define	_ENV_H

void initenv(void);
void addenv(const char *, const char *);
void set_env(int, char * const *);
void sanitize_env(void);

#endif

