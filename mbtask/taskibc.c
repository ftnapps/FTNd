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
#include "taskchat.h"
#include "taskibc.h"



#ifdef  USE_EXPERIMENT

int		    ibc_run = FALSE;	    /* Thread running		*/
extern int	    T_Shutdown;		    /* Program shutdown		*/
extern int	    internet;		    /* Internet status		*/
time_t		    scfg_time = (time_t)0;  /* Servers config time	*/
time_t		    now;		    /* Current time		*/
ncs_list	    *ncsl = NULL;	    /* Neighbours list		*/
srv_list	    *servers = NULL;	    /* Active servers		*/
usr_list	    *users = NULL;	    /* Active users		*/
chn_list	    *channels = NULL;	    /* Active channels		*/
ban_list	    *banned = NULL;	    /* Banned users		*/
nick_list	    *nicknames = NULL;	    /* Known nicknames		*/
int		    ls;			    /* Listen socket		*/
struct sockaddr_in  myaddr_in;		    /* Listen socket address	*/
struct sockaddr_in  clientaddr_in;	    /* Remote socket address	*/
char		    crbuf[512];		    /* Chat receive buffer	*/
int		    srvchg = FALSE;	    /* Is serverlist changed	*/
int		    usrchg = FALSE;	    /* Is userlist changed	*/
int		    chnchg = FALSE;	    /* Is channellist changed	*/
int		    banchg = FALSE;	    /* Is banned users changed	*/
int		    nickchg = FALSE;	    /* Is nicknames changed	*/



pthread_mutex_t b_mutex = PTHREAD_MUTEX_INITIALIZER;


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
void add_server(srv_list **, char *, int, char *, char *, char *, char *);
void del_server(srv_list **, char *);
void del_router(srv_list **, char *);
int  send_msg(ncs_list *, const char *, ...);
void broadcast(char *, const char *, ...);
void check_servers(void);
int  command_pass(char *, char *);
int  command_server(char *, char *);
int  command_squit(char *, char *);
int  command_user(char *, char *);
int  command_quit(char *, char *);
int  command_nick(char *, char *);
int  command_join(char *, char *);
int  command_part(char *, char *);
int  command_topic(char *, char *);
int  command_privmsg(char *, char *);
void receiver(struct servent *);



/*
 * Add a server to the serverlist
 */
void fill_ncslist(ncs_list **fdp, char *server, char *myname, char *passwd)
{
    ncs_list	*tmp, *ta;

    pthread_mutex_lock(&b_mutex);

    tmp = (ncs_list *)malloc(sizeof(ncs_list));
    memset(tmp, 0, sizeof(tmp));
    tmp->next = NULL;
    strncpy(tmp->server, server, 63);
    strncpy(tmp->myname, myname, 63);
    strncpy(tmp->passwd, passwd, 15);
    tmp->state = NCS_INIT;
    tmp->action = now;
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
	for (ta = *fdp; ta; ta = ta->next) {
	    if (ta->next == NULL) {
		ta->next = (ncs_list *)tmp;
		break;
	    }
	}
    }

    pthread_mutex_unlock(&b_mutex);
}



