/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Monitor Program 
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "proglock.h"
#include "mutil.h"


int		lines = 24;
int		columns = 80;

extern int	bbs_free;
extern int	ttyfd;
extern pid_t	mypid;

struct sysconfig    CFG;


char  	rbuf[50][80];	    /* Chat receive buffer	*/ /* FIXME: must be a dynamic buffer */
int	rpointer = 0;	    /* Chat receive pointer	*/
int	rsize = 5;	    /* Chat receive size	*/


static void die(int onsig)
{
    char    buf[128];
    
    signal(onsig, SIG_IGN);

    /*
     * Prevent clear screen when the program was locked
     */
    if (onsig != MBERR_NO_PROGLOCK)
	screen_stop(); 
    
    if (onsig && (onsig <= NSIG))
	Syslog('?', "MBMON Finished on signal %s", SigName[onsig]);
    else
	Syslog(' ', "MBMON Normally finished");
    
    sprintf(buf, "CSYS:2,%d,0;", mypid);
    if (socket_send(buf) == 0)
	sprintf(buf, "%s", socket_receive());
    ulockprogram((char *)"mbmon");
    ExitClient(0);
}



void ShowSysinfo(void)
{
    int	    ch;
    char    buf[128], *cnt;

    clr_index();
    set_color(WHITE, BLACK);
    mvprintw( 5, 6, "4.   SHOW BBS SYSTEM INFO");
    set_color(CYAN, BLACK);
    mvprintw( 7, 6, "1.   Total calls");
    mvprintw( 8, 6, "2.   Pots calls");
    mvprintw( 9, 6, "3.   ISDN calls");
    mvprintw(10, 6, "4.   Network calls");
    mvprintw(11, 6, "5.   Local calls");
    mvprintw(12, 6, "6.   Date started");
    mvprintw(13, 6, "7.   Last caller");
    center_addstr(lines - 3, (char *)"Press any key");
    IsDoing("View System Info");

    do {
	show_date(LIGHTGRAY, BLACK, 0, 0);
	set_color(LIGHTGRAY, BLACK);
	sprintf(buf, "GSYS:1,%d;", getpid());
	if (socket_send(buf) == 0) {
	    sprintf(buf, "%s", socket_receive());
	    if (strncmp(buf, "100:7,", 6) == 0) {
		cnt = strtok(buf, ",");
		mvprintw( 7,26, "%s", strtok(NULL, ","));
		mvprintw( 8,26, "%s", strtok(NULL, ","));
		mvprintw( 9,26, "%s", strtok(NULL, ","));
		mvprintw(10,26, "%s", strtok(NULL, ","));
		mvprintw(11,26, "%s", strtok(NULL, ","));
		mvprintw(12,26, "%s", strtok(NULL, ","));
		mvprintw(13,26, "%s", strtok(NULL, ";"));
		fflush(stdout);
	    }
	}
	ch = testkey(lines - 3, columns / 2 + 8);
    } while (ch == '\0');
}



