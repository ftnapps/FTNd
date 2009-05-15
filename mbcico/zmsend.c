/*****************************************************************************
 *
 * $Id: zmsend.c,v 1.14 2005/10/11 20:49:46 mbse Exp $
 * Purpose ...............: Fidonet mailer 
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
#include "../lib/nodelist.h"
#include "ttyio.h"
#include "session.h"
#include "zmodem.h"
#include "lutil.h"
#include "emsi.h"
#include "filelist.h"

static int initsend(void);
static int sendzfile(char*,char*);
static int finsend(void);

static int getzrxinit(void);
static int sendzsinit(void);
static int zfilbuf(void);
static int zsendfile(char*,int);
static int zsendfdata(void);
static int getinsync(int);
void initzsendmsk(char *);

static FILE *in;
static int Eofseen;		/* EOF seen on input set by zfilbuf */
static int Rxflags = 0;
static int Usevhdrs;
static int Wantfcs32=TRUE;	/* Want to send 32 bit FCS */
static int Rxbuflen;
static unsigned Txwindow;	/* Control the size of the transmitted window */
static unsigned Txwspac;	/* Spacing between zcrcq requests */
static unsigned Txwcnt;		/* Counter used to space ack requests */
//static long Tframlen = 0;	/* Override for tx frame length */
static int blklen = 128;	/* Length of transmitted records */
static int blkopt;		/* Override value for zmodem blklen */
static int errors;
static int Lastsync;
static int bytcnt;
static int Lrxpos=0;
static int Lztrans=0;
static int Lzmanag=0;
static int Lskipnocor=0;
static int Lzconv=0;
static int Beenhereb4;
static char Myattn[]={0};
static int skipsize;
struct timeval	starttime, endtime;
struct timezone	tz;

extern unsigned int sentbytes;
extern int Rxhlen;
extern void get_frame_buffer(void);
extern void free_frame_buffer(void);


int zmsndfiles(file_list *lst)
{
	int		rc, maxrc = 0;
	file_list	*tmpf;

	Syslog('+', "Zmodem: start %s send", (emsi_local_protos & PROT_ZAP)?"ZedZap":"Zmodem");

	get_frame_buffer();

	if ((rc = initsend())) {
		if (txbuf)
			free(txbuf);
		txbuf = NULL;
		free_frame_buffer();
		return abs(rc);
	}

	for (tmpf = lst; tmpf && (maxrc < 2); tmpf = tmpf->next) {
		if (tmpf->remote) {
			rc = sendzfile(tmpf->local, tmpf->remote);
			rc = abs(rc);
			if (rc > maxrc) 
				maxrc = rc;
			if (rc == 0) 
				execute_disposition(tmpf);
		} else if (maxrc == 0) 
			execute_disposition(tmpf);
	}

	if (maxrc < 2) {
		rc = finsend();
		rc = abs(rc);
	}

	if (rc > maxrc) 
		maxrc = rc;

	if (txbuf)
		free(txbuf);
	txbuf = NULL;
	free_frame_buffer();

	Syslog('z', "Zmodem: send rc=%d", maxrc);
	return (maxrc < 2)?0:maxrc;
}



static int initsend(void)
{
	Syslog('z', "Zmodem: initsend");

	PUTSTR((char *)"rz\r");
	stohdr(0x80L);		/* Show we can do var header */
	zshhdr(4, ZRQINIT, Txhdr);

	if (getzrxinit()) {
		Syslog('+', "Zmodem: Unable to initiate send");
		return 1;
	}

	return 0;
}



/*
 * Say "bibi" to the receiver, try to do it cleanly
 */
static int finsend(void)
{
	int i, rc = 0;

	Syslog('z', "Zmodem: finsend");
	while (GETCHAR(1) >= 0) /*nothing*/;
	for (i = 0; i < 30; i++) {
		stohdr(0L);
		zshhdr(4, ZFIN, Txhdr);
		if ((rc = zgethdr(Rxhdr)) == ZFIN)
			PUTSTR((char *)"OO");
		if ((rc == ZFIN) || (rc == ZCAN) || (rc < 0)) 
			break;
	}
	return (rc != ZFIN);
}



