/*****************************************************************************
 * 
 * ftnmon.c
 * Purpose ...............: Monitor Program 
 *
 *****************************************************************************
 * Copyright (C) 2012-2013 Robert James Clay <jame@rocasa.us>
 * Copyright (C) 1997-2011 Michiel Broek <mbse@mbse.eu>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/ftndlib.h"
#include "../lib/users.h"
#include "../lib/ftnddb.h"
#include "proglock.h"
#include "ftnutil.h"


int     cols = 80;
int     rows = 24;

extern int  bbs_free;
extern int  ttyfd;
extern pid_t    mypid;

struct sysconfig    CFG;


char    rbuf[50][81];   /* Chat receive buffer  */ /* FIXME: must be a dynamic buffer */
int rpointer = 0;       /* Chat receive pointer */
int rsize = 5;      /* Chat receive size    */


static void die(int onsig)
{
    char    buf[128];

    signal(onsig, SIG_IGN);

    /*
     * Prevent clear screen when the program was locked
     */
    if (onsig != FTNERR_NO_PROGLOCK)
        screen_stop(); 

    if (onsig && (onsig <= NSIG))
        syslog('?', "FTNMON Finished on signal %s", SigName[onsig]);
    else
        Syslog(' ', "FTNMON Normally finished");

    snprintf(buf, 128, "CSYS:2,%d,0;", mypid);
    if (socket_send(buf) == 0)
        snprintf(buf, 128, "%s", socket_receive());
    ulockprogram((char *)"ftnmon");
    ExitClient(0);
}



void ShowSysinfo(void)
{
    int     ch;
    char    buf[128], *lc;

    clr_index();
    set_color(WHITE, BLACK);
    ftnd_mvprintw( 5, 6, "4.   SHOW BBS SYSTEM INFO");
    set_color(CYAN, BLACK);
    ftnd_mvprintw( 7, 6, "1.   Total calls");
    ftnd_mvprintw( 8, 6, "2.   Pots calls");
    ftnd_mvprintw( 9, 6, "3.   ISDN calls");
    ftnd_mvprintw(10, 6, "4.   Network calls");
    ftnd_mvprintw(11, 6, "5.   Local calls");
    ftnd_mvprintw(12, 6, "6.   Date started");
    ftnd_mvprintw(13, 6, "7.   Last caller");
    center_addstr(rows - 3, (char *)"Press any key");
    IsDoing("View System Info");

    do {
        show_date(LIGHTGRAY, BLACK, 0, 0);
        set_color(LIGHTGRAY, BLACK);
        snprintf(buf, 128, "GSYS:1,%d;", getpid());
        if (socket_send(buf) == 0) {
            snprintf(buf, 128, "%s", socket_receive());
        if (strncmp(buf, "100:7,", 6) == 0) {
            strtok(buf, ",");
            ftnd_mvprintw( 7,26, "%s", strtok(NULL, ","));
            ftnd_mvprintw( 8,26, "%s", strtok(NULL, ","));
            ftnd_mvprintw( 9,26, "%s", strtok(NULL, ","));
            ftnd_mvprintw(10,26, "%s", strtok(NULL, ","));
            ftnd_mvprintw(11,26, "%s", strtok(NULL, ","));
            ftnd_mvprintw(12,26, "%s", strtok(NULL, ","));
            lc = xstrcpy(cldecode(strtok(NULL, ";")));
            ftnd_mvprintw(13,26, "%s", lc);
            free(lc);
            fflush(stdout);
        }
    }
    ch = testkey(rows - 3, cols / 2 + 8);
    } while (ch == '\0');
}



