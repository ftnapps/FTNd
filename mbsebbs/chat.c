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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/ansi.h"
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


void Showline(int, int, char *);
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
 * Colorize the chat window
 */
void Showline(int y, int x, char *msg)
{
    int	    i;

    if (strlen(msg)) {
	if (msg[0] == '<') {
	    locate(y, x);
	    colour(LIGHTCYAN, BLACK);
	    putchar('<');
	    colour(LIGHTBLUE, BLACK);
	    for (i = 1; i < strlen(msg); i++) {
		if (msg[i] == '>') {
		    colour(LIGHTCYAN, BLACK);
		    putchar(msg[i]);
		    colour(CYAN, BLACK);
		} else {
		    putchar(msg[i]);
		}
	    }
	} else if (msg[0] == '*') {
	    colour(LIGHTRED, BLACK);
	    mvprintw(y, x, msg);
	} else {
	    colour(GREEN, BLACK);
	    mvprintw(y, x, msg);
	}
    }
}



/*
 * Display received chat message in the chat window.
 */
void DispMsg(char *msg)
{
    int     i;

    strncpy(rbuf[rpointer], msg, 80);
    Showline(2 + rpointer, 1, rbuf[rpointer]);
    if (rpointer == rsize) {
	/*
	 * Scroll buffer
	 */
	for (i = 0; i <= rsize; i++) {
	    locate(i + 2, 1);
	    clrtoeol();
	    sprintf(rbuf[i], "%s", rbuf[i+1]);
	    Showline(i + 2, 1, rbuf[i]);
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
    int		    curpos = 0, stop = FALSE, data, rc;
    unsigned char   ch;
    char	    sbuf[81], resp[128], *cnt, *msg;
    static char	    buf[200];

    WhosDoingWhat(SYSOPCHAT, NULL);
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

    sprintf(buf, "CCON,3,%d,%s,0;", mypid, exitinfo.Name);
    Syslog('-', "> %s", buf);
    if (socket_send(buf) == 0) {
	strncpy(buf, socket_receive(), sizeof(buf)-1);
	Syslog('-', "< %s", buf);
	if (strncmp(buf, "100:1,", 6) == 0) {
	    cnt = strtok(buf, ",");
	    msg = strtok(NULL, "\0");
	    msg[strlen(msg)-1] = '\0';
	    colour(LIGHTRED, BLACK);
	    mvprintw(4, 1, msg);
	    sleep(2);
	    Pause();
	    chat_with_sysop = FALSE;
	    return;
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

    /*
     * If username and channelname are given, send the /nick and /join
     * commands to the chatserver.
     */
    if (username && channel) {
	sprintf(buf, "CPUT:2,%d,/nick %s;", mypid, username);
	if (socket_send(buf) == 0)
	    strcpy(buf, socket_receive());
	sprintf(buf, "CPUT:2,%d,/join %s;", mypid, channel);
	if (socket_send(buf) == 0)
	    strcpy(buf, socket_receive());
    }

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
		strncpy(buf, socket_receive(), sizeof(buf)-1);
		if (strncmp(buf, "100:2,", 6) == 0) {
		    Syslog('-', "> CGET:1,%d;", mypid);
		    Syslog('-', "< %s", buf);
		    strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100        */
		    strncpy(resp, strtok(NULL, ","), 5);    /* Should be 2          */
		    strncpy(resp, strtok(NULL, ","), 5);    /* 1= fatal, chat ended */
		    rc = atoi(resp);
		    memset(&resp, 0, sizeof(resp));
		    strncpy(resp, strtok(NULL, "\0"), 80);  /* The message          */
		    resp[strlen(resp)-1] = '\0';
		    DispMsg(resp);
		    if (rc == 1) {
			Syslog('+', "Chat server error: %s", resp);
			stop = TRUE;
			data = FALSE;
		    }
		} else {
		    data = FALSE;
		}
	    }
	}

	if (stop)
	    break;

        /*
	 * Check for a pressed key, if so then process it
	 */
	ch = testkey(exitinfo.iScreenLen -1, curpos + 2);
	if (isprint(ch)) {
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
	    sprintf(buf, "CPUT:2,%d,%s;", mypid, sbuf);
	    Syslog('-', "> %s", buf);
	    if (socket_send(buf) == 0) {
		strcpy(buf, socket_receive());
		Syslog('-', "< %s", buf);
		if (strncmp(buf, "100:2,", 6) == 0) {
		    strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100            */
		    strncpy(resp, strtok(NULL, ","), 5);    /* Should be 2              */
		    strncpy(resp, strtok(NULL, ","), 5);    /* 1= fatal, chat ended	*/
		    rc = atoi(resp);
		    strncpy(resp, strtok(NULL, "\0"), 80);  /* The message              */
		    resp[strlen(resp)-1] = '\0';
		    DispMsg(resp);
		    if (rc == 1) {
			Syslog('+', "Chat server error: %s", resp);
			stop = TRUE;
		    }
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
	    strncpy(buf, socket_receive(), sizeof(buf)-1);
	    if (strncmp(buf, "100:2,", 6) == 0) {
		Syslog('-', "> CGET:1,%d;", mypid);
		Syslog('-', "< %s", buf);
		strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100        */
		strncpy(resp, strtok(NULL, ","), 5);    /* Should be 2          */
		strncpy(resp, strtok(NULL, ","), 5);	/* 1= fatal error	*/
		rc = atoi(resp);
		memset(&resp, 0, sizeof(resp));
		strncpy(resp, strtok(NULL, "\0"), 80);  /* The message          */
		resp[strlen(resp)-1] = '\0';
		DispMsg(resp);
		if (rc == 1) {
		    Syslog('+', "Chat server error: %s", resp);
		    data = FALSE;   /* Even if there is more, prevent a loop */
		}
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
    sprintf(buf, "CCLO,1,%d;", mypid);
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


