/*****************************************************************************
 *
 * $Id: xmsend.c,v 1.11 2005/10/11 20:49:46 mbse Exp $
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
#include "session.h"
#include "ttyio.h"
#include "statetbl.h"
#include "xmsend.h"
#include "m7send.h"
#include "filelist.h"
#include "filetime.h"


#define XMBLKSIZ 128
#define DEFAULT_WINDOW 127


static char	*ln,*rn;
static int	flg;
static int	xm_send(void);

extern unsigned int	sentbytes;


int xmsend(char *local, char *Remote, int fl)
{
	int rc;

	ln=local;
	rn=Remote;
	flg=fl;
	rc=xm_send();
	if (rc) 
		Syslog('+', "xmsend failed");
	return rc;
}



SM_DECL(xm_send,(char *)"xmsend")
SM_STATES
	sendm7,
	sendblk0,
	waitack0,
	sendblk,
	writeblk,
	waitack,
	resync,
	sendeot
SM_NAMES
	(char *)"sendm7",
	(char *)"sendblk0",
	(char *)"waitack0",
	(char *)"sendblk",
	(char *)"writeblk",
	(char *)"waitack",
	(char *)"resync",
	(char *)"sendeot"
SM_EDECL

	FILE		*fp;
	struct		stat st;
	struct		flock fl;
	unsigned short 	lcrc=0,rcrc;
	int		startstate;
	int		crcmode,seamode,telink;
	int		a,a1,a2;
	int		i;
	time_t		seatime;
	struct timeval	starttime, endtime;
	struct timezone	tz;
	unsigned char	header=SOH;
	struct _xmblk {
		unsigned char	n1;
		unsigned char	n2;
		char		data[XMBLKSIZ];
		unsigned char	c1;
		unsigned char	c2;
	} xmblk;
	int		count=0;
	int		cancount=0;
	int		window;
	int		last_blk;
	int		send_blk;
	int		next_blk;
	int		ackd_blk;
	int		tmp;
	char		resynbuf[16];

	fl.l_type=F_RDLCK;
	fl.l_whence=0;
	fl.l_start=0L;
	fl.l_len=0L;

	gettimeofday(&starttime, &tz);

	/* if we got 'C' than hopefully remote is sealink capable... */

	if (session_flags & FTSC_XMODEM_CRC) {
		telink=0;
		crcmode=1;
		session_flags |= FTSC_XMODEM_RES;
		session_flags |= FTSC_XMODEM_SLO;
		session_flags |= FTSC_XMODEM_XOF;
		window=DEFAULT_WINDOW;
		send_blk=0L;
		next_blk=0L;
		ackd_blk=-1L;
		startstate=sendblk0;
	} else {
		telink=1;
		crcmode=0;
		session_flags &= ~FTSC_XMODEM_RES;
		session_flags |= FTSC_XMODEM_SLO;
		session_flags |= FTSC_XMODEM_XOF;
		window=1;
		send_blk=0L;
		next_blk=0L;
		ackd_blk=-1L;
		if (flg && !(session_flags & SESSION_IFNA))
			startstate = sendm7;
		else 
			startstate = sendblk0;
	}

	seamode=-1; /* not yet sure about numbered ACKs */

	if (stat(ln,&st) != 0) {
		WriteError("$cannot stat local file \"%s\" to send",MBSE_SS(ln));
		return 1;
	}
	last_blk=(st.st_size-1)/XMBLKSIZ+1;

	if ((fp=fopen(ln,"r")) == NULL) {
		WriteError("$cannot open local file \"%s\" to send",MBSE_SS(ln));
		return 1;
	}
	fl.l_pid = getpid();
	if (fcntl(fileno(fp),F_SETLK,&fl) != 0) {
		WriteError("$cannot lock local file \"%s\" to send, skip it",MBSE_SS(ln));
		return 0;
	}
	if (stat(ln,&st) != 0) {
		WriteError("$cannot access local file \"%s\" to send, skip it",MBSE_SS(ln));
		return 0;
	}

	Syslog('+', "Xmodem send \"%s\" as \"%s\", size=%lu", MBSE_SS(ln),MBSE_SS(rn),(unsigned long)st.st_size);
	sentbytes += (unsigned long)st.st_size;

