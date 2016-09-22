/*****************************************************************************
 *
 * ftnftoberep.c
 * Purpose: File Database Maintenance - Show toberep database
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This is free software; you can redistribute it and/or modify it
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
#include "ftnfutil.h"
#include "ftnftoberep.h"



extern int	do_quiet;		/* Suppress screen output	    */



/*
 * Show the toberep database
 */
void ToBeRep(void)
{
    char		*temp;
    FILE		*fp;
    struct _filerecord	rep;

    if (do_quiet)
	return;

    IsDoing("Toberep");

    ftnd_colour(CYAN, BLACK);
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/toberep.data", getenv("FTND_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	printf("No toberep database present\n");
    } else {

	//      12345678901234567890123456789012345678901234567890123456789012345678901234567890
	printf("     File echo           Group       File name    Kbyte     Date     Announce\n");
	printf("--------------------  ------------  ------------  -----  ----------  --------\n");
	ftnd_colour(LIGHTGRAY, BLACK);
	
	while (fread(&rep, sizeof(rep), 1, fp) == 1) {
	    printf("%-20s  %-12s  %-12s  %5d  %s     %s\n", 
		rep.Echo, rep.Group, rep.Name, rep.SizeKb, StrDateDMY(rep.Fdate), rep.Announce ? "Yes":"No ");
	    Syslog('f', "%-20s  %-12s  %-12s  %5d  %s     %s",
		    rep.Echo, rep.Group, rep.Name, rep.SizeKb, StrDateDMY(rep.Fdate), rep.Announce ? "Yes":"No ");
	}

	fclose(fp);
    }

    free(temp);
}


