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
#include "taskstat.h"
#include "taskutil.h"
#include "taskibc.h"



#ifdef  USE_EXPERIMENT

int		    ibc_run = FALSE;	    /* Thread running		*/
extern int	    T_Shutdown;		    /* Program shutdown		*/
extern int	    internet;		    /* Internet status		*/
time_t		    scfg_time = (time_t)0;  /* Servers config time	*/
ncs_list	    *ncsl = NULL;	    /* Neighbours list		*/
srv_list	    *servers = NULL;	    /* Active servers		*/
int		    ls;			    /* Listen socket		*/
struct sockaddr_in  myaddr_in;		    /* Listen socket address	*/
struct sockaddr_in  clientaddr_in;	    /* Remote socket address	*/
int		    changed = FALSE;	    /* Databases changed	*/
char		    crbuf[512];		    /* Chat receive buffer	*/
char		    csbuf[512];		    /* Chat send buffer		*/



typedef enum {NCS_INIT, NCS_CALL, NCS_WAITPWD, NCS_CONNECT, NCS_HANGUP, NCS_FAIL, NCS_DEAD} NCSTYPE;


static char *ncsstate[] = {
    (char *)"init", (char *)"call", (char *)"waitpwd", (char *)"connect", 
    (char *)"hangup", (char *)"fail", (char *)"dead"
};


/*
 * Internal prototypes
 */
void fill_ncslist(ncs_list **, char *, char *, char *);
void dump_ncslist(void);
void tidy_servers(srv_list **);
void add_server(srv_list **, char *, int);
void del_server(srv_list **, char *);
int send_msg(int, struct sockaddr_in, char *, char *);
void check_servers(void);
void command_pass(char *, char *);
void command_server(char *, char *);
void command_squit(char *, char *);
void receiver(struct servent *);



/*
 * Add a server to the serverlist
 */
void fill_ncslist(ncs_list **fdp, char *server, char *myname, char *passwd)
{
    ncs_list *tmp, *ta;

    tmp = (ncs_list *)malloc(sizeof(ncs_list));
    memset(tmp, 0, sizeof(tmp));
    tmp->next = NULL;
    strncpy(tmp->server, server, 63);
    strncpy(tmp->myname, myname, 63);
    strncpy(tmp->passwd, passwd, 15);
    tmp->state = NCS_INIT;
    tmp->action = time(NULL);
    tmp->last = (time_t)0;
    tmp->version = 0;
    tmp->remove = FALSE;
    tmp->socket = -1;
    tmp->token = 0;
    tmp->gotpass = FALSE;
    tmp->gotserver = FALSE;

    if (*fdp == NULL) {
	*fdp = tmp;
    } else {
	for (ta = *fdp; ta; ta = ta->next)
	if (ta->next == NULL) {
	    ta->next = (ncs_list *)tmp;
	    break;
	}
    }
}



void dump_ncslist(void)
{   
    ncs_list	*tmp;
    srv_list	*srv;
    time_t	now;

    if (!changed)
	return;

    now = time(NULL);
    Syslog('r', "Server                         State   Del Pwd Srv Next action");
    Syslog('r', "------------------------------ ------- --- --- --- -----------");
    
    for (tmp = ncsl; tmp; tmp = tmp->next) {
	Syslog('r', "%-30s %-7s %s %s %s %d", tmp->server, ncsstate[tmp->state], 
		tmp->remove ? "yes":"no ", tmp->gotpass ? "yes":"no ", 
		tmp->gotserver ? "yes":"no ", (int)tmp->action - (int)now);
    }

    Syslog('r', "Server                          Hops Users Connect time");
    Syslog('r', "------------------------------ ----- ----- --------------------");
    for (srv = servers; srv; srv = srv->next) {
	Syslog('r', "%-30s %5d %5d %s", srv->server, srv->hops, srv->users, rfcdate(srv->connected));
    }
    changed = FALSE;
}