void ShowLastcaller(void)
{
    int	    records, maxrows, ch, i, y, o;
    char    buf[128], *cnt;
	
    clr_index();
    set_color(WHITE, BLACK);
    mvprintw( 4, 6, "5.    SHOW BBS LASTCALLERS");
    set_color(YELLOW, RED);
    mvprintw( 6, 1, "Nr Username       Location     Level Device Time  Mins Calls Speed     Actions ");
    set_color(CYAN, BLACK);
    center_addstr(lines - 1, (char *)"Press any key");
    IsDoing("View Lastcallers");
    maxrows = lines - 10;

    do {
	show_date(LIGHTGRAY, BLACK, 0, 0);
	records = 0;
	sprintf(buf, "GLCC:0;");
	if (socket_send(buf) == 0) {
	    sprintf(buf, "%s", socket_receive());
	    if (strncmp(buf, "100:1,", 6) == 0) {
		cnt = strtok(buf, ",");
		records = atoi(strtok(NULL, ";"));
	    }
	}

	if (records) {
	    y = 7;
	    if (records > maxrows)
		o = records - maxrows;
	    else
		o = 1;
	    set_color(CYAN, BLACK);
	    for (i = o; i <= records; i++) {
		sprintf(buf, "GLCR:1,%d;", i);
		if (socket_send(buf) == 0) {
		    sprintf(buf, "%s", socket_receive());
		    if (strncmp(buf, "100:9,", 6) == 0) {
			cnt = strtok(buf, ",");
			mvprintw(y, 1, "%2d", i);
			mvprintw(y, 4, "%s", strtok(NULL, ","));
			mvprintw(y,19, "%s", strtok(NULL, ","));
			mvprintw(y,32, "%s", strtok(NULL, ","));
			mvprintw(y,38, "%s", strtok(NULL, ","));
			mvprintw(y,45, "%s", strtok(NULL, ","));
			mvprintw(y,51, "%s", strtok(NULL, ","));
			mvprintw(y,56, "%s", strtok(NULL, ","));
			mvprintw(y,62, "%s", strtok(NULL, ","));
			mvprintw(y,72, "%s", strtok(NULL, ";"));
			y++;
		    }
		}
	    }
	}
	ch = testkey(lines - 1, columns / 2 + 8);
    } while (ch == '\0');
}



void system_moni(void)
{
    int	    ch, y, eof;
    char    *cnt, buf[128];
    time_t  start, now;

    clr_index();
    set_color(WHITE, BLACK);
    mvprintw( 5, 6, "1.    SERVER CLIENTS");
    set_color(YELLOW, RED);
    mvprintw( 7, 1, "Pid   tty    user     program  city            doing                      time ");
    set_color(CYAN, BLACK);
    center_addstr(lines - 1, (char *)"Press any key");
    IsDoing("System Monitor");

    do {
	show_date(LIGHTGRAY, BLACK, 0, 0);

	eof = 0;
	set_color(LIGHTGRAY, BLACK);

	for (y = 8; y <= lines - 2; y++) { 
	    if (y == 8)
		sprintf(buf, "GMON:1,1;");
	    else
		sprintf(buf, "GMON:1,0;");
	    if (eof == 0) {
		if (socket_send(buf) == 0) {
		    strcpy(buf, socket_receive());
		    locate(y, 1);
		    clrtoeol();
		    if (strncmp(buf, "100:0;", 6) == 0) {
			/*
			 * There's no more information 
			 */
			eof = 1;
		    } else {
			cnt = strtok(buf, ",");
			mvprintw(y, 1, (char *)"%.5s", strtok(NULL, ","));
			mvprintw(y, 7, (char *)"%.6s", strtok(NULL, ","));
			mvprintw(y,14, (char *)"%.8s", strtok(NULL, ","));
			mvprintw(y,23, (char *)"%.8s", strtok(NULL, ","));
			mvprintw(y,32, (char *)"%.15s", strtok(NULL, ","));
			mvprintw(y,48, (char *)"%.26s", strtok(NULL, ","));
			start = atoi(strtok(NULL, ";"));
			now = time(NULL);
			mvprintw(y,75, (char *)"%s", t_elapsed(start, now));
		    }
		}
	    } else {
		/*
		 *  If no valid data, clear line
		 */
		locate(y, 1);
		clrtoeol();
	    }
	} /* for () */

	ch = testkey(lines - 1, columns / 2 + 8);
    } while (ch == '\0');
}



