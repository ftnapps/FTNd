/*****************************************************************************
 *
 * $Id: zmrecv.c,v 1.15 2007/08/26 14:02:28 mbse Exp $
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "../lib/nodelist.h"
#include "lutil.h"
#include "ttyio.h"
#include "session.h"
#include "zmodem.h"
#include "config.h"
#include "emsi.h"
#include "openfile.h"
#include "filelist.h"
#include "openport.h"


static FILE *fout=NULL;

static int Usevhdrs;
static off_t rxbytes;
static int Eofseen;		/* indicates cpm eof (^Z) has been received */
static int errors;
static int sbytes;
struct timeval starttime, endtime;
struct timezone tz;

#define DEFBYTL 2000000000L	/* default rx file size */
static int Bytesleft;		/* number of bytes of incoming file left */
static int Modtime;		/* Unix style mod time for incoming file */
static int Filemode;		/* Unix style mode for incoming file */

static int Thisbinary;		/* current file is to be received in bin mode */
char Lzconv;			/* Local ZMODEM file conversion request */
char Lzmanag;			/* Local file management request */

static char *secbuf=0;

static int tryzhdrtype;
static char zconv;		/* ZMODEM file conversion request */
static char zmanag;		/* ZMODEM file management request */
static char ztrans;		/* ZMODEM file transport request */

static int resync(off_t);
static int tryz(void);
static int rzfiles(void);
static int rzfile(void);
static void zmputs(char*);
int closeit(int);
static int putsec(char*,int);
static int procheader(char*);
static int ackbibi(void);
static int getfree(void);


void get_frame_buffer(void);
void free_frame_buffer(void);

extern unsigned int rcvdbytes;