void tidy_servers(srv_list ** fdp)
{
    srv_list *tmp, *old;

    for (tmp = *fdp; tmp; tmp = old) {
	old = tmp->next;
	free(tmp);
    }
    *fdp = NULL;
}



void add_server(srv_list **fdp, char *name, int hops)
{
    srv_list *tmp, *ta;

    tmp = (srv_list *)malloc(sizeof(srv_list));
    memset(tmp, 0, sizeof(tmp));
    tmp->next = NULL;
    strncpy(tmp->server, name, 63);
    tmp->connected = time(NULL);
    tmp->users = 0;
    tmp->hops = hops;

    if (*fdp == NULL) {
	*fdp = tmp;
    } else {
	for (ta = *fdp; ta; ta = ta->next)
	    if (ta->next == NULL) {
		ta->next = (srv_list *)tmp;
		break;
	    }
    }
}



void del_server(srv_list **fdp, char *name)
{
    srv_list *tmp, *old = NULL;

    Syslog('r', "delserver %s", name);

    for (tmp = *fdp; tmp; tmp = old) {
	Syslog('r', " loop %s", tmp->server);
	if (strcmp(tmp->server, name) == 0) {
	    if (old) {
		Syslog('r', " old is true");
		old->next = tmp->next;
	    } else {
		Syslog('r', " old = false");
		*fdp = tmp->next;
	    }
	    old = tmp->next;
	    free(tmp);
	} else {
	    old = tmp->next;
	}
    }
}



/*
 * Send a message to all servers
 */
void send_all(char *msg)
{
    ncs_list	*tnsl;

    sprintf(csbuf, "%s\r\n", msg);

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (tnsl->state == NCS_CONNECT) {
	    send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
	}
    }
}



/*
 * Send message to a server
 */
int send_msg(int s, struct sockaddr_in servaddr, char *host, char *msg)
{
    Syslog('r', "> %s: %s", host, printable(msg, 0));

    if (sendto(s, msg, strlen(msg), 0, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) == -1) {
	Syslog('r', "$IBC: can't send message");
	return -1;
    }
    return 0;
}



