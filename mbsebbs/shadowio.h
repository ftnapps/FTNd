#ifndef	_SHADOWIO_H
#define	_SHADOWIO_H

#ifdef SHADOW_PASSWORD
#ifndef SHADOW_FILE
#define SHADOW_FILE "/etc/shadow"
#endif

#ifdef SHADOWGRP
#ifndef SGROUP_FILE
#define SGROUP_FILE "/etc/gshadow"
#endif
#endif

#endif


struct spwd *__spw_dup (const struct spwd *);
void __spw_set_changed (void);
int spw_close (void);
int spw_file_present (void);
const struct spwd *spw_locate (const char *);
int spw_lock (void);
int spw_name (const char *);
const struct spwd *spw_next (void);
int spw_open (int);
int spw_remove (const char *);
int spw_rewind (void);
int spw_unlock (void);
int spw_update (const struct spwd *);

#endif