SM_START(startstate)

SM_STATE(sendm7)

	if (m7send(rn)) {
		SM_PROCEED(sendblk0);
	} else {
		SM_ERROR;
	}

SM_STATE(sendblk0)

	Syslog('x', "xmsendblk0 send:%ld, next:%ld, ackd:%ld, last:%ld", send_blk,next_blk,ackd_blk,last_blk);

	memset(xmblk.data,0,sizeof(xmblk.data));

	xmblk.data[0]=(st.st_size)&0xff;
	xmblk.data[1]=(st.st_size>>8)&0xff;
	xmblk.data[2]=(st.st_size>>16)&0xff;
	xmblk.data[3]=(st.st_size>>24)&0xff;
	seatime=mtime2sl(st.st_mtime);
	xmblk.data[4]=(seatime)&0xff;
	xmblk.data[5]=(seatime>>8)&0xff;
	xmblk.data[6]=(seatime>>16)&0xff;
	xmblk.data[7]=(seatime>>24)&0xff;
	strncpy(xmblk.data+8,rn,17);
	if (telink) 
		for (i=23;(i>8) && (xmblk.data[i] == '\0');i--)
			xmblk.data[i]=' ';
	snprintf(xmblk.data+25, 15, "mbcico %s", VERSION);
	xmblk.data[40]=((session_flags & FTSC_XMODEM_SLO) != 0);
	xmblk.data[41]=((session_flags & FTSC_XMODEM_RES) != 0);
	xmblk.data[42]=((session_flags & FTSC_XMODEM_XOF) != 0);

	Syslog('x', "sealink block: \"%s\"",printable(xmblk.data,44));

	next_blk=send_blk+1;
	SM_PROCEED(sendblk);

SM_STATE(sendblk)

	Syslog('x', "xmsendblk send:%ld, next:%ld, ackd:%ld, last:%ld", send_blk,next_blk,ackd_blk,last_blk);
	if (send_blk == 0) {
		SM_PROCEED(writeblk);
	}

	if (send_blk > last_blk) {
		Syslog('x', "send_blk > last_blk");
		if (send_blk == (last_blk+1)) {
			SM_PROCEED(sendeot);
		} else if (ackd_blk < last_blk) {
			SM_PROCEED(waitack);
		} else {
			gettimeofday(&endtime, &tz);
			Syslog('+', "Xmodem: OK %s", transfertime(starttime, endtime, st.st_size, TRUE));
			sentbytes += (unsigned long)st.st_size;
			fclose(fp);
			SM_SUCCESS;
		}
	}

	memset(xmblk.data, SUB, sizeof(xmblk.data));

	if (send_blk != next_blk)
		if (fseek(fp,(send_blk-1)*XMBLKSIZ,SEEK_SET) != 0) {
			WriteError("$fseek error setting block %ld (byte %lu) in file \"%s\"", 
				send_blk,(send_blk-1)*XMBLKSIZ,MBSE_SS(ln));
			SM_ERROR;
		}
	if (fread(xmblk.data,1,XMBLKSIZ,fp) <= 0) {
		WriteError("$read error for block %lu in file \"%s\"", send_blk,MBSE_SS(ln));
		SM_ERROR;
	}
	next_blk=send_blk+1;

	SM_PROCEED(writeblk);

