/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/nodelist.h"
#include "../lib/mberrors.h"
#include "ttyio.h"
#include "statetbl.h"
#include "emsi.h"
#include "ftsc.h"
#include "session.h"
#include "yoohoo.h"
#include "mbcico.h"
#include "binkp.h"
#include "callstat.h"


extern	int	tcp_mode;


node	*nlent;
fa_list *remote=NULL;
int	session_flags;
int	remote_flags;

int	tx_define_type(void);
int	rx_define_type(void);

static	int type;
static	char *data=NULL;

struct  sockaddr_in peeraddr;


char *typestr(int);
char *typestr(int tp)
{
	switch (tp) {
		case SESSION_FTSC:	return (char *)"FTSC";
		case SESSION_YOOHOO:	return (char *)"YooHoo/2U2";
		case SESSION_EMSI:	return (char *)"EMSI";
		case SESSION_BINKP:	return (char *)"Binkp";
		default:		return (char *)"Unknown";
	}
}



int session(faddr *a, node *nl, int role, int tp, char *dt)
{
	int	rc = MBERR_OK;
	fa_list *tmpl;
	int	addrlen = sizeof(struct sockaddr_in);

	session_flags = 0;
	type = tp;
	nlent = nl;

	if (role) {
		Syslog('s', "Start outbound session type %s with %s", typestr(type), ascfnode(a,0x1f));
	} else
		Syslog('s', "Start inbound session type %s", typestr(type));

	if (getpeername(0,(struct sockaddr*)&peeraddr,&addrlen) == 0) {
		Syslog('s', "TCP connection: len=%d, family=%hd, port=%hu, addr=%s",
			addrlen,peeraddr.sin_family, peeraddr.sin_port, inet_ntoa(peeraddr.sin_addr));
		if (role == 0) {
			if (tcp_mode == TCPMODE_IBN) {
				Syslog('+', "Incoming IBN/TCP connection from %s", inet_ntoa(peeraddr.sin_addr));
				IsDoing("Incoming IBN/TCP");
			} else if (tcp_mode == TCPMODE_ITN) {
				Syslog('+', "Incoming ITN/TCP connection from %s", inet_ntoa(peeraddr.sin_addr));
				IsDoing("Incoming ITN/TCP");
			} else if (tcp_mode == TCPMODE_IFC) {
				Syslog('+', "Incoming IFC/TCP connection from %s", inet_ntoa(peeraddr.sin_addr));
				IsDoing("Incoming IFC/TCP");
			} else if (tcp_mode == TCPMODE_NONE) {
				WriteError("Unknown TCP connection, parameter missing");
				die(MBERR_COMMANDLINE);
			}
		}
		session_flags |= SESSION_TCP;
	}

	if (data)
		free(data);
	data=NULL;

	if (dt)
		data=xstrcpy(dt);

	emsi_local_protos=0;
	emsi_local_opts=0;
	emsi_local_lcodes=0;
	
	tidy_falist(&remote);
	remote=NULL;
	if (a) {
		remote=(fa_list*)malloc(sizeof(fa_list));
		remote->next=NULL;
		remote->addr=(faddr*)malloc(sizeof(faddr));
		remote->addr->zone=a->zone;
		remote->addr->net=a->net;
		remote->addr->node=a->node;
		remote->addr->point=a->point;
		remote->addr->domain=xstrcpy(a->domain);
		remote->addr->name=NULL;
	} else {
		remote=NULL;
	}

	remote_flags=SESSION_FNC;

	if (role) {
		if (type == SESSION_UNKNOWN) 
			(void)tx_define_type();
		switch(type) {
			case SESSION_UNKNOWN:	rc = MBERR_UNKNOWN_SESSION; break;
			case SESSION_FTSC:	rc = tx_ftsc(); break;
			case SESSION_YOOHOO:	rc = tx_yoohoo(); break;
			case SESSION_EMSI:	rc = tx_emsi(data); break;
			case SESSION_BINKP:	rc = binkp(role); break;
		}
	} else {
		if (type == SESSION_FTSC) 
			session_flags |= FTSC_XMODEM_CRC;
		if (type == SESSION_UNKNOWN) 
			(void)rx_define_type();
		switch(type) {
			case SESSION_UNKNOWN:	rc = MBERR_UNKNOWN_SESSION; break;
			case SESSION_FTSC:	rc = rx_ftsc(); break;
			case SESSION_YOOHOO:	rc = rx_yoohoo(); break;
			case SESSION_EMSI:	rc = rx_emsi(data); break;
			case SESSION_BINKP:	rc = binkp(role); break;
		}
	}
	sleep(2);
	for (tmpl = remote; tmpl; tmpl = tmpl->next) {
		/*
		 * Unlock all nodes, locks not owned by us are untouched.
		 */
		(void)nodeulock(tmpl->addr);
		/*
		 * If successfull session, reset all status records.
		 */
		if (rc == 0)
			putstatus(tmpl->addr, 0, 0);
	}
	tidy_falist(&remote);
	if (data)
		free(data);
	data = NULL;

	if (emsi_local_password)
		free(emsi_local_password);
	if (emsi_remote_password)
		free(emsi_remote_password);

	if (nlent->addr.domain)
		free(nlent->addr.domain);

	return rc;
}



