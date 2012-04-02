/*****************************************************************************
 *
 * Purpose ...............: Fidonet mailer
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
#include "session.h"
#include "ttyio.h"
#include "statetbl.h"
#include "config.h"
#include "lutil.h"
#include "openfile.h"
#include "m7recv.h"
#include "xmrecv.h"
#include "filelist.h"
#include "filetime.h"


#define XMBLKSIZ 128


static int	xm_recv(void);
static int	resync(off_t);
static int	closeit(int);

static char	*recvname=NULL;
static char	*fpath=NULL;
static FILE	*fp=NULL;
static int	last;
struct timeval	starttime, endtime;
struct timezone	tz;
static off_t	startofs;
static int	recv_blk;

extern unsigned int rcvdbytes;



int xmrecv(char *Name)
{
	int rc;

	Syslog('+', "Xmodem start receive \"%s\"",MBSE_SS(Name));
	recvname = Name;
	last = 0;
	rc = xm_recv();
	if (fp) 
		closeit(0);
	if (rc) 
		return -1;
	else 
		if (last) 
			return 1;
		else 
			return 0;
}



int closeit(int success)
{
	off_t	endofs;

	endofs = recv_blk*XMBLKSIZ;
	gettimeofday(&endtime, &tz);
	if (success)
	    Syslog('+', "Xmodem: OK %s", transfertime(starttime, endtime, endofs-startofs, FALSE));
	else
	    Syslog('+', "Xmodem: dropped after %ld bytes", endofs-startofs);
	rcvdbytes += (unsigned int)(endofs-startofs);
	fp = NULL;
	return closefile();
}



SM_DECL(xm_recv,(char *)"xmrecv")
SM_STATES
	sendnak0,
	waitblk0,
	sendnak,
	waitblk,
	recvblk,
	sendack,
	checktelink,
	recvm7,
	goteof
SM_NAMES
	(char *)"sendnak0",
	(char *)"waitblk0",
	(char *)"sendnak",
	(char *)"waitblk",
	(char *)"recvblk",
	(char *)"sendack",
	(char *)"checktelink",
	(char *)"recvm7",
	(char *)"goteof"
SM_EDECL

	int		tmp, i;
	int		SEAlink = FALSE;
	int		crcmode = session_flags & FTSC_XMODEM_CRC;
	int		count=0,junk=0,cancount=0;
	int		header = 0;
	struct _xmblk {
		unsigned char	n1,n2;
		unsigned char	data[XMBLKSIZ];
		unsigned char	c1,c2;
	} xmblk;
	unsigned short	localcrc,remotecrc;
	unsigned char	localcs,remotecs;
	int		ackd_blk=-1L;
	int		next_blk=1L;
	int		last_blk=0L;
	off_t		resofs;
	char		tmpfname[16];
	off_t		wsize;
	time_t		remtime=0L;
	off_t		remsize=0;
	int		goteot = FALSE;

	gettimeofday(&starttime, &tz);
	recv_blk=-1L;

	memset(&tmpfname, 0, sizeof(tmpfname));
	if (recvname) 
		strncpy(tmpfname,recvname,sizeof(tmpfname)-1);

SM_START(sendnak0)

SM_STATE(sendnak0)

	Syslog('x', "xmrecv SENDNAK0 count=%d mode=%s", count, crcmode?"crc":"cksum");
	if (count++ > 9) {
		Syslog('+', "too many errors while xmodem receive init");
		SM_ERROR;
	}
	if ((ackd_blk < 0) && crcmode && (count > 5)) {
		Syslog('x', "no responce to 'C', try checksum mode");
		session_flags &= ~FTSC_XMODEM_CRC;
		crcmode = FALSE;
	}

	if (crcmode) 
		PUTCHAR('C');
	else 
		PUTCHAR(NAK);
	junk = 0;
	SM_PROCEED(waitblk0);

SM_STATE(waitblk0)

	header = GETCHAR(5);
	if (header == TIMEOUT) {
		Syslog('x', "timeout waiting for xmodem block 0 header, count=%d", count);
		if ((count > 2) && (session_flags & SESSION_IFNA)) {
			Syslog('+', "Timeout waiting for file in WaZOO session, report success");
			last=1;
			SM_SUCCESS;
		}
		SM_PROCEED(sendnak0);
	} else if (header < 0) {
		Syslog('x', "Error");
		SM_ERROR;
	} else {
		switch (header) {
		case EOT:	Syslog('x', "got EOT");
				if (ackd_blk == -1L)
					last=1;
				else {
					ackd_blk++;
					PUTCHAR(ACK);
					if (SEAlink) {
						PUTCHAR(ackd_blk);
						PUTCHAR(~ackd_blk);
					}
				}
				if (STATUS) {
					SM_ERROR;
				}
				SM_SUCCESS;
				break;
		case CAN:	Syslog('+', "Got CAN while xmodem receive init");
				SM_ERROR;
				break;
		case SOH:	SEAlink = TRUE;
				Syslog('x', "Got SOH, SEAlink mode");
				SM_PROCEED(recvblk);
				break;
		case SYN:	SEAlink = FALSE;
				Syslog('x', "Got SYN, Telink mode");
				SM_PROCEED(recvblk);
				break;
		case ACK:	SEAlink = FALSE;
				Syslog('x', "Got ACK, Modem7 mode");
				SM_PROCEED(recvm7);
				break;
		case TSYNC:	Syslog('x', "Got TSYSNC char");
				SM_PROCEED(sendnak0);
				break;
		case NAK:
		case 'C':	Syslog('x', "Got %s waiting for block 0, sending EOT", printablec(header));
				PUTCHAR(EOT); /* other end still waiting us to send? */
				SM_PROCEED(waitblk0);
				break;
		default:	Syslog('x', "Got '%s' waiting for block 0", printablec(header));
				if (junk++ > 300) {
					SM_PROCEED(sendnak0);
				} else {
					SM_PROCEED(waitblk0);
				}
				break;
		}
	}

