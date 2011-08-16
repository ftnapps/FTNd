/*****************************************************************************
 *
 * $Id: mbuseradd.c,v 1.24 2007/05/28 10:40:24 mbse Exp $
 * Purpose ...............: setuid root version of useradd
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <grp.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <syslog.h>
#include <time.h>

#if (defined(__FreeBSD__) && __FreeBSD_version >= 600000)
#define FreeBSD_sysctl 1
#endif

#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(FreeBSD_sysctl)
#include <sys/sysctl.h>
#endif

#include "xmalloc.h"
#include "mbuseradd.h"


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
	if ((errno == EINVAL) || (errno == EFAULT))
	    break;

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
    int	    i, pid, status = 0, rc = 0;

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



void makedir(char *path, mode_t mode, uid_t owner, gid_t group)
{
    if (mkdir(path, mode) != 0) {
	syslog(LOG_WARNING, "Can't create directory %s:%s", path, strerror(errno));
	exit(2);
    }
    if ((chown(path, owner, group)) == -1) {
	syslog(LOG_WARNING, "Unable to change ownership of %s", path);
	exit(2);
    }
}



/*
 *  Internal version of basename to make this better portable.
 */
char *Basename(char *str)
{
    char *cp = strrchr(str, '/');

    return cp ? cp+1 : str;
}



/*
 * Function will create the users name in the passwd file
 * Note that this function must run setuid root!
 */
int main(int argc, char *argv[])
{
    char	    *temp, *shell, *homedir, *args[16], *parent;
    int		    i;
    struct passwd   *pwent, *pwuser;
    struct group    *gr;
    pid_t	    ppid;
#if defined(__OpenBSD__)
#define ARG_SIZE 60
    static char	    **s, buf[ARG_SIZE];
    size_t	    siz = 100;
    char	    **p;
    int		    mib[4];
#elif defined(__NetBSD__) || defined(FreeBSD_sysctl)
    static char     **s;
    size_t          siz = 100;
    int             mib[4];
#else
    FILE	    *fp;
#endif

    if (argc != 5)
	Help();

    /*
     *  First simple check for argument overflow
     */
    for (i = 1; i < 5; i++) {
	if (strlen(argv[i]) > 80) {
	    fprintf(stderr, "mbuseradd: Argument %d is too long\n", i);
	    exit(1);
	}
    }

    /*
     * Check calling username
     */
    ppid = getuid();
    pwent = getpwuid(ppid);
    if (!pwent) {
	fprintf(stderr, "mbuseradd: Cannot determine your user name.\n");
	exit(1);
    }
    if (strcmp(pwent->pw_name, (char *)"mbse") && strcmp(pwent->pw_name, (char *)"bbs")) {
	fprintf(stderr, "mbuseradd: only users `mbse' and `bbs' may do this.\n");
	exit(1);
    }

    /*
     * Get my groupname, this must be "bbs", other users may not
     * use this program, not even root.
     */
    gr = getgrgid(pwent->pw_gid);
    if (!gr) {
	fprintf(stderr, "mbuseradd: Cannot determine group name.\n");
	exit(1);
    }
    if (strcmp(gr->gr_name, (char *)"bbs")) {
	fprintf(stderr, "mbuseradd: You are not a member of group `bbs'.\n");
	exit(1);
    }

    /*
     * Find out the name of our parent.
     */
    temp = calloc(PATH_MAX, sizeof(char));
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
	fprintf(stderr, "mbuseradd: no memory\n");
	exit(1);
    }
    if (sysctl(mib, 4, s, &siz, NULL, 0) == -1) {
	perror("");
	fprintf(stderr, "mbuseradd: sysctl call failed\n");
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
        fprintf(stderr, "mbuseradd: no memory\n");
        exit(1);
    }
    if (sysctl(mib, 4, s, &siz, NULL, 0) == -1) {
        perror("");
        fprintf(stderr, "mbuseradd: sysctl call failed\n");
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
	fprintf(stderr, "mbuseradd: no memory\n");
	exit(1);
    }
    if (sysctl(mib, 4, s, &siz, NULL, 0) == -1) {
	perror("");
	fprintf(stderr, "mbuseradd: sysctl call failed\n");
   	exit(1);
    }
    parent = xstrcpy((char *)s);
