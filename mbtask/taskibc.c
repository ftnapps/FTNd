/*****************************************************************************
 *
 * $Id: taskibc.c,v 1.132 2007/02/11 20:29:53 mbse Exp $
 * Purpose ...............: mbtask - Internet BBS Chat (but it looks like...)
 *
 *****************************************************************************
 * Copyright (C) 1997-2006
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




extern int		    internet;		    /* Internet status		*/
time_t			    scfg_time = (time_t)0;  /* Servers config time	*/
time_t			    now;		    /* Current time		*/
int			    callchg = FALSE;	    /* Is call state changed	*/
int			    srvchg = FALSE;	    /* Is serverlist changed	*/
int			    usrchg = FALSE;	    /* Is userlist changed	*/
int			    chnchg = FALSE;	    /* Is channellist changed	*/
int			    banchg = FALSE;	    /* Is banned users changed	*/
int			    nickchg = FALSE;	    /* Is nicknames changed	*/
time_t			    resettime;		    /* Time to reset all	*/
int			    do_reset = FALSE;	    /* Reset init		*/
int			    link_reset = FALSE;	    /* Reset one link		*/
extern struct sockaddr_in   clientaddr_in;          /* IBC remote socket	*/


#define	PING_PONG_LOG	1

typedef enum {NCS_INIT, NCS_CALL, NCS_WAITPWD, NCS_CONNECT, NCS_HANGUP, NCS_FAIL, NCS_DEAD} NCSTYPE;


static char *ncsstate[] = {
    (char *)"init", (char *)"call", (char *)"waitpwd", (char *)"connect", 
    (char *)"hangup", (char *)"fail", (char *)"dead"
};


static char *mon[] = {
    (char *)"Jan",(char *)"Feb",(char *)"Mar",
    (char *)"Apr",(char *)"May",(char *)"Jun",
    (char *)"Jul",(char *)"Aug",(char *)"Sep",
    (char *)"Oct",(char *)"Nov",(char *)"Dec"
};



/*
 * Internal prototypes
 */
void fill_ncslist(char *, char *, char *, int, unsigned int);
void dump_ncslist(void);
int  add_server(char *, int, char *, char *, char *, char *);
void del_server(char *);
void del_router(char *);
int  send_msg(int, char *);
void broadcast(char *, char *);
void check_servers(void);
int  command_pass(int, char *, char *);
int  command_server(char *, char *);
int  command_squit(int, char *, char *);
int  command_user(int, char *, char *);
int  command_quit(int, char *, char *);
int  command_nick(int, char *, char *);
int  command_join(int, char *, char *);
int  command_part(int, char *, char *);
int  command_topic(int, char *, char *);
int  command_privmsg(int, char *, char *);



/*
 * Add a server to the neighbour serverlist
 */
void fill_ncslist(char *server, char *myname, char *passwd, int dyndns, unsigned int crc)
{
    int	    i;

    for (i = 0; i < MAXIBC_NCS; i++) {
	if (strlen(ncs_list[i].server) == 0) {
	    strncpy(ncs_list[i].server, server, 63);
	    strncpy(ncs_list[i].resolved, server, 63);
	    strncpy(ncs_list[i].myname, myname, 63);
	    strncpy(ncs_list[i].passwd, passwd, 15);
	    ncs_list[i].state = NCS_INIT;
	    ncs_list[i].action = now;
	    ncs_list[i].last = (time_t)0;
	    ncs_list[i].version = 0;
	    ncs_list[i].remove = FALSE;
	    ncs_list[i].socket = -1;
	    ncs_list[i].token = 0;
	    ncs_list[i].gotpass = FALSE;
	    ncs_list[i].gotserver = FALSE;
	    ncs_list[i].dyndns = dyndns;
	    ncs_list[i].halfdead = 0;
	    ncs_list[i].crc = crc;
	    return;
	}
    }

    WriteError("IBC: ncs_list is full");
}



void dump_ncslist(void)
{   
    char	temp1[128], temp2[128], buf[21];
    struct tm	ptm;
    time_t	tnow;
    int		i, first;

    if (!callchg && !srvchg && !usrchg && !chnchg && !banchg && !nickchg)
	return;

    if (callchg) {
	first = FALSE;
	for (i = 0; i < MAXIBC_NCS; i++) {
	    if (strlen(ncs_list[i].server)) {
		if (! first) {
		    Syslog('r', "IBC: Idx Server                         State   Del Pwd Srv Dyn 1/2 Next action");
		    Syslog('r', "IBC: --- ------------------------------ ------- --- --- --- --- --- -----------");
		    first = TRUE;
		}
		snprintf(temp1, 30, "%s", ncs_list[i].server);
		Syslog('r', "IBC: %3d %-30s %-7s %s %s %s %s %3d %d", i, temp1, ncsstate[ncs_list[i].state], 
		    ncs_list[i].remove ? "yes":"no ", ncs_list[i].gotpass ? "yes":"no ", 
		    ncs_list[i].gotserver ? "yes":"no ", ncs_list[i].dyndns ? "yes":"no ",
		    ncs_list[i].halfdead, (int)ncs_list[i].action - (int)now);
	    }
	} 
	if (! first) {
	    Syslog('r', "IBC: No servers configured");
	}
    }

    if (srvchg) {
	first = FALSE;
	for (i = 0; i < MAXIBC_SRV; i++) {
	    if (strlen(srv_list[i].server)) {
		if (! first) {
		    Syslog('+', "IBC: Idx Server                    Router                    Hops User Connect time");
		    Syslog('+', "IBC: --- ------------------------- ------------------------- ---- ---- --------------------");
		    first = TRUE;
		}
		snprintf(temp1, 25, "%s", srv_list[i].server);
		snprintf(temp2, 25, "%s", srv_list[i].router);
		tnow = (time_t)srv_list[i].connected;
		localtime_r(&tnow, &ptm);
		snprintf(buf, 21, "%02d-%s-%04d %02d:%02d:%02d", ptm.tm_mday, mon[ptm.tm_mon], ptm.tm_year+1900,
			ptm.tm_hour, ptm.tm_min, ptm.tm_sec);
		Syslog('+', "IBC: %3d %-25s %-25s %4d %4d %s", i, temp1, temp2, srv_list[i].hops, srv_list[i].users, buf);
	    }
	} 
	if (! first) {
	    Syslog('+', "IBC: Servers list is empty");
	}
    }
    
    if (usrchg) {
	first = FALSE;
	for (i = 0; i < MAXIBC_USR; i++) {
	    if (strlen(usr_list[i].server)) {
		if (! first) {
		    Syslog('+', "IBC: Idx Server               User             Name/Nick Channel       Sys Connect time");
		    Syslog('+', "IBC: --- -------------------- ---------------- --------- ------------- --- --------------------");
		    first = TRUE;
		}
		snprintf(temp1, 20, "%s", usr_list[i].server);
		snprintf(temp2, 16, "%s", usr_list[i].realname);
		tnow = (time_t)usr_list[i].connected;
		localtime_r(&tnow, &ptm);
		snprintf(buf, 21, "%02d-%s-%04d %02d:%02d:%02d", ptm.tm_mday, mon[ptm.tm_mon], ptm.tm_year+1900,
			ptm.tm_hour, ptm.tm_min, ptm.tm_sec);
		Syslog('+', "IBC: %3d %-20s %-16s %-9s %-13s %s %s", i, temp1, temp2, usr_list[i].nick, usr_list[i].channel,
		    usr_list[i].sysop ? "yes":"no ", buf);
	    }
	} 
	if (! first) {
	    Syslog('+', "IBC: Users list is empty");
	}
    }

    if (chnchg) {
	first = FALSE;
	for (i = 0; i < MAXIBC_CHN; i++) {
	    if (strlen(chn_list[i].server)) {
		if (! first) {
		    Syslog('+', "IBC: Idx Channel              Owner     Topic                           Usr Created");
		    Syslog('+', "IBC: --- -------------------- --------- ------------------------------- --- --------------------");
		    first = TRUE;
		}
		tnow = (time_t)chn_list[i].created;
		localtime_r(&tnow, &ptm);
		snprintf(buf, 21, "%02d-%s-%04d %02d:%02d:%02d", ptm.tm_mday, mon[ptm.tm_mon], ptm.tm_year+1900,
			ptm.tm_hour, ptm.tm_min, ptm.tm_sec);
		Syslog('+', "IBC: %3d %-20s %-9s %-31s %3d %s", i, chn_list[i].name, 
			chn_list[i].owner, chn_list[i].topic, chn_list[i].users, buf);
	    }
	} 
	if (! first) {
	    Syslog('+', "IBC: Channels list is empty");
	}
    }

    callchg = srvchg = usrchg = chnchg = banchg = nickchg = FALSE;
}



