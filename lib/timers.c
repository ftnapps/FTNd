/*****************************************************************************
 *
 * timers.c
 * Purpose ...............: General Purpose Timers
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
#include "ftndlib.h"


/*
 * Number of timers
 */
#define	GENTIMERS   3

static time_t	gentimer[GENTIMERS];


/*
 * Reset a timer
 */
int gpt_resettimer(int tno)
{
    if (tno >= GENTIMERS) {
	errno = EINVAL;
	WriteError("invalid timer number for gpt_resettimer(%d)", tno);
	return -1;
    }

    gentimer[tno] = (time_t) 0;
    return 0;
}



void gpt_resettimers(void)
{
    int	    i;

    for (i = 0; i < GENTIMERS; i++)
	gpt_resettimer(i);
}



/*
 * Set timer
 */
int gpt_settimer(int tno, int interval)
{
    if (tno >= GENTIMERS) {
	errno = EINVAL;
	WriteError("invalid timer number for gpt_settimer(%d)", tno);
	return -1;
    }

    gentimer[tno] = time((time_t*)NULL) + interval;
    return 0;
}



/*
 * Check if timer is expired
 */
int gpt_expired(int tno)
{
    time_t  now;

    if (tno >= GENTIMERS) {
	errno = EINVAL;
	WriteError("invalid timer number for gpt_expired(%d)", tno);
	return -1;
    }

    /*
     * Check if timer is running
     */
    if (gentimer[tno] == (time_t) 0)
	return 0;

    now = time(NULL);
    return (now >= gentimer[tno]);
}



int gpt_running(int tno)
{
    if (tno >= GENTIMERS) {
	errno = EINVAL;
	WriteError("invalid timer number for gpt_running(%d)", tno);
	return -1;
    }

    if (gentimer[tno] == (time_t) 0)
	return 0;
    else
	return 1;
}



/*
 * Milliseconds timer, returns 0 on success.
 */
int msleep(int msecs)
{
    int		    rc;
    struct timespec req, rem;

    rem.tv_sec = 0;
    rem.tv_nsec = 0;
    req.tv_sec = msecs / 1000;
    req.tv_nsec = (msecs % 1000) * 1000000;

    while (TRUE) {

	rc = nanosleep(&req, &rem);
	if (rc == 0)
	    break;
	if ((errno == EINVAL) || (errno == EFAULT)) {
	    WriteError("$msleep(%d)", msecs);
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


