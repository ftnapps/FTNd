#ifndef	_PWIO_H
#define	_PWIO_H

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

