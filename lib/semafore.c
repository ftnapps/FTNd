/*****************************************************************************
 *
 * semafore.c
 * Purpose ...............: Create, test and remove semafore's
 *
 *****************************************************************************
 * Copyright (C) 1997-2005 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
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
#include "ftndlib.h"


void CreateSema(char *sem)
{
	char	temp[40];

	snprintf(temp, 40, "%s", SockR("SECR:1,%s;", sem));
	if (strncmp(temp, "200", 3) == 0)
		WriteError("Can't create semafore %s", sem);
}



void RemoveSema(char *sem)
{
        char    temp[40];

        snprintf(temp, 40, "%s", SockR("SERM:1,%s;", sem));
        if (strncmp(temp, "200", 3) == 0)
                WriteError("Can't remove semafore %s", sem);
}



int IsSema(char *sem)
{
        char    temp[40];

        snprintf(temp, 40, "%s", SockR("SEST:1,%s;", sem));
        if (strncmp(temp, "200", 3) == 0) {
                WriteError("Can't read semafore %s", sem);
		return FALSE;
	}
	strtok(temp, ",");
	return atoi(strtok(NULL, ";"));
}