static int sendzfile(char *ln, char *rn)
{
	int rc=0;
	struct stat st;
	struct flock fl;
	int bufl;
	int sverr;

	fl.l_type   = F_RDLCK;
	fl.l_whence = 0;
	fl.l_start  = 0L;
	fl.l_len    = 0L;
	if (txbuf == NULL) 
		txbuf = malloc(MAXBLOCK);

	skipsize = 0L;
	if ((in = fopen(ln, "r")) == NULL) {
		sverr = errno;
		if ((sverr == ENOENT) || (sverr == EINVAL)) {
			Syslog('+', "File %s doesn't exist, removing", MBSE_SS(ln));
			return 0;
		} else {
			WriteError("$Zmodem: cannot open file %s, skipping", MBSE_SS(ln));
			return 1;
		}
	}

	if (fcntl(fileno(in),F_SETLK,&fl) != 0) {
		Syslog('+', "$Zmodem: cannot lock file %s, skipping",MBSE_SS(ln));
		fclose(in);
		return 1;
	}

	if (stat(ln,&st) != 0) {
		Syslog('+', "$Zmodem: cannot access \"%s\", skipping",MBSE_SS(ln));
		fclose(in);
		return 1;
	}

	Syslog('+', "Zmodem: send \"%s\" as \"%s\"", MBSE_SS(ln), MBSE_SS(rn));
	Syslog('+', "Zmodem: size %lu bytes, dated %s", (unsigned int)st.st_size, date(st.st_mtime));
	gettimeofday(&starttime, &tz);

	snprintf(txbuf,MAXBLOCK, "%s %u %o %o 0 0 0", rn,
		(unsigned int)st.st_size, (int)st.st_mtime+((int)st.st_mtime%2), st.st_mode);
	bufl = strlen(txbuf);
	*(strchr(txbuf,' ')) = '\0'; /*hope no blanks in filename*/

	Eofseen = 0;
	rc = zsendfile(txbuf,bufl);
	if (rc == ZSKIP) {
		Syslog('+', "Zmodem: remote skipped %s, is OK",MBSE_SS(ln));
		return 0;
	} else if ((rc == OK) && (st.st_size - skipsize)) {
		gettimeofday(&endtime, &tz);
		Syslog('+', "Zmodem: OK %s", transfertime(starttime, endtime, (unsigned int)st.st_size - skipsize, TRUE));
		sentbytes += (unsigned int)st.st_size - skipsize;
		return 0;
	} else 
		return 2;
}



/*
 * Get the receiver's init parameters
 */
int getzrxinit(void)	// CHECKED BUT NOT WELL TESTED
{
	int	n;

	Syslog('z', "getzrxinit");
	for (n=10; --n>=0; ) {
		
		switch (zgethdr(Rxhdr)) {
		case ZCHALLENGE:	/* Echo receiver's challenge numbr */
			stohdr(Rxpos);
			zshhdr(4, ZACK, Txhdr);
			continue;
		case ZCOMMAND:		/* They didn't see out ZRQINIT */
			stohdr(0L);
			zshhdr(4, ZRQINIT, Txhdr);
			continue;
		case ZRINIT:
			Rxflags = 0377 & Rxhdr[ZF0];
			Usevhdrs = Rxhdr[ZF1] & CANVHDR;
			Txfcs32 = (Wantfcs32 && (Rxflags & CANFC32));
			Zctlesc |= Rxflags & TESCCTL;
			Rxbuflen = (0377 & Rxhdr[ZP0])+((0377 & Rxhdr[ZP1])<<8);
			if ( !(Rxflags & CANFDX))
				Txwindow = 0;
			Syslog('z', "Zmodem: Remote allowed Rxbuflen=%d", Rxbuflen);

			/* Set initial subpacket length */
			if (blklen < 1024) {	/* Command line override? */
				blklen = 1024;
			}
			if (Rxbuflen && blklen>Rxbuflen)
				blklen = Rxbuflen;
			if (blkopt && blklen > blkopt)
				blklen = blkopt;
			Syslog('z', "Rxbuflen=%d blklen=%d", Rxbuflen, blklen);
			Syslog('z', "Txwindow = %u Txwspac = %d", Txwindow, Txwspac);

			if (Lztrans == ZTRLE && (Rxflags & CANRLE))
				Txfcs32 = 2;
			else
				Lztrans = 0;

			return (sendzsinit());
		case ZCAN:
		case TIMEOUT:
			return TERROR;
		case HANGUP:
			return HANGUP;
		case ZRQINIT:
			if (Rxhdr[ZF0] == ZCOMMAND)
				continue;
		default:
			zshhdr(4, ZNAK, Txhdr);
			continue;
		}
	}
	return TERROR;
}