SM_STATE(writeblk)

	Nopper();
	xmblk.n1=send_blk&0xff;
	xmblk.n2=~xmblk.n1;
	if (crcmode) {
		lcrc=crc16xmodem(xmblk.data,sizeof(xmblk.data));
		xmblk.c1=(lcrc>>8)&0xff;
		xmblk.c2=lcrc&0xff;
	} else {
		xmblk.c1=checksum(xmblk.data,sizeof(xmblk.data));
	}
	
	PUTCHAR(header);
	PUT((char*)&xmblk,crcmode?sizeof(xmblk):sizeof(xmblk)-1);
	if (STATUS) {
		SM_ERROR;
	}
	if (crcmode)
		Syslog('x', "sent '0x%02x',no 0x%02x, %d bytes crc 0x%04x", header, xmblk.n1, XMBLKSIZ, lcrc);
	else
		Syslog('x', "sent '0x%02x',no 0x%02x, %d bytes checksum 0x%02x", header, xmblk.n1, XMBLKSIZ, xmblk.c1);
	send_blk++;
	SM_PROCEED(waitack);

SM_STATE(waitack)

	if ((count > 4) && (ackd_blk < 0)) {
		Syslog('+', "Cannot send sealink block, try xmodem");
		window=1;
		ackd_blk++;
		SM_PROCEED(sendblk);
	}
	if (count > 9) {
		Syslog('+', "Too many errors in xmodem send");
		SM_ERROR;
	}

	if (!((ackd_blk < 0) || (send_blk > (last_blk+1)) || ((send_blk-ackd_blk) > window))) {
		if ((WAITPUTGET(0) & 3) == 2) {
			SM_PROCEED(sendblk);
		}
	}

	a = GETCHAR(20);
	if (a == TIMEOUT) {
		if (count++ > 9) {
			Syslog('+', "too many tries to send block");
			SM_ERROR;
		}
		Syslog('x', "timeout waiting for ACK");
		send_blk=ackd_blk+1;
		SM_PROCEED(sendblk);
	} else if (a < 0) {
		SM_ERROR;
	} else switch (a) {
	case ACK:
		Syslog('x', "got ACK seamode=%d", seamode);
		count=0;
		cancount=0;
		switch (seamode) {
		case -1:if ((a1=GETCHAR(1)) < 0) {
				seamode=0;
				UNGETCHAR(a);
				SM_PROCEED(waitack);
			} else if ((a2=GETCHAR(1)) < 0) {
				seamode=0;
				UNGETCHAR(a1);
				UNGETCHAR(a);
				SM_PROCEED(waitack);
			} else if ((a1&0xff) != ((~a2)&0xff)) {
				seamode=0;
				UNGETCHAR(a2);
				UNGETCHAR(a1);
				UNGETCHAR(a);
				SM_PROCEED(waitack);
			} else {
				seamode=1;
				UNGETCHAR(a2);
				UNGETCHAR(a1);
				UNGETCHAR(a);
				SM_PROCEED(waitack);
			}
			break;
		case 0:
			ackd_blk++;
			SM_PROCEED(sendblk);
			break;
		case 1:
			a1=GETCHAR(1);
			a2=GETCHAR(1);
			if ((a1 < 0) || (a2 < 0) || (a1 != ((~a2)&0xff))) {
				Syslog('x', "bad ACK: 0x%02x/0x%02x, ignore", a1,a2);
				SM_PROCEED(sendblk);
			} else {
				if (a1 == ((send_blk-1) & 0xff)) {
					/* FD seems only to ACK last received block which is allright, 31-12-2000 MB. */
					Syslog('x', "got ACK %d", a1);
				} else if (a1 != ((ackd_blk+1) & 0xff)) {
					Syslog('x', "got ACK %d, expected %d", a1,(ackd_blk+1)&0xff);
					ackd_blk++;
				}
				tmp=send_blk-((send_blk-a1)&0xff);
				if ((tmp > ackd_blk) && (tmp < send_blk))
					ackd_blk=tmp;
				else
					Syslog('x', "bad ACK: %ld, ignore", a1,a2);
				if ((ackd_blk+1) == send_blk) {
					SM_PROCEED(sendblk);
				} else { /* read them all if more than 1 */
					SM_PROCEED(waitack);
				}
			}
			break;
		}
		break;
	case NAK:	if (ackd_blk <= 0) 
				crcmode=0;
			count++;
			send_blk=ackd_blk+1;
			SM_PROCEED(sendblk);
			break;
	case SYN:	SM_PROCEED(resync);
			break;
	case DC3:	if (session_flags & FTSC_XMODEM_XOF) {
				while (((a=GETCHAR(15)) > 0) && (a != DC1))
					Syslog('x', "got '%s' waiting for DC1", printablec(a));
			}
			SM_PROCEED(waitack);
			break;
	case CAN:	if (cancount++ > 5) {
				Syslog('+', "Remote requested cancel transfer");
				SM_ERROR;
			} else {
				SM_PROCEED(waitack);
			}
			break;
	case 'C':	if (ackd_blk < 0) {
				crcmode=1;
				count++;
				send_blk=ackd_blk+1;
				SM_PROCEED(sendblk);
			}
			/* fallthru */
	default:	Syslog('x', "Got '%s' waiting for ACK",printablec(a));
			SM_PROCEED(waitack);
			break;
	}

