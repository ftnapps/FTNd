/*****************************************************************************
 *
 * File ..................: mbpasswd.c
 * Purpose ...............: setuid root version of passwd
 * Last modification date : 27-Jun-2001
 * Shadow Suite (c) ......: Julianne Frances Haugh
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include <stdio.h>
#include <sys/resource.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#if defined(SHADOW_PASSWORD)
#include <shadow.h>
#endif

#include "encrypt.h"
#include "rad64.h"
#include "myname.h"
#include "xmalloc.h"
#include "pwio.h"
#include "shadowio.h"
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



static void fail_exit(int status)
{
	pw_unlock();
#ifdef SHADOWPWD
	spw_unlock();
#endif
	exit(status);
}



static void oom(void)
{
	fprintf(stderr, "mbpasswd: out of memory\n");
	fail_exit(3);
}



/*
 * insert_crypt_passwd - add an "old-style" password to authentication string
 * result now malloced to avoid overflow, just in case.  --marekm
 */
static char *
insert_crypt_passwd(const char *string, char *passwd)
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
	signal(SIGALRM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
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

        /*
         * The last and max fields must be present for an account
         * to have an expired password.  A maximum of >10000 days
         * is considered to be infinite.
         */
        if (sp->sp_lstchg == -1 ||
                        sp->sp_max == -1 || sp->sp_max >= (10000L*DAY/SCALE))
                return 0;

        /*
         * Calculate today's day and the day on which the password
         * is going to expire.  If that date has already passed,
         * the password has expired.
         */
        if (now >= sp->sp_lstchg + sp->sp_max)
                return 1;
#endif
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

#ifdef SHADOW_PASSWORD
	exp_status = isexpired(pw, sp);
#else
	exp_status = isexpired(pw);
#endif

	time(&now);

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
		exit (1);
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
		exit (1);
	}
	last = 0;
	ok = 0;
#endif /* !SHADOW_PASSWORD */
	if (now < ok) {
		fprintf(stderr, "Sorry, the password for %s cannot be changed yet.\n", pw->pw_name);
		syslog(LOG_WARNING, "now < minimum age for `%s'", pw->pw_name);
		closelog();
		exit (1);
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

        /*
         * Defaults used if there is no pw_age information.
         */
        sp.sp_min = 0;
        sp.sp_max = (10000L * DAY) / SCALE;
        sp.sp_lstchg = time((time_t *) 0) / SCALE;

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

	sprintf(pass, "%s", newpasswd);

	/*
	 * Encrypt the password, then wipe the cleartext password.
	 */
	cp = pw_encrypt(pass, crypt_make_salt());
	bzero(pass, sizeof pass);

#ifdef HAVE_LIBCRACK_HIST
	HistUpdate(pw->pw_name, crypt_passwd);
#endif
	STRFCPY(crypt_passwd, cp);
	return 0;
}



static void update_noshadow(int shadow_locked)
{
	const struct passwd *pw;
	struct passwd *npw;

	/*
	 * call this with shadow_locked != 0 to avoid calling lckpwdf()
	 * twice (which will fail).  XXX - pw_lock(), pw_unlock(),
	 * spw_lock(), spw_unlock() really should track the lock count
	 * and call lckpwdf() only before the first lock, and ulckpwdf()
	 * after the last unlock.
	 */
	if (!(shadow_locked ? pw_lock() : pw_lock_first())) {
	fprintf(stderr, "Cannot lock the password file; try again later.\n");
		syslog(LOG_WARNING, "can't lock password file");
		exit(5);
	}
	if (!pw_open(O_RDWR)) {
		fprintf(stderr, "Cannot open the password file.\n");
		syslog(LOG_ERR, "can't open password file");
		fail_exit(3);
	}
	pw = pw_locate(name);
	if (!pw) {
		fprintf(stderr, "mbpasswd: user %s not found in /etc/passwd\n", name);
		fail_exit(1);
	}
	npw = __pw_dup(pw);
	if (!npw)
		oom();
	npw->pw_passwd = update_crypt_pw(npw->pw_passwd);
	if (!pw_update(npw)) {
		fprintf(stderr, "Error updating the password entry.\n");
		syslog(LOG_ERR, "error updating password entry");
		fail_exit(3);
	}
	if (!pw_close()) {
		fprintf(stderr, "Cannot commit password file changes.\n");
		syslog(LOG_ERR, "can't rewrite password file");
		fail_exit(3);
	}
        pw_unlock();
}



