/*****************************************************************************
 *
 * $Id: lutil.c,v 1.10 2005/10/11 20:49:46 mbse Exp $
 * Purpose ...............: Fidonet mailer
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "lutil.h"


char		*myname=(char *)"unknown";


void setmyname(char *arg)
{
	if ((myname = strrchr(arg, '/'))) 
		myname++;
	else 
		myname = arg;
}



static char *mon[] = {
(char *)"Jan",(char *)"Feb",(char *)"Mar",
(char *)"Apr",(char *)"May",(char *)"Jun",
(char *)"Jul",(char *)"Aug",(char *)"Sep",
(char *)"Oct",(char *)"Nov",(char *)"Dec"
};



char *date(time_t t)
{
	struct tm ptm;
	time_t now;
	static char buf[20];

	if (t) 
		now = t; 
	else 
		now = time(NULL);
	ptm=*localtime(&now);
	snprintf(buf, 20, "%s %02d %02d:%02d:%02d", mon[ptm.tm_mon],ptm.tm_mday,ptm.tm_hour,ptm.tm_min,ptm.tm_sec);
	return(buf);
}



int IsZMH()
{
	static	char buf[81];

	snprintf(buf, 81, "SBBS:0;");
	if (socket_send(buf) == 0) {
		strncpy(buf, socket_receive(), 80);
		if (strncmp(buf, "100:2,2", 7) == 0)
			return TRUE;
	}
	return FALSE;
}


unsigned int rnd (void)
{
    static int	i;

    if (!i) {
	i = 1;
	srand (time (0));
    }
    return (time (0) + rand ()) & 0xFFFFFFFFul;
}

