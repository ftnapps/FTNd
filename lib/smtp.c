/*****************************************************************************
 *
 * $Id: smtp.c,v 1.10 2007/08/25 15:29:14 mbse Exp $
 * Purpose ...............: MBSE BBS Internet Library
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
#include "mbselib.h"
#include "mbinet.h"


static int		smtpsock = -1;	/* TCP/IP socket		*/
struct hostent		*shp;		/* Host info remote		*/
struct servent		*ssp;		/* Service information		*/
struct sockaddr_in	smtp_loc;	/* For local socket address	*/
struct sockaddr_in	smtp_rem;	/* For remote socket address	*/



int smtp_connect(void)
{
	socklen_t addrlen;
	char	*p, temp[40];

	if (smtpsock != -1)
		return smtpsock;

	if (!strlen(CFG.smtpnode)) {
		WriteError("SMTP: host not configured");
		return -1;
	}

	Syslog('+', "SMTP: connecting host: %s", CFG.smtpnode);
	memset(&smtp_loc, 0, sizeof(struct sockaddr_in));
	memset(&smtp_rem, 0, sizeof(struct sockaddr_in));

	smtp_rem.sin_family = AF_INET;

	if ((shp = gethostbyname(CFG.smtpnode)) == NULL) {
		WriteError("SMTP: can't find host %s", CFG.smtpnode);
		return -1;
	}

	smtp_rem.sin_addr.s_addr = ((struct in_addr *)(shp->h_addr))->s_addr;

	if ((ssp = getservbyname("smtp", "tcp")) == NULL) {
		WriteError("SMTP: can't find service port for smtp/tcp");
		return -1;
	}
	smtp_rem.sin_port = ssp->s_port;

	if ((smtpsock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		WriteError("$SMTP: unable to create tcp socket");
		return -1;
	}

	if (connect(smtpsock, (struct sockaddr *)&smtp_rem, sizeof(struct sockaddr_in)) == -1) {
		WriteError("$SMTP: can't connect tcp socket");
		return -1;
	}

	addrlen = sizeof(struct sockaddr_in);

	if (getsockname(smtpsock, (struct sockaddr *)&smtp_loc, &addrlen) == -1) {
		WriteError("$SMTP: unable to read socket address");
		return -1;
	}

	p = smtp_receive();
	if (strlen(p) == 0) {
		WriteError("SMTP: no response");
		smtp_close();
		return -1;
	}

	if (strncmp(p, "220", 3)) {
		WriteError("SMTP: bad response: %s", p);
		smtp_close();
		return -1;
	}

	Syslog('+', "SMTP: %s", p);
	
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



