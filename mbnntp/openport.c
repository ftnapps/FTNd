/*****************************************************************************
 *
 * $Id: openport.c,v 1.4 2005/09/12 18:00:26 mbse Exp $
 * File ..................: mbnntp/openport.c
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
#include "ttyio.h"
#include "openport.h"


int	hanged_up = 0;

void    linedrop(int);
void    sigpipe(int);



void linedrop(int sig)
{
    Syslog('+', "openport: Lost Carrier");
    hanged_up=1;
    return;
}



void sigpipe(int sig)
{
    Syslog('+', "openport: Got SIGPIPE");
    hanged_up=1;
    return;
}



int rawport(void)
{
    tty_status = 0;
    signal(SIGHUP, linedrop);
    signal(SIGPIPE, sigpipe);
    return 0;
}



int cookedport(void)
{
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    return 0;
}

