/*****************************************************************************
 *
 * utmp.c
 * Purpose ...............: FTNd Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyright ....: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2004 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "ftnlogin.h"
#include <utmp.h>

#if HAVE_UTMPX_H
#include <utmpx.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_LIBUTIL_H
#include <libutil.h>
#endif
#if HAVE_UTIL_H
#include <util.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include "utmp.h"

#if defined(__NetBSD__)
#undef LOGIN_PROCESS
#undef HAVE_UTMPX_H
#endif

#if HAVE_UTMPX_H
extern	struct	utmpx	utxent;
#endif
extern	struct	utmp	utent;


#define	NO_UTENT \
	"No utmp entry.  You must exec \"login\" from the lowest level \"sh\""
#define	NO_TTY \
	"Unable to determine your tty name."

/*
 * checkutmp - see if utmp file is correct for this process
 *
 *	System V is very picky about the contents of the utmp file
 *	and requires that a slot for the current process exist.
 *	The utmp file is scanned for an entry with the same process
 *	ID.  If no entry exists the process exits with a message.
 *
 *	The "picky" flag is for network and other logins that may
 *	use special flags.  It allows the pid checks to be overridden.
 *	This means that getty should never invoke login with any
 *	command line flags.
 */

#if defined(__linux__)  /* XXX */

void checkutmp(int picky)
{
	char *line;
	struct utmp *ut;
	pid_t pid = getpid();

	setutent();

	/* First, try to find a valid utmp entry for this process.  */
	while ((ut = getutent()))
		if (ut->ut_pid == pid && ut->ut_line[0] && ut->ut_id[0] &&
		    (ut->ut_type==LOGIN_PROCESS || ut->ut_type==USER_PROCESS))
			break;

	/* If there is one, just use it, otherwise create a new one.  */
	if (ut) {
		utent = *ut;
	} else {
		if (picky) {
			puts(NO_UTENT);
			exit(1);
		}
		line = ttyname(0);
		if (!line) {
			puts(NO_TTY);
			exit(1);
		}
		if (strncmp(line, "/dev/", 5) == 0)
			line += 5;
		memset((void *) &utent, 0, sizeof utent);
		utent.ut_type = LOGIN_PROCESS;
		utent.ut_pid = pid;
		strncpy(utent.ut_line, line, sizeof utent.ut_line);
		/* XXX - assumes /dev/tty?? */
		strncpy(utent.ut_id, utent.ut_line + 3, sizeof utent.ut_id);
		strcpy(utent.ut_user, "LOGIN");
		time(&utent.ut_time);
	}
}

#elif defined(LOGIN_PROCESS)

void checkutmp(int picky)
{
	char *line;
	struct utmp *ut;
#if HAVE_UTMPX_H
	struct utmpx *utx;
#endif

	pid_t pid = getpid();

#if HAVE_UTMPX_H
	setutxent();
#endif
	setutent();

	if (picky) {
#if HAVE_UTMPX_H
		while ((utx = getutxent()))
			if (utx->ut_pid == pid)
				break;

		if (utx)
			utxent = *utx;
#endif
		while ((ut = getutent()))
			if (ut->ut_pid == pid)
				break;

		if (ut)
			utent = *ut;

#if HAVE_UTMPX_H
		endutxent();
#endif
		endutent();

		if (!ut) {
 			puts(NO_UTENT);
			exit(1);
		}
#ifndef	UNIXPC

		/*
		 * If there is no ut_line value in this record, fill
		 * it in by getting the TTY name and stuffing it in
		 * the structure.  The UNIX/PC is broken in this regard
		 * and needs help ...
		 */

		if (utent.ut_line[0] == '\0')
#endif	/* !UNIXPC */
		{
			if (!(line = ttyname(0))) {
				puts(NO_TTY);
				exit(1);
			}
			if (strncmp(line, "/dev/", 5) == 0)
				line += 5;
			strncpy(utent.ut_line, line, sizeof utent.ut_line);
#if HAVE_UTMPX_H
			strncpy(utxent.ut_line, line, sizeof utxent.ut_line);
#endif
		}
	} else {
		if (!(line = ttyname(0))) {
			puts(NO_TTY);
			exit(1);
		}
		if (strncmp(line, "/dev/", 5) == 0)
			line += 5;

 		strncpy (utent.ut_line, line, sizeof utent.ut_line);
		if ((ut = getutline(&utent)))
 			strncpy(utent.ut_id, ut->ut_id, sizeof ut->ut_id);

		strcpy(utent.ut_user, "LOGIN");
		utent.ut_pid = getpid();
		utent.ut_type = LOGIN_PROCESS;
		time(&utent.ut_time);
#if HAVE_UTMPX_H
		strncpy(utxent.ut_line, line, sizeof utxent.ut_line);
		if ((utx = getutxline(&utxent)))
			strncpy(utxent.ut_id, utx->ut_id, sizeof utxent.ut_id);

		strcpy(utxent.ut_user, "LOGIN");
		utxent.ut_pid = utent.ut_pid;
		utxent.ut_type = utent.ut_type;
		gettimeofday((struct timeval *) &utxent.ut_tv, NULL);
		utent.ut_time = utxent.ut_tv.tv_sec;
#endif
	}
}

