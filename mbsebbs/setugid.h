/* $Id$ */

#ifndef _SETUGID_H
#define	_SETUGID_H

int setup_groups(const struct passwd *);
int change_uid(const struct passwd *);
int setup_uid_gid(const struct passwd *, int);

#endif
