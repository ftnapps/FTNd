/*****************************************************************************
 *
 * $Id: mutil.c,v 1.21 2005/10/17 18:02:00 mbse Exp $
 * Purpose ...............: Utilities
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "mutil.h"


extern int  rows, cols;
extern int  ttyfd;
int	    bbs_free;


unsigned char readkey(int y, int x, int fg, int bg)
{
    int		rc = -1, i;
    unsigned char 	ch = 0;

    if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
	perror("open /dev/tty");
	exit(MBERR_TTYIO_ERROR);
    }
    mbse_Setraw();

    i = 0;
    while (rc == -1) {
	if ((i % 10) == 0)
	    show_date(fg, bg, 0, 0);

	mbse_locate(y, x);
	fflush(stdout);
        rc = mbse_Waitchar(&ch, 5);
        if ((rc == 1) && (ch != KEY_ESCAPE))
	   break;

	if ((rc == 1) && (ch == KEY_ESCAPE))
	    rc = mbse_Escapechar(&ch);

	if (rc == 1)
	    break;
	i++;
	Nopper();
    }

    mbse_Unsetraw();
    close(ttyfd);

    return ch;
}



unsigned char testkey(int y, int x)
{
	int		rc;
	unsigned char	ch = 0;

	Nopper();
	mbse_locate(y, x);
	fflush(stdout);

	if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		perror("open /dev/tty");
		exit(MBERR_TTYIO_ERROR);
	}
	mbse_Setraw();

	rc = mbse_Waitchar(&ch, 50);
	if (rc == 1) {
		if (ch == KEY_ESCAPE)
			rc = mbse_Escapechar(&ch);
	}

	mbse_Unsetraw();
	close(ttyfd);

	if (rc == 1)
		return ch;
	else
		return '\0';
}



void show_field(int y, int x, char *str, int length, int fill)
{
    mbse_mvprintw(y, x, padleft(str, length, fill));
}


int insertflag = 0;

void newinsert(int i, int fg, int bg)
{
    insertflag = i;
    set_color(YELLOW, RED);
    if (insertflag != 0) {
	mbse_mvprintw(2,36," INS ");
    } else {
	mbse_mvprintw(2,36," OVR ");
    }
    set_color(fg, bg);
}




char *edit_field(int y, int x, int w, int p, char *s_)
{
	int		i, charok, first, curpos;
	static char	s[256];
	unsigned int	ch;

	memset((char *)s, 0, 256);
	snprintf(s, 256, "%s", s_);
	curpos = 0;
	first = 1;
	newinsert(1, YELLOW, BLUE);

	do {
		set_color(YELLOW, BLUE);
		show_field(y, x, s, w, '_');
		mbse_locate(y, x + curpos);
		do {
			ch = readkey(y, x + curpos, YELLOW, BLUE);
			set_color(YELLOW, BLUE);

			/*
			 *  Test if the pressed key is a valid key.
			 */
			charok = 0;
			if ((ch >= ' ') && (ch <= '~')) {
				switch(p) {
				case '!':	
					ch = toupper(ch);
					charok = 1;
					break;
				case 'X':
					charok = 1;
					break;
				case '9':
					if (ch == ' ' || ch == '-' || ch == ',' ||
					    ch == '.' || isdigit(ch)) 
						charok = 1;
					break;
				case 'U':
					ch = toupper(ch);
					if (isupper(ch))
						charok = 1;
					break;
				default:
					putchar(7);
					break;
				}
			}

		} while (charok == 0 && ch != KEY_ENTER && ch != KEY_LINEFEED &&
			   ch != KEY_DEL && ch != KEY_INS && ch != KEY_HOME &&
			   ch != KEY_LEFT && ch != KEY_RIGHT && ch != KEY_ESCAPE &&
			   ch != KEY_BACKSPACE && ch != KEY_RUBOUT && ch != KEY_END);


		if (charok == 1) {
			if (first == 1) {
				first = 0;
				memset((char *)s, 0, 256);
				curpos = 0;
			}
			if (curpos < w) {
				if (insertflag == 1) {
					/*
					 *  Insert mode 
					 */
					if (strlen(s) < w) {
						if (curpos < strlen(s)) {
							for (i = strlen(s); i >= curpos; i--)
								s[i+1] = s[i];
						} 
						s[curpos] = ch;
						if (curpos < w)
							curpos++;
					} else {
						putchar(7);
					}
				} else {
					/*
					 *  Overwrite mode
					 */
					s[curpos] = ch;
					if (curpos < w)
						curpos++;
				}
			} else {
				/*
				 *  The field is full 
				 */
				putchar(7);
			}
		} /* if charok */

		first = 0;
		switch (ch) {
		case KEY_HOME:
				curpos = 0;	
				break;
		case KEY_END:
				curpos = strlen(s);
				break;
		case KEY_LEFT:
				if (curpos > 0)
					curpos--;
				else
					putchar(7);
				break;
		case KEY_RIGHT:
				if (curpos < strlen(s))
					curpos++;
				else
					putchar(7);
				break;
		case KEY_INS:
				if (insertflag == 1)
					newinsert(0, YELLOW, BLUE);
				else
					newinsert(1, YELLOW, BLUE);
				break;
		case KEY_BACKSPACE:
		case KEY_RUBOUT:
				if (strlen(s) > 0) {
					if (curpos >= strlen(s)) {
						curpos--;
						s[curpos] = '\0';
					} else {
						for (i = curpos; i < strlen(s); i++)
							s[i] = s[i+1];
						s[i] = '\0';
					}
				} else
					putchar(7);
				break;
		case KEY_DEL:
				if (strlen(s) > 0) {
					if ((curpos) == (strlen(s) -1)) {
						s[curpos] = '\0';
					} else {
						for (i = curpos; i < strlen(s); i++)
							s[i] = s[i+1];
						s[i] = '\0';
					}
				} else
					putchar(7);
				break;
		}
	} while ((ch != KEY_ENTER) && (ch != KEY_LINEFEED) && (ch != KEY_ESCAPE));

	set_color(LIGHTGRAY, BLUE);
	mbse_mvprintw(2,36, "     ");
	set_color(LIGHTGRAY, BLACK);
	return s;
}



