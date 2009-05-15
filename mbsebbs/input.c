/*****************************************************************************
 *
 * $Id: input.c,v 1.29 2007/08/25 18:32:08 mbse Exp $
 * Purpose ...............: Input functions, also for some utils.
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "input.h"
#include "timeout.h"
#include "language.h"
#include "term.h"
#include "ttyio.h"



extern int  cols;
extern int  rows;



void CheckScreen(void)
{
    struct winsize  ws;

    if (ioctl(1, TIOCGWINSZ, &ws) != -1 && (ws.ws_col > 0) && (ws.ws_row > 0)) {
	if (ws.ws_col != 80)
	    ws.ws_col = 80;
	if (ws.ws_row < 24)
	    ws.ws_row = 24;
	if ((ws.ws_col != cols) || (ws.ws_row != rows)) {
	    cols = ws.ws_col;
	    rows = ws.ws_row;
	    Syslog('+', "User screensize changed to %dx%d", cols, rows);
	}
    }
}



/*
 *  Wait for a character for a maximum of wtime * 10 mSec.
 */
int Waitchar(unsigned char *ch, int wtime)
{
    int	    i, rc = TIMEOUT;

    for (i = 0; i < wtime; i++) {
	CheckScreen();
	rc = GETCHAR(0);
	if (tty_status == STAT_SUCCESS) {
	    *ch = (unsigned char)rc;
	    return 1;
	}
	if (tty_status != STAT_TIMEOUT) {
	    return rc;
	}
	msleep(10);
    }
    return rc;
}



