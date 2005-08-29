/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Product information
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
#include "pinfo.h"
#include "input.h"
#include "term.h"
#include "ttyio.h"


void ls(int a)
{
    PUTCHAR(a ? 179 : '|');
}



void rs(int a)
{
    colour(DARKGRAY, BLACK);
    PUTCHAR(a ? 179 : '|');
    Enter(1);
}



void wl(int a)
{
    int	i;

    ls(a);
    for(i = 0; i < 76; i++)
	PUTCHAR(' ');
    rs(a);
}



/*
 * Product information screen
 */
void cr(void)
{
    int	    a, i;
    char    *string, *temp;

    a = exitinfo.GraphMode;

    string     = calloc(81, sizeof(char));
    temp       = calloc(81, sizeof(char));

    clear();
    colour(DARKGRAY, BLACK);

    /* Print top row */
    PUTCHAR(a ? 213 : '+');
    for (i = 0; i < 76; i++)
	PUTCHAR(a ? 205 : '=');
    PUTCHAR(a ? 184 : '+');
    Enter(1);

    wl(a);
    ls(a);
    snprintf(temp, 81, "MBSE Bulletin Board System %s (%s-%s)", VERSION, OsName(), OsCPU());
    pout(YELLOW, BLACK, padleft(temp, 76, ' '));
    rs(a);
    wl(a);
    ls(a);
    snprintf(temp, 81, "%s", COPYRIGHT);
    pout(LIGHTCYAN, BLACK, padleft(temp, 76, ' '));
    rs(a);
    wl(a);
    ls(a);
    snprintf(temp, 81, "Compiled on %s at %s", __DATE__, __TIME__);
    pout(LIGHTRED, BLACK, padleft(temp, 76, ' '));
    rs(a);
    wl(a);
    ls(a);
    pout(LIGHTCYAN, BLACK, (char *)"MBSE has been written and designed by Michiel Broek. Many others have given ");
    rs(a);
    ls(a);
    pout(LIGHTCYAN, BLACK, (char *)"valuable time in the form of new ideas and suggestions on how to make MBSE  ");
    rs(a);
    ls(a);
    pout(LIGHTCYAN, BLACK, (char *)"BBS a better BBS                                                            ");
    rs(a);
    wl(a);
    ls(a);
    pout(WHITE, BLACK, (char *)"Available from http://www.mbse.dds.nl or 2:280/2802                         ");
    rs(a);
    wl(a);
    ls(a);
    pout(LIGHTRED, BLACK, (char *)"JAM(mbp) - Copyright 1993 Joaquim Homrighausen, Andrew Milner,              ");
    rs(a);
    ls(a);
    pout(LIGHTRED, BLACK, (char *)"                          Mats Birch, Mats Wallin.                          ");
    rs(a);
    ls(a);
    pout(LIGHTRED, BLACK, (char *)"                          ALL RIGHTS RESERVED.                              ");
    rs(a);
    wl(a);
    ls(a);
    pout(LIGHTBLUE, BLACK,  (char *)"This is free software; released under the terms of the GNU General Public   ");
    rs(a);
    ls(a);
    pout(LIGHTBLUE, BLACK,  (char *)"License as published by the Free Software Foundation.                       ");
    rs(a);
    wl(a);

    PUTCHAR(a ? 212 : '+');
    for (i = 0; i < 76; i++)
	PUTCHAR(a ? 205 : '=');
    PUTCHAR(a ? 190 : '+');
  
    free(string);
    free(temp);
    Enter(1);
    Pause();
}


