/*****************************************************************************
 *
 * $Id: pop3.c,v 1.8 2007/08/25 15:29:14 mbse Exp $
 * Purpose ...............: MBSE BBS Internet Library
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
#include "mbselib.h"
#include "mbinet.h"


static int		pop3sock = -1;	/* TCP/IP socket		*/
struct hostent		*php;		/* Host info remote		*/
struct servent		*psp;		/* Service information		*/
struct sockaddr_in	pop3_loc;	/* For local socket address	*/
struct sockaddr_in	pop3_rem;	/* For remote socket address	*/



int pop3_connect(void)
{
	socklen_t addrlen;
	char	*p;

	if (!strlen(CFG.popnode)) {
		WriteError("POP3: host not configured");
		return -1;
	}

	Syslog('+', "POP3: connecting host: %s", CFG.popnode);
	memset(&pop3_loc, 0, sizeof(struct sockaddr_in));
	memset(&pop3_rem, 0, sizeof(struct sockaddr_in));

	pop3_rem.sin_family = AF_INET;

	if ((php = gethostbyname(CFG.popnode)) == NULL) {
		WriteError("POP3: can't find host %s", CFG.popnode);
		return -1;
	}

	pop3_rem.sin_addr.s_addr = ((struct in_addr *)(php->h_addr))->s_addr;

	if ((psp = getservbyname("pop3", "tcp")) == NULL) {
		/*
		 * RedHat doesn't follow IANA specs and uses pop-3 in /etc/services
		 */
		if ((psp = getservbyname("pop-3", "tcp")) == NULL) {
			WriteError("POP3: can't find service port for pop3/tcp");
			return -1;
		}
	}
	pop3_rem.sin_port = psp->s_port;

	if ((pop3sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		WriteError("$POP3: unable to create tcp socket");
		return -1;
	}

	if (connect(pop3sock, (struct sockaddr *)&pop3_rem, sizeof(struct sockaddr_in)) == -1) {
		WriteError("$POP3: cannot connect tcp socket");
		return -1;
	}

	addrlen = sizeof(struct sockaddr_in);

	if (getsockname(pop3sock, (struct sockaddr *)&pop3_loc, &addrlen) == -1) {
		WriteError("$POP3: unable to read socket address");
		return -1;
	}

	p = pop3_receive();
	if (strlen(p) == 0) {
		WriteError("POP3: no response from server");
		pop3_close();
		return -1;
	}

	if (strncmp(p, "+OK", 3)) {
		WriteError("POP3: bad response: %s", p);
		pop3_close();
		return -1;
	}

	Syslog('+', "POP3: %s", p);
	
	return pop3sock;
}



int pop3_send(char *buf)
{
	if (pop3sock == -1)
		return -1;

	if (send(pop3sock, buf, strlen(buf), 0) != strlen(buf)) {
		WriteError("$POP3: socket send failed");
		return -1;
	}
	return 0;
}



/*
 *  Return empty buffer if something went wrong, else the complete
 *  dataline is returned
 */
char *pop3_receive(void)
{
	static char	buf[SS_BUFSIZE];
	int		i, j;

	memset((char *)&buf, 0, SS_BUFSIZE);
	i = 0;
	while (TRUE) {
		j = recv(pop3sock, &buf[i], 1, 0);
		if (j == -1) {
			WriteError("$POP3: error reading socket");
			memset((char *)&buf, 0, SS_BUFSIZE);
			return buf;
		}
		if (buf[i] == '\n')
			break;
		i += j;
	}

	for (i = 0; i < strlen(buf); i++) {
		if (buf[i] == '\n')
			buf[i] = '\0';
		if (buf[i] == '\r')
			buf[i] = '\0';
	}

	return buf;
}



int pop3_close(void)
{
	if (pop3sock == -1)
		return 0;

	if (shutdown(pop3sock, 1) == -1) {
		WriteError("$POP3: can't close socket");
		return -1;
	}

	pop3sock = -1;
	Syslog('+', "POP3: closed");
	return 0;
}



int pop3_cmd(char *cmd)
{
	char	*p;

	if (pop3_send(cmd) == -1)
		return -1;

	p = pop3_receive();

	if (strncmp(p, "+OK", 3)) {
		WriteError("POP3> %s", cmd);
		WriteError("POP3< %s", p);
		return -1;
	}
	return 0;
}