SM_DECL(tx_define_type,(char *)"tx_define_type")
SM_STATES
	skipjunk,
	wake,
	waitchar,
	nextchar,
	checkintro,
	sendinq
SM_NAMES
	(char *)"skipjunk",
	(char *)"wake",
	(char *)"waitchar",
	(char *)"nextchar",
	(char *)"checkintro",
	(char *)"sendinq"
SM_EDECL
	int c=0;
	int count=0;
	char buf[256],*p;
	char ebuf[13],*ep;
	int standby=0;

	int maybeftsc=0;
	int maybeyoohoo=0;

	type=SESSION_UNKNOWN;
	ebuf[0]='\0';
	ep=ebuf;
	buf[0]='\0';
	p=buf;

SM_START(skipjunk)

SM_STATE(skipjunk)

	Syslog('S', "tx_define_type SKIPJUNK");
	while ((c = GETCHAR(1)) >= 0) /*nothing*/ ;
	if (c == TIMEOUT) {
		SM_PROCEED(wake);
	} else {
		SM_ERROR;
	}

SM_STATE(wake)

	Syslog('S', "tx_define_type WAKE");
	if (count++ > 20) {
		Syslog('+', "Remote did not respond");
		SM_ERROR;
	}

	p=buf;
	PUTCHAR('\r');
	if ((c = GETCHAR(2)) == TIMEOUT) {
		SM_PROCEED(wake);
	} else if (c < 0) {
		WriteError("Error while waking remote");
		SM_ERROR;
	} else {
		count = 0;
		Syslog('S', "Got %c wakeup", c);
		SM_PROCEED(nextchar);
	}

SM_STATE(waitchar)

	Syslog('S', "tx_define_type WAITCHAR");
	if ((c = GETCHAR(2)) == TIMEOUT) { /* Was 4 seconds */
		standby = 0;
		ep = ebuf;
		ebuf[0] = '\0';
		if (count++ > 30) { /* Was 8 loops */
			Syslog('+', "Too many tries waking remote");
			SM_ERROR;
		}
		SM_PROCEED(sendinq);
	} else if (c < 0) {
		Syslog('+', "Error while getting intro from remote");
		SM_ERROR;
	} else {
		SM_PROCEED(nextchar);
	}

