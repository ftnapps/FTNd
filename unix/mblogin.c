/*****************************************************************************
 *
 * $Id: mblogin.c,v 1.9 2007/08/26 15:05:33 mbse Exp $
 * Purpose ...............: Login program for MBSE BBS.
 * Shadow Suite (c) ......: Julianne Frances Haugh
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
 *   
 * Michiel Broek        FIDO:           2:280/2802
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
#include "mblogin.h"
#include "../lib/users.h"
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <pwd.h>
#include <grp.h>
#if defined(SHADOW_PASSWORD)
#include <shadow.h>
#endif
#if defined(__NetBSD__)
#undef HAVE_UTMPX_H
#endif
#if HAVE_UTMPX_H
#include <utmpx.h>
#endif
#include <utmp.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include "getdef.h"

#ifdef SVR4_SI86_EUA
#include <sys/proc.h>
#include <sys/sysi86.h>
#endif

#ifdef UT_ADDR
#include <netdb.h>
#endif

#include "env.h"
#include "chowntty.h"
#include "basename.h"
#include "shell.h"
#include "pwdcheck.h"
#include "pwauth.h"
#include "loginprompt.h"
#include "utmp.h"
#include "limits.h"
#include "setupenv.h"
#include "sub.h"
#include "log.h"
#include "setugid.h"


/*
 * Login parameters
 */
#define	LOGIN_DELAY	3
#define	LOGIN_TIMEOUT	300
#define	LOGIN_RETRIES	10



/*
 * Needed for MkLinux DR1/2/2.1 - J.
 */
#ifndef LASTLOG_FILE
#define LASTLOG_FILE "/var/log/lastlog"
#endif

const char *hostname = "";

struct	passwd	pwent, *pw;
#if HAVE_UTMPX_H
struct	utmpx	utxent, failent;
struct	utmp	utent;
#else
struct	utmp	utent, failent;
#endif
struct	lastlog	lastlog;
static int preauth_flag = 0;

/*
 * Global variables.
 */

static int hflg = 0;
static int pflg = 0;
static char *Prog;
static int amroot;
static int timeout;

/*
 * External identifiers.
 */

extern char **newenvp;
extern size_t newenvc;
extern	int	optind;
extern	char	*optarg;
extern	char	**environ;


#ifndef	ALARM
#define	ALARM	60
#endif

#ifndef	RETRIES
#define	RETRIES	3
#endif


#define	NO_SHADOW	"no shadow password for `%s'%s\n"
#define	BAD_PASSWD	"invalid password for `%s'%s\n"
#define	BAD_DIALUP	"invalid dialup password for `%s' on `%s'\n"
#define	BAD_ROOT_LOGIN	"ILLEGAL ROOT LOGIN%s\n"
#define BAD_GROUP	"user `%s' not in group bbs\n"
#define	ROOT_LOGIN	"ROOT LOGIN%s\n"
#define	FAILURE_CNT	"exceeded failure limit for `%s'%s\n"
#define REG_LOGIN	"`%s' logged in%s\n"
#define LOGIN_REFUSED	"LOGIN `%s' REFUSED%s\n"
#define REENABLED2 \
	"login `%s' re-enabled after temporary lockout (%d failures).\n"
#define MANY_FAILS	"REPEATED login failures%s\n"

/* local function prototypes */
static void usage(void);
static void setup_tty(void);
static void check_flags(int, char * const *);
static void check_nologin(void);
static void init_env(void);
static RETSIGTYPE alarm_handler(int);
int main(int, char **);



/*
 * usage - print login command usage and exit
 *
 * login [ name ]
 * login -a hostname	(for NetBSD)
 * login -r hostname    (for rlogind)
 * login -h hostname    (for telnetd, etc.)
 * login -f name        (for pre-authenticated login: datakit, xterm, etc.)
 */

static void
usage(void)
{
    fprintf(stderr, _("usage: %s [-p] [name]\n"), Prog);
    if (!amroot)
	exit(1);
    fprintf(stderr, _("       %s [-p] [-h host] [-f name]\n"), Prog);
    exit(1);
}


