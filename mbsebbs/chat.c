/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Sysop to user chat utility
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/ansi.h"
#include "../lib/memwatch.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/mberrors.h"
#include "chat.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "misc.h"
#include "whoson.h"


int		chat_with_sysop = FALSE;    /* Global sysop chat flag	*/
int		chatting = FALSE;	    /* Global chatting flag	*/
char		rbuf[50][80];		    /* Chat receive buffer	*/ /* FIXME: must be a dynamic buffer */
int		rpointer = 0;		    /* Chat receive pointer	*/
int		rsize = 5;		    /* Chat receive size	*/
extern pid_t	mypid;


void DispMsg(char *);
void clrtoeol(void);
unsigned char testkey(int, int);



unsigned char testkey(int y, int x)
{
    int             rc;
    unsigned char   ch = 0;

    Nopper();
    locate(y, x);
    fflush(stdout);

    if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
	perror("open /dev/tty");
	exit(MBERR_TTYIO_ERROR);
    }
    Setraw();

    rc = Waitchar(&ch, 50);
    if (rc == 1) {
	if (ch == KEY_ESCAPE)
	rc = Escapechar(&ch);
    }
    Unsetraw();
    close(ttyfd);
    if (rc == 1)
	return ch;
    else
	return '\0';
}



/*
 * Display received chat message in the chat window.
 */
void DispMsg(char *msg)
{
    int     i;

    strncpy(rbuf[rpointer], msg, 80);
    mvprintw(2 + rpointer, 1, rbuf[rpointer]);
    if (rpointer == rsize) {
	/*
	 * Scroll buffer
	 */
	for (i = 0; i <= rsize; i++) {
	    locate(i + 2, 1);
	    clrtoeol();
	    sprintf(rbuf[i], "%s", rbuf[i+1]);
	    mvprintw(i + 2, 1, rbuf[i]);
	}
    } else {
	rpointer++;
    }
    fflush(stdout);
}



/*
 * Clear to End Of Line
 */
void clrtoeol(void)
{
    fprintf(stdout, ANSI_CLREOL);
}



/*
 * Chat, if the parameters are not NULL, a connection with the named
 * channel is made with the give username which will be forced to the
 * used nick name. This mode is used for forced sysop chat.
 * If the parameters are NULL, then it's up to the user what happens.
 */
