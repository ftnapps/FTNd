/* $Id$ */

#ifndef	_SETUPENV_H
#define	_SETUPENV_H

void addenv_path(const char *, const char *, const char *);
void read_env_file(const char *);
void setup_env(struct passwd *);

#endif