/*
 * Select menu, max is the highest item to pick. Returns zero if
 * "-" (previous level) is selected.
 */
int select_menu(int max)
{
    static char	*menu=(char *)"-";
    char	help[80];
    int		pick;

    snprintf(help, 80, "Select menu item (1..%d) or ^\"-\"^ for previous level.", max);
    showhelp(help);

    /* 
     * Loop forever until it's right.
     */
    for (;;) {
	mbse_mvprintw(rows - 2, 6, "Enter your choice >");
	menu = (char *)"-";
	menu = edit_field(rows - 2, 26, 3, '9', menu);
	mbse_locate(rows - 2, 6);
	clrtoeol();

	if (strncmp(menu, "-", 1) == 0) 
	    return 0;

	pick = atoi(menu);
	if ((pick >= 1) && (pick <= max)) 
	    return pick;

	working(2, 0, 0);
	working(0, 0, 0);
    }
}



void clrtoeol()
{
    int	i;

    printf("\r");
    for (i = 0; i < cols; i++)
	putchar(' ');
    printf("\r");
    fflush(stdout);
}



void hor_lin(int y, int x, int len)
{
    int	i;

    mbse_locate(y, x);
    for (i = 0; i < len; i++)
	putchar('-');
    fflush(stdout);
}



static int	old_f = -1;
static int	old_b = -1;

void set_color(int f, int b)
{
    if ((f != old_f) || (b != old_b)) {
	old_f = f;
	old_b = b;
	mbse_colour(f, b);
	fflush(stdout);
    }
}



static time_t lasttime;

/*
 *  Show the current date & time in the second status row.
 *  Show user paging status in third screen row.
 */
