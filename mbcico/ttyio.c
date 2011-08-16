/*****************************************************************************
 *
 * $Id: ttyio.c,v 1.20 2004/02/21 17:22:01 mbroek Exp $
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
 *   
 * Michiel Broek		FIDO:		2:280/2802
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

/* ### Modified by P.Saratxaga on 25 Oct 1995 ###
 * - Added if (inetaddr) code from T. Tanaka
 */
#include "../config.h"
#include "../lib/mbselib.h"
#include "ttyio.h"
#include "lutil.h"

extern	int	hanged_up;
extern	char	*inetaddr;

#define TT_BUFSIZ	1024
#define NUMTIMERS	3


int		tty_status = 0;
int		f_flags;
static	char	buffer[TT_BUFSIZ];
static	char	*next;
static	int	left = 0;


static	time_t	timer[NUMTIMERS];

char *ttystat[]= {(char *)"Ok", 
		  (char *)"Error", 
		  (char *)"TimeOut", 
		  (char *)"EOF", 
		  (char *)"Hangup", 
		  (char *)"Empty",
		  (char *)"UnCompress"};

int  tty_resettimer(int tno);
void tty_resettimers(void);
int  tty_settimer(int,int);
int  tty_expired(int);
int  tty_running(int);

#define RESETTIMER(x) tty_resettimer(x)
#define RESETTIMERS() tty_resettimers()
#define SETTIMER(x,y) tty_settimer(x,y)
#define EXPIRED(x) tty_expired(x)
#define RUNNING(x) tty_running(x)



/*
 * timer functions
 */

int tty_resettimer(int tno)
{
    if (tno >= NUMTIMERS) {
	errno = EINVAL;
	WriteError("ttyio: invalid timer No for resettimer(%d)", tno);
	return -1;
    }

    timer[tno] = (time_t) 0;
    return 0;
}



void tty_resettimers(void)
{
    int i;

    for (i = 0; i < NUMTIMERS; i++) 
	timer[i] = (time_t)0;
}



int tty_settimer(int tno, int interval)
{
    if (tno >= NUMTIMERS) {
	errno = EINVAL;
	WriteError("ttyio: invalid timer No for settimer(%d)", tno);
	return -1;
    }

    timer[tno]=time((time_t*)NULL)+interval;
    return 0;
}



int tty_expired(int tno)
{
    time_t	now;

    if (tno >= NUMTIMERS) {
	errno = EINVAL;
	WriteError("ttyio: invalid timer No for expired(%d)", tno);
	return -1;
    }

    /*
     * Check if timer is running
     */
    if (timer[tno] == (time_t) 0)
	return 0;

    now = time(NULL);
    return (now >= timer[tno]);
}



int tty_running(int tno)
{
    if (tno > NUMTIMERS) {
	errno = EINVAL;
	WriteError("ttyio: invalid timer for tty_running(%d)", tno);
	return -1;
    }

    /*
     * check if timer is running
     */
    if (timer[tno] == (time_t) 0)
	return 0;
    else
	return 1;
}



/*
 * private r/w functions
 */
static int tty_read(char *buf, int size, int tot)
{
    time_t	    timeout, now;
    int		    i, rc;
    fd_set	    readfds, writefds, exceptfds;
    struct timeval  seltimer;

    if (size == 0) 
	return 0;
    tty_status = 0;

    now = time(NULL);
    timeout = (time_t)300; /* maximum of 5 minutes */

    for (i = 0; i < NUMTIMERS; i++) {
	if (timer[i]) {
	    if (now >= timer[i]) {
		tty_status=STAT_TIMEOUT;
		Syslog('!', "tty_read: timer %d already expired, return", i);
		return -tty_status;
	    } else {
		if (timeout > (timer[i]-now))
		    timeout=timer[i]-now;
	    }
	}
    }
    if ((tot != -1) && (timeout > tot))
	timeout=tot;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(0,&readfds);
    FD_SET(0,&exceptfds);
    seltimer.tv_sec=timeout;
    seltimer.tv_usec=0;

    rc = select(1,&readfds,&writefds,&exceptfds,&seltimer);

    if (rc < 0) {
	if (hanged_up) {
	    tty_status=STAT_HANGUP;
	    WriteError("tty_read: hanged_up flag");
	} else {
	    WriteError("$tty_read: select for read failed");
	    tty_status = STAT_ERROR;
	}
    } else if (rc == 0) {
	tty_status = STAT_TIMEOUT;
    } else { /* rc > 0 */
	if (FD_ISSET(0,&exceptfds)) {
	    Syslog('!', "$tty_read: exeption error");
	    tty_status = STAT_ERROR;
	}
    }

    if (tty_status) {
	return -tty_status;
    }

    if (!FD_ISSET(0,&readfds)) {
	WriteError("tty_read: Cannot be: select returned but read fd not set");
	tty_status = STAT_ERROR;
	return -tty_status;
    }

    rc = read(0,buf,size);

    if (rc <= 0) {
	Syslog('t', "tty_read: return %d",rc);
	if (hanged_up || (errno == EPIPE) || (errno == ECONNRESET)) {
	    tty_status = STAT_HANGUP;
	    WriteError("tty_read: hanged_up flag");
	} else {
	    tty_status = STAT_ERROR;
	    Syslog('!', "tty_read: error flag");
	}
	rc=-tty_status;
    }

    return rc;
}



