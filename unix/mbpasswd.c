/*****************************************************************************
 *
 * Purpose ...............: setuid root version of passwd
 * Shadow Suite (c) ......: Julianne Frances Haugh
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
 *   
 * Michiel Broek	FIDO:		2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#if defined(SHADOW_PASSWORD)
#include <shadow.h>
#endif
#ifdef HAVE_USERSEC_H
#include <userpw.h>
#include <usersec.h>
#include <userconf.h>
#endif

#if (defined(__FreeBSD__) && __FreeBSD_version >= 600000)
#define FreeBSD_sysctl 1
#endif

#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(FreeBSD_sysctl)
#include <sys/sysctl.h>
#endif

#include "encrypt.h"
#include "rad64.h"
#include "myname.h"
#include "xmalloc.h"
#include "pwio.h"
#include "shadowio.h"
#include "pw_util.h"
#include "getdef.h"
#include "mbpasswd.h"


#ifndef AGING
#if defined(SHADOW_PASSWORD)
#define AGING   1
#endif
#else
#if !defined(SHADOW_PASSWORD)
#undef AGING
#endif
#endif

static int do_update_age = 0;
#ifndef PAM
static char crypt_passwd[128];  /* The "old-style" password, if present */
static int do_update_pwd = 0;
#endif


#ifdef SHADOW_PASSWORD
static int is_shadow_pwd;
static void check_password(const struct passwd *, const struct spwd *);
#else /* !SHADOW_PASSWORD */
static void check_password(const struct passwd *);
#endif /* !SHADOW_PASSWORD */

#ifndef DAY
#define DAY (24L*3600L)
#endif

#define WEEK (7*DAY)
#define SCALE DAY


/*
 * string to use for the pw_passwd field in /etc/passwd when using
 * shadow passwords - most systems use "x" but there are a few
 * exceptions, so it can be changed here if necessary.  --marekm
 */
#ifndef SHADOW_PASSWD_STRING
#define SHADOW_PASSWD_STRING "x"
#endif


/*
 * Global variables
 */

static char *name;	/* The name of user whose password is being changed */
static char *myname;	/* The current user's name */
static int  force;	/* Force update of locked passwords */


#ifdef _VPOPMAIL_PATH

/*
 * Milliseconds timer, returns 0 on success.
 */
int msleep(int msecs)
{
    int             rc;
    struct timespec req, rem;

    rem.tv_sec = 0;
    rem.tv_nsec = 0;
    req.tv_sec = msecs / 1000;
    req.tv_nsec = (msecs % 1000) * 1000000;

    while (1) {

	rc = nanosleep(&req, &rem);
	if (rc == 0)
	    break;
	if ((errno == EINVAL) || (errno == EFAULT)) {
	    break;
	}

	/*
	 * Error was EINTR, run timer again to complete.
	 */
	req.tv_sec = rem.tv_sec;
	req.tv_nsec = rem.tv_nsec;
	rem.tv_sec = 0;
	rem.tv_nsec = 0;
    }

    return rc;
}



int execute(char **args, char *in, char *out, char *err)
{
    char    buf[PATH_MAX];
    int     i, pid, status = 0, rc = 0;

    memset(&buf, 0, sizeof(buf));
    for (i = 0; i < 16; i++) {
        if (args[i])
            snprintf(buf + strlen(buf), PATH_MAX - strlen(buf), " %s", args[i]);
        else
            break;
    }
    syslog(LOG_WARNING, "Execute:%s", buf);
    fflush(stdout);
    fflush(stderr);

    if ((pid = fork()) == 0) {
	msleep(150);

        if (in) {
            close(0);
            if (open(in, O_RDONLY) != 0) {
                syslog(LOG_WARNING, "Reopen of stdin to %s failed", in);
                _exit(-1);
            }
        }
        if (out) {
            close(1);
            if (open(out, O_WRONLY | O_APPEND | O_CREAT,0600) != 1) {
                syslog(LOG_WARNING, "Reopen of stdout to %s failed", out);
                _exit(-1);
            }
        }
        if (err) {
            close(2);
            if (open(err, O_WRONLY | O_APPEND | O_CREAT,0600) != 2) {
                syslog(LOG_WARNING, "Reopen of stderr to %s failed", err);
                _exit(-1);
            }
        }
        rc = execv(args[0],args);
        syslog(LOG_WARNING, "Exec \"%s\" returned %d", args[0], rc);
        _exit(-1);
    }

    do {
        rc = wait(&status);
    } while (((rc > 0) && (rc != pid)) || ((rc == -1) && (errno == EINTR)));

    return 0;
}
#endif



