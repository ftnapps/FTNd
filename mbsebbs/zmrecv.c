/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Zmodem receive
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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
#include "../lib/users.h"
#include "ttyio.h"
#include "transfer.h"
#include "zmmisc.h"
#include "zmrecv.h"
#include "openport.h"


static FILE *fout = NULL;
char	    *curfile = NULL;

static off_t rxbytes;
static int Eofseen;		/* indicates cpm eof (^Z) has been received */
static int errors;
static long sbytes;
struct timeval starttime, endtime;
struct timezone tz;

#define DEFBYTL 2000000000L	/* default rx file size */
static long Bytesleft;		/* number of bytes of incoming file left */
static long Modtime;		/* Unix style mod time for incoming file */
static int Filemode;		/* Unix style mode for incoming file */
static int Thisbinary = TRUE;	/* current file is to be received in bin mode */
static char *secbuf=0;		/* "sector" buffer */
static int tryzhdrtype;
static char zconv;		/* ZMODEM file conversion request */
static char zmanag;		/* ZMODEM file management request */
static char ztrans;		/* ZMODEM file transport request */

static int tryz(void);
static int rzfiles(void);
static int rzfile(void);
static void zmputs(char*);
int closeit(int);
static int putsec(char*,int);
static int procheader(char*);
static int ackbibi(void);
static long getfree(void);


extern unsigned long	rcvdbytes;



int zmrcvfiles(void)
{
    int rc;

    Syslog('+', "Zmodem: start Zmodem receive");

    get_frame_buffer();

    if (secbuf == NULL) 
	secbuf = malloc(MAXBLOCK+1);
    tryzhdrtype = ZRINIT;
    if ((rc = tryz()) < 0) {
	Syslog('+', "Zmodem: could not initiate receive, rc=%d",rc);
    } else
	switch (rc) {
	    case ZCOMPL:    rc = 0; 
			    break;
	    case ZFILE:	    rc = rzfiles(); 
			    break;
	}

    if (fout) {
	if (closeit(0)) {
	    WriteError("Zmodem: Error closing file");
	}
    }

    if (secbuf)
	free(secbuf);
    secbuf = NULL;
    free_frame_buffer();

    io_mode(0, 1);
    Syslog('z', "Zmodem: receive rc=%d",rc);
    return abs(rc);
}



/*
 * Initialize for Zmodem receive attempt, try to activate Zmodem sender
 *  Handles ZSINIT frame
 *  Return ZFILE if Zmodem filename received, -1 on error,
 *   ZCOMPL if transaction finished,  else 0
 */
