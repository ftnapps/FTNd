/* $Id$ */

#ifndef	_LIMITS_H_
#define	_LIMITS_H_
    
    
int setrlimit_value(unsigned int, const char *, unsigned int);
int set_prio(const char *);
int set_umask(const char *);
int check_logins(const char *, const char *);
int do_user_limits(const char *, const char *);
int setup_user_limits(const char *);
void setup_usergroups(const struct passwd *);
void setup_limits(const struct passwd *);
void set_filesize_limit(int);

#endif
