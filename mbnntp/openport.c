/*****************************************************************************
 *
 * $Id$
 * File ..................: mbnntp/openport.c
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
 *   
 * Michiel Broek		FIDO:	2:280/2802
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
#include "openport.h"


int		hanged_up = 0;



void linedrop(int sig)
{
    Syslog('+', "openport: Lost Carrier");
    hanged_up=1;
    return;
}



void sigpipe(int sig)
{
    Syslog('+', "openport: Got SIGPIPE");
    hanged_up=1;
    return;
}



int rawport(void)
{
    tty_status = 0;
    Syslog('t', "SIGHUP => linedrop()");
    signal(SIGHUP, linedrop);
    Syslog('t', "SIGPIPE => sigpipe()");
    signal(SIGPIPE, sigpipe);

    if (isatty(0)) 
	return tty_raw(0);
    else 
	return 0;
}



int cookedport(void)
{
    Syslog('t', "SIGHUP => SIG_IGN");
    signal(SIGHUP, SIG_IGN);
    Syslog('t', "SIGPIPE => SIG_IGN");
    signal(SIGPIPE, SIG_IGN);
    if (isatty(0)) 
	return tty_cooked();
    else 
	return 0;
}


static struct termios savetios;
static struct termios tios;


int tty_raw(int speed)
{
    int	    rc;

    Syslog('t', "Set tty raw");

    if ((rc = tcgetattr(0,&savetios))) {
	WriteError("$tcgetattr(0,save) return %d",rc);
	return rc;
    }

    tios = savetios;
    tios.c_iflag = 0;
    tios.c_oflag = 0;
    tios.c_cflag &= ~(CSTOPB | PARENB | PARODD);
    tios.c_cflag |= CS8 | CREAD | HUPCL | CLOCAL;
    tios.c_lflag = 0;
    tios.c_cc[VMIN] = 1;
    tios.c_cc[VTIME] = 0;

    if ((rc = tcsetattr(0,TCSADRAIN,&tios)))
	WriteError("$tcsetattr(0,TCSADRAIN,raw) return %d",rc);

    return rc;
}


int tty_cooked(void)
{
    int	    rc;

    if ((rc = tcsetattr(0,TCSAFLUSH,&savetios)))
	Syslog('t', "$tcsetattr(0,TCSAFLUSH,save) return %d",rc);
    return rc;
}