void check_servers(void)
{
    char	    *errmsg, scfgfn[PATH_MAX];
    FILE	    *fp;
    ncs_list	    *tnsl;
    int		    j, inlist;
    time_t	    now;
    int		    a1, a2, a3, a4;
    struct servent  *se;
    struct hostent  *he;

    sprintf(scfgfn, "%s/etc/ibcsrv.data", getenv("MBSE_ROOT"));
    
    /*
     * Check if configuration is changed, if so then apply the changes.
     */
    if (file_time(scfgfn) != scfg_time) {
	Syslog('r', "%s filetime changed, rereading");

	if ((fp = fopen(scfgfn, "r"))) {
	    fread(&ibcsrvhdr, sizeof(ibcsrvhdr), 1, fp);

	    while (fread(&ibcsrv, ibcsrvhdr.recsize, 1, fp)) {
		Syslog('r', "IBC server \"%s\", Active %s", ibcsrv.server, ibcsrv.Active ?"Yes":"No");
		if (ibcsrv.Active) {
		    inlist = FALSE;
		    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
			if (strcmp(tnsl->server, ibcsrv.server) == 0) {
			    inlist = TRUE;
			}
		    }
		    if (!inlist ) {
			Syslog('r', "  not in neighbour list, add");
			fill_ncslist(&ncsl, ibcsrv.server, ibcsrv.myname, ibcsrv.passwd);
			changed = TRUE;
			Syslog('+', "IBC: added Internet BBS Chatserver %s", ibcsrv.server);
			if (servers == NULL) {
			    /*
			     * First add this server name to the servers database.
			     */
			    add_server(&servers, ibcsrv.myname, 0);
			}
		    }
		}
	    }

	    /*
	     * Now check for neighbours to delete
	     */
	    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
		fseek(fp, ibcsrvhdr.hdrsize, SEEK_SET);
		inlist = FALSE;
		Syslog('r', "IBC server \"%s\"", ibcsrv.server);

		while (fread(&ibcsrv, ibcsrvhdr.recsize, 1, fp)) {
		    if ((strcmp(tnsl->server, ibcsrv.server) == 0) && ibcsrv.Active) {
			inlist = TRUE;
		    }
		}
		if (!inlist) {
		    Syslog('r', "  not in configuration, remove");
		    tnsl->remove = TRUE;
		    tnsl->action = time(NULL);
		    changed = TRUE;
		}
	    }
	    fclose(fp);
	}

	scfg_time = file_time(scfgfn);
    }

    dump_ncslist();

    /*
     * Check if we need to make state changes
     */
    now = time(NULL);
    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (((int)tnsl->action - (int)now) <= 0) {
	    switch (tnsl->state) {
		case NCS_INIT:	    Syslog('r', "%s init", tnsl->server);
				    changed = TRUE;

				    /*
				     * If Internet is available, setup the connection.
				     */
				    if (internet) {
					/*
					 * Get IP address for the hostname, set default next action
					 * to 60 seconds.
					 */
					tnsl->action = now + (time_t)60;
					memset(&tnsl->servaddr_in, 0, sizeof(struct sockaddr_in));
					se = getservbyname("fido", "udp");
					tnsl->servaddr_in.sin_family = AF_INET;
					tnsl->servaddr_in.sin_port = se->s_port;
					
					if (sscanf(tnsl->server,"%d.%d.%d.%d",&a1,&a2,&a3,&a4) == 4)
					    tnsl->servaddr_in.sin_addr.s_addr = inet_addr(tnsl->server);
					else if ((he = gethostbyname(tnsl->server)))
					    memcpy(&tnsl->servaddr_in.sin_addr, he->h_addr, he->h_length);
					else {
					    switch (h_errno) {
						case HOST_NOT_FOUND:    errmsg = (char *)"Authoritative: Host not found"; break;
						case TRY_AGAIN:         errmsg = (char *)"Non-Authoritive: Host not found"; break;
						case NO_RECOVERY:       errmsg = (char *)"Non recoverable errors"; break;
						default:                errmsg = (char *)"Unknown error"; break;
					    }
					    Syslog('+', "IBC: no IP address for %s: %s", tnsl->server, errmsg);
					    tnsl->action = now + (time_t)120;
					    tnsl->state = NCS_FAIL;
					    changed = TRUE;
					    break;
					}
					
					tnsl->socket = socket(AF_INET, SOCK_DGRAM, 0);
					if (tnsl->socket == -1) {
					    Syslog('+', "$IBC: can't create socket for %s", tnsl->server);
					    tnsl->state = NCS_FAIL;
					    tnsl->action = now + (time_t)120;
					    changed = TRUE;
					    break;
					}

					Syslog('r', "socket %d", tnsl->socket);
					tnsl->state = NCS_CALL;
					tnsl->action = now + (time_t)1;
				    } else {
					tnsl->action = now + (time_t)10;
				    }
				    break;
				    
		case NCS_CALL:	    /*
				     * In this state we accept PASS and SERVER commands from
				     * the remote with the same token as we have sent.
				     */
				    Syslog('r', "%s call", tnsl->server);
				    tnsl->token = gettoken();
				    sprintf(csbuf, "PASS %s 0000 IBC| %s\r\n", tnsl->passwd, tnsl->compress ? "Z":"");
				    send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
				    sprintf(csbuf, "SERVER %s 0 %ld mbsebbs v%s\r\n",  tnsl->myname, tnsl->token, VERSION);
				    send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
				    tnsl->action = now + (time_t)10;
				    tnsl->state = NCS_WAITPWD;
				    changed = TRUE;
				    break;
				    
		case NCS_WAITPWD:   /*
				     * This state can be left by before the timeout is reached
				     * by a reply from the remote if the connection is accepted.
				     */
				    Syslog('r', "%s waitpwd", tnsl->server);
				    tnsl->token = 0;
				    tnsl->state = NCS_CALL;
				    srand(getpid());
				    while (TRUE) {
					j = 1+(int) (1.0 * CFG.dialdelay * rand() / (RAND_MAX + 1.0));
					if ((j > (CFG.dialdelay / 10)) && (j > 9))
					    break;
				    }
				    Syslog('r', "next call in %d %d seconds", CFG.dialdelay, j);
				    tnsl->action = now + (time_t)j;
				    changed = TRUE;
				    break;

		case NCS_CONNECT:   /*
				     * In this state we check if the connection is still alive
				     */
				    Syslog('r', "%s connect", tnsl->server);
				    if (((int)now - (int)tnsl->last) > 70) {
					Syslog('+', "IBC: server %s connection is dead", tnsl->server);
					tnsl->state = NCS_DEAD;
					tnsl->action = now + (time_t)120;    // 2 minutes delay before calling again.
					tnsl->gotpass = FALSE;
					tnsl->gotserver = FALSE;
					tnsl->token = 0;
					changed = TRUE;
					break;
				    }
				    if (((int)now - (int)tnsl->last) > 60) {
					sprintf(csbuf, "PING\r\n");
					send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
				    }
				    tnsl->action = now + (time_t)10;
				    break;

		case NCS_HANGUP:    Syslog('r', "%s hangup", tnsl->server);
				    tnsl->action = now + (time_t)1;
				    tnsl->state = NCS_CALL;
				    changed = TRUE;
				    break;

		case NCS_DEAD:	    Syslog('r', "%s dead", tnsl->server);
				    tnsl->action = now + (time_t)1;
				    tnsl->state = NCS_CALL;
				    changed = TRUE;
				    break;

		case NCS_FAIL:	    Syslog('r', "%s fail", tnsl->server);
				    tnsl->action = now + (time_t)1;
				    tnsl->state = NCS_INIT;
				    changed = TRUE;
				    break;
	    }
	}
    }

    dump_ncslist();
}