void show_date(int fg, int bg, int y, int x)
{
    time_t	now;
    char	*p, buf[128], *pid, *page, *reason;

    now = time(NULL);
    if (now != lasttime) {
	lasttime = now;
	set_color(LIGHTGREEN, BLUE);
	p = ctime(&now);
	Striplf(p);
	mbse_mvprintw(1, cols - 36, (char *)"%s TZUTC %s", p, gmtoffset(now)); 
	p = asctime(gmtime(&now));
	Striplf(p);
	mbse_mvprintw(2, cols - 36, (char *)"%s UTC", p);

	/*
	 * Indicator if bbs is free
	 */
	strcpy(buf, SockR("SFRE:0;"));
	if (strncmp(buf, "100:0;", 6) == 0) {
	    strcpy(buf, SockR("SBBS:0;"));
	    if (strncmp(buf, "100:2,1", 7) == 0) {
		set_color(WHITE, RED);
		mbse_mvprintw(2,cols - 6, (char *)" Down ");
	    } else {
		set_color(WHITE, BLUE);
		mbse_mvprintw(2,cols - 6, (char *)" Free ");
	    }
	    bbs_free = TRUE;
	} else {
	    set_color(WHITE, RED);
	    mbse_mvprintw(2,cols - 6, (char *)" Busy ");
	    bbs_free = FALSE;
	}

	/*
	 * Check paging status
	 */
	strcpy(buf, SockR("CCKP:0;"));
	if (strcmp(buf, "100:0;") == 0) {
	    mbse_locate(3, 1);
	    set_color(LIGHTGRAY, BLACK);
	    clrtoeol();
	} else {
	    pid = strtok(buf, ",");
	    pid = strtok(NULL, ",");
	    page = strtok(NULL, ",");
	    reason = xstrcpy(cldecode(strtok(NULL, ";")));
	    if (strlen(reason) > 60)
		reason[60] = '\0';

	    mbse_locate(3, 1);
	    if (strcmp(page, "1")) {
		set_color(RED, BLACK);
		mbse_mvprintw(3, 1, "   Old page (%s) %-60s", pid, reason);
		if ((now % 10) == 0)	/* Every 10 seconds */
		    putchar(7);
	    } else {
		set_color(LIGHTRED, BLACK);
		mbse_mvprintw(3, 1, " Sysop page (%s) %-60s", pid, reason);
		putchar(7);		/* Each second	    */
	    }
	    free(reason);
	}

	if (y && x)
	    mbse_locate(y, x);
	set_color(fg, bg);
    }
}



void center_addstr(int y, char *s)
{
    mbse_mvprintw(y, (cols / 2) - (strlen(s) / 2), s);
}



/*
 * Curses and screen initialisation.
 */
void screen_start(char *name)
{
    int	i;

    mbse_TermInit(1, cols, rows);
    /*
     *  Overwrite screen the first time, if user had it black on white
     *  it will change to white on black. clear() won't do the trick.
     */
    set_color(LIGHTGRAY, BLUE);
    mbse_locate(1, 1);
    for (i = 0; i < rows; i++) {
	if (i == 3)
	    mbse_colour(LIGHTGRAY, BLACK);
	clrtoeol();
	if (i < rows)
	    printf("\n");
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
	case 0: mbse_mvprintw(4, cols - 14, (char *)"             ");
		break;
	case 1: mbse_mvprintw(4, cols - 14, (char *)"Working . . .");
		break;
	case 2:	mbse_mvprintw(4, cols - 14, (char *)">>> ERROR <<<");
		for (i = 1; i <= 5; i++) {
		    putchar(7);
		    fflush(stdout);
		    msleep(150);
		}
		msleep(550);
		break;
	case 3: mbse_mvprintw(4, cols - 14, (char *)"Form inserted");
		putchar(7);
		fflush(stdout);
		sleep(1);
		break;
	case 4: mbse_mvprintw(4, cols - 14, (char *)"Form deleted ");
		putchar(7);
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
    for (i = 4; i <= (rows); i++) {
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
    mbse_locate(rows, 1);
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
	    putchar(T[i]);
	    x++;
	}
    }
    set_color(LIGHTGRAY, BLACK);
    fflush(stdout);
}


