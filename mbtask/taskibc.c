/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: mbtask - Internet BBS Chat (but it looks like...)
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "../lib/mbselib.h"
#include "taskibc.h"



#ifdef  USE_EXPERIMENT

int		ibc_run = FALSE;	    /* Thread running	    */
extern int	T_Shutdown;		    /* Program shutdown	    */



/*
 * Send a UDP message to one other server.
 */
void send_server(char *name, char *msg)
{
    int			a1, a2, a3, a4;
    int                 s;		    /* Socket                       */
    struct hostent      *he;		    /* Host info                    */
    struct servent	*se;		    /* Service information          */
    struct sockaddr_in	server;		    /* Destination address          */
    char		*errmsg;

    /*
     * Get IP address for the hostname
     */
    if (sscanf(name,"%d.%d.%d.%d",&a1,&a2,&a3,&a4) == 4)
	server.sin_addr.s_addr = inet_addr(name);
    else if ((he = gethostbyname(name)))
	memcpy(&server.sin_addr,he->h_addr,he->h_length);
    else {
	switch (h_errno) {
	    case HOST_NOT_FOUND:    errmsg = (char *)"Authoritative: Host not found"; break;
	    case TRY_AGAIN:         errmsg = (char *)"Non-Authoritive: Host not found"; break;
	    case NO_RECOVERY:       errmsg = (char *)"Non recoverable errors"; break;
	    default:                errmsg = (char *)"Unknown error"; break;
	}
	Syslog('+', "No IP address for %s: %s", name, errmsg);
	return;
    }

    se = getservbyname("fido", "udp");
    if (se == NULL) {
	Syslog('n', "Service fido udp not in /etc/services");
	return;
    }

    server.sin_family = AF_INET;
    server.sin_port = se->s_port;

    Syslog('r', "Send to %s, port %d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
	Syslog('r', "$Can't create socket");
	return;
    }

    if (bind(s, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) == -1) {
	Syslog('r', "$Can't bind socket");
	return;
    }

    if (sendto(s, msg, strlen(msg), 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) == -1) {
	Syslog('r', "$Can't send message");
	return;
    }
}



/*
 * Send a message to all servers
 */
void send_all(char *msg)
{
    send_server((char *)"router.mbse.ym", msg);
    send_server((char *)"hppa.mbse.ym", msg);
}



/*
 * IRC thread
 */
void *ibc_thread(void *dummy)
{
    struct servent  *se;

    if ((se = getservbyname("fido", "udp")) == NULL) {
	Syslog('!', "No fido udp entry in /etc/services, cannot start Internet BBS Chat");
	goto exit;
    }

    Syslog('+', "Starting IBC thread");
    ibc_run = TRUE;

    while (! T_Shutdown) {
	sleep(1);
    }

exit:
    ibc_run = FALSE;
    Syslog('+', "IBC thread stopped");
    pthread_exit(NULL);
}



#endif