void Chat(char *username, char *channel)
{
    int		    curpos = 0, stop = FALSE, data;
    unsigned char   ch;
    char	    sbuf[81], resp[128], *cnt, *msg;
    static char	    buf[200];

    WhosDoingWhat(SYSOPCHAT);
    clear();

    rsize = exitinfo.iScreenLen - 5;
    rpointer = 0;
    
    if (username && channel) {
	colour(LIGHTGREEN, BLACK);
	/* *** Sysop is starting chat *** */
	printf("\007%s\n\r", (char *) Language(59));
	fflush(stdout);
	sleep(1);
	printf("\007");
	fflush(stdout);
	sleep(1);
	printf("\007");
	fflush(stdout);
	Syslog('+', "Sysop chat started");
	chat_with_sysop = TRUE;
    } else {
	Syslog('+', "User started chatting");
    }
    
    /*
     * Setup the screen, this is only possible in ANSI mode.
     */
    clear();
    locate(1, 1);
    colour(WHITE, BLUE);
    clrtoeol();
    mvprintw(1, 2, "MBSE BBS Chat Server");

    sprintf(buf, "CCON,2,%d,%s", mypid, CFG.sysop);
    Syslog('-', "> %s", buf);
    if (socket_send(buf) == 0) {
	strcpy(buf, socket_receive());
	Syslog('-', "< %s", buf);
	if (strncmp(buf, "100:1,", 6) == 0) {
	    cnt = strtok(buf, ",");
	    msg = strtok(NULL, "\0");
	    msg[strlen(msg)-1] = '\0';
	    colour(LIGHTRED, BLACK);
	    mvprintw(4, 1, msg);
	    sleep(2);
	    Pause();
	    return;
	    chat_with_sysop = FALSE;
	}
    }

    locate(exitinfo.iScreenLen - 2, 1);
    colour(WHITE, BLUE);
    clrtoeol();
    mvprintw(exitinfo.iScreenLen - 2, 2, "Chat, type \"/EXIT\" to exit");

    colour(LIGHTGRAY, BLACK);
    mvprintw(exitinfo.iScreenLen - 1, 1, ">");
    memset(&sbuf, 0, sizeof(sbuf));
    memset(&rbuf, 0, sizeof(rbuf));

    Syslog('-', "Start loop");
    chatting = TRUE;

    while (stop == FALSE) {

	/*
	 * Check for new message, loop fast until no more data available.
	 */
	data = TRUE;
	while (data) {
	    sprintf(buf, "CGET:1,%d;", mypid);
	    if (socket_send(buf) == 0) {
		strcpy(buf, socket_receive());
		if (strncmp(buf, "100:1,", 6) == 0) {
		    Syslog('-', "> CGET:1,%d;", mypid);
		    Syslog('-', "< %s", buf);
		    strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100        */
		    strncpy(resp, strtok(NULL, ","), 5);    /* Should be 1          */
		    strncpy(resp, strtok(NULL, "\0"), 80);  /* The message          */
		    resp[strlen(resp)-1] = '\0';
		    DispMsg(resp);
		} else {
		    data = FALSE;
		}
	    }
	}

        /*
	 * Check for a pressed key, if so then process it
	 */
	ch = testkey(exitinfo.iScreenLen -1, curpos + 2);
	if (ch == '@') {
	    break;
	} else if (isprint(ch)) {
	    if (curpos < 77) {
		putchar(ch);
		fflush(stdout);
		sbuf[curpos] = ch;
		curpos++;
	    } else {
		putchar(7);
		fflush(stdout);
	    }
	} else if ((ch == KEY_BACKSPACE) || (ch == KEY_RUBOUT) || (ch == KEY_DEL)) {
	    if (curpos) {
		curpos--;
		sbuf[curpos] = '\0';
		printf("\b \b");
	    } else {
		putchar(7);
	    }
	} else if ((ch == '\r') && curpos) {
	    if (strncasecmp(sbuf, "/exit", 5) == 0)
		stop = TRUE;
	    sprintf(buf, "CPUT:2,%d,%s;", mypid, sbuf);
	    Syslog('-', "> %s", buf);
	    if (socket_send(buf) == 0) {
		strcpy(buf, socket_receive());
		Syslog('-', "< %s", buf);
		if (strncmp(buf, "100:1,", 6) == 0) {
		    strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100            */
		    strncpy(resp, strtok(NULL, ","), 5);    /* Should be 1              */
		    strncpy(resp, strtok(NULL, "\0"), 80);  /* The message              */
		    resp[strlen(resp)-1] = '\0';
		    DispMsg(resp);
		}
	    }
	    curpos = 0;
	    memset(&sbuf, 0, sizeof(sbuf));
	    locate(exitinfo.iScreenLen - 1, 2);
	    clrtoeol();
	    mvprintw(exitinfo.iScreenLen - 1, 1, ">");
	}
    }
    chatting = FALSE;


    /* 
     * Before sending the close command, purge all outstanding messages.
     */
    data = TRUE;
    while (data) {
	sprintf(buf, "CGET:1,%d;", mypid);
	if (socket_send(buf) == 0) {
	    strcpy(buf, socket_receive());
	    if (strncmp(buf, "100:1,", 6) == 0) {
		Syslog('-', "> CGET:1,%d;", mypid);
		Syslog('-', "< %s", buf);
		strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100        */
		strncpy(resp, strtok(NULL, ","), 5);    /* Should be 1          */
		strncpy(resp, strtok(NULL, "\0"), 80);  /* The message          */
		resp[strlen(resp)-1] = '\0';
		DispMsg(resp);
	    } else {
		data = FALSE;
	    }
	}
    }

    if (username && channel) {
	/*
	 * Disjoin sysop channel
	 */

	/* *** Sysop has terminated chat *** */
	sprintf(buf, "%s", (char *) Language(60));
	DispMsg(buf);
	Syslog('+', "Sysop chat ended");
	chat_with_sysop = FALSE;
    } else {
	Syslog('+', "User chat ended");
    }

    /*
     * Close server connection
     */
    sprintf(buf, "CCLO,1,%d", mypid);
    Syslog('-', "> %s", buf);
    if (socket_send(buf) == 0) {
	strcpy(buf, socket_receive());
	Syslog('-', "< %s", buf);
	if (strncmp(buf, "100:1,", 6)) {
	}
    }
    sleep(2);
    clear();
}


