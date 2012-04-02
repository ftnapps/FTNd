/*****************************************************************************
 *
 * $Id: screen.c,v 1.16 2004/10/27 11:08:16 mbse Exp $
 * Purpose ...............: Screen functions for setup. 
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
#include "screen.h"


extern int  init;
int	    bbs_free;



/*************************************************************************
 *
 *  Global section
 */


void clrtoeol()
{
    int	i;

    fprintf(stdout, "\r");
    for (i = 0; i < COLS; i++)
	fputc(' ', stdout);
    fprintf(stdout, "\r");
    fflush(stdout);
}



void hor_lin(int y, int x, int len)
{
    int	i;

    mbse_locate(y, x);
    for (i = 0; i < len; i++)
	fputc('-', stdout);
    fflush(stdout);
}



static int	old_f = -1;
static int	old_b = -1;

void set_color(int f, int b)
{
    f = f & 15;
    b = b & 7;

    if ((f != old_f) || (b != old_b)) {
	old_f = f;
	old_b = b;
	mbse_colour(f, b);
	fflush(stdout);
    }
}



static time_t lasttime;

/*
 * Show the current date & time in the second status row
 */
void show_date(int fg, int bg, int y, int x)
{
    time_t	now;
    char	*p, buf[128];

    now = time(NULL);
    if (now != lasttime) {
	lasttime = now;
	set_color(LIGHTGREEN, BLUE);
	p = ctime(&now);
	Striplf(p);
	mbse_mvprintw(1, 44, (char *)"%s TZUTC %s", p, gmtoffset(now)); 
	p = asctime(gmtime(&now));
	Striplf(p);
	mbse_mvprintw(2, 44, (char *)"%s UTC", p);

	/*
	 * Indicator if bbs is free
	 */
	strcpy(buf, SockR("SFRE:0;"));
	if (strncmp(buf, "100:0;", 6) == 0) {
	    strcpy(buf, SockR("SBBS:0;"));
	    if (strncmp(buf, "100:2,1", 7) == 0) {
		set_color(WHITE, RED);
		mbse_mvprintw(2,74, (char *)" Down ");
	    } else {
		set_color(WHITE, BLUE);
		mbse_mvprintw(2,74, (char *)" Free ");
	    }
	    bbs_free = TRUE;
	} else {
	    set_color(WHITE, RED);
	    mbse_mvprintw(2,74, (char *)" Busy ");
	    bbs_free = FALSE;
	}

	if (y && x)
	    mbse_locate(y, x);
	set_color(fg, bg);
    }
}



void center_addstr(int y, char *s)
{
    mbse_mvprintw(y, (COLS / 2) - (strlen(s) / 2), s);
}



/*
 * Curses and screen initialisation.
 */
void screen_start(char *name)
{
    int	i;

    mbse_TermInit(1, COLS, LINES);
    /*
     *  Overwrite screen the first time, if user had it black on white
     *  it will change to white on black. clear() won't do the trick.
     */
    set_color(LIGHTGRAY, BLUE);
    mbse_locate(1, 1);
    for (i = 0; i < LINES; i++) {
	if (i == 3)
	    mbse_colour(LIGHTGRAY, BLACK);
	clrtoeol();
	if (i < LINES)
	    fprintf(stdout, "\n");
    }
    fflush(stdout);

    set_color(WHITE, BLUE);
    mbse_locate(1, 1);
    printf((char *)"%s for MBSE BBS version %s", name, VERSION);  
    set_color(YELLOW, BLUE);
    mbse_locate(2, 1);
    printf((char *)SHORTRIGHT);
    set_color(LIGHTGRAY, BLACK);
    show_date(LIGHTGRAY, BLACK, 0, 0);
    fflush(stdout);
}



/*
 * Screen deinit
 */
void screen_stop()
{
    set_color(LIGHTGRAY, BLACK);
    mbse_clear();
    fflush(stdout);
}



/*
 * Message at the upperright window about actions 
 */
void working(int txno, int y, int x)
{
    int	i;

    if (init)
	return;

    /*
     * If txno not 0 there will be something written. The 
     * reversed attributes for mono, or white on red for
     * color screens is set. The cursor is turned off and 
     * original cursor position is saved.
     */
    show_date(LIGHTGRAY, BLACK, 0, 0);

    if (txno != 0)
	set_color(YELLOW, RED);
    else
	set_color(LIGHTGRAY, BLACK);

    switch (txno) {
	case 0: mbse_mvprintw(4, 66, (char *)"             ");
		break;
	case 1: mbse_mvprintw(4, 66, (char *)"Working . . .");
		break;
	case 2:	mbse_mvprintw(4, 66, (char *)">>> ERROR <<<");
		for (i = 1; i <= 5; i++) {
		    fputc(7, stdout);
		    fflush(stdout);
		    msleep(150);
		}
		msleep(550);
		break;
	case 3: mbse_mvprintw(4, 66, (char *)"Form inserted");
		fputc(7, stdout);
		fflush(stdout);
		sleep(1);
		break;
	case 4: mbse_mvprintw(4, 66, (char *)"Form deleted ");
		fputc(7, stdout);
		fflush(stdout);
		sleep(1);
		break;
	case 5: mbse_mvprintw(4, 66, (char *)"Moving . . . ");
		break;
	case 6:	mbse_mvprintw(4, 66, (char *)"Data updated ");
		fputc(7, stdout);
		fflush(stdout);
		sleep(1);
		break;
    }

    show_date(LIGHTGRAY, BLACK, 0, 0);
    set_color(LIGHTGRAY, BLACK);
    if (y && x)
	mbse_locate(y, x);
    fflush(stdout);
}



/*
 *  Clear the middle window 
 */
void clr_index()
{
    int i;

    set_color(LIGHTGRAY, BLACK);
    for (i = 3; i <= (LINES - 1); i++) {
	mbse_locate(i, 1);
	clrtoeol();
    }
}



/*
 * Show help at the bottom of the screen.
 */
void showhelp(char *T)
{
    int f, i, x, forlim;

    f = FALSE;
    mbse_locate(LINES-1, 1);
    set_color(WHITE, RED);
    clrtoeol();
    x = 0;
    forlim = strlen(T);

    for (i = 0; i < forlim; i++) {
	if (T[i] == '^') {
	    if (f == FALSE) {
		f = TRUE;
		set_color(YELLOW, RED);
	    } else {
		f = FALSE;
		set_color(WHITE, RED);
	    }
	} else {
	    fputc(T[i], stdout);
	    x++;
	}
    }
    set_color(LIGHTGRAY, BLACK);
    fflush(stdout);
}