/*
 * Add one user to the userlist. Returns:
 *  0 = Ok
 *  1 = User already registered.
 *  2 = Too many users.
 */
int add_user(char *server, char *name, char *realname)
{
    int	    i, j, Found = FALSE;

    for (j = 0; j < MAXIBC_USR; j++) {
	if ((strcmp(usr_list[j].server, server) == 0) && (strcmp(usr_list[j].realname, realname) == 0)) {
	    Syslog('!', "IBC: add_user(%s, %s, %s), already registered", server, name, realname);
	    return 1;
	}
    }

    for (i = 0; i < MAXIBC_SRV; i++) {
	if (strcmp(srv_list[i].server, server) == 0) {
	    Found = TRUE;
	    break;
	}
    }
    if (!Found) {
	Syslog('!', "IBC: add_user(%s, %s, %s), unknown server", server, name, realname);
	return 1;
    }

    for (j = 0; j < MAXIBC_USR; j++) {
	if (strlen(usr_list[j].server) == 0) {
	    strncpy(usr_list[j].server, server, 63);
	    strncpy(usr_list[j].name, name, 9);
	    strncpy(usr_list[j].nick, name, 9);
	    strncpy(usr_list[j].realname, realname, 36);
	    usr_list[j].connected = now;
	    srv_list[i].users++;
	    srvchg = usrchg = TRUE;
	    Syslog('r', "IBC: add_user (%s, %s, %s) slot=%d", server, name, realname, i);
	    return 0;
	}
    }

    WriteError("IBC: add_user(%s, %s, %s), user list is full", server, name, realname);
    return 2;
}


/*
 * Delete user from channel
 */
void del_userchannel(char *channel)
{
    int	    i;

    for (i = 0; i < MAXIBC_CHN; i++) {
	if (strcmp(chn_list[i].name, channel) == 0 && chn_list[i].users) {
	    chnchg = TRUE;
	    chn_list[i].users--;
	    Syslog('r', "IBC: del_userchannel(%s), %d users left", channel, chn_list[i].users);
	    if (chn_list[i].users == 0) {
		Syslog('+', "IBC: deleted empty channel %s", channel);
		del_channel(channel);
	    }
	    break;
	}
    }
}



/*
 * Delete one user. If name == NULL then delete all users of a server.
 */
void del_user(char *server, char *name)
{
    int	    i, count = 0;

    for (i = 0; i < MAXIBC_USR; i++) {
	if (name && (strcmp(usr_list[i].server, server) == 0) && (strcmp(usr_list[i].name, name) == 0)) {
	    if (strlen(usr_list[i].channel))
		del_userchannel(usr_list[i].channel);
	    Syslog('r', "IBC: del_user(%s, %s) from slot %d", server, printable(name, 0), i);
	    memset(&usr_list[i], 0, sizeof(_usr_list));
	    usrchg = TRUE;
	    count++;
	} else if ((name == NULL) && (strcmp(usr_list[i].server, server) == 0)) {
	    if (strlen(usr_list[i].channel))
		del_userchannel(usr_list[i].channel);
	    Syslog('r', "IBC: del_user(%s, %s) user %s from slot %d", server, printable(name, 0), usr_list[i].name, i);
	    memset(&usr_list[i], 0, sizeof(_usr_list));
	    usrchg = TRUE;
	    count++;
	}
    }

    /*
     * Update users list of the server
     */
    for (i = 0; i < MAXIBC_SRV; i++) {
	if ((strcmp(srv_list[i].server, server) == 0) && srv_list[i].users) {
	    srv_list[i].users -= count;
	    if (srv_list[i].users < 0)
		srv_list[i].users = 0;	/* Just in case, nothing is perfect */
	    srvchg = TRUE;
	}
    }
}



/*
 * Add channel with owner, returns:
 *   0 = Success
 *   1 = Already registered
 *   2 = Too many channels
 */
int add_channel(char *name, char *owner, char *server)
{
    int	    i;

    for (i = 0; i < MAXIBC_CHN; i++) {
	if ((strcmp(chn_list[i].name, name) == 0) && (strcmp(chn_list[i].owner, owner) == 0) && 
		(strcmp(chn_list[i].server, server) == 0)) {
	    Syslog('!', "IBC: add_channel(%s, %s, %s), already registered", name, owner, server);
	    return 1;
	}
    }

    for (i = 0; i < MAXIBC_CHN; i++) {
	if (strlen(chn_list[i].server) == 0) {
	    strncpy(chn_list[i].name, name, 20);
	    strncpy(chn_list[i].owner, owner, 9);
	    strncpy(chn_list[i].server, server, 63);
	    chn_list[i].users = 1;
	    chn_list[i].created = now;
	    chnchg = TRUE;
	    Syslog('r', "IBC: add_channel (%s, %s, %s) slot=%d", name, owner, server, i);
	    return 0;
	}
    }

    WriteError("IBC: add_channel(%s, %s, %s), too many channels", name, owner, server);
    return 2;
}



void del_channel(char *name)
{
    int	    i;
	        
    for (i = 0; i < MAXIBC_CHN; i++) {
	if (strlen(chn_list[i].server) && (strcmp(chn_list[i].name, name) == 0)) {
	    Syslog('r', "IBC: del_channel(%s), slot=%d", name, i);
	    memset(&chn_list[i], 0, sizeof(_chn_list));
	    return;
	}
    }
}



/*
 * Add a server to the list, returns
 *  0 = error
 *  1 = success
 */
int add_server(char *name, int hops, char *prod, char *vers, char *fullname, char *router)
{
    int	    i, haverouter = FALSE;

    for (i = 0; i < MAXIBC_SRV; i++) {
	if (strlen(srv_list[i].server) && (strcmp(srv_list[i].server, name) == 0)) {
	    Syslog('r', "IBC: add_server %s %d %s %s \"%s\" %s, duplicate, ignore", name, hops, prod, vers, fullname, router);
	    return 0;
	}
    }

    /*
     * If name <> router it's a remote server, then check if we have a router in our list.
     */
    if (strcmp(name, router) && hops) {
	for (i = 0; i < MAXIBC_SRV; i++) {
	    if (strcmp(srv_list[i].server, router) == 0) {
		haverouter = TRUE;
		break;
	    }
	}
	if (! haverouter) {
	    Syslog('r', "IBC: add_server %s %d %s %s \"%s\" %s, no router, ignore", name, hops, prod, vers, fullname, router);
	    return 0;
	}
    }

    for (i = 0; i < MAXIBC_SRV; i++) {
	if (strlen(srv_list[i].server) == 0) {
	    strncpy(srv_list[i].server, name, 63);
	    strncpy(srv_list[i].router, router, 63);
	    strncpy(srv_list[i].prod, prod, 20);
	    strncpy(srv_list[i].vers, vers, 20);
	    strncpy(srv_list[i].fullname, fullname, 35);
	    srv_list[i].connected = now;
	    srv_list[i].users = 0;
	    srv_list[i].hops = hops;
	    srvchg = TRUE;
	    Syslog('r', "IBC: add_server %s %d %s %s \"%s\" %s, slot=%d", name, hops, prod, vers, fullname, router, i);
	    return 1;
	}
    }

    WriteError("IBC: add_server %s %d %s %s \"%s\" %s, serverlist full", name, hops, prod, vers, fullname, router);
    return 0;
}