#else
    /*
     *  Systems with /proc filesystem like Linux, old FreeBSD
     */
    snprintf(temp, PATH_MAX, "/proc/%d/cmdline", ppid);
    if ((fp = fopen(temp, "r")) == NULL) {
	fprintf(stderr, "mbuseradd: can't read %s\n", temp);
	exit(1);
    }
    fgets(temp, PATH_MAX-1, fp);
    fclose(fp);
    parent = xstrcpy(Basename(temp));
#endif

    if (strcmp((char *)"-mbnewusr", parent)) {
	fprintf(stderr, "mbuseradd: illegal parent \"%s\"\n", parent);
	free(temp);
	free(parent);
	exit(1);
    }

    memset(args, 0, sizeof(args));

    shell   = calloc(PATH_MAX, sizeof(char));
    homedir = calloc(PATH_MAX, sizeof(char));

    if (setuid(0) == -1 || setgid(1) == -1) {
        perror("");
        fprintf(stderr, "mbuseradd: Unable to setuid(root) or setgid(root)\n");
        fprintf(stderr, "Make sure that this program is set to -rwsr-sr-x\n");
        fprintf(stderr, "Owner must be root and group root\n");
        exit(1);
    }
    umask(0000);

    /*
     * We don't log into MBSE BBS logfiles but to the system logfiles,
     * because we are modifying system files not belonging to MBSE BBS.
     */
    openlog("mbuseradd", LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_AUTH);
    syslog(LOG_WARNING, "mbuseradd %s %s %s %s", argv[1], argv[2], argv[3], argv[4]);

    /*
     * Build command to add user entry to the /etc/passwd and /etc/shadow
     * files. We use the systems own useradd program.
     */
#if defined(__linux__) || defined(__NetBSD__) || defined(__OpenBSD__)
    if ((access("/usr/bin/useradd", R_OK)) == 0)
	args[0] = (char *)"/usr/bin/useradd";
    else if ((access("/bin/useradd", R_OK)) == 0)
	args[0] = (char *)"/bin/useradd";
    else if ((access("/usr/sbin/useradd", R_OK)) == 0)
	args[0] = (char *)"/usr/sbin/useradd";
    else if ((access("/sbin/useradd", R_OK)) == 0)
	args[0] = (char *)"/sbin/useradd";
    else {
	syslog(LOG_WARNING, "Can't find useradd");
	exit(1);
    }
#elif __FreeBSD__
    if ((access("/usr/sbin/pw", X_OK)) == 0)
	args[0] = (char *)"/usr/sbin/pw";
    else if ((access("/sbin/pw", X_OK)) == 0)
	args[0] = (char *)"/sbin/pw";
    else {
	syslog(LOG_WARNING, "Can't find pw");
	exit(1);
    }
#else
#error "Don't know how to add a user on this OS"
#endif

    snprintf(shell, PATH_MAX, "%s/bin/mbsebbs", getenv("MBSE_ROOT"));
    snprintf(homedir, PATH_MAX, "%s/%s", argv[4], argv[2]);

#if defined(__linux__)
    args[1] = (char *)"-c";
    args[2] = argv[3];
    args[3] = (char *)"-d";
    args[4] = homedir;
    args[5] = (char *)"-g";
    args[6] = argv[1];
    args[7] = (char *)"-s";
    args[8] = shell;
    args[9] = argv[2];
    args[10] = NULL;
#elif defined(__NetBSD__) || defined(__OpenBSD__)
    args[1] = (char *)"-c";
    args[2] = argv[3];
    args[3] = (char *)"-d";
    args[4] = homedir;
    args[5] = (char *)"-g";
    args[6] = argv[1];
    args[7] = (char *)"-s";
    args[8] = shell;
    args[9] = (char *)"-m";
    args[10] = argv[2];
    args[11] = NULL;