SM_STATE(nextchar)

	Syslog('S', "tx_define_type NEXTCHAR");
	if (c == 'C') {
		session_flags |= FTSC_XMODEM_CRC;
		maybeftsc++;
	} 
	if (c == NAK) {
		session_flags &= ~FTSC_XMODEM_CRC;
		maybeftsc++;
	} 
	if (c == ENQ) 
		maybeyoohoo++;

	if (((localoptions & NOWAZOO) == 0) && (maybeyoohoo > 1)) {
		type = SESSION_YOOHOO;
		SM_SUCCESS;
	}

	if (maybeftsc > 1) {
		type = SESSION_FTSC;
		SM_SUCCESS;
	}

	if ((c >= ' ') && (c <= '~')) {
		if (c != 'C') 
			maybeftsc = 0;
		maybeyoohoo = 0;

		if ((p-buf) < (sizeof(buf)-1)) {
			*p++ = c;
			*p = '\0';
		}

		if (c == '*') {
			standby = 1;
			ep = ebuf;
			buf[0] = '\0';
		} else if (standby) {
			if ((ep - ebuf) < (sizeof(ebuf) - 1)) {
				*ep++ = c;
				*ep = '\0';
			} 
			if ((ep - ebuf) >= (sizeof(ebuf) - 1)) {
				standby = 0;
				SM_PROCEED(checkintro);
			}
		}
	} else switch (c) {
	case DC1:	break;
	case '\r':
	case '\n':	standby = 0;
			ep = ebuf;
			ebuf[0] = '\0';
			if (buf[0]) 
				Syslog('+', "Intro: \"%s\"", printable(buf, 0));
			p = buf;
			buf[0] = '\0';
			break;
	default:	standby = 0;
			ep = ebuf;
			ebuf[0] = '\0';
			Syslog('i', "Got '%s' reading intro", printablec(c));
			break;
	}

	SM_PROCEED(waitchar);

SM_STATE(checkintro)

	Syslog('S', "tx_define_type CHECKINTRO");
	Syslog('i', "Check \"%s\" for being EMSI request",ebuf);

	if (((localoptions & NOEMSI) == 0) && (strncasecmp(ebuf,"EMSI_REQA77E",12) == 0)) {
		type = SESSION_EMSI;
		data = xstrcpy((char *)"**EMSI_REQA77E");
		Syslog('i', "Sending **EMSI_INQC816 (2 times)");
		PUTSTR((char *)"\r**EMSI_INQC816\r**EMSI_INQC816\r\021");
		SM_SUCCESS;
	} else {
		p = buf;
		buf[0] = '\0';
		SM_PROCEED(waitchar);
	}

SM_STATE(sendinq)

	Syslog('S', "tx_define_type SENDINQ");
	PUTCHAR(DC1);
	if ((localoptions & NOEMSI) == 0) {
		Syslog('S', "send **EMSI_INQC816 (2 times)");
		PUTSTR((char *)"\r**EMSI_INQC816**EMSI_INQC816");
	}
	if ((localoptions & NOWAZOO) == 0) {
		Syslog('S', "send YOOHOO char");
		PUTCHAR(YOOHOO);
	}
	Syslog('S', "send TSYNC char");
	PUTCHAR(TSYNC);
	if ((localoptions & NOEMSI) == 0)
		PUTSTR((char *)"\r\021");
	SM_PROCEED(waitchar);

SM_END
SM_RETURN



SM_DECL(rx_define_type,(char *)"rx_define_type")
SM_STATES
	sendintro,
	waitchar,
	nextchar,
	checkemsi,
	getdat
SM_NAMES
	(char *)"sendintro",
	(char *)"waitchar",
	(char *)"nextchar",
	(char *)"checkemsi",
	(char *)"getdat"
SM_EDECL
	int count=0;
	int c=0;
	int maybeftsc=0,maybeyoohoo=0;
	char ebuf[13],*ep;
	char *p;
	int standby=0;
	int datasize;

	type=SESSION_UNKNOWN;
	session_flags|=FTSC_XMODEM_CRC;
	ebuf[0]='\0';
	ep=ebuf;
	Syslog('S', "rxdefine_type INIT");

SM_START(sendintro)

SM_STATE(sendintro)

	Syslog('S', "rxdefine_type SENDINTRO");
	if (count++ > 6) {  /* Was 16, is 6 according to the EMSI spec */
		Syslog('+', "Too many tries to get anything from the caller");
		SM_ERROR;
	}

	standby = 0;
	ep = ebuf;
	ebuf[0] = '\0';

	if ((localoptions & NOEMSI) == 0) {
		PUTSTR((char *)"**EMSI_REQA77E\r\021");
	}
	PUTSTR((char *)"\r\rAddress: ");
	PUTSTR(aka2str(CFG.aka[0]));
	PUTSTR((char *)" using mbcico ");
	PUTSTR((char *)VERSION);
	switch (tcp_mode) {
	case TCPMODE_IFC:	PUTSTR((char *)"; IFC");
				break;
	case TCPMODE_ITN:	PUTSTR((char *)"; ITN");
				break;
	case TCPMODE_IBN:	PUTSTR((char *)"; IBN");
				break;
	}
	PUTCHAR('\r');
	if (STATUS) {
		SM_ERROR;
	} else {
		SM_PROCEED(waitchar);
	}

