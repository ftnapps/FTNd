/*****************************************************************************
 *
 * $Id: pw_util.c,v 1.11 2004/12/28 15:30:53 mbse Exp $
 * Purpose ...............: FreeBSD/NetBSD password utilities.
 * Remark ................: Taken from FreeBSD and modified for MBSE BBS.
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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

/* 
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)

#include "../config.h"
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <syslog.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pw_util.h"

char		*tempname;
static int	lockfd;



void pw_init()
{
	struct rlimit rlim;

	/* Unlimited resource limits. */
	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	(void)setrlimit(RLIMIT_CPU, &rlim);
	(void)setrlimit(RLIMIT_FSIZE, &rlim);
	(void)setrlimit(RLIMIT_STACK, &rlim);
	(void)setrlimit(RLIMIT_DATA, &rlim);
	(void)setrlimit(RLIMIT_RSS, &rlim);

	/* Don't drop core (not really necessary, but GP's). */
	rlim.rlim_cur = rlim.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rlim);

	/* Turn off signals. */
	(void)signal(SIGALRM, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGPIPE, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGTERM, SIG_IGN);

	/* Create with exact permissions. */
	(void)umask(0);
}


int pw_lock()
{
	/*
	 * If the master password file doesn't exist, the system is hosed.
	 * Might as well try to build one.  Set the close-on-exec bit so
	 * that users can't get at the encrypted passwords while editing.
	 * Open should allow flock'ing the file; see 4.4BSD.	XXX
	 */
	for (;;) {
		struct stat st;

		lockfd = open(_PATH_MASTERPASSWD, O_RDONLY, 0);
		if (lockfd < 0 || fcntl(lockfd, F_SETFD, 1) == -1) {
			syslog(LOG_ERR, "%s", _PATH_MASTERPASSWD);
			fprintf(stderr, "%s: %s\n", strerror(errno), _PATH_MASTERPASSWD);
			exit(1);
		}
		if (flock(lockfd, LOCK_EX|LOCK_NB)) {
			syslog(LOG_ERR, "the password db file is busy");
			fprintf(stderr, "the password db file is busy\n");
			exit(1);
		}

		/*
		 * If the password file was replaced while we were trying to
		 * get the lock, our hardlink count will be 0 and we have to
		 * close and retry.
		 */
		if (fstat(lockfd, &st) < 0) {
			syslog(LOG_ERR, "fstat() failed");
			fprintf(stderr, "%s: %s\n", strerror(errno), "fstat() failed");
			exit(1);
		}
		if (st.st_nlink != 0)
			break;
		close(lockfd);
		lockfd = -1;
	}
	return (lockfd);
}


int pw_tmp()
{
	static char	path[MAXPATHLEN] = _PATH_MASTERPASSWD;
	int		fd;
	char		*p;

	if ((p = strrchr(path, '/')))
		++p;
	else
		p = path;
	strcpy(p, "pw.XXXXXX");
	if ((fd = mkstemp(path)) == -1) {
		syslog(LOG_ERR, "%s", path);
		fprintf(stderr, "%s: %s\n", strerror(errno), path);
		exit(1);
	}
	tempname = path;
	return (fd);
}



int pw_mkdb(char *username)
{
	int	pstat;
	pid_t	pid;

	(void)fflush(stderr);
	if (!(pid = fork())) {
		if(!username) {
			syslog(LOG_WARNING, "rebuilding the database...");
			execl(_PATH_PWD_MKDB, "pwd_mkdb", "-p", tempname, NULL);
		} else {
			syslog(LOG_WARNING, "updating the database for %s...", username);
#if defined(__FreeBSD__) || defined(__OpenBSD__)
			execl(_PATH_PWD_MKDB, "pwd_mkdb", "-p", "-u", username, tempname, NULL);
#elif defined(__NetBSD__)
			execl(_PATH_PWD_MKDB, "pwd_mkdb", "-p", tempname, NULL);
#else                   
#error "Not FreeBSD, OpenBSD or NetBSD - don't know what to do"
#endif
		}
		pw_error((char *)_PATH_PWD_MKDB, 1, 1);
	}
	pid = waitpid(pid, &pstat, 0);
	if (pid == -1 || !WIFEXITED(pstat) || WEXITSTATUS(pstat) != 0)
		return (0);
	syslog(LOG_WARNING, "done");
	return (1);
}



void pw_error(char *name, int errn, int eval)
{
#ifdef YP
	extern int _use_yp;
#endif /* YP */
	if (errn)
		syslog(LOG_WARNING, name);
#ifdef YP
	if (_use_yp)
		syslog(LOG_WARNING, "NIS information unchanged");
	else
#endif /* YP */
	syslog(LOG_WARNING, "%s: unchanged", _PATH_MASTERPASSWD);
	(void)unlink(tempname);
	exit(eval);
}


