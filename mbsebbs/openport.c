/*****************************************************************************
 *
 * $Id$
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

static struct termios savetios;
static struct termios tios;



/*
 * Set port to local, hangup using DTR drop.
 */
void hangup(void)
{
    struct termios  Tios;
    tcflag_t        cflag;
    speed_t         ispeed, ospeed;
    int             rc;

    Syslog('t', "Setting port local");
    
    if (isatty(0)) {
	if ((rc = tcgetattr(0,&Tios))) {
	    WriteError("$tcgetattr(0,save) return %d",rc);
	    return;
	}
	Syslog('+', "Lowering DTR to hangup");

	cflag = Tios.c_cflag | CLOCAL;

	ispeed = cfgetispeed(&tios);
	ospeed = cfgetospeed(&tios);
	cfsetispeed(&Tios,0);
	cfsetospeed(&Tios,0);
	if ((rc = tcsetattr(0,TCSADRAIN,&Tios)))
	    WriteError("$tcsetattr(0,TCSADRAIN,hangup) return %d",rc);

	sleep(1); /* as far as I notice, DTR goes back high on next op. */

	Tios.c_cflag = cflag;
	cfsetispeed(&Tios,ispeed);
	cfsetospeed(&Tios,ospeed);
	if ((rc = tcsetattr(0,TCSADRAIN,&Tios)))
	    Syslog('t', "$tcsetattr(0,TCSADRAIN,clocal) return %d",rc);
    } else {
	Syslog('t', "Not at a tty");
    }

    return;
}



/*
 * Put the current opened tty in raw mode.
 */
int rawport(void)
{
    int     rc = 0;

    Syslog('t', "rawport()");
    tty_status = 0;

    Syslog('t', "ttyname 0 %s", ttyname(0));
    Syslog('t', "ttyname 1 %s", ttyname(1));
    Syslog('t', "fd = %d", fileno(stdin));
    Syslog('t', "fd = %d", fileno(stdout));
    Syslog('t', "fd = %d", fileno(stderr));
    
    if (isatty(0)) {
	if ((rc = tcgetattr(0,&savetios))) {
	    WriteError("$tcgetattr(0,save) return %d",rc);
	    return rc;
	}

	tios = savetios;
    	tios.c_iflag &= ~(INLCR | ICRNL | ISTRIP | IXON  ); /* IUCLC removed for FreeBSD */
        /*
	 *  Map CRNL modes strip control characters and flow control
	 */
        tios.c_oflag &= ~OPOST;            /* Don't do ouput character translation */
	tios.c_lflag &= ~(ICANON | ECHO);  /* No canonical input and no echo       */
	tios.c_cc[VMIN] = 1;               /* Receive 1 character at a time        */
	tios.c_cc[VTIME] = 0;              /* No time limit per character          */

	if ((rc = tcsetattr(0,TCSADRAIN,&tios)))
	    WriteError("$tcsetattr(0,TCSADRAIN,raw) return %d",rc);

    } else {
	Syslog('t', "not at a tty");
    }
    return rc;
}



int cookedport(void)
{
    int	    rc = 0;

    Syslog('t', "cookedport()");
    if (isatty(0)) {
	if ((rc = tcsetattr(0,TCSAFLUSH,&savetios)))
	    Syslog('t', "$tcsetattr(0,TCSAFLUSH,save) return %d", rc);
    }

    return rc;
}



void sendbrk(void)
{
    Syslog('t', "Send break");
    
    if (isatty(0)) {
#if (defined(TIOCSBRK))
	Syslog('t', "TIOCSBRK");
	ioctl(0, TIOCSBRK, 0L);
#elif (defined(TCSBRK))
	Syslog('t', "TCSBRK");
	ioctl(0, TCSBRK, 0L);
#else /* any ideas about BSD? */
	;
#endif
    }
}