void dump_ncslist(void)
{   
    ncs_list	*tmp;
    srv_list	*srv;
    usr_list	*usrp;
    chn_list	*chnp;

    if (!srvchg && !usrchg && !chnchg && !banchg && !nickchg)
	return;

    if (srvchg) {
	if (ncsl) {
	    Syslog('r', "Server                         State   Del Pwd Srv Next action");
	    Syslog('r', "------------------------------ ------- --- --- --- -----------");
	    for (tmp = ncsl; tmp; tmp = tmp->next) {
		Syslog('r', "%-30s %-7s %s %s %s %d", tmp->server, ncsstate[tmp->state], 
		    tmp->remove ? "yes":"no ", tmp->gotpass ? "yes":"no ", 
		    tmp->gotserver ? "yes":"no ", (int)tmp->action - (int)now);
	    }
	} else {
	    Syslog('r', "No servers configured");
	}

	if (servers) {
	    Syslog('+', "IBC: Server                    Router                     Hops Users Connect time");
	    Syslog('+', "IBC: ------------------------- ------------------------- ----- ----- --------------------");
	    for (srv = servers; srv; srv = srv->next) {
		Syslog('+', "IBC: %-25s %-25s %5d %5d %s", srv->server, srv->router, srv->hops, 
			srv->users, rfcdate(srv->connected));
	    }
	} else {
	    Syslog('+', "IBC: Servers list is empty");
	}
    }
    
    if (usrchg) {
	if (users) {
	    Syslog('+', "IBC: Server               User                 Name/Nick Channel       Sys Connect time");
	    Syslog('+', "IBC: -------------------- -------------------- --------- ------------- --- --------------------");
	    for (usrp = users; usrp; usrp = usrp->next) {
		Syslog('+', "IBC: %-20s %-20s %-9s %-13s %s %s", usrp->server, usrp->realname, usrp->nick, usrp->channel,
		    usrp->sysop ? "yes":"no ", rfcdate(usrp->connected));
	    }
	} else {
	    Syslog('+', "IBC: Users list is empty");
	}
    }

    if (chnchg) {
	if (channels) {
	    Syslog('+', "IBC: Channel              Owner     Topic                               Usr Created");
	    Syslog('+', "IBC: -------------------- --------- ----------------------------------- --- --------------------");
	    for (chnp = channels; chnp; chnp = chnp->next) {
		Syslog('+', "IBC: %-20s %-9s %-35s %3d %s", chnp->name, chnp->owner, chnp->topic, 
			chnp->users, rfcdate(chnp->created));
	    }
	} else {
	    Syslog('+', "IBC: Channels list is empty");
	}
    }

    srvchg = FALSE;
    usrchg = FALSE;
    chnchg = FALSE;
    banchg = FALSE;
    nickchg = FALSE;
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



/*
 * Add one user to the userlist. Returns:
 *  0 = Ok
 *  1 = User already registered.
 */
int add_user(usr_list **fap, char *server, char *name, char *realname)
{
    usr_list    *tmp, *ta;
    srv_list	*sl;

    Syslog('r', "add_user %s %s %s", server, name, realname);

    for (ta = *fap; ta; ta = ta->next) {
	if ((strcmp(ta->server, server) == 0) && (strcmp(ta->realname, realname) == 0)) {
	    Syslog('-', "IBC: add_user(%s %s %s), already registered", server, name, realname);
	    return 1;
	}
    }

    pthread_mutex_lock(&b_mutex);

    tmp = (usr_list *)malloc(sizeof(usr_list));
    memset(tmp, 0, sizeof(usr_list));
    tmp->next = NULL;
    strncpy(tmp->server, server, 63);
    strncpy(tmp->name, name, 9);
    strncpy(tmp->nick, name, 9);
    strncpy(tmp->realname, realname, 36);
    tmp->connected = now;

    if (*fap == NULL) {
	*fap = tmp;
    } else {
	for (ta = *fap; ta; ta = ta->next) {
	    if (ta->next == NULL) {
		ta->next = (usr_list *)tmp;
		break;
	    }
	}
    }

    for (sl = servers; sl; sl = sl->next) {
	if (strcmp(sl->server, server) == 0) {
	    sl->users++;
	    srvchg = TRUE;
	}
    }

    pthread_mutex_unlock(&b_mutex);

    usrchg = TRUE;
    return 0;
}



/*
 * Delete one user. If name == NULL then delete all users of a server.
 */
void del_user(usr_list **fap, char *server, char *name)
{
    usr_list    **tmp, *tmpa;
    srv_list	*sl;

    Syslog('r', "deluser %s %s", server, printable(name, 0));

    if (*fap == NULL)
	return;

    if (name)
	pthread_mutex_lock(&b_mutex);

    tmp = fap;
    while (*tmp) {
	if (name && (strcmp((*tmp)->server, server) == 0) && (strcmp((*tmp)->name, name) == 0)) {
	    tmpa = *tmp;
	    *tmp=(*tmp)->next;
	    free(tmpa);
	    usrchg = TRUE;
	} else if ((name == NULL) && (strcmp((*tmp)->server, server) == 0)) {
	    Syslog('r', "removed user %s from %s", (*tmp)->name, (*tmp)->server);
	    tmpa = *tmp;
	    *tmp=(*tmp)->next;
	    free(tmpa);
	    usrchg = TRUE;
	} else {
	    tmp = &((*tmp)->next);
	}
    }

    for (sl = servers; sl; sl = sl->next) {
	if ((strcmp(sl->server, server) == 0) && sl->users) {
	    sl->users--;
	    srvchg = TRUE;
	}
    }

    if (name)
	pthread_mutex_unlock(&b_mutex);
}



/*
 * Add channel with owner.
 */
int add_channel(chn_list **fap, char *name, char *owner, char *server)
{
    chn_list    *tmp, *ta;

    Syslog('r', "add_channel %s %s %s", name, owner, server);

    for (ta = *fap; ta; ta = ta->next) {
	if ((strcmp(ta->name, name) == 0) && (strcmp(ta->owner, owner) == 0) && (strcmp(ta->server, server) == 0)) {
	    Syslog('-', "IBC: add_channel(%s %s %s), already registered", name, owner, server);
	    return 1;
	}
    }

    pthread_mutex_lock(&b_mutex);

    tmp = (chn_list *)malloc(sizeof(chn_list));
    memset(tmp, 0, sizeof(chn_list));
    tmp->next = NULL;
    strncpy(tmp->name, name, 20);
    strncpy(tmp->owner, owner, 9);
    strncpy(tmp->server, server, 63);
    tmp->users = 1;
    tmp->created = now;

    if (*fap == NULL) {
	*fap = tmp;
    } else {
	for (ta = *fap; ta; ta = ta->next) {
	    if (ta->next == NULL) {
		ta->next = (chn_list *)tmp;
		break;
	    }
	}
    }

    pthread_mutex_unlock(&b_mutex);

    chnchg = TRUE;
    return 0;
}



void del_channel(chn_list **fap, char *name)
{
    chn_list    **tmp, *tmpa;
	        
    Syslog('r', "del_channel %s", name);

    if (*fap == NULL)
	return;

    pthread_mutex_lock(&b_mutex);

    tmp = fap;
    while (*tmp) {
	if (strcmp((*tmp)->name, name) == 0) {
	    tmpa = *tmp;
	    *tmp=(*tmp)->next;
	    free(tmpa);
	    chnchg = TRUE;
	} else {
	    tmp = &((*tmp)->next);
	}
    }

    pthread_mutex_unlock(&b_mutex);
}



void add_server(srv_list **fdp, char *name, int hops, char *prod, char *vers, char *fullname, char *router)
{
    srv_list	*tmp, *ta;
    
    Syslog('r', "add_server %s %d %s %s %s", name, hops, prod, vers, fullname);
 
    for (ta = *fdp; ta; ta = ta->next) {
	if (strcmp(ta->server, name) == 0) {
	    Syslog('r', "duplicate, ignore");
	    return;
	}
    }

    pthread_mutex_lock(&b_mutex);
    
    tmp = (srv_list *)malloc(sizeof(srv_list));
    memset(tmp, 0, sizeof(tmp));
    tmp->next = NULL;
    strncpy(tmp->server, name, 63);
    strncpy(tmp->router, router, 63);
    strncpy(tmp->prod, prod, 20);
    strncpy(tmp->vers, vers, 20);
    strncpy(tmp->fullname, fullname, 35);
    tmp->connected = now;
    tmp->users = 0;
    tmp->hops = hops;

    if (*fdp == NULL) {
	*fdp = tmp;
    } else {
	for (ta = *fdp; ta; ta = ta->next) {
	    if (ta->next == NULL) {
		ta->next = (srv_list *)tmp;
		break;
	    }
	}
    }

    pthread_mutex_unlock(&b_mutex);
    srvchg = TRUE;
}



/*
 * Delete server.
 */
void del_server(srv_list **fap, char *name)
{
    srv_list	*ta, *tan;
    
    Syslog('r', "delserver %s", name);

    if (*fap == NULL)
	return;

    pthread_mutex_lock(&b_mutex);

    for (ta = *fap; ta; ta = ta->next) {
	while ((tan = ta->next) && (strcmp(tan->server, name) == 0)) {
	    ta->next = tan->next;
	    free(tan);
	    srvchg = TRUE;
	}
	ta->next = tan;
    }

    pthread_mutex_unlock(&b_mutex);
}



/*  
 * Delete router.
 */ 
void del_router(srv_list **fap, char *name)
{   
    srv_list	*ta, *tan;
    
    Syslog('r', "delrouter %s", name);

    if (*fap == NULL)
	return;

    pthread_mutex_lock(&b_mutex);

    for (ta = *fap; ta; ta = ta->next) {
	while ((tan = ta->next) && (strcmp(tan->router, name) == 0)) {
	    del_user(&users, tan->server, NULL);
	    ta->next = tan->next;
	    free(tan);
	    srvchg = TRUE;
	}
	ta->next = tan;
    }

    pthread_mutex_unlock(&b_mutex);
}



/*
 * Send a message to all servers
 */
void send_all(const char *format, ...)
{
    ncs_list	*tnsl;
    char	buf[512];
    va_list	va_ptr;

    va_start(va_ptr, format);
    vsprintf(buf, format, va_ptr);
    va_end(va_ptr);

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (tnsl->state == NCS_CONNECT) {
	    send_msg(tnsl, buf);
	}
    }
}



