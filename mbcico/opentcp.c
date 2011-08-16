/*****************************************************************************
 *
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
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
extern int	sentbytes;
extern int	rcvdbytes;
extern int	Loaded;
static int	tcp_is_open = FALSE;



/*
 * Parameter may be:
 *    host.example.com  
 *    host.example.com:portnumber
 *    host.example.com:portname
 */
int opentcp(char *servname)
{
    struct servent	*se;
    struct sockaddr_in  server;
    int			rc, GotPort = FALSE;
    char		*portname, *ipver = NULL, servport[20], ipstr[INET6_ADDRSTRLEN];
    u_int16_t		portnum;
    struct addrinfo	hints, *res=NULL, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    Syslog('+', "Open TCP connection to \"%s\"", MBSE_SS(servname));
    tcp_is_open = FALSE;

    /*
     * Get port number from name argument if there is a : part
     */
    if ((portname = strchr(servname,':'))) {
	*portname++='\0';
	if ((portnum = atoi(portname))) {
	    server.sin_port=htons(portnum);
	    GotPort = TRUE;
	} else if ((se = getservbyname(portname, "tcp"))) {
	    server.sin_port = se->s_port;
	    GotPort = TRUE;
	}
    }

    /*
     * If not a forced port number, get the defaults.
     */
    if (! GotPort) {
	switch (tcp_mode) {
	    case TCPMODE_IFC:	if ((se = getservbyname("fido", "tcp")))
				    server.sin_port = se->s_port;
				else
				    server.sin_port = htons(FIDOPORT);
				break;
	    case TCPMODE_ITN:   if ((se = getservbyname("telnet", "tcp")))
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
    snprintf(servport, 19, "%d", ntohs(server.sin_port));

    /*
     * Lookup hostname and return list of IPv4 and or IPv6 addresses.
     */
    if ((rc = getaddrinfo(servname, servport, &hints, &res))) {
	if (rc == EAI_SYSTEM)
	    WriteError("opentcp: getaddrinfo() failed");
	else
	    Syslog('+', "Host not found --> %s\n", gai_strerror(rc));
	return -1;
    }

    signal(SIGPIPE, sigpipe);
    signal(SIGHUP, linedrop);

    fflush(stdin);
    fflush(stdout);
    setbuf(stdin,NULL);
    setbuf(stdout,NULL);
    close(0);
    close(1);

    /*
     * In case a node has a A and AAAA dns entry, we now have a list of
     * possible connections to make, we try them until we succeed.
     * Most likely, the first one is ok.
     * Nice that this also works for clustered hosts, not that we will
     * find any in fidonet, but .....
     */
    for (p = res; p != NULL; p = p->ai_next) {
	void	*addr;

	rc = 0;
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
	Syslog('+', "Trying %s %s port %s", ipver, ipstr, servport);

    	if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) != 0) {
	    if (fd > 0)
		WriteError("Cannot create socket (got %d, expected 0)", fd);
	    else
            	WriteError("$Cannot create socket");
            rc = -1;
    	} else {
    	    if (dup(fd) != 1) {
        	WriteError("$Cannot dup socket");
        	rc = -1;
            } else {
    		clearerr(stdin);
    		clearerr(stdout);
    		if (connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
        	    Syslog('+', "$Cannot connect %s port %s", ipstr, servport);
		    close(fd+1);	/* close duped socket	*/
		    close(fd);		/* close this socket	*/
		    rc = -1;
    		} else {
		    /*
		     * Connected, leave this loop
		     */
		    break;
		}
	    }
	}
    }
    if (rc == -1) {
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_WRONLY);
	freeaddrinfo(res);
	return -1;
    }

    f_flags=0;
    Syslog('+', "Established %s/TCP %s connection with %s, port %s", 
	(tcp_mode == TCPMODE_IFC) ? "IFC":(tcp_mode == TCPMODE_ITN) ?"ITN":(tcp_mode == TCPMODE_IBN) ? "IBN":"Unknown",
	ipver, ipstr, servport);
    freeaddrinfo(res);
    c_start = time(NULL);
    carrier = TRUE;
    tcp_is_open = TRUE;
    return 0;
}



void closetcp(void)
{
    FILE    *fph;
    char    *tmp;

    if (!tcp_is_open)
	return;

    shutdown(fd, SHUT_RDWR);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    if (carrier) {
	c_end = time(NULL);
	online += (c_end - c_start);
	Syslog('+', "Closing TCP connection, connected %s", t_elapsed(c_start, c_end));
	carrier = FALSE;
	history.offline = (int)c_end;
	history.online  = (int)c_start;
	history.sent_bytes = sentbytes;
	history.rcvd_bytes = rcvdbytes;
	history.inbound = ~master;
	tmp = calloc(PATH_MAX, sizeof(char));
	snprintf(tmp, PATH_MAX -1, "%s/var/mailer.hist", getenv("MBSE_ROOT"));
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


