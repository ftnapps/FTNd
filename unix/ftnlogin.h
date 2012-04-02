/* $Id: mblogin.h,v 1.3 2003/12/23 18:33:42 mbroek Exp $ */

#ifndef	_MBLOGIN_H
#define	_MBLOGIN_H

#define ReleaseDate __DATE__

#define ISDIGIT_LOCALE(c) (IN_CTYPE_DOMAIN (c) && isdigit (c))

/* Take care of NLS matters.  */

#if HAVE_LOCALE_H
# include <locale.h>
#endif
#if !HAVE_SETLOCALE
# define setlocale(Category, Locale) /* empty */
#endif

#define gettext_noop(String) (String)
/* #define gettext_def(String) "#define String" */

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# undef bindtextdomain
# define bindtextdomain(Domain, Directory) /* empty */
# undef textdomain
# define textdomain(Domain) /* empty */
# define _(Text) Text
#endif

#ifndef P_
# ifdef PROTOTYPES
#  define P_(x) x
# else
#  define P_(x) ()
# endif
#endif

#if STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else  /* not STDC_HEADERS */
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr(), *strrchr(), *strtok();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy((s), (d), (n))
# endif
#endif /* not STDC_HEADERS */

#include <sys/stat.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else  /* not TIME_WITH_SYS_TIME */
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif /* not TIME_WITH_SYS_TIME */

#ifdef HAVE_MEMSET
# define memzero(ptr, size) memset((void *)(ptr), 0, (size))
#else
# define memzero(ptr, size) bzero((char *)(ptr), (size))
#endif
#define strzero(s) memzero(s, strlen(s))  /* warning: evaluates twice */

#ifdef HAVE_DIRENT_H  /* DIR_SYSV */
# include <dirent.h>
# define DIRECT dirent
#else
# ifdef HAVE_SYS_NDIR_H  /* DIR_XENIX */
#  include <sys/ndir.h>
# endif
# ifdef HAVE_SYS_DIR_H  /* DIR_??? */
#  include <sys/dir.h>
# endif
# ifdef HAVE_NDIR_H  /* DIR_BSD */
#  include <ndir.h>
# endif
# define DIRECT direct
#endif

#ifdef SHADOWPWD
/*
 * Possible cases:
 * - /usr/include/shadow.h exists and includes the shadow group stuff.
 * - /usr/include/shadow.h exists, but we use our own gshadow.h.
 * - /usr/include/shadow.h doesn't exist, use our own shadow.h and gshadow.h.
 */
#if HAVE_SHADOW_H
#include <shadow.h>
#if defined(SHADOWGRP) && !defined(GSHADOW)
#include "gshadow_.h"
#endif 
#else  /* not HAVE_SHADOW_H */
#include "shadow_.h"
#ifdef SHADOWGRP
#include "gshadow_.h"
#endif  
#endif  /* not HAVE_SHADOW_H */
#endif  /* SHADOWPWD */

#include <limits.h>

#ifndef NGROUPS_MAX
#ifdef  NGROUPS
#define NGROUPS_MAX     NGROUPS
#else
#define NGROUPS_MAX     64
#endif
#endif


#ifndef F_OK
# define F_OK 0
# define X_OK 1
# define W_OK 2
# define R_OK 4
#endif

#if HAVE_TERMIOS_H
# include <termios.h>
# define STTY(fd, termio) tcsetattr(fd, TCSANOW, termio)
# define GTTY(fd, termio) tcgetattr(fd, termio)
# define TERMIO struct termios
# define USE_TERMIOS
#else  /* assumed HAVE_TERMIO_H */
# include <sys/ioctl.h>
# include <termio.h>
# define STTY(fd, termio) ioctl(fd, TCSETA, termio)
# define GTTY(fd, termio) ioctl(fd, TCGETA, termio)
# define TEMRIO struct termio
# define USE_TERMIO
#endif

/*
 * Password aging constants
 *
 * DAY - seconds / day
 * WEEK - seconds / week
 * SCALE - seconds / aging unit
 */

/* Solaris defines this in shadow.h */
#ifndef DAY
#define DAY (24L*3600L)
#endif

#define WEEK (7*DAY)

#ifdef ITI_AGING
#define SCALE 1
#else
#define SCALE DAY
#endif

/* Copy string pointed by B to array A with size checking.  It was originally
 * in lmain.c but is _very_ useful elsewhere.  Some setuid root programs with
 * very sloppy coding used to assume that BUFSIZ will always be enough...  */

                                        /* danger - side effects */
#define STRFCPY(A,B) \
        (strncpy((A), (B), sizeof(A) - 1), (A)[sizeof(A) - 1] = '\0')

/* get rid of a few ugly repeated #ifdefs in pwent.c and grent.c */
/* XXX - this is ugly too, configure should test it and not check for
   any hardcoded system names, if possible.  --marekm */
#if defined(SVR4) || defined(AIX) || defined(__linux__)
#define SETXXENT_TYPE void
#define SETXXENT_RET(x) return
#define SETXXENT_TEST(x) x; if (0) /* compiler should optimize this away */
#else
#define SETXXENT_TYPE int
#define SETXXENT_RET(x) return(x)
#define SETXXENT_TEST(x) if (x)
#endif
	
#ifndef PASSWD_FILE
#define PASSWD_FILE "/etc/passwd"
#endif

#ifndef GROUP_FILE
#define GROUP_FILE "/etc/group"
#endif

#ifdef SHADOWPWD
#ifndef SHADOW_FILE
#define SHADOW_FILE "/etc/shadow"
#endif
#endif

#ifdef SHADOWGRP
#ifndef SGROUP_FILE
#define SGROUP_FILE "/etc/gshadow"
#endif
#endif

#define PASSWD_PAG_FILE  PASSWD_FILE ".pag"
#define GROUP_PAG_FILE   GROUP_FILE  ".pag"
#define SHADOW_PAG_FILE  SHADOW_FILE ".pag"
#define SGROUP_PAG_FILE  SGROUP_FILE ".pag"

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifdef sun  /* hacks for compiling on SunOS */
# ifndef SOLARIS
extern int fputs();
extern char *strdup();
extern char *strerror();
# endif
#endif


/*
 * string to use for the pw_passwd field in /etc/passwd when using
 * shadow passwords - most systems use "x" but there are a few
 * exceptions, so it can be changed here if necessary.  --marekm
 */
#ifndef SHADOW_PASSWD_STRING
#define SHADOW_PASSWD_STRING "x"
#endif

#ifdef PAM_STRERROR_NEEDS_TWO_ARGS  /* Linux-PAM 0.59+ */
#define PAM_STRERROR(pamh, err) pam_strerror(pamh, err)
#else
#define PAM_STRERROR(pamh, err) pam_strerror(err)
#endif


#endif