#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)
static void fail_exit(int status)
{
//	gr_unlock();
#ifdef SHADOWGRP
	if (is_shadow_grp)
		sgr_unlock();
#endif
#ifdef SHADOW_PASSWORD
	if (is_shadow_pwd)
		spw_unlock();
#endif
	pw_unlock();
	exit(status);
}



static void oom(void)
{
	fprintf(stderr, "mbpasswd: out of memory\n");
	fail_exit(E_FAILURE);
}



/*
 * insert_crypt_passwd - add an "old-style" password to authentication string
 * result now malloced to avoid overflow, just in case.  --marekm
 */
static char *insert_crypt_passwd(const char *string, char *passwd)
{
#ifdef AUTH_METHODS
        if (string && *string) {
                char *cp, *result;

                result = xmalloc(strlen(string) + strlen(passwd) + 1);
                cp = result;
                while (*string) {
                        if (string[0] == ';') {
                                *cp++ = *string++;
                        } else if (string[0] == '@') {
                                while (*string && *string != ';')
                                        *cp++ = *string++;
                        } else {
                                while (*passwd)
                                        *cp++ = *passwd++;
                                while (*string && *string != ';')
                                        string++;
                        }
                }
                *cp = '\0';
                return result;
        }
#endif
        return xstrdup(passwd);
}


static char *update_crypt_pw(char *cp)
{
	if (do_update_pwd)
		cp = insert_crypt_passwd(cp, crypt_passwd);

	return cp;
}



/*
 * pwd_init - ignore signals, and set resource limits to safe
 * values.  Call this before modifying password files, so that
 * it is less likely to fail in the middle of operation.
 */
