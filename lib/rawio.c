/*****************************************************************************
 *
 * $Id: rawio.c,v 1.11 2005/10/11 20:49:46 mbse Exp $
 * Purpose ...............: Raw I/O routines.
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "mbselib.h"


int rawset = FALSE;


/*
 * Sets raw mode and saves the terminal setup
 */
void mbse_Setraw()
{
    int	rc;

    if ((rc = tcgetattr(ttyfd, &tbufs))) {
	perror("");
	printf("$tcgetattr(0, save) return %d\n", rc);
	exit(MBERR_TTYIO_ERROR);
    }

    tbufsavs = tbufs;
    tbufs.c_iflag &= ~(INLCR | ICRNL | ISTRIP | IXON  ); /* IUCLC removed for FreeBSD */
    /*
     *  Map CRNL modes strip control characters and flow control
     */
    tbufs.c_oflag &= ~OPOST;		/* Don't do ouput character translation	*/
    tbufs.c_lflag &= ~(ICANON | ECHO);	/* No canonical input and no echo	*/
    tbufs.c_cc[VMIN] = 1;		/* Receive 1 character at a time	*/
    tbufs.c_cc[VTIME] = 0;		/* No time limit per character		*/

    if ((rc = tcsetattr(ttyfd, TCSADRAIN, &tbufs))) {
	perror("");
	printf("$tcsetattr(%d, TCSADRAIN, raw) return %d\n", ttyfd, rc);
	exit(MBERR_TTYIO_ERROR);
    }

    rawset = TRUE;
}



/*
 * Unsets raw mode and returns state of terminal
 */
void mbse_Unsetraw()
{
    int	rc;

    /*
     * Only unset the mode if it is set to raw mode
     */
    if (rawset == TRUE) {
	if ((rc = tcsetattr(ttyfd, TCSAFLUSH, &tbufsavs))) {
	    perror("");
	    printf("$tcsetattr(%d, TCSAFLUSH, save) return %d\n", ttyfd, rc);
	    exit(MBERR_TTYIO_ERROR);
	}
    }
    rawset = FALSE;
}



/* 
 * This function is used to get a single character from a user ie for a 
 * menu option
 */
unsigned char mbse_Getone()
{
    unsigned char   c = 0;

    if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
	perror("open 8");
	exit(MBERR_TTYIO_ERROR);
    }
    mbse_Setraw();

    c = mbse_Readkey();

    mbse_Unsetraw();
    close(ttyfd);
    return(c);
}



/*
 * Read the (locked) speed from the tty
 */
int mbse_Speed(void)
{
	speed_t		mspeed;

	mspeed = cfgetospeed(&tbufs);
#ifdef CBAUD
	switch (mspeed & CBAUD) {
#else
	switch (mspeed) {
#endif
	    case B0:	    return 0;
#if defined(B50)
	    case B50:	    return 50;
#endif
#if defined(B75)
	    case B75:	    return 75;
#endif
#if defined(B110)
	    case B110:	    return 110;
#endif
#if defined(B134)
	    case B134:	    return 134;
#endif
#if defined(B150)
	    case B150:	    return 150;
#endif
#if defined(B200)
	    case B200:	    return 200;
#endif
#if defined(B300)
	    case B300:	    return 300;
#endif
#if defined(B600)
	    case B600:	    return 600;
#endif
#if defined(B1200)
	    case B1200:	    return 1200;
#endif
#if defined(B1800)
	    case B1800:	    return 1800;
#endif
#if defined(B2400)
	    case B2400:	    return 2400;
#endif
#if defined(B4800)
	    case B4800:	    return 4800;
#endif
#if defined(B9600)
	    case B9600:	    return 9600;
#endif
#if defined(B19200)
	    case B19200:    return 19200;
#endif
#if defined(B38400)
	    case B38400:    return 38400;
#endif
#if defined(B57600)
	    case B57600:    return 57600;
#endif
#if defined(B115200)
	    case B115200:   return 115200;
#endif
#if defined(B230400)
	    case B230400:   return 203400;
#endif
#if defined(B460800)
	    case B460800:   return 460800;
#endif
#if defined(B500000)
	    case B500000:   return 500000;
#endif
#if defined(B576000)
	    case B576000:   return 576000;
#endif
#if defined(B921600)
	    case B921600:   return 921600;
#endif
#if defined(B1000000)
	    case B1000000:  return 1000000;
#endif
#if defined(B1152000)
	    case B1152000:  return 1152000;
#endif
#if defined(B1500000)
	    case B1500000:  return 1500000;
#endif
#if defined(B2000000)
	    case B2000000:  return 2000000;
#endif
#if defined(B2500000)
	    case B2500000:  return 2500000;
#endif
#if defined(B3000000)
	    case B3000000:  return 3000000;
#endif
#if defined(B3500000)
	    case B3500000:  return 3500000;
#endif
#if defined(B4000000)
	    case B4000000:  return 4000000;
#endif
	    default:	    return 9600;
	}
}



/*
 *  Wait for a character for a maximum of wtime * 10 mSec.
 */
int mbse_Waitchar(unsigned char *ch, int wtime)
{
	int	i, rc = -1;

	for (i = 0; i < wtime; i++) {
		rc = read(ttyfd, ch, 1);
		if (rc == 1)
			return rc;
		msleep(10);
	}
	return rc;
}



int mbse_Escapechar(unsigned char *ch)
{
    int		    rc;
    unsigned char   c;
	
    /* 
     * Escape character, if nothing follows within 
     * 50 mSec, the user really pressed <esc>.
     */
    if ((rc = mbse_Waitchar(ch, 5)) == -1)
	return rc;

    if (*ch == '[') {
	/*
	 *  Start of CSI sequence. If nothing follows,
	 *  return immediatly.
	 */
	if ((rc = mbse_Waitchar(ch, 5)) == -1)
	    return rc;

	/*
	 *  Test for the most important keys. Note
	 *  that only the cursor movement keys are
	 *  guaranteed to work with PC-clients.
	 */
	c = *ch;
	if (c == 'A')
	    c = KEY_UP;
	if (c == 'B')
	    c = KEY_DOWN;
	if (c == 'C')
	    c = KEY_RIGHT;
	if (c == 'D')
	    c = KEY_LEFT;
	if ((c == '1') || (c == 'H') || (c == 0))
	    c = KEY_HOME;
	if ((c == '4') || (c == 'K') || (c == 101) || (c == 144))
	    c = KEY_END;
	if (c == '2')
	    c = KEY_INS;
	if (c == '3')
	    c = KEY_DEL;
	if (c == '5')
	    c = KEY_PGUP;
	if (c == '6')
	    c = KEY_PGDN;
	memcpy(ch, &c, sizeof(unsigned char));
	return rc;
    }
    return -1;
}



/*
 *  This next function will detect the grey keys on the keyboard for
 *  VT100, VT220, Xterm, PC-ANSI, and Linux console. Works with 
 *  several terminals on serial lines (tested 1200 bps).
 *  If for example cursur keys are detected, this function returns
 *  a translated value.
 */
unsigned char mbse_Readkey(void)
{
    unsigned char   ch = 0;
    int		    rc = -1;

    while (rc == -1) {
	rc = mbse_Waitchar(&ch, 5);

	/*
	 * If the character is not an Escape character,
	 * then this function is finished.
	 */
	if ((rc == 1) && (ch != KEY_ESCAPE))
	    return ch;

	if ((rc == 1) && (ch == KEY_ESCAPE)) {
	    rc = mbse_Escapechar(&ch);
	    if (rc == 1)
		return ch;
	    else
		return KEY_ESCAPE;
	}
    }

    return(ch);
}