int tty_write(char *buf, int size)
{
    int result;

    tty_status=0;
    result = write(1, buf, size);

    if (result != size) {
	if (hanged_up || (errno == EPIPE) || (errno == ECONNRESET)) {
	    tty_status = STAT_HANGUP;
	    WriteError("tty_write: hanged_up flag");
	} else {
	    tty_status=STAT_ERROR;
	    Syslog('!', "tty_write: error flag");
	}
    }
    if (tty_status)
	Syslog('t', "tty_write: error %s", ttystat[tty_status]);

    return -tty_status;
}



/* public r/w functions */

/*
 * Check if there is data available on stdin.
 */
int tty_check(void)
{
    int rc;

    // try to read available (timeout = 0) data if we have no data in
    // our buffer

    if (!left) {
	rc = tty_read(buffer, TT_BUFSIZ, 0);
	if (rc > 0) {
	    left = rc;
	}
    }

    return (left > 0);
}



int tty_putcheck(int size)
{
    fd_set	    set;
    struct timeval  timeout;
     
    /*
     * Initialize the file descriptor set. 
     */
    FD_ZERO(&set);
    FD_SET(1, &set);
     
    /*
     * Initialize the timeout data structure. 
     */
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
     
    /*
     * `select' returns 0 if timeout, 1 if input available, -1 if error. 
     */
    return select(FD_SETSIZE, NULL, &set, NULL, &timeout);
}



int tty_waitputget(int tot)
{
    int		    i, rc;
    time_t	    timeout, now;
    fd_set	    readfds, writefds, exceptfds;
    struct timeval  seltimer;

    tty_status=0;
    now = time(NULL);
    timeout=(time_t)300; /* maximum of 5 minutes */

    for (i = 0; i < NUMTIMERS; i++) {
	if (timer[i]) {
	    if (now >= timer[i]) {
		tty_status = STAT_TIMEOUT;
		WriteError("tty_waitputget: timer %d already expired, return",i);
		return -tty_status;
	    } else {
		if (timeout > (timer[i]-now))
		    timeout = timer[i]-now;
	    }
	}
    }
    if ((tot != -1) && (timeout > tot))
	timeout=tot;
    Syslog('t', "tty_waitputget: timeout=%d",timeout);

    /*
     * Initialize the file descriptor set. 
     */
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);

    FD_SET(0, &readfds);
    FD_SET(1, &writefds);
    FD_SET(0, &exceptfds);
    FD_SET(1, &exceptfds);
     
    /*
     * Initialize the timeout data structure. 
     */
    seltimer.tv_sec = timeout;
    seltimer.tv_usec = 0;
     
    /*
     * `select' returns 0 if timeout, 1 if input available, -1 if error. 
     */
    rc = select(FD_SETSIZE, &readfds, &writefds, &exceptfds, &seltimer);

    if (rc < 0) {
	if (hanged_up) {
	    tty_status=STAT_HANGUP;
	    WriteError("tty_waitputget: hanged_up flag");
	} else {
	    WriteError("$tty_waitputget: select failed");
	    tty_status=STAT_ERROR;
	}
    } else if (rc == 0) {
	tty_status=STAT_TIMEOUT;
    } else { 
	/* rc > 0 */
	if ((FD_ISSET(0,&exceptfds)) || (FD_ISSET(1,&exceptfds))) {
	    WriteError("$tty_waitputget: exeption error");
	    tty_status=STAT_ERROR;
	}
    }

    if (tty_status) {
	Syslog('t', "tty_waitputget: return after select status %s",ttystat[tty_status]);
	return -tty_status;
    }

    rc = 0;

    if (FD_ISSET(0,&readfds))
	rc |= 1;

    if (FD_ISSET(1,&writefds))
	rc |= 2;

    return rc;
}



