/*****************************************************************************
 *
 * $Id: ttyio.c,v 1.4 2005/09/07 20:44:37 mbse Exp $
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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

#include "../config.h"
#include "../lib/mbselib.h"
#include "ttyio.h"

extern	int	hanged_up;
extern	char	*inetaddr;

#define TT_BUFSIZ	1024


int		tty_status = 0;
int		f_flags;
static	char	buffer[TT_BUFSIZ];
static	char	*next;
static	int	left = 0;



char *ttystat[]= {(char *)"Ok", 
		  (char *)"Error", 
		  (char *)"TimeOut", 
		  (char *)"EOF", 
		  (char *)"Hangup", 
		  (char *)"Empty",
		  (char *)"UnCompress"};



/*
 * private r/w functions
 */
static int tty_read(char *buf, int size, int tot)
{
    int		    rc;
    fd_set	    readfds, writefds, exceptfds;
    struct timeval  seltimer;

    if (size == 0) 
	return 0;
    tty_status = 0;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(0,&readfds);
    FD_SET(0,&exceptfds);
    seltimer.tv_sec=tot;
    seltimer.tv_usec=0;

    rc = select(1,&readfds,&writefds,&exceptfds,&seltimer);

    if (rc < 0) {
	if (hanged_up) {
	    tty_status=STAT_HANGUP;
	    WriteError("tty_read: hanged_up flag");
	} else {
	    WriteError("tty_read: select for read failed");
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



void tty_flushin(void)
{
    tcflush(0, TCIFLUSH);
}



void tty_flushout(void)
{
    tcflush(1, TCOFLUSH);
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


int tty_put(char *buf, int size)
{
    return tty_write(buf,size);
}


