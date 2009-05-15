/*****************************************************************************
 *
 * $Id: nntp.c,v 1.16 2008/12/28 12:20:14 mbse Exp $
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


static int		nntpsock = -1;	/* TCP/IP socket		*/
struct hostent		*nhp;		/* Host info remote		*/
struct servent		*nsp;		/* Service information		*/
struct sockaddr_in	nntp_loc;	/* For local socket address	*/
struct sockaddr_in	nntp_rem;	/* For remote socket address	*/




int nntp_connect(void)
{
    socklen_t	addrlen;
    char	*p;

    if (nntpsock != -1)
	return nntpsock;

    if (!strlen(CFG.nntpnode)) {
	WriteError("NNTP: host not configured");
	return -1;
    }
	
    Syslog('+', "NNTP: connecting host: %s:%d", CFG.nntpnode, CFG.nntpport);
    memset(&nntp_loc, 0, sizeof(struct sockaddr_in));
    memset(&nntp_rem, 0, sizeof(struct sockaddr_in));

    nntp_rem.sin_family = AF_INET;
	
    if ((nhp = gethostbyname(CFG.nntpnode)) == NULL) {
	WriteError("NNTP: can't find host %s", CFG.nntpnode);
	return -1;
    }

    nntp_rem.sin_addr.s_addr = ((struct in_addr *)(nhp->h_addr))->s_addr;
    nntp_rem.sin_port = htons(CFG.nntpport);

    if ((nntpsock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	WriteError("$NNTP: unable to create tcp socket");
	return -1;
    }

    if (connect(nntpsock, (struct sockaddr *)&nntp_rem, sizeof(struct sockaddr_in)) == -1) {
	WriteError("$NNTP: cannot connect tcp socket");
	return -1;
    }

    addrlen = sizeof(struct sockaddr_in);

    if (getsockname(nntpsock, (struct sockaddr *)&nntp_loc, &addrlen) == -1) {
	WriteError("$NNTP: unable to read socket address");
	return -1;
    }

    p = nntp_receive();
    if (strlen(p) == 0) {
	WriteError("NNTP: no response");
	nntp_close();
	return -1;
    }
    Syslog('+', "NNTP: %s", p);

    if ((strncmp(p, "480", 3) == 0) || CFG.nntpforceauth) {
	/*
	 *  Must login with username and password
	 */
	if (nntp_auth() == FALSE) {
	    WriteError("Authorisation failure");
	    nntp_close();
	    return -1;
	}
    } else if (strncmp(p, "200", 3)) {
	WriteError("NNTP: bad response: %s", p);
//		nntp_close();  FIXME: Don't close, the other end might have done that already
		//		      If we do also, this program hangs. Must be fixed!
	return -1;
    }

    if (CFG.modereader) {
	Syslog('+', "NNTP: setting mode reader");
	nntp_send((char *)"MODE READER\r\n");
	p = nntp_receive();
	Syslog('+', "NNTP: %s", p);
	if (strncmp(p, "480", 3) == 0) {
	    /*
	     *  Must login with username and password
	     */
	    Syslog('+', "NNTP: %s", p);
	    if (nntp_auth() == FALSE) {
		WriteError("NNTP: authorisation failure");
		nntp_close();
		return -1;
	    }
	} else if (strncmp(p, "200", 3)) {
	    WriteError("NNTP: bad response: %s", p);
	    nntp_close();
	    return -1;
	}
    }
    return nntpsock;
}



int nntp_send(char *buf)
{
    if (nntpsock == -1)
	return -1;

    if (send(nntpsock, buf, strlen(buf), 0) != strlen(buf)) {
	WriteError("$NNTP: socket send failed");
	if (errno == ENOTCONN || errno == EPIPE) {
	    WriteError("NNTP: closing local side");
	    nntpsock = -1;
	}
	return -1;
    }
    return 0;
}



/*
 *  Return empty buffer if something went wrong, else the complete
 *  dataline is returned
 */
char *nntp_receive(void)
{
    static char	buf[SS_BUFSIZE];
    int		i = 0, j;

    if (nntpsock == -1)
	return NULL;

    memset((char *)&buf, 0, SS_BUFSIZE);
    while (TRUE) {
	j = recv(nntpsock, &buf[i], 1, 0);
	if (j == -1) {
	    WriteError("$NNTP: error reading socket");
	    memset((char *)&buf, 0, SS_BUFSIZE);
	    if (errno == ENOTCONN || errno == EPIPE) {
		WriteError("NNTP: closing local side");
		nntpsock = -1;
	    }
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



int nntp_close(void)
{
	if (nntpsock == -1)
		return 0;

	nntp_cmd((char *)"QUIT\r\n", 205);

	if (shutdown(nntpsock, 1) == -1) {
		WriteError("$NNTP: can't close socket");
		return -1;
	}

	nntpsock = -1;
	Syslog('+', "NNTP: closed");
	return 0;
}



/*
 *  Send NNTP command, check response code. 
 *  If the code doesn't match, the value is returned, else zero.
 */
int nntp_cmd(char *cmd, int resp)
{
	char	*p, rsp[6];

	if (nntp_send(cmd) == -1)
		return -1;

	snprintf(rsp, 6, "%d", resp);
	p = nntp_receive();

        if (strncmp(p, "480", 3) == 0) {
                /*
                 *  Must login with username and password
                 */
		Syslog('+', "NNTP: %s", p);
                if (nntp_auth() == FALSE) {
                        WriteError("Authorisation failure");
                        nntp_close();
                        return -1;
                }
		/*
		 *  Now send command again, we are now authorized.
		 */
		if (nntp_send(cmd) == -1)
			return -1;
		p = nntp_receive();
	}

	if (strncmp(p, rsp, strlen(rsp))) {
		WriteError("NNTP> %s", cmd);
		WriteError("NNTP< %s", p);
		memset(&resp, 0, sizeof(rsp));
		strncpy(rsp, p, 3);
		return atoi(rsp);
	}
	return 0;
}



int nntp_auth(void)
{
	char	*cmd;

	if (!(strlen(CFG.nntpuser) && strlen(CFG.nntppass))) {
		WriteError("NNTP: password required but not configured");
		return FALSE;
	}
	cmd = calloc(128, sizeof(char));

	snprintf(cmd, 128, "AUTHINFO USER %s\r\n", CFG.nntpuser);
	if (nntp_cmd(cmd, 381))
		return FALSE;

	snprintf(cmd, 128, "AUTHINFO PASS %s\r\n", CFG.nntppass);
	if (nntp_cmd(cmd, 281) == 0) {
		free(cmd);
		Syslog('+', "NNTP: logged in");
		return TRUE;
	} else {
		free(cmd);
		return FALSE;
	}
}