int tryz(void)
{
    int	    c, n;
    int	    cmdzack1flg;

    for (n = 15; --n >= 0; ) {
	/*
	 * Set buffer length (0) and capability flags
	 */
	Syslog('z', "tryz attempt %d", n);
	stohdr(0L);
	Txhdr[ZF0] = CANFC32|CANFDX|CANOVIO;
//	Txhdr[ZF0] = CANFC32;
	if (Zctlesc)
	    Txhdr[ZF0] |= TESCCTL;
//	Txhdr[ZF0] |= CANRLE;
//	Txhdr[ZF1] = CANVHDR;
	zshhdr(tryzhdrtype, Txhdr);
	if (tryzhdrtype == ZSKIP)       /* Don't skip too far */
	    tryzhdrtype = ZRINIT;	/* CAF 8-21-87 */
again:
	switch (zgethdr(Rxhdr)) {
	    case ZRQINIT:   // if (Rxhdr[ZF3] & 0x80)
				// Usevhdrs = TRUE; /* we can var header */
			    continue;
	    case ZEOF:	    continue;
	    case TIMEOUT:   Syslog('+', "Zmodem: tryz() timeout attempt %d", n);
			    continue;
	    case ZFILE:	    zconv = Rxhdr[ZF0];
			    if (!zconv) {
				Syslog('z', "*** !zconv %d", zconv);
				zconv = ZCBIN;
			    }
			    zmanag = Rxhdr[ZF1];
			    ztrans = Rxhdr[ZF2];
//			    if (Rxhdr[ZF3] & ZCANVHDR)
//				Usevhdrs = TRUE;
			    tryzhdrtype = ZRINIT;
			    c = zrdata(secbuf, MAXBLOCK);
			    io_mode(0, 3);
			    if (c == GOTCRCW) {
				Syslog('z', "tryz return ZFILE");
				return ZFILE;
			    }
			    zshhdr(ZNAK, Txhdr);
			    goto again;
	    case ZSINIT:    /* this once was:
			     * Zctlesc = TESCCTL & Rxhdr[ZF0];
			     * trouble: if rz get --escape flag:
			     * - it sends TESCCTL to sz, 
			     *   get a ZSINIT _without_ TESCCTL (yeah - sender didn't know), 
			     *   overwrites Zctlesc flag ...
			     * - sender receives TESCCTL and uses "|=..."
			     * so: sz escapes, but rz doesn't unescape ... not good.
			     */
			    Zctlesc |= TESCCTL & Rxhdr[ZF0];
			    if (zrdata(Attn, ZATTNLEN) == GOTCRCW) {
				stohdr(1L);
				zshhdr(ZACK, Txhdr);
				goto again;
			    }
			    zshhdr(ZNAK, Txhdr);
			    goto again;
	    case ZFREECNT:  stohdr(getfree());
			    zshhdr(ZACK, Txhdr);
			    goto again;
	    case ZCOMMAND:  cmdzack1flg = Rxhdr[ZF0];
			    if (zrdata(secbuf, MAXBLOCK) == GOTCRCW) {
				if (cmdzack1flg & ZCACK1)
				    stohdr(0L);
				else
				    Syslog('+', "Zmodem: request for command \"%s\" ignored", printable(secbuf,-32));
				stohdr(0L);
				do {
				    zshhdr(ZCOMPL, Txhdr);
				} while (++errors<20 && zgethdr(Rxhdr) != ZFIN);
				return ackbibi();
			    }
			    zshhdr(ZNAK, Txhdr); 
			    goto again;
	    case ZCOMPL:    goto again;
	    case ZRINIT:    Syslog('z', "tryz: got ZRINIT");
			    return TERROR;
	    case ZFIN:	    /* do not beleive in first ZFIN */
			    ackbibi(); 
			    return ZCOMPL;
	    case TERROR:
	    case HANGUP:
	    case ZCAN:	    return TERROR;
	    default:	    continue;
	}
    }
    Syslog('z', "tryz return 0");
    return 0;
}



/*
 * Receive 1 or more files with ZMODEM protocol
 */
int rzfiles(void)
{
    int c;

    Syslog('z', "rzfiles");

    for (;;) {
	switch (c = rzfile()) {
	    case ZEOF:
	    case ZSKIP:
	    case ZFERR:	    switch (tryz()) {
				case ZCOMPL:	return OK;
				default:	return TERROR;
				case ZFILE:	break;
			    }
			    continue;
	    default:	    return c;
	    case TERROR:    return TERROR;
	}
    }
    /* NOTREACHED */
}



/*
 * Receive a file with ZMODEM protocol
 *  Assumes file name frame is in secbuf
 */