/*
 * Delete server.
 */
void del_server(char *name)
{
    int	    i;

    for (i = 0; i < MAXIBC_SRV; i++) {
	if (strcmp(srv_list[i].server, name) == 0) {
	    Syslog('r', "IBC: del_server(%s), slot %d", name, i);
	    del_user(srv_list[i].server, NULL);
	    memset(&srv_list[i], 0, sizeof(_srv_list));
	    srvchg = TRUE;
	}
    }
}



/*  
 * Delete router.
 */ 
void del_router(char *name)
{   
    int	    i;
    
    for (i = 0; i < MAXIBC_SRV; i++) {
	if (strcmp(srv_list[i].router, name) == 0) {
	    Syslog('r', "IBC: del_router(%s) slot %d", name, i);
	    del_user(srv_list[i].server, NULL);
	    memset(&srv_list[i], 0, sizeof(_srv_list));
	    srvchg = TRUE;
	}
    }
}



/*
 * Send a message to all servers
 */
void send_all(char *msg)
{
    int	    i;

    for (i = 0; i < MAXIBC_NCS; i++) {
	if (strlen(ncs_list[i].server) && (ncs_list[i].state == NCS_CONNECT)) {
	    send_msg(i, msg);
	}
    }
}



/*
 * Send command message to each connected neighbour server and use the correct own fqdn.
 */
void send_at(char *cmd, char *nick, char *param)
{   
    int	    i;
    char    *p;

    for (i = 0; i < MAXIBC_NCS; i++) {
	if (strlen(ncs_list[i].server) && (ncs_list[i].state == NCS_CONNECT)) {
	    p = xstrcpy(cmd);
	    p = xstrcat(p, (char *)" ");
	    p = xstrcat(p, nick);
	    p = xstrcat(p, (char *)"@");
	    p = xstrcat(p, ncs_list[i].myname);
	    p = xstrcat(p, (char *)" ");
	    p = xstrcat(p, param);
	    p = xstrcat(p, (char *)"\r\n");
	    send_msg(i, p);
	    free(p);
	}
    }
}



void send_nick(char *nick, char *name, char *realname)
{
    int	    i;
    char    *p;

    for (i = 0; i < MAXIBC_NCS; i++) {
	if (strlen(ncs_list[i].server) && (ncs_list[i].state == NCS_CONNECT)) {
	    p = xstrcpy((char *)"NICK ");
	    p = xstrcat(p, nick);
	    p = xstrcat(p, (char *)" ");
	    p = xstrcat(p, name);
	    p = xstrcat(p, (char *)" ");
	    p = xstrcat(p, ncs_list[i].myname);
	    p = xstrcat(p, (char *)" ");
	    p = xstrcat(p, realname);
	    p = xstrcat(p, (char *)"\r\n");
	    send_msg(i, p);
	    free(p);
	}
    }
}



/*
 * Broadcast a message to all servers except the originating server
 */
void broadcast(char *origin, char *msg)
{
    int	    i;

    for (i = 0; i < MAXIBC_NCS; i++) {
	if (strlen(ncs_list[i].server) && (ncs_list[i].state == NCS_CONNECT) && (strcmp(origin, ncs_list[i].server))) {
	    send_msg(i, msg);
	}
    }
}



/*
 * Send message to a server
 */
int send_msg(int slot, char *msg)
{
    char	*p;
	
#ifndef	PING_PONG_LOG
    if (strcmp(msg, "PING\r\n") && strcmp(msg, "PONG\r\n")) {
#endif
	p = xstrcpy((char *)"IBC: > ");
	p = xstrcat(p, ncs_list[slot].server);
	p = xstrcat(p, (char *)": ");
	p = xstrcat(p, printable(msg, 0));
	Syslogp('r', p);
	free(p);
#ifndef PING_PONG_LOG
    }
#endif

    if (sendto(ncs_list[slot].socket, msg, strlen(msg), 0, 
		(struct sockaddr *)&ncs_list[slot].servaddr_in, sizeof(struct sockaddr_in)) == -1) {
	WriteError("$IBC: can't send message");
	return -1;
    }
    return 0;
}



