/*****************************************************************************
 *
 * $Id: openport.c,v 1.16 2007/02/11 14:52:48 mbse Exp $
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "zmmisc.h"

#ifdef USE_SGTTY
#  ifdef LLITOUT
int Locmode;           /* Saved "local mode" for 4.x BSD "new driver" */
int Locbit = LLITOUT;  /* Bit SUPPOSED to disable output translations */
#  endif
#endif

#ifdef USE_TERMIOS
struct termios oldtty, tty;
#else
#  if defined(USE_TERMIO)
struct termio oldtty, tty;
#  else
struct sgttyb oldtty, tty;
struct tchars oldtch, tch;
#  endif
#endif

int			hanged_up = 0;
unsigned		Baudrate = 9600;    /* Default, set by first io_mode() call */



static struct {
    unsigned	baudr;
    speed_t	speedcode;
} speeds[] = {
    {110,   B110},
    {300,   B300},
    {600,   B600},
    {1200,  B1200},
    {2400,  B2400},
    {4800,  B4800},
    {9600,  B9600},
#ifdef B12000
    {12000, B12000},
#endif
#ifdef B14400
    {14400, B14400},
#endif
#ifdef B19200
    {19200,  B19200},
#endif
#ifdef B38400
    {38400,  B38400},
#endif
#ifdef B57600
    {57600,  B57600},
#endif
#ifdef B115200
    {115200,  B115200},
#endif
#ifdef B230400
    {230400,  B230400},
#endif
#ifdef B460800
    {460800,  B460800},
#endif
#ifdef B500000
    {500000, B500000},
#endif
#ifdef EXTA
    {19200, EXTA},
#endif
#ifdef EXTB
    {38400, EXTB},
#endif
    {0, 0}
};



unsigned getspeed(speed_t);
unsigned getspeed(speed_t code)
{
    int n;

    for (n = 0; speeds[n].baudr; ++n)
	if (speeds[n].speedcode == code)
	    return speeds[n].baudr;
    return 38400;   /* Assume fifo if ioctl failed */
}



/*
 * Set port to local, hangup using DTR drop.
 */
void hangup(void)
{
    struct termios  Tios;
    tcflag_t        cflag;
    speed_t         ispeed, ospeed;
    int             rc;

    Syslog('t', "hangup()");
    
    if (isatty(0)) {
	if ((rc = tcgetattr(0,&Tios))) {
	    WriteError("$tcgetattr(0,save) return %d",rc);
	    return;
	}
	Syslog('+', "Lowering DTR to hangup");

	cflag = Tios.c_cflag | CLOCAL;

	ispeed = cfgetispeed(&tty);
	ospeed = cfgetospeed(&tty);
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
 * mode(n)
 *  3: save old tty stat, set raw mode with flow control
 *  2: set XON/XOFF for sb/sz with ZMODEM or YMODEM-g
 *  1: save old tty stat, set raw mode 
 *  0: restore original tty mode
 */
char *io_names[] = { (char *)"restore-tty", (char *)"save-tty set-raw", (char *)"set-Xon/Xoff", (char *)"save-tty set-raw-flow" };
int io_mode(int fd, int n)
{
    static int	did0 = FALSE;

    Syslog('t', "io_mode(%d, %d) (%s)", fd, n, io_names[n]);

    switch(n) {

#ifdef USE_TERMIOS
	case 2:
		if (!did0) {
		    tcgetattr(fd,&oldtty);
		}
		tty = oldtty;

		tty.c_iflag = BRKINT | IXON;
		tty.c_oflag = 0;		    /* Transparent output	*/
		tty.c_cflag &= ~( CSIZE | PARENB ); /* Disable parity and all character sizes	*/
		tty.c_cflag |= CS8 | CREAD;
		tty.c_lflag = (protocol == ZM_ZMODEM) ? 0 : ISIG;
		tty.c_cc[VINTR] = (protocol == ZM_ZMODEM) ? -1:030; /* Interrupt char */
		tty.c_cc[VQUIT] = -1;		    /* Quit char */
		tty.c_cc[VMIN] = 3;		    /* This many chars satisfies reads */
		tty.c_cc[VTIME] = 1;

		tcsetattr(fd,TCSADRAIN,&tty);
		did0 = TRUE;
                Baudrate = getspeed(cfgetospeed(&tty));
		Syslog('t', "Baudrate = %d", Baudrate);
				
		return 0;
	case 1:
	case 3:
		if (!did0) {
		    tcgetattr(fd,&oldtty);
		}

		tty = oldtty;

		tty.c_iflag = n==3 ? (IXON|IXOFF) : IXOFF;

		/* 
		 * Setup raw mode: no echo, noncanonical (no edit chars),
		 * no signal generating chars, and no extended chars (^V, 
		 * ^O, ^R, ^W).
		 */
		tty.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
		tty.c_oflag = 0;		    /* Transparent output	*/

		tty.c_cflag &= ~( CSIZE | PARENB ); /* Same baud rate, disable parity */
		tty.c_cflag |= CS8;
		tty.c_cc[VMIN] = 2;		    /* This many chars satisfies reads */
		tty.c_cc[VTIME] = 1;
		tcsetattr(fd,TCSADRAIN,&tty);
		Baudrate = getspeed(cfgetospeed(&tty));
		did0 = TRUE;

		return 0;
	case 0:
		if (!did0)
		    return -1;
		tcdrain (fd);			    /* wait until everything is sent */
		tcflush (fd,TCIOFLUSH); 	    /* flush input queue */
		tcsetattr (fd,TCSADRAIN,&oldtty);   /* Restore */
		tcflow (fd,TCOON);		    /* restart output */

		return 0;
#endif

#ifdef USE_TERMIO
#error USE_TERMIO driver not coded
#endif

#ifdef USE_SGTTY
#error USE_SGTTY driver not coded
#endif
    }
    return -1;
}



int rawport(void)
{
    Syslog('t', "rawport()");
    return io_mode(0, 1);
}



int cookedport(void)
{
    Syslog('t', "cookedport()");
    return io_mode(0, 0);
}



void sendbrk(void)
{
    Syslog('t', "Send break");

    if (isatty(0)) {
#ifdef USE_TERMIOS
	tcsendbreak(0, 0);
#endif
#ifdef USE_TERMIO
	ioctl(0, TCSBRK, 0);
#endif
#ifdef USE_SGTTY
#ifdef TIOCSBRK
	sleep(1);
	ioctl(0, TIOCSBRK, 0);
	sleep(1);
	ioctl(0, TIOCCBRK, 0);
#endif
#endif
    }
}


