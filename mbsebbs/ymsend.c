/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Ymodem sender
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
#include "ymsend.h"
#include "ttyio.h"
#include "zmmisc.h"
#include "transfer.h"
#include "openport.h"
#include "timeout.h"
#include "term.h"


#define MAX_BLOCK 8192
#define sendline(c) PUTCHAR((c) & 0377)


FILE *input_f;
char Crcflg;
char Lastrx;
int Fullname=0;         /* transmit full pathname */
int Filesleft;
long Totalleft;
long bytes_sent;
int Ascii=0;            /* Add CR's for brain damaged programs */
int firstsec;
int Optiong;            /* Let it rip no wait for sector ACK's */
int Lfseen=0;
int Totsecs;            /* total number of sectors this file */
char *txbuf;
size_t blklen=128;              /* length of transmitted records */
int zmodem_requested = 0;
int Dottoslash=0;       /* Change foo.bar.baz to foo/bar/baz */
static int no_unixmode;

extern int  Rxtimeout;


static int getnak(void);
int wcputsec(char *, int, size_t);
static size_t filbuf(char *, size_t);


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
    input_f = fopen(fname, "r");

    if (protocol == ZM_XMODEM) {
	if (*fname && fstat(fileno(input_f), &f) != -1) {
	    Syslog('y', "Sending %s, %ld blocks: ", fname, (long) (f.st_size >> 7));
	}
	PUTSTR((char *)"Give your local XMODEM receive command now.");
	Enter(1);
	return OK;
    }

    if (!zmodem_requested)
	if (getnak()) {
	    PUTSTR((char *)"getnak failed");
	    Syslog('+', "%s/%s: getnak failed", fname, protname());
	    return ERROR;
	}

    q = (char *) 0;
    if (Dottoslash) {               /* change . to . */
	for (p = fname; *p; ++p) {
	    if (*p == '/')
		q = p;
	    else if (*p == '.')
		*(q=p) = '/';
	}
	if (q && strlen(++q) > 8) {     /* If name>8 chars */
	    q += 8;                 /*   make it .ext */
	    strcpy(name2, q);       /* save excess of name */
	    *q = '.';
	    strcpy(++q, name2);     /* add it back */
	}
    }

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
    if (!Ascii && (input_f != stdin) && *fname && fstat(fileno(input_f), &f)!= -1)
	sprintf(p, "%lu %lo %o 0 %d %ld", (long) f.st_size, f.st_mtime,
	    (unsigned int)((no_unixmode) ? 0 : f.st_mode), Filesleft, Totalleft);
//    if (Verbose)
//	vstringf(_("Sending: %s\n"),txbuf);
    Totalleft -= f.st_size;
    if (--Filesleft <= 0)
	Totalleft = 0;
    if (Totalleft < 0)
	Totalleft = 0;

    /* force 1k blocks if name won't fit in 128 byte block */
    if (txbuf[125])
	blklen=1024;
    else {          /* A little goodie for IMP/KMD */
	txbuf[127] = (f.st_size + 127) >>7;
	txbuf[126] = (f.st_size + 127) >>15;
    }
//    if (zmodem_requested)
//	return zsendfile(zi,txbuf, 1+strlen(p)+(p-txbuf));
    if (wcputsec(txbuf, 0, 128)==ERROR) {
	PUTSTR((char *)"wcputsec failed");
	Syslog('+', "%s/%s: wcputsec failed", fname,protname());
	return ERROR;
    }
    return OK;
}



static int getnak(void)
{
    int firstch;
    int tries = 0;

    Lastrx = 0;
    for (;;) {
	tries++;
	switch (firstch = GETCHAR(10)) {
//	    case ZPAD:
//			if (getzrxinit())
//			    return ERROR;
//			Ascii = 0;      /* Receiver does the conversion */
//			return FALSE;
	    case TIMEOUT:
			/* 30 seconds are enough */
			if (tries == 3) {
			    Syslog('y', "Timeout on pathname");
			    return TRUE;
			}
			/* don't send a second ZRQINIT _directly_ after the
			 * first one. Never send more then 4 ZRQINIT, because
			 * omen rz stops if it saw 5 of them */
//			if ((zrqinits_sent>1 || tries>1) && zrqinits_sent<4) {
			    /* if we already sent a ZRQINIT we are using zmodem
			     * protocol and may send further ZRQINITs 
			     */
//			    stohdr(0L);
//			    zshhdr(ZRQINIT, Txhdr);
//			    zrqinits_sent++;
//			}
			continue;
	    case WANTG:
			io_mode(0, 2);  /* Set cbreak, XON/XOFF, etc. */
			Optiong = TRUE;
			blklen=1024;
	    case WANTCRC:
			Crcflg = TRUE;
	    case NAK:
			return FALSE;
	    case CAN:
			if ((firstch = GETCHAR(2)) == CAN && Lastrx == CAN)
			    return TRUE;
	    default:
			break;
	}
    Lastrx = firstch;
    }
}



