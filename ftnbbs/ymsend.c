/*****************************************************************************
 *
 * $Id: ymsend.c,v 1.14 2005/10/11 20:49:48 mbse Exp $
 * Purpose ...............: Ymodem sender
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "transfer.h"
#include "openport.h"
#include "timeout.h"
#include "term.h"
#include "ymsend.h"


#define MAX_BLOCK 8192
#define sendline(c) PUTCHAR((c & 0377))


FILE		*input_f;
char		Crcflg;
char		Lastrx;
int		Fullname = 0;		/* transmit full pathname */
int		Filesleft;
int		Totalleft;
int		bytes_sent;
int		firstsec;
int		Optiong;		/* Let it rip no wait for sector ACK's */
int		Totsecs;		/* total number of sectors this file */
char		*txbuf;
size_t		blklen = 128;		/* length of transmitted records */
int		zmodem_requested = FALSE;
static int	no_unixmode;
struct timeval  starttime, endtime;
struct timezone	tz;
int		skipsize;


extern int  Rxtimeout;


static int wctxpn(char *);
static int getnak(void);
static int wctx(int);
static int wcputsec(char *, int, size_t);
static size_t filbuf(char *, size_t);



int ymsndfiles(down_list *lst, int use1k)
{
    int         maxrc = 0;
    down_list   *tmpf;

    Syslog('+', "%s: start send files", protname());
    txbuf = malloc(MAX_BLOCK);

    /*
     * Count files to transmit
     */
    Totalleft = Filesleft = 0;
    for (tmpf = lst; tmpf; tmpf = tmpf->next) {
	if (tmpf->remote) {
	    Filesleft++;
	    Totalleft += tmpf->size;
	}
    }
    Syslog('x', "%s: %d files, size %d bytes", protname(), Filesleft, Totalleft);

    /*
     * This is not documented as auto start sequence, but a lot of terminal
     * programs seem to react on this.
     */
    if (protocol == ZM_YMODEM)
	PUTSTR((char *)"rb\r");

    for (tmpf = lst; tmpf && (maxrc < 2); tmpf = tmpf->next) {
	if (tmpf->remote) {
	    bytes_sent = 0;
	    skipsize = 0L;
	    Totsecs = 0;

	    switch (wctxpn(tmpf->remote)) {
		case TERROR:	Syslog('x', "wctxpn returns error");
				tmpf->failed = TRUE;
				maxrc = 2;
				break;
		case ZSKIP:	Syslog('x', "wctxpn returns skip");
				tmpf->failed = TRUE;
				break;
		case OK:	gettimeofday(&starttime, &tz);
				if ((blklen == 128) && use1k) {
				    Syslog('x', "%s: will use 1K blocks", protname());
				    blklen = 1024;
				}
				if (wctx(tmpf->size) == TERROR) {
				    Syslog('+', "%s: wctx returned error", protname());
				    tmpf->failed = TRUE;
				} else {
				    tmpf->sent = TRUE;
				    gettimeofday(&endtime, &tz);
				    Syslog('+', "%s: OK %s", protname(), 
					    transfertime(starttime, endtime, (unsigned int)tmpf->size - skipsize, TRUE));
				}
	    }

	} else if (maxrc == 0) {
	    tmpf->failed = TRUE;
	}
	if (unlink(tmpf->remote))
	    Syslog('+', "%s: could not unlink %s", protname(), tmpf->remote);
    }

    if (maxrc == 2) {
	canit(1);
	purgeline(50);
    } else {
	if (protocol == ZM_YMODEM) {
	    /*
	     * Send empty filename to signal end of batch
	     */
	    wctxpn((char *)"");
	}
    }

    if (txbuf)
	free(txbuf);
    txbuf = NULL;
    io_mode(0, 1);

    Syslog('+', "%s: end send files rc=%d", protname(), maxrc);
    return (maxrc < 2)?0:maxrc;
}



/*
 * generate and transmit pathname block consisting of
 *  pathname (null terminated),
 *  file length, mode time and file mode in octal
 *  as provided by the Unix fstat call.
 *  N.B.: modifies the passed name, may extend it!
 */
