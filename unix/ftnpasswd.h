#ifndef _MBPASSWD_H
#define _MBPASSWD_H

/* $Id: mbpasswd.h,v 1.4 2005/08/27 12:06:49 mbse Exp $ */


                                        /* danger - side effects */
#define STRFCPY(A,B) \
        (strncpy((A), (B), sizeof(A) - 1), (A)[sizeof(A) - 1] = '\0')

/*
 * exit status values
 */

#define E_SUCCESS       0       /* success */
#define E_NOPERM        1       /* permission denied */
#define E_USAGE         2       /* invalid combination of options */
#define E_FAILURE       3       /* unexpected failure, nothing done */
#define E_MISSING       4       /* unexpected failure, passwd file missing */
#define E_PWDBUSY       5       /* passwd file busy, try again later */
#define E_BAD_ARG       6       /* invalid argument to option */



/*
 * Function prototypes
 */
struct passwd	*get_my_pwent(void);
static int      new_password (const struct passwd *, char *);
void		pwd_init(void);
char            *crypt_make_salt(void);
char            *pw_encrypt(const char *, const char *);
int             i64c(int);
char            *l64a(long);
#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)
static void	fail_exit(int);
static void	oom(void);
static void	update_noshadow(int);
#endif
#ifdef SHADOW_PASSWORD
struct spwd     *pwd_to_spwd(const struct passwd *);
static void	update_shadow(void);
#endif
char		*Basename(char *);

#endif