/*
 * Broadcast a message to all servers except the originating server
 */
void broadcast(char *origin, const char *format, ...)
{
    ncs_list    *tnsl;
    va_list     va_ptr;
    char	buf[512];

    va_start(va_ptr, format);
    vsprintf(buf, format, va_ptr);
    va_end(va_ptr);
    
    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if ((tnsl->state == NCS_CONNECT) && (strcmp(origin, tnsl->server))) {
	    send_msg(tnsl, buf);
	}
    }
}



/*
 * Send message to a server
 */
int send_msg(ncs_list *tnsl, const char *format, ...)
{
    char	buf[512];
    va_list     va_ptr;
	
    va_start(va_ptr, format);
    vsprintf(buf, format, va_ptr);
    va_end(va_ptr);

    Syslog('r', "> %s: %s", tnsl->server, printable(buf, 0));

    if (sendto(tnsl->socket, buf, strlen(buf), 0, (struct sockaddr *)&tnsl->servaddr_in, sizeof(struct sockaddr_in)) == -1) {
	Syslog('!', "$IBC: can't send message");
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
    int		    a1, a2, a3, a4;
    struct servent  *se;
    struct hostent  *he;

    sprintf(scfgfn, "%s/etc/ibcsrv.data", getenv("MBSE_ROOT"));
    
    /*
     * Check if configuration is changed, if so then apply the changes.
     */
    if (file_time(scfgfn) != scfg_time) {
	Syslog('r', "%s filetime changed, rereading");

	if (servers == NULL) {
	    /*
	     * First add this server name to the servers database.
	     */
	    add_server(&servers, CFG.myfqdn, 0, (char *)"mbsebbs", (char *)VERSION, CFG.bbs_name, (char *)"none");
	}

	if ((fp = fopen(scfgfn, "r"))) {
	    fread(&ibcsrvhdr, sizeof(ibcsrvhdr), 1, fp);

	    while (fread(&ibcsrv, ibcsrvhdr.recsize, 1, fp)) {
		if (ibcsrv.Active) {
		    inlist = FALSE;
		    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
			if (strcmp(tnsl->server, ibcsrv.server) == 0) {
			    inlist = TRUE;
			}
		    }
		    if (!inlist ) {
			fill_ncslist(&ncsl, ibcsrv.server, ibcsrv.myname, ibcsrv.passwd);
			srvchg = TRUE;
			Syslog('+', "IBC: new configured Internet BBS Chatserver: %s", ibcsrv.server);
		    }
		}
	    }

	    /*
	     * Now check for neighbours to delete
	     */
	    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
		fseek(fp, ibcsrvhdr.hdrsize, SEEK_SET);
		inlist = FALSE;

		while (fread(&ibcsrv, ibcsrvhdr.recsize, 1, fp)) {
		    if ((strcmp(tnsl->server, ibcsrv.server) == 0) && ibcsrv.Active) {
			inlist = TRUE;
		    }
		}
		if (!inlist) {
		    Syslog('+', "IBC: server %s removed from configuration", tnsl->server);
		    tnsl->remove = TRUE;
		    tnsl->action = now;
		    srvchg = TRUE;
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
    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (((int)tnsl->action - (int)now) <= 0) {
	    switch (tnsl->state) {
		case NCS_INIT:	    Syslog('r', "%s init", tnsl->server);
				    srvchg = TRUE;

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
					    Syslog('!', "IBC: no IP address for %s: %s", tnsl->server, errmsg);
					    tnsl->action = now + (time_t)120;
					    tnsl->state = NCS_FAIL;
					    srvchg = TRUE;
					    break;
					}
					
					tnsl->socket = socket(AF_INET, SOCK_DGRAM, 0);
					if (tnsl->socket == -1) {
					    Syslog('!', "$IBC: can't create socket for %s", tnsl->server);
					    tnsl->state = NCS_FAIL;
					    tnsl->action = now + (time_t)120;
					    srvchg = TRUE;
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
				    send_msg(tnsl, "PASS %s 0100 %s\r\n", tnsl->passwd, tnsl->compress ? "Z":"");
				    send_msg(tnsl, "SERVER %s 0 %ld mbsebbs %s %s\r\n",  tnsl->myname, tnsl->token, 
					    VERSION, CFG.bbs_name);
				    tnsl->action = now + (time_t)10;
				    tnsl->state = NCS_WAITPWD;
				    srvchg = TRUE;
				    break;
				    
		case NCS_WAITPWD:   /*
				     * This state can be left by before the timeout is reached
				     * by a reply from the remote if the connection is accepted.
				     */
				    Syslog('r', "%s waitpwd", tnsl->server);
				    tnsl->token = 0;
				    tnsl->state = NCS_CALL;
				    while (TRUE) {
					j = 1+(int) (1.0 * CFG.dialdelay * rand() / (RAND_MAX + 1.0));
					if ((j > (CFG.dialdelay / 10)) && (j > 9))
					    break;
				    }
				    Syslog('r', "next call in %d %d seconds", CFG.dialdelay, j);
				    tnsl->action = now + (time_t)j;
				    srvchg = TRUE;
				    break;

		case NCS_CONNECT:   /*
				     * In this state we check if the connection is still alive
				     */
				    if (((int)now - (int)tnsl->last) > 130) {
					/*
					 * Missed 3 PING replies
					 */
					Syslog('+', "IBC: server %s connection is dead", tnsl->server);
					tnsl->state = NCS_DEAD;
					tnsl->action = now + (time_t)120;    // 2 minutes delay before calling again.
					tnsl->gotpass = FALSE;
					tnsl->gotserver = FALSE;
					tnsl->token = 0;
					del_router(&servers, tnsl->server);
					broadcast(tnsl->server, "SQUIT %s Connection died\r\n", tnsl->server);
					srvchg = TRUE;
					system_shout("*** NETWORK SPLIT, lost connection with server %s", tnsl->server);
					break;
				    }
				    /*
				     * Ping at 60, 90 and 120 seconds
				     */
				    if (((int)now - (int)tnsl->last) > 120) {
					Syslog('r', "sending 3rd PING at 120 seconds");
					send_msg(tnsl, "PING\r\n");
				    } else if (((int)now - (int)tnsl->last) > 90) {
					Syslog('r', "sending 2nd PING at 90 seconds");
					send_msg(tnsl, "PING\r\n");
				    } else if (((int)now - (int)tnsl->last) > 60) {
					send_msg(tnsl, "PING\r\n");
				    }
				    tnsl->action = now + (time_t)10;
				    break;

		case NCS_HANGUP:    Syslog('r', "%s hangup => call", tnsl->server);
				    tnsl->action = now + (time_t)1;
				    tnsl->state = NCS_CALL;
				    srvchg = TRUE;
				    break;

		case NCS_DEAD:	    Syslog('r', "%s dead -> call", tnsl->server);
				    tnsl->action = now + (time_t)1;
				    tnsl->state = NCS_CALL;
				    srvchg = TRUE;
				    break;

		case NCS_FAIL:	    Syslog('r', "%s fail => init", tnsl->server);
				    tnsl->action = now + (time_t)1;
				    tnsl->state = NCS_INIT;
				    srvchg = TRUE;
				    break;
	    }
	}
    }

    dump_ncslist();
}



int command_pass(char *hostname, char *parameters)
{
    ncs_list	*tnsl;
    char	*passwd, *version, *lnk;

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }

    passwd = strtok(parameters, " \0");
    version = strtok(NULL, " \0");
    lnk = strtok(NULL, " \0");

    if (version == NULL) {
	send_msg(tnsl, "400 PASS: Not enough parameters\r\n");
	return 400;
    }

    if (strcmp(passwd, tnsl->passwd)) {
	Syslog('!', "IBC: got bad password %s from %s", passwd, hostname);
	return 0;
    }

    if (tnsl->state == NCS_CONNECT) {
	send_msg(tnsl, "401: PASS: Already registered\r\n");
	return 401;
    }

    tnsl->gotpass = TRUE;
    tnsl->version = atoi(version);
    if (lnk && strchr(lnk, 'Z'))
	tnsl->compress = TRUE;
    srvchg = TRUE;
    return 0;
}



int command_server(char *hostname, char *parameters)
{
    ncs_list	    *tnsl;
    srv_list	    *ta;
    usr_list	    *tmp;
    chn_list	    *tmpc;
    char	    *name, *hops, *id, *prod, *vers, *fullname;
    unsigned long   token;
    int		    ihops, found = FALSE;

    name = strtok(parameters, " \0");
    hops = strtok(NULL, " \0");
    id = strtok(NULL, " \0");
    prod = strtok(NULL, " \0");
    vers = strtok(NULL, " \0");
    fullname = strtok(NULL, "\0");
    ihops = atoi(hops) + 1;

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, name) == 0) {
	    found = TRUE;
	    break;
	}
    }

    if (fullname == NULL) {
	send_msg(tnsl, "400 SERVER: Not enough parameters\r\n");
	return 400;
    }

    token = atoi(id);

    if (found && tnsl->token) {
	/*
	 * We are in calling state, so we expect the token from the
	 * remote is the same as the token we sent.
	 * In that case, the session is authorized.
	 */
	if (tnsl->token == token) {
	    broadcast(tnsl->server, "SERVER %s %d %s %s %s %s\r\n", name, ihops, id, prod, vers, fullname);
	    system_shout("* New server: %s, %s", name, fullname);
	    tnsl->gotserver = TRUE;
	    srvchg = TRUE;
	    tnsl->state = NCS_CONNECT;
	    tnsl->action = now + (time_t)10;
	    Syslog('+', "IBC: connected with neighbour server: %s", tnsl->server);
	    /*
	     * Send all already known servers
	     */
	    for (ta = servers; ta; ta = ta->next) {
		if (ta->hops) {
		    send_msg(tnsl, "SERVER %s %d 0 %s %s %s\r\n", ta->server, ta->hops, ta->prod, ta->vers, ta->fullname);
		}
	    }
	    /*
	     * Send all known users
	     */
	    for (tmp = users; tmp; tmp = tmp->next) {
		send_msg(tnsl, "USER %s@%s %s\r\n", tmp->name, tmp->server, tmp->realname);
		if (strcmp(tmp->name, tmp->nick))
		    send_msg(tnsl, "NICK %s %s %s %s\r\n", tmp->nick, tmp->name, tmp->server, tmp->realname);
		if (strlen(tmp->channel)) {
		    for (tmpc = channels; tmpc; tmpc = tmpc->next) {
			if (strcasecmp(tmpc->name, tmp->channel) == 0) {
			    send_msg(tnsl, "JOIN %s@%s %s\r\n", tmpc->owner, tmpc->server, tmpc->name);
			    if (strlen(tmpc->topic) && (strcmp(tmpc->server, CFG.myfqdn) == 0)) {
				send_msg(tnsl, "TOPIC %s %s\r\n", tmpc->name, tmpc->topic);
			    }
			}
		    }
		}
	    }
	    add_server(&servers, tnsl->server, ihops, prod, vers, fullname, hostname);
	    return 0;
	}
	Syslog('r', "IBC: call collision with %s", tnsl->server);
	tnsl->state = NCS_WAITPWD; /* Experimental, should fix state when state was connect while it wasn't. */
	return 0;
    }

    /*
     * We are in waiting state, so we sent our PASS and SERVER
     * messages and set the session to connected if we got a
     * valid PASS command.
     */
    if (found && tnsl->gotpass) {
	send_msg(tnsl, "PASS %s 0100 %s\r\n", tnsl->passwd, tnsl->compress ? "Z":"");
	send_msg(tnsl, "SERVER %s 0 %ld mbsebbs %s %s\r\n",  tnsl->myname, token, VERSION, CFG.bbs_name);
	broadcast(tnsl->server, "SERVER %s %d %s %s %s %s\r\n", name, ihops, id, prod, vers, fullname);
	system_shout("* New server: %s, %s", name, fullname);
	tnsl->gotserver = TRUE;
	tnsl->state = NCS_CONNECT;
	tnsl->action = now + (time_t)10;
	Syslog('+', "IBC: connected with neighbour server: %s", tnsl->server);
	/*
	 * Send all already known servers
	 */
	for (ta = servers; ta; ta = ta->next) {
	    if (ta->hops) {
		send_msg(tnsl, "SERVER %s %d 0 %s %s %s\r\n", ta->server, ta->hops, ta->prod, ta->vers, ta->fullname);
	    }
	}
	/*
	 * Send all known users. If a user is in a channel, send a JOIN.
	 * If the user is one of our own and has set a channel topic, send it.
	 */
	for (tmp = users; tmp; tmp = tmp->next) {
	    send_msg(tnsl, "USER %s@%s %s\r\n", tmp->name, tmp->server, tmp->realname);
	    if (strcmp(tmp->name, tmp->nick))
		send_msg(tnsl, "NICK %s %s %s %s\r\n", tmp->nick, tmp->name, tmp->server, tmp->realname);
	    if (strlen(tmp->channel)) {
		for (tmpc = channels; tmpc; tmpc = tmpc->next) {
		    if (strcasecmp(tmpc->name, tmp->channel) == 0) {
			send_msg(tnsl, "JOIN %s@%s %s\r\n", tmpc->owner, tmpc->server, tmpc->name);
			if (strlen(tmpc->topic) && (strcmp(tmpc->server, CFG.myfqdn) == 0)) {
			    send_msg(tnsl, "TOPIC %s %s\r\n", tmpc->name, tmpc->topic);
			}
		    }
		}
	    }
	}
	add_server(&servers, tnsl->server, ihops, prod, vers, fullname, hostname);
	srvchg = TRUE;
	return 0;
    }

    if (! found) {
       /*
	* Got a message about a server that is not our neighbour.
	*/
	add_server(&servers, name, ihops, prod, vers, fullname, hostname);
	broadcast(hostname, "SERVER %s %d %s %s %s %s\r\n", name, ihops, id, prod, vers, fullname);
	srvchg = TRUE;
	Syslog('+', "IBC: new relay server %s: %s", name, fullname);
	system_shout("* New server: %s, %s", name, fullname);
	return 0;
    }

    Syslog('r', "IBC: got SERVER command without PASS command from %s", hostname);
    return 0;
}