void ShowLastcaller(void)
{
    int     records, maxrows, ch, i, y, o;
    char    buf[128];

    clr_index();
    set_color(WHITE, BLACK);
    ftnd_mvprintw( 4, 6, "5.    SHOW BBS LASTCALLERS");
    set_color(YELLOW, RED);
    ftnd_mvprintw( 6, 1, "Nr Username       Location     Level Device Time  Mins Calls Speed     Actions ");
    set_color(CYAN, BLACK);
    center_addstr(rows - 1, (char *)"Press any key");
    IsDoing("View Lastcallers");
    maxrows = rows - 10;

    do {
        show_date(LIGHTGRAY, BLACK, 0, 0);
        records = 0;
        snprintf(buf, 128, "GLCC:0;");
        if (socket_send(buf) == 0) {
            snprintf(buf, 128, "%s", socket_receive());
        if (strncmp(buf, "100:1,", 6) == 0) {
            strtok(buf, ",");
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
                snprintf(buf, 128, "GLCR:1,%d;", i);
                if (socket_send(buf) == 0) {
                    snprintf(buf, 128, "%s", socket_receive());
                if (strncmp(buf, "100:9,", 6) == 0) {
                    strtok(buf, ",");
                    ftnd_mvprintw(y, 1, "%2d", i);
                    ftnd_mvprintw(y, 4, "%s", cldecode(strtok(NULL, ",")));
                    ftnd_mvprintw(y,19, "%s", cldecode(strtok(NULL, ",")));
                    ftnd_mvprintw(y,32, "%s", strtok(NULL, ","));
                    ftnd_mvprintw(y,38, "%s", strtok(NULL, ","));
                    ftnd_mvprintw(y,45, "%s", strtok(NULL, ","));
                    ftnd_mvprintw(y,51, "%s", strtok(NULL, ","));
                    ftnd_mvprintw(y,56, "%s", strtok(NULL, ","));
                    ftnd_mvprintw(y,62, "%s", strtok(NULL, ","));
                    ftnd_mvprintw(y,72, "%s", strtok(NULL, ";"));
                    y++;
                }
            }
        }
    }
    ch = testkey(rows - 1, cols / 2 + 8);
    } while (ch == '\0');
}



void system_moni(void)
{
    int	    ch, y, eof;
    char    buf[128];
    time_t  start, now;

    clr_index();
    set_color(WHITE, BLACK);
    ftnd_mvprintw( 5, 6, "1.    SERVER CLIENTS");
    set_color(YELLOW, RED);
    ftnd_mvprintw( 7, 1, "Pid   tty    user     program  city            doing                      time ");
    set_color(CYAN, BLACK);
    center_addstr(rows - 1, (char *)"Press any key");
    IsDoing("System Monitor");

    do {
        show_date(LIGHTGRAY, BLACK, 0, 0);

        eof = 0;
        set_color(LIGHTGRAY, BLACK);

        for (y = 8; y <= rows - 2; y++) { 
            if (y == 8)
                snprintf(buf, 128, "GMON:1,1;");
            else
                snprintf(buf, 128, "GMON:1,0;");
            if (eof == 0) {
                if (socket_send(buf) == 0) {
                    strncpy(buf, socket_receive(), 128);
                    ftnd_locate(y, 1);
                    clrtoeol();
                if (strncmp(buf, "100:0;", 6) == 0) {
                    /*
                     * There's no more information 
                     */
                    eof = 1;
                } else {
                    strtok(buf, ",");
                    ftnd_mvprintw(y, 1, (char *)"%.5s", strtok(NULL, ","));
                    ftnd_mvprintw(y, 7, (char *)"%.6s", strtok(NULL, ","));
                    ftnd_mvprintw(y,14, (char *)"%.8s", cldecode(strtok(NULL, ",")));
                    ftnd_mvprintw(y,23, (char *)"%.8s", cldecode(strtok(NULL, ",")));
                    ftnd_mvprintw(y,32, (char *)"%.15s", cldecode(strtok(NULL, ",")));
                    ftnd_mvprintw(y,48, (char *)"%.26s", cldecode(strtok(NULL, ",")));
                    start = atoi(strtok(NULL, ";"));
                    now = time(NULL);
                    ftnd_mvprintw(y,75, (char *)"%s", t_elapsed(start, now));
                }
            }
        } else {
            /*
             *  If no valid data, clear line
             */
            ftnd_locate(y, 1);
            clrtoeol();
        }
    } /* for () */

    ch = testkey(rows - 1, cols / 2 + 8);
    } while (ch == '\0');
}



