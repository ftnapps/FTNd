/* $Id: setugid.h,v 1.1 2002/01/05 13:57:10 mbroek Exp $ */

#ifndef _SETUGID_H
#define	_SETUGID_H

int setup_groups(const struct passwd *);
int change_uid(const struct passwd *);
int setup_uid_gid(const struct passwd *, int);

#endif