int command_squit(char *hostname, char *parameters)
{
    ncs_list    *tnsl;
    char        *name, *message;
    
    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }

    name = strtok(parameters, " \0");
    message = strtok(NULL, "\0");

    if (strcmp(name, tnsl->server) == 0) {
	Syslog('+', "IBC: disconnect neighbour server %s: %s", name, message);
	tnsl->state = NCS_HANGUP;
	tnsl->action = now + (time_t)120;	// 2 minutes delay before calling again.
	tnsl->gotpass = FALSE;
	tnsl->gotserver = FALSE;
	tnsl->token = 0;
	del_router(&servers, name);
    } else {
	Syslog('+', "IBC: disconnect relay server %s: %s", name, message);
	del_server(&servers, name);
    }

    system_shout("* Server %s disconnected: %s", name, message);
    broadcast(hostname, "SQUIT %s %s\r\n", name, message);
    srvchg = TRUE;
    return 0;
}



int command_user(char *hostname, char *parameters)
{
    ncs_list    *tnsl;
    char	*name, *server, *realname;

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }
	
    name = strtok(parameters, "@\0");
    server = strtok(NULL, " \0");
    realname = strtok(NULL, "\0");

    if (realname == NULL) {
	send_msg(tnsl, "400 USER: Not enough parameters\r\n");
	return 400;
    }
    
    if (add_user(&users, server, name, realname) == 0) {
	broadcast(hostname, "USER %s@%s %s\r\n", name, server, realname);
	system_shout("* New user %s@%s (%s)", name, server, realname);
    }
    return 0;
}



