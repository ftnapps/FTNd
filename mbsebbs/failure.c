/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: MBSE BBS Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyrioght ...: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
 *   
 * Michiel Broek                FIDO:           2:280/2802
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
#include <fcntl.h>
#include <stdio.h>
#include "mblogin.h"
#include "getdef.h"
#include <utmp.h>
#include "failure.h"

#define	YEAR	(365L*DAY)

/*
 * failure - make failure entry
 *
 *	failure() creates a new (struct faillog) entry or updates an
 *	existing one with the current failed login information.
 */

void failure(uid_t uid, const char *tty, struct faillog *fl)
{
	int fd;

	/*
	 * Don't do anything if failure logging isn't set up.
	 */

	if ((fd = open(FAILLOG_FILE, O_RDWR)) < 0)
		return;

	/*
	 * The file is indexed by uid value meaning that shared UID's
	 * share failure log records.  That's OK since they really
	 * share just about everything else ...
	 */

	lseek(fd, (off_t) (sizeof *fl) * uid, SEEK_SET);
	if (read(fd, (char *) fl, sizeof *fl) != sizeof *fl)
		memzero(fl, sizeof *fl);

	/*
	 * Update the record.  We increment the failure count to log the
	 * latest failure.  The only concern here is overflow, and we'll
	 * check for that.  The line name and time of day are both
	 * updated as well.
	 */

	if (fl->fail_cnt + 1 > 0)
		fl->fail_cnt++;

	strncpy(fl->fail_line, tty, sizeof fl->fail_line);
	time(&fl->fail_time);

	/*
	 * Seek back to the correct position in the file and write the
	 * record out.  Ideally we should lock the file in case the same
	 * account is being logged simultaneously.  But the risk doesn't
	 * seem that great.
	 */

	lseek(fd, (off_t) (sizeof *fl) * uid, SEEK_SET);
	write(fd, (char *) fl, sizeof *fl);
	close(fd);
}

static int too_many_failures(const struct faillog *fl)
{
	time_t now;

	if (fl->fail_max == 0 || fl->fail_cnt < fl->fail_max)
		return 0;

	if (fl->fail_locktime == 0)
		return 1;  /* locked until reset manually */

	time(&now);
	if (fl->fail_time + fl->fail_locktime > now)
		return 0;  /* enough time since last failure */

	return 1;
}

/*
 * failcheck - check for failures > allowable
 *
 *	failcheck() is called AFTER the password has been validated.  If the
 *	account has been "attacked" with too many login failures, failcheck()
 *	returns FALSE to indicate that the login should be denied even though
 *	the password is valid.
 */

int failcheck(uid_t uid, struct faillog *fl, int failed)
{
	int	fd;
	struct	faillog	fail;

	/*
	 * Suppress the check if the log file isn't there.
	 */

	if ((fd = open(FAILLOG_FILE, O_RDWR)) < 0)
		return 1;

	/*
	 * Get the record from the file and determine if the user has
	 * exceeded the failure limit.  If "max" is zero, any number
	 * of failures are permitted.  Only when "max" is non-zero and
	 * "cnt" is greater than or equal to "max" is the account
	 * considered to be locked.
	 *
	 * If read fails, there is no record for this user yet (the
	 * file is initially zero length and extended by writes), so
	 * no need to reset the count.
	 */

	lseek (fd, (off_t) (sizeof *fl) * uid, SEEK_SET);
	if (read(fd, (char *) fl, sizeof *fl) != sizeof *fl) {
		close(fd);
		return 1;
	}

	if (too_many_failures(fl)) {
		close(fd);
		return 0;
	}

	/*
	 * The record is updated if this is not a failure.  The count will
	 * be reset to zero, but the rest of the information will be left
	 * in the record in case someone wants to see where the failed
	 * login originated.
	 */

	if (!failed) {
		fail = *fl;
		fail.fail_cnt = 0;

		lseek (fd, (off_t) sizeof fail * uid, SEEK_SET);
		write (fd, (char *) &fail, sizeof fail);
	}
	close (fd);
	return 1;
}

/*
 * failprint - print line of failure information
 *
 *	failprint takes a (struct faillog) entry and formats it into a
 *	message which is displayed at login time.
 */

void failprint(const struct faillog *fail)
{
	struct	tm	*tp;
#if HAVE_STRFTIME
	char	lasttimeb[256];
	char	*lasttime = lasttimeb;
	const char *fmt;
#else
	char	*lasttime;
#endif
	time_t NOW;

	if (fail->fail_cnt == 0)
		return;

	tp = localtime (&(fail->fail_time));
	time(&NOW);

#if HAVE_STRFTIME
	/*
	 * Only print as much date and time info as it needed to
	 * know when the failure was.
	 */

	if (NOW - fail->fail_time >= YEAR)
		fmt = "%Y";
	else if (NOW - fail->fail_time >= DAY)
		fmt = "%A %T";
	else
		fmt = "%T";
	strftime(lasttimeb, sizeof lasttimeb, fmt, tp);
#else

	/*
	 * Do the same thing, but don't use strftime since it
	 * probably doesn't exist on this system
	 */

	lasttime = asctime (tp);
	lasttime[24] = '\0';

	if (NOW - fail->fail_time < YEAR)
		lasttime[19] = '\0';
	if (NOW - fail->fail_time < DAY)
		lasttime = lasttime + 11;

	if (*lasttime == ' ')
		lasttime++;
#endif
	printf ("%d %s since last login.  Last was %s on %s.\n",
		fail->fail_cnt, fail->fail_cnt > 1 ? "failures":"failure",
		lasttime, fail->fail_line);
}

/*
 * failtmp - update the cummulative failure log
 *
 *	failtmp updates the (struct utmp) formatted failure log which
 *	maintains a record of all login failures.
 */

void failtmp(const struct utmp *failent)
{
	char *ftmp;
	int fd;

	/*
	 * Get the name of the failure file.  If no file has been defined
	 * in login.defs, don't do this.
	 */

	if (!(ftmp = getdef_str("FTMP_FILE")))
		return;

	/*
	 * Open the file for append.  It must already exist for this
	 * feature to be used.
	 */

	if ((fd = open(ftmp, O_WRONLY|O_APPEND)) == -1)
		return;

	/*
	 * Output the new failure record and close the log file.
	 */

	write(fd, (const char *) failent, sizeof *failent);
	close(fd);
}


