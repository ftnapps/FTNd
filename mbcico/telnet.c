/*****************************************************************************
 *
 * $Id: telnet.c,v 1.6 2004/02/21 17:22:01 mbroek Exp $
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
#include "telnet.h"


/*
 * Telnet I/O filters. See RFC854 for details.
 */



/*
 * Send options to the remote.
 */
void telnet_init(int Fd)
{
    Syslog('s', "telnet_init(%d)", Fd);
    telnet_answer(DO, TOPT_SUPP, Fd);
    telnet_answer(WILL, TOPT_SUPP, Fd);
    telnet_answer(DO, TOPT_BIN, Fd);
    telnet_answer(WILL, TOPT_BIN, Fd);
    telnet_answer(DO, TOPT_ECHO, Fd);
    telnet_answer(WILL, TOPT_ECHO, Fd);
}



/*
 * Answer options requested by remote.
 */
void telnet_answer(int tag, int opt, int Fd)
{
    char    buf[3];
    char    *r = (char *)"???";
	            
    switch (tag) {
	case WILL:  r = (char *)"WILL";
		    break;
	case WONT:  r = (char *)"WONT";
		    break;
	case DO:    r = (char *)"DO";
		    break;
	case DONT:  r = (char *)"DONT";
		    break;
    }
    Syslog('s', "Telnet: send %s %d", r, opt);

    buf[0] = IAC;
    buf[1] = tag;
    buf[2] = opt;
    if (write (Fd, buf, 3) != 3)
	WriteError("$answer cant send");
}



/*
 * Telnet output filter, escape IAC.
 */
void telout_filter(int fdi, int fdo)
{
    int	    rc, c;
    char    ch;

    Syslog('s', "telout_filter: in=%d out=%d", fdi, fdo);

    while ((rc = read(fdi, &ch, 1)) > 0) {
	c = (int)ch & 0xff;
	if (c == IAC) {
	    /*
	     * Escape IAC characters by sending it twice.
	     */
	    rc = write(fdo, &ch, 1);
	}
	if ((rc = write(fdo, &ch, 1)) == -1) {
	    Syslog('s', "$telout_filter: write failed");
	    exit(MBERR_TTYIO_ERROR);
	}
    }

    Syslog('s', "telout_filter: finished rc=%d", rc);
    exit(0);
}



/*
 * Telnet input filter, interpret telnet commands.
 */
void telin_filter(int fdo, int fdi)
{
    int     rc, c, m;
    char    ch;

    Syslog('s', "telin_filter: in=%d out=%d", fdi, fdo);

    while ((rc = read(fdi, &ch, 1)) > 0) {
	c = (int)ch & 0xff;
	if (c == IAC) {

	    if ((read(fdi, &ch, 1) < 0))
		break;
	    m = (int)ch & 0xff;

	    switch (m) {
		case WILL:  read(fdi, &ch, 1);
			    m = (int)ch & 0xff;
			    Syslog('s', "Telnet: recv WILL %d", m);
			    if (m != TOPT_BIN && m != TOPT_SUPP && m != TOPT_ECHO)
				telnet_answer(DONT, m, fdo);
			    break;
		case WONT:  read(fdi, &ch, 1);
			    m = (int)ch & 0xff;
			    Syslog('s', "Telnet: recv WONT %d", m);
			    break;
		case DO:    read(fdi, &ch, 1);
			    m = (int)ch & 0xff;
			    Syslog('s', "Telnet: recv DO %d", m);
			    if (m != TOPT_BIN && m != TOPT_SUPP && m != TOPT_ECHO)
				telnet_answer(WONT, m, fdo);
			    break;
		case DONT:  read(fdi, &ch, 1);
			    m = (int)ch & 0xff;
			    Syslog('s', "Telnet: recv DONT %d", m);
			    break;
		case IAC:   ch = (char)m;
			    rc = write(fdo, &ch, 1);
			    break;
		default:    Syslog('s', "Telnet: recv IAC %d, not good", m);
			    break;
	    }

	} else {
	    ch = (char)c;
	    if ((rc = write(fdo, &ch, 1)) == -1) {
		Syslog('s', "$telin_filter: write failed");
		exit(MBERR_TTYIO_ERROR);
	    }
	}
    }

    
    Syslog('s', "telin_filter: finished rc=%d", rc);
    exit(0);
}