int rzfile(void)
{
    int	c, n;

    Syslog('z', "rzfile");

    Eofseen=FALSE;
    rxbytes = 0l;
    if ((c = procheader(secbuf))) {
	return (tryzhdrtype = c);
    }

    n = 20;

    for (;;) {
	Syslog('z', "rxbytes %ld", rxbytes);
	stohdr(rxbytes);
	zshhdr(ZRPOS, Txhdr);
nxthdr:
	switch (c = zgethdr(Rxhdr)) {
	    default:	    Syslog('z', "rzfile: Wrong header %d", c);
			    if ( --n < 0) {
				Syslog('+', "Zmodem: wrong header %d", c);
				return TERROR;
			    }
			    continue;
	    case ZCAN:	    Syslog('+', "Zmodem: sender CANcelled");
			    return TERROR;
	    case ZNAK:	    if ( --n < 0) {
				Syslog('+', "Zmodem: Got ZNAK");
				return TERROR;
			    }
			    continue;
	    case TIMEOUT:   if ( --n < 0) {
				Syslog('+', "Zmodem: TIMEOUT");
				return TERROR;
			    }
			    continue;
	    case ZFILE:	    zrdata(secbuf, MAXBLOCK);
			    continue;
	    case ZEOF:	    if (rclhdr(Rxhdr) != rxbytes) {
				/*
				 * Ignore eof if it's at wrong place - force
				 *  a timeout because the eof might have gone
				 *  out before we sent our zrpos.
				 */
				errors = 0;
				goto nxthdr;
			    }
			    if (closeit(1)) {
				tryzhdrtype = ZFERR;
				Syslog('+', "Zmodem: error closing file");
				return TERROR;
			    }
			    fout = NULL;
			    Syslog('z', "rzfile: normal EOF");
			    return c;
	    case HANGUP:    Syslog('+', "Zmodem: Lost Carrier");
			    return TERROR;
	    case TERROR:    /* Too much garbage in header search error */
			    if (--n < 0) {
				Syslog('+', "Zmodem: Too many errors");
				return TERROR;
			    }
			    zmputs(Attn);
			    continue;
	    case ZSKIP:	    Modtime = 1;
			    closeit(1);
			    Syslog('+', "Zmodem: Sender SKIPPED file");
			    return c;
	    case ZDATA:	    if (rclhdr(Rxhdr) != rxbytes) {
				if ( --n < 0) {
				    Syslog('+', "Zmodem: Data has bad address");
				    return TERROR;
				}
				zmputs(Attn);  
				continue;
			    }
moredata:
			    Syslog('z', "%7ld ZMODEM%s    ", rxbytes, Crc32r?" CRC-32":"");
			    Nopper();
			    switch (c = zrdata(secbuf, MAXBLOCK)) {
				case ZCAN:	Syslog('+', "Zmodem: sender CANcelled");
						return TERROR;
				case HANGUP:	Syslog('+', "Zmodem: Lost Carrier");
						return TERROR;
				case TERROR:	/* CRC error */
						if (--n < 0) {
						    Syslog('+', "Zmodem: Too many errors");
						    return TERROR;
						}
						zmputs(Attn);
						continue;
				case TIMEOUT:	if ( --n < 0) {
						    Syslog('+', "Zmodem: TIMEOUT");
						    return TERROR;
						}
						continue;
				case GOTCRCW:	n = 20;
						putsec(secbuf, Rxcount);
						rxbytes += Rxcount;
						Syslog('z', "rxbytes %ld, will ACK", rxbytes);
						stohdr(rxbytes);
						PUTCHAR(XON);
						zshhdr(ZACK, Txhdr);
						goto nxthdr;
				case GOTCRCQ:	n = 20;
						putsec(secbuf, Rxcount);
						rxbytes += Rxcount;
						stohdr(rxbytes);
						zshhdr(ZACK, Txhdr);
						goto moredata;
				case GOTCRCG:	n = 20;
						putsec(secbuf, Rxcount);
						rxbytes += Rxcount;
						goto moredata;
				case GOTCRCE:	n = 20;
						putsec(secbuf, Rxcount);
						rxbytes += Rxcount;
						goto nxthdr;
			    }
	}
    }
}



/*
 * Send a string to the modem, processing for \336 (sleep 1 sec)
 *   and \335 (break signal)
 */
void zmputs(char *s)
{
    int c;

    Syslog('z', "zmputs: \"%s\"", printable(s, strlen(s)));

    while (*s) {
	switch (c = *s++) {
	    case '\336':    Syslog('z', "zmputs: sleep(1)");
			    sleep(1); 
			    continue;
	    case '\335':    Syslog('z', "zmputs: send break");
			    sendbrk(); 
			    continue;
	    default:	    PUTCHAR(c);
	}
    }
}



int closeit(int success)
{
    struct utimbuf  ut;
    int		    rc;

    Syslog('z', "closeit(%d)", success);
    
    if ((fout == NULL) || (curfile == NULL)) {
	Syslog('+', "closeit(), nothing to close");
	return 1;
    }

    rc = fclose(fout);
    fout = NULL;

    if (rc == 0) {
	ut.actime = Modtime;
	ut.modtime = Modtime;
	if ((rc = utime(curfile, &ut)))
	    WriteError("$utime failed");
    }
    free(curfile);
    curfile = NULL;

    sbytes = rxbytes - sbytes;
    gettimeofday(&endtime, &tz);
    if (success)
        Syslog('+', "Zmodem: OK %s", transfertime(starttime, endtime, sbytes, FALSE));
    else
	Syslog('+', "Zmodem: dropped after %lu bytes", sbytes);
    rcvdbytes += sbytes;
    return rc;
}



/*
 * Ack a ZFIN packet, let byegones be byegones
 */