void system_stat(void)
{
    int	    ch;
    char    buf[256], *cnt;
    time_t  now;

    clr_index();
    set_color(WHITE, BLACK);
    mvprintw( 5, 6, "2.    SERVER STATISTICS");
    set_color(CYAN, BLACK);
    mvprintw( 7, 6, "First date started");
    mvprintw( 7,62, "BBS Open");
    mvprintw( 8, 6, "Last date started");
    mvprintw( 8,62, "ZMH");
    mvprintw( 9, 6, "Total server starts");
    mvprintw( 9,62, "Internet");
    mvprintw(10, 6, "Connected clients");
    mvprintw(10,62, "Need inet");
    mvprintw(11,62, "Running");
    mvprintw(12,30, "Total          Today");
    mvprintw(12,62, "Load avg");
    hor_lin(13,30,8);
    hor_lin(13,45,8);
    mvprintw(13,62, "Diskspace");
    mvprintw(14, 6, "Client connects");
    mvprintw(15, 6, "Peak connections");
    mvprintw(16, 6, "Protocol syntax errors");
    mvprintw(17, 6, "Communication errors");
    mvprintw(19, 6, "Next sequence number");
    mvprintw(19,62, "Press any key");
    IsDoing("System Statistics");

    do {
	show_date(LIGHTGRAY, BLACK, 0, 0);

	sprintf(buf, "GSTA:1,%d;", getpid());
	if (socket_send(buf) == 0) {
	    strcpy(buf, socket_receive());
	    set_color(LIGHTGRAY, BLACK);
	    cnt = strtok(buf, ",");
	    now = atoi(strtok(NULL, ","));
	    mvprintw(7, 30, "%s", ctime(&now));
	    now = atoi(strtok(NULL, ","));
	    mvprintw(8, 30, "%s", ctime(&now));
	    cnt = strtok(NULL, ",");
	    mvprintw(9, 30, (char *)"%s ", strtok(NULL, ","));
	    mvprintw(10,30, (char *)"%s ", strtok(NULL, ","));
	    mvprintw(14,30, (char *)"%s ", strtok(NULL, ","));
	    mvprintw(15,30, (char *)"%s ", strtok(NULL, ","));
	    mvprintw(16,30, (char *)"%s ", strtok(NULL, ","));
	    mvprintw(17,30, (char *)"%s ", strtok(NULL, ","));
	    mvprintw(14,45, (char *)"%s ", strtok(NULL, ","));
	    mvprintw(15,45, (char *)"%s ", strtok(NULL, ","));
	    mvprintw(16,45, (char *)"%s ", strtok(NULL, ","));
	    mvprintw(17,45, (char *)"%s ", strtok(NULL, ","));
	    mvprintw(7,72, "%s", atoi(strtok(NULL, ",")) == 1?"Yes":"No ");
	    mvprintw(8,72, "%s", atoi(strtok(NULL, ",")) == 1?"Yes":"No ");
	    mvprintw(9,72, "%s", atoi(strtok(NULL, ",")) == 1?"Yes":"No ");
	    mvprintw(10,72,"%s", atoi(strtok(NULL, ",")) == 1?"Yes":"No ");
	    mvprintw(11,72,"%s", atoi(strtok(NULL, ",")) == 1?"Yes":"No ");
	    mvprintw(12,72, "%s ", strtok(NULL, ","));
	    mvprintw(19,30, (char *)"%s", strtok(NULL, ";"));
	}

	switch (enoughspace(CFG.freespace)) {
	    case 0: mvprintw(13, 72, "Full ");
		    break;
	    case 1: mvprintw(13, 72, "Ok   ");
		    break;
	    case 2: mvprintw(13, 72, "N/A  ");
		    break;
	    case 3: mvprintw(13, 72, "Error");
		    break;
	}

	ch = testkey(19,76);
    } while (ch == '\0');
}



