/*****************************************************************************
 *
 * $Id: session.c,v 1.37 2007/10/14 13:15:34 mbse Exp $
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "ttyio.h"
#include "statetbl.h"
#include "emsi.h"
#include "ftsc.h"
#include "session.h"
#include "yoohoo.h"
#include "mbcico.h"
#include "binkp.h"
#include "callstat.h"
#include "inbound.h"
#include "opentcp.h"
#include "telnet.h"


extern int	tcp_mode;
extern pid_t	mypid;


node	*nlent = NULL;
fa_list *remote = NULL;
int	session_flags;
int	remote_flags;
int	laststat = 0;		    /* Last session status with remote	*/

int	tx_define_type(void);
int	rx_define_type(void);

int	session_type = SESSION_UNKNOWN;
int	session_state = STATE_BAD;

static	char *data=NULL;

struct  sockaddr_in peeraddr;


char *typestr(int);
char *typestr(int tp)
{
    switch (tp) {
	case SESSION_FTSC:	return (char *)"FTS-0001";
	case SESSION_YOOHOO:	return (char *)"YooHoo/2U2";
	case SESSION_EMSI:	return (char *)"EMSI";
	case SESSION_BINKP:	return (char *)"Binkp";
	default:		return (char *)"Unknown";
    }
}



#ifdef	HAVE_GEOIP_H

extern void _GeoIP_setup_dbfilename(void);


void geoiplookup(GeoIP* gi, char *hostname, int i) 
{
    const char * country_code;
    const char * country_name;
    const char * country_continent;
    int country_id;

    if (GEOIP_COUNTRY_EDITION == i) {
	country_id = GeoIP_id_by_name(gi, hostname);
	country_code = GeoIP_country_code[country_id];
	country_name = GeoIP_country_name[country_id];
	country_continent = GeoIP_country_continent[country_id];
	if (country_code == NULL) {
	    Syslog('+', "%s: IP Address not found\n", GeoIPDBDescription[i]);
	} else {
	    Syslog('+', "GeoIP location: %s, %s %s\n", country_name, country_code, country_continent);
	}
    }
}
#endif