void system_stat(void)
{
    int	    ch;
    char    buf[256];
    time_t  now;

    clr_index();
    set_color(WHITE, BLACK);
    ftnd_mvprintw( 5, 6, "2.    SERVER STATISTICS");
    set_color(CYAN, BLACK);
    ftnd_mvprintw( 7, 6, "First date started");
    ftnd_mvprintw( 7,59, "BBS Open");
    ftnd_mvprintw( 8, 6, "Last date started");
    ftnd_mvprintw( 8,59, "ZMH");
    ftnd_mvprintw( 9, 6, "Total server starts");
    ftnd_mvprintw( 9,59, "Internet up");
    ftnd_mvprintw(10, 6, "Connected clients");
    ftnd_mvprintw(10,59, "Need inet");
    ftnd_mvprintw(11,59, "Running");
    ftnd_mvprintw(12,30, "Total          Today");
    ftnd_mvprintw(12,59, "Load average");
    hor_lin(13,30,8);
    hor_lin(13,45,8);
    ftnd_mvprintw(13,59, "Diskspace");
    ftnd_mvprintw(14, 6, "Client connects");
    ftnd_mvprintw(14,59, "IBC servers");
    ftnd_mvprintw(15, 6, "Peak connections");
    ftnd_mvprintw(15,59, "IBC channels");
    ftnd_mvprintw(16, 6, "Protocol syntax errors");
    ftnd_mvprintw(16,59, "IBC users");
    ftnd_mvprintw(17, 6, "Communication errors");
    ftnd_mvprintw(19, 6, "Next sequence number");
    ftnd_mvprintw(rows -3,59, "Press any key");
    IsDoing("System Statistics");

    do {
        show_date(LIGHTGRAY, BLACK, 0, 0);

    snprintf(buf, 256, "GSTA:1,%d;", getpid());
    if (socket_send(buf) == 0) {
        strncpy(buf, socket_receive(), 256);
        set_color(LIGHTGRAY, BLACK);
        strtok(buf, ",");
        now = atoi(strtok(NULL, ","));
        ftnd_mvprintw(7, 30, "%s", ctime(&now));
        now = atoi(strtok(NULL, ","));
        ftnd_mvprintw(8, 30, "%s", ctime(&now));
        strtok(NULL, ",");
        ftnd_mvprintw(9, 30, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(10,30, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(14,30, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(15,30, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(16,30, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(17,30, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(14,45, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(15,45, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(16,45, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(17,45, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(7,72, "%s", atoi(strtok(NULL, ",")) == 1?"Yes":"No ");
        ftnd_mvprintw(8,72, "%s", atoi(strtok(NULL, ",")) == 1?"Yes":"No ");
        ftnd_mvprintw(9,72, "%s", atoi(strtok(NULL, ",")) == 1?"Yes":"No ");
        ftnd_mvprintw(10,72,"%s", atoi(strtok(NULL, ",")) == 1?"Yes":"No ");
        ftnd_mvprintw(11,72,"%s", atoi(strtok(NULL, ",")) == 1?"Yes":"No ");
        ftnd_mvprintw(12,72, "%s ", strtok(NULL, ","));
        ftnd_mvprintw(19,30, (char *)"%s", strtok(NULL, ","));
        ftnd_mvprintw(14,72, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(15,72, (char *)"%s ", strtok(NULL, ","));
        ftnd_mvprintw(16,72, (char *)"%s ", strtok(NULL, ";"));
    }

    switch (enoughspace(CFG.freespace)) {
        case 0: ftnd_mvprintw(13, 72, "Full ");
            break;
        case 1: ftnd_mvprintw(13, 72, "Ok   ");
            break;
        case 2: ftnd_mvprintw(13, 72, "N/A  ");
            break;
        case 3: ftnd_mvprintw(13, 72, "Error");
            break;
    }

    ch = testkey(rows -3,73);
    } while (ch == '\0');
}



void disk_stat(void)
{
    int     ch, i, ro;
    char    buf[1024], *cnt, *type, *fs, *p, sign;
    unsigned int    last[10], size, used, perc, avail;

    clr_index();
    set_color(WHITE, BLACK);
    ftnd_mvprintw( 5, 6, "3.    FILESYSTEM USAGE");
    set_color(YELLOW, RED);
    ftnd_mvprintw( 7, 1, " Size MB   Free MB     Used  FS-Type   St Mountpoint                          ");
    set_color(CYAN, BLACK);
    ftnd_mvprintw(rows - 2, 6, "Press any key");
    IsDoing("Filesystem Usage");
    for (i = 0; i < 10; i++)
        last[i] = 0;

    do {
        show_date(LIGHTGRAY, BLACK, 0, 0);

        snprintf(buf, 1024, "DGFS:0;");
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
            type = strtok(NULL, " ");
            ro = atoi(strtok(NULL, ",;"));
            if (used > last[i])
                sign = '^';
            if (used < last[i])
                sign = 'v';
            if (last[i] == 0)
                sign = ' ';
                last[i] = used;
                set_color(CYAN, BLACK);
                ftnd_mvprintw(i+8, 1, "%8lu  %8lu  ", size, avail);
                set_color(WHITE, BLACK);
                printf("%c  ", sign);
            if (ro == 0) {
                if (avail <= CFG.freespace)
                    set_color(LIGHTRED, BLACK);
                else if (avail <= (CFG.freespace * 4))
                    set_color(YELLOW, BLACK);
                    else
                        set_color(CYAN, BLACK);
              } else {
                set_color(GREEN, BLACK);
            }
            printf("%3u", perc);
            putchar('%');
            set_color(CYAN, BLACK);
            printf("  %-8s  %s %-37s", type, ro ?"RO":"RW", fs);
        }
        ftnd_locate(i+8, 1);
        clrtoeol();
        }
    }

    ch = testkey(rows - 2, 20);
    } while (ch == '\0');
}



void soft_info(void)
{
    char    temp[81], *p;

    clr_index();
    set_color(YELLOW, BLACK);

#if defined(__linux__)
	p = xstrcpy((char *)"FTNd (GNU/Linux");
#elif defined(__FreeBSD__)
	p = xstrcpy((char *)"FTNd (FreeBSD");
#elif defined(__NetBSD__)
	p = xstrcpy((char *)"FTNd (NetBSD");
#elif defined(__OpenBSD__)
	p = xstrcpy((char *)"FTNd (OpenBSD");
#else
#error "Unknown OS"
#endif

#if defined(__i386__)
	p = xstrcat(p, (char *)" i386)");
#elif defined(__x86_64__)
	p = xstrcat(p, (char *)" x86-64");
#elif defined(__PPC__) || defined(__ppc__)
	p = xstrcat(p, (char *)" PPC)");
#elif defined(__sparc__)
	p = xstrcat(p, (char *)" Sparc)");
#elif defined(__alpha__)
	p = xstrcat(p, (char *)" Alpha)");
#elif defined(__hppa__)
	p = xstrcat(p, (char *)" HPPA)");
#elif defined(__arm__)
	p = xstrcat(p, (char *)" ARM)");
#else
#error "Unknown CPU"
#endif

    center_addstr( 6, p);
    free(p);
    set_color(WHITE, BLACK);
    center_addstr( 8, (char *)COPYRIGHT);
    set_color(YELLOW, BLACK);
    center_addstr(10, (char *)"Made in the Netherlands.");
    set_color(WHITE, BLACK);
#ifdef __GLIBC__
    snprintf(temp, 81, "Compiled on glibc v%d.%d", __GLIBC__, __GLIBC_MINOR__);
#else
#ifdef __GNU_LIBRARY__
    snprintf(temp, 81, "Compiled on libc v%d", __GNU_LIBRARY__);
#else
    snprintf(temp, 81, "Compiled on unknown library");
#endif
#endif
    center_addstr(12, temp);
    set_color(LIGHTCYAN, BLACK);
    center_addstr(14, (char *)"http://rocasa.org");
    set_color(LIGHTGREEN, BLACK);
    center_addstr(rows -7, (char *)"This is free software; released under the terms of the GNU General");
    center_addstr(rows -6, (char *)"Public License as published by the Free Software Foundation.");
    set_color(CYAN, BLACK);
    center_addstr(rows -4, (char *)"Press any key");
    readkey(rows - 4, cols / 2 + 8, LIGHTGRAY, BLACK);
}



/*
 * Colorize the chat window
 */
void Showline(int y, int x, char *msg)
{
    int     i, done = FALSE;

    if (strlen(msg)) {
        ftnd_locate(y, x);
    if (msg[0] == '<') {
        ftnd_colour(LIGHTCYAN, BLACK);
        putchar('<');
        ftnd_colour(LIGHTBLUE, BLACK);
        for (i = 1; i < strlen(msg); i++) {
            if ((msg[i] == '>') && (! done)) {
                ftnd_colour(LIGHTCYAN, BLACK);
                putchar(msg[i]);
                ftnd_colour(CYAN, BLACK);
                done = TRUE;
            } else {
                putchar(msg[i]);
            }
        }
    } else if (msg[0] == '*') {
        if (msg[1] == '*') {
            if (msg[2] == '*')
                ftnd_colour(YELLOW, BLACK);
            else
                ftnd_colour(LIGHTRED, BLACK);
        } else {
            ftnd_colour(LIGHTMAGENTA, BLACK);
        }
        for (i = 0; i < strlen(msg); i++)
            putchar(msg[i]);
        } else {
            ftnd_colour(GREEN, BLACK);
            for (i = 0; i < strlen(msg); i++)
            putchar(msg[i]);
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

    /*
     * Beep on minor system messages
     */
    if ((msg[0] == '*') && (msg[1] != '*'))
        putchar('\007');

        strncpy(rbuf[rpointer], msg, 81);
        Showline(4+rpointer, 1, rbuf[rpointer]);
    if (rpointer == rsize) {
        /*
         * Scroll buffer
         */
        for (i = 0; i <= rsize; i++) {
            ftnd_locate(i+4,1);
            clrtoeol();
            strncpy(rbuf[i], rbuf[i+1], 81);
//           snprintf(rbuf[i], 81, "%s", rbuf[i+1]);
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
    int     curpos = 0, width, stop = FALSE, data, rc;
    unsigned char   ch = 0;
    char        *p, sbuf[81], resp[128], *sysop_name, *name;
    static char     buf[200];

    clr_index();
    rsize = rows - 7;
    rpointer = 0;

    sysop_name = xstrcpy(clencode(CFG.sysop_name));
    name = xstrcpy(clencode(CFG.sysop));
    width = cols - (strlen(name) + 3);
    snprintf(buf, 200, "CCON,4,%d,%s,%s,%s;", mypid, sysop_name, name, sysop ? "1":"0");
    free(sysop_name);
    free(name);

    if (socket_send(buf) == 0) {
        strcpy(buf, socket_receive());
        if (strncmp(buf, "200:1,", 6) == 0) {
            set_color(LIGHTRED, BLACK);
            ftnd_mvprintw(4, 1, (char *)"Add \"fido  60179/udp  # Chatserver\" to /etc/services");
            ftnd_mvprintw(5, 1, (char *)"Leave ftnmon, then restart ftntask and come back here");
            working(2, 0, 0);
            working(0, 0, 0);
            center_addstr(rows -4, (char *)"Press any key");
            readkey(rows - 4, cols / 2 + 8, LIGHTGRAY, BLACK);
            return;
        }
    }

    ftnd_locate(rows - 2, 1);
    set_color(WHITE, BLUE);
    clrtoeol();
    ftnd_mvprintw(rows - 2, 2, "Chat, type \"/EXIT\" to exit or \"/HELP\" for help");

    set_color(WHITE, BLACK);
    ftnd_mvprintw(rows - 1, 1, ">");
    ftnd_mvprintw(rows - 1, width + 2, "<");
    memset(&sbuf, 0, sizeof(sbuf));
    memset(&rbuf, 0, sizeof(rbuf));

    if (sysop) {
        /*
         * Join channel #sysop automatic
         */
        snprintf(buf, 200, "CPUT:2,%d,/JOIN #sysop;", mypid);
        if (socket_send(buf) == 0) {
            strcpy(buf, socket_receive());
        }
    }

    while (stop == FALSE) {

        /*
         * Check for new message, loop fast until no more data available.
         */
        data = TRUE;
        while (data) {
            snprintf(buf, 200, "CGET:1,%d;", mypid);
            if (socket_send(buf) == 0) {
                memset(&buf, 0, sizeof(buf));
                strncpy(buf, socket_receive(), sizeof(buf)-1);
            if (strncmp(buf, "100:2,", 6) == 0) {
                strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100        */
                strncpy(resp, strtok(NULL, ","), 5);    /* Should be 2          */
                strncpy(resp, strtok(NULL, ","), 5);    /* 1= fatal error       */
                rc = atoi(resp);
                memset(&resp, 0, sizeof(resp));
                strncpy(resp, cldecode(strtok(NULL, ";")), 81);  /* The message     */
                DispMsg(resp);
                if (rc == 1) {
                    Syslog('+', "Chat server message: %s", resp);
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
    ch = testkey(rows -1, curpos + 2);
    if (isprint(ch)) {
        set_color(CYAN, BLACK);
        if (curpos < width) {
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
        snprintf(buf, 12, "%d", mypid);
        p = xstrcpy((char *)"CPUT:2,");
        p = xstrcat(p, buf);
        p = xstrcat(p, (char *)",");
        p = xstrcat(p, clencode(sbuf));
        p = xstrcat(p, (char *)";");
        strncpy(buf, p, 200);
        free(p);
        if (socket_send(buf) == 0) {
            strcpy(buf, socket_receive());
            if (strncmp(buf, "100:2,", 6) == 0) {
                strncpy(resp, strtok(buf, ":"), 10);    /* Should be 100            */
                strncpy(resp, strtok(NULL, ","), 5);    /* Should be 2              */
                strncpy(resp, strtok(NULL, ","), 5);    /* 1= fatal error, end chat	*/
                rc = atoi(resp);
                strncpy(resp, cldecode(strtok(NULL, ";")), 81);  /* The message              */
                DispMsg(resp);
                if (rc == 1) {
                    Syslog('+', "Chat server error: %s", resp);
                stop = TRUE;
                }
            }
        }
        curpos = 0;
        memset(&sbuf, 0, sizeof(sbuf));
        ftnd_locate(rows - 1, 2);
        clrtoeol();
        set_color(WHITE, BLACK);
        ftnd_mvprintw(rows - 1, 1, ">");
        ftnd_mvprintw(rows - 1, width + 2, "<");
        }
    }

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
                strncpy(resp, strtok(NULL, ","), 5);    /* 1= fatal error   */
                rc = atoi(resp);
                memset(&resp, 0, sizeof(resp));
                strncpy(resp, cldecode(strtok(NULL, ";")), 80);  /* The message          */
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
    snprintf(buf, 200, "CCLO,1,%d;", mypid);
    if (socket_send(buf) == 0) {
        strcpy(buf, socket_receive());
    }
    sleep(1);
}



int main(int argc, char *argv[])
{
    struct passwd   *pw;
    struct winsize  ws;
    char    buf[128];

    /*
     * Find out who is on the keyboard or automated the keyboard.
     */
    pw = getpwuid(geteuid());
    if (strcmp(pw->pw_name, (char *)"ftnd")) {
        printf("ERROR: only user \"ftnd\" may use this program\n");
        exit(FTNERR_INIT_ERROR);
    }

    /*
     * Read the global configuration data, registrate connection
     */
    InitConfig();
    InitClient(pw->pw_name, (char *)"ftnmon", CFG.location, (char *)"ftnmon.log", CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);
    Syslog(' ', "FTNMON Started by %s", pw->pw_name);
    bbs_free = FALSE;

    /*
     * Report sysop available for chat
     */
    snprintf(buf, 128, "CSYS:2,%d,1;", mypid);
    if (socket_send(buf) == 0)
        snprintf(buf, 128, "%s", socket_receive());


    /*
     * Setup several signals so when the program terminate's it
     * will properly close.
     */
    signal(SIGINT, (void (*))die);
    signal(SIGBUS, (void (*))die);
    signal(SIGSEGV,(void (*))die);
    signal(SIGTERM,(void (*))die);
    signal(SIGKILL,(void (*))die);
    signal(SIGIOT,(void (*))die);

    if (ioctl(1, TIOCGWINSZ, &ws) != -1 && (ws.ws_col > 0) && (ws.ws_row > 0)) {
        rows = ws.ws_row;
    if (rows < 24) {
        Syslog('!', "Warning, only %d screen rows, forcing to 24", rows);
        rows = 24;
    }
    }
    Syslog('+', "Screen size set to %dx%d", cols, rows);

    if (lockprogram((char *)"ftnmon")) {
        printf("\n\7Another ftnmon is already running, abort.\n\n");
        die(FTNERR_NO_PROGLOCK);
    }

    screen_start((char *)"FTNmon");

    for (;;) {

        IsDoing("Browsing Menu");
        clr_index();
        set_color(WHITE, BLACK);
        ftnd_mvprintw( 5, 6, "0.    FTNd MONITOR");
        set_color(CYAN, BLACK);
        ftnd_mvprintw( 7, 6, "1.    View Server Clients");
        ftnd_mvprintw( 8, 6, "2.    View Server Statistics");
        ftnd_mvprintw( 9, 6, "3.    View Filesystem Usage");
        ftnd_mvprintw(10, 6, "4.    View BBS System Information");
        ftnd_mvprintw(11, 6, "5.    View BBS Lastcallers List");
        ftnd_mvprintw(12, 6, "6.    Chat with any user");
        ftnd_mvprintw(13, 6, "7.    Respond to sysop page");
        ftnd_mvprintw(14, 6, "8.    View Software Information");

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