int command_quit(char *hostname, char *parameters)
{
    ncs_list    *tnsl;
    char	*name, *server, *message;

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }

    name = strtok(parameters, "@\0");
    server = strtok(NULL, " \0");
    message = strtok(NULL, "\0");

    if (server == NULL) {
	send_msg(tnsl, "400 QUIT: Not enough parameters\r\n");
	return 400;
    }

    if (message) {
	system_shout("* User %s is leaving: %s", name, message);
    } else {
	system_shout("* User %s is leaving", name);
    }
    del_user(&users, server, name);
    broadcast(hostname, "QUIT %s@%s %s\r\n", name, server, parameters);
    return 0;
}



int command_nick(char *hostname, char *parameters)
{
    ncs_list    *tnsl;
    usr_list	*tmp;
    char        *nick, *name, *server, *realname;
    int		found;

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }

    nick = strtok(parameters, " \0");
    name = strtok(NULL, " \0");
    server = strtok(NULL, " \0");
    realname = strtok(NULL, "\0");

    if (realname == NULL) {
	send_msg(tnsl, "400 NICK: Not enough parameters\r\n");
	return 1;
    }

    if (strlen(nick) > 9) {
	send_msg(tnsl, "402 %s: Erroneous nickname\r\n", nick);
	return 402;
    }
    
    // FIXME: check 1st char is alpha, rest alpha/digit

    found = FALSE;
    for (tmp = users; tmp; tmp = tmp->next) {
	if ((strcmp(tmp->name, nick) == 0) || (strcmp(tmp->nick, nick) == 0)) {
	    found = TRUE;
	    break;
	}
    }
    if (found) {
	send_msg(tnsl, "403 %s: Nickname is already in use\r\n", nick);
	return 403;
    }

    for (tmp = users; tmp; tmp = tmp->next) {
	if ((strcmp(tmp->server, server) == 0) && (strcmp(tmp->realname, realname) == 0) && (strcmp(tmp->name, name) == 0)) {
	    pthread_mutex_lock(&b_mutex);
	    strncpy(tmp->nick, nick, 9);
	    pthread_mutex_unlock(&b_mutex);
	    found = TRUE;
	    Syslog('+', "IBC: user %s set nick to %s", name, nick);
	    usrchg = TRUE;
	}
    }
    if (!found) {
	send_msg(tnsl, "404 %s@%s: Can't change nick\r\n", name, server);
	return 404;
    }

    broadcast(hostname, "NICK %s %s %s %s\r\n", nick, name, server, realname);
    return 0;
}



