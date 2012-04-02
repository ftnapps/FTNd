/*****************************************************************************
 *
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
#include "ulock.h"
#include "ttyio.h"
#include "mbcico.h"
#include "openport.h"


static char	*openedport = NULL, *pname;
static int	need_detach = 1;
extern int	f_flags;
static speed_t	transpeed(int);
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



void interrupt(int sig)
{
    Syslog('+', "openport: Got SIGINT");
    signal(SIGINT,interrupt);
    return;
}



int openport(char *port, int speed)
{
    int	    rc, rc2, fd, outflags;
    char    *errtty=NULL;

    Syslog('t', "Try opening port \"%s\" at %d",MBSE_SS(port),speed);
    if (openedport) 
	free(openedport);
    openedport = NULL;
    if (port[0] == '/') 
	openedport = xstrcpy(port);
    else {
	openedport = xstrcpy((char *)"/dev/");
	openedport = xstrcat(openedport, port);
    }
    pname = strrchr(openedport, '/');

    if ((rc = lock(pname))) {
	Syslog('+', "Port %s is locked (rc = %d)", port, rc);
	free(openedport);
	openedport = NULL;
	return rc;
    }

    if (need_detach) {
	fflush(stdin);
	fflush(stdout);
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	close(0);
	close(1);

	/*
	 * If we were manual started the error tty must be closed.
	 */
	if ((errtty = ttyname(2))) {
	    Syslog('t', "openport: stderr was on \"%s\", closing",errtty);
	    fflush(stderr);
	    close(2);
	}
    }
    tty_status = 0;
    hanged_up = 0;
    signal(SIGHUP, linedrop);
    signal(SIGPIPE, sigpipe);
    signal(SIGINT, interrupt);
    rc = 0;
    rc2 = 0;

    if ((fd = open(openedport,O_RDONLY|O_NONBLOCK)) != 0) {
	rc = 1;
	Syslog('+', "$Cannot open \"%s\" as stdin",MBSE_SS(openedport));
	fd = open("/dev/null",O_RDONLY);
    }

    if ((fd = open(openedport,O_WRONLY|O_NONBLOCK)) != 1) {
	rc = 1;
	Syslog('+', "$Cannot open \"%s\" as stdout",MBSE_SS(openedport));
	fd = open("/dev/null",O_WRONLY);
    }

    clearerr(stdin);
    clearerr(stdout);
    if (need_detach) {
#ifdef TIOCSCTTY
	if ((rc2 = ioctl(0,TIOCSCTTY,1L)) < 0) {
	    Syslog('t', "$TIOCSCTTY failed rc = %d", rc2);
	}
#endif
	if (errtty) {
	    rc = rc || (open(errtty,O_WRONLY) != 2);
	}
	need_detach=0;
    }

    if (rc) 
	Syslog('+', "cannot switch i/o to port \"%s\"",MBSE_SS(openedport));
    else {
	if (tty_raw(speed)) {
	    WriteError("$cannot set raw mode for \"%s\"",MBSE_SS(openedport));
	    rc=1;
	}

	if (((f_flags = fcntl(0, F_GETFL, 0L)) == -1) || ((outflags = fcntl(1, F_GETFL, 0L)) == -1)) {
	    rc = 1;
	    WriteError("$GETFL error");
	    f_flags = 0;
	    outflags = 0;
	} else {
	    Syslog('t', "Return to blocking mode");
	    f_flags &= ~O_NONBLOCK;
	    outflags &= ~O_NONBLOCK;
	    if ((fcntl(0, F_SETFL, f_flags) != 0) || (fcntl(1, F_SETFL, outflags) != 0)) {
		rc = 1;
		WriteError("$SETFL error");
	    }
	}
	Syslog('t', "File flags: stdin: 0x%04x, stdout: 0x%04x", f_flags,outflags);
    }

    if (rc) 
	closeport();
    else
	SetTTY(port);

    return rc;
}



/*
 * Set port to local, hangup using DTR drop.
 */
void localport(void)
{
    Syslog('t', "Setting port \"%s\" local",MBSE_SS(openedport));
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    
    if (isatty(0)) 
	tty_local();
    
    return;
}