int session(faddr *a, node *nl, int role, int tp, char *dt)
{
    int	    rc = MBERR_OK; 
    socklen_t	addrlen = sizeof(struct sockaddr_in);
    fa_list *tmpl;
    int	    Fdo = -1, input_pipe[2], output_pipe[2];
    pid_t   ipid, opid;
#ifdef	HAVE_GEOIP_H
    char    *hostname;
    GeoIP   *gi;
#endif

    session_flags = 0;
    session_type = tp;
    nlent = nl;

    if (getpeername(0,(struct sockaddr*)&peeraddr,&addrlen) == 0) {
	Syslog('s', "TCP connection: len=%d, family=%hd, port=%hu, addr=%s",
			addrlen,peeraddr.sin_family, peeraddr.sin_port, inet_ntoa(peeraddr.sin_addr));
	if (role == 0) {
	    if (tcp_mode == TCPMODE_IBN) {
		Syslog('+', "Incoming IBN/TCP connection from %s", inet_ntoa(peeraddr.sin_addr));
		IsDoing("Incoming IBN/TCP");
	    } else if (tcp_mode == TCPMODE_IFC) {
		Syslog('+', "Incoming IFC/TCP connection from %s", inet_ntoa(peeraddr.sin_addr));
		IsDoing("Incoming IFC/TCP");
	    } else if (tcp_mode == TCPMODE_ITN) {
		Syslog('+', "Incoming ITN/TCP connection from %s", inet_ntoa(peeraddr.sin_addr));
		IsDoing("Incoming ITN/TCP");
	    } else if (tcp_mode == TCPMODE_NONE) {
		WriteError("Unknown TCP connection, parameter missing");
		die(MBERR_COMMANDLINE);
	    }
	}
	session_flags |= SESSION_TCP;

#ifdef	HAVE_GEOIP_H
	hostname = inet_ntoa(peeraddr.sin_addr);
	_GeoIP_setup_dbfilename();
	if (GeoIP_db_avail(GEOIP_COUNTRY_EDITION)) {
	    if ((gi = GeoIP_open_type(GEOIP_COUNTRY_EDITION, GEOIP_STANDARD)) != NULL) {
		geoiplookup(gi, hostname, GEOIP_COUNTRY_EDITION);
	    }
	    GeoIP_delete(gi);
	}
#endif

	if (tcp_mode == TCPMODE_ITN) {
	    Syslog('s', "Installing telnet filter...");

	    /*
	     * First make sure the current input socket gets a new file descriptor
	     * since it's now on stadin and stdout.
	     */
	    Fdo = dup(0);
	    Syslog('s', "session: new socket %d", Fdo);

	     /*
	      * Close stdin and stdout so that when we create the pipes to
	      * the telnet filter they get stdin and stdout as file descriptors.
	      */
	    fflush(stdin);
	    fflush(stdout);
	    setbuf(stdin,NULL);
	    setbuf(stdout, NULL);
	    close(0);
	    close(1);

	    /*
	     * Create output pipe and start output filter.
	     */
	    if (pipe(output_pipe) == -1) {
		WriteError("$could not create output_pipe");
		die(MBERR_TTYIO_ERROR);
	    }
	    opid = fork();
	    switch (opid) {
		case -1:    WriteError("fork for telout_filter failed");
			    die(MBERR_TTYIO_ERROR);
		case 0:     if (close(output_pipe[1]) == -1) {
				WriteError("$error close output_pipe[1]");
				die(MBERR_TTYIO_ERROR);
			    }
			    telout_filter(output_pipe[0], Fdo);
			    /* NOT REACHED */
	    }
	    if (close(output_pipe[0] == -1)) {
		WriteError("$error close output_pipe[0]");
		die(MBERR_TTYIO_ERROR);
	    }
	    Syslog('s', "telout_filter forked with pid %d", opid);
	
	    /*
	     * Create input pipe and start input filter
	     */
	    if (pipe(input_pipe) == -1) {
		WriteError("$could not create input_pipe");
		die(MBERR_TTYIO_ERROR);
	    }
	    ipid = fork();
	    switch (ipid) {
		case -1:    WriteError("fork for telin_filter failed");
			    die(MBERR_TTYIO_ERROR);
		case 0:     if (close(input_pipe[0]) == -1) {
				WriteError("$error close input_pipe[0]");
				die(MBERR_TTYIO_ERROR);
			    }
			    telin_filter(input_pipe[1], Fdo);
			    /* NOT REACHED */
	    }
	    if (close(input_pipe[1]) == -1) {
		WriteError("$error close input_pipe[1]");
		die(MBERR_TTYIO_ERROR);
	    }
	    Syslog('s', "telin_filter forked with pid %d", ipid);

	    Syslog('s', "stdout = %d", output_pipe[1]);
	    Syslog('s', "stdin  = %d", input_pipe[0]);

	    if ((input_pipe[0] != 0) || (output_pipe[1] != 1)) {
		WriteError("Failed to create pipes on stdin and stdout");
		die(MBERR_TTYIO_ERROR);
	    }
	    Syslog('+', "Telnet I/O filters installed");
	    telnet_init(Fdo);
	}
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
	if (session_type == SESSION_UNKNOWN) 
	    (void)tx_define_type();
	Syslog('+', "Start outbound %s session with %s", typestr(session_type), ascfnode(a,0x1f));
	switch(session_type) {
	    case SESSION_UNKNOWN:   rc = MBERR_UNKNOWN_SESSION; break;
	    case SESSION_FTSC:	    rc = tx_ftsc(); break;
	    case SESSION_YOOHOO:    rc = tx_yoohoo(); break;
	    case SESSION_EMSI:	    rc = tx_emsi(data); break;
	    case SESSION_BINKP:	    rc = binkp(role); break;
	}
    } else {
	if (session_type == SESSION_FTSC) 
	    session_flags |= FTSC_XMODEM_CRC;
	if (session_type == SESSION_UNKNOWN) 
	    (void)rx_define_type();
	Syslog('+', "Start inbound %s session", typestr(session_type));
	switch(session_type) {
	    case SESSION_UNKNOWN:   rc = MBERR_UNKNOWN_SESSION; break;
	    case SESSION_FTSC:	    rc = rx_ftsc(); break;
	    case SESSION_YOOHOO:    rc = rx_yoohoo(); break;
	    case SESSION_EMSI:	    rc = rx_emsi(data); break;
	    case SESSION_BINKP:	    rc = binkp(role); break;
	}
    }
    sleep(2);
    for (tmpl = remote; tmpl; tmpl = tmpl->next) {
	/*
	 * Unlock all nodes, locks not owned by us are untouched.
	 */
	(void)nodeulock(tmpl->addr, mypid);
	/*
	 * If successfull session, reset all status records.
	 */
	if (rc == 0)
	    putstatus(tmpl->addr, 0, 0);
    }

    if (rc)
	session_state = STATE_BAD;

    /*
     * If the socket for the telnet filter is open, close it so that the telnet filters exit.
     * After that wait a little while to let the filter childs die before the main program
     * does, else we get zombies.
     */
    if (Fdo != -1) {
	Syslog('s', "shutdown filter sockets and stdio");
	shutdown(Fdo, 2);
	close(0);
	close(1);
	close(output_pipe[1]);
	close(input_pipe[0]);
	msleep(100); 
    }

    tidy_falist(&remote);
    if (data)
	free(data);
    data = NULL;

    if (emsi_local_password)
	free(emsi_local_password);
    if (emsi_remote_password)
	free(emsi_remote_password);

    inbound_close(rc == 0);
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
    int	    c = 0;
    char    buf[256], *p;
    char    ebuf[13], *ep;
    int	    standby = 0;
    
    int	    maybeftsc=0;
    int	    maybeyoohoo=0;

    session_type = SESSION_UNKNOWN;
    ebuf[0] = '\0';
    ep = ebuf;
    buf[0] = '\0';
    p = buf;

SM_START(skipjunk)

SM_STATE(skipjunk)

    while ((c = GETCHAR(1)) >= 0) /*nothing*/ ;
    if (c == TIMEOUT) {
	gpt_resettimers();
	gpt_settimer(0, 60);    /* 60 second master timer */
	SM_PROCEED(wake);
    } else {
	SM_ERROR;
    }

SM_STATE(wake)

    if (gpt_expired(0)) {
	Syslog('+', "Remote did not respond");
	SM_ERROR;
    }

    p = buf;
    PUTCHAR('\r');
    if ((c = GETCHAR(2)) == TIMEOUT) {
	SM_PROCEED(wake);
    } else if (c < 0) {
	WriteError("Error while waking remote");
	SM_ERROR;
    } else {
	gpt_settimer(0, 60);
	SM_PROCEED(nextchar);
    }

SM_STATE(waitchar)

    if ((c = GETCHAR(2)) == TIMEOUT) {
	standby = 0;
	ep = ebuf;
	ebuf[0] = '\0';
	if (gpt_expired(0)) {
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
	session_type = SESSION_YOOHOO;
	SM_SUCCESS;
    }

    if (maybeftsc > 1) {
	session_type = SESSION_FTSC;
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
    } else { 
	switch (c) {
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
    }
    SM_PROCEED(waitchar);

SM_STATE(checkintro)

    Syslog('i', "Check \"%s\" for being EMSI request",ebuf);

    if (((localoptions & NOEMSI) == 0) && (strncasecmp(ebuf,"EMSI_REQA77E",12) == 0)) {
	session_type = SESSION_EMSI;
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

    PUTCHAR(DC1);
    if ((localoptions & NOEMSI) == 0) {
	PUTSTR((char *)"\r**EMSI_INQC816\r**EMSI_INQC816");
    }
    if ((localoptions & NOWAZOO) == 0) {
	PUTCHAR(YOOHOO);
    }
    PUTCHAR(TSYNC);
    if ((localoptions & NOEMSI) == 0)
	PUTSTR((char *)"\r\021");
    SM_PROCEED(waitchar);

SM_END
SM_RETURN



SM_DECL(rx_define_type,(char *)"rx_define_type")
SM_STATES
    sendintro,
    settimer,
    waitchar,
    nextchar,
    checkemsi,
    getdat
SM_NAMES
    (char *)"sendintro",
    (char *)"settimer",
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

    session_type=SESSION_UNKNOWN;
    session_flags|=FTSC_XMODEM_CRC;
    ebuf[0]='\0';
    ep=ebuf;
    gpt_resettimers();
    gpt_settimer(0, 60);
    gpt_settimer(1, 20);

SM_START(sendintro)

SM_STATE(sendintro)

    if (count++ > 6) {
	Syslog('+', "Too many tries to get anything from the caller");
	SM_ERROR;
    }

    Syslog('s', "rxdefine_type SENDINTRO count=%d", count);

    standby = 0;
    ep = ebuf;
    ebuf[0] = '\0';

    if ((localoptions & NOEMSI) == 0) {
	PUTSTR((char *)"**EMSI_REQA77E\r\021");
    }
    PUTCHAR('\r');
    if (STATUS) {
	SM_ERROR;
    } else {
	SM_PROCEED(settimer);
    }

SM_STATE(settimer)

    Syslog('s', "Set 20 secs timer");
    gpt_settimer(1, 20);
    SM_PROCEED(waitchar);

SM_STATE(waitchar)

    if (gpt_expired(0)) {
        Syslog('+', "Session setup timeout");
        SM_ERROR;
    }

    if (gpt_expired(1)) {
	Syslog('s', "20 sec timer timeout");
	SM_PROCEED(sendintro);
    }

    if ((c = GETCHAR(1)) == TIMEOUT) {
	SM_PROCEED(waitchar);
    } else if (c < 0) {
	Syslog('+', "Session setup error");
	SM_ERROR;
    } else {
	SM_PROCEED(nextchar);
    }

SM_STATE(nextchar)

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
    } else {
	switch (c) {
	    case DC1:	SM_PROCEED(waitchar);
			break;
	    case TSYNC:	standby = 0;
			ep = ebuf;
			ebuf[0] = '\0';
			if (++maybeftsc > 1) {
			    session_type = SESSION_FTSC;
			    SM_SUCCESS;
			} else {
			    SM_PROCEED(waitchar);
			}
			break;
	    case YOOHOO:standby = 0;
			ep = ebuf;
			ebuf[0] = '\0';
			if (++maybeyoohoo > 1) {
			    session_type = SESSION_YOOHOO;
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
			    /*
			     * If the 20 second timer is expired or after the 
			     * first sendintro, send the intro again. After
			     * that take it easy.
			     */
			    if (gpt_expired(1) || (count == 1)) {
				Syslog('s', "sendintro after eol char");
				SM_PROCEED(sendintro);
			    } else {
				Syslog('s', "waitchar after eol char");
				SM_PROCEED(waitchar);
			    }
			}
			break;
	    default:	standby = 0;
			ep = ebuf;
			ebuf[0] = '\0';
			Syslog('i', "Got '%s' from remote", printablec(c));
			SM_PROCEED(waitchar);
			break;
	}
    }

SM_STATE(checkemsi)

    Syslog('i', "check \"%s\" for being EMSI inquery or data",ebuf);

    if (localoptions & NOEMSI) {
	Syslog('s', "Force sendintro");
	SM_PROCEED(sendintro);
    }

    if (strncasecmp(ebuf, "EMSI_INQC816", 12) == 0) {
	session_type = SESSION_EMSI;
	data = xstrcpy((char *)"**EMSI_INQC816");
	SM_SUCCESS;
    } else if (strncasecmp(ebuf, "EMSI_HBT", 8) == 0) {
	standby = 0;
        ep = ebuf;
	ebuf[0] = '\0';
	SM_PROCEED(settimer);
    } else if (strncasecmp(ebuf, "EMSI_DAT", 8) == 0) {
	SM_PROCEED(getdat);
    } else {
	SM_PROCEED(settimer);
    }

SM_STATE(getdat)

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
	Syslog('s', "c = TIMEOUT -> sendintro");
	SM_PROCEED(sendintro);
    } else if (c < 0) {
	Syslog('+', "Error while reading EMSI_DAT from the caller");
	SM_ERROR;
    }
    session_type = SESSION_EMSI;
    SM_SUCCESS;

SM_END
SM_RETURN