int zmrcvfiles(void)
{
	int rc;

	Syslog('+', "Zmodem: start %s receive", (emsi_local_protos & PROT_ZAP)?"ZedZap":"Zmodem");

	get_frame_buffer();

	if (secbuf == NULL) 
		secbuf = malloc(MAXBLOCK+1);
	tryzhdrtype = ZRINIT;
	if ((rc = tryz()) < 0) {
		Syslog('+', "Zmodem: could not initiate receive, rc=%d",rc);
	} else
		switch (rc) {
			case ZCOMPL:	rc = 0; break;
			case ZFILE:	rc = rzfiles(); break;
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
	int	c, n;
	int	cmdzack1flg;

	for (n = 15; --n >= 0; ) {
		/*
		 * Set buffer length (0) and capability flags
		 */
		Syslog('z', "tryz attempt %d", n);
		stohdr(0L);
		Txhdr[ZF0] = CANFC32|CANFDX|CANOVIO;
		if (Zctlesc)
			Txhdr[ZF0] |= TESCCTL;
		Txhdr[ZF0] |= CANRLE;
		Txhdr[ZF1] = CANVHDR;
		zshhdr(4, tryzhdrtype, Txhdr);
		if (tryzhdrtype == ZSKIP)       /* Don't skip too far */
			tryzhdrtype = ZRINIT;   /* CAF 8-21-87 */
again:
		switch (zgethdr(Rxhdr)) {
		case ZRQINIT:
			if (Rxhdr[ZF3] & 0x80)
				Usevhdrs = TRUE; /* we can var header */
			continue;
		case ZEOF:
			continue;
		case TIMEOUT:
			Syslog('+', "Zmodem: tryz() timeout attempt %d", n);
			continue;
		case ZFILE:
			zconv = Rxhdr[ZF0];
			zmanag = Rxhdr[ZF1];
			ztrans = Rxhdr[ZF2];
			if (Rxhdr[ZF3] & ZCANVHDR)
				Usevhdrs = TRUE;
			tryzhdrtype = ZRINIT;
			c = zrdata(secbuf, MAXBLOCK);
			if (c == GOTCRCW)
				return ZFILE;
			zshhdr(4,ZNAK, Txhdr);
			goto again;
		case ZSINIT:
			Zctlesc = TESCCTL & Rxhdr[ZF0];
			if (zrdata(Attn, ZATTNLEN) == GOTCRCW) {
				stohdr(1L);
				zshhdr(4,ZACK, Txhdr);
				goto again;
			}
			zshhdr(4,ZNAK, Txhdr);
			goto again;
		case ZFREECNT:
			stohdr(getfree());
			zshhdr(4,ZACK, Txhdr);
			goto again;
		case ZCOMMAND:
			cmdzack1flg = Rxhdr[ZF0];
			if (zrdata(secbuf, MAXBLOCK) == GOTCRCW) {
				if (cmdzack1flg & ZCACK1)
					stohdr(0L);
				else
					Syslog('+', "Zmodem: request for command \"%s\" ignored", printable(secbuf,-32));
				stohdr(0L);
				do {
					zshhdr(4,ZCOMPL, Txhdr);
				} while (++errors<20 && zgethdr(Rxhdr) != ZFIN);
				return ackbibi();
			}
			zshhdr(4,ZNAK, Txhdr); goto again;
		case ZCOMPL:
			goto again;
		case ZRINIT:
		case ZFIN: /* do not beleive in first ZFIN */
			ackbibi(); return ZCOMPL;
		case TERROR:
		case HANGUP:
		case ZCAN:
			return TERROR;
		default:
			continue;
		}
	}
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
			case ZFERR:
				switch (tryz()) {
					case ZCOMPL:
						return OK;
					default:
						return TERROR;
					case ZFILE:
						break;
					}
					continue;
			default:
				return c;
			case TERROR:
				return TERROR;
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
		stohdr(rxbytes);
		zshhdr(4,ZRPOS, Txhdr);
nxthdr:
		switch (c = zgethdr(Rxhdr)) {
		default:
			Syslog('z', "rzfile: Wrong header %d", c);
			if ( --n < 0) {
				Syslog('+', "Zmodem: wrong header %d", c);
				return TERROR;
			}
			continue;
		case ZCAN:
			Syslog('+', "Zmodem: sender CANcelled");
			return TERROR;
		case ZNAK:
			if ( --n < 0) {
				Syslog('+', "Zmodem: Got ZNAK");
				return TERROR;
			}
			continue;
		case TIMEOUT:
			if ( --n < 0) {
				Syslog('+', "Zmodem: TIMEOUT");
				return TERROR;
			}
			continue;
		case ZFILE:
			zrdata(secbuf, MAXBLOCK);
			continue;
		case ZEOF:
			if (rclhdr(Rxhdr) != rxbytes) {
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
			fout=NULL;
			Syslog('z', "rzfile: normal EOF");
			return c;
		case HANGUP:
			Syslog('+', "Zmodem: Lost Carrier");
			return TERROR;
		case TERROR:	/* Too much garbage in header search error */
			if (--n < 0) {
				Syslog('+', "Zmodem: Too many errors");
				return TERROR;
			}
			zmputs(Attn);
			continue;
		case ZSKIP:
			Modtime = 1;
			closeit(1);
			Syslog('+', "Zmodem: Sender SKIPPED file");
			return c;
		case ZDATA:
			if (rclhdr(Rxhdr) != rxbytes) {
				if ( --n < 0) {
					Syslog('+', "Zmodem: Data has bad address");
					return TERROR;
				}
				zmputs(Attn);  continue;
			}
moredata:
				Syslog('z', "%7ld ZMODEM%s    ",
				  rxbytes, Crc32r?" CRC-32":"");
			Nopper();
			switch (c = zrdata(secbuf, MAXBLOCK)) {
			case ZCAN:
				Syslog('+', "Zmodem: sender CANcelled");
				return TERROR;
			case HANGUP:
				Syslog('+', "Zmodem: Lost Carrier");
				return TERROR;
			case TERROR:	/* CRC error */
				if (--n < 0) {
					Syslog('+', "Zmodem: Too many errors");
					return TERROR;
				}
				zmputs(Attn);
				continue;
			case TIMEOUT:
				if ( --n < 0) {
					Syslog('+', "Zmodem: TIMEOUT");
					return TERROR;
				}
				continue;
			case GOTCRCW:
				n = 20;
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				stohdr(rxbytes);
				PUTCHAR(XON);
				zshhdr(4,ZACK, Txhdr);
				goto nxthdr;
			case GOTCRCQ:
				n = 20;
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				stohdr(rxbytes);
				zshhdr(4,ZACK, Txhdr);
				goto moredata;
			case GOTCRCG:
				n = 20;
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				goto moredata;
			case GOTCRCE:
				n = 20;
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
			case '\336':
					Syslog('z', "zmputs: sleep(1)");
					sleep(1); continue;
			case '\335':
					Syslog('z', "zmputs: send break");
					sendbrk(); continue;
			default:
					PUTCHAR(c);
		}
	}
}



int resync(off_t off)
{
	return 0;
}



int closeit(int success)
{
	int rc;

	rc = closefile();
	fout = NULL;
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
		zshhdr(4,ZFIN, Txhdr);
		switch ((c=GETCHAR(10))) {
		case 'O':
			GETCHAR(1);	/* Discard 2nd 'O' */
			Syslog('z', "Zmodem: ackbibi complete");
			return ZCOMPL;
		case TERROR:
		case HANGUP:
			Syslog('z', "Zmodem: ackbibi got %d, ignore",c);
			return 0;
		case TIMEOUT:
		default:
			Syslog('z', "Zmodem: ackbibi got '%s', continue", printablec(c));
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
	register char *openmode, *p;
	static	int dummy;
	char	ctt[32];

	Syslog('z', "procheader \"%s\"",printable(Name,0));
	/* set default parameters and overrides */
	openmode = (char *)"w";

	/*
	 *  Process ZMODEM remote file management requests
	 */
	Thisbinary = (zconv != ZCNL);	/* Remote ASCII override */
	if (zmanag == ZMAPND)
		openmode = (char *)"a";

	Bytesleft = DEFBYTL; 
	Filemode = 0; 
	Modtime = 0L;

	p = Name + 1 + strlen(Name);
	sscanf(p, "%d%o%o%o%d%d%d%d", &Bytesleft, &Modtime, &Filemode, &dummy, &dummy, &dummy, &dummy, &dummy);
	strcpy(ctt,date(Modtime));
	Syslog('+', "Zmodem: \"%s\" %d bytes, %s mode %o", Name, Bytesleft, ctt, Filemode);

	fout = openfile(Name,Modtime,Bytesleft,&rxbytes,resync);
	gettimeofday(&starttime, &tz);
	sbytes = rxbytes;

	if (Bytesleft == rxbytes) {
		Syslog('+', "Zmodem: Skipping %s", Name);
		fout = NULL;
		return ZSKIP;
	} else if (!fout) 
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



int getfree(void)
{
#ifdef	__NetBSD__
        struct  statvfs sfs;
 
        if (statvfs(inbound, &sfs) != 0) {
                WriteError("$cannot statvfs \"%s\", assume enough space", inbound);
                return -1L;
        } else
                return (sfs.f_bsize * sfs.f_bfree);
#else
	struct	statfs sfs;

	if (statfs(inbound, &sfs) != 0) {
		WriteError("$cannot statfs \"%s\", assume enough space", inbound);
		return -1L;
	} else 
		return (sfs.f_bsize * sfs.f_bfree);
#endif
}


