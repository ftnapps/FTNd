/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - Show toberep database
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "mbfutil.h"
#include "mbftoberep.h"



extern int	do_quiet;		/* Supress screen output	    */



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

    colour(CYAN, BLACK);
    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	printf("No toberep database present\n");
    } else {

	//      12345678901234567890123456789012345678901234567890123456789012345678901234567890
	printf("     File echo           Group       File name    Kbyte     Date     Announce\n");
	printf("--------------------  ------------  ------------  -----  ----------  --------\n");
	colour(LIGHTGRAY, BLACK);
	
	while (fread(&rep, sizeof(rep), 1, fp) == 1) {
	    printf("%-20s  %-12s  %-12s  %5ld  %s     %s\n", 
		rep.Echo, rep.Group, rep.Name, rep.SizeKb, StrDateDMY(rep.Fdate), rep.Announce ? "Yes":"No ");
	}

	fclose(fp);
    }

    free(temp);
}