int command_join(char *hostname, char *parameters)
{
    ncs_list    *tnsl;
    chn_list    *tmp;
    usr_list	*tmpu;
    char        *nick, *server, *channel, msg[81];
    int         found;

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
        }
    }

    nick = strtok(parameters, "@\0");
    server = strtok(NULL, " \0");
    channel = strtok(NULL, "\0");

    if (channel == NULL) {
	send_msg(tnsl, "400 JOIN: Not enough parameters\r\n");
	return 400;
    }

    if (strlen(channel) > 20) {
	send_msg(tnsl, "402 %s: Erroneous channelname\r\n", nick);
	return 402;
    }

    if (strcasecmp(channel, "#sysop") == 0) {
	Syslog('+', "IBC: ignored JOIN for #sysop channel");
	return 0;
    }

    found = FALSE;
    for (tmp = channels; tmp; tmp = tmp->next) {
	if (strcmp(tmp->name, channel) == 0) {
	    found = TRUE;
	    tmp->users++;
	    break;
	}
    }
    if (!found) {
	Syslog('+', "IBC: create channel %s owned by %s@%s", channel, nick, server);
	add_channel(&channels, channel, nick, server);
	system_shout("* New channel %s created by %s@%s", channel, nick, server);
    }

    for (tmpu = users; tmpu; tmpu = tmpu->next) {
	if ((strcmp(tmpu->server, server) == 0) && ((strcmp(tmpu->nick, nick) == 0) || (strcmp(tmpu->name, nick) == 0))) {
	    pthread_mutex_lock(&b_mutex);
	    strncpy(tmpu->channel, channel, 20);
	    pthread_mutex_unlock(&b_mutex);
	    Syslog('+', "IBC: user %s joined channel %s", nick, channel);
	    usrchg = TRUE;
	    sprintf(msg, "* %s@%s has joined %s", nick, server, channel);
	    chat_msg(channel, NULL, msg);
	}
    }

    broadcast(hostname, "JOIN %s@%s %s\r\n", nick, server, channel);
    chnchg = TRUE;
    return 0;
}