void nolocalport(void)
{
    Syslog('t', "Setting port \"%s\" non-local",MBSE_SS(openedport));
    if (isatty(0)) 
	tty_nolocal();
    
    return;
}



int rawport(void)
{
    tty_status = 0;
    signal(SIGHUP, linedrop);
    signal(SIGPIPE, sigpipe);

    if (isatty(0)) 
	return tty_raw(0);
    else 
	return 0;
}



int cookedport(void)
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    if (isatty(0)) 
	return tty_cooked();
    else 
	return 0;
}



void closeport(void)
{
    if (openedport == NULL)
	return;

    Syslog('t', "Closing port \"%s\"",MBSE_SS(openedport));
    fflush(stdin);
    fflush(stdout);
    tty_cooked();
    close(0);
    close(1);
    ulock(pname);
    if (openedport) 
	free(openedport);
    openedport = NULL;
    SetTTY((char *)"-");
    return;
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



char *bstr(speed_t);
char *bstr(speed_t sp)
{
#ifdef CBAUD
	switch(sp & CBAUD) {
#else
	switch(sp) {
#endif
		case 0:		return (char *)"0";
#if defined(B50)
		case B50:	return (char *)"50";
#endif
#if defined(B75)
		case B75:	return (char *)"75";
#endif
#if defined(B110)
		case B110:	return (char *)"110";
#endif
#if defined(B134)
		case B134:	return (char *)"134";
#endif
#if defined(B150)
		case B150:	return (char *)"150";
#endif
#if defined(B200)
		case B200:	return (char *)"200";
#endif
#if defined(B300)
		case B300:	return (char *)"300";
#endif
#if defined(B600)
		case B600:	return (char *)"600";
#endif
#if defined(B1200)
		case B1200:	return (char *)"1.200";
#endif
#if defined(B1800)
		case B1800:	return (char *)"1.800";
#endif
#if defined(B2400)
		case B2400:	return (char *)"2.400";
#endif
#if defined(B4800)
		case B4800:	return (char *)"4.800";
#endif
#if defined(B9600)
		case B9600:	return (char *)"9.600";
#endif
#if defined(B19200)
		case B19200:	return (char *)"19.200";
#endif
#if defined(B38400)
		case B38400:	return (char *)"38.400";
#endif
#if defined(B57600)
		case B57600:	return (char *)"57.600";
#endif
#if defined(B115200)
		case B115200:	return (char *)"115.200";
#endif
#if defined(B230400)
		case B230400:	return (char *)"230.400";
#endif
#if defined(B460800)
		case B460800:	return (char *)"460.800";
#endif
#if defined(B500000)
		case B500000:	return (char *)"500.000";
#endif
#if defined(B576000)
		case B576000:	return (char *)"576.000";
#endif
#if defined(B921600)
		case B921600:	return (char *)"921.600";
#endif
#if defined(B1000000)
		case B1000000:	return (char *)"1.000.000";
#endif
#if defined(B1152000)
		case B1152000:	return (char *)"1.152.000";
#endif
#if defined(B1500000)
		case B1500000:	return (char *)"1.500.000";
#endif
#if defined(B2000000)
		case B2000000:	return (char *)"2.000.000";
#endif
#if defined(B2500000)
		case B2500000:	return (char *)"2.500.000";
#endif
#if defined(B3000000)
		case B3000000:	return (char *)"3.000.000";
#endif
#if defined(B3500000)
		case B3500000:	return (char *)"3.500.000";
#endif
#if defined(B4000000)
		case B4000000:	return (char *)"4.000.000";
#endif
		default:	return (char *)"Unknown";
	}
}



static struct termios savetios;
static struct termios tios;


int tty_raw(int speed)
{
    int	    rc;
    speed_t tspeed;

    Syslog('t', "Set tty raw");
    tspeed = transpeed(speed);

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

    if (tspeed) {
	cfsetispeed(&tios,tspeed);
	cfsetospeed(&tios,tspeed);
    }

    if ((rc = tcsetattr(0,TCSADRAIN,&tios)))
	WriteError("$tcsetattr(0,TCSADRAIN,raw) return %d",rc);

    cfgetispeed(&tios);
    cfgetospeed(&tios);

    return rc;
}



int tty_local(void)
{
    struct termios  Tios;
    tcflag_t	    cflag;
    speed_t	    ispeed, ospeed;
    int		    rc;

    if ((rc = tcgetattr(0,&Tios))) {
	WriteError("$tcgetattr(0,save) return %d",rc);
	return rc;
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
    return rc;
}



int tty_nolocal(void)
{
    struct termios  Tios;
    int		    rc;

    if ((rc = tcgetattr(0,&Tios))) {
	WriteError("$tcgetattr(0,save) return %d",rc);
	return rc;
    }
    Tios.c_cflag &= ~CLOCAL;
    Tios.c_cflag |= CRTSCTS;

    if ((rc = tcsetattr(0,TCSADRAIN,&Tios)))
	Syslog('t', "$tcsetattr(0,TCSADRAIN,clocal) return %d",rc);
    return rc;
}



int tty_cooked(void)
{
    int	    rc;

    if ((rc = tcsetattr(0,TCSAFLUSH,&savetios)))
	Syslog('t', "$tcsetattr(0,TCSAFLUSH,save) return %d",rc);
    return rc;
}



speed_t transpeed(int speed)
{
    speed_t tspeed;

    switch (speed) {
	case 0:		tspeed=0; break;
#if defined(B50)
	case 50:	tspeed=B50; break;
#endif
#if defined(B75)
	case 75:	tspeed=B75; break;
#endif
#if defined(B110)
	case 110:	tspeed=B110; break;
#endif
#if defined(B134)
	case 134:	tspeed=B134; break;
#endif
#if defined(B150)
	case 150:	tspeed=B150; break;
#endif
#if defined(B200)
	case 200:	tspeed=B200; break;
#endif
#if defined(B300)
	case 300:	tspeed=B300; break;
#endif
#if defined(B600)
	case 600:	tspeed=B600; break;
#endif
#if defined(B1200)
	case 1200:	tspeed=B1200; break;
#endif
#if defined(B1800)
	case 1800:	tspeed=B1800; break;
#endif
#if defined(B2400)
	case 2400:	tspeed=B2400; break;
#endif
#if defined(B4800)
	case 4800:	tspeed=B4800; break;
#endif
#if defined(B7200)
	case 7200:	tspeed=B7200; break;
#endif
#if defined(B9600)
	case 9600:	tspeed=B9600; break;
#endif
#if defined(B12000)
	case 12000:	tspeed=B12000; break;
#endif
#if defined(B14400)
	case 14400:	tspeed=B14400; break;
#endif
#if defined(B19200)
	case 19200:	tspeed=B19200; break;
#elif defined(EXTA)
	case 19200:	tspeed=EXTA; break;
#endif
#if defined(B38400)
	case 38400:	tspeed=B38400; break;
#elif defined(EXTB)
	case 38400:	tspeed=EXTB; break;
#endif
#if defined(B57600)
	case 57600:	tspeed=B57600; break;
#endif
#if defined(B115200)
	case 115200:	tspeed=B115200; break;
#endif
#if defined(B230400)
	case 230400:	tspeed=B230400; break;
#endif
#if defined(B460800)
	case 460800:	tspeed=B460800; break;
#endif
#if defined(B500000)
	case 500000:	tspeed=B500000; break;
#endif
#if defined(B576000)
	case 576000:	tspeed=B576000; break;
#endif
#if defined(B921600)
	case 921600:	tspeed=B921600; break;
#endif
#if defined(B1000000)
	case 1000000:	tspeed=B1000000; break;
#endif
#if defined(B1152000)
	case 1152000:	tspeed=B1152000; break;
#endif
#if defined(B1500000)
	case 1500000:	tspeed=B1500000; break;
#endif
#if defined(B2000000)
	case 2000000:	tspeed=B2000000; break;
#endif
#if defined(B2500000)
	case 2500000:	tspeed=B2500000; break;
#endif
#if defined(B3000000)
	case 3000000:	tspeed=B3000000; break;
#endif
#if defined(B3500000)
	case 3500000:	tspeed=B3500000; break;
#endif
#if defined(B4000000)
	case 4000000:	tspeed=B4000000; break;
#endif
	default:	WriteError("requested invalid speed %d",speed);
			tspeed=0; break;
    }

    return tspeed;
}


