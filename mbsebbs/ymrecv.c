/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Ymodem receiver
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
 *   
 * Michiel Broek                FIDO:           2:280/2802
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
#include "ttyio.h"
#include "zmmisc.h"
#include "ymrecv.h"



static int	Firstsec;
static int	eof_seen;
static int	errors;

extern int	Crcflg;
extern char	Lastrx;

#define sendline(c) PUTCHAR((c) & 0377)

int wcgetsec(size_t *, char *, unsigned int);

int wcrxpn(char *rpn)
{
    register int    c;
    size_t	    Blklen = 0;                /* record length of received packets */

    purgeline(0);

et_tu:
    Firstsec = TRUE;
    eof_seen = FALSE;
    sendline(Crcflg?WANTCRC:NAK);
    fflush(stdout);
    purgeline(0); /* Do read next time ... */
    while ((c = wcgetsec(&Blklen, rpn, 100)) != 0) {
	if (c == WCEOT) {
	    Syslog('x', "Pathname fetch returned EOT");
	    sendline(ACK);
	    fflush(stdout);
	    purgeline(0);   /* Do read next time ... */
	    GETCHAR(1);
	    goto et_tu;
	}
	return ERROR;
    }
    sendline(ACK);
    fflush(stdout);
    return OK;
}



int wcrx(void)
{
}


int wcgetsec(size_t *Blklen, char *rxbuf, unsigned int maxtime)
{
    register int Checksum, wcj, firstch;
    register unsigned short oldcrc;
    register char *p;
    int sectcurr;

    for (Lastrx = errors = 0; errors < RETRYMAX; errors++) {

	if ((firstch = GETCHAR(maxtime)) == STX) {
	    *Blklen = 1024; goto get2;
	}
	if (firstch == SOH) {
	    *Blklen=128;
get2:
	    sectcurr = GETCHAR(1);
	    if ((sectcurr + (oldcrc = GETCHAR(1))) == 0377) {
		oldcrc = Checksum = 0;
		for (p = rxbuf, wcj = *Blklen; --wcj >= 0; ) {
		    if ((firstch = GETCHAR(1)) < 0)
			goto bilge;
		    oldcrc = updcrc16(firstch, oldcrc);
		    Checksum += (*p++ = firstch);
		}
		if ((firstch = GETCHAR(1)) < 0)
		    goto bilge;
		if (Crcflg) {
		    oldcrc = updcrc16(firstch, oldcrc);
		    if ((firstch = GETCHAR(1)) < 0)
			goto bilge;
		    oldcrc = updcrc16(firstch, oldcrc);
		    if (oldcrc & 0xFFFF)
			Syslog('x', "CRC");
		    else {
			Firstsec=FALSE;
			return sectcurr;
		    }
		} else if (((Checksum - firstch) & 0377) == 0) {
		    Firstsec = FALSE;
		    return sectcurr;
		} else
		    Syslog('x', "Checksum");
	    } else
		Syslog('x', "Sector number garbled");
	}
	/* make sure eot really is eot and not just mixmash */
	else if (firstch == EOT && GETCHAR(1) == TIMEOUT)
	    return WCEOT;
	else if (firstch == CAN) {
	    if (Lastrx == CAN) {
		Syslog('x', "Sender Cancelled");
		return ERROR;
	    } else {
		Lastrx=CAN;
		continue;
	    }
	} else if (firstch == TIMEOUT) {
	    if (Firstsec)
		goto humbug;
bilge:
	    Syslog('x', "TIMEOUT");
	} else
	    Syslog('x', "Got 0%o sector header", firstch);
humbug:
	Lastrx = 0;
	{
	    int cnt = 1000;
	    while (cnt-- && GETCHAR(1) != TIMEOUT);
	}
	
	if (Firstsec) {
	    sendline(Crcflg ? WANTCRC:NAK);
	    fflush(stdout);
	    purgeline(0);   /* Do read next time ... */
	} else {
	    maxtime = 40;
	    sendline(NAK);
	    fflush(stdout);
	    purgeline(0);   /* Do read next time ... */
	}
    }
    /* try to stop the bubble machine. */
    canit(STDOUT_FILENO);
    return ERROR;
}