SM_STATE(sendnak)

	if (ackd_blk < 0) {
		SM_PROCEED(sendnak0);
	}

	if (count++ > 9) {
		Syslog('+', "too many errors while xmodem receive");
		SM_ERROR;
	}

	junk = 0;

	if (remote_flags & FTSC_XMODEM_RES) {
		if (resync(ackd_blk*XMBLKSIZ)) {
			SM_ERROR;
		} else {
			SM_PROCEED(waitblk);
		}
	} else { /* simple NAK */
		Syslog('x', "negative acknowlege block %ld",ackd_blk+1);

		if (crcmode)
			PUTCHAR('C');
		else
			PUTCHAR(NAK);
		if (SEAlink) {
			PUTCHAR(ackd_blk+1);
			PUTCHAR(~(ackd_blk+1));
		}
		if (STATUS) {
			SM_ERROR;
		} else {
			SM_PROCEED(waitblk);
		}
	}

SM_STATE(sendack)

	Syslog('x', "xmrecv SENDACK block=%d", recv_blk);
	ackd_blk = recv_blk;
	count = 0;
	cancount = 0;

	PUTCHAR(ACK);
	if (SEAlink) {
		PUTCHAR(ackd_blk);
		PUTCHAR(~ackd_blk);
	}
	if (STATUS) {
		SM_ERROR;
	}
	if (goteot) {
		SM_SUCCESS;
	}
	SM_PROCEED(waitblk);

SM_STATE(waitblk)

	header = GETCHAR(15);
	if (header == TIMEOUT) {
		Syslog('x', "timeout waiting for xmodem block header, count=%d", count);
		SM_PROCEED(sendnak);
	} else if (header < 0) {
		SM_ERROR;
	} else {
		switch (header) {
		case EOT:	if (last_blk && (ackd_blk != last_blk)) {
					Syslog('x', "false EOT after %ld block, need after %ld", ackd_blk,last_blk);
					SM_PROCEED(waitblk);
				} else {
					SM_PROCEED(goteof);
				}
				break;
		case CAN:	if (cancount++ > 4) {
					closeit(0);
					Syslog('+', "Got CAN while xmodem receive");
					SM_ERROR;
				} else {
					SM_PROCEED(waitblk);
				}
				break;
		case SOH:	SM_PROCEED(recvblk);
				break;
		default:	Syslog('x', "got '%s' waiting SOH", printablec(header));
				if (junk++ > 200) {
					SM_PROCEED(sendnak);
				} else {
					SM_PROCEED(waitblk);
				}
				break;
		}
	}