SM_STATE(waitchar)

	Syslog('S', "rxdefine_type WAITCHAR");
	if ((c=GETCHAR(20)) == TIMEOUT) { /* Timeout was 8, must be 20. */
		SM_PROCEED(sendintro);
	} else if (c < 0) {
		Syslog('+', "EMSI error while getting from caller");
		SM_ERROR;
	} else {
		SM_PROCEED(nextchar);
	}

SM_STATE(nextchar)

	Syslog('S', "rxdefine_type NEXTCHAR");
	if ((c >= ' ') && (c <= 'z')) {
		if (c == '*') {
			standby = 1;
			ep = ebuf;
			ebuf[0] = '\0';
		} else if (standby) {
			if ((ep - ebuf) < (sizeof(ebuf) - 1)) {
				*ep++ = c;
				*ep = '\0';
			}
			if ((ep - ebuf) >= (sizeof(ebuf) - 1)) {
				standby = 0;
				SM_PROCEED(checkemsi);
			}
		}
		SM_PROCEED(waitchar);
	} else switch (c) {
	case DC1:	SM_PROCEED(waitchar);
			break;
	case TSYNC:	standby = 0;
			ep = ebuf;
			ebuf[0] = '\0';
			if (++maybeftsc > 1) {
				type = SESSION_FTSC;
				SM_SUCCESS;
			} else {
				SM_PROCEED(waitchar);
			}
			break;
	case YOOHOO:	standby = 0;
			ep = ebuf;
			ebuf[0] = '\0';
			if (++maybeyoohoo > 1) {
				type = SESSION_YOOHOO;
				SM_SUCCESS;
			} else {
				SM_PROCEED(waitchar);
			}
			break;
	case '\r':
	case '\n':	standby = 0;
			ep = ebuf;
			ebuf[0] = '\0';
			if (ebuf[0]) {
				SM_PROCEED(checkemsi);
			} else {
				SM_PROCEED(sendintro);
			}
			break;
	default:	standby = 0;
			ep = ebuf;
			ebuf[0] = '\0';
			Syslog('i', "Got '%s' from remote", printablec(c));
			SM_PROCEED(waitchar);
			break;
	}

SM_STATE(checkemsi)

	Syslog('S', "rxdefine_type CHECKEMSI");
	Syslog('i', "check \"%s\" for being EMSI inquery or data",ebuf);

	if (localoptions & NOEMSI) {
		SM_PROCEED(sendintro);
	}

	if (strncasecmp(ebuf, "EMSI_INQC816", 12) == 0) {
		type = SESSION_EMSI;
		data = xstrcpy((char *)"**EMSI_INQC816");
		SM_SUCCESS;
	} else if (strncasecmp(ebuf, "EMSI_DAT", 8) == 0) {
		SM_PROCEED(getdat);
	} else {
		SM_PROCEED(sendintro);
	}

SM_STATE(getdat)

	Syslog('S', "rxdefine_type GETDAT");
	Syslog('i', "Try get emsi_dat packet starting with \"%s\"",ebuf);

	if (sscanf(ebuf+8, "%04x", &datasize) != 1) {
		SM_PROCEED(sendintro);
	}

	datasize += 18; /* strlen("**EMSI_DATxxxxyyyy"), include CRC */
	data=malloc(datasize+1);
	strcpy(data,"**");
	strcat(data, ebuf);
	p = data + strlen(data);

	while (((p - data) < datasize) && ((c = GETCHAR(8)) >= 0)) {
		*p++ = c;
		*p= '\0';
	}
	if (c == TIMEOUT) {
		SM_PROCEED(sendintro);
	} else if (c < 0) {
		Syslog('+', "Error while reading EMSI_DAT from the caller");
		SM_ERROR;
	}
	type = SESSION_EMSI;
	SM_SUCCESS;

SM_END
SM_RETURN