void command_pass(char *hostname, char *parameters)
{
    ncs_list	*tnsl;
    char	*passwd, *version, *opts, *lnk;

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }

    passwd = strtok(parameters, " \0");
    version = strtok(NULL, " \0");
    opts = strtok(NULL, " \0");
    lnk = strtok(NULL, " \0");

    Syslog('r', "passwd \"%s\"", printable(passwd, 0));
    Syslog('r', "version \"%s\"", printable(version, 0));
    Syslog('r', "opts \"%s\"", printable(opts, 0));
    Syslog('r', "link \"%s\"", printable(lnk, 0));

    if (version == NULL) {
	sprintf(csbuf, "461 PASS: Not enough parameters\r\n");
	send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
	return;
    }

    if (strcmp(passwd, tnsl->passwd)) {
	Syslog('!', "IBC: got bad password %s from %s", passwd, hostname);
	return;
    }

    tnsl->gotpass = TRUE;
    tnsl->version = atoi(version);
    if (lnk && strchr(lnk, 'Z'))
	tnsl->compress = TRUE;
    changed = TRUE;
}



void command_server(char *hostname, char *parameters)
{
    ncs_list	    *tnsl;
    char	    *name, *hops, *id, *vers;
    time_t	    now;
    unsigned long   token;

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }

    name = strtok(parameters, " \0");
    hops = strtok(NULL, " \0");
    id = strtok(NULL, " \0");
    vers = strtok(NULL, "\0");

    Syslog('r', "name \"%s\"", printable(name, 0));
    Syslog('r', "hops \"%s\"", printable(hops, 0));
    Syslog('r', "id \"%s\"", printable(id, 0));
    Syslog('r', "vers \"%s\"", printable(vers, 0));

    if (id == NULL) {
	sprintf(csbuf, "461 SERVER: Not enough parameters\r\n");
	send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
	return;
    }

    token = atoi(id);
    now = time(NULL);

    if (tnsl->token) {
	/*
	 * We are in calling state, so we expect the token from the
	 * remote is the same as the token we sent.
	 * In that case, the session is authorized.
	 */
	if (tnsl->token == token) {
	    tnsl->gotserver = TRUE;
	    changed = TRUE;
	    tnsl->state = NCS_CONNECT;
	    tnsl->action = now + (time_t)10;
	    Syslog('+', "IBC: connected with %s", tnsl->server);
	    add_server(&servers, tnsl->server, 1);
	    return;
	}
	Syslog('r', "IBC: collision with %s", tnsl->server);
	return;
    }

    /*
     * We are in waiting state, so we sent our PASS and SERVER
     * messages and set the session to connected if we got a
     * valid PASS command.
     */
    if (tnsl->gotpass) {
	sprintf(csbuf, "PASS %s 0000 IBC| %s\r\n", tnsl->passwd, tnsl->compress ? "Z":"");
	send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
	sprintf(csbuf, "SERVER %s 0 %ld mbsebbs v%s\r\n",  tnsl->myname, token, VERSION);
	send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
	tnsl->gotserver = TRUE;
	tnsl->state = NCS_CONNECT;
	tnsl->action = now + (time_t)10;
	Syslog('+', "IBC: connected with %s", tnsl->server);
	add_server(&servers, tnsl->server, 1);
	changed = TRUE;
    } else {
	Syslog('r', "IBC: got SERVER command without PASS command from %s", hostname);
    }
    return;
}



