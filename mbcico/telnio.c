/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Telnet IO filter
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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
#include "../lib/libs.h"
#include "../lib/clcomm.h"
#include "hydra.h"
#include "telnio.h"


static int	buflen = 0;


void telnet_answer(int tag, int opt)
{
    char    buf[3];
    char    *r = (char *)"???";
	
    switch (tag) {
	case WILL:
	    r = (char *)"WILL";
	    break;
	case WONT:
	    r = (char *)"WONT";
	    break;
	case DO:
	    r = (char *)"DO";
	    break;
	case DONT:
	    r = (char *)"DONT";
	    break;
    }
    Syslog('s', "TELNET send %s %d", r, opt);

    buf[0] = IAC;
    buf[1] = tag;
    buf[2] = opt;
    if (write (1, buf, 3) != 3)
	WriteError("$answer cant send");
}



int telnet_init(void)
{
    Syslog('s', "telnet_init()");
    telnet_answer(DO, TOPT_SUPP);
    telnet_answer(WILL, TOPT_SUPP);
    telnet_answer(DO, TOPT_BIN);
    telnet_answer(WILL, TOPT_BIN);
    telnet_answer(DO, TOPT_ECHO);
    telnet_answer(WILL, TOPT_ECHO);
    return 1;
}



/*
 * Telnet output filter, IAC characters are escaped.
 */
int telnet_write(char *buf, int len)
{
    char    *q;
    int	    k, l;
    int	    c = 0;
    
    Syslog('s', "telnet_write(buf, %d)", len);
    l = len;
    while ((len > 0) && (q = memchr(buf, IAC, len))) {
	k = (q - buf) + 1;
	if ((write(1, buf, k) != k) || (write(1, q, 1) != 1)) {
	    return -1;
	}
	c += k;
	c += 1;
	buf += k;
	len -= k;
    }

    c += len;
    if ((len > 0) && write(1, buf, len) != len) {
	return -1;
    }

    Syslog('s', "telnet_write: real sent %d", c);
    return l;
}



/*
 * Process a passed buffer for telnet escapes sequences. Use a temp
 * buffer for sequences at the begin and end of a data block so that
 * series of buffers are processed as if it is one stream.
 */
int telnet_buffer(char *buf, int len)
{
    int		i, j, m = 0, rc;
    static char	telbuf[4];
    
    Syslog('s', "telnet_buffer(buf, %d) buflen=%d", len, buflen);
    rc = len;

    if (buflen > 2) {
	WriteError("buflen=%d");
	buflen = 0;
    }

    /*
     * If from a previous call there are some telnet escape characters left in 
     * telbuf, insert these into the buffer array.
     */
    if (buflen) {
	memmove(buf+buflen, buf, (size_t)len);
	memcpy(buf, telbuf, (size_t)buflen);
	rc += buflen;
	buflen = 0;
    }

    if (memchr (buf, IAC, rc)) {
	j = 0;
	for (i = 0; i < rc; i++) {
	    if ((buf[i] & 0xff) == IAC) {
		telbuf[buflen] = buf[i] & 0xff;
		buflen++;
		i++;
		if (i >= rc)
		    break;
		switch (buf[i] & 0xff) {
		    case WILL:  if ((i+1) < rc) {
				    buflen = 0;
				    i++;
				    m = buf[i] & 0xff;
				    Syslog('s', "Telnet recv WILL %d", m);
				    if (m != TOPT_BIN && m != TOPT_SUPP && m != TOPT_ECHO)
					telnet_answer(DONT, m);
				} else {
				    telbuf[buflen] = WILL;
				    buflen++;
				}
				break;
		    case WONT:  if ((i+1) < rc) {
				    buflen = 0;
				    i++;
				    m = buf[i] & 0xff;
				    Syslog('s', "Telnet recv WONT %d", m);
				} else {
				    telbuf[buflen] = WONT;
				    buflen++;
				}
				break;
		    case DO:    if ((i+1) < rc) {
				    buflen = 0;
				    i++;
				    m = buf[i] & 0xff;
				    Syslog('s', "Telnet recv DO %d", m);
				    if (m != TOPT_BIN && m != TOPT_SUPP && m != TOPT_ECHO)
					telnet_answer(WONT, m);
				} else {
				    telbuf[buflen] = DO;
				    buflen++;
				}
				break;
		    case DONT:  if ((i+1) < rc) {
				    buflen = 0;
				    i++;
				    m = buf[i] & 0xff;
				    Syslog('s', "Telnet recv DONT %d", m);
				} else {
				    telbuf[buflen] = DONT;
				    buflen++;
				}
				break;
		    case IAC:   buf[j] = buf[i];
				j++;
				buflen = 0;
				break;
		    default:    m = buf[i] & 0xff;
				Syslog('s', "TELNET: recv IAC %d, this is not good, i=%d j=%d", m, i, j);
				buflen = 0;
				buf[j] = IAC;
				j++;
				buf[j] = m;
				j++;
				break;
		}
	    } else {
		buf[j] = buf[i];
		j++;
	    }
	}
	rc = j;
    }

    Syslog('s', "telnet_buffer: rc=%d buflen=%d", rc, buflen);

    return rc;
}