/*
 * Send send-init information
 */
int sendzsinit(void)
{
	int	c;

	if (Myattn[0] == '\0' && (!Zctlesc || (Rxflags & TESCCTL)))
		return OK;
	errors = 0;
	for (;;) {
		stohdr(0L);
		if (Zctlesc) {
			Txhdr[ZF0] |= TESCCTL; zshhdr(4, ZSINIT, Txhdr);
		}
		else
			zsbhdr(4, ZSINIT, Txhdr);
		zsdata(Myattn, ZATTNLEN, ZCRCW);
		c = zgethdr(Rxhdr);
		switch (c) {
		case ZCAN:
			return TERROR;
		case HANGUP:
			return HANGUP;
		case ZACK:
			return OK;
		default:
			if (++errors > 19)
				return TERROR;
			continue;
		}
	}
}



/*
 * Fill buffer with blklen chars
 */
int zfilbuf(void)
{
	int n;

	n = fread(txbuf, 1, blklen, in);
	if (n < blklen) {
		Eofseen = 1;
		Syslog('z', "zfilbuf return %d", n);
	}

	return n;
}



/*
 * Send file name and related info
 */
int zsendfile(char *buf, int blen)
{
	int	c;
	register unsigned int crc = -1;
	int lastcrcrq = -1;

	Syslog('z', "zsendfile %s (%d)", buf, blen);
	for (errors=0; ++errors<11;) {
		Txhdr[ZF0] = Lzconv;	/* file conversion request */
		Txhdr[ZF1] = Lzmanag;	/* file management request */
		if (Lskipnocor)
			Txhdr[ZF1] |= ZMSKNOLOC;
		Txhdr[ZF2] = Lztrans;	/* file transport request */
		Txhdr[ZF3] = 0;
		zsbhdr(4, ZFILE, Txhdr);
		zsdata(buf, blen, ZCRCW);
again:
		c = zgethdr(Rxhdr);
		switch (c) {
		case ZRINIT:
			while ((c = GETCHAR(5)) > 0)
				if (c == ZPAD) {
					goto again;
				}
			continue;
		case ZCAN:
		case TIMEOUT:
		case ZABORT:
		case ZFIN:
			Syslog('+', "Zmodem: Got %s on pathname", frametypes[c+FTOFFSET]);
			return TERROR;
		default:
			Syslog('+', "Zmodem: Got %d frame type on pathname", c);
			continue;
		case TERROR:
		case ZNAK:
			continue;
		case ZCRC:
			if (Rxpos != lastcrcrq) {
				lastcrcrq = Rxpos;
				crc = 0xFFFFFFFFL;
				fseek(in, 0L, 0);
				while (((c = getc(in)) != EOF) && --lastcrcrq)
					crc = updcrc32(c, crc);
				crc = ~crc;
				clearerr(in);	/* Clear possible EOF */
				lastcrcrq = Rxpos;
			}
			stohdr(crc);
			zsbhdr(4, ZCRC, Txhdr);
			goto again;
		case ZFERR:
		case ZSKIP:
			Syslog('+', "Zmodem: File skipped by receiver request");
			fclose(in); return c;
		case ZRPOS:
			/*
			 * Suppress zcrcw request otherwise triggered by
			 * lastyunc==bytcnt
			 */
			if (Rxpos > 0)
				skipsize = Rxpos;
			if (fseek(in, Rxpos, 0))
				return TERROR;
			Lastsync = (bytcnt = Txpos = Lrxpos = Rxpos) -1;
			return zsendfdata();
		}
	}
	fclose(in); return TERROR;
}