int ackbibi(void)
{
    int n;
    int c;

    Syslog('z', "ackbibi:");
    stohdr(0L);
	
    for (n=3; --n>=0; ) {
	zshhdr(ZFIN, Txhdr);
		
	switch ((c = GETCHAR(10))) {
	    case 'O':	    GETCHAR(1);	/* Discard 2nd 'O' */
			    Syslog('z', "Zmodem: ackbibi complete");
			    return ZCOMPL;
	    case TERROR:
	    case HANGUP:    Syslog('z', "Zmodem: ackbibi got %d, ignore",c);
			    return 0;
	    case TIMEOUT:
	    default:	    Syslog('z', "Zmodem: ackbibi got '%s', continue", printablec(c));
			    break;
	}
    }
    return ZCOMPL;
}



/*
 * Process incoming file information header
 */
int procheader(char *Name)
{
    register char   *openmode, *p;
    static int	    dummy;
    char	    ctt[32];

    Syslog('z', "procheader \"%s\"",printable(Name,0));
    /* set default parameters and overrides */
    openmode = (char *)"w";

    Syslog('z', "zmanag=%d", zmanag);
    Syslog('z', "zconv=%d", zconv);

    /*
     *  Process ZMODEM remote file management requests
     */
    if (!Thisbinary && zconv == ZCNL)	/* Remote ASCII override */
	Thisbinary = FALSE;
    if (zconv == ZCBIN)			/* Remote Binary override */
	Thisbinary = TRUE;
    if (zmanag == ZMAPND)
	openmode = (char *)"a";

    Syslog('z', "Thisbinary %s", Thisbinary ?"TRUE":"FALSE");

    Bytesleft = DEFBYTL; 
    Filemode = 0; 
    Modtime = 0L;
    Eofseen = FALSE;

    p = Name + 1 + strlen(Name);
    // FIXME: Here we must add code that checks the Name for slashes, spaces and other
    // illegal characters in the filename.
    sscanf(p, "%ld%lo%o%o%d%d%d%d", &Bytesleft, &Modtime, &Filemode, &dummy, &dummy, &dummy, &dummy, &dummy);
    strcpy(ctt, rfcdate(Modtime));
    Syslog('+', "Zmodem: \"%s\" %ld bytes, %s mode %o", Name, Bytesleft, ctt, Filemode);

    if (curfile)
	free(curfile);
    curfile = NULL;

    curfile = xstrcpy(CFG.bbs_usersdir);
    curfile = xstrcat(curfile, (char *)"/");
    curfile = xstrcat(curfile, exitinfo.Name);
    curfile = xstrcat(curfile, (char *)"/upl/");
    curfile = xstrcat(curfile, Name);
    Syslog('z', "try open %s mode \"%s\"", curfile, openmode);
    if ((fout = fopen(curfile, openmode)) == NULL) {
	WriteError("$Can't open %s mode %s", curfile, openmode);
    }

    gettimeofday(&starttime, &tz);
    sbytes = rxbytes = 0;

    Syslog('z', "result %s", fout ? "Ok":"Failed");

/*  if (Bytesleft == rxbytes) { FIXME: if file already received, use this.
	Syslog('+', "Zmodem: Skipping %s", Name);
	fout = NULL;
	return ZSKIP;
    } else */ if (!fout) 
	return ZFERR;
    else 
	return 0;
}



/*
 * Putsec writes the n characters of buf to receive file fout.
 *  If not in binary mode, carriage returns, and all characters
 *  starting with CPMEOF are discarded.
 */
int putsec(char *buf, int n)
{
    register char *p;

    Syslog('z', "putsec %d characters %s mode", n, Thisbinary ? "binary":"ascii");

    if (n == 0)
	return OK;

    if (Thisbinary) {
	for (p = buf; --n>=0; )
	    putc( *p++, fout);
    } else {
	if (Eofseen)
	    return OK;
	for (p = buf; --n>=0; ++p ) {
	    if ( *p == '\r')
		continue;
	    if (*p == SUB) {
		Eofseen=TRUE; 
		return OK;
	    }
	    putc(*p ,fout);
	}
    }
    return OK;
}



long getfree(void)
{
    struct statfs   sfs;
    char	    *temp;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/%s/upl", CFG.bbs_usersdir, exitinfo.Name);
    
    if (statfs(temp, &sfs) != 0) {
	WriteError("$cannot statfs \"%s\", assume enough space", temp);
	free(temp);
	return -1L;
    }

    free(temp);
    return (sfs.f_bsize * sfs.f_bfree);
}


