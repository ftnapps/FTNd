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


static int		smtpsock = -1;	/* TCP/IP socket		*/



int smtp_connect(void)
{
    char                *q, *ipver = NULL, ipstr[INET6_ADDRSTRLEN], temp[41];
    struct addrinfo     hints, *res = NULL, *p;
    int                 rc;

    if (smtpsock != -1)
	return smtpsock;

    if (!strlen(CFG.smtpnode)) {
	WriteError("SMTP: host not configured");
	return -1;
    }

    Syslog('+', "SMTP: connecting host: %s", CFG.smtpnode);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rc = getaddrinfo(CFG.popnode, "smtp", &hints, &res)) != 0) {
        WriteError("getaddrinfo %s: %s\n", CFG.popnode, gai_strerror(rc));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        void    *addr;

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

        if ((smtpsock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            WriteError("$socket()");
            return -1;
        } else {
            if (connect(smtpsock, p->ai_addr, p->ai_addrlen) == -1) {
                WriteError("$connect %s port pop3", ipstr);
                close(smtpsock);
            } else {
                break;
            }
        }
    }
    if (p == NULL) {
        return -1;      /* Not connected */
    }

    q = smtp_receive();
    if (strlen(q) == 0) {
	WriteError("SMTP: no response");
	smtp_close();
	return -1;
    }

    if (strncmp(q, "220", 3)) {
	WriteError("SMTP: bad response: %s", q);
	smtp_close();
	return -1;
    }

    Syslog('+', "SMTP: %s", q);

    snprintf(temp, 40, "HELO %s\r\n", CFG.sysdomain);
    if (smtp_cmd(temp, 250)) {
	smtp_close();
	return -1;
    }

    return smtpsock;
}



int smtp_send(char *buf)
{
	if (smtpsock == -1)
		return -1;

	if (send(smtpsock, buf, strlen(buf), 0) != strlen(buf)) {
		WriteError("$SMTP: socket send failed");
		return -1;
	}
	return 0;
}



/*
 *  Return empty buffer if something went wrong, else the complete
 *  dataline is returned
 */
char *smtp_receive(void)
{
	static char	buf[SS_BUFSIZE];
	int		i, j;

	memset((char *)&buf, 0, SS_BUFSIZE);
	i = 0;
	while ((strchr(buf, '\n')) == NULL) {
		j = recv(smtpsock, &buf[i], SS_BUFSIZE-i, 0);
		if (j == -1) {
			WriteError("$SMTP: error reading socket");
			memset((char *)&buf, 0, SS_BUFSIZE);
			return buf;
		}
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



int smtp_close(void)
{
	if (smtpsock == -1)
		return 0;

	smtp_cmd((char *)"QUIT\r\n", 221);

	if (shutdown(smtpsock, 1) == -1) {
		WriteError("$SMTP: can't close socket");
		return -1;
	}

	smtpsock = -1;
	Syslog('+', "SMTP: closed");
	return 0;
}



/*
 * Send command to the SMTP service. On error return the
 * received error number, else return zero.
 */
int smtp_cmd(char *cmd, int resp)
{
	char	*p, rsp[6];

	if (smtp_send(cmd) == -1)
		return -1;

	snprintf(rsp, 6, "%d", resp);
	p = smtp_receive();

	if (strncmp(p, rsp, strlen(rsp))) {
		WriteError("SMTP> %s", cmd);
		WriteError("SMTP< %s", p);
		memset(&resp, 0, sizeof(rsp));
		strncpy(rsp, p, 3);
		return atoi(rsp);
	}
	return 0;
}