void check_servers(void)
{
    char	    *errmsg, *p, scfgfn[PATH_MAX];
    FILE	    *fp;
    int		    i, j, inlist, Remove, local_reset, conf_changed;
    int		    a1, a2, a3, a4, users = 0, channels = 0;
    unsigned int    crc;
    struct servent  *se;
    struct hostent  *he;

    now = time(NULL);
    snprintf(scfgfn, PATH_MAX, "%s/etc/ibcsrv.data", getenv("MBSE_ROOT"));
    
    /*
     * Check if we reached the global reset time
     */
    if (((int)now > (int)resettime) && !do_reset) {
	resettime = time(NULL) + (time_t)86400;
	do_reset = TRUE;
	Syslog('+', "IBC: global reset time reached, preparing reset");
    }

    local_reset = conf_changed = FALSE;
    for (i = 0; i < MAXIBC_USR; i++)
	if (strlen(usr_list[i].server))
	    users++;
    for (i = 0; i < MAXIBC_CHN; i++)
	if (strlen(chn_list[i].server))
	    channels++;
    if (!users && !channels && do_reset) {
	Syslog('+', "IBC: no channels and users, performing reset");
	local_reset = TRUE;
	do_reset = FALSE;
    }

    if (file_time(scfgfn) != scfg_time) {
	conf_changed = TRUE;
	Syslog('+', "IBC: %s filetime changed, rereading configuration", scfgfn);
    }

    /*
     * Check if configuration is changed, if so then apply the changes.
     */
    if (conf_changed || local_reset || link_reset) {

	if (strlen(srv_list[0].server) == 0) {
	    /*
	     * First add this server name to the servers database.
	     */
	    add_server(CFG.myfqdn, 0, (char *)"mbsebbs", (char *)VERSION, CFG.bbs_name, (char *)"none");
	}

	/*
	 * Local reset, make all crc's invalid so the connections will restart.
	 */
	if (local_reset) {
	    for (i = 0; i < MAXIBC_NCS; i++)
		if (strlen(ncs_list[i].server))
		    ncs_list[i].crc--;
	}
	
	if (link_reset) {
	    Syslog('r', "IBC: link_reset starting");
	    link_reset = FALSE;
	}

	if ((fp = fopen(scfgfn, "r"))) {
	    fread(&ibcsrvhdr, sizeof(ibcsrvhdr), 1, fp);

            /*
	     * Check for neighbour servers to delete
	     */
	    for (i = 0; i < MAXIBC_NCS; i++) {
		if (strlen(ncs_list[i].server)) {
		    fseek(fp, ibcsrvhdr.hdrsize, SEEK_SET);
		    inlist = FALSE;
		    while (fread(&ibcsrv, ibcsrvhdr.recsize, 1, fp)) {
			crc = 0xffffffff;
			crc = upd_crc32((char *)&ibcsrv, crc, sizeof(ibcsrv));
			if ((strcmp(ncs_list[i].server, ibcsrv.server) == 0) && ibcsrv.Active && (ncs_list[i].crc == crc)) {
			    inlist = TRUE;
			}
		    }
		    if (!inlist) {
			if (local_reset || link_reset)
			    Syslog('+', "IBC: server %s connection reset", ncs_list[i].server);
			else
			    Syslog('+', "IBC: server %s configuration changed or removed", ncs_list[i].server);
			ncs_list[i].remove = TRUE;
			ncs_list[i].action = now;
			srvchg = TRUE;
			callchg = TRUE;
		    }
		}
	    }

	    /*
	     * Start removing servers
	     */
	    Remove = FALSE;
	    for (i = 0; i < MAXIBC_NCS; i++) {
		if (ncs_list[i].remove) {
		    Remove = TRUE;
		    Syslog('r', "IBC: Remove server %s", ncs_list[i].server);
		    if (ncs_list[i].state == NCS_CONNECT) {
			p = calloc(512, sizeof(char));
			if (local_reset) {
			    snprintf(p, 512, "SQUIT %s Reset connection\r\n", ncs_list[i].server);
			    broadcast(ncs_list[i].server, p);
			    snprintf(p, 512, "SQUIT %s Your system connection is reset\r\n", ncs_list[i].myname);
			    send_msg(i, p);
			} else {
			    snprintf(p, 512, "SQUIT %s Removed from configuration\r\n", ncs_list[i].server);
			    broadcast(ncs_list[i].server, p);
			    snprintf(p, 512, "SQUIT %s Your system is removed from configuration\r\n", ncs_list[i].myname);
			    send_msg(i, p);
			}
			free(p);
			del_router(ncs_list[i].server);
		    }
		    if (ncs_list[i].socket != -1) {
			Syslog('r', "IBC: Closing socket %d", ncs_list[i].socket);
			shutdown(ncs_list[i].socket, SHUT_WR);
			ncs_list[i].socket = -1;
			ncs_list[i].state = NCS_HANGUP;
		    }
		    callchg = TRUE;
		    srvchg = TRUE;
		}
	    }
	    dump_ncslist();

	    if (link_reset) {
		link_reset = FALSE;
	    }

	    /*
	     * If a neighbour is removed by configuration, remove it from the list.
	     */
	    if (Remove) {
		Syslog('r', "IBC: Starting remove list");
		for (i = 0; i < MAXIBC_NCS; i++) {
		    if (ncs_list[i].remove) {
		        Syslog('r', " do %s", ncs_list[i].server);
			memset(&ncs_list[i], 0, sizeof(_ncs_list));
		        callchg = TRUE;
		    }
		}
	    }
	    dump_ncslist();
	    
	    /*
	     * Changed or deleted servers are now removed, add new
	     * configured servers or changed servers.
	     */
	    fseek(fp, ibcsrvhdr.hdrsize, SEEK_SET);
	    while (fread(&ibcsrv, ibcsrvhdr.recsize, 1, fp)) {

		/*
		 * Check for new configured servers
		 */
		if (ibcsrv.Active && strlen(ibcsrv.myname) && strlen(ibcsrv.server) && strlen(ibcsrv.passwd)) {
		    inlist = FALSE;
		    for (i = 0; i < MAXIBC_NCS; i++) {
			if (strcmp(ncs_list[i].server, ibcsrv.server) == 0) {
			    inlist = TRUE;
			}
		    }
		    for (j = 0; j < MAXIBC_SRV; j++) {
			if ((strcmp(srv_list[j].server, ibcsrv.server) == 0) && (strcmp(srv_list[j].router, ibcsrv.server))) {
			    inlist = TRUE;
			    Syslog('+', "IBC: can't add new configured server %s: already connected via %s", 
				    ibcsrv.server, srv_list[j].router);
			}
		    }
		    if (!inlist ) {
			crc = 0xffffffff;
			crc = upd_crc32((char *)&ibcsrv, crc, sizeof(ibcsrv));
			fill_ncslist(ibcsrv.server, ibcsrv.myname, ibcsrv.passwd, ibcsrv.Dyndns, crc);
			srvchg = TRUE;
			callchg = TRUE;
			Syslog('+', "IBC: new configured Internet BBS Chatserver: %s", ibcsrv.server);
		    }
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
    for (i = 0; i < MAXIBC_NCS; i++) {

	if (strlen(ncs_list[i].server) == 0)
	    continue;

	if (((int)ncs_list[i].action - (int)now) <= 0) {
	    switch (ncs_list[i].state) {
		case NCS_INIT:	    if (internet) {
					/*
					 * Internet is available, setup the connection.
					 * Get IP address for the hostname.
					 * Set default next action to 60 seconds.
					 */
					ncs_list[i].action = now + (time_t)60;
// Gives SIGBUS on Sparc		memset(&ncs_list[i].servaddr_in, 0, sizeof(struct sockaddr_in));
					se = getservbyname("fido", "udp");
					ncs_list[i].servaddr_in.sin_family = AF_INET;
					ncs_list[i].servaddr_in.sin_port = se->s_port;
					
					if (sscanf(ncs_list[i].server,"%d.%d.%d.%d",&a1,&a2,&a3,&a4) == 4)
					    ncs_list[i].servaddr_in.sin_addr.s_addr = inet_addr(ncs_list[i].server);
					else if ((he = gethostbyname(ncs_list[i].server)))
					    memcpy(&ncs_list[i].servaddr_in.sin_addr, he->h_addr, he->h_length);
					else {
					    switch (h_errno) {
						case HOST_NOT_FOUND:    errmsg = (char *)"Authoritative: Host not found"; break;
						case TRY_AGAIN:         errmsg = (char *)"Non-Authoritive: Host not found"; break;
						case NO_RECOVERY:       errmsg = (char *)"Non recoverable errors"; break;
						default:                errmsg = (char *)"Unknown error"; break;
					    }
					    Syslog('!', "IBC: no IP address for %s: %s", ncs_list[i].server, errmsg);
					    ncs_list[i].action = now + (time_t)120;
					    ncs_list[i].state = NCS_FAIL;
					    callchg = TRUE;
					    break;
					}
					
					if (ncs_list[i].socket == -1) {
					    ncs_list[i].socket = socket(AF_INET, SOCK_DGRAM, 0);
					    if (ncs_list[i].socket == -1) {
						Syslog('!', "$IBC: can't create socket for %s", ncs_list[i].server);
						ncs_list[i].state = NCS_FAIL;
						ncs_list[i].action = now + (time_t)120;
						callchg = TRUE;
						break;
					    }
					    Syslog('r', "IBC: socket %d created for %s", ncs_list[i].socket, ncs_list[i].server);
					} else {
					    Syslog('r', "IBC: socket %d reused for %s", ncs_list[i].socket, ncs_list[i].server);
					}

					ncs_list[i].state = NCS_CALL;
					ncs_list[i].action = now + (time_t)1;
					callchg = TRUE;
				    } else {
					/*
					 * No internet, just wait
					 */
					ncs_list[i].action = now + (time_t)10;
				    }
				    break;
				    
		case NCS_CALL:	    /*
				     * In this state we accept PASS and SERVER commands from
				     * the remote with the same token as we have sent.
				     */
				    Syslog('r', "IBC: %s call", ncs_list[i].server);
				    if (strlen(ncs_list[i].passwd) == 0) {
					Syslog('!', "IBC: no password configured for %s", ncs_list[i].server);
					ncs_list[i].state = NCS_FAIL;
					ncs_list[i].action = now + (time_t)300;
					callchg = TRUE;
					break;
				    }
				    ncs_list[i].token = gettoken();
				    p = calloc(512, sizeof(char));
				    snprintf(p, 512, "PASS %s 0100 %s\r\n", ncs_list[i].passwd, ncs_list[i].compress ? "Z":"");
				    send_msg(i, p);
				    snprintf(p, 512, "SERVER %s 0 %d mbsebbs %s %s\r\n",  ncs_list[i].myname, ncs_list[i].token, 
					    VERSION, CFG.bbs_name);
				    send_msg(i, p);
				    free(p);
				    ncs_list[i].action = now + (time_t)10;
				    ncs_list[i].state = NCS_WAITPWD;
				    callchg = TRUE;
				    break;
				    
		case NCS_WAITPWD:   /*
				     * This state can be left by before the timeout is reached
				     * by a reply from the remote if the connection is accepted.
				     */
				    Syslog('r', "IBC: %s waitpwd", ncs_list[i].server);
				    ncs_list[i].token = 0;
				    ncs_list[i].state = NCS_CALL;
				    while (TRUE) {
					j = 1+(int) (1.0 * CFG.dialdelay * rand() / (RAND_MAX + 1.0));
					if ((j > (CFG.dialdelay / 10)) && (j > 9))
					    break;
				    }
				    Syslog('r', "IBC: next call in %d %d seconds", CFG.dialdelay, j);
				    ncs_list[i].action = now + (time_t)j;
				    callchg = TRUE;
				    break;

		case NCS_CONNECT:   /*
				     * In this state we check if the connection is still alive
				     */
				    j = (int)now - (int)ncs_list[i].last;
				    if (ncs_list[i].halfdead > 5) {
					/*
					 * Halfdead means 5 times received a PASS while we are in
					 * connected state. This means the other side "thinks" it's
					 * not connected and tries to connect. This can be caused by
					 * temporary internet problems. 
					 * Reset our side of the connection.
					 */
					Syslog('+', "IBC: server %s connection is half dead", ncs_list[i].server);
					ncs_list[i].state = NCS_DEAD;
					ncs_list[i].action = now + (time_t)60;    // 1 minute delay before calling again.
					ncs_list[i].gotpass = FALSE;
					ncs_list[i].gotserver = FALSE;
					ncs_list[i].token = 0;
					ncs_list[i].halfdead = 0;
					p = calloc(81, sizeof(char));
					snprintf(p, 81, "SQUIT %s Connection died\r\n", ncs_list[i].server);
					broadcast(ncs_list[i].server, p);
					free(p);
					callchg = TRUE;
					srvchg = TRUE;
					system_shout("*** NETWORK SPLIT, lost connection with server %s", ncs_list[i].server);
					del_router(ncs_list[i].server);
					break;
				    }
				    if (((int)now - (int)ncs_list[i].last) > 130) {
					/*
					 * Missed 3 PING replies
					 */
					Syslog('+', "IBC: server %s connection is dead", ncs_list[i].server);
					ncs_list[i].state = NCS_DEAD;
					ncs_list[i].action = now + (time_t)120;    // 2 minutes delay before calling again.
					ncs_list[i].gotpass = FALSE;
					ncs_list[i].gotserver = FALSE;
					ncs_list[i].token = 0;
					ncs_list[i].halfdead = 0;
					p = calloc(81, sizeof(char));
					snprintf(p, 81, "SQUIT %s Connection died\r\n", ncs_list[i].server);
					broadcast(ncs_list[i].server, p);
					free(p);
					callchg = TRUE;
					srvchg = TRUE;
					system_shout("*** NETWORK SPLIT, lost connection with server %s", ncs_list[i].server);
					del_router(ncs_list[i].server);
					break;
				    }
				    /*
				     * Ping at 60, 90 and 120 seconds
				     */
				    if (((int)now - (int)ncs_list[i].last) > 120) {
					Syslog('r', "IBC: sending 3rd PING at 120 seconds");
					send_msg(i, (char *)"PING\r\n");
				    } else if (((int)now - (int)ncs_list[i].last) > 90) {
					Syslog('r', "IBC: sending 2nd PING at 90 seconds");
					send_msg(i, (char *)"PING\r\n");
				    } else if (((int)now - (int)ncs_list[i].last) > 60) {
					send_msg(i, (char *)"PING\r\n");
				    }
				    ncs_list[i].action = now + (time_t)10;
				    break;

		case NCS_HANGUP:    Syslog('r', "IBC: %s hangup => call", ncs_list[i].server);
				    ncs_list[i].action = now + (time_t)1;
				    ncs_list[i].state = NCS_CALL;
				    callchg = TRUE;
				    srvchg = TRUE;
				    break;

		case NCS_DEAD:	    Syslog('r', "IBC: %s dead -> call", ncs_list[i].server);
				    ncs_list[i].action = now + (time_t)1;
				    ncs_list[i].state = NCS_CALL;
				    callchg = TRUE;
				    srvchg = TRUE;
				    break;

		case NCS_FAIL:	    Syslog('r', "IBC: %s fail => init", ncs_list[i].server);
				    ncs_list[i].action = now + (time_t)1;
				    ncs_list[i].state = NCS_INIT;
				    callchg = TRUE;
				    srvchg = TRUE;
				    break;
	    }
	}
    }

    dump_ncslist();
}



int command_pass(int slot, char *hostname, char *parameters)
{
    char    *passwd, *version, *lnk;

    ncs_list[slot].gotpass = FALSE;
    passwd = strtok(parameters, " \0");
    version = strtok(NULL, " \0");
    lnk = strtok(NULL, " \0");

    if (strcmp(passwd, "0100") == 0) {
	send_msg(slot, (char *)"414 PASS: Got empty password\r\n");
	return 414;
    }

    if (version == NULL) {
	send_msg(slot, (char *)"400 PASS: Not enough parameters\r\n");
	return 400;
    }

    if (strcmp(passwd, ncs_list[slot].passwd)) {
	Syslog('!', "IBC: got bad password %s from %s", passwd, hostname);
	return 0;
    }

    if (ncs_list[slot].state == NCS_CONNECT) {
	send_msg(slot, (char *)"401: PASS: Already registered\r\n");
	ncs_list[slot].halfdead++;   /* Count them   */
	srvchg = TRUE;
	return 401;
    }

    ncs_list[slot].gotpass = TRUE;
    ncs_list[slot].version = atoi(version);
    if (lnk && strchr(lnk, 'Z'))
	ncs_list[slot].compress = TRUE;
    return 0;
}



int command_server(char *hostname, char *parameters)
{
    char	    *p, *name, *hops, *prod, *vers, *fullname;
    unsigned int    token;
    int		    i, j, ihops, found = FALSE;

    name = strtok(parameters, " \0");
    hops = strtok(NULL, " \0");
    token = atoi(strtok(NULL, " \0"));
    prod = strtok(NULL, " \0");
    vers = strtok(NULL, " \0");
    fullname = strtok(NULL, "\0");
    ihops = atoi(hops) + 1;

    /*
     * Check if this is our neighbour server
     */
    for (i = 0; i < MAXIBC_NCS; i++) {
	if (strcmp(ncs_list[i].server, name) == 0) {
	    found = TRUE;
	    break;
	}
    }

    if (found && fullname == NULL) {
	send_msg(i, (char *)"400 SERVER: Not enough parameters\r\n");
	return 400;
    }

    if (found && ncs_list[i].token) {
	/*
	 * We are in calling state, so we expect the token from the
	 * remote is the same as the token we sent.
	 * In that case, the session is authorized.
	 */
	if (ncs_list[i].token == token) {
	    p = calloc(512, sizeof(char));
	    snprintf(p, 512, "SERVER %s %d %d %s %s %s\r\n", name, ihops, token, prod, vers, fullname);
	    broadcast(ncs_list[i].server, p);
	    free(p);
	    system_shout("* New server: %s, %s", name, fullname);
	    ncs_list[i].gotserver = TRUE;
	    callchg = TRUE;
	    srvchg = TRUE;
	    ncs_list[i].state = NCS_CONNECT;
	    ncs_list[i].action = now + (time_t)10;
	    Syslog('+', "IBC: connected with neighbour server: %s", ncs_list[i].server);
	    /*
	     * Send all already known servers
	     */
	    for (j = 0; j < MAXIBC_SRV; j++) {
		if (strlen(srv_list[j].server) && srv_list[j].hops) {
		    p = calloc(512, sizeof(char));
		    snprintf(p, 512, "SERVER %s %d 0 %s %s %s\r\n", srv_list[j].server, 
			    srv_list[j].hops, srv_list[j].prod, srv_list[j].vers, srv_list[j].fullname);
		    send_msg(i, p);
		    free(p);
		}
	    }
	    /*
	     * Send all known users, nicknames and join channels
	     */
	    for (j = 0; j < MAXIBC_USR; j++) {
		if (strlen(usr_list[j].server)) {
		    p = calloc(512, sizeof(char));
		    snprintf(p, 512, "USER %s@%s %s\r\n", usr_list[j].name, usr_list[j].server, usr_list[j].realname);
		    send_msg(i, p);
		    if (strcmp(usr_list[j].name, usr_list[j].nick)) {
			snprintf(p, 512, "NICK %s %s %s %s\r\n", usr_list[j].nick, 
				usr_list[j].name, usr_list[j].server, usr_list[j].realname);
			send_msg(i, p);
		    }
		    if (strlen(usr_list[j].channel)) {
			snprintf(p, 512, "JOIN %s@%s %s\r\n", usr_list[j].name, usr_list[j].server, usr_list[j].channel);
			send_msg(i, p);
		    }
		    free(p);
		}
	    }
	    /*
	     * Send all known channel topics
	     */
	    for (j = 0; j < MAXIBC_CHN; j++) {
		if (strlen(chn_list[j].topic) && (strcmp(chn_list[j].server, CFG.myfqdn) == 0)) {
		    p = xstrcpy((char *)"TOPIC ");
		    p = xstrcat(p, chn_list[j].name);
		    p = xstrcat(p, (char *)" ");
		    p = xstrcat(p, chn_list[j].topic);
		    p = xstrcat(p, (char *)"\r\n");
		    send_msg(i, p);
		    free(p);
		}
	    }
	    add_server(ncs_list[i].server, ihops, prod, vers, fullname, hostname);
	    return 0;
	}
	Syslog('r', "IBC: call collision with %s", ncs_list[i].server);
	ncs_list[i].state = NCS_WAITPWD; /* Experimental, should fix state when state was connect while it wasn't. */
	return 0;
    }

    /*
     * We are in waiting state, so we sent our PASS and SERVER
     * messages and set the session to connected if we got a
     * valid PASS command.
     */
    if (found && ncs_list[i].gotpass) {
	p = calloc(512, sizeof(char));
	snprintf(p, 512, "PASS %s 0100 %s\r\n", ncs_list[i].passwd, ncs_list[i].compress ? "Z":"");
	send_msg(i, p);
	snprintf(p, 512, "SERVER %s 0 %d mbsebbs %s %s\r\n",  ncs_list[i].myname, token, VERSION, CFG.bbs_name);
	send_msg(i, p);
	snprintf(p, 512, "SERVER %s %d 0 %s %s %s\r\n", name, ihops, prod, vers, fullname);
	broadcast(ncs_list[i].server, p);
	system_shout("* New server: %s, %s", name, fullname);
	ncs_list[i].gotserver = TRUE;
	ncs_list[i].state = NCS_CONNECT;
	ncs_list[i].action = now + (time_t)10;
	Syslog('+', "IBC: connected with neighbour server: %s", ncs_list[i].server);
	/*
	 * Send all already known servers
	 */
	for (j = 0; j < MAXIBC_SRV; j++) {
	    if (strlen(srv_list[j].server) && srv_list[j].hops) {
		snprintf(p, 512, "SERVER %s %d 0 %s %s %s\r\n", srv_list[j].server, 
			srv_list[j].hops, srv_list[j].prod, srv_list[j].vers, srv_list[j].fullname);
		send_msg(i, p);
	    }
	}
	/*
	 * Send all known users. If a user is in a channel, send a JOIN.
	 * If the user is one of our own and has set a channel topic, send it.
	 */
	for (j = 0; j < MAXIBC_USR; j++) {
	    if (strlen(usr_list[j].server)) {
		snprintf(p, 512, "USER %s@%s %s\r\n", usr_list[j].name, usr_list[j].server, usr_list[j].realname);
		send_msg(i, p);
		if (strcmp(usr_list[j].name, usr_list[j].nick)) {
		    snprintf(p, 512, "NICK %s %s %s %s\r\n", usr_list[j].nick, 
			usr_list[j].name, usr_list[j].server, usr_list[j].realname);
		    send_msg(i, p);
		}
		if (strlen(usr_list[j].channel)) {
		    snprintf(p, 512, "JOIN %s@%s %s\r\n", usr_list[j].name, usr_list[j].server, usr_list[j].channel);
		    send_msg(i, p);
		}
	    }
	}
	free(p);
	for (j = 0; j < MAXIBC_CHN; j++) {
	    if (strlen(chn_list[j].topic) && (strcmp(chn_list[j].server, CFG.myfqdn) == 0)) {
		p = xstrcpy((char *)"TOPIC ");
		p = xstrcat(p, chn_list[j].name);
		p = xstrcat(p, (char *)" ");
		p = xstrcat(p, chn_list[j].topic);
		p = xstrcat(p, (char *)"\r\n");
		send_msg(i, p);
		free(p);
	    }
	}
	add_server(ncs_list[i].server, ihops, prod, vers, fullname, hostname);
	srvchg = TRUE;
	callchg = TRUE;
	return 0;
    }

    if (! found) {
       /*
	* Got a message about a server that is not our neighbour, could be a relayed server.
	*/
	if (add_server(name, ihops, prod, vers, fullname, hostname)) {
	    p = calloc(512, sizeof(char));
	    snprintf(p, 512, "SERVER %s %d 0 %s %s %s\r\n", name, ihops, prod, vers, fullname);
	    broadcast(hostname, p);
	    free(p);
	    srvchg = TRUE;
	    Syslog('+', "IBC: new relay server %s: %s", name, fullname);
	    system_shout("* New server: %s, %s", name, fullname);
	}
	return 0;
    }

    Syslog('r', "IBC: got SERVER command without PASS command from %s", hostname);
    return 0;
}



int command_squit(int slot, char *hostname, char *parameters)
{
    char    *p, *name, *message;
    
    name = strtok(parameters, " \0");
    message = strtok(NULL, "\0");

    if (strcmp(name, ncs_list[slot].server) == 0) {
	Syslog('+', "IBC: disconnect neighbour server %s: %s", name, message);
	ncs_list[slot].state = NCS_HANGUP;
	ncs_list[slot].action = now + (time_t)120;	// 2 minutes delay before calling again.
	ncs_list[slot].gotpass = FALSE;
	ncs_list[slot].gotserver = FALSE;
	ncs_list[slot].token = 0;
	del_router(name);
    } else {
	Syslog('+', "IBC: disconnect relay server %s: %s", name, message);
	del_server(name);
    }

    system_shout("* Server %s disconnected: %s", name, message);
    p = calloc(512, sizeof(char));
    snprintf(p, 512, "SQUIT %s %s\r\n", name, message);
    broadcast(hostname, p);
    free(p);
    srvchg = TRUE;
    return 0;
}



int command_user(int slot, char *hostname, char *parameters)
{
    char    *p, *name, *server, *realname;

    name = strtok(parameters, "@\0");
    server = strtok(NULL, " \0");
    realname = strtok(NULL, "\0");

    if (realname == NULL) {
	send_msg(slot, (char *)"400 USER: Not enough parameters\r\n");
	return 400;
    }
    
    if (add_user(server, name, realname) == 0) {
	p = calloc(512, sizeof(char));
	snprintf(p, 512, "USER %s@%s %s\r\n", name, server, realname);
	broadcast(hostname, p);
	free(p);
	system_shout("* New user %s@%s (%s)", name, server, realname);
    }
    return 0;
}



int command_quit(int slot, char *hostname, char *parameters)
{
    char    *p, *name, *server, *message;

    name = strtok(parameters, "@\0");
    server = strtok(NULL, " \0");
    message = strtok(NULL, "\0");

    if (server == NULL) {
	send_msg(slot, (char *)"400 QUIT: Not enough parameters\r\n");
	return 400;
    }

    if (message) {
	system_shout("* User %s is leaving: %s", name, message);
    } else {
	system_shout("* User %s is leaving", name);
    }
    del_user(server, name);
    p = calloc(512, sizeof(char));
    snprintf(p, 512, "QUIT %s@%s %s\r\n", name, server, parameters);
    broadcast(hostname, p);
    free(p);
    return 0;
}



int command_nick(int slot, char *hostname, char *parameters)
{
    char    *p, *nick, *name, *server, *realname;
    int	    i, found, nickerror = FALSE;

    nick = strtok(parameters, " \0");
    name = strtok(NULL, " \0");
    server = strtok(NULL, " \0");
    realname = strtok(NULL, "\0");

    if (realname == NULL) {
	send_msg(slot, (char *)"400 NICK: Not enough parameters\r\n");
	return 1;
    }

    /*
     * Check nickname syntax, max 9 characters, first is alpha, rest alpha or digits.
     */
    if (strlen(nick) > 9) {
	nickerror = TRUE;
    }
    if (! nickerror) {
	if (! isalpha(nick[0]))
	    nickerror = TRUE;
    }
    if (! nickerror) {
	for (i = 1; i < strlen(nick); i++) {
	    if (! isalnum(nick[i])) {
		nickerror = TRUE;
		break;
	    }
	}
    }
    if (nickerror) {
	p = calloc(81, sizeof(char));
	snprintf(p, 81, "402 %s: Erroneous nickname\r\n", nick);
	send_msg(slot, p);
	free(p);
	return 402;
    }
    
    found = FALSE;
    for (i = 0; i < MAXIBC_USR; i++) {
	if ((strcmp(usr_list[i].name, nick) == 0) || (strcmp(usr_list[i].nick, nick) == 0)) {
	    found = TRUE;
	    break;
	}
    }
    if (found) {
	p = calloc(81, sizeof(char));
	snprintf(p, 81, "403 %s: Nickname is already in use\r\n", nick);
	send_msg(slot, p);
	free(p);
	return 403;
    }

    for (i = 0; i < MAXIBC_USR; i++) {
	if ((strcmp(usr_list[i].server, server) == 0) && 
		(strcmp(usr_list[i].realname, realname) == 0) && (strcmp(usr_list[i].name, name) == 0)) {
	    strncpy(usr_list[i].nick, nick, 9);
	    found = TRUE;
	    Syslog('+', "IBC: user %s set nick to %s", name, nick);
	    usrchg = TRUE;
	}
    }
    if (!found) {
	p = calloc(81, sizeof(char));
	snprintf(p, 81, "404 %s@%s: Can't change nick\r\n", name, server);
	send_msg(slot, p);
	free(p);
	return 404;
    }

    p = calloc(512, sizeof(char));
    snprintf(p, 512, "NICK %s %s %s %s\r\n", nick, name, server, realname);
    broadcast(hostname, p);
    free(p);
    return 0;
}



int command_join(int slot, char *hostname, char *parameters)
{
    char        *p, *nick, *server, *channel, msg[81];
    int		i, found;

    nick = strtok(parameters, "@\0");
    server = strtok(NULL, " \0");
    channel = strtok(NULL, "\0");

    if (channel == NULL) {
	send_msg(slot, (char *)"400 JOIN: Not enough parameters\r\n");
	return 400;
    }

    if (strlen(channel) > 20) {
	p = calloc(81, sizeof(char));
	snprintf(p, 81, "402 %s: Erroneous channelname\r\n", nick);
	send_msg(slot, p);
	free(p);
	return 402;
    }

    if (strcasecmp(channel, "#sysop") == 0) {
	Syslog('+', "IBC: ignored JOIN for #sysop channel");
	return 0;
    }

    found = FALSE;
    for (i = 0; i < MAXIBC_CHN; i++) {
	if (strcmp(chn_list[i].name, channel) == 0) {
	    found = TRUE;
	    chn_list[i].users++;
	    break;
	}
    }
    if (!found) {
	Syslog('+', "IBC: create channel %s owned by %s@%s", channel, nick, server);
	add_channel(channel, nick, server);
	system_shout("* New channel %s created by %s@%s", channel, nick, server);
    }

    for (i = 0; i < MAXIBC_USR; i++) {
	if ((strcmp(usr_list[i].server, server) == 0) && 
		((strcmp(usr_list[i].nick, nick) == 0) || (strcmp(usr_list[i].name, nick) == 0))) {
	    strncpy(usr_list[i].channel, channel, 20);
	    Syslog('+', "IBC: user %s joined channel %s", nick, channel);
	    usrchg = TRUE;
	    snprintf(msg, 81, "* %s@%s has joined %s", nick, server, channel);
	    chat_msg(channel, NULL, msg);
	}
    }

    p = calloc(512, sizeof(char));
    snprintf(p, 512, "JOIN %s@%s %s\r\n", nick, server, channel);
    broadcast(hostname, p);
    free(p);
    chnchg = TRUE;
    return 0;
}



int command_part(int slot, char *hostname, char *parameters)
{
    char        *p, *nick, *server, *channel, *message, msg[81];
    int		i;

    nick = strtok(parameters, "@\0");
    server = strtok(NULL, " \0");
    channel = strtok(NULL, " \0");
    message = strtok(NULL, "\0");

    if (channel == NULL) {
	send_msg(slot, (char *)"400 PART: Not enough parameters\r\n");
	return 400;
    }

    if (strcasecmp(channel, "#sysop") == 0) {
	Syslog('+', "IBC: ignored PART from #sysop channel");
	return 0;
    }

    del_userchannel(channel);

    for (i = 0; i < MAXIBC_USR; i++) {
	if ((strcmp(usr_list[i].server, server) == 0) && 
		((strcmp(usr_list[i].nick, nick) == 0) || (strcmp(usr_list[i].name, nick) == 0))) {
	    usr_list[i].channel[0] = '\0';
	    if (message) {
		Syslog('+', "IBC: user %s left channel %s: %s", nick, channel, message);
		snprintf(msg, 81, "* %s@%s has left channel %s: %s", nick, server, channel, message);
	    } else {
		Syslog('+', "IBC: user %s left channel %s", nick, channel);
		snprintf(msg, 81, "* %s@%s has silently left channel %s", nick, server, channel);
	    }
	    chat_msg(channel, NULL, msg);
	    usrchg = TRUE;
	}
    }

    p = xstrcpy((char *)"PART ");
    p = xstrcat(p, nick);
    p = xstrcat(p, (char *)"@");
    p = xstrcat(p, server);
    p = xstrcat(p, (char *)" ");
    p = xstrcat(p, channel);
    if (message) {
	p = xstrcat(p, (char *)" ");
	p = xstrcat(p, message);
    }
    p = xstrcat(p, (char *)"\r\n");
    broadcast(hostname, p);
    free(p);
    return 0;
}



int command_topic(int slot, char *hostname, char *parameters)
{
    char    *p, *channel, *topic, msg[81];
    int	    i;

    channel = strtok(parameters, " \0");
    topic = strtok(NULL, "\0");

    if (topic == NULL) {
	send_msg(slot, (char *)"400 TOPIC: Not enough parameters\r\n");
	return 400;
    }

    for (i = 0; i < MAXIBC_CHN; i++) {
	if (strcmp(chn_list[i].name, channel) == 0) {
	    chnchg = TRUE;
	    strncpy(chn_list[i].topic, topic, 54);
	    Syslog('+', "IBC: channel %s topic: %s", channel, topic);
	    snprintf(msg, 81, "* Channel topic is now: %s", chn_list[i].topic);
	    chat_msg(channel, NULL, msg);
	    break;
	}
    }

    p = xstrcpy((char *)"TOPIC ");
    p = xstrcat(p, channel);
    p = xstrcat(p, (char *)" ");
    p = xstrcat(p, topic);
    p = xstrcat(p, (char *)"\r\n");
    broadcast(hostname, p);
    free(p);
    return 0;
}



int command_privmsg(int slot, char *hostname, char *parameters)
{
    char    *p, *channel, *msg;
    int	    i;
    
    channel = strtok(parameters, " \0");
    msg = strtok(NULL, "\0");

    if (msg == NULL) {
	send_msg(slot, (char *)"412 PRIVMSG: No text to send\r\n");
	return 412;
    }

    if (channel[0] != '#') {
	send_msg(slot, (char *)"499 PRIVMSG: Not for a channel\r\n"); // FIXME: also check users
	return 499;
    }

    for (i = 0; i < MAXIBC_CHN; i++) {
	if (strcmp(chn_list[i].name, channel) == 0) {
	    chn_list[i].lastmsg = now;
	    chat_msg(channel, NULL, msg);
	    p = xstrcpy((char *)"PRIVMSG ");
	    p = xstrcat(p, channel);
	    p = xstrcat(p, (char *)" ");
	    p = xstrcat(p, msg);
	    p = xstrcat(p, (char *)"\r\n");
	    broadcast(hostname, p);
	    free(p);
	    return 0;
	}
    }

    p = calloc(81, sizeof(char));
    snprintf(p, 81, "409 %s: Cannot sent to channel\r\n", channel);
    send_msg(slot, p);
    free(p);
    return 409;
}



int do_command(char *hostname, char *command, char *parameters)
{
    char    *p;
    int	    i, found = FALSE;

    for (i = 0; i < MAXIBC_NCS; i++) {
	if (strcmp(ncs_list[i].server, hostname) == 0) {
	    found = TRUE;
	    break;
	}
    }

    if (! found) {
	Syslog('!', "IBC: got a command from unknown %s", hostname);
	return 0;
    }

    /*
     * First the commands that don't have parameters
     */
    if (! strcmp(command, (char *)"PING")) {
	send_msg(i, (char *)"PONG\r\n");
	return 0;
    }

    if (! strcmp(command, (char *)"PONG")) {
	/*
	 * Just accept, but reset halfdead counter.
	 */
	if (ncs_list[i].halfdead) {
	    Syslog('r', "IBC: Reset halfdead counter");
	    ncs_list[i].halfdead = 0;
	    srvchg = TRUE;
	}
	return 0;
    }

    /*
     * Commands with parameters
     */
    if (parameters == NULL) {
	p = calloc(81, sizeof(char));
	snprintf(p, 81, "400 %s: Not enough parameters\r\n", command);
	send_msg(i, p);
	free(p);
	return 400;
    }

    if (! strcmp(command, (char *)"PASS")) {
	return command_pass(i, hostname, parameters);
    } 
    if (! strcmp(command, (char *)"SERVER")) {
	return command_server(hostname, parameters);
    } 
    if (! strcmp(command, (char *)"SQUIT")) {
	return command_squit(i, hostname, parameters);
    } 
    if (! strcmp(command, (char *)"USER")) {
	return command_user(i, hostname, parameters);
    } 
    if (! strcmp(command, (char *)"QUIT")) {
	return command_quit(i, hostname, parameters);
    } 
    if (! strcmp(command, (char *)"NICK")) {
	return command_nick(i, hostname, parameters);
    }
    if (! strcmp(command, (char *)"JOIN")) {
	return command_join(i, hostname, parameters);
    }
    if (! strcmp(command, (char *)"PART")) {
	return command_part(i, hostname, parameters);
    } 
    if (! strcmp(command, (char *)"TOPIC")) {
	return command_topic(i, hostname, parameters);
    }
    if (! strcmp(command, (char *)"PRIVMSG")) {
	return command_privmsg(i, hostname, parameters);
    }

    p = calloc(81, sizeof(char));
    snprintf(p, 81, "413 %s: Unknown command\r\n", command);
    send_msg(i, p);
    free(p);
    return 413;
}



void ibc_receiver(char *crbuf)
{
    struct hostent  *hp, *tp;
    struct in_addr  in;
    int             inlist, i;
    char            *hostname, *command, *parameters, *ipaddress;

    hp = gethostbyaddr((char *)&clientaddr_in.sin_addr, sizeof(struct in_addr), clientaddr_in.sin_family);
    if (hp == NULL)
	hostname = inet_ntoa(clientaddr_in.sin_addr);
    else
	hostname = hp->h_name;

    if ((crbuf[strlen(crbuf) -2] != '\r') && (crbuf[strlen(crbuf) -1] != '\n')) {
	Syslog('!', "IBC: got message not terminated with CR-LF, dropped");
	return;
    }

    /*
     * First check for a fixed IP address.
     */
    inlist = FALSE;
    for (i = 0; i < MAXIBC_NCS; i++) {
	if (strcmp(ncs_list[i].server, hostname) == 0) {
	    inlist = TRUE;
	    break;
	}
    }
    if (!inlist) {
	/*
	 * Check for dynamic dns address
	 */
	ipaddress = xstrcpy(inet_ntoa(clientaddr_in.sin_addr));
	for (i = 0; i < MAXIBC_NCS; i++) {
	    if (strlen(ncs_list[i].server) && ncs_list[i].dyndns) {
		tp = gethostbyname(ncs_list[i].server);
		if (tp != NULL) {
		    memcpy(&in, tp->h_addr, tp->h_length);
		    if (strcmp(inet_ntoa(in), ipaddress) == 0) {
			/*
			 * Test if the backresolved dynamic DNS name is changed, but exclude the
			 * initial value which is the same as the real server name.
			 */
			if (strcmp(ncs_list[i].resolved, hostname) && strcmp(ncs_list[i].resolved, ncs_list[i].server)) {
			    Syslog('r', "IBC: GrepThiz old resolved %s new resolved %s state %s", 
					ncs_list[i].resolved, hostname, ncsstate[ncs_list[i].state]);
			    Syslog('+', "IBC: server %s resolved FQDN changed, restarting", ncs_list[i].server);
			    ncs_list[i].crc--;
			    link_reset = TRUE;
			    /*
			     * This would be the moment to reset this neighbour
			     * Double check state: waitpwd or call ?
			     */
			}
			/*
			 * Store the back resolved IP fqdn for reference and change the
			 * FQDN to the one from the setup, so we continue to use the
			 * dynamic FQDN.
			 */
			if (strcmp(ncs_list[i].resolved, hostname))
			    Syslog('r', "IBC: setting '%s' to dynamic dns '%s'", hostname, ncs_list[i].server);
			strncpy(ncs_list[i].resolved, hostname, 63);
			inlist = TRUE;
			hostname = ncs_list[i].server;
			break;
		    }
		}
	    }
	}
	free(ipaddress);
    }
    if (!inlist) {
	Syslog('r', "IBC: message from unknown host (%s), dropped", hostname);
	return;
    }

    if (ncs_list[i].state == NCS_INIT) {
	Syslog('r', "IBC: message received from %s while in init state, dropped", hostname);
	return;
    }

    ncs_list[i].last = now;
    crbuf[strlen(crbuf) -2] = '\0';
#ifndef PING_PONG_LOG
    if (strcmp(crbuf, (char *)"PING") && strcmp(crbuf, (char *)"PONG"))
#endif
	Syslog('r', "IBC: < %s: \"%s\"", hostname, printable(crbuf, 0));

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
}



void ibc_init(void)
{
    memset(&ncs_list, 0, sizeof(ncs_list));
    memset(&srv_list, 0, sizeof(srv_list));
    memset(&usr_list, 0, sizeof(usr_list));
    memset(&chn_list, 0, sizeof(chn_list));
}



void ibc_shutdown(void)
{
    char    *p;
    int	    i;

    Syslog('r', "IBC: start shutdown connections");

    p = calloc(512, sizeof(char));
    for (i = 0; i < MAXIBC_USR; i++) {
	if (strcmp(usr_list[i].server, CFG.myfqdn) == 0) {
	    /*
	     * Our user, still connected
	     */
	    if (strlen(usr_list[i].channel) && strcmp(usr_list[i].channel, "#sysop")) {
		/*
		 * In a channel
		 */
		snprintf(p, 512, "PART %s@%s %s System shutdown\r\n", usr_list[i].nick, usr_list[i].server, usr_list[i].channel);
		broadcast((char *)"foobar", p);
	    }
	    snprintf(p, 512, "QUIT %s@%s System shutdown\r\n", usr_list[i].nick, usr_list[i].server);
	    broadcast((char *)"foobar", p);
	}
    }

    for (i = 0; i < MAXIBC_NCS; i++) {
	if (strlen(ncs_list[i].server) && ncs_list[i].state == NCS_CONNECT) {
	    snprintf(p, 512, "SQUIT %s System shutdown\r\n", ncs_list[i].myname);
	    send_msg(i, p);
	}
    }
    free(p);
}

