/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet mailer 
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/nodelist.h"
#include "../lib/dbnode.h"
#include "session.h"
#include "ttyio.h"
#include "openport.h"
#include "telnet.h"
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
static int	tcp_is_open = FALSE;



/* opentcp() was rewritten by Martin Junius */

int opentcp(char *name)
{
    struct servent	*se;
    struct hostent	*he;
    struct sockaddr_in  server;
    int			a1, a2, a3, a4, rc, Fd, Fdo, GotPort = FALSE;
    char		*errmsg, *portname;
    short		portnum;
    int			input_pipe[2], output_pipe[2];
    pid_t		fpid;

    Syslog('+', "Open TCP connection to \"%s\"", MBSE_SS(name));

    tcp_is_open = FALSE;
    server.sin_family = AF_INET;

    /*
     * Get port number from name argument if there is a : part
     */
    if ((portname = strchr(name,':'))) {
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
#ifdef USE_EXPERIMENT
	    case TCPMODE_ITN:   if ((se = getservbyname("telnet", "tcp")))
				    server.sin_port = se->s_port;
				else
				    server.sin_port = htons(TELNPORT);
				break;
#endif
	    case TCPMODE_IBN:	if ((se = getservbyname("binkd", "tcp")))
				    server.sin_port = se->s_port;
				else
				    server.sin_port = htons(BINKPORT);
				break;
	    default:		server.sin_port = htons(FIDOPORT);
	}
    }

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
	    case TRY_AGAIN:	    errmsg = (char *)"Non-Authoritive: Host not found"; break;
	    case NO_RECOVERY:	    errmsg = (char *)"Non recoverable errors"; break;
	    default:		    errmsg = (char *)"Unknown error"; break;
	}
	Syslog('+', "No IP address for %s: %s\n", name, errmsg);
	return -1;
    }

    Syslog('d', "SIGPIPE => sigpipe()");
    signal(SIGPIPE, sigpipe);
    Syslog('d', "SIGHUP => linedrop()");
    signal(SIGHUP, linedrop);

#ifdef USE_EXPERIMENT
    if (tcp_mode == TCPMODE_ITN) {
	Syslog('s', "Installing telnet filter...");

	/*
	 * Create TCP socket and open
	 */
	if ((Fdo = socket(AF_INET,SOCK_STREAM,0)) == -1) {
	    WriteError("$Cannot create socket");
	    return -1;
	}
	if (connect(Fdo,(struct sockaddr *)&server,sizeof(server)) == -1) {
	    Syslog('+', "Cannot connect %s",inet_ntoa(server.sin_addr));
	    return -1;
	}
	Syslog('s', "socket %d", Fdo);

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
	if ((rc = pipe(output_pipe)) == -1) {
	    WriteError("$could not create output_pipe");
	    return -1;
	}
	fpid = fork();
	switch (fpid) {
	    case -1:    WriteError("fork for telout_filter failed");
			return -1;
	    case 0:     if (close(output_pipe[1]) == -1) {
			    WriteError("$error close output_pipe[1]");
			    return -1;
			}
			telout_filter(output_pipe[0], Fdo);
			/* NOT REACHED */
	}
	if (close(output_pipe[0] == -1)) {
	    WriteError("$error close output_pipe[0]");
	    return -1;
	}
	Syslog('s', "telout_filter forked with pid %d", fpid);

	/*
	 * Create input pipe and start input filter
	 */
	if ((rc = pipe(input_pipe)) == -1) {
	    WriteError("$could not create input_pipe");
	    return -1;
	}
	fpid = fork();
	switch (fpid) {
	    case -1:    WriteError("fork for telin_filter failed");
			return -1;
	    case 0:	if (close(input_pipe[0]) == -1) {
			    WriteError("$error close input_pipe[0]");
			    return -1;
			}
			telin_filter(input_pipe[1], Fdo);
			/* NOT REACHED */
	}
	if (close(input_pipe[1]) == -1) {
	    WriteError("$error close input_pipe[1]");
	    return -1;
	}
	Syslog('s', "telin_filter forked with pid %d", fpid);

	Syslog('s', "stdout = %d", output_pipe[1]);
	Syslog('s', "stdin  = %d", input_pipe[0]);

	if ((input_pipe[0] != 0) || (output_pipe[1] != 1)) {
	    WriteError("Failed to create pipes on stdin and stdout");
	    return -1;
	}

	Syslog('+', "Telnet I/O filters installed");
	telnet_init(Fdo); /* Do we need that as originating system? */
	f_flags=0;

    } else {
#endif
	/*
	 * Transparant 8 bits connection
	 */
	fflush(stdin);
	fflush(stdout);
	setbuf(stdin,NULL);
	setbuf(stdout,NULL);
	close(0);
	close(1);

	if ((Fd = socket(AF_INET,SOCK_STREAM,0)) != 0) {
	    WriteError("$Cannot create socket (got %d, expected 0)", Fd);
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
#ifdef USE_EXPERIMENT
    }
#endif

    Syslog('+', "Established %s/TCP connection with %s, port %d", 
	(tcp_mode == TCPMODE_IFC) ? "IFC":(tcp_mode == TCPMODE_ITN) ?"ITN":(tcp_mode == TCPMODE_IBN) ? "IBN":"Unknown",
	inet_ntoa(server.sin_addr), (int)ntohs(server.sin_port));
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

#ifdef USE_EXPERIMENT
    if (tcp_mode == TCPMODE_ITN) {
	/*
	 * Check if telout thread is running
	 */
    }
#endif

    shutdown(fd, 2);
    Syslog('d', "SIGHUP => SIG_IGN");
    signal(SIGHUP, SIG_IGN);
    Syslog('d', "SIGPIPE => SIG_IGN");
    signal(SIGPIPE, SIG_IGN);

    if (carrier) {
	c_end = time(NULL);
	online += (c_end - c_start);
	Syslog('+', "Closing TCP connection, connected %s", t_elapsed(c_start, c_end));
	carrier = FALSE;
	history.offline = c_end;
	history.online  = c_start;
	history.sent_bytes = sentbytes;
	history.rcvd_bytes = rcvdbytes;
	history.inbound = ~master;
	tmp = calloc(PATH_MAX, sizeof(char));
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