void tty_flushin(void)
{
    tcflush(0, TCIFLUSH);
}



void tty_flushout(void)
{
    tcflush(1, TCOFLUSH);
}



int tty_ungetc(int c)
{
    if (next == buffer) {
	if (left >= TT_BUFSIZ) {
	    return -1;
	}

	next = buffer + TT_BUFSIZ - left;
	memcpy(next, buffer, left);
    }

    next--;
    *next = c;
    left++;

    return 0;
}



int tty_getc(int tot)
{
    if (!left) {
	left=tty_read(buffer,TT_BUFSIZ,tot);
	next=buffer;
    }

    if (left <= 0) {
	left=0;
	return -tty_status;
    } else {
	left--;
	return (*next++)&0xff;
    }
}



int tty_get(char *buf, int size, int tot)
{
    int result=0;

    if (left >= size) {
	memcpy(buf,next,size);
	next += size;
	left -= size;
	return 0;
    }

    if (left > 0) {
	memcpy(buf,next,left);
	buf += left;
	next += left;
	size -= left;
	left=0;
    }

    while ((result=tty_read(buf,size,tot)) > 0) {
	buf += result;
	size -= result;
    }

    return result;
}



int tty_putc(int c)
{
    char    buf = c;

    return tty_write(&buf,1);
}



int tty_put(char *buf, int size)
{
    return tty_write(buf,size);
}



int tty_putget(char **obuf, int *osize, char **ibuf, int *isize)
{
    time_t	    timeout, now;
    int		    i, rc;
    fd_set	    readfds, writefds, exceptfds;
    struct timeval  seltimer;

    tty_status = 0;
    now = time(NULL);
    timeout = (time_t)300; /* maximum of 5 minutes */

    for (i = 0; i < NUMTIMERS; i++) {
	if (timer[i]) {
	    if (now >= timer[i]) {
		tty_status = STAT_TIMEOUT;
		WriteError("tty_putget: timer %d already expired, return",i);
		return -tty_status;
	    } else {
		if (timeout > (timer[i]-now))
		    timeout=timer[i]-now;
	    }
	}
    }

    Syslog('t', "tty_putget: timeout=%d",timeout);

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(0,&readfds);
    FD_SET(1,&writefds);
    FD_SET(0,&exceptfds);
    FD_SET(1,&exceptfds);
    seltimer.tv_sec=timeout;
    seltimer.tv_usec=0;

    rc=select(2,&readfds,&writefds,&exceptfds,&seltimer);
    if (rc < 0) {
	if (hanged_up) {
	    tty_status=STAT_HANGUP;
	    WriteError("tty_putget: hanged_up flag");
	} else {
	    WriteError("$tty_putget: select failed");
	    tty_status=STAT_ERROR;
	}
    } else if (rc == 0) {
	tty_status=STAT_TIMEOUT;
    } else {
	/* rc > 0 */
	if ((FD_ISSET(0,&exceptfds)) || (FD_ISSET(1,&exceptfds))) {
	    WriteError("$tty_putget: exeption error");
	    tty_status=STAT_ERROR;
	}
    }

    if (tty_status) {
	Syslog('t', "tty_putget: return after select status %s",ttystat[tty_status]);
	return -tty_status;
    }

    if (FD_ISSET(0,&readfds) && *isize) {
	rc = read(0, *ibuf, *isize);
	if (rc < 0) {
	    WriteError("$tty_putget: read failed");
	    tty_status=STAT_ERROR;
	} else {
	    (*ibuf)+=rc;
	    (*isize)-=rc;
	}
    }

    if (FD_ISSET(1,&writefds) && *osize) {
	rc=write(1, *obuf, *osize);
	if (rc < 0) {
	    WriteError("$tty_putget: write failed");
	    tty_status=STAT_ERROR;
	} else {
	    (*obuf)+=rc;
	    (*osize)-=rc;
	}
    }

    if (tty_status) 
	return -tty_status;
    else 
	return ((*isize == 0) | ((*osize == 0) << 1));
}