#else	/* !USG */

/*
 * Also used for FreeBSD
 */
void checkutmp(int picky)
{
	char *line;

	/*
	 * Hand-craft a new utmp entry.
	 */

	memzero(&utent, sizeof utent);
	if (! (line = ttyname (0))) {
		puts (NO_TTY);
		exit (1);
	}
	if (strncmp (line, "/dev/", 5) == 0)
		line += 5;

	(void) strncpy (utent.ut_line, line, sizeof utent.ut_line);
	(void) time (&utent.ut_time);
}

#endif	/* !USG */


/*
 * Some systems already have updwtmp() and possibly updwtmpx().  Others
 * don't, so we re-implement these functions if necessary.  --marekm
 */

#ifndef HAVE_UPDWTMP
void updwtmp(const char *filename, const struct utmp *ut)
{
	int fd;

	fd = open(filename, O_APPEND | O_WRONLY, 0);
	if (fd >= 0) {
		write(fd, (const char *) ut, sizeof(*ut));
		close(fd);
	}
}
#endif  /* ! HAVE_UPDWTMP */

#ifdef HAVE_UTMPX_H
#ifndef HAVE_UPDWTMPX
static void updwtmpx(const char *filename, const struct utmpx *utx)
{
	int fd;

	fd = open(filename, O_APPEND | O_WRONLY, 0);
	if (fd >= 0) {
		write(fd, (const char *) utx, sizeof(*utx));
		close(fd);
	}
}
#endif  /* ! HAVE_UPDWTMPX */
#endif  /* ! HAVE_UTMPX_H */


/*
 * setutmp - put a USER_PROCESS entry in the utmp file
 *
 *	setutmp changes the type of the current utmp entry to
 *	USER_PROCESS.  the wtmp file will be updated as well.
 */

#if defined(__linux__) /* XXX */

void setutmp(const char *name, const char *line, const char *host)
{
	utent.ut_type = USER_PROCESS;
	strncpy(utent.ut_user, name, sizeof utent.ut_user);
	time(&utent.ut_time);
	/* other fields already filled in by checkutmp above */
	setutent();
	pututline(&utent);
	endutent();
	updwtmp(_WTMP_FILE, &utent);
}

#elif HAVE_UTMPX_H

