/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
#include "../lib/dbnode.h"
#include "session.h"
#include "ttyio.h"
#include "openport.h"
#include "opentcp.h"


#define	BINKPORT 24554
#define	TELNPORT 23
#define FIDOPORT 60179		/* Eugene G. Crossers birthday */


static int	fd=-1;
extern int	f_flags;
extern int	tcp_mode;
extern time_t	c_start;
extern time_t	c_end;
extern int	online;
extern int	master;
extern int	carrier;
extern long	sentbytes;
extern long	rcvdbytes;
extern int	Loaded;


char	telnet_options[256];
char	do_dont_resp[256];
char	will_wont_resp[256];

void	tel_enter_binary(int rw);
void	tel_leave_binary(int rw);
void	send_do(register int);
void	send_dont(register int);
void	send_will(register int);
void	send_wont(register int);

static int	tcp_is_open = FALSE;


/* opentcp() was rewritten by Martin Junius */
/* telnet mode was written by T.Tanaka      */

int opentcp(char *name)
{
	struct servent		*se;
	struct hostent		*he;
	int			a1,a2,a3,a4;
	char			*errmsg;
	char			*portname;
	int			Fd;
	short			portnum;
	struct sockaddr_in	server;

	Syslog('d', "Try open tcp connection to %s",MBSE_SS(name));

	tcp_is_open = FALSE;
	memset(&telnet_options, 0, sizeof(telnet_options));
	server.sin_family = AF_INET;

	if ((portname = strchr(name,':'))) {
		*portname++='\0';
		if ((portnum = atoi(portname)))
			server.sin_port=htons(portnum);
		else if ((se = getservbyname(portname, "tcp")))
			server.sin_port = se->s_port;
		else 
			server.sin_port = htons(FIDOPORT);
	} else {
		switch (tcp_mode) {
		case TCPMODE_IFC:	if ((se = getservbyname("fido", "tcp")))
						server.sin_port = se->s_port;
					else
						server.sin_port = htons(FIDOPORT);
					break;
		case TCPMODE_ITN:	if ((se = getservbyname("tfido", "tcp")))
						server.sin_port = se->s_port;
					else
						server.sin_port = htons(TELNPORT);
					break;
		case TCPMODE_IBN:	if ((se = getservbyname("binkd", "tcp")))
						server.sin_port = se->s_port;
					else
						server.sin_port = htons(BINKPORT);
					break;
		default:		server.sin_port = htons(FIDOPORT);
		}
	}

	if (sscanf(name,"%d.%d.%d.%d",&a1,&a2,&a3,&a4) == 4)
		server.sin_addr.s_addr = inet_addr(name);
	else if ((he = gethostbyname(name)))
		memcpy(&server.sin_addr,he->h_addr,he->h_length);
	else {
		switch (h_errno) {
		case HOST_NOT_FOUND:	errmsg=(char *)"Authoritative: Host not found"; break;
		case TRY_AGAIN:		errmsg=(char *)"Non-Authoritive: Host not found"; break;
		case NO_RECOVERY:	errmsg=(char *)"Non recoverable errors"; break;
		default:		errmsg=(char *)"Unknown error"; break;
		}
		Syslog('+', "No IP address for %s: %s\n", name, errmsg);
		return -1;
	}

	Syslog('d', "Trying %s at port %d",
		inet_ntoa(server.sin_addr),(int)ntohs(server.sin_port));

	signal(SIGPIPE,linedrop);
	fflush(stdin);
	fflush(stdout);
	setbuf(stdin,NULL);
	setbuf(stdout,NULL);
	close(0);
	close(1);
	if ((Fd = socket(AF_INET,SOCK_STREAM,0)) != 0) {
		WriteError("$Cannot create socket (got %d, expected 0");
		open("/dev/null",O_RDONLY);
		open("/dev/null",O_WRONLY);
		return -1;
	}
	if (dup(Fd) != 1) {
		WriteError("$Cannot dup socket");
		open("/dev/null",O_WRONLY);
		return -1;
	}
	clearerr(stdin);
	clearerr(stdout);
	if (connect(Fd,(struct sockaddr *)&server,sizeof(server)) == -1) {
		Syslog('+', "Cannot connect %s",inet_ntoa(server.sin_addr));
		return -1;
	}

	f_flags=0;

	switch (tcp_mode) {
	case TCPMODE_ITN:	tel_enter_binary(3);
				Syslog('+', "Established ITN/TCP connection with %s", inet_ntoa(server.sin_addr));
				break;
	case TCPMODE_IFC:	Syslog('+', "Established IFC/TCP connection with %s", inet_ntoa(server.sin_addr));
				break;
	case TCPMODE_IBN:	Syslog('+', "Established IBN/TCP connection with %s", inet_ntoa(server.sin_addr));
				break;
	default:		WriteError("Established TCP connection with unknow protocol");
	}
	c_start = time(NULL);
	carrier = TRUE;
	tcp_is_open = TRUE;
	return 0;
}



void closetcp(void)
{
	FILE	*fph;
	char	*tmp;

	if (!tcp_is_open)
		return;

	Syslog('d', "Closing TCP connection");

	if (tcp_mode == TCPMODE_ITN)
		tel_leave_binary(3);

	shutdown(fd,2);
	signal(SIGPIPE,SIG_DFL);

	if (carrier) {
		c_end = time(NULL);
		online += (c_end - c_start);
		Syslog('+', "Connection time %s", t_elapsed(c_start, c_end));
		carrier = FALSE;
		history.offline = c_end;
		history.online  = c_start;
		history.sent_bytes = sentbytes;
		history.rcvd_bytes = rcvdbytes;
		history.inbound = ~master;
		tmp = calloc(128, sizeof(char));
		sprintf(tmp, "%s/var/mailer.hist", getenv("MBSE_ROOT"));
		if ((fph = fopen(tmp, "a")) == NULL)
			WriteError("$Can't open %s", tmp);
		else {
			fwrite(&history, sizeof(history), 1, fph);
			fclose(fph);
		}
		free(tmp);
		memset(&history, 0, sizeof(history));
		if (Loaded) {
			nodes.LastDate = time(NULL);
			UpdateNode();
		}
	}
	tcp_is_open = FALSE;
}



void tel_enter_binary(int rw)
{
	Syslog('d', "Telnet enter binary %d", rw);
	if (rw & 1)
		send_do(TELOPT_BINARY);
	if (rw & 2)
		send_will(TELOPT_BINARY);

	send_dont(TELOPT_ECHO);
	send_do(TELOPT_SGA);
	send_dont(TELOPT_RCTE);
	send_dont(TELOPT_TTYPE);

	send_wont(TELOPT_ECHO);
	send_will(TELOPT_SGA);
	send_wont(TELOPT_RCTE);
	send_wont(TELOPT_TTYPE);
}



void tel_leave_binary(int rw)
{
	Syslog('d', "Telnet leave binary %d", rw);
	if (rw & 1)
		send_dont(TELOPT_BINARY);
	if (rw & 2)
		send_wont(TELOPT_BINARY);
}



/*
 * These routines are in charge of sending option negotiations
 * to the other side.
 * The basic idea is that we send the negotiation if either side
 * is in disagreement as to what the current state should be.
 */

void send_do(register int c)
{
	NET2ADD(IAC, DO);
	NETADD(c);
}


void send_dont(register int c)
{
	NET2ADD(IAC, DONT);
	NETADD(c);
}


void send_will(register int c)
{
	NET2ADD(IAC, WILL);
	NETADD(c);
}


void send_wont(register int c)
{
	NET2ADD(IAC, WONT);
	NETADD(c);
}



