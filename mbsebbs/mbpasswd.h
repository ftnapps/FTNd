#ifndef _MBUSERADD_H
#define _MBUSERADD_H


                                        /* danger - side effects */
#define STRFCPY(A,B) \
        (strncpy((A), (B), sizeof(A) - 1), (A)[sizeof(A) - 1] = '\0')


/*
 * Function prototypes
 */
struct passwd	*get_my_pwent(void);
static int      new_password (const struct passwd *, char *);
static void	fail_exit(int);
static void	oom(void);
void		pwd_init(void);
char            *crypt_make_salt(void);
char            *pw_encrypt(const char *, const char *);
int             i64c(int);
char            *l64a(long);
static void	update_noshadow(int);

#ifdef SHADOW_PASSWORD
struct spwd     *pwd_to_spwd(const struct passwd *);
static void	update_shadow(void);
#endif

#endif

