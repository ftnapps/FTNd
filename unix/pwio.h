/* $Id: pwio.h,v 1.3 2004/12/28 15:30:53 mbse Exp $ */

#ifndef	_PWIO_H
#define	_PWIO_H

#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)

#ifndef PASSWD_FILE
#define PASSWD_FILE "/etc/passwd"
#endif

#ifndef GROUP_FILE
#define GROUP_FILE "/etc/group"
#endif

struct passwd *__pw_dup (const struct passwd *);
void __pw_set_changed (void);
int pw_close (void);
const struct passwd *pw_locate (const char *);
int pw_lock (void);
int pw_lock_first (void);
int pw_name (const char *);
const struct passwd *pw_next (void);
int pw_open (int);
int pw_remove (const char *);
int pw_rewind (void);
int pw_unlock (void);
int pw_update (const struct passwd *);

#endif

#endif