/* 
 * Send the data in the file
 */
int zsendfdata(void)
{
	int	c=0, e, n;
	int	newcnt;
	int	tcount = 0;
	int	junkcount; /* Counts garbage chars received by TX */
	int	maxblklen, goodblks = 0, goodneeded = 8;

	if (emsi_local_protos & PROT_ZAP) 
		maxblklen = MAXBLOCK;
	else 
		maxblklen = 1024;
	Syslog('z', "zsendfdata() maxblklen=%d", maxblklen);

	junkcount = 0;
	Beenhereb4 = 0;
somemore:
	if (0) {
waitack:
		junkcount = 0;
		c = getinsync(0);
gotack:
		switch (c) {
		default:
		case ZCAN:
			fclose(in);
			return TERROR;
		case ZRINIT:
			fclose(in);
			return ZSKIP;
		case ZSKIP:
			fclose(in);
			return c;
		case ZACK:
			break;	// Possible bug, added 23-08-99
		case ZRPOS:
			blklen = ((blklen >> 2) > 64) ? (blklen >> 2) : 64;
			goodblks = 0;
			goodneeded = ((goodneeded << 1) > 16) ? 16 : goodneeded << 1;
			Syslog('z', "zmsend: blocklen now %d", blklen);
			break;
		case TIMEOUT: /* Put back here 08-09-1999 mb */
			Syslog('z', "zmsend: zsendfdata TIMEOUT");
			goto to;
		case HANGUP: /* New, added 08-09-1999 mb */
			Syslog('z', "zmsend: zsendfdata HANGUP");
			fclose(in);
			return c;
		}
		/*
		 * If the reverse channel can be tested for data,
		 *  this logic may be used to detect error packets
		 *  sent by the receiver, in place of setjmp/longjmp
		 *  rdchk(fd) returns non 0 if a character is available
		 */
		if (TCHECK()) {
			c = GETCHAR(1);
			Syslog('z', "zsendfdata(): check getchar(1)=%d", c);
			if (c < 0) {
				return c;
			} else switch (c) {
			case CAN:
			case ZPAD:
				c = getinsync(1);
				goto gotack;
			case XOFF:		/* Wait a while for an XON */
			case XOFF|0200:
				GETCHAR(10);
			}
		}
	}
to:
	newcnt = Rxbuflen;
	Txwcnt = 0;
	stohdr(Txpos);
	zsbhdr(4, ZDATA, Txhdr);

	do {
		n = zfilbuf();
		if (Eofseen)
			e = ZCRCE;
		else if (junkcount > 3)
			e = ZCRCW;
		else if (bytcnt == Lastsync)
			e = ZCRCW;
		else if (Rxbuflen && (newcnt -= n) <= 0)
			e = ZCRCW;
		else if (Txwindow && (Txwcnt += n) >= Txwspac) {
			Txwcnt = 0;  e = ZCRCQ;
		} else
			e = ZCRCG;
		Syslog('z', "%7ld ZMODEM%s    ",
			  Txpos, Crc32t?" CRC-32":"");
		Nopper();
		zsdata(txbuf, n, e);
		bytcnt = Txpos += n;

		if ((blklen < maxblklen) && (++goodblks > goodneeded)) {
			blklen = ((blklen << 1) < maxblklen) ? blklen << 1 : maxblklen;
			goodblks = 0;
			Syslog('z', "zmsend: blocklen now %d", blklen);
		}

		if (e == ZCRCW)
			goto waitack;
		/*
		 * If the reverse channel can be tested for data,
		 *  this logic may be used to detect error packets
		 *  sent by the receiver, in place of setjmp/longjmp
		 *  rdchk(fd) returns non 0 if a character is available
		 */
		if (TCHECK()) {
			c = GETCHAR(1);
			if (c < 0) {
				return c;
			} else switch (c) {
			case CAN:
			case ZPAD:
				c = getinsync(1);
				if (c == ZACK)
					break;
				/* zcrce - dinna wanna starta ping-pong game */
				zsdata(txbuf, 0, ZCRCE);
				goto gotack;
			case XOFF:		/* Wait a while for an XON */
			case XOFF|0200:
				GETCHAR(10);
			default:
				++junkcount;
			}
		}
		if (Txwindow) {
			while ((tcount = (Txpos - Lrxpos)) >= Txwindow) {
				Syslog('z', "%ld window >= %u", tcount, Txwindow);
				if (e != ZCRCQ)
					zsdata(txbuf, 0, e = ZCRCQ);
				c = getinsync(1);
				if (c != ZACK) {
					zsdata(txbuf, 0, ZCRCE);
					goto gotack;
				}
			}
			Syslog('z', "window = %ld", tcount);
		}
	} while (!Eofseen);

	for (;;) {
		stohdr(Txpos);
		zsbhdr(4, ZEOF, Txhdr);
egotack:
		switch (getinsync(0)) {
		case ZACK:
			Syslog('z', "zsendfdata() ZACK");
			goto egotack;	// continue in old source
		case ZRPOS:
			goto somemore;
		case ZRINIT:
			fclose(in);
			return OK;
		case ZSKIP:
			fclose(in);
			Syslog('+', "Zmodem: File skipped by receiver request");
			return ZSKIP;
		default:
			Syslog('+', "Zmodem: Got %d trying to send end of file", c);
		case TERROR:
			fclose(in);
			return TERROR;
		}
	}
}