#elif defined(__FreeBSD__)
    args[1] = (char *)"useradd";
    args[2] = argv[2];
    args[3] = (char *)"-c";
    args[4] = argv[3];
    args[5] = (char *)"-d";
    args[6] = homedir;
    args[7] = (char *)"-g";
    args[8] = argv[1];
    args[9] = (char *)"-s";
    args[10] = shell;
    args[11] = NULL;
#else
#error "Don't know how to add a user on this OS"
#endif

    if (execute(args, (char *)"/dev/tty", (char *)"/dev/tty", (char *)"/dev/tty") != 0) {
	syslog(LOG_WARNING, "Failed to create unix account");
	exit(1);
    } 
    syslog(LOG_WARNING, "Created Unix account");

    /*
     * Now create directories and files for this user.
     */
    if ((pwent = getpwnam((char *)"mbse")) == NULL) {
	syslog(LOG_WARNING, "Can't get password entry for \"mbse\"");
	exit(2);
    }

    /*
     *
     *  Check bbs users base home directory
     */
    if ((access(argv[4], R_OK)) != 0) {
	syslog(LOG_WARNING, "No bbs base homedirectory, creating..");
	makedir(argv[4], 0770, pwent->pw_uid, pwent->pw_gid);
    }

    /*
     * Now create users home directory. Check for an existing directory,
     * some systems have already created a home directory. If one is found
     * it is removed to create a fresh one.
     */
    if ((access(homedir, R_OK)) == 0) {
	if ((access("/bin/rm", X_OK)) == 0)
	    args[0] = (char *)"/bin/rm";
	else if ((access("/usr/bin/rm", X_OK)) == 0)
	    args[0] = (char *)"/usr/bin/rm";
	else {
	    syslog(LOG_WARNING, "Can't find rm");
	    exit(2);
	}
	args[1] = (char *)"-Rf";
	args[2] = homedir;
	args[3] = NULL;
	i = execute(args, (char *)"/dev/tty", (char *)"/dev/tty", (char *)"/dev/tty");
	if (i != 0) {
	    syslog(LOG_WARNING, "Unable remove old home directory");
	    exit(2);
	}
    }

    /*
     *  Create users home directory.
     */
    if ((pwuser = getpwnam(argv[2])) == NULL) {
	syslog(LOG_WARNING, "Can't get passwd entry for %s", argv[2]);
	exit(2);
    }
    makedir(homedir, 0770, pwuser->pw_uid, pwent->pw_gid);

    /*
     *  Create Maildir and subdirs for Qmail.
     */
    snprintf(temp, PATH_MAX, "%s/%s/Maildir", argv[4], argv[2]);
    makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);
    snprintf(temp, PATH_MAX, "%s/%s/Maildir/cur", argv[4], argv[2]);
    makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);
    snprintf(temp, PATH_MAX, "%s/%s/Maildir/new", argv[4], argv[2]);
    makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);
    snprintf(temp, PATH_MAX, "%s/%s/Maildir/tmp", argv[4], argv[2]);
    makedir(temp, 0700, pwuser->pw_uid, pwent->pw_gid);

#ifdef _VPOPMAIL_PATH
    snprintf(temp, PATH_MAX, "%s/vadduser", _VPOPMAIL_PATH);
    args[0] = temp;
    args[1] = argv[2];
    args[2] = argv[2];
    args[3] = NULL;
    
    if (execute(args, (char *)"/dev/tty", (char *)"/dev/tty", (char *)"/dev/tty") != 0) {
	syslog(LOG_WARNING, "Failed to create vpopmail account");
    } else {
	syslog(LOG_WARNING, "Created vpopmail account");
    }
#endif

    free(shell);
    free(temp);
    free(homedir);
    syslog(LOG_WARNING, "Added system account for user\"%s\"", argv[2]);
    exit(0);
}



void Help()
{
    fprintf(stderr, "\nmbuseradd commandline:\n\n");
    fprintf(stderr, "mbuseradd [gid] [name] [comment] [usersdir]\n");
    exit(1);
}