int Escapechar(unsigned char *ch)
{
    int             rc;
    unsigned char   c;
	            
    /* 
     * Escape character, if nothing follows within 
     * 50 mSec, the user really pressed <esc>.
     */
    if ((rc = Waitchar(ch, 5)) == TIMEOUT) {
	return rc;
    }

    if (*ch == '[') {
	/*
         *  Start of CSI sequence. If nothing follows,
         *  return immediatly.
         */
	if ((rc = Waitchar(ch, 5)) == TIMEOUT) {
	    return rc;
	}

        /*
         *  Test for the most important keys. Note
         *  that only the cursor movement keys are
         *  guaranteed to work with PC-clients.
         */
        c = *ch;
        if (c == 'A')
	    c = KEY_UP;
	else if (c == 'B')
	    c = KEY_DOWN;
	else if (c == 'C')
	    c = KEY_RIGHT;
	else if (c == 'D')
	    c = KEY_LEFT;
	else if ((c == 'H') || (c == 0))
	    c = KEY_HOME;
	else if ((c == '4') || (c == 'K') || (c == 'F') || (c == 101) || (c == 144))
	    c = KEY_END;
	else if ((c == '2') || (c == 'L')) {
	    Waitchar(ch, 5);	/* Eat following ~ char	*/
	    c = KEY_INS;
	} else if (c == '3') {
	    Waitchar(ch, 5);    /* Eat following ~ char */
	    c = KEY_DEL;
	} else if ((c == '5') || (c == 'I')) {
	    Waitchar(ch, 5);    /* Eat following ~ char */
	    c = KEY_PGUP;
	} else if ((c == '6') || (c == 'G')) {
	    Waitchar(ch, 5);    /* Eat following ~ char */
	    c = KEY_PGDN;
	} else if (c == '1') {
	    if ((rc = Waitchar(ch, 5)) == TIMEOUT) {
		c = KEY_HOME;
	    } else {
		c = *ch;
		Waitchar(ch, 5);    /* Eat following ~ char */
		switch (c) {
		    case '1'	: c = KEY_F1;	break;
		    case '2'	: c = KEY_F2;	break;
		    case '3'	: c = KEY_F3;	break;
		    case '4'	: c = KEY_F4;	break;
		    case '5'	: c = KEY_F5;	break;
		    case '7'	: c = KEY_F6;	break;
		    case '8'	: c = KEY_F7;	break;
		    case '9'	: c = KEY_F8;	break;
		}
	    }
	}
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
    unsigned char   ch = 0;
    int             rc = TIMEOUT;

    while (rc == TIMEOUT) {
	rc = Waitchar(&ch, 5);

	/*
         * If the character is not an Escape character,
         * then this function is finished.
         */
        if ((rc == 1) && (ch != KEY_ESCAPE)) {
	    return ch;
	}

	if ((rc == 1) && (ch == KEY_ESCAPE)) {
	    rc = Escapechar(&ch);
	    if (rc == 1) {
		return ch;
	    } else {
		return KEY_ESCAPE;
	    }
	}
    }
    return rc;
}



/*
 * Read the (locked) speed from the tty
 */
int Speed(void)
{
    speed_t	mspeed;

    mspeed = cfgetospeed(&tbufs);
#ifdef CBAUD
    switch (mspeed & CBAUD) {
#else
    switch (mspeed) {
#endif
	case B0:        return 0;
#if defined(B50)
	case B50:       return 50;
#endif
#if defined(B75)
	case B75:       return 75;
#endif
#if defined(B110)
	case B110:      return 110;
#endif
#if defined(B134)
	case B134:      return 134;
#endif
#if defined(B150)
	case B150:      return 150;
#endif
#if defined(B200)
	case B200:      return 200;
#endif
#if defined(B300)
	case B300:      return 300;
#endif
#if defined(B600)
	case B600:      return 600;
#endif
#if defined(B1200)
	case B1200:     return 1200;
#endif
#if defined(B1800)
	case B1800:     return 1800;
#endif
#if defined(B2400)
	case B2400:     return 2400;
#endif
#if defined(B4800)
	case B4800:     return 4800;
#endif
#if defined(B9600)
	case B9600:     return 9600;
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
	default:        return 9600;
    }
}



int traduce(char *ch)
{
   int i;
   
   for (i = 0; i < 81; i++){
       if ( Language(35)[i] == '\0' ) break;
       if ( *ch == Language(35)[i] ){
          if ( Language(36)[i] != '\0'){
               *ch = ( Language(36)[i] ); 
           }                   
       return TRUE; 
       }
   }
   for (i = 0; i < 81; i++){
       if ( Language(33)[i] == '\0' ) break;
       if ( *ch == Language(33)[i] ){
          if ( Language(34)[i] != '\0'){
               *ch = ( Language(34)[i] ); 
           }                   
       return TRUE; 
       }
   } 
  
   return FALSE;
}



void BackErase(void)
{
    PUTCHAR('\b');
    PUTCHAR(' ');
    PUTCHAR('\b');
}



/*
 * Get a character string with cursor position
 */
void GetstrP(char *sStr, int iMaxLen, int Position)
{
    unsigned char   ch = 0;
    int		    iPos = Position;

    FLUSHIN();
    alarm_on();

    while (ch != KEY_ENTER) {

	ch = Readkey();
	if ((ch == KEY_BACKSPACE) || (ch == KEY_DEL) || (ch == KEY_RUBOUT)) {
	    if (iPos > 0) {
		BackErase();
		sStr[--iPos] = '\0';
	    } else
		PUTCHAR('\007');

	    /* if 13 < DEL < 127 , should not output again */
	} else if ((ch > 31 && ch < 127) || traduce((char *)&ch)) {
	    if (iPos <= iMaxLen) {
		iPos++;
		snprintf(sStr + strlen(sStr), 5, "%c", ch);
		PUTCHAR(ch);
	    } else {
		PUTCHAR('\007');
	    }
	}
    }

    PUTCHAR('\r');
    PUTCHAR('\n');
}



/*
 * Get a character string
 */
void GetstrC(char *sStr, int iMaxlen)
{
    unsigned char   ch = 0;
    int		    iPos = 0;

    FLUSHIN();
    strcpy(sStr, "");
    alarm_on();

    while (ch != 13) {

	ch = Readkey();

	if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
	    if (iPos > 0) {
		BackErase();
		sStr[--iPos] = '\0';
	    } else
		PUTCHAR('\007');
	}

	if ((ch > 31) && (ch < 127)) {
	    if (iPos <= iMaxlen) {
		iPos++;
		snprintf(sStr + strlen(sStr), 5, "%c", ch);
		PUTCHAR(ch);
	    } else
		PUTCHAR('\007');
	}
    }

    PUTCHAR('\r');
    PUTCHAR('\n');
}



/*
 *  get a string, don't allow spaces (for Unix accounts)
 */
void GetstrU(char *sStr, int iMaxlen)
{
    unsigned char   ch = 0;
    int		    iPos = 0;

    FLUSHIN();
    strcpy(sStr, "");
    alarm_on();

    while (ch != 13) {

	ch = Readkey();

	if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
	    if (iPos > 0) {
		BackErase();
		sStr[--iPos] = '\0';
	    } else
		PUTCHAR('\007');
	}

	if (isalnum(ch) || (ch == '@') || (ch == '.') || (ch == '-') || (ch == '_')) {
	    if (iPos <= iMaxlen) {
		iPos++;
		snprintf(sStr + strlen(sStr), 5, "%c", ch);
		PUTCHAR(ch);
	    } else
		PUTCHAR('\007');
	}
    }

    PUTCHAR('\r');
    PUTCHAR('\n');
}



