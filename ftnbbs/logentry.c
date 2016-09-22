/*****************************************************************************
 *
 * logentry.c
 * Purpose ...............: Make a log entry
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
#include "../lib/ftndlib.h"
#include "../lib/ftnd.h"
#include "../lib/users.h"
#include "logentry.h"



/*
 * Function will make log entry in users logfile
 * Understands @ for Fileareas and ^ for Message Areas
 */
void LogEntry(char *Log)
{
	char *Entry, *temp;
	int i;

	Entry = calloc(256, sizeof(char));
	temp  = calloc(1, sizeof(char));

	for(i = 0; i < strlen(Log); i++) {
		if(*(Log + i) == '@')
			strcat(Entry, sAreaDesc);
		else 
			if(*(Log + i) == '^')
				strcat(Entry, sMsgAreaDesc);
			else {
				snprintf(temp, 1, "%c", *(Log + i));
				strcat(Entry, temp);
			}
	}

	Syslog('+', Entry);
	free(Entry);
	free(temp);
}


