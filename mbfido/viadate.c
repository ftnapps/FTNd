/*****************************************************************************
 *
 * File ..................: mbfido/viadate.c
 * Purpose ...............: Create a Via date 
 * Last modification date : 20-Jan-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
 *   
 * Michiel Broek                FIDO:           2:280/2802
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "viadate.h"



static char *months[] = {
	(char *)"Jan", (char *)"Feb", (char *)"Mar",
	(char *)"Apr", (char *)"May", (char *)"Jun",
	(char *)"Jul", (char *)"Aug", (char *)"Sep",
	(char *)"Oct", (char *)"Nov", (char *)"Dec"
};



static char *weekday[] = {
	(char *)"Sun", (char *)"Mon", (char *)"Tue",
	(char *)"Wed", (char *)"Thu", (char *)"Fri",
	(char *)"Sat"
};



char *viadate(void)
{
	static char     buf[64];
	time_t          t;
	struct tm       *ptm;

	time(&t);
	ptm = localtime(&t);
	sprintf(buf,"%s %s %d %d at %02d:%02d", weekday[ptm->tm_wday],months[ptm->tm_mon],
	ptm->tm_mday,ptm->tm_year+1900,ptm->tm_hour,ptm->tm_min);
	return buf;
}