void command_squit(char *hostname, char *parameters)
{
    ncs_list        *tnsl;
    char            *name, *message;

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }

    name = strtok(parameters, " \0");
    message = strtok(NULL, "\0");

    if (strcmp(name, tnsl->server) == 0) {
	Syslog('+', "IBC: disconnect server %s: %s", name, message);
	tnsl->state = NCS_HANGUP;
	tnsl->action = time(NULL) + (time_t)120;	// 2 minutes delay before calling again.
	tnsl->gotpass = FALSE;
	tnsl->gotserver = FALSE;
	tnsl->token = 0;
    } else {
	Syslog('r', "IBC: disconnect server %s: message is not for us, but update database");
    }
    del_server(&servers, name);
    changed = TRUE;
}



void receiver(struct servent  *se)
{
    struct pollfd   pfd;
    struct hostent  *hp;
    int             rc, len, inlist;
    socklen_t       sl;
    ncs_list	    *tnsl;
    char            temp[512], *hostname, *prefix, *command, *parameters;

    pfd.fd = ls;
    pfd.events = POLLIN;
    pfd.revents = 0;

    if ((rc = poll(&pfd, 1, 1000) < 0)) {
	Syslog('r', "$poll/select failed");
	return;
    } 
	
    if (pfd.revents & POLLIN || pfd.revents & POLLERR || pfd.revents & POLLHUP || pfd.revents & POLLNVAL) {
	sl = sizeof(myaddr_in);
	memset(&clientaddr_in, 0, sizeof(struct sockaddr_in));
        memset(&crbuf, 0, sizeof(crbuf));
        if ((len = recvfrom(ls, &crbuf, sizeof(crbuf)-1, 0,(struct sockaddr *)&clientaddr_in, &sl)) != -1) {
	    hp = gethostbyaddr((char *)&clientaddr_in.sin_addr, sizeof(struct in_addr), clientaddr_in.sin_family);
	    if (hp == NULL)
	        hostname = inet_ntoa(clientaddr_in.sin_addr);
	    else
	        hostname = hp->h_name;

	    if ((crbuf[strlen(crbuf) -2] != '\r') && (crbuf[strlen(crbuf) -1] != '\n')) {
	        Syslog('r', "Message not terminated with CR-LF, dropped");
	        return;
	    }

	    inlist = FALSE;
	    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
		if (strcmp(tnsl->server, hostname) == 0) {
		    inlist = TRUE;
		    break;
		}
	    }
	    if (!inlist) {
		Syslog('!', "IBC: message from unknown host (%s), dropped", hostname);
		return;
	    }

	    if (tnsl->state == NCS_INIT) {
		Syslog('r', "IBC: message received from %s while in init state, dropped", hostname);
		return;
	    }

	    tnsl->last = time(NULL);
	    crbuf[strlen(crbuf) -2] = '\0';
	    Syslog('r', "< %s: \"%s\"", hostname, printable(crbuf, 0));

	    /*
	     * Parse message
	     */
	    if (crbuf[0] == ':') {
		prefix = strtok(crbuf, " ");
		command = strtok(NULL, " \0");
		parameters = strtok(NULL, "\0");
	    } else {
		prefix = NULL;
		command = strtok(crbuf, " \0");
		parameters = strtok(NULL, "\0");
	    }
//	    Syslog('r', "prefix     \"%s\"", printable(prefix, 0));
//	    Syslog('r', "command    \"%s\"", printable(command, 0));
//	    Syslog('r', "parameters \"%s\"", printable(parameters, 0));

	    if (! strcmp(command, (char *)"PASS")) {
		if (parameters == NULL) {
		    sprintf(csbuf, "461 %s: Not enough parameters\r\n", command);
		    send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
		} else {
		    command_pass(hostname, parameters);
		}
	    } else if (! strcmp(command, (char *)"SERVER")) {
		if (parameters == NULL) {
		    sprintf(csbuf, "461 %s: Not enough parameters\r\n", command);
		    send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
		} else {
		    command_server(hostname, parameters);
		}
	    } else if (! strcmp(command, (char *)"PING")) {
		sprintf(csbuf, "PONG\r\n");
		send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
	    } else if (! strcmp(command, (char *)"PONG")) {
		/*
		 * Just accept
		 */
	    } else if (! strcmp(command, (char *)"SQUIT")) {
		if (parameters == NULL) {
		    sprintf(csbuf, "461 %s: Not enough parameters\r\n", command);
		    send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
		} else {
		    sprintf(temp, "SQUIT %s %s", hostname, parameters);
		    command_squit(hostname, parameters);
		    send_all(temp);
		}
	    } else if (atoi(command)) {
		Syslog('r', "IBC: Got error %d", atoi(command));
	    } else if (tnsl->state == NCS_CONNECT) {
		/*
		 * Only if connected we send a error response
		 */
		sprintf(csbuf, "421 %s: Unknown command\r\n", command);
		send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
	    }
	} else {
	    Syslog('r', "recvfrom returned len=%d", len);
	}
    }
}