/*
 * Get a phone number, only allow digits, + and - characters.
 */
void GetPhone(char *sStr, int iMaxlen)
{
    unsigned char   ch = 0; 
    int		    iPos = 0;

    FLUSHIN();
    
    strcpy(sStr, "");
    alarm_on();

    while (ch != 13) {

	ch = Readkey();

	if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) { 
	    if (iPos > 0) {
		BackErase();
		sStr[--iPos]='\0';
	    } else
		PUTCHAR('\007');
	}

	if ((ch >= '0' && ch <= '9') || (ch == '-') || (ch == '+')) {
	    if (iPos <= iMaxlen) {
		iPos++;
		snprintf(sStr + strlen(sStr), 5, "%c", ch);
		PUTCHAR(ch);
	    } else 
		PUTCHAR('\007');
	}
    }

    PUTCHAR('\r');
    PUTCHAR('\n');
}



/*
 * Get a number, allow digits, spaces, minus sign, points and comma's
 */
void Getnum(char *sStr, int iMaxlen)
{
    unsigned char   ch = 0; 
    int		    iPos = 0;

    FLUSHIN();

    strcpy(sStr, "");
    alarm_on();

    while (ch != 13) {

	ch = Readkey();

	if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
	    if (iPos > 0) {
		BackErase();
		sStr[--iPos]='\0';
	    } else
		PUTCHAR('\007');
	}

	if ((ch >= '0' && ch <= '9') || (ch == '-') || (ch == ' ') || (ch == ',') || (ch == '.')) {

	    if (iPos <= iMaxlen) {
		iPos++;
		snprintf(sStr + strlen(sStr), 5, "%c", ch);
		PUTCHAR(ch);
	    } else
		PUTCHAR('\007');
	}
    }

    PUTCHAR('\r');
    PUTCHAR('\n');
}



/*
 * This function gets the date from the user checking the length and
 * putting two minus signs in the right places
 */
void GetDate(char *sStr, int iMaxlen)
{
    unsigned char   ch = 0; 
    int		    iPos = 0;

    FLUSHIN();
    strcpy(sStr, "");

    alarm_on();
    while (ch != 13) {

	ch = Readkey();

	if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
	    if (iPos > 0)
		BackErase();
	    else
		PUTCHAR('\007');

	    if (iPos == 3 || iPos == 6) {
		BackErase();
		--iPos;
	    }

	    sStr[--iPos]='\0';
	}

	if (ch >= '0' && ch <= '9') {
	    if (iPos < iMaxlen) {
		iPos++;
		snprintf(sStr + strlen(sStr), 5, "%c", ch);
		PUTCHAR(ch);
		if (iPos == 2 || iPos == 5) {
		    PUTCHAR('-');
		    snprintf(sStr + strlen(sStr), 2, "-");
		    iPos++;
		}
	    } else
		PUTCHAR('\007');
	}
    }

    PUTCHAR('\r');
    PUTCHAR('\n');
}