static void setup_tty(void)
{
    TERMIO termio;

    GTTY(0, &termio);		/* get terminal characteristics */

    /*
     * Add your favorite terminal modes here ...
     */
    termio.c_lflag |= ISIG|ICANON|ECHO|ECHOE;
    termio.c_iflag |= ICRNL;

#if defined(ECHOKE) && defined(ECHOCTL)
    termio.c_lflag |= ECHOKE|ECHOCTL;
#endif
#if defined(ECHOPRT) && defined(NOFLSH) && defined(TOSTOP)
    termio.c_lflag &= ~(ECHOPRT|NOFLSH|TOSTOP);
#endif
#ifdef	ONLCR
    termio.c_oflag |= ONLCR;
#endif

#ifdef	SUN4

    /*
     * Terminal setup for SunOS 4.1 courtesy of Steve Allen
     * at UCO/Lick.
     */

    termio.c_cc[VEOF] = '\04';
    termio.c_cflag &= ~CSIZE;
    termio.c_cflag |= (PARENB|CS7);
    termio.c_lflag |= (ISIG|ICANON|ECHO|IEXTEN);
    termio.c_iflag |= (BRKINT|IGNPAR|ISTRIP|IMAXBEL|ICRNL|IXON);
    termio.c_iflag &= ~IXANY;
    termio.c_oflag |= (XTABS|OPOST|ONLCR);
#endif

    /* leave these values unchanged if not specified in login.defs */
    termio.c_cc[VERASE] = getdef_num("ERASECHAR", termio.c_cc[VERASE]);
    termio.c_cc[VKILL] = getdef_num("KILLCHAR", termio.c_cc[VKILL]);

    /*
     * ttymon invocation prefers this, but these settings won't come into
     * effect after the first username login 
     */

    STTY(0, &termio);
}



static void check_flags(int argc, char * const *argv)
{
    int arg;

    /*
     * Check the flags for proper form.  Every argument starting with
     * "-" must be exactly two characters long.  This closes all the
     * clever rlogin, telnet, and getty holes.
     */
    for (arg = 1; arg < argc; arg++) {
	if (argv[arg][0] == '-' && strlen(argv[arg]) > 2)
	    usage();
    }
}



static void check_nologin(void)
{
    char *fname;

    /*
     * Check to see if system is turned off for non-root users.
     * This would be useful to prevent users from logging in
     * during system maintenance.  We make sure the message comes
     * out for root so she knows to remove the file if she's
     * forgotten about it ...
     */

    fname = getdef_str("NOLOGINS_FILE");
    if (access(fname, F_OK) == 0) {
	FILE	*nlfp;
	int	c;

	/*
	 * Cat the file if it can be opened, otherwise just
	 * print a default message
	 */

	if ((nlfp = fopen (fname, "r"))) {
	    while ((c = getc (nlfp)) != EOF) {
		if (c == '\n')
		    putchar ('\r');

		putchar (c);
	    }
	    fflush (stdout);
	    fclose (nlfp);
	} else
	    printf("\nSystem closed for routine maintenance\n");

	free(fname);
	closelog();
	exit(0);
    }

    free(fname);
}



static void init_env(void)
{
    char    *cp, *tmp;

    if ((tmp = getenv("LANG"))) {
	addenv("LANG", tmp);
    }

    /*
     * Add the timezone environmental variable so that time functions
     * work correctly.
     */

    if ((tmp = getenv("TZ"))) {
	addenv("TZ", tmp);
    } 

    /* 
     * Add the clock frequency so that profiling commands work
     * correctly.
     */

    if ((tmp = getenv("HZ"))) {
	addenv("HZ", tmp);
    } else if ((cp = getdef_str("ENV_HZ"))) {
	addenv(cp, NULL);
    }
}



static RETSIGTYPE alarm_handler(int sig)
{
    fprintf(stderr, "\nLogin timed out after %d seconds.\n", timeout);
    exit(0);
}