int command_part(char *hostname, char *parameters)
{
    ncs_list    *tnsl;
    chn_list    *tmp;
    usr_list    *tmpu;
    char        *nick, *server, *channel, *message, msg[81];

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }

    nick = strtok(parameters, "@\0");
    server = strtok(NULL, " \0");
    channel = strtok(NULL, " \0");
    message = strtok(NULL, "\0");

    if (channel == NULL) {
	send_msg(tnsl, "400 PART: Not enough parameters\r\n");
	return 400;
    }

    if (strcasecmp(channel, "#sysop") == 0) {
	Syslog('+', "IBC: ignored PART from #sysop channel");
	return 0;
    }

    for (tmp = channels; tmp; tmp = tmp->next) {
	if (strcmp(tmp->name, channel) == 0) {
	    chnchg = TRUE;
	    tmp->users--;
	    if (tmp->users == 0) {
		Syslog('+', "IBC: deleted empty channel %s", channel);
		del_channel(&channels, channel);
	    }
	    break;
	}
    }

    for (tmpu = users; tmpu; tmpu = tmpu->next) {
	if ((strcmp(tmpu->server, server) == 0) && ((strcmp(tmpu->nick, nick) == 0) || (strcmp(tmpu->name, nick) == 0))) {
	    pthread_mutex_lock(&b_mutex);
	    tmpu->channel[0] = '\0';
	    pthread_mutex_unlock(&b_mutex);
	    if (message) {
		Syslog('+', "IBC: user %s left channel %s: %s", nick, channel, message);
		sprintf(msg, "* %s@%s has left: %s", nick, server, message);
	    } else {
		Syslog('+', "IBC: user %s left channel %s", nick, channel);
		sprintf(msg, "* %s@%s has silently left this channel", nick, server);
	    }
	    chat_msg(channel, NULL, msg);
	    usrchg = TRUE;
	}
    }

    if (message)
	broadcast(hostname, "PART %s@%s %s %s\r\n", nick, server, channel, message);
    else
	broadcast(hostname, "PART %s@%s %s\r\n", nick, server, channel);
    return 0;
}