void disk_stat(void)
{
    int		    ch, i;
    char	    buf[1024], *cnt, *type, *fs, *p, sign;
    unsigned long   last[10], size, used, perc, avail;

    clr_index();
    set_color(WHITE, BLACK);
    mvprintw( 5, 6, "3.    FILESYSTEM USAGE");
    set_color(YELLOW, RED);
    mvprintw( 7, 1, " Size MB   Used MB     Perc. FS-Type   Mountpoint                             ");
    set_color(CYAN, BLACK);
    mvprintw(lines - 2, 6, "Press any key");
    IsDoing("Filesystem Usage");
    for (i = 0; i < 10; i++)
	last[i] = 0;

    do {
	show_date(LIGHTGRAY, BLACK, 0, 0);

	sprintf(buf, "DGFS:0;");
	if (socket_send(buf) == 0) {
	    strcpy(buf, socket_receive());
	    set_color(LIGHTGRAY, BLACK);
	    cnt = strtok(buf, ":");
	    cnt = strtok(NULL, ",;");
	    if (atoi(cnt)) {
		for (i = 0; i < atoi(cnt); i++) {
		    p = strtok(NULL, " ");
		    size = atoi(p);
		    avail = atoi(strtok(NULL, " "));
		    used = size - avail;
		    perc = (used * 100) / size;
		    sign = ' ';
		    fs = strtok(NULL, " ");
		    type = strtok(NULL, ",;");
		    if (used > last[i])
			sign = '^';
		    if (used < last[i])
			sign = 'v';
		    if (last[i] == 0)
			sign = ' ';
		    last[i] = used;
		    set_color(CYAN, BLACK);
		    mvprintw(i+8, 1, "%8lu  %8lu  ", size, used);
		    set_color(WHITE, BLACK);
		    printf("%c  ", sign);
		    if (strstr(type, "iso") == NULL) {
			if (avail <= CFG.freespace)
			    set_color(LIGHTRED, BLACK);
			else if (avail <= (CFG.freespace * 4))
			    set_color(YELLOW, BLACK);
			else
			    set_color(CYAN, BLACK);
		    } else {
			set_color(CYAN, BLACK);
		    }
		    printf("%3lu", perc);
		    putchar('%');
		    set_color(CYAN, BLACK);
		    printf("  %-8s  %-40s", type, fs);
		}
		locate(i+8, 1);
		clrtoeol();
	    }
	}

	ch = testkey(lines - 2, 20);
    } while (ch == '\0');
}



void soft_info(void)
{
	char	temp[81], *p;

	clr_index();
	set_color(YELLOW, BLACK);
#ifdef __linux__
	p = xstrcpy((char *)"MBSE BBS (GNU/Linux");
#elif __FreeBSD__
	p = xstrcpy((char *)"MBSE BBS (FreeBSD");
#elif __NetBSD__
	p = xstrcpy((char *)"MBSE BBS (NetBSD");
#else
	p = xstrcpy((char *)"MBSE BBS (Unknown");
#endif
#ifdef __i386__
	p = xstrcat(p, (char *)" i386)");
#elif __PPC__
	p = xstrcat(p, (char *)" PPC)");
#elif __sparc__
	p = xstrcat(p, (char *)" Sparc)");
#elif __alpha__
	p = xstrcat(p, (char *)" Alpha)");
#elif __hppa__
	p = xstrcat(p, (char *)" HPPA)");
#else
	p = xstrcat(p, (char *)" Unknown)");
#endif
	center_addstr( 6, p);
	free(p);
	set_color(WHITE, BLACK);
	center_addstr( 8, (char *)COPYRIGHT);
	set_color(YELLOW, BLACK);
	center_addstr(10, (char *)"Made in the Netherlands.");
	set_color(WHITE, BLACK);
#ifdef __GLIBC__
	sprintf(temp, "Compiled on glibc v%d.%d", __GLIBC__, __GLIBC_MINOR__);
#else
#ifdef __GNU_LIBRARY__
	sprintf(temp, "Compiled on libc v%d", __GNU_LIBRARY__);
#else
	sprintf(temp, "Compiled on unknown library");
#endif
#endif
	center_addstr(12, temp);
	set_color(LIGHTCYAN, BLACK);
	center_addstr(14, (char *)"http://www.mbse.dds.nl or 2:280/2802");
	set_color(LIGHTGREEN, BLACK);
	center_addstr(lines -7, (char *)"This is free software; released under the terms of the GNU General");
	center_addstr(lines -6, (char *)"Public License as published by the Free Software Foundation.");
	set_color(CYAN, BLACK);
	center_addstr(lines -4, (char *)"Press any key");
	readkey(lines - 4, columns / 2 + 8, LIGHTGRAY, BLACK);
}



