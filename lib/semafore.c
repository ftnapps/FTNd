/*****************************************************************************
 *
 * File ..................: semafore.c
 * Purpose ...............: Create, test and remove semafore's
 * Last modification date : 18-Mar-2000
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
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
#include "libs.h"
#include "structs.h"
#include "clcomm.h"
#include "common.h"


void CreateSema(char *sem)
{
	char	temp[40];

	sprintf(temp, "%s", SockR("SECR:1,%s;", sem));
	if (strncmp(temp, "200", 3) == 0)
		WriteError("Can't create semafore %s", sem);
}



void RemoveSema(char *sem)
{
        char    temp[40];

        sprintf(temp, "%s", SockR("SERM:1,%s;", sem));
        if (strncmp(temp, "200", 3) == 0)
                WriteError("Can't remove semafore %s", sem);
}



int IsSema(char *sem)
{
        char    temp[40];

        sprintf(temp, "%s", SockR("SEST:1,%s;", sem));
        if (strncmp(temp, "200", 3) == 0) {
                WriteError("Can't read semafore %s", sem);
		return FALSE;
	}
	strtok(temp, ",");
	return atoi(strtok(NULL, ";"));
}


