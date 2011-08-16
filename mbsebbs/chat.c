/*****************************************************************************
 *
 * $Id: chat.c,v 1.32 2008/02/12 19:59:45 mbse Exp $
 * Purpose ...............: Sysop to user chat utility
 *
 *****************************************************************************
 * Copyright (C) 1997-2008
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
#include "chat.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "misc.h"
#include "whoson.h"
#include "term.h"
#include "ttyio.h"
#include "timeout.h"


#define	RBUFLEN	    81

int			chat_with_sysop = FALSE;    /* Global sysop chat flag	*/
int			chatting = FALSE;	    /* Global chatting flag	*/
char			rbuf[50][RBUFLEN];	    /* Chat receive buffer	*/ /* FIXME: must be a dynamic buffer */
int			rpointer = 0;		    /* Chat receive pointer	*/
int			rsize = 5;		    /* Chat receive size	*/
extern pid_t		mypid;
extern int		cols;
extern int		rows;
extern unsigned int	mib_chats;
extern unsigned int	mib_chatminutes;



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

    rc = Waitchar(&ch, 50);
    if (rc == 1) {
	if (ch == KEY_ESCAPE)
	rc = Escapechar(&ch);
    }
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
    int	    i, done = FALSE;

    if (strlen(msg)) {
	if (msg[0] == '<') {
	    locate(y, x);
	    colour(LIGHTCYAN, BLACK);
	    PUTCHAR('<');
	    colour(LIGHTBLUE, BLACK);
	    for (i = 1; i < strlen(msg); i++) {
		if ((msg[i] == '>') && (! done)) {
		    colour(LIGHTCYAN, BLACK);
		    PUTCHAR(msg[i]);
		    colour(CYAN, BLACK);
		    done = TRUE;
		} else {
		    PUTCHAR(msg[i]);
		}
	    }
	} else if (msg[0] == '*') {
	    if (msg[1] == '*') {
		if (msg[2] == '*')
		    mbse_colour(YELLOW, BLACK);
		else
		    colour(LIGHTRED, BLACK);
	    } else {
		colour(LIGHTMAGENTA, BLACK);
	    }
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

    /*
     * Beep on minor system messages
     */
    if ((msg[0] == '*') && (msg[1] != '*'))
	putchar('\007');
    
    strncpy(rbuf[rpointer], msg, RBUFLEN);
    Showline(2 + rpointer, 1, rbuf[rpointer]);
    if (rpointer == rsize) {
	/*
	 * Scroll buffer
	 */
	for (i = 0; i <= rsize; i++) {
	    locate(i + 2, 1);
	    clrtoeol();
	    snprintf(rbuf[i], RBUFLEN, "%s", rbuf[i+1]);
	    Showline(i + 2, 1, rbuf[i]);
	}
    } else {
	rpointer++;
    }
}



/*
 * Clear to End Of Line
 */
void clrtoeol(void)
{
    PUTSTR((char *)ANSI_CLREOL);
}



/*
 * Chat, if the parameters are not NULL, a connection with the named
 * channel is made with the give username which will be forced to the
 * used nick name. This mode is used for forced sysop chat.
 * If the parameters are NULL, then it's up to the user what happens.
 */
void Chat(char *username, char *channel)
{
    int		    width, curpos = 0, stop = FALSE, data, rc;
    unsigned char   ch;
    char	    sbuf[81], resp[128], *name, *mname;
    static char	    buf[200];
    time_t	    c_start, c_end;


    WhosDoingWhat(SYSOPCHAT, NULL);
    clear();

    rsize = rows - 5;
    rpointer = 0;

    if (SYSOP == TRUE) {
	/*
	 * Forbid the sysop to chat, the sysop MUST use mbmon.
	 */
	Syslog('+', "The Sysop attempted to chat");
	pout(LIGHTRED, BLACK, (char *) Language(29));
	Enter(1);
	Pause();
	return;
    }
   
    if (username && channel) {
	colour(LIGHTGREEN, BLACK);
	PUTCHAR('\007');
	/* *** Sysop is starting chat *** */
	pout(LIGHTGREEN, BLACK, (char *) Language(59));
	Enter(1);
	sleep(1);
	PUTCHAR('\007');
	sleep(1);
	PUTCHAR('\007');
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
    snprintf(buf, 200, "%-*s", cols, " MBSE BBS Chat Server");
    mvprintw(1, 1, buf);

    mname = xstrcpy(clencode(exitinfo.sUserName));
    name  = xstrcpy(clencode(exitinfo.Name));
    width = cols - (strlen(name) + 3);
    snprintf(buf, 200, "CCON,4,%d,%s,%s,0;", mypid, mname, name);
    free(mname);
    free(name);
    if (socket_send(buf) == 0) {
	strncpy(buf, socket_receive(), sizeof(buf)-1);
	if (strncmp(buf, "200:1,", 6) == 0) {
	    Syslog('!', "Chatsever is not available");
	    colour(LIGHTRED, BLACK);
	    mvprintw(4, 1, (char *) Language(30));
	    Enter(2);
	    Pause();
	    chat_with_sysop = FALSE;
	    return;
	}
    }

    locate(rows - 2, 1);
    colour(WHITE, BLUE);
    snprintf(buf, 200, "%-*s", cols, " Chat, type \"/EXIT\" to exit or \"/HELP\" for help");
    mvprintw(rows - 2, 1, buf);

    colour(WHITE, BLACK);
    mvprintw(rows - 1, 1, ">");
    mvprintw(rows - 1, width + 2, "<");
    memset(&sbuf, 0, sizeof(sbuf));
    memset(&rbuf, 0, sizeof(rbuf));
    colour(LIGHTGRAY, BLACK);

    /*
     * If username and channelname are given, send the /nick and /join
     * commands to the chatserver.
     */
    if (username && channel) {
	snprintf(buf, 200, "CPUT:2,%d,/nick %s;", mypid, clencode(username));
	if (socket_send(buf) == 0)
	    strcpy(buf, socket_receive());
	snprintf(buf, 200, "CPUT:2,%d,/join %s;", mypid, clencode(channel));
	if (socket_send(buf) == 0)
	    strcpy(buf, socket_receive());
    }

    chatting = TRUE;
    c_start = time(NULL);

    while (stop == FALSE) {

	/*
	 * Check for new message, loop fast until no more data available.
	 */
	data = TRUE;
	while (data) {
	    snprintf(buf, 200, "CGET:1,%d;", mypid);
	    if (socket_send(buf) == 0) {
		strncpy(buf, socket_receive(), sizeof(buf)-1);
		if (strncmp(buf, "100:2,", 6) == 0) {
		    strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100        */
		    strncpy(resp, strtok(NULL, ","), 5);    /* Should be 2          */
		    strncpy(resp, strtok(NULL, ","), 5);    /* 1= fatal, chat ended */
		    rc = atoi(resp);
		    memset(&resp, 0, sizeof(resp));
		    strncpy(resp, cldecode(strtok(NULL, ";")), 80);  /* The message          */
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
	 * Check for a pressed key, if so then process it.
	 * Allow hi-ascii for multi-language.
	 */
	ch = testkey(rows -1, curpos + 2);
	if ((ch == KEY_BACKSPACE) || (ch == KEY_RUBOUT) || (ch == KEY_DEL)) {
	    alarm_on();
	    if (curpos) {
		curpos--;
		sbuf[curpos] = '\0';
		BackErase();
	    } else {
		PUTCHAR(7);
	    }
	/* if KEY_DEL isprint, do no output again */
	} else if (isprint(ch) || traduce((char *)&ch)) {
	    alarm_on();
	    if (curpos < width) {
		PUTCHAR(ch);
		sbuf[curpos] = ch;
		curpos++;
	    } else {
		PUTCHAR(7);
	    }
	} else if ((ch == '\r') && curpos) {
	    alarm_on();
	    snprintf(buf, 200, "CPUT:2,%d,%s;", mypid, clencode(sbuf));
	    if (socket_send(buf) == 0) {
		strcpy(buf, socket_receive());
		if (strncmp(buf, "100:2,", 6) == 0) {
		    strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100            */
		    strncpy(resp, strtok(NULL, ","), 5);    /* Should be 2              */
		    strncpy(resp, strtok(NULL, ","), 5);    /* 1= fatal, chat ended	*/
		    rc = atoi(resp);
		    strncpy(resp, cldecode(strtok(NULL, ";")), 80);  /* The message              */
		    DispMsg(resp);
		    if (rc == 1) {
			Syslog('+', "Chat server error: %s", resp);
			stop = TRUE;
		    }
		}
	    }
	    curpos = 0;
	    memset(&sbuf, 0, sizeof(sbuf));
	    locate(rows - 1, 2);
	    clrtoeol();
	    colour(WHITE, BLACK);
	    mvprintw(rows - 1, 1, ">");
	    mvprintw(rows - 1, width + 2, "<");
	    colour(LIGHTGRAY, BLACK);
	}
    }
    chatting = FALSE;
    c_end = time(NULL);
    mib_chats++;
    mib_chatminutes += (unsigned int) ((c_end - c_start) / 60);


    /* 
     * Before sending the close command, purge all outstanding messages.
     */
    data = TRUE;
    while (data) {
	snprintf(buf, 200, "CGET:1,%d;", mypid);
	if (socket_send(buf) == 0) {
	    strncpy(buf, socket_receive(), sizeof(buf)-1);
	    if (strncmp(buf, "100:2,", 6) == 0) {
		strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100        */
		strncpy(resp, strtok(NULL, ","), 5);    /* Should be 2          */
		strncpy(resp, strtok(NULL, ","), 5);	/* 1= fatal error	*/
		rc = atoi(resp);
		memset(&resp, 0, sizeof(resp));
		strncpy(resp, cldecode(strtok(NULL, ";")), 80);  /* The message          */
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
	snprintf(buf, 200, "%s", (char *) Language(60));
	DispMsg(buf);
	Syslog('+', "Sysop chat ended");
	chat_with_sysop = FALSE;
    } else {
	Syslog('+', "User chat ended");
    }

    /*
     * Close server connection
     */
    snprintf(buf, 200, "CCLO,1,%d;", mypid);
    if (socket_send(buf) == 0) {
	strcpy(buf, socket_receive());
    }
    sleep(2);
    clear();
}