void setutmp(const char *name, const char *line, const char *host)
{
	struct	utmp	*utmp, utline;
	struct	utmpx	*utmpx, utxline;
	pid_t	pid = getpid ();
	int	found_utmpx = 0, found_utmp = 0;

	printf("setutmp HAVE_UTMP_H\n");
	/*
	 * The canonical device name doesn't include "/dev/"; skip it
	 * if it is already there.
	 */

	if (strncmp (line, "/dev/", 5) == 0)
		line += 5;

	/*
	 * Update utmpx.  We create an empty entry in case there is
	 * no matching entry in the utmpx file.
	 */

	setutxent ();
	setutent ();

	while (utmpx = getutxent ()) {
		if (utmpx->ut_pid == pid) {
			found_utmpx = 1;
			break;
		}
	}
	while (utmp = getutent ()) {
		if (utmp->ut_pid == pid) {
			found_utmp = 1;
			break;
		}
	}

	/*
	 * If the entry matching `pid' cannot be found, create a new
	 * entry with the device name in it.
	 */

	if (! found_utmpx) {
		memset ((void *) &utxline, 0, sizeof utxline);
		strncpy (utxline.ut_line, line, sizeof utxline.ut_line);
		utxline.ut_pid = getpid ();
	} else {
		utxline = *utmpx;
		if (strncmp (utxline.ut_line, "/dev/", 5) == 0) {
			memmove (utxline.ut_line, utxline.ut_line + 5,
				sizeof utxline.ut_line - 5);
			utxline.ut_line[sizeof utxline.ut_line - 5] = '\0';
		}
	}
	if (! found_utmp) {
		memset ((void *) &utline, 0, sizeof utline);
		strncpy (utline.ut_line, utxline.ut_line,
			sizeof utline.ut_line);
		utline.ut_pid = utxline.ut_pid;
	} else {
		utline = *utmp;
		if (strncmp (utline.ut_line, "/dev/", 5) == 0) {
			memmove (utline.ut_line, utline.ut_line + 5,
				sizeof utline.ut_line - 5);
			utline.ut_line[sizeof utline.ut_line - 5] = '\0';
		}
	}

	/*
	 * Fill in the fields in the utmpx entry and write it out.  Do
	 * the utmp entry at the same time to make sure things don't
	 * get messed up.
	 */

	strncpy (utxline.ut_user, name, sizeof utxline.ut_user);
	strncpy (utline.ut_user, name, sizeof utline.ut_user);

	utline.ut_type = utxline.ut_type = USER_PROCESS;

	gettimeofday(&utxline.ut_tv, NULL);
	utline.ut_time = utxline.ut_tv.tv_sec;

	strncpy(utxline.ut_host, host ? host : "", sizeof utxline.ut_host);

	pututxline (&utxline);
	pututline (&utline);

	updwtmpx(_WTMP_FILE "x", &utxline);
	updwtmp(_WTMP_FILE, &utline);

	utxent = utxline;
	utent = utline;
}

#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__

/*
 * FreeBSD/NetBSD/OpenBSD version, simple and mean.
 */
void setutmp(const char *name, const char *line, const char *host)
{
	struct	utmp	utmp;

	memset(&utmp, 0, sizeof(utmp));

	strncpy(utmp.ut_line, line, (int) sizeof utmp.ut_line);
	strncpy(utmp.ut_name, name, sizeof utent.ut_name);
	strncpy(utmp.ut_host, host, sizeof utent.ut_host);
	(void) time (&utmp.ut_time);

	login(&utmp);
	utent = utmp;
}


#else /* !SVR4 */

void setutmp(const char *name, const char *line)
{
	struct	utmp	utmp;
	int	fd;
	int	found = 0;

	if ((fd = open(_UTMP_FILE, O_RDWR)) < 0)
		return;

#if !defined(SUN) && !defined(BSD) && !defined(SUN4)  /* XXX */
 	while (!found && read(fd, (char *)&utmp, sizeof utmp) == sizeof utmp) {
 		if (! strncmp (line, utmp.ut_line, (int) sizeof utmp.ut_line))
			found++;
	}
#endif

	if (! found) {

		/*
		 * This is a brand-new entry.  Clear it out and fill it in
		 * later.
		 */

  		memzero(&utmp, sizeof utmp);
 		strncpy(utmp.ut_line, line, (int) sizeof utmp.ut_line);
	}

	/*
	 * Fill in the parts of the UTMP entry.  BSD has just the name,
	 * while System V has the name, PID and a type.
	 */

	strncpy(utmp.ut_user, name, sizeof utent.ut_user);
#ifdef USER_PROCESS
	utmp.ut_type = USER_PROCESS;
	utmp.ut_pid = getpid ();
#endif

	/*
	 * Put in the current time (common to everyone)
	 */

	(void) time (&utmp.ut_time);

#ifdef UT_HOST
	/*
	 * Update the host name field for systems with networking support
	 */

	(void) strncpy (utmp.ut_host, utent.ut_host, (int) sizeof utmp.ut_host);
#endif

	/*
	 * Locate the correct position in the UTMP file for this
	 * entry.
	 */

#ifdef HAVE_TTYSLOT
	(void) lseek (fd, (off_t) (sizeof utmp) * ttyslot (), SEEK_SET);
#else
	if (found)	/* Back up a splot */
		lseek (fd, (off_t) - sizeof utmp, SEEK_CUR);
	else		/* Otherwise, go to the end of the file */
		lseek (fd, (off_t) 0, SEEK_END);
#endif

	/*
	 * Scribble out the new entry and close the file.  We're done
	 * with UTMP, next we do WTMP (which is real easy, put it on
	 * the end of the file.
	 */

	(void) write (fd, (char *) &utmp, sizeof utmp);
	(void) close (fd);

	updwtmp(_WTMP_FILE, &utmp);
 	utent = utmp;
}

#endif /* SVR4 */
