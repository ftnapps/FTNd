/*****************************************************************************
 *
 * $Id: term.c,v 1.13 2005/08/28 13:34:43 mbse Exp $
 * Purpose ...............: Terminal output routines.
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/


#include "../config.h"
#include "mbselib.h"
#include "users.h"


int termmode;			/* 0 = tty, 1 = ANSI			   */
int termx = 80;
int termy = 24;


void mbse_TermInit(int mode, int x, int y)
{
    termmode = mode;
    termx = x;
    termy = y;
}



/*
 * Changes ansi background and foreground color
 */
void mbse_colour(int fg, int bg)
{
    if (termmode == 1) {
  
	int att=0, fore=37, back=40;

	if (fg<0 || fg>31 || bg<0 || bg>7) {
	    fprintf(stdout, "ANSI: Illegal colour specified: %i, %i\n", fg, bg);
	    fflush(stdout);
	    return; 
	}

	fprintf(stdout, "[");
	if ( fg > WHITE) {
	    fprintf(stdout, "5;");
	    fg-= 16;
	}
	if (fg > LIGHTGRAY) {
	    att=1;
	    fg=fg-8;
	}

	if      (fg == BLACK)   fore=30;
	else if (fg == BLUE)    fore=34;
	else if (fg == GREEN)   fore=32;
	else if (fg == CYAN)    fore=36;
	else if (fg == RED)     fore=31;
	else if (fg == MAGENTA) fore=35;
	else if (fg == BROWN)   fore=33;
	else                    fore=37;

	if      (bg == BLUE)      back=44;
	else if (bg == GREEN)     back=42;
	else if (bg == CYAN)      back=46;
	else if (bg == RED)       back=41;
	else if (bg == MAGENTA)   back=45;
	else if (bg == BROWN)     back=43;
	else if (bg == LIGHTGRAY) back=47;
	else                      back=40;
		
	fprintf(stdout, "%d;%d;%dm", att, fore, back);
	fflush(stdout);
    }
}



void mbse_clear()
{
    if (termmode == 1) {
	mbse_colour(LIGHTGRAY, BLACK);
	fprintf(stdout, ANSI_HOME);
	fprintf(stdout, ANSI_CLEAR);
    } else {
	fprintf(stdout, "\n");
    }
    fflush(stdout);
}



/*
 * Moves cursor to specified position
 */
void mbse_locate(int y, int x)
{
    if (termmode > 0) {
	if (y > termy || x > termx) {
	    fprintf(stdout, "ANSI: Invalid screen coordinates: %i, %i\n", y, x);
	    fflush(stdout);
	    return; 
	}
	fprintf(stdout, "\x1B[%i;%iH", y, x);
	fflush(stdout);
    }
}



/*
 * curses compatible functions
 */
void mbse_mvprintw(int y, int x, const char *format, ...)
{
    char	*outputstr;
    va_list	va_ptr;

    outputstr = calloc(2048, sizeof(char));

    va_start(va_ptr, format);
    vsnprintf(outputstr, 2048, format, va_ptr);
    va_end(va_ptr);

    mbse_locate(y, x);
    fprintf(stdout, outputstr);
    free(outputstr);
    fflush(stdout);
}


