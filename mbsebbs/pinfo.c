/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Product information
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
#include "../lib/libs.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "pinfo.h"
#include "input.h"



void ls(int a)
{
	printf("%c ", a ? 179 : '|');
}



void rs(int a)
{
	colour(8, 0);
	printf("%c\n", a ? 179 : '|');
}



void wl(int a)
{
	int	i;

	ls(a);
	for(i = 0; i < 76; i++)
		printf(" ");
	rs(a);
}



/*
 * Product information screen
 */
void cr(void)
{
	int	a, i;
	char	*string, *temp;

	a = exitinfo.GraphMode;

	string     = calloc(81, sizeof(char));
	temp       = calloc(81, sizeof(char));

	clear();
	colour(8, 0);

	/* Print top row */
	printf("%c", a ? 213 : '+');
	for(i = 0; i < 77; i++)
		printf("%c", a ? 205 : '=');
	printf("%c\n", a ? 184 : '+');

	wl(a);
	ls(a);
	sprintf(temp, "MBSE Bulletin Board System %s (%s-%s)", VERSION, OsName(), OsCPU());
	pout(14, 0, padleft(temp, 76, ' '));
	rs(a);
	wl(a);
	ls(a);
	sprintf(temp, "%s", COPYRIGHT);
	pout(11, 0, padleft(temp, 76, ' '));
	rs(a);
	wl(a);
	ls(a);
	sprintf(temp, "Compiled on %s at %s", __DATE__, __TIME__);
	pout(14, 0, padleft(temp, 76, ' '));
	rs(a);
	wl(a);
	ls(a);
	pout(11, 0, (char *)"MBSE has been written and designed by Michiel Broek. Many others have given ");
	rs(a);
	ls(a);
	pout(11, 0, (char *)"valuable time in the form of new ideas and suggestions on how to make MBSE  ");
	rs(a);
	ls(a);
	pout(11, 0, (char *)"BBS a better BBS                                                            ");
	rs(a);
	wl(a);
	ls(a);
	pout(15, 0, (char *)"Available from http://mbse.sourceforge.net or 2:280/2802                    ");
	rs(a);
	wl(a);
	ls(a);
	pout(12, 0, (char *)"JAM(mbp) - Copyright 1993 Joaquim Homrighausen, Andrew Milner,              ");
	rs(a);
	ls(a);
	pout(12, 0, (char *)"                          Mats Birch, Mats Wallin.                          ");
	rs(a);
	ls(a);
	pout(12, 0, (char *)"                          ALL RIGHTS RESERVED.                              ");
	rs(a);
	wl(a);
	ls(a);
	pout(9, 0,  (char *)"This is free software; released under the terms of the GNU General Public   ");
	rs(a);
	ls(a);
	pout(9, 0,  (char *)"License as published by the Free Software Foundation.                       ");
	rs(a);
	wl(a);

	printf("%c", a ? 212 : '+');
	for(i = 0; i < 77; i++)
		printf("%c", a ? 205 : '=');
  	printf("%c", a ? 190 : '+');
  
	free(string);
	free(temp);
  	printf("\n");
  	Pause();
}