#ifdef SHADOW_PASSWORD
static void update_shadow(void)
{
        const struct spwd *sp;
        struct spwd *nsp;

        if (!spw_lock_first()) {
                fprintf(stderr, "Cannot lock the password file; try again later.\n");
                syslog(LOG_WARNING, "can't lock password file");
                exit(5);
        }
        if (!spw_open(O_RDWR)) {
                fprintf(stderr, "Cannot open the password file.\n");
                syslog(LOG_ERR, "can't open password file");
                fail_exit(3);
        }
        sp = spw_locate(name);
        if (!sp) {
#if 0
                fprintf(stderr, "%s: user %s not found in /etc/shadow\n",
                        Prog, name);
                fail_exit(1);
#else
                /* Try to update the password in /etc/passwd instead.  */
                spw_unlock();
                update_noshadow(1);
                return;
#endif
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
                fail_exit(3);
        }
        if (!spw_close()) {
                fprintf(stderr, "Cannot commit password file changes.\n");
                syslog(LOG_ERR, "can't rewrite password file");
                fail_exit(3);
        }
        spw_unlock();
}
#endif  /* SHADOWPWD */



/*
 * Function will set a new password in the users password file.
 * Note that this function must run setuid root!
 */
int main(int argc, char *argv[])
{
	const struct passwd	*pw;
	const struct group	*gr;
#ifdef SHADOW_PASSWORD
	const struct spwd	*sp;
#endif
	char		*cp;

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
		exit(1);
	}
	if (strcmp(gr->gr_name, (char *)"bbs")) {
		fprintf(stderr, "mbpasswd: You are not a member of group \"bbs\".\n");
		exit(1);
	}

//	NOOT dit programma moet kontroleren of het is aangeroepen door mbsebbs.
//	     ook kontroleren of de originele uid en gid correct zijn.
//           Gewone stervelingen mogen dit niet kunnen starten.
//	     Dit programma is een groot security gat.

	if (argc != 4) {
		printf("\nmbpasswd commandline:\n\n");
		printf("mbpasswd [-opt] [username] [newpassword]\n");
		printf("options are:  -n   normal password change\n");
		printf("              -f   forced password change\n");
		exit(1);
	}

	if (strncmp(argv[1], "-f", 2) == 0)
		force = 1;
	else
		force = 0;

	/*
	 *  Check stringlengths
	 */
	if (strlen(argv[2]) > 16) {
		fprintf(stderr, "mbpasswd: Username too long\n");
		exit(1);
	}
	if (strlen(argv[3]) > 16) {
		fprintf(stderr, "mbpasswd: Password too long\n");
		exit(1);
	}

	name = strdup(argv[2]);
	if ((pw = getpwnam(name)) == NULL) {
		fprintf(stderr, "mbpasswd: Unknown user %s\n", name);
		exit(1);
	}

	openlog("mbpasswd", LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);

#ifdef SHADOW_PASSWORD
	sp = getspnam(name);
	if (!sp)
		sp = pwd_to_spwd(pw);

	cp = sp->sp_pwdp;
#else
	cp = pw->pw_passwd;
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

	if (new_password(pw, argv[3])) {
		fprintf(stderr, "The password for %s is unchanged.\n", name);
		closelog();
		exit(1);
	}
	do_update_pwd = 1;
	do_update_age = 1;

	/*
	 * Before going any further, raise the ulimit to prevent
	 * colliding into a lowered ulimit, and set the real UID
	 * to root to protect against unexpected signals.  Any
	 * keyboard signals are set to be ignored.
	 */
	pwd_init();

	if (setuid(0)) {
		fprintf(stderr, "Cannot change ID to root.\n");
		syslog(LOG_ERR, "can't setuid(0)");
		closelog();
		exit(1);
	}

#ifdef SHADOW_PASSWORD
	if (spw_file_present())
		update_shadow();
	else
#endif
		update_noshadow(0);

	syslog(LOG_INFO, "password for `%s' changed by user `%s'", name, myname);
	closelog();
	exit(0);
}


