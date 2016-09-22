/*****************************************************************************
 *
 * openport.c 
 * File ..................: ftnnntp/openport.c
 *
 *****************************************************************************
 * Copyright (C) 1997-2005  Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2012    Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2, or (at your option) any
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