/*
 * login - create a new login session for a user
 *
 *	login is typically called by getty as the second step of a
 *	new user session.  getty is responsible for setting the line
 *	characteristics to a reasonable set of values and getting
 *	the name of the user to be logged in.  login may also be
 *	called to create a new user session on a pty for a variety
 *	of reasons, such as X servers or network logins.
 */

int main(int argc, char **argv)
{
    char	    username[37];
    char	    tty[BUFSIZ];
    char	    userfile[PATH_MAX];
    FILE	    *ufp;
    int		    reason = PW_LOGIN;
    int		    delay;
    int		    retries;
    int		    failed;
    int		    flag;
    int		    subroot = 0;
    int		    is_console = 0;
    int		    FoundName;
    const char	    *cp;
    char	    *tmp;
    char	    fromhost[512];
    struct passwd   *pwd;
    char	    **envp = environ;
    static char	    temp_pw[2];
    static char	    temp_shell[] = "/bin/sh";
#ifdef	SHADOW_PASSWORD
	struct	spwd	*spwd=NULL;
#endif
#if defined(DES_RPC) || defined(KERBEROS)
    /* from pwauth.c */
    extern char	    *clear_pass;
    extern int	    wipe_clear_pass;

    /*
     * We may need the password later, don't want pw_auth() to wipe it
     * (we do it ourselves when it is no longer needed).  --marekm
     */
    wipe_clear_pass = 0;
#endif

    /*
     * Some quick initialization.
     */

    sanitize_env();

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    initenv();

    username[0] = '\0';
    amroot = (getuid() == 0);
    Prog = Basename(argv[0]);

    check_flags(argc, argv);

    while ((flag = getopt(argc, argv, "a:d:h:p")) != EOF) {
	switch (flag) {
	    case 'p':
		    pflg++;
		    break;
#ifdef __NetBSD__
	    case 'a':
#endif
	    case 'h':
		    hflg++;
		    hostname = optarg;
		    reason = PW_TELNET;
		    break;
	    case 'd':
		    /* "-d device" ignored for compatibility */
		    break;
	    default:
		    usage();
	}
    }

    /*
     * Allow authentication bypass only if real UID is zero.
     */
    if (hflg && !amroot) {
	fprintf(stderr, _("%s: permission denied\n"), Prog);
	exit(1);
    }

    if (!isatty(0) || !isatty(1) || !isatty(2))
	exit(1);		/* must be a terminal */

    if (hflg) {
	/*
	 * Only show this before a prompt from telnetd
	 */
	printf("\nMBSE BBS v%s (Release: %s)\n", VERSION, ReleaseDate);
	printf("%s\n\n", COPYRIGHT);
    }

    /*
     * Be picky if run by normal users (possible if installed setuid
     * root), but not if run by root.  This way it still allows logins
     * even if your getty is broken, or if something corrupts utmp,
     * but users must "exec login" which will use the existing utmp
     * entry (will not overwrite remote hostname).  --marekm
     */
    checkutmp(!amroot);
    STRFCPY(tty, utent.ut_line);

    if (hflg) {
#ifdef UT_ADDR
	struct hostent *he;

        /*
         * Fill in the ut_addr field (remote login IP address).
         * XXX - login from util-linux does it, but this is not
         * the right place to do it.  The program that starts
         * login (telnetd, rlogind) knows the IP address, so it
         * should create the utmp entry and fill in ut_addr.
         * gethostbyname() is not 100% reliable (the remote host
         * may be unknown, etc.).  --marekm
         */

        if ((he = gethostbyname(hostname))) {
	    utent.ut_addr = *((int32_t *)(he->h_addr_list[0]));
#endif
#ifdef UT_HOST
	    strncpy(utent.ut_host, hostname, sizeof(utent.ut_host));
#endif
#if HAVE_UTMPX_H
	    strncpy(utxent.ut_host, hostname, sizeof(utxent.ut_host));
#endif
	    /*
	     * Add remote hostname to the environment.  I think
	     * (not sure) I saw it once on Irix.  --marekm
	     */
	    addenv("REMOTEHOST", hostname);
	}

#ifdef __linux__ 
	/* workaround for init/getty leaving junk in ut_host at least in some
	   version of RedHat.  --marekm */
	else if (amroot)
	    memzero(utent.ut_host, sizeof utent.ut_host);
#endif

	openlog("mblogin", LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
	setup_tty();
	umask(getdef_num("UMASK", 007));

	if (pflg)
	    while (*envp)           /* add inherited environment, */
		addenv(*envp++, NULL); /* some variables change later */
	
	/* preserve TERM from getty */
	if (!pflg && (tmp = getenv("TERM")))
	    addenv("TERM", tmp);

	/* preserver CONNECT messages from mgetty */
	if ((tmp = getenv("CONNECT")))
	    addenv("CONNECT", tmp);
	if ((tmp = getenv("CALLER_ID")))
	    addenv("CALLER_ID", tmp);

	/* get the mbse environment */
	pw = getpwnam("mbse");
	addenv("MBSE_ROOT", pw->pw_dir);
	snprintf(userfile, PATH_MAX, "%s/etc/users.data",  pw->pw_dir);

	check_nologin();

	init_env();

        if (optind < argc) {            /* get the user name */

#ifdef SVR4
	    /*
	     * The "-h" option can't be used with a command-line username,
	     * because telnetd invokes us as: login -h host TERM=...
	     */

	    if (! hflg)
#endif
		{
		    STRFCPY(username, argv[optind]);
		    strzero(argv[optind]);
		    ++optind;
		}
	}
	
#ifdef SVR4
	/*
	 * check whether ttymon has done the prompt for us already
	 */

	{
	    char *ttymon_prompt;

	    if ((ttymon_prompt = getenv("TTYPROMPT")) != NULL && (*ttymon_prompt != 0)) {
		/* read name, without prompt */
		login_prompt((char *)0, username, sizeof username);
	    }
	}
#endif /* SVR4 */
	if (optind < argc)		/* now set command line variables */
	    set_env(argc - optind, &argv[optind]);

	if (hflg)
	    cp = hostname;
	else
#ifdef	UT_HOST
	if (utent.ut_host[0])
	    cp = utent.ut_host;
	else
#endif
#if HAVE_UTMPX_H
	if (utxent.ut_host[0])
	    cp = utxent.ut_host;
	else
#endif
	    cp = "";

	if (*cp)
	    snprintf(fromhost, sizeof fromhost, _(" on `%s' from `%s'"), tty, cp);
	else
	    snprintf(fromhost, sizeof fromhost, _(" on `%s'"), tty);

top:
	/* only allow ALARM sec. for login */
	signal(SIGALRM, alarm_handler);
	timeout = getdef_num("LOGIN_TIMEOUT", ALARM);
	if (timeout > 0)
	    alarm(timeout);

	environ = newenvp;		/* make new environment active */
	delay   = getdef_num("FAIL_DELAY", 1);
	retries = getdef_num("LOGIN_RETRIES", RETRIES);

	while (1) {			/* repeatedly get login/password pairs */
	    failed = 0;			/* haven't failed authentication yet */
	    if (! username[0]) {	/* need to get a login id */
		if (subroot) {
		    closelog ();
		    exit (1);
		}
		preauth_flag = 0;
		login_prompt(_("login: "), username, sizeof username);
		continue;
	    }

	    /*
	     * Here we try usernames on unix names and Fidonet style
	     * names that are stored in the bbs userdatabase.
	     * The name "bbs" is for new users, don't check the bbs userfile.
	     * If allowed from login.defs accept the name "mbse".
	     */
	    FoundName = 0;
	    if (strcmp(username, getdef_str("NEWUSER_ACCOUNT")) == 0) {
		FoundName = 1;
	    } 
	    if ((getdef_bool("ALLOW_MBSE") != 0) && (strcmp(username, "mbse") == 0)) {
	        FoundName = 1;
	    }
	    if (! FoundName) {
	        if ((ufp = fopen(userfile, "r"))) {
		    fread(&usrconfighdr, sizeof(usrconfighdr), 1, ufp);
		    while (fread(&usrconfig, usrconfighdr.recsize, 1, ufp) == 1) {
		        if ((strcasecmp(usrconfig.sUserName, username) == 0) ||
			    (strcasecmp(usrconfig.sHandle, username) == 0) ||
			    (strcmp(usrconfig.Name, username) == 0)) {
			    FoundName = 1;
			    STRFCPY(username, usrconfig.Name);
			    break;
			}
		    }
		    fclose(ufp);
		}
	    }

	    if (!FoundName) {
		if  (getdef_bool("ASK_NEWUSER") != 0) {
		    /*
		     * User entered none excisting name, offer him/her the choice
		     * to register as a new user.
		     */
		    login_prompt(_("Do you want to register as new user? [y/N]: "), username, sizeof username);
		    if ((username[0] && (toupper(username[0]) == 'Y'))) {
			FoundName = 1;
			preauth_flag = 0;
			STRFCPY(username, getdef_str("NEWUSER_ACCOUNT"));
			syslog(LOG_WARNING, "unknown user wants to register");
		    }
		}
	    }

	    if ((! (pwd = getpwnam(username))) || (FoundName == 0)) {
		pwent.pw_name = username;
		strcpy(temp_pw, "!");
		pwent.pw_passwd = temp_pw;
		pwent.pw_shell = temp_shell;

		preauth_flag = 0;
		failed = 1;
	    } else {
		pwent = *pwd;
	    }
#ifdef	SHADOW_PASSWORD
	    spwd = NULL;
	    if (pwd && strcmp(pwd->pw_passwd, SHADOW_PASSWD_STRING) == 0) {
		spwd = getspnam(username);
		if (spwd)
		    pwent.pw_passwd = spwd->sp_pwdp;
		else
		    syslog(LOG_WARNING, NO_SHADOW, username, fromhost);
	    }
#endif	/* SHADOWPWD */

	    /*
	     * If the encrypted password begins with a "!", the account
	     * is locked and the user cannot login, even if they have
	     * been "pre-authenticated."
	     */
	    if (pwent.pw_passwd[0] == '!' || pwent.pw_passwd[0] == '*')
		failed = 1;

	    /*
	     * The -r and -f flags provide a name which has already
	     * been authenticated by some server.
	     */
	    if (preauth_flag)
		goto auth_ok;

	    if (pw_auth(pwent.pw_passwd, username, reason, (char *) 0) == 0)
		goto auth_ok;

	    /*
	     * Don't log unknown usernames - I mistyped the password for
	     * username at least once...  Should probably use LOG_AUTHPRIV
	     * for those who really want to log them.  --marekm
	     */
	    syslog(LOG_WARNING, BAD_PASSWD, (pwd || getdef_bool("LOG_UNKFAIL_ENAB")) ? username : "UNKNOWN", fromhost);
	    failed = 1;

auth_ok:
	    /*
	     * This is the point where all authenticated users
	     * wind up.  If you reach this far, your password has
	     * been authenticated and so on.
	     */

#if defined(RADIUS) && !(defined(DES_RPC) || defined(KERBEROS))
	    if (clear_pass) {
		strzero(clear_pass);
		clear_pass = NULL;
	    }
#endif

	    if (! failed && pwent.pw_name && pwent.pw_uid == 0 && ! is_console) {
		syslog(LOG_CRIT, BAD_ROOT_LOGIN, fromhost);
		failed = 1;
	    }
#ifdef LOGIN_ACCESS
	    if (!failed && !login_access(username, *hostname ? hostname : tty)) {
		syslog(LOG_WARNING, LOGIN_REFUSED, username, fromhost);
		failed = 1;
	    }
#endif
	    if (! failed)
		break;

	    memzero(username, sizeof username);

	    if (--retries <= 0)
		syslog(LOG_CRIT, MANY_FAILS, fromhost);
#if 1
	    /*
	     * If this was a passwordless account and we get here,
	     * login was denied (securetty, faillog, etc.).  There
	     * was no password prompt, so do it now (will always
	     * fail - the bad guys won't see that the passwordless
	     * account exists at all).  --marekm
	     */

	    if (pwent.pw_passwd[0] == '\0')
		pw_auth("!", username, reason, (char *) 0);
#endif
	    /*
	     * Wait a while (a la SVR4 /usr/bin/login) before attempting
	     * to login the user again.  If the earlier alarm occurs
	     * before the sleep() below completes, login will exit.
	     */

	    if (delay > 0)
		sleep(delay);

	    puts(_("Login incorrect"));

	}  /* while (1) */
	(void) alarm (0);		/* turn off alarm clock */

	if (getenv("IFS"))		/* don't export user IFS ... */
	    addenv("IFS= \t\n", NULL);  /* ... instead, set a safe IFS */

	setutmp(username, tty, hostname); /* make entry in utmp & wtmp files */

	if (pwent.pw_shell[0] == '*') {	/* subsystem root */
	    subsystem (&pwent);		/* figure out what to execute */
	    subroot++;			/* say i was here again */
	    endpwent ();		/* close all of the file which were */
	    endgrent ();		/* open in the original rooted file */
#ifdef	SHADOW_PASSWORD
	    endspent ();		/* system.  they will be re-opened */
#endif
#ifdef	SHADOWGRP
	    endsgent ();		/* in the new rooted file system */
#endif
	    goto top;			/* go do all this all over again */
	}

	if (getdef_bool("LASTLOG_ENAB")) /* give last login and log this one */
	    dolastlog(&lastlog, &pwent, utent.ut_line, hostname);

#ifdef SVR4_SI86_EUA
	sysi86(SI86LIMUSER, EUA_ADD_USER);	/* how do we test for fail? */
#endif

#ifdef	AGING
	/*
	 * Have to do this while we still have root privileges, otherwise
	 * we don't have access to /etc/shadow.  expire() closes password
	 * files, and changes to the user in the child before executing
	 * the passwd program.  --marekm
	 */
#ifdef	SHADOW_PASSWORD
	if (spwd) {			/* check for age of password */
	    if (expire (&pwent, spwd)) {
		pwd = getpwnam(username);
		spwd = getspnam(username);
		if (pwd)
		    pwent = *pwd;
	    }
	}
#else
#ifdef	ATT_AGE
	if (pwent.pw_age && pwent.pw_age[0]) {
	    if (expire (&pwent)) {
		pwd = getpwnam(username);
		if (pwd)
		    pwent = *pwd;
	    }
	}
#endif	/* ATT_AGE */
#endif /* SHADOWPWD */
#endif	/* AGING */

	setup_limits(&pwent);  /* nice, ulimit etc. */
	chown_tty(tty, &pwent);

	if (setup_uid_gid(&pwent, is_console))
	    exit(1);

#ifdef KERBEROS
	if (clear_pass)
	    login_kerberos(username, clear_pass);
#endif
#ifdef DES_RPC
	if (clear_pass)
	    login_desrpc(clear_pass);
#endif
#if defined(DES_RPC) || defined(KERBEROS)
	if (clear_pass)
	    strzero(clear_pass);
#endif

	setup_env(&pwent);  /* set env vars, cd to the home dir */

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	addenv("HUSHLOGIN=TRUE", NULL);

	signal(SIGQUIT, SIG_DFL);	/* default quit signal */
	signal(SIGTERM, SIG_DFL);	/* default terminate signal */
	signal(SIGALRM, SIG_DFL);	/* default alarm signal */
	signal(SIGHUP, SIG_DFL);	/* added this.  --marekm */
	signal(SIGINT, SIG_DFL);	/* default interrupt signal */

	endpwent();			/* stop access to password file */
	endgrent();			/* stop access to group file */
#ifdef	SHADOW_PASSWORD
	endspent();			/* stop access to shadow passwd file */
#endif
#ifdef	SHADOWGRP
	endsgent();			/* stop access to shadow group file */
#endif
	if (pwent.pw_uid == 0)
	    syslog(LOG_NOTICE, ROOT_LOGIN, fromhost);
	else
	    syslog(LOG_INFO, REG_LOGIN, username, fromhost);
	closelog();

	shell (pwent.pw_shell, (char *) 0); /* exec the shell finally. */
	/*NOTREACHED*/
	return 0;
}