static int wctxpn(char *fname)
{
    register char   *p, *q;
    char	    *name2;
    struct stat	    f;

    name2 = alloca(PATH_MAX+1);

    if ((input_f = fopen(fname, "r"))) {
	fstat(fileno(input_f), &f);

	Syslog('+', "%s: send \"%s\"", protname(), MBSE_SS(fname));
	Syslog('+', "%s: size %lu bytes, dated %s", protname(), (unsigned int)f.st_size, rfcdate(f.st_mtime));
	    
	if (protocol == ZM_XMODEM) {
	    if (*fname) {
		snprintf(name2, PATH_MAX +1, "Sending %s, %d blocks: ", fname, (int) (f.st_size >> 7));
		PUTSTR(name2);
		Enter(1);
	    }
	    PUTSTR((char *)"Give your local XMODEM receive command now.");
	    Enter(1);
	    return OK;
	}
    } else {
	/*
	 * Reset, this normally happens for the ymodem end of batch block.
	 */
	f.st_size = 0;
	f.st_mtime = 0;
	f.st_mode = 0;
    }

    if (!zmodem_requested)
	if (getnak()) {
	    Syslog('+', "%s: timeout sending filename %s", protname(), MBSE_SS(fname));
	    return TERROR;
	}

    q = (char *) 0;

    for (p = fname, q = txbuf ; *p; )
	if ((*q++ = *p++) == '/' && !Fullname)
	    q = txbuf;
    *q++ = 0;
    p = q;
    while (q < (txbuf + MAX_BLOCK))
	*q++ = 0;

    /* 
     * note that we may lose some information here in case mode_t is wider than an 
     * int. But i believe sending %lo instead of %o _could_ break compatability
     */
    if ((input_f != stdin) && *fname)
	snprintf(p, MAXBLOCK + 1024, "%u %o %o 0 %d %d", (int) f.st_size, (int) f.st_mtime,
	    (unsigned int)((no_unixmode) ? 0 : f.st_mode), Filesleft, Totalleft);

    Totalleft -= f.st_size;
    if (--Filesleft <= 0)
	Totalleft = 0;
    if (Totalleft < 0)
	Totalleft = 0;

    purgeline(1);
    if (wcputsec(txbuf, 0, 128) == TERROR) {
	Syslog('+', "%s: failed to send filename", protname());
	return TERROR;
    }
    return OK;
}



/*
 * Get NAK, C or G
 */
static int getnak(void)
{
    int firstch;
    int tries = 0;

    Syslog('x', "getnak()");
    Lastrx = 0;

    for (;;) {
	tries++;
	switch (firstch = GETCHAR(10)) {
	    case TIMEOUT:
			Syslog('x', "getnak: timeout try %d", tries);
			/* 50 seconds are enough (was 30, 26-11-2004 MB) */
			if (tries == 5) {
			    Syslog('x', "Timeout on pathname");
			    return TRUE;
			}
			continue;
	    case WANTG:
			Syslog('x', "getnak: got WANTG");
			io_mode(0, 2);  /* Set cbreak, XON/XOFF, etc. */
			if (!Optiong)
			    Syslog('+', "%s: using Ymodem-G", protname());
			Optiong = TRUE;
			blklen=1024;
			Crcflg = TRUE;
			return FALSE;
	    case WANTCRC:
			Syslog('x', "getnak: got WANTCRC");
			Crcflg = TRUE;
			return FALSE;
	    case NAK:
			Syslog('x', "getnak: got NAK");
			return FALSE;
	    case CAN:
			Syslog('x', "getnak: got CAN");
			if ((firstch = GETCHAR(2)) == CAN && Lastrx == CAN) {
			    Syslog('+', "%s: Receiver cancelled", protname());
			    return TRUE;
			}
	    default:
			Syslog('x', "got %d", firstch);
			break;
	}
    Lastrx = firstch;
    }
    Syslog('x', "getnak: done");
}