/*
 * Get a string, capitalize only if set in config.
 */
void Getname(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0; 
	int		iPos = 0, iNewPos = 0;

	fflush(stdout);

	strcpy(sStr, "");

	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos]='\0';
			} else
				putchar('\007');
		}

		if (ch > 31 && (ch < 127)) {
			if (iPos < iMaxlen) {
				iPos++;
				if (iPos == 1 && CFG.iCapUserName)
					ch = toupper(ch);

				if (ch == 32) {
					iNewPos = iPos;
					iNewPos++;
				}

				if (iNewPos == iPos && CFG.iCapUserName)
					ch = toupper(ch);
				else
					if (CFG.iCapUserName)
						ch = tolower(ch);

				if (iPos == 1 && CFG.iCapUserName)
					ch = toupper(ch);

				snprintf(sStr + strlen(sStr), 5, "%c", ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	printf("\n");
}



/*
 * Get a Fidonet style username, always capitalize.
 * Also used for Location Names.
 */
void GetnameNE(char *sStr, int iMaxlen)         
{                                              
    unsigned char   ch = 0; 
    int		    iPos = 0, iNewPos = 0;

    fflush(stdout);

    strcpy(sStr, "");

    alarm_on();

    while (ch != 13) {

	fflush(stdout);
	ch = Readkey();

	if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
	    if (iPos > 0) {
		printf("\b \b");
		sStr[--iPos]='\0';
	    } else 
		putchar('\007');
	}

	if ((ch > 31) && (ch < 127)) {
	    if (iPos < iMaxlen) {
		iPos++;

		if (iPos == 1)
		    ch = toupper(ch);

		if (ch == 32) {
		    iNewPos = iPos;
		    iNewPos++;
		}

		if (iNewPos == iPos)
		    ch = toupper(ch);
		else
		    ch = tolower(ch);

		if (iPos == 1)
		    ch = toupper(ch);

		snprintf(sStr + strlen(sStr), 5, "%c", ch);
		printf("%c", ch);
	    } else
		putchar('\007');
	}
    }

    printf("\n");
}



/*
 * Open up /dev/tty to get the password from the user 
 * because this is done in raw mode, it makes life a bit
 * more difficult. 
 * This function gets a password from a user, upto Max_passlen
 */
void Getpass(char *theword)
{
        unsigned char   c = 0;
        int             counter = 0;
        char            password[Max_passlen+1];

        alarm_on();

        /* 
         * Till the user presses ENTER or reaches the maximum length allowed
         */
        while ((c != 13) && (counter < Max_passlen )) {

                fflush(stdout);
                c = Readkey();  /* Reads a character from the raw device */

                if (((c == 8) || (c == KEY_DEL) || (c == 127)) && (counter != 0 )) { /* If its a BACKSPACE */
                        counter--;
                        password[counter] = '\0';
                        printf("\x008 \x008");
                        continue;
                }  /* Backtrack to fix the BACKSPACE */

                if (((c == 8) || (c == KEY_DEL) || (c == 127)) && (counter == 0) ) {
                        printf("\x007");
                        continue;
                } /* Don't Backtrack as we are at the begining of the passwd field */

                if (isalnum(c)) {
                        password[counter] = c;
                        counter++;
                        printf("%c", CFG.iPasswd_Char);
                }
        }

        password[counter] = '\0';  /* Make sure the string has a NULL at the end*/
        strcpy(theword,password);
}



void Pause()
{
    int	    i, x;
    char    *string;

    string = malloc(81);

    /* Press (Enter) to continue: */
    snprintf(string, 81, "\r%s", (char *) Language(375));
    colour(CFG.CRColourF, CFG.CRColourB);
    PUTSTR(string);
    
    do {
	alarm_on();
	i = Readkey();
    } while ((i != '\r') && (i != '\n'));

    x = strlen(string);
    for(i = 0; i < x; i++)
	PUTCHAR('\b');
    for(i = 0; i < x; i++)
	PUTCHAR(' ');
    for(i = 0; i < x; i++)
	PUTCHAR('\b');

    free(string);
}