void pw_copy(int ffd, int tfd, struct passwd *pw)
{
        FILE	*from, *to;
        int	done;
        char	*p, buf[8192];
        char	uidstr[20];
        char	gidstr[20];
        char	chgstr[20];
        char	expstr[20];

        snprintf(uidstr, sizeof(uidstr), "%d", pw->pw_uid);
        snprintf(gidstr, sizeof(gidstr), "%d", pw->pw_gid);
        snprintf(chgstr, sizeof(chgstr), "%ld", (long)pw->pw_change);
        snprintf(expstr, sizeof(expstr), "%ld", (long)pw->pw_expire);

        if (!(from = fdopen(ffd, "r")))
                pw_error((char *)_PATH_MASTERPASSWD, 1, 1);
        if (!(to = fdopen(tfd, "w")))
                pw_error(tempname, 1, 1);

        for (done = 0; fgets(buf, sizeof(buf), from);) {
                if (!strchr(buf, '\n')) {
                        syslog(LOG_WARNING, "%s: line too long", _PATH_MASTERPASSWD);
                        pw_error(NULL, 0, 1);
                }
                if (done) {
                        (void)fprintf(to, "%s", buf);
                        if (ferror(to))
                                goto err;
                        continue;
                }
		for (p = buf; *p != '\n'; p++)
			if (*p != ' ' && *p != '\t')
				break;
		if (*p == '#' || *p == '\n') {
			(void)fprintf(to, "%s", buf);
			if (ferror(to))
				goto err;
			continue;
		}
                if (!(p = strchr(buf, ':'))) {
                        syslog(LOG_WARNING, "%s: corrupted entry", _PATH_MASTERPASSWD);
                        pw_error(NULL, 0, 1);
                }
                *p = '\0';
                if (strcmp(buf, pw->pw_name)) {
                        *p = ':';
                        (void)fprintf(to, "%s", buf);
                        if (ferror(to))
                                goto err;
                        continue;
                }
#if defined(__FreeBSD__)
                (void)fprintf(to, "%s:%s:%s:%s:%s:%s:%s:%s:%s:%s\n",
                    pw->pw_name, pw->pw_passwd,
                    pw->pw_fields & _PWF_UID ? uidstr : "",
                    pw->pw_fields & _PWF_GID ? gidstr : "",
                    pw->pw_class,
                    pw->pw_fields & _PWF_CHANGE ? chgstr : "",
                    pw->pw_fields & _PWF_EXPIRE ? expstr : "",
                    pw->pw_gecos, pw->pw_dir, pw->pw_shell);
#elif defined(__NetBSD__) || defined(__OpenBSD__)
		(void)fprintf(to, "%s:%s:%s:%s:%s:%s:%s:%s:%s:%s\n",
		    pw->pw_name, pw->pw_passwd,
		    uidstr, gidstr,
		    pw->pw_class,
		    chgstr, expstr,
		    pw->pw_gecos, pw->pw_dir, pw->pw_shell);
#else
#error "Not FreeBSD, OpenBSD or NetBSD - don't know what to do"
#endif
                done = 1;
                if (ferror(to))
                        goto err;
        }
        if (!done) {
#ifdef YP
        /* Ultra paranoid: shouldn't happen. */
                if (getuid())  {
                        syslog(LOG_WARNING, "%s: not found in %s -- permission denied",
                                        pw->pw_name, _PATH_MASTERPASSWD);
                        pw_error(NULL, 0, 1);
                } else
#endif /* YP */
#if defined(__FreeBSD__)
                (void)fprintf(to, "%s:%s:%s:%s:%s:%s:%s:%s:%s:%s\n",
                    pw->pw_name, pw->pw_passwd,
                    pw->pw_fields & _PWF_UID ? uidstr : "",
                    pw->pw_fields & _PWF_GID ? gidstr : "",
                    pw->pw_class,
                    pw->pw_fields & _PWF_CHANGE ? chgstr : "",
                    pw->pw_fields & _PWF_EXPIRE ? expstr : "",
                    pw->pw_gecos, pw->pw_dir, pw->pw_shell);
#elif defined(__NetBSD__) || defined(__OpenBSD__)
		(void)fprintf(to, "%s:%s:%s:%s:%s:%s:%s:%s:%s:%s\n",
		    pw->pw_name, pw->pw_passwd,
		    uidstr, gidstr,
		    pw->pw_class,
		    chgstr, expstr,
		    pw->pw_gecos, pw->pw_dir, pw->pw_shell);
#else
#error "Not FreeBSD, OpenBSD or NetBSD - don't know what to do"
#endif
        }

        if (ferror(to))
err:            pw_error(NULL, 1, 1);
        (void)fclose(to);
}


#endif