static int wctx(int bytes_total)
{
    register size_t thisblklen;
    register int    sectnum, attempts, firstch;

    firstsec=TRUE;  
    thisblklen = blklen;
    Syslog('x', "wctx: file length=%ld, blklen=%d", bytes_total, blklen);

    while ((firstch = GETCHAR(Rxtimeout)) != NAK && firstch != WANTCRC
	    && firstch != WANTG && firstch != TIMEOUT && firstch != CAN);
    if (firstch == CAN) {
	Syslog('+', "%s: receiver Cancelled during transfer", protname());
	return TERROR;
    }

    if (firstch == WANTCRC)
	Crcflg = TRUE;
    if (firstch == WANTG)
	Crcflg = TRUE;
    sectnum = 0;

    for (;;) {
	if (bytes_total <= (bytes_sent + 896L))
	    thisblklen = 128;
	if ( !filbuf(txbuf, thisblklen))
	    break;
	if (wcputsec(txbuf, ++sectnum, thisblklen) == TERROR)
	    return TERROR;
	bytes_sent += thisblklen;

	/*
	 * Keep connections alive
	 */
	Nopper();
	alarm_on();
    }
    
    fclose(input_f);
    attempts = 0;
    
    do {
	purgeline(5);
	PUTCHAR(EOT);
	++attempts;
    } while ((firstch = (GETCHAR(Rxtimeout)) != ACK) && attempts < RETRYMAX);
    if (attempts == RETRYMAX) {
	Syslog('x', "No ACK on EOT");
	return TERROR;
    } else
	return OK;
}



static int wcputsec(char *buf, int sectnum, size_t cseclen)
{
    int		Checksum, wcj;
    char	*cp;
    unsigned	oldcrc;
    int		firstch;
    int		attempts;

    firstch = 0;      /* part of logic to detect CAN CAN */
    
    Syslog('x', "%s: wcputsec: sectnum %d, len %d, %s", protname(), sectnum, cseclen, Crcflg ? "crc":"checksum");
    
    for (attempts = 0; attempts <= RETRYMAX; attempts++) {
	Lastrx = firstch;
	sendline(cseclen == 1024 ? STX:SOH);
	sendline(sectnum);
	sendline((-sectnum -1));
	oldcrc = Checksum = 0;
	for (wcj = cseclen, cp = buf; --wcj >= 0; ) {
	    sendline(*cp);
	    oldcrc = updcrc16((0377& *cp), oldcrc);
	    Checksum += *cp++;
	}
	if (Crcflg) {
	    oldcrc = updcrc16(0, updcrc16(0, oldcrc));
	    sendline((int)oldcrc >> 8);
	    sendline((int)oldcrc);
	} else {
	    sendline(Checksum);
	}

	if (Optiong) {
	    firstsec = FALSE; 
	    return OK;
	}
	firstch = GETCHAR(Rxtimeout);
gotnak:
	switch (firstch) {
	    case CAN:
			Syslog('x', "got CAN");
			if(Lastrx == CAN) {
cancan:
			    Syslog('x', "Cancelled");  
			    return TERROR;
			}
			break;
	    case TIMEOUT:
			Syslog('x', "Timeout on sector ACK"); 
			continue;
	    case WANTCRC:
			if (firstsec)
			    Crcflg = TRUE;
	    case NAK:
			Syslog('+', "%s: got NAK on sector %d", protname(), sectnum); 
			purgeline(20);
			continue;
	    case ACK:
			firstsec=FALSE;
			Totsecs += (cseclen>>7);
			return OK;
	    case TERROR:
			Syslog('x', "Got burst for sector ACK"); 
			break;
	    default:
			Syslog('x', "Got %02x for sector ACK", firstch); 
			break;
	}
	for (;;) {
	    Lastrx = firstch;
	    if ((firstch = GETCHAR(Rxtimeout)) == TIMEOUT)
		break;
	    if (firstch == NAK || firstch == WANTCRC)
		goto gotnak;
	    if (firstch == CAN && Lastrx == CAN)
		goto cancan;
	}
    }
    Syslog('x', "Retry Count Exceeded");
    return TERROR;
}



/* 
 * fill buf with count chars padding with ^Z for CPM 
 */
static size_t filbuf(char *buf, size_t count)
{
    size_t  m;

    m = read(fileno(input_f), buf, count);
    if (m <= 0)
	return 0;
    
    while (m < count)
        buf[m++] = 032;
    return count;
}