SM_STATE(recvblk)

	Nopper();
	GET((char*)&xmblk,(crcmode && (header != SYN))?  sizeof(xmblk): sizeof(xmblk)-1,15);
	if (STATUS == STAT_TIMEOUT) {
		Syslog('x', "xmrecv timeout waiting for block body");
		SM_PROCEED(sendnak);
	}
	if (STATUS) {
		SM_ERROR;
	}
	if ((xmblk.n1 & 0xff) != ((~xmblk.n2) & 0xff)) {
		Syslog('x', "bad block number: 0x%02x/0x%02x (0x%02x)", xmblk.n1,xmblk.n2,(~xmblk.n2)&0xff);
		SM_PROCEED(waitblk);
	}
	recv_blk = xmblk.n1 + (ackd_blk & ~0xff);
	if (abs(recv_blk - ackd_blk) > 128)
		recv_blk += 256;

	if (crcmode && (header != SYN)) {
		remotecrc = (short)xmblk.c1 << 8 | xmblk.c2;
		localcrc = crc16xmodem((char *)xmblk.data, sizeof(xmblk.data));
		if (remotecrc != localcrc) {
			Syslog('x', "bad crc: 0x%04x/0x%04x",remotecrc,localcrc);
			if (recv_blk == (ackd_blk+1)) {
				SM_PROCEED(sendnak);
			} else {
				SM_PROCEED(waitblk);
			}
		}
	} else {
		remotecs = xmblk.c1;
		localcs = checksum((char *)xmblk.data, sizeof(xmblk.data));
		if (remotecs != localcs) {
			Syslog('x', "bad checksum: 0x%02x/0x%02x",remotecs,localcs);
			if (recv_blk == (ackd_blk+1)) {
				SM_PROCEED(sendnak);
			} else {
				SM_PROCEED(waitblk);
			}
		}
	}
	if ((ackd_blk == -1L) && (recv_blk == 0L)) {
		SM_PROCEED(checktelink);
	}
	if ((ackd_blk == -1L) && (recv_blk == 1L)) {
		if (count < 3) {
			SM_PROCEED(sendnak0);
		} else 
			ackd_blk=0L;
	}
	if (recv_blk < (ackd_blk+1L)) {
		Syslog('x', "old block number %ld after %ld, go on", recv_blk,ackd_blk);
		SM_PROCEED(waitblk);
	} else if (recv_blk > (ackd_blk+1L)) {
		Syslog('x', "bad block order: %ld after %ld, go on", recv_blk,ackd_blk);
		SM_PROCEED(waitblk);
	}

	if (fp == NULL) {
		if ((fp = openfile(tmpfname,remtime,remsize,&resofs,resync)) == NULL) {
			SM_ERROR;
		} else {
			if (resofs) 
				ackd_blk=(resofs-1)/XMBLKSIZ+1L;
			else 
				ackd_blk=-1L;
		}
		startofs=resofs;
		Syslog('+', "Xmodem receive: \"%s\"",tmpfname);
	}
	
	if (recv_blk > next_blk) {
		WriteError("xmrecv internal error: recv_blk %ld > next_blk %ld", recv_blk,next_blk);
		SM_ERROR;
	}
	if (recv_blk == next_blk) {
		if (recv_blk == last_blk) 
			wsize=remsize%XMBLKSIZ;
		else 
			wsize=XMBLKSIZ;
		if (wsize == 0) 
			wsize=XMBLKSIZ;
		if ((tmp = fwrite(xmblk.data,wsize,1,fp)) != 1) {
			WriteError("$error writing block %l (%d bytes) to file \"%s\" (fwrite return %d)",
				recv_blk,wsize,fpath,tmp);
			SM_ERROR;
		} else 
			Syslog('x', "Block %ld size %d written (ret %d)", recv_blk,wsize,tmp);
		next_blk++;
	} else {
		Syslog('x', "recv_blk %ld < next_blk %ld, ack without writing", recv_blk,next_blk);
	}
	SM_PROCEED(sendack);