/*
 * IBC thread
 */
void *ibc_thread(void *dummy)
{
    struct servent  *se;
    ncs_list        *tnsl;

    Syslog('+', "Starting IBC thread");

    if ((se = getservbyname("fido", "udp")) == NULL) {
	Syslog('!', "IBC: no fido udp entry in /etc/services, cannot start Internet BBS Chat");
	goto exit;
    }

    myaddr_in.sin_family = AF_INET;
    myaddr_in.sin_addr.s_addr = INADDR_ANY;
    myaddr_in.sin_port = se->s_port;
    Syslog('+', "IBC: listen on %s, port %d", inet_ntoa(myaddr_in.sin_addr), ntohs(myaddr_in.sin_port));

    ls = socket(AF_INET, SOCK_DGRAM, 0);
    if (ls == -1) {
	Syslog('!', "$IBC: can't create listen socket");
	goto exit;
    }

    if (bind(ls, (struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1) {
	Syslog('!', "$IBC: can't bind listen socket");
	goto exit;
    }

    ibc_run = TRUE;

    while (! T_Shutdown) {
	/*
	 * Check neighbour servers state
	 */
	check_servers();

	/*
	 * Get any incoming messages
	 */
	receiver(se);
    }

    Syslog('r', "IBC: start shutdown connections");
    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (tnsl->state == NCS_CONNECT) {
	    sprintf(csbuf, "SQUIT %s System shutdown\r\n", tnsl->myname);
	    send_msg(tnsl->socket, tnsl->servaddr_in, tnsl->server, csbuf);
	}
    }

    tidy_servers(&servers);

exit:
    ibc_run = FALSE;
    Syslog('+', "IBC thread stopped");
    pthread_exit(NULL);
}


#endif
