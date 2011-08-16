/*****************************************************************************
 *
 * $Id: mib.c,v 1.1 2008/02/12 19:59:45 mbse Exp $
 * Purpose ...............: snmp MIB counters
 *
 *****************************************************************************
 * Copyright (C) 1997-2008 
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
#include "mib.h"


unsigned int    mib_sessions = 0;
unsigned int    mib_minutes = 0;
unsigned int    mib_posted = 0;
unsigned int    mib_uploads = 0;
unsigned int    mib_kbupload = 0;
unsigned int    mib_downloads = 0;
unsigned int    mib_kbdownload = 0;
unsigned int    mib_chats = 0;
unsigned int    mib_chatminutes = 0;



void sendmibs(void)
{
    SockS("MSBB:9,%d,%d,%d,%d,%d,%d,%d,%d,%d;", mib_sessions, mib_minutes, mib_posted,
	    mib_uploads, mib_kbupload, mib_downloads, mib_kbdownload, mib_chats, mib_chatminutes);
}