/*
 * Respond to receiver's complaint, get back in sync with receiver
 */
int getinsync(int flag)
{
	int	c = 0;

	Syslog('z', "getinsync(%d)", flag);

	for (;;) {
		c = zgethdr(Rxhdr);
		switch (c) {
		case HANGUP:
			return HANGUP;
		case ZCAN:
		case ZABORT:
		case ZFIN:
		case TERROR:
		case TIMEOUT:
			Syslog('+', "Zmodem: Got %s sending data", frametypes[c+FTOFFSET]);
			return TERROR;
		case ZRPOS:
			/* ************************************* */
			/*  If sending to a buffered modem, you  */
			/*   might send a break at this point to */
			/*   dump the modem's buffer.		 */
			clearerr(in);	/* In case file EOF seen */
			if (fseek(in, Rxpos, 0)) {
				Syslog('+', "Zmodem: Bad Seek to %ld", Rxpos);
				return TERROR;
			}
			Eofseen = 0;
			bytcnt = Lrxpos = Txpos = Rxpos;
			if (Lastsync == Rxpos) {
				if (++Beenhereb4 > 12) {
					Syslog('+', "Zmodem: Can't send block");
					return TERROR;
				}
				if (Beenhereb4 > 4)
					if (blklen > 32) {
						blklen /= 2;
						Syslog('z', "Zmodem: blocklen now %d", blklen);
					}
			}
			else 
				Beenhereb4=0;
			Lastsync = Rxpos;
			return c;
		case ZACK:
			Lrxpos = Rxpos;
			if (flag || Txpos == Rxpos)
				return ZACK;
			continue;
		case ZRINIT:
			return c;
		case ZSKIP:
			Syslog('+', "Zmodem: File skipped by receiver request");
			return c;
		default:
			zsbhdr(4, ZNAK, Txhdr);
			continue;
		}
	}
}



/*
 * Set additional control chars to mask in Zsendmask
 * according to bit array stored in char array at p
 */
void initzsendmsk(register char *p)
{
        int	c;

        for (c = 0; c < 33; ++c) {
                if (p[c>>3] & (1 << (c & 7))) {
                        Zsendmask[c] = 1;
                        Syslog('z', "Zmodem: Escaping %02o", c);
                }
        }
}