void pwd_init(void)
{
#ifdef HAVE_SYS_RESOURCE_H
	struct rlimit rlim;

#ifdef RLIMIT_CORE
	rlim.rlim_cur = rlim.rlim_max = 0;
	setrlimit(RLIMIT_CORE, &rlim);
#endif
	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
#ifdef RLIMIT_AS
	setrlimit(RLIMIT_AS, &rlim);
#endif
#ifdef RLIMIT_CPU
	setrlimit(RLIMIT_CPU, &rlim);
#endif
#ifdef RLIMIT_DATA
	setrlimit(RLIMIT_DATA, &rlim);
#endif
#ifdef RLIMIT_FSIZE
	setrlimit(RLIMIT_FSIZE, &rlim);
#endif
#ifdef RLIMIT_NOFILE
	setrlimit(RLIMIT_NOFILE, &rlim);
#endif
#ifdef RLIMIT_RSS
	setrlimit(RLIMIT_RSS, &rlim);
#endif
#ifdef RLIMIT_STACK
	setrlimit(RLIMIT_STACK, &rlim);
#endif
#endif /* !HAVE_SYS_RESOURCE_H */

	signal(SIGALRM, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	signal(SIGINT,  SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
#ifdef SIGTSTP
	signal(SIGTSTP, SIG_IGN);
#endif
#ifdef SIGTTOU
	signal(SIGTTOU, SIG_IGN);
#endif
  
	umask(077);
}
#endif /* not FreeBSD && NetBSD */


/*
 * isexpired - determine if account is expired yet
 *
 *      isexpired calculates the expiration date based on the
 *      password expiration criteria.
 */
#ifdef  SHADOW_PASSWORD
int isexpired(const struct passwd *pw, const struct spwd *sp)
{
#else
int isexpired(const struct passwd *pw)
{
#endif
        long    now;
#ifdef  HAVE_USERSEC_H
        int     minage = 0;
        int     maxage = 10000;
        int     curage = 0;
        struct  userpw  *pu;
#endif

        now = time ((time_t *) 0) / SCALE;

#ifdef  SHADOW_PASSWORD

        if (!sp)
                sp = pwd_to_spwd(pw);

        /*
         * Quick and easy - there is an expired account field
         * along with an inactive account field.  Do the expired
         * one first since it is worse.
         */
        if (sp->sp_expire > 0 && now >= sp->sp_expire)
                return 3;

        /*
         * Last changed date 1970-01-01 (not very likely) means that
         * the password must be changed on next login (passwd -e).
         *
         * The check for "x" is a workaround for RedHat NYS libc bug -
         * if /etc/shadow doesn't exist, getspnam() still succeeds and
         * returns sp_lstchg==0 (must change password) instead of -1!
         */
        if (sp->sp_lstchg == 0 && !strcmp(pw->pw_passwd, SHADOW_PASSWD_STRING))
                return 1;
        if (sp->sp_lstchg > 0 && sp->sp_max >= 0 && sp->sp_inact >= 0 &&
                        now >= sp->sp_lstchg + sp->sp_max + sp->sp_inact)
                return 2;
#endif
#ifdef  HAVE_USERSEC_H  /*{*/
        /*
         * The aging information lives someplace else.  Get it from the
         * login.cfg file
         */

        if (getconfattr (SC_SYS_PASSWD, SC_MINAGE, &minage, SEC_INT))
                minage = -1;

        if (getconfattr (SC_SYS_PASSWD, SC_MAXAGE, &maxage, SEC_INT))
                maxage = -1;

        pu = getuserpw (pw->pw_name);
        curage = (time (0) - pu->upw_lastupdate) / (7*86400L);

        if (maxage != -1 && curage > maxage)
                return 1;
#else   /*} !HAVE_USERSEC_H */

        /*
         * The last and max fields must be present for an account
         * to have an expired password.  A maximum of >10000 days
         * is considered to be infinite.
         */

#ifdef  SHADOW_PASSWORD
        if (sp->sp_lstchg == -1 ||
                        sp->sp_max == -1 || sp->sp_max >= (10000L*DAY/SCALE))
                return 0;
#endif
#ifdef  ATT_AGE
        if (pw->pw_age[0] == '\0' || pw->pw_age[0] == '/')
                return 0;
#endif

        /*
         * Calculate today's day and the day on which the password
         * is going to expire.  If that date has already passed,
         * the password has expired.
         */

#ifdef  SHADOW_PASSWORD
        if (now >= sp->sp_lstchg + sp->sp_max)
                return 1;
#endif
#ifdef  ATT_AGE
        if (a64l (pw->pw_age + 2) + c64i (pw->pw_age[1]) < now / 7)
                return 1;
#endif
#endif  /*} HAVE_USERSEC_H */
        return 0;
}



/*
 * check_password - test a password to see if it can be changed
 *
 *      check_password() sees if the invoker has permission to change the
 *      password for the given user.
 */
#ifdef SHADOW_PASSWORD
static void check_password(const struct passwd *pw, const struct spwd *sp)
{
#else
static void check_password(const struct passwd *pw)
{
#endif
	time_t now, last, ok;
	int exp_status;
#ifdef HAVE_USERSEC_H
	struct userpw *pu;
#endif

#ifdef SHADOW_PASSWORD
	exp_status = isexpired(pw, sp);
#else
	exp_status = isexpired(pw);
#endif

	now = time(NULL);

#ifdef SHADOW_PASSWORD
	/*
	 * If the force flag is set (for new users) this check is skipped.
	 */
	if (force == 1)
		return;

	/*
	 * Expired accounts cannot be changed ever.  Passwords
	 * which are locked may not be changed.  Passwords where
	 * min > max may not be changed.  Passwords which have
	 * been inactive too long cannot be changed.
	 */
	if (sp->sp_pwdp[0] == '!' || exp_status > 1 ||
		(sp->sp_max >= 0 && sp->sp_min > sp->sp_max)) {
		fprintf (stderr, "The password for %s cannot be changed.\n", sp->sp_namp);
		syslog(LOG_WARNING, "password locked for %s", sp->sp_namp);
		closelog();
		exit (E_NOPERM);
	}

	/*
	 * Passwords may only be changed after sp_min time is up.
	 */

	last = sp->sp_lstchg * SCALE;
	ok = last + (sp->sp_min > 0 ? sp->sp_min * SCALE : 0);
#else /* !SHADOW_PASSWORD */
	if (pw->pw_passwd[0] == '!' || exp_status > 1) {
		fprintf (stderr, "The password for %s cannot be changed.\n", pw->pw_name);
		syslog(LOG_WARNING, "password locked for %s", pw->pw_name);
		closelog();
		exit (E_NOPERM);
	}
#ifdef ATT_AGE
        /*
         * Can always be changed if there is no age info
         */

        if (! pw->pw_age[0])
                return;

        last = a64l (pw->pw_age + 2) * WEEK;
        ok = last + c64i (pw->pw_age[1]) * WEEK;
#else   /* !ATT_AGE */
#ifdef HAVE_USERSEC_H
        pu = getuserpw(pw->pw_name);
        last = pu ? pu->upw_lastupdate : 0L;
        ok = last + (minage > 0 ? minage * WEEK : 0);
#else
        last = 0;
        ok = 0;
#endif
#endif /* !ATT_AGE */
#endif /* !SHADOW_PASSWORD */
	if (now < ok) {
		fprintf(stderr, "Sorry, the password for %s cannot be changed yet.\n", pw->pw_name);
		syslog(LOG_WARNING, "now < minimum age for `%s'", pw->pw_name);
		closelog();
		exit (E_NOPERM);
	}
}



/*
 * pwd_to_spwd - create entries for new spwd structure
 *
 *      pwd_to_spwd() creates a new (struct spwd) containing the
 *      information in the pointed-to (struct passwd).
 *
 * This function is borrowed from the Shadow Password Suite.
 */
#ifdef SHADOW_PASSWORD
struct spwd *pwd_to_spwd(const struct passwd *pw)
{
        static struct spwd sp;

        /*
         * Nice, easy parts first.  The name and passwd map directly
         * from the old password structure to the new one.
         */
        sp.sp_namp = pw->pw_name;
        sp.sp_pwdp = pw->pw_passwd;

#ifdef ATT_AGE
        /*
         * AT&T-style password aging maps the sp_min, sp_max, and
         * sp_lstchg information from the pw_age field, which appears
         * after the encrypted password.
         */
        if (pw->pw_age[0]) {
                sp.sp_max = (c64i(pw->pw_age[0]) * WEEK) / SCALE;

                if (pw->pw_age[1])
                        sp.sp_min = (c64i(pw->pw_age[1]) * WEEK) / SCALE;
                else
                        sp.sp_min = (10000L * DAY) / SCALE;

                if (pw->pw_age[1] && pw->pw_age[2])
                        sp.sp_lstchg = (a64l(pw->pw_age + 2) * WEEK) / SCALE;
                else
                        sp.sp_lstchg = time((time_t *) 0) / SCALE;
        } else
#endif
	{
        	/*
        	 * Defaults used if there is no pw_age information.
        	 */
        	sp.sp_min = 0;
        	sp.sp_max = (10000L * DAY) / SCALE;
        	sp.sp_lstchg = time((time_t *) 0) / SCALE;
	}

        /*
         * These fields have no corresponding information in the password
         * file.  They are set to uninitialized values.
         */
        sp.sp_warn = -1;
        sp.sp_expire = -1;
        sp.sp_inact = -1;
        sp.sp_flag = -1;

        return &sp;
}
#endif



/*
 * new_password - validate old password and replace with new
 * (both old and new in global "char crypt_passwd[128]")
 */
static int new_password(const struct passwd *pw, char *newpasswd)
{
	char    *cp;            /* Pointer to getpass() response */
	char    pass[200];      /* New password */
#ifdef HAVE_LIBCRACK_HIST
	int HistUpdate P_((const char *, const char *));
#endif

	snprintf(pass, 200, "%s", newpasswd);

	/*
	 * Encrypt the password, then wipe the cleartext password.
	 */
	cp = pw_encrypt(pass, crypt_make_salt());
	memset(&pass, 0, sizeof(pass));

#ifdef HAVE_LIBCRACK_HIST
	HistUpdate(pw->pw_name, crypt_passwd);
#endif
	STRFCPY(crypt_passwd, cp);
	return 0;
}


#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)

static void update_noshadow(int shadow_locked)
{
	const struct passwd	*pw;
	struct passwd		*npw;
#ifdef ATT_AGE
        char			age[5];
        long			week = time((time_t *) 0) / WEEK;
        char			*cp;
#endif

	/*
	 * call this with shadow_locked != 0 to avoid calling lckpwdf()
	 * twice (which will fail).  XXX - pw_lock(), pw_unlock(),
	 * spw_lock(), spw_unlock() really should track the lock count
	 * and call lckpwdf() only before the first lock, and ulckpwdf()
	 * after the last unlock.
	 */
	if (!pw_lock()) {
		fprintf(stderr, "Cannot lock the password file; try again later.\n");
		syslog(LOG_WARNING, "can't lock password file");
		exit(E_PWDBUSY);
	}
	if (!pw_open(O_RDWR)) {
		fprintf(stderr, "Cannot open the password file.\n");
		syslog(LOG_ERR, "can't open password file");
		fail_exit(E_MISSING);
	}
	pw = pw_locate(name);
	if (!pw) {
		fprintf(stderr, "mbpasswd: user %s not found in /etc/passwd\n", name);
		fail_exit(E_NOPERM);
	}
	npw = __pw_dup(pw);
	if (!npw)
		oom();
	npw->pw_passwd = update_crypt_pw(npw->pw_passwd);
#ifdef ATT_AGE
        memset(age, 0, sizeof(age));
        STRFCPY(age, npw->pw_age);

        /*
         * Just changing the password - update the last change date
         * if there is one, otherwise the age just disappears.
         */
        if (do_update_age) {
                if (strlen(age) > 2) {
                        cp = l64a(week);
                        age[2] = cp[0];
                        age[3] = cp[1];
                } else {
                        age[0] = '\0';
                }
        }

        if (xflg) {
                if (age_max > 0)
                        age[0] = i64c((age_max + 6) / 7);
                else
                        age[0] = '.';

                if (age[1] == '\0')
                        age[1] = '.';
        }
        if (nflg) {
                if (age[0] == '\0')
                        age[0] = 'z';

                if (age_min > 0)
                        age[1] = i64c((age_min + 6) / 7);
                else
                        age[1] = '.';
        }
        /*
         * The last change date is added by -n or -x if it's
         * not already there.
         */
        if ((nflg || xflg) && strlen(age) <= 2) {
                cp = l64a(week);
                age[2] = cp[0];
                age[3] = cp[1];
        }

        /*
         * Force password change - if last change date is
         * present, it will be set to (today - max - 1 week).
         * Otherwise, just set min = max = 0 (will disappear
         * when password is changed).
         */
        if (eflg) {
                if (strlen(age) > 2) {
                        cp = l64a(week - c64i(age[0]) - 1);
                        age[2] = cp[0];
                        age[3] = cp[1];
                } else {
                        strcpy(age, "..");
                }
        }

        npw->pw_age = age;
#endif

	if (!pw_update(npw)) {
		fprintf(stderr, "Error updating the password entry.\n");
		syslog(LOG_ERR, "error updating password entry");
		fail_exit(E_FAILURE);
	}
#ifdef NDBM
        if (pw_dbm_present() && !pw_dbm_update(npw)) {
                fprintf(stderr, _("Error updating the DBM password entry.\n"));
                SYSLOG((LOG_ERR, DBMERROR2));
                fail_exit(E_FAILURE);
        }
        endpwent();
#endif
	if (!pw_close()) {
		fprintf(stderr, "Cannot commit password file changes.\n");
		syslog(LOG_ERR, "can't rewrite password file");
		fail_exit(E_FAILURE);
	}
        pw_unlock();
}

#endif /* Not __FreeBSD__ && __NetBSD__ && __OpenBSD__ */


#ifdef SHADOW_PASSWORD
static void update_shadow(void)
{
        const struct spwd *sp;
        struct spwd *nsp;

        if (!spw_lock()) {
                fprintf(stderr, "Cannot lock the password file; try again later.\n");
                syslog(LOG_WARNING, "can't lock password file");
                exit(E_PWDBUSY);
        }
        if (!spw_open(O_RDWR)) {
                fprintf(stderr, "Cannot open the password file.\n");
                syslog(LOG_ERR, "can't open password file");
                fail_exit(E_FAILURE);
        }
        sp = spw_locate(name);
        if (!sp) {
                /* Try to update the password in /etc/passwd instead.  */
                spw_close();
                update_noshadow(1);
		spw_unlock();
                return;
        }
        nsp = __spw_dup(sp);
        if (!nsp)
                oom();
        nsp->sp_pwdp = update_crypt_pw(nsp->sp_pwdp);
        if (do_update_age)
                nsp->sp_lstchg = time((time_t *) 0) / SCALE;

        if (!spw_update(nsp)) {
                fprintf(stderr, "Error updating the password entry.\n");
                syslog(LOG_ERR, "error updating password entry");
                fail_exit(E_FAILURE);
        }
#ifdef NDBM
        if (sp_dbm_present() && !sp_dbm_update(nsp)) {
                fprintf(stderr, _("Error updating the DBM password entry.\n"));
                SYSLOG((LOG_ERR, DBMERROR2));
                fail_exit(E_FAILURE);
        }
        endspent();
#endif
        if (!spw_close()) {
                fprintf(stderr, "Cannot commit password file changes.\n");
                syslog(LOG_ERR, "can't rewrite password file");
                fail_exit(E_FAILURE);
        }
        spw_unlock();
}
#endif  /* SHADOWPWD */



/*
 *  Internal version of basename to make this better portable.
 */
char *Basename(char *str)
{
    char *cp = strrchr(str, '/');

    return cp ? cp+1 : str;
}



/*
 * Function will set a new password in the users password file.
 * Note that this function must run setuid root!
 */
int main(int argc, char *argv[])
{
#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)
    const struct passwd	    *pw;
    const struct group	    *gr;
#ifdef SHADOW_PASSWORD
    const struct spwd	    *sp;
#endif
#else
    static struct passwd    *pw;
    static struct group	    *gr;
    int			    pfd, tfd;
#endif
#ifdef _VPOPMAIL_PATH
    char		    *args[16];
#endif
    pid_t		    ppid;
    char		    *parent;
#if defined(__OpenBSD__)
#define ARG_SIZE 60
    static char		    **s, buf[ARG_SIZE];
    size_t		    siz = 100;
    char		    **p;
    int			    mib[4];
#elif defined(__NetBSD__) || defined(FreeBSD_sysctl)
    static char             **s;
    size_t                  siz = 100;
    int                     mib[4];
#else
    char                    temp[PATH_MAX];
    FILE		    *fp;
#endif

    /*
     * Init $MBSE_ROOT/etc/login.defs file before the *pw gets overwritten.
     */
    def_load();

    /*
     * Get my username
     */
    pw = get_my_pwent();
    if (!pw) {
	fprintf(stderr, "mbpasswd: Cannot determine your user name.\n");
	exit(1);
    }
    myname = xstrdup(pw->pw_name);

    /*
     * Get my groupname, this must be "bbs", other users may not
     * use this program, not even root.
     */
    gr = getgrgid(pw->pw_gid);
    if (!gr) {
	fprintf(stderr, "mbpasswd: Cannot determine group name.\n");
	free(myname);
	exit(E_NOPERM);
    }
    if (strcmp(gr->gr_name, (char *)"bbs")) {
	fprintf(stderr, "mbpasswd: You are not a member of group \"bbs\".\n");
	free(myname);
	exit(E_NOPERM);
    }

    /*
     * We don't log into MBSE BBS logfiles but to the system logfiles,
     * because we are modifying system files not belonging to MBSE BBS.
     */
    openlog("mbpasswd", LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);

    /*
     * Find out the name of our parent.
     */
    ppid = getppid();

#if defined(__OpenBSD__)
    /*
     * Systems that use sysctl to get process information
     */
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC_ARGS;
    mib[2] = ppid;
    mib[3] = KERN_PROC_ARGV; 
    if ((s = realloc(s, siz)) == NULL) { 
	fprintf(stderr, "mbpasswd: no memory\n");
	exit(1);
    }
    if (sysctl(mib, 4, s, &siz, NULL, 0) == -1) {
	perror("");
	fprintf(stderr, "mbpasswd: sysctl call failed\n");
	exit(1);
    }
    buf[0] = '\0';
    for (p = s; *p != NULL; p++) {
	if (p != s)
	    strlcat(buf, " ", sizeof(buf));
	strlcat(buf, *p, sizeof(buf));
    }
    parent = xstrcpy(buf);
#elif defined(__NetBSD__)
    /*
     * Systems that use sysctl to get process information
     */
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC_ARGS;
    mib[2] = ppid;
    mib[3] = KERN_PROC_ARGV;
    if ((s = realloc(s, siz)) == NULL) {
        fprintf(stderr, "mbpasswd: no memory\n");
        exit(1);
    }
    if (sysctl(mib, 4, s, &siz, NULL, 0) == -1) {
        perror("");
        fprintf(stderr, "mbpasswd: sysctl call failed\n");
        exit(1);
    }
    parent = xstrcpy((char *)s);
#elif defined(FreeBSD_sysctl)
    /*
     * For FreeBSD 6.0 and later.
     */
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_ARGS;
    mib[3] = ppid;
    if ((s = realloc(s, siz)) == NULL) {
        fprintf(stderr, "mbpasswd: no memory\n");
        exit(1);
    }
    if (sysctl(mib, 4, s, &siz, NULL, 0) == -1) {
        perror("");
        fprintf(stderr, "mbpasswd: sysctl call failed\n");
        exit(1);
    }
    parent = xstrcpy((char *)s);
#else
    /*
     *  Systems with /proc filesystem like Linux, old FreeBSD
     */
    snprintf(temp, PATH_MAX, "/proc/%d/cmdline", ppid);
    if ((fp = fopen(temp, "r")) == NULL) {
	fprintf(stderr, "mbpasswd: can't read %s\n", temp);
	syslog(LOG_ERR, "mbpasswd: can't read %s", temp);
	free(myname);
	exit(E_FAILURE);
    }
    fgets(temp, PATH_MAX-1, fp);
    fclose(fp);
    parent = xstrcpy(Basename(temp));
#endif

    if (strcmp((char *)"mbsetup", parent) && strcmp((char *)"-mbsebbs", parent) && strcmp((char *)"-mbnewusr", parent)) {
	fprintf(stderr, "mbpasswd: illegal parent \"%s\"\n", parent);
	syslog(LOG_ERR, "mbpasswd: illegal parent \"%s\"", parent);
	free(myname);
	exit(E_FAILURE);
    }

    if (argc != 3) {
	fprintf(stderr, "\nmbpasswd commandline:\n\n");
	fprintf(stderr, "mbpasswd [username] [newpassword]\n");
	free(myname);
	exit(E_FAILURE);
    }

    if ((strcmp((char *)"-mbnewusr", parent) == 0) || (strcmp((char *)"mbsetup", parent) == 0)) {
	/*
	 * This is a new user setting his password, or the sysop resetting
	 * the password using mbsetup. This program runs under account mbse.
	 */
	force = 1;
	if (strcmp(pw->pw_name, (char *)"mbse") && strcmp(pw->pw_name, (char *)"bbs")) {
	    fprintf(stderr, "mbpasswd: only users `mbse' and `bbs' may do this.\n");
	    free(myname);
	    exit(E_NOPERM);
	}
    } else if (strcmp((char *)"-mbsebbs", parent) == 0) {
	/*
	 * Normal password change by user, check caller is the user.
	 * Calling program is only mbsebbs.
	 */
	force = 0;
	if (strcmp(pw->pw_name, argv[1])) {
	    fprintf(stderr, "mbpasswd: only owner may do this.\n");
	    free(myname);
	    exit(E_NOPERM);
	}
    } else {
	syslog(LOG_ERR, "mbpasswd: illegal called");
	free(myname);
	exit(E_NOPERM);
    }

    /*
     *  Check commandline arguments
     */
    if (strlen(argv[1]) > 32) {
	fprintf(stderr, "mbpasswd: Username too long\n");
	free(myname);
	exit(E_FAILURE);
    }
    if (strlen(argv[2]) > 32) {
	fprintf(stderr, "mbpasswd: Password too long\n");
	free(myname);
	exit(E_FAILURE);
    }

    name = strdup(argv[1]);
    if ((pw = getpwnam(name)) == NULL) {
	syslog(LOG_ERR, "mbpasswd: Unknown user %s", name);
	fprintf(stderr, "mbpasswd: Unknown user %s\n", name);
	free(myname);
	exit(E_FAILURE);
    }


#ifdef SHADOW_PASSWORD
    is_shadow_pwd = spw_file_present();

    sp = getspnam(name);
    if (!sp)
	sp = pwd_to_spwd(pw);
#endif

    /*
     * See if the user is permitted to change the password.
     * Otherwise, go ahead and set a new password.
     */
#ifdef SHADOW_PASSWORD
    check_password(pw, sp);
#else
    check_password(pw);
#endif

    if (new_password(pw, argv[2])) {
	fprintf(stderr, "The password for %s is unchanged.\n", name);
	syslog(LOG_ERR, "The password for %s is unchanged", name);
	closelog();
	free(myname);
	exit(E_FAILURE);
    }
    do_update_pwd = 1;
    do_update_age = 1;

    /*
     * Before going any further, raise the ulimit to prevent
     * colliding into a lowered ulimit, and set the real UID
     * to root to protect against unexpected signals.  Any
     * keyboard signals are set to be ignored.
     */
#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)
    pwd_init();
#else
    pw_init();
#endif

    if (setuid(0)) {
	fprintf(stderr, "Cannot change ID to root.\n");
	syslog(LOG_ERR, "can't setuid(0)");
	closelog();
	free(myname);
	exit(E_FAILURE);
    }

#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)

#ifdef  HAVE_USERSEC_H
    update_userpw(pw->pw_passwd);
#else  /* !HAVE_USERSEC_H */

#ifdef SHADOW_PASSWORD
    if (spw_file_present())
	update_shadow();
    else
#endif
	update_noshadow(0);
#endif /* !HAVE_USERSEC_H */

#else /* __FreeBSD__ && __NetBSD__ && __OpenBSD__ */
    /*
     *  FreeBSD password change, borrowed from the original FreeBSD sources
     */

    /*
     * Get the new password.  Reset passwd change time to zero by
     * default.
     */
    pw->pw_change = 0;
    pw->pw_passwd = crypt_passwd;

    pfd = pw_lock();
    tfd = pw_tmp();
    pw_copy(pfd, tfd, pw);

    if (!pw_mkdb(pw->pw_name))
	pw_error((char *)NULL, 0, 1);

#endif /* __FreeBSD__ && __NetBSD__ && __OpenBSD__ */

#ifdef _VPOPMAIL_PATH
    fflush(stdout);
    fflush(stdin);
    memset(args, 0, sizeof(args));
    
    snprintf(temp, PATH_MAX, "%s/vpasswd", (char *)_VPOPMAIL_PATH);
    args[0] = temp;
    args[1] = argv[1];
    args[2] = argv[2];
    args[3] = NULL;

    if (execute(args, (char *)"/dev/tty", (char *)"/dev/tty", (char *)"/dev/tty") != 0) {
	perror("mbpasswd: Failed to change vpopmail password\n");
	syslog(LOG_ERR, "Failed to change vpopmail password");
    }
#endif
	
    syslog(LOG_NOTICE, "password for `%s' changed by user `%s'", name, myname);
    closelog();
    free(myname);
    exit(E_SUCCESS);
}