SM_STATE(resync)

	if (count++ > 9) {
		Syslog('+', "too may tries to resync");
		SM_ERROR;
	}

	i=-1;
	do {
		a=GETCHAR(15);
		resynbuf[++i]=a;
	}
	while ((a >= '0') && (a <= '9') && (i < sizeof(resynbuf)-1));
	resynbuf[i]='\0';
	Syslog('x', "got resync \"%s\", i=%d",resynbuf,i);
	lcrc=crc16xmodem(resynbuf,strlen(resynbuf));
	rcrc=0;
	if (a != ETX) {
		if (a > 0) 
			Syslog('+', "Got %d waiting for resync",a);
		else 
			Syslog('+', "Got %s waiting for resync",printablec(a));
		PUTCHAR(NAK);
		SM_PROCEED(waitack);
	}
	if ((a=GETCHAR(1)) < 0) {
		PUTCHAR(NAK);
		SM_PROCEED(waitack);
	}
	rcrc=a&0xff;
	if ((a=GETCHAR(1)) < 0) {
		PUTCHAR(NAK);
		SM_PROCEED(waitack);
	}
	rcrc |= (a << 8);
	if (rcrc != lcrc) {
		Syslog('+', "Bad resync crc: 0x%04x != 0x%04x",lcrc,rcrc);
		PUTCHAR(NAK);
		SM_PROCEED(waitack);
	}
	send_blk=atol(resynbuf);
	ackd_blk=send_blk-1;
	Syslog('+', "Resyncing at block %ld (byte %lu)", send_blk,(send_blk-1L)*XMBLKSIZ);
	PUTCHAR(ACK);
	SM_PROCEED(sendblk);

SM_STATE(sendeot)

	PUTCHAR(EOT);
	if (STATUS) {
		SM_ERROR;
	}
	send_blk++;
	SM_PROCEED(waitack);

SM_END
SM_RETURN



int xmsndfiles(file_list *tosend)
{
	int		rc,c = 0,gotnak,count;
	file_list	*nextsend;

	Syslog('x', "Xmodem send files start");
	for (nextsend=tosend;nextsend;nextsend=nextsend->next) {
		if (*(nextsend->local) != '~') {
			if (nextsend->remote) {
				if ((rc=xmsend(nextsend->local,nextsend->remote, (nextsend != tosend)))) {/* send m7 for rest */
					Syslog('x', "Xmodem send files failed, rc=%d", rc);
					return rc; /* and thus avoid execute_disposition() */
				} else {
					gotnak=0;
					count=0;
					while (!gotnak && (count < 6)) {
						c=GETCHAR(15);
						if (c < 0) 
							return STATUS;
						if (c == CAN) {
							Syslog('+', "Remote refused receiving");
							return 1;
						}
						if ((c == 'C') || (c == NAK)) 
							gotnak=1;
						else 
							Syslog('x', "Got '%s' waiting NAK", printablec(c));
					}
					if (c == 'C') 
						session_flags |= FTSC_XMODEM_CRC;
					if (!gotnak) 
						return 1;
				}
			}
			execute_disposition(nextsend);
		}
	}
	Syslog('x', "Xmodem send files finished");
	PUTCHAR(EOT);
	return STATUS;
}


