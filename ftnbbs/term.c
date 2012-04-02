/*****************************************************************************
 *
 * $Id: term.c,v 1.6 2007/08/25 12:49:09 mbse Exp $
 * Purpose ...............: Terminal output routines.
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "term.h"
#include "ttyio.h"


extern int  cols;
extern int  rows;



/*
 * Function will print about of enters specified
 */
void Enter(int num)
{
    int i;

    for (i = 0; i < num; i++) {
	PUTCHAR('\r');
	PUTCHAR('\n');
    }
}



char *pout_str(int fg, int bg, char *Str)
{
    static char	temp[256];

    strncpy(temp, colour_str(fg, bg), 255);
    strncat(temp, Str, 255);
    return temp;
}



void pout(int fg, int bg, char *Str)
{
    PUTSTR(pout_str(fg, bg, Str));
}



char *poutCenter_str(int fg, int bg, char *Str)
{
    static char	temp[256];

    strncpy(temp, colour_str(fg, bg), 255);
    strncat(temp, Center_str(Str), 255);
    return temp;
}



void poutCenter(int fg, int bg, char *Str)
{
    PUTSTR(poutCenter_str(fg, bg, Str));
}



char *poutCR_str(int fg, int bg, char *Str)
{
    static char	temp[256];

    strncpy(temp, colour_str(fg, bg), 255);
    strncat(temp, Str, 255);
    strncat(temp, (char *)"\r\n", 255);
    return temp;
}



void poutCR(int fg, int bg, char *Str)
{
    PUTSTR(poutCR_str(fg, bg, Str));
}



/*
 * Changes ansi background and foreground color
 */
char *colour_str(int fg, int bg)
{
    static char	temp[61];
    char	tmp1[41];
    
    int att = 0, fore = 37, back = 40;
    
    if (fg<0 || fg>31 || bg<0 || bg>7) {
	snprintf(temp, 61, "ANSI: Illegal colour specified: %i, %i\n", fg, bg);
	return temp;
    }

    strcpy(temp, "\x1B[");
    
    if ( fg > WHITE) {
	strcat(temp, (char *)"5;");
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

    snprintf(tmp1, 40, "%d;%d;%dm", att, fore, back);
    strncat(temp, tmp1, 60);
    return temp;
}



void colour(int fg, int bg)
{
    PUTSTR(colour_str(fg, bg));
}



char *Center_str(char *string)
{
    int		Strlen, Maxlen = cols, i, x, z;
    static char	Str[256];

    Strlen = strlen(string);
    if (Maxlen > 255)
	Maxlen = 255;

    if (Strlen == Maxlen)
	strncpy(Str, string, 255);
    else {
	strcpy(Str, (char *)"");
	x = Maxlen - Strlen;
	z = x / 2;
	for (i = 0; i < z; i++)
	    strcat(Str, " ");
	strncat(Str, string, 255);
    }

    strncat(Str, (char *)"\r\n", 255);
    return Str;
}



void Center(char *string)
{
    PUTSTR(Center_str(string));
}



char *clear_str(void)
{
    static char	temp[41];

    strncpy(temp, colour_str(LIGHTGRAY, BLACK), 40);
    strncat(temp, (char *)ANSI_HOME, 40);
    strncat(temp, (char *)ANSI_CLEAR, 40);
    return temp;
}



void clear()
{
    PUTSTR(clear_str());
}



/*
 * Moves cursor to specified position
 */
char *locate_str(int y, int x)
{
    static char	temp[61];
    
    if (y > rows || x > cols) {
	snprintf(temp, 61, "ANSI: Invalid screen coordinates: %i, %i\n", y, x);
    } else {
	snprintf(temp, 61, "\x1B[%i;%iH", y, x);
    }
    return temp;
}



void locate(int y, int x)
{
    PUTSTR(locate_str(y, x));
}



char *hLine_str(int Len)
{
    int		x;
    static char	temp[256];
 
    strcpy(temp, "");   
    for (x = 0; x < Len; x++)
	strncat(temp, (char *)"\xC4", 255);

    return temp;
}



char *fLine_str(int Len)
{
    static char	temp[255];

    strncpy(temp, hLine_str(Len), 255);
    strncat(temp, (char *)"\r\n", 255);
    return temp;
}



void fLine(int Len)
{
    PUTSTR(fLine_str(Len));
}



char *sLine_str(void)
{
    return fLine_str(cols -1);
}



void  sLine()
{
    fLine(cols -1);
}



/*
 * curses compatible functions
 */
void mvprintw(int y, int x, const char *format, ...)
{
    char	*outputstr;
    va_list	va_ptr;

    outputstr = calloc(2048, sizeof(char));

    va_start(va_ptr, format);
    vsnprintf(outputstr, 2048, format, va_ptr);
    va_end(va_ptr);

    locate(y, x);
    PUTSTR(outputstr);
    free(outputstr);
}



