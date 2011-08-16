/*****************************************************************************
 *
 * Purpose ...............: MBSE BBS Internet Library
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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


static int	pop3sock = -1;	/* TCP/IP socket		*/



int pop3_connect(void)
{
    char		*q, *ipver = NULL, ipstr[INET6_ADDRSTRLEN];
    struct addrinfo	hints, *res = NULL, *p;
    int			rc;

    if (!strlen(CFG.popnode)) {
	WriteError("POP3: host not configured");
	return -1;
    }

    Syslog('+', "POP3: connecting host: %s", CFG.popnode);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rc = getaddrinfo(CFG.popnode, "pop3", &hints, &res)) != 0) {
	WriteError("getaddrinfo %s: %s\n", CFG.popnode, gai_strerror(rc));
	return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
	void	*addr;

	if (p->ai_family == AF_INET) {
	    struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
	    addr = &(ipv4->sin_addr);
	    ipver = (char *)"IPv4";
	} else {
	    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
	    addr = &(ipv6->sin6_addr);
	    ipver = (char *)"IPv6";
	}
	inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
	Syslog('+', "Trying %s %s port pop3", ipver, ipstr);

	if ((pop3sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
	    WriteError("$socket()");
	    return -1;
	} else {
	    if (connect(pop3sock, p->ai_addr, p->ai_addrlen) == -1) {
	    	WriteError("$connect %s port pop3", ipstr);
	    	close(pop3sock);
	    } else {
	    	break;
	    }
	}
    }
    if (p == NULL) {
	return -1;	/* Not connected */
    }

    q = pop3_receive();
    if (strlen(q) == 0) {
	WriteError("POP3: no response from server");
	pop3_close();
	return -1;
    }

    if (strncmp(q, "+OK", 3)) {
	WriteError("POP3: bad response: %s", q);
	pop3_close();
	return -1;
    }

    Syslog('+', "POP3: %s", q);

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



