/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Monitor Program 
 * Todo ..................: Chat with user via server
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "common.h"
#include "mutil.h"


extern int  bbs_free;


static void die(int onsig)
{
	signal(onsig, SIG_IGN);
	screen_stop(); 
	if (onsig && (onsig <= NSIG))
		Syslog('?', "Finished on signal %s", SigName[onsig]);
	else
		Syslog(' ', "Normally finished");
	ExitClient(0);
}



void ShowSysinfo(void)
{
	int	ch;
	char	buf[128], *cnt;

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
	center_addstr(LINES - 4, (char *)"Press any key");
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
		ch = testkey(LINES - 4, COLS / 2 + 8);
	} while (ch == '\0');
}



void ShowLastcaller(void)
{
	int	records, ch, i, y, o;
	char	buf[128], *cnt;
	
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 4, 6, "5.    SHOW BBS LASTCALLERS");
	set_color(YELLOW, RED);
	mvprintw( 6, 1, "Nr Username       Location     Level Device Time  Mins Calls Speed     Actions ");
	set_color(CYAN, BLACK);
	center_addstr(LINES - 4, (char *)"Press any key");
	IsDoing("View Lastcallers");

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
			if (records > 10)
				o = records - 10;
			else
				o = 1;
			set_color(CYAN, BLACK);
			for (i = o; i <= records; i++) {
				sprintf(buf, "GLCR:1,%d;", i);
				if (socket_send(buf) == 0) {
					sprintf(buf, "%s", socket_receive());
					if (strncmp(buf, "100:9,", 6) == 0) {
						cnt = strtok(buf, ",");
						if (records > 10) {
						    /*
						     * Only clear line if there's a change to scroll
						     */
						    locate(y, 1);
						    clrtoeol();
						}
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
		ch = testkey(LINES - 4, COLS / 2 + 8);
	} while (ch == '\0');
}



void system_moni(void)
{
	int ch, y, eof;
	char *cnt;
	char buf[128];
	time_t start, now;

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "1.    SERVER CLIENTS");
	set_color(YELLOW, RED);
	mvprintw( 7, 1, "Pid   tty    user             program  city            doing              time ");
	set_color(CYAN, BLACK);
	center_addstr(LINES - 4, (char *)"Press any key");
	IsDoing("System Monitor");

	do {
		show_date(LIGHTGRAY, BLACK, 0, 0);

		eof = 0;
		set_color(LIGHTGRAY, BLACK);

		for (y = 8; y <= LINES - 5; y++) { 
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
						mvprintw(y,14, (char *)"%.16s", strtok(NULL, ","));
						mvprintw(y,31, (char *)"%.8s", strtok(NULL, ","));
						mvprintw(y,40, (char *)"%.15s", strtok(NULL, ","));
						mvprintw(y,56, (char *)"%.18s", strtok(NULL, ","));
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

		ch = testkey(LINES - 4, COLS / 2 + 8);
	} while (ch == '\0');
}



void system_stat(void)
{
	int	ch;
	char	buf[256];
	char	*cnt;
	time_t	now;

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

		ch = testkey(19,76);
	} while (ch == '\0');
}



void disk_stat(void)
{
	int		ch, i;
	char		buf[1024];
	char		*cnt, *type, *fs, *p;
	unsigned long	last[10];
	unsigned long	size, used, perc;
	char		sign;

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "3.    FILESYSTEM USAGE");
	set_color(YELLOW, RED);
	mvprintw( 7, 1, " Size MB   Used MB     Perc. FS-Type   Mountpoint                             ");
	set_color(CYAN, BLACK);
	mvprintw(LINES - 2, 6, "Press any key");
	IsDoing("Filesystem Usage");

	do {
		show_date(LIGHTGRAY, BLACK, 0, 0);

		sprintf(buf, "GDST:1,%d;", getpid());
		if (socket_send(buf) == 0) {
			strcpy(buf, socket_receive());
			set_color(LIGHTGRAY, BLACK);
			cnt = strtok(buf, ":");
			cnt = strtok(NULL, ",;");
			if (atoi(cnt)) {
				for (i = 0; i < atoi(cnt); i++) {
					p = strtok(NULL, " ");
					size = atoi(p);
					p = strtok(NULL, " ");
					used = size - atoi(p);
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
					mvprintw(i+8, 1, "%8lu  %8lu  ", 
							size, used);
					set_color(WHITE, BLACK);
					printf("%c  ", sign);
					set_color(CYAN, BLACK);
					if (strstr(type, "iso") == NULL) {
						if (perc >= 95)
							set_color(LIGHTRED, BLACK);
						else if (perc >= 80)
							set_color(YELLOW, BLACK);
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

		ch = testkey(LINES - 2, 20);
	} while (ch == '\0');
}



void soft_info(void)
{
	char	temp[81];

	clr_index();
	set_color(YELLOW, BLACK);
#ifdef __linux__
	center_addstr( 6, (char *)"MBSE BBS (Linux)");
#elif __FreeBSD__
	center_addstr( 6, (char *)"MBSE BBS (FreeBSD)");
#elif __NetBSD__
	center_addstr( 6, (char *)"MBSE BBS (NetBSD)");
#else
	center_addstr( 6, (char *)"MBSE BBS (Unknown)");
#endif
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
	center_addstr(14, (char *)"http://mbse.sourceforge.net or 2:280/2802");
	set_color(LIGHTGREEN, BLACK);
	center_addstr(LINES -7, (char *)"This is free software; released under the terms of the GNU General");
	center_addstr(LINES -6, (char *)"Public License as published by the Free Software Foundation.");
	set_color(CYAN, BLACK);
	center_addstr(LINES -4, (char *)"Press any key");
	readkey(LINES - 4, COLS / 2 + 8, LIGHTGRAY, BLACK);
}



int main(int argc, char *argv[])
{
	struct passwd	*pw;

#ifdef MEMWATCH
	mwInit();
#endif

	/*
	 * Find out who is on the keyboard or automated the keyboard.
	 */
	pw = getpwuid(getuid());
	InitClient(pw->pw_name);
	Syslog(' ', "Started by %s", pw->pw_name);
	bbs_free = FALSE;

	
	/*
	 * Setup several signals so when the program terminate's it
	 * will properly close.
	 */
	signal(SIGINT, (void (*))die);
	signal(SIGBUS, (void (*))die);
	signal(SIGSEGV,(void (*))die);
	signal(SIGTERM,(void (*))die);
	signal(SIGKILL,(void (*))die);

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
		mvprintw(12, 6, "6.    View Software Information");

		switch(select_menu(6)) {
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
			soft_info();
			break;
		}
	}
}