int command_topic(char *hostname, char *parameters)
{
    ncs_list    *tnsl;
    chn_list    *tmp;
    char        *channel, *topic, msg[81];
		        
    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }

    channel = strtok(parameters, " \0");
    topic = strtok(NULL, "\0");

    if (topic == NULL) {
	send_msg(tnsl, "400 TOPIC: Not enough parameters\r\n");
	return 400;
    }

    for (tmp = channels; tmp; tmp = tmp->next) {
	if (strcmp(tmp->name, channel) == 0) {
	    chnchg = TRUE;
	    strncpy(tmp->topic, topic, 54);
	    Syslog('+', "IBC: channel %s topic: %s", channel, topic);
	    sprintf(msg, "* Channel topic is now: %s", tmp->topic);
	    chat_msg(channel, NULL, msg);
	    break;
	}
    }

    broadcast(hostname, "TOPIC %s %s\r\n", channel, topic);
    return 0;
}



int command_privmsg(char *hostname, char *parameters)
{
    ncs_list    *tnsl;
    chn_list    *tmp;
    char	*channel, *msg;

    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }

    channel = strtok(parameters, " \0");
    msg = strtok(NULL, "\0");

    if (msg == NULL) {
	send_msg(tnsl, "412 PRIVMSG: No text to send\r\n");
	return 412;
    }

    if (channel[0] != '#') {
	send_msg(tnsl, "499 PRIVMSG: Not for a channel\r\n"); // FIXME: also check users
	return 499;
    }

    for (tmp = channels; tmp; tmp = tmp->next) {
	if (strcmp(tmp->name, channel) == 0) {
	    tmp->lastmsg = now;
	    chat_msg(channel, NULL, msg);
	    broadcast(hostname, "PRIVMSG %s %s\r\n", channel, msg);
	    return 0;
	}
    }

    send_msg(tnsl, "409 %s: Cannot sent to channel\r\n", channel);
    return 409;
}



int do_command(char *hostname, char *command, char *parameters)
{
    ncs_list    *tnsl;
    
    for (tnsl = ncsl; tnsl; tnsl = tnsl->next) {
	if (strcmp(tnsl->server, hostname) == 0) {
	    break;
	}
    }

    /*
     * First the commands that don't have parameters
     */
    if (! strcmp(command, (char *)"PING")) {
	send_msg(tnsl, "PONG\r\n");
	return 0;
    }
    if (! strcmp(command, (char *)"PONG")) {
	/*
	 * Just accept
	 */
	return 0;
    }

    /*
     * Commands with parameters
     */
    if (parameters == NULL) {
	send_msg(tnsl, "400 %s: Not enough parameters\r\n", command);
	return 400;
    }

    if (! strcmp(command, (char *)"PASS")) {
	return command_pass(hostname, parameters);
    } 
    if (! strcmp(command, (char *)"SERVER")) {
	return command_server(hostname, parameters);
    } 
    if (! strcmp(command, (char *)"SQUIT")) {
	return command_squit(hostname, parameters);
    } 
    if (! strcmp(command, (char *)"USER")) {
	return command_user(hostname, parameters);
    } 
    if (! strcmp(command, (char *)"QUIT")) {
	return command_quit(hostname, parameters);
    } 
    if (! strcmp(command, (char *)"NICK")) {
	return command_nick(hostname, parameters);
    }
    if (! strcmp(command, (char *)"JOIN")) {
	return command_join(hostname, parameters);
    }
    if (! strcmp(command, (char *)"PART")) {
	return command_part(hostname, parameters);
    } 
    if (! strcmp(command, (char *)"TOPIC")) {
	return command_topic(hostname, parameters);
    }
    if (! strcmp(command, (char *)"PRIVMSG")) {
	return command_privmsg(hostname, parameters);
    }

    send_msg(tnsl, "413 %s: Unknown command\r\n", command);
    return 413;
}



void receiver(struct servent  *se)
{
    struct pollfd   pfd;
    struct hostent  *hp;
    int             rc, len, inlist;
    socklen_t       sl;
    ncs_list	    *tnsl;
    char            *hostname, *command, *parameters;

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
	        Syslog('!', "IBC: got message not terminated with CR-LF, dropped");
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

	    tnsl->last = now;
	    crbuf[strlen(crbuf) -2] = '\0';
	    Syslog('r', "< %s: \"%s\"", hostname, printable(crbuf, 0));

	    /*
	     * Parse message
	     */
	    command = strtok(crbuf, " \0");
	    parameters = strtok(NULL, "\0");

	    if (atoi(command)) {
		Syslog('r', "IBC: Got error %d", atoi(command));
	    } else {
		do_command(hostname, command, parameters);
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
    srand(getpid());

    while (! T_Shutdown) {

	now = time(NULL);
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
	    send_msg(tnsl, "SQUIT %s System shutdown\r\n", tnsl->myname);
	}
    }

    tidy_servers(&servers);

exit:
    ibc_run = FALSE;
    Syslog('+', "IBC thread stopped");
    pthread_exit(NULL);
}


#endif
