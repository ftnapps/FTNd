/*****************************************************************************
 *
 * File ..................: rawio.c
 * Purpose ...............: Raw I/O routines.
 * Last modification date : 07-Aug-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "libs.h"
#include "structs.h"
#include "common.h"


int rawset = FALSE;


/*
 * Sets raw mode and saves the terminal setup
 */
void Setraw()
{
	int	rc;

//	if (ioctl(ttyfd, TCGETA, &tbuf) == -1) {
//		perror("TCGETA Failed");
//		exit(1);  /* ERROR  - could not set get tty ioctl */
//	}
	if ((rc = tcgetattr(ttyfd, &tbufs))) {
		perror("");
		printf("$tcgetattr(0, save) return %d\n", rc);
		exit(1);
	}

	tbufsavs = tbufs;
	tbufs.c_iflag &= ~(INLCR | ICRNL | ISTRIP | IXON  ); /* IUCLC removed for FreeBSD */
        /*
         *  Map CRNL modes strip control characters and flow control
         */
	tbufs.c_oflag &= ~OPOST;		/* Don't do ouput character translation */
	tbufs.c_lflag &= ~(ICANON | ECHO);	/* No canonical input and no echo */
	tbufs.c_cc[VMIN] = 1;			/* Receive 1 character at a time */
	tbufs.c_cc[VTIME] = 0;			/* No time limit per character */

	if ((rc = tcsetattr(ttyfd, TCSADRAIN, &tbufs))) {
		perror("");
		printf("$tcsetattr(%d, TCSADRAIN, raw) return %d\n", ttyfd, rc);
		exit(1);
	}

//	if (ioctl(ttyfd, TCSETAF, &tbuf) == -1) {
//		perror("TCSETAF failed");
//		exit(1);  /* ERROR - could not set tty ioctl */
//	}

	rawset = TRUE;
}



/*
 * Unsets raw mode and returns state of terminal
 */
void Unsetraw()
{
	int	rc;

	/*
	 * Only unset the mode if it is set to raw mode
	 */
	if (rawset == TRUE) {
//		if (ioctl(ttyfd, TCSETAF, &tbufsav) == -1) {
//			perror("TCSETAF Normal Failed");
//			exit(1);  /* ERROR  - could not save original tty ioctl */
//		}
		if ((rc = tcsetattr(ttyfd, TCSAFLUSH, &tbufsavs))) {
			perror("");
			printf("$tcsetattr(%d, TCSAFLUSH, save) return %d\n", ttyfd, rc);
			exit(1);
		}
	}
	rawset = FALSE;
}



/* 
 * This function is used to get a single character from a user ie for a 
 * menu option
 */
unsigned char Getone()
{
	unsigned char	c = 0;

	if ((ttyfd = open ("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open 8");
		exit(1);
	}
	Setraw();

	c = Readkey();

	Unsetraw();
	close(ttyfd);
	return(c);
}



/*
 * Read the (locked) speed from the tty
 */
int Speed(void)
{
//	int		mspeed;
//	struct termio	ttyhold;
	speed_t		mspeed;

//	static int baud[16] = {0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400};

//	ioctl(0, TCGETA, &ttyhold);
//	mspeed = baud[ttyhold.c_cflag & 017];
//	ioctl(0, TCSETAF, &ttyhold);

	mspeed = cfgetospeed(&tbufs);

	return(mspeed);
}



/*
 *  Wait for a character for a maximum of wtime * 10 mSec.
 */
int Waitchar(unsigned char *ch, int wtime)
{
	int	i, rc = -1;

	for (i = 0; i < wtime; i++) {
		rc = read(ttyfd, ch, 1);
		if (rc == 1)
			return rc;
		usleep(10000);
	}
	return rc;
}



int Escapechar(unsigned char *ch)
{
	int		rc;
	unsigned char	c;
	
	/* 
	 * Escape character, if nothing follows within 
	 * 50 mSec, the user really pressed <esc>.
	 */
	if ((rc = Waitchar(ch, 5)) == -1)
		return rc;

	if (*ch == '[') {
		/*
		 *  Start of CSI sequence. If nothing follows,
		 *  return immediatly.
		 */
		if ((rc = Waitchar(ch, 5)) == -1)
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
unsigned char Readkey(void)
{
	unsigned char	ch = 0;
	int		rc = -1;

	while (rc == -1) {
		rc = Waitchar(&ch, 5);

		/*
		 * If the character is not an Escape character,
		 * then this function is finished.
		 */
		if ((rc == 1) && (ch != KEY_ESCAPE))
			return ch;

		if ((rc == 1) && (ch == KEY_ESCAPE)) {
			rc = Escapechar(&ch);
			if (rc == 1)
				return ch;
			else
				return KEY_ESCAPE;
		}
	}

	return(ch);
}