/*
 * Colorize the chat window
 */
void Showline(int y, int x, char *msg)
{
    int     i;

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
 * Display received chat message
 */
void DispMsg(char *);
void DispMsg(char *msg)
{
    int	    i;

    strncpy(rbuf[rpointer], msg, 80);
    Showline(4+rpointer, 1, rbuf[rpointer]);
    if (rpointer == rsize) {
	/*
	 * Scroll buffer
	 */
	for (i = 0; i <= rsize; i++) {
	    locate(i+4,1);
	    clrtoeol();
	    sprintf(rbuf[i], "%s", rbuf[i+1]);
	    Showline(i+4, 1, rbuf[i]);
	}
    } else {
	rpointer++;
    }
    fflush(stdout);
}



/*
 * Sysop/user chat
 */
void Chat(int sysop)
{
    int		    curpos = 0, stop = FALSE, data, rc;
    unsigned char   ch = 0;
    char	    sbuf[81], resp[128], *cnt, *msg;
    static char	    buf[200];

    clr_index();
    rsize = lines - 7;
    rpointer = 0;

    sprintf(buf, "CCON,3,%d,%s,%s;", mypid, CFG.sysop, sysop ? "1":"0");
    Syslog('-', "> %s", buf);
    if (socket_send(buf) == 0) {
	strcpy(buf, socket_receive());
	Syslog('-', "< %s", buf);
	if (strncmp(buf, "100:1,", 6) == 0) {
	    cnt = strtok(buf, ",");
	    msg = strtok(NULL, "\0");
	    msg[strlen(msg)-1] = '\0';
	    set_color(LIGHTRED, BLACK);
	    mvprintw(4, 1, msg);
	    working(2, 0, 0);
	    working(0, 0, 0);
	    center_addstr(lines -4, (char *)"Press any key");
	    readkey(lines - 4, columns / 2 + 8, LIGHTGRAY, BLACK);
	    return;
	}
    }

    locate(lines - 2, 1);
    set_color(WHITE, BLUE);
    clrtoeol();
    mvprintw(lines - 2, 2, "Chat, type \"/EXIT\" to exit");

    set_color(WHITE, BLACK);
    mvprintw(lines - 1, 1, ">");
    memset(&sbuf, 0, sizeof(sbuf));
    memset(&rbuf, 0, sizeof(rbuf));

    if (sysop) {
	/*
	 * Join channel #sysop automatic
	 */
	sprintf(buf, "CPUT:2,%d,/JOIN #sysop;", mypid);
	if (socket_send(buf) == 0) {
	    strcpy(buf, socket_receive());
	}
    }

    Syslog('-', "Start loop");

    while (stop == FALSE) {

	/*
	 * Check for new message, loop fast until no more data available.
	 */
	data = TRUE;
	while (data) {
	    sprintf(buf, "CGET:1,%d;", mypid);
	    if (socket_send(buf) == 0) {
		memset(&buf, 0, sizeof(buf));
		strncpy(buf, socket_receive(), sizeof(buf)-1);
		if (strncmp(buf, "100:2,", 6) == 0) {
		    Syslog('-', "> CGET:1,%d;", mypid);
		    Syslog('-', "< %s", buf);
		    strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100	    */
		    strncpy(resp, strtok(NULL, ","), 5);    /* Should be 2	    */
		    strncpy(resp, strtok(NULL, ","), 5);    /* 1= fatal error	    */
		    rc = atoi(resp);
		    memset(&resp, 0, sizeof(resp));
		    strncpy(resp, strtok(NULL, "\0"), 80);  /* The message	    */
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
	 * Update top bars
	 */
	show_date(LIGHTGRAY, BLACK, 0, 0);

	/*
	 * Check for a pressed key, if so then process it
	 */
	ch = testkey(lines -1, curpos + 2);
	if (isprint(ch)) {
	    set_color(CYAN, BLACK);
	    if (curpos < 77) {
		putchar(ch);
		fflush(stdout);
		sbuf[curpos] = ch;
		curpos++;
	    } else {
		putchar(7);
	    }
	} else if ((ch == KEY_BACKSPACE) || (ch == KEY_RUBOUT) || (ch == KEY_DEL)) {
	    set_color(CYAN, BLACK);
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
		    strncpy(resp, strtok(NULL, ","), 5);    /* 1= fatal error, end chat	*/
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
	    locate(lines - 1, 2);
	    clrtoeol();
	    set_color(WHITE, BLACK);
	    mvprintw(lines - 1, 1, ">");
	}
    }

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
		    data = FALSE;
		}
	    } else {
		data = FALSE;
	    }
	}
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
    sleep(1);
}



int main(int argc, char *argv[])
{
    struct passwd   *pw;
    char	    buf[128];
    int		    rc;


    /*
     * Find out who is on the keyboard or automated the keyboard.
     */
    pw = getpwuid(geteuid());
    if (strcmp(pw->pw_name, (char *)"mbse")) {
	printf("ERROR: only user \"mbse\" may use this program\n");
	exit(MBERR_INIT_ERROR);
    }

    /*
     * Read the global configuration data, registrate connection
     */
    InitConfig();
    InitClient(pw->pw_name, (char *)"mbmon", CFG.location, (char *)"mbmon.log", CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);
    Syslog(' ', "MBMON Started by %s", pw->pw_name);
    bbs_free = FALSE;

    /*
     * Report sysop available for chat
     */
    sprintf(buf, "CSYS:2,%d,1;", mypid);
    if (socket_send(buf) == 0)
	sprintf(buf, "%s", socket_receive());

	
    /*
     * Setup several signals so when the program terminate's it
     * will properly close.
     */
    signal(SIGINT, (void (*))die);
    signal(SIGBUS, (void (*))die);
    signal(SIGSEGV,(void (*))die);
    signal(SIGTERM,(void (*))die);
    signal(SIGKILL,(void (*))die);

    /*
     * Find out if the environment variables LINES and COLUMNS are present,
     * if so, then use these for screen dimensions.
     */
    if (getenv("LINES")) {
	rc = atoi(getenv("LINES"));
	if (rc >= 24)
	    lines = rc;
    }
    if (getenv("COLUMNS")) {
	rc = atoi(getenv("COLUMNS"));
	if (rc >= 80)
	    columns = rc;
    }
    Syslog('-', "Screen size set to %dx%d", columns, lines);

    if (lockprogram((char *)"mbmon")) {
	printf("\n\7Another mbmon is already running, abort.\n\n");
	die(MBERR_NO_PROGLOCK);
    }
    
    screen_start((char *)"MBmon");

    for (;;) {

	IsDoing("Browsing Menu");
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "0.    MBSE BBS MONITOR");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.    View Server Clients");
	mvprintw( 8, 6, "2.    View Server Statistics");
	mvprintw( 9, 6, "3.    View Filesystem Usage");
	mvprintw(10, 6, "4.    View BBS System Information");
	mvprintw(11, 6, "5.    View BBS Lastcallers List");
	mvprintw(12, 6, "6.    Chat with any user");
	mvprintw(13, 6, "7.    Respond to sysop page");
	mvprintw(14, 6, "8.    View Software Information");

	switch(select_menu(8)) {
	    case 0:
		    die(0);
		    break;
	    case 1:
		    system_moni();
		    break;
	    case 2:
		    system_stat();
		    break;
	    case 3:
		    disk_stat();
		    break;
	    case 4:
		    ShowSysinfo();
		    break;
	    case 5:
		    ShowLastcaller();
		    break;
	    case 6:
		    Chat(FALSE);
		    break;
	    case 7:
		    Chat(TRUE);
		    break;
	    case 8:
		    soft_info();
		    break;
	}
    }
}