static int wctx(long bytes_total)
{
    register size_t thisblklen;
    register int    sectnum, attempts, firstch;

    firstsec=TRUE;  thisblklen = blklen;
    Syslog('y', "wctx:file length=%ld", bytes_total);

    while ((firstch = GETCHAR(Rxtimeout))!=NAK && firstch != WANTCRC
	    && firstch != WANTG && firstch != TIMEOUT && firstch != CAN);
    if (firstch == CAN) {
	Syslog('y', "Receiver Cancelled");
	return ERROR;
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
	    return ERROR;
	bytes_sent += thisblklen;
    }
    
    fclose(input_f);
    attempts = 0;
    
    do {
//	purgeline(io_mode_fd);
	PUTCHAR(EOT);
	fflush(stdout);
	++attempts;
    } while ((firstch = (GETCHAR(Rxtimeout)) != ACK) && attempts < RETRYMAX);
    if (attempts == RETRYMAX) {
	Syslog('y', "No ACK on EOT");
	return ERROR;
    } else
	return OK;
}



int wcputsec(char *buf, int sectnum, size_t cseclen)
{
    int		Checksum, wcj;
    char	*cp;
    unsigned	oldcrc;
    int		firstch;
    int		attempts;

    firstch = 0;      /* part of logic to detect CAN CAN */

    Syslog('y', "%s sectors/kbytes sent: %3d/%2dk", protname(), Totsecs, Totsecs/8 );
    
    for (attempts = 0; attempts <= RETRYMAX; attempts++) {
	Lastrx = firstch;
	sendline(cseclen == 1024 ? STX:SOH);
	sendline(sectnum);
	sendline(-sectnum -1);
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
	}
	else
	    sendline(Checksum);

	fflush(stdout);
	if (Optiong) {
	    firstsec = FALSE; 
	    return OK;
	}
	firstch = GETCHAR(Rxtimeout);
gotnak:
	switch (firstch) {
	    case CAN:
			if(Lastrx == CAN) {
cancan:
			    Syslog('y', "Cancelled");  
			    return ERROR;
			}
			break;
	    case TIMEOUT:
			Syslog('y', "Timeout on sector ACK"); 
			continue;
	    case WANTCRC:
			if (firstsec)
			    Crcflg = TRUE;
	    case NAK:
			Syslog('y', "NAK on sector"); 
			continue;
	    case ACK: 
			firstsec=FALSE;
			Totsecs += (cseclen>>7);
			return OK;
	    case TERROR:
			Syslog('y', "Got burst for sector ACK"); 
			break;
	    default:
			Syslog('y', "Got %02x for sector ACK", firstch); 
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
    Syslog('y', "Retry Count Exceeded");
    return ERROR;
}



/* fill buf with count chars padding with ^Z for CPM */
static size_t filbuf(char *buf, size_t count)
{
    int	    c;
    size_t  m;

    if ( !Ascii) {
	m = read(fileno(input_f), buf, count);
	if (m <= 0)
	    return 0;
	while (m < count)
	    buf[m++] = 032;
	return count;
    }
    m=count;
    if (Lfseen) {
	*buf++ = 012; --m; Lfseen = 0;
    }
    while ((c=getc(input_f))!=EOF) {
	if (c == 012) {
	    *buf++ = 015;
	    if (--m == 0) {
		Lfseen = TRUE; break;
	    }
	}
	*buf++ =c;
	if (--m == 0)
	    break;
    }
    if (m==count)
        return 0;
    else
        while (m--!=0)
	    *buf++ = CPMEOF;
    return count;
}