SM_STATE(checktelink)

	Syslog('x', "checktelink got \"%s\"",printable((char *)xmblk.data,45));
	if (tmpfname[0] == '\0') {
		strncpy(tmpfname,(char *)xmblk.data+8,16);
		/*
		 *  Some systems fill the rest of the filename with spaces, sigh.
		 */
		for (i = 16; i; i--) {
			if ((tmpfname[i] == ' ') || (tmpfname[i] == '\0'))
				tmpfname[i] = '\0';
			else
				break;
		}
	} else {
		Syslog('+', "Remote uses %s",printable((char *)xmblk.data+25,-14));
		Syslog('x', "Remote file name \"%s\" discarded", printable((char *)xmblk.data+8,-16));
	}
	remsize = ((off_t)xmblk.data[0]) + ((off_t)xmblk.data[1]<<8) + ((off_t)xmblk.data[2]<<16) + ((off_t)xmblk.data[3]<<24);
	last_blk = (remsize-1)/XMBLKSIZ+1;
	if (header == SOH) {
		/*
		 * SEAlink block
		 */
		remtime=sl2mtime(((time_t)xmblk.data[4])+ ((time_t)xmblk.data[5]<<8)+
			((time_t)xmblk.data[6]<<16)+ ((time_t)xmblk.data[7]<<24));
		if (xmblk.data[40]) {
			remote_flags |= FTSC_XMODEM_SLO;
		} else 
			remote_flags &= ~FTSC_XMODEM_SLO;
		if (xmblk.data[41]) 
			remote_flags |= FTSC_XMODEM_RES;
		else 
			remote_flags &= ~FTSC_XMODEM_RES;
		if (xmblk.data[42]) 
			remote_flags |= FTSC_XMODEM_XOF;
		else 
			remote_flags &= ~FTSC_XMODEM_XOF;
	} else if (header == SYN) {
		/*
		 * Telink block
		 */
		remtime=tl2mtime(((time_t)xmblk.data[4])+ ((time_t)xmblk.data[5]<<8)+
			((time_t)xmblk.data[6]<<16)+ ((time_t)xmblk.data[7]<<24));
		if (xmblk.data[41]) 
			session_flags |= FTSC_XMODEM_CRC;
		else 
			session_flags &= ~FTSC_XMODEM_CRC;
	} else {
		WriteError("Got data block with header 0x%02x", header);
		SM_PROCEED(sendnak0);
	}

	Syslog('x', "%s block, session_flags=0x%04x, remote_flags=0x%04x",
		(header == SYN)?"Telink":"Sealink",session_flags,remote_flags);

	if ((fp = openfile(tmpfname,remtime,remsize,&resofs,resync)) == NULL) {
		SM_ERROR;
	}
	if (resofs) 
		ackd_blk=(resofs-1)/XMBLKSIZ+1L;
	else 
		ackd_blk=-1L;
	startofs=resofs;

	Syslog('+', "Xmodem %s receive: \"%s\" %ld bytes dated %s", (header == SYN)?"Telink":"Sealink",
		tmpfname, remsize, rfcdate(remtime));

	if (ackd_blk == -1) {
		SM_PROCEED(sendack);
	} else {
		SM_PROCEED(waitblk);
	}

SM_STATE(recvm7)

	switch (m7recv(tmpfname)) {
		case 0:		ackd_blk=0; 
				SM_PROCEED(sendnak); 
				break;
		case 1:		last=1; 
				SM_SUCCESS; 
				break;
		default: 	SM_PROCEED(sendnak);
	}

SM_STATE(goteof)

	closeit(1);
	if (ackd_blk == -1L) 
		last=1;
	else {
		ackd_blk++;
		PUTCHAR(ACK);
		if (SEAlink) {
			PUTCHAR(ackd_blk);
			PUTCHAR(~ackd_blk);
		}
	}
	if (STATUS) {
		SM_ERROR;
	}
	SM_SUCCESS;

SM_END
SM_RETURN



int resync(off_t resofs)
{
	char	resynbuf[16];
	short	lcrc;
	int	count=0;
	int	gotack,gotnak;
	int	c;
	int	sblk;

	Syslog('x', "trying to resync at offset %d", (int)resofs);

	sblk=resofs/XMBLKSIZ+1;
	snprintf(resynbuf,16,"%d",sblk);
	lcrc=crc16xmodem(resynbuf,strlen(resynbuf));
	gotack=0;
	gotnak=0;

	do {
		count++;
		PUTCHAR(SYN);
		PUTSTR(resynbuf);
		PUTCHAR(ETX);
		PUTCHAR(lcrc&0xff);
		PUTCHAR(lcrc>>8);
		do {
			if ((c=GETCHAR(5)) == ACK) {
				if ((c=GETCHAR(1)) == SOH) 
					gotack=1;
				UNGETCHAR(c);
			} else if (c == NAK) {
				if ((c=GETCHAR(1)) == TIMEOUT) 
					gotnak=1;
				UNGETCHAR(c);
			}
		}
		while (!gotack && !gotnak && (c >= 0));
		if ((c < 0) && (c != TIMEOUT)) 
			return 1;
	}
	while (!gotack && !gotnak && (count < 6));

	if (gotack) {
		Syslog('x', "resyncing at offset %ld",resofs);
		return 0;
	} else {
		Syslog('+', "sealink resync at offset %ld failed",resofs);
		return 1;
	}
}


