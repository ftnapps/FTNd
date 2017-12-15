/*****************************************************************************
 *
 * pinfo.c
 * Purpose ...............: Product information
 *
 *****************************************************************************
 * Copyright (C) 2013-2017 Robert James Clay <jame@rocasa.us>
 * Copyright (C) 1997-2007 Michiel Broek <mbse@mbse.eu>
 *
 * This file is part of FTNd.
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2, or (at your option) any later
 * version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with FTNd; see the file COPYING.  If not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/ftndlib.h"
#include "../lib/ftnd.h"
#include "../lib/users.h"
#include "pinfo.h"
#include "input.h"
#include "term.h"
#include "ttyio.h"


char	pstr[256];


void ls(void)
{
    strcpy(pstr, (char *)"\xB3");
}



void rs(void)
{
    strncat(pstr, colour_str(DARKGRAY, BLACK), 255);
    strncat(pstr, (char *)"\xB3\r\n", 255);
}



void wl(void)
{
    int	i;

    ls();
    for(i = 0; i < 76; i++)
	strncat(pstr, (char *)" ", 255);
    rs();
}



/*
 * Product information screen
 */
void cr(void)
{
    char    *temp;

    temp       = calloc(81, sizeof(char));

    if (utf8)
	chartran_init((char *)"CP437", (char *)"UTF-8", 'B');

    strncpy(pstr, clear_str(), 255);
    strncat(pstr, colour_str(DARKGRAY, BLACK), 255);

    /* Print top row */
    strncat(pstr, (char *)"\xDA", 255);
    strncat(pstr, hLine_str(76), 255);
    strncat(pstr, (char *)"\xBF\r\n", 255);
    PUTSTR(chartran(pstr));

    wl();
    PUTSTR(chartran(pstr));

    ls();
    snprintf(temp, 80, "FTND Bulletin Board System %s (%s-%s)", VERSION, OsName(), OsCPU());
    strncat(pstr, pout_str(YELLOW, BLACK, padleft(temp, 76, ' ')), 255);
    rs();
    PUTSTR(chartran(pstr));

    wl();
    PUTSTR(chartran(pstr));

    ls();
    snprintf(temp, 81, "%s", COPYRIGHT);
    strncat(pstr, pout_str(LIGHTCYAN, BLACK, padleft(temp, 76, ' ')), 255);
    rs();
    PUTSTR(chartran(pstr));

    wl();
    PUTSTR(chartran(pstr));

    ls();
    snprintf(temp, 81, "Compiled on %s at %s", __DATE__, __TIME__);
    strncat(pstr, pout_str(LIGHTRED, BLACK, padleft(temp, 76, ' ')), 255);
    rs();
    PUTSTR(chartran(pstr));

    wl();
    PUTSTR(chartran(pstr));

    ls();
    strncat(pstr, pout_str(LIGHTCYAN, BLACK, (char *)"FTNd and ftnbbs was originally derived from MBSE BBS."), 255);
    rs();
    PUTSTR(chartran(pstr));

    ls();
    strncat(pstr, pout_str(LIGHTCYAN, BLACK, (char *)"MBSE was written and designed by Michiel Broek. Many others gave "), 255);
    rs();
    PUTSTR(chartran(pstr));

    ls();
    strncat(pstr, pout_str(LIGHTCYAN, BLACK, (char *)"valuable time in the form of new ideas and suggestions on how to make MBSE  "), 255);
    rs();
    PUTSTR(chartran(pstr));

    ls();
    strncat(pstr, pout_str(LIGHTCYAN, BLACK, (char *)"BBS a better BBS, which has also extended to the FTNd development         "), 255);
    rs();
    PUTSTR(chartran(pstr));
    
    wl();
    PUTSTR(chartran(pstr));

    ls();
    strncat(pstr, pout_str(WHITE, BLACK, (char *)"Available from https://ftn.rocasa.net or 1:120/544                             "), 255);
    rs();
    PUTSTR(chartran(pstr));

    wl();
    PUTSTR(chartran(pstr));

    ls();
    strncat(pstr, pout_str(LIGHTRED, BLACK, (char *)"JAM(mbp) - Copyright 1993 Joaquim Homrighausen, Andrew Milner,              "),
 255);
    rs();
    PUTSTR(chartran(pstr));

    ls();
    strncat(pstr, pout_str(LIGHTRED, BLACK, (char *)"                          Mats Birch, Mats Wallin.                          "), 255);
    rs();
    PUTSTR(chartran(pstr));

    ls();
    strncat(pstr, pout_str(LIGHTRED, BLACK, (char *)"                          ALL RIGHTS RESERVED.                              "), 255);
    rs();
    PUTSTR(chartran(pstr));

    wl();
    PUTSTR(chartran(pstr));

    ls();
    strncat(pstr, pout_str(LIGHTBLUE, BLACK,  (char *)"This is free software; released under the terms of the GNU General Public   "), 255);
    rs();
    PUTSTR(chartran(pstr));

    ls();
    strncat(pstr, pout_str(LIGHTBLUE, BLACK,  (char *)"License as published by the Free Software Foundation.                       "), 255);
    rs();
    PUTSTR(chartran(pstr));

    wl();
    PUTSTR(chartran(pstr));

    strcpy(pstr, (char *)"\xC0");
    strncat(pstr, hLine_str(76), 255);
    strncat(pstr, (char *)"\xD9\r\n", 255);
    PUTSTR(chartran(pstr));

    free(temp);
    chartran_close();
    Enter(1);
    Pause();
}


