/*****************************************************************************
 *
 * $Id: taskchat.c,v 1.63 2006/05/27 13:19:53 mbse Exp $
 * Purpose ...............: mbtask - chat server
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
#include "taskutil.h"
#include "taskregs.h"
#include "taskchat.h"
#include "taskibc.h"


#define	MAXMESSAGES 100		    /* Maximum ringbuffer for messages	*/



/*
 *  Buffer for messages, this is the structure of a ringbuffer which will
 *  hold all messages, private public etc. There is one global input pointer
 *  which points to the current head of the ringbuffer. When a user connects
 *  to a channel, he will get the latest messages in the channel if they
 *  are present.
 */
typedef struct	_chatmsg {
    pid_t	topid;		    /* Destination pid of message	*/
    char	fromname[36];	    /* Message from user		*/
    char	message[81];	    /* The message to display		*/
    time_t	posted;		    /* Timestamp for posted message	*/
} _chat_messages;



/*
 *  The buffers
 */
_chat_messages		chat_messages[MAXMESSAGES];


int			buffer_head = 0;    /* Messages buffer head	*/
extern struct sysconfig CFG;		    /* System configuration	*/
extern int		s_bbsopen;	    /* The BBS open status	*/
extern int		usrchg;
extern int		chnchg;
extern int		srvchg;
extern int		Run_IBC;



/*
 * Prototypes
 */
void Chatlog(char *, char *, char *);
void chat_dump(void);
void system_msg(pid_t, char *);
void chat_help(pid_t, int);
int join(pid_t, char *, int);
int part(pid_t, char*);



void Chatlog(char *level, char *channel, char *msg)
{
    char    *logm;

    if (CFG.iAutoLog && strlen(CFG.chat_log)) {
	logm = calloc(PATH_MAX, sizeof(char));
	snprintf(logm, PATH_MAX, "%s/log/%s", getenv("MBSE_ROOT"), CFG.chat_log);
	ulog(logm, level, channel, (char *)"-1", msg);
	free(logm);
    }
}



void chat_dump(void)
{
    int	    i, first;
    
    first = TRUE;
    for (i = 0; i < MAXIBC_USR; i++) {
	if (usr_list[i].pid) {
	    if (first) {
		Syslog('c', "  pid username                             nick      channel              sysop");
		Syslog('c', "----- ------------------------------------ --------- -------------------- -----");
		first = FALSE;
	    }
	    Syslog('c', "%5d %-36s %-9s %-20s %s", usr_list[i].pid, usr_list[i].realname, usr_list[i].nick,
		usr_list[i].channel, usr_list[i].sysop?"True ":"False");
	}
    }
}



/*
 * Put a system message into the chatbuffer
 */
void system_msg(pid_t pid, char *msg)
{
    if (buffer_head < MAXMESSAGES)
	buffer_head++;
    else
	buffer_head = 0;

    memset(&chat_messages[buffer_head], 0, sizeof(_chat_messages));
    chat_messages[buffer_head].topid = pid;
    strncpy(chat_messages[buffer_head].fromname, "Server", 36);
    strncpy(chat_messages[buffer_head].message, msg, 80);
    chat_messages[buffer_head].posted = time(NULL);
}



/*
 * Shout a message to all users
 */
void system_shout(const char *format, ...)
{
    char        *buf;
    va_list     va_ptr;
    int		i;

    buf = calloc(512, sizeof(char));
    va_start(va_ptr, format);
    vsnprintf(buf, 512, format, va_ptr);
    va_end(va_ptr);

    for (i = 0; i < MAXIBC_USR; i++) {
	if (usr_list[i].pid) {
	    system_msg(usr_list[i].pid, buf);
	}
    }

    Chatlog((char *)"-", (char *)" ", buf);
    free(buf);
}



/*
 * Show help
 */
void chat_help(pid_t pid, int owner)
{
    system_msg(pid, (char *)"                     Help topics available:");
    system_msg(pid, (char *)"");
    system_msg(pid, (char *)" /BYE              - Exit from chatserver");
    system_msg(pid, (char *)" /ECHO <message>   - Echo message to yourself");
    system_msg(pid, (char *)" /EXIT             - Exit from chatserver");
    system_msg(pid, (char *)" /JOIN #channel    - Join or create a channel");
    system_msg(pid, (char *)" /J #channel       - Join or create a channel");
//  system_msg(pid, (char *)" /KICK <nick>      - Kick nick out of the channel");
    system_msg(pid, (char *)" /LIST             - List active channels");
    system_msg(pid, (char *)" /NAMES            - List nicks in current channel");
    system_msg(pid, (char *)" /NICK <name>      - Set new nickname");
    system_msg(pid, (char *)" /PART             - Leave current channel");
    system_msg(pid, (char *)" /QUIT             - Exit from chatserver");
    if (owner)
	system_msg(pid, (char *)" /TOPIC <topic>    - Set topic for current channel");
    system_msg(pid, (char *)"");
    system_msg(pid, (char *)" All other input (without a starting /) is sent to the channel.");
}



/*
 * Join a channel
 */
int join(pid_t pid, char *channel, int sysop)
{
    char	buf[81];
    int		i, j;

    Syslog('c', "Join pid %d to channel %s", pid, channel);

    for (i = 0; i < MAXIBC_CHN; i++) {
	if (strcmp(chn_list[i].name, channel) == 0) {
	    for (j = 0; j < MAXIBC_USR; j++) {
		if (usr_list[j].pid == pid) {

		    strncpy(usr_list[j].channel, channel, 20);
		    chn_list[i].users++;
		    Syslog('+', "IBC: user %s has joined channel %s", usr_list[j].nick, channel);
		    usrchg = TRUE;
		    srvchg = TRUE;
		    chnchg = TRUE;

		    chat_dump();
		    snprintf(buf, 81, "%s has joined channel %s, now %d users", usr_list[j].nick, channel, chn_list[i].users);
		    chat_msg(channel, NULL, buf);

		    /*
		     * The sysop channel is private to the system, no broadcast
		     */
		    if (strcasecmp(channel, "#sysop"))
		        send_at((char *)"JOIN", usr_list[j].nick, channel);
		    return TRUE;
		}
	    }
	}
    }

    /*
     * A new channel must be created, but only the sysop may create the "sysop" channel
     */
    if (!sysop && (strcasecmp(channel, "#sysop") == 0)) {
	snprintf(buf, 81, "*** Only the sysop may create channel \"%s\"", channel);
	system_msg(pid, buf);
	return FALSE;
    }

    /*
     * No matching channel found, add a new channel.
     */
    for (j = 0; j < MAXIBC_USR; j++) {
	if (usr_list[j].pid == pid) {
	    if (add_channel(channel, usr_list[j].nick, CFG.myfqdn) == 0) {

		strncpy(usr_list[j].channel, channel, 20);
		Syslog('+', "IBC: user %s created and joined channel %s", usr_list[j].nick, channel);
		usrchg = TRUE;
		chnchg = TRUE;
		srvchg = TRUE;

		snprintf(buf, 81, "* Created channel %s", channel);
		chat_msg(channel, NULL, buf);
		chat_dump();
		if (strcasecmp(channel, "#sysop"))
		    send_at((char *)"JOIN", usr_list[j].nick, channel);

		return TRUE;
	    }
	}
    }

    /*
     * No matching or free channels
     */
    snprintf(buf, 81, "*** Cannot create chat channel %s, no free channels", channel);
    system_msg(pid, buf);
    return FALSE;
}



/*
 * Part from a channel
 */
int part(pid_t pid, char *reason)
{
    char    buf[81], *p;
    int	    i, j;

    if (strlen(reason) > 54)
	reason[54] = '\0';

    Syslog('c', "Part pid %d from channel, reason %s", pid, reason);

    for (i = 0; i < MAXIBC_USR; i++) {
	if ((usr_list[i].pid == pid) && strlen(usr_list[i].channel)) {
	    for (j = 0; j < MAXIBC_CHN; j++) {
		if (strcmp(chn_list[j].name, usr_list[i].channel) == 0) {
		    /*
		     * Inform other users
		     */
		    if (reason != NULL) {
			chat_msg(usr_list[i].channel, usr_list[i].nick, reason);
		    }
		    snprintf(buf, 81, "%s has left channel %s, %d users left", 
			    usr_list[i].nick, chn_list[j].name, chn_list[j].users -1);
		    chat_msg(usr_list[i].channel, NULL, buf);
		    if (strcasecmp(chn_list[j].name, (char *)"#sysop")) {
			p = xstrcpy(chn_list[j].name);
			if (reason && strlen(reason)) {
			    p = xstrcat(p, (char *)" ");
			    p = xstrcat(p, reason);
			}
			send_at((char *)"PART", usr_list[i].nick, p);
			free(p);
		    }

		    /*
		     * Clean channel
		     */
		    if (chn_list[j].users > 0)
			chn_list[j].users--;
		    Syslog('+', "IBC: nick %s leaves channel %s, %d user left", 
			    usr_list[i].nick, chn_list[j].name, chn_list[j].users);
		    if (chn_list[j].users == 0) {
			/*
			 * Last user in channel, remove channel
			 */
			snprintf(buf, 81, "* Removed channel %s", chn_list[j].name);
			system_msg(pid, buf);
			Syslog('+', "IBC: removed channel %s, no more users left", chn_list[j].name);
			del_channel(chn_list[j].name);
		    }

		    /*
		     * Update user data
		     */
		    usr_list[i].channel[0] = '\0';
		    usrchg = TRUE;
		    chnchg = TRUE;
		    srvchg = TRUE;

		    chat_dump();
		    return TRUE;
		}
	    }
	}
    }

    return FALSE;
}



void chat_init(void)
{
    memset(&chat_messages, 0, sizeof(chat_messages));
}



void chat_cleanuser(pid_t pid)
{
    part(pid, (char *)"I'm hanging up!");
}



/*
 * Send message into channel
 */
void chat_msg(char *channel, char *nick, char *msg)
{
    char    *p;
    int	    i;
    
    if (nick == NULL)
	p = xstrcpy(msg);
    else {
	p = xstrcpy((char *)"<");
	p = xstrcat(p, nick);
	p = xstrcat(p, (char *)"> ");
	p = xstrcat(p, msg);
    }
    Chatlog((char *)"+", channel, p);

    for (i = 0; i < MAXIBC_USR; i++) {
	if (strlen(usr_list[i].channel) && (strcmp(usr_list[i].channel, channel) == 0)) {
	    system_msg(usr_list[i].pid, p);
	}
    }

    free(p);
}



/*
 * Connect a session to the chatserver.
 */
void chat_connect_r(char *data, char *buf)
{
    char	*realname, *nick, *temp;
    int		i, j, rc, count = 0, sys = FALSE;
    pid_t	pid;

    Syslog('c', "CCON:%s", data);

    if (! Run_IBC) {
	snprintf(buf, 200, "100:1,*** Chatserver not configured;");
	return;
    }

    if (IsSema((char *)"upsalarm")) {
	snprintf(buf, 200, "100:1,*** Power failure, running on UPS;");
	return;
    }

    if (s_bbsopen == FALSE) {
	snprintf(buf, 200, "100:1,*** The BBS is closed now;");
	return;
    }

    /*
     * Register with IBC
     */
    strtok(data, ",");				    /* Should be 3  */
    pid = (pid_t)atoi(strtok(NULL, ","));	    /* The pid      */
    realname = xstrcpy(cldecode(strtok(NULL, ",")));/* Username     */
    nick = xstrcpy(cldecode(strtok(NULL, ",")));    /* Nickname     */
    sys = atoi(strtok(NULL, ";"));		    /* Sysop flag   */

    rc = add_user(CFG.myfqdn, nick, realname);
    if (rc == 1) {
	free(realname);
	free(nick);
	snprintf(buf, 200, "100:1,Already registered;");
	return;
    } else if (rc == 2) {
	free(realname);
	free(nick);
	snprintf(buf, 200, "100:1,Too many users connected;");
	return;
    }

    send_at((char *)"USER", nick, realname);
    
    /*
     * Now search the added entry to update the data
     */
    for (i = 0; i < MAXIBC_USR; i++) {
	if ((strcmp(usr_list[i].server, CFG.myfqdn) == 0) && 
		(strcmp(usr_list[i].name, nick) == 0) && (strcmp(usr_list[i].realname, realname) == 0)) {
	    /*
	     * Oke, found
	     */
	    usr_list[i].pid = pid;
	    usr_list[i].pointer = buffer_head;
	    usr_list[i].sysop = sys;
	    usrchg = TRUE;
	    srvchg = TRUE;
	    Syslog('c', "Connected user %s (%d) with chatserver, sysop %s", realname, (int)pid, sys ? "True":"False");

            /*
	     * Now put welcome message into the ringbuffer and report success.
	     */
	    temp = calloc(81, sizeof(char));
	    snprintf(temp, 200, "MBSE BBS v%s chat server; type /help for help", VERSION);
	    system_msg(usr_list[i].pid, temp);
	    snprintf(temp, 200, "Welcome to the Internet BBS Chat Network");
	    system_msg(usr_list[i].pid, temp);
	    snprintf(temp, 200, "Current connected servers:");
	    system_msg(usr_list[i].pid, temp);
	    for (j = 0; j < MAXIBC_SRV; j++) {
		if (strlen(srv_list[j].server)) {
		    snprintf(temp, 200, "  %d user%s at '%s'",
			    srv_list[j].users, (srv_list[j].users == 1) ? " ":"s", srv_list[j].fullname);
		    system_msg(usr_list[i].pid, temp);
		    count += srv_list[j].users;
		}
	    }
	    snprintf(temp, 200, "There %s %d user%s connected", (count != 1)?"are":"is", count, (count != 1)?"s":"");
	    system_msg(usr_list[i].pid, temp);
	    snprintf(buf, 200, "100:0;");
	    free(realname);
	    free(nick);
	    free(temp);
	    return;
	}
    }

    return;
}



void chat_close_r(char *data, char *buf)
{
    pid_t   pid;
    int	    i;

    Syslog('c', "CCLO:%s", data);
    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ";"));
 
    for (i = 0; i < MAXIBC_USR; i++) {
	if (usr_list[i].pid == pid) {
	    /*
	     * Remove from IBC network
	     */
	    send_at((char *)"QUIT", usr_list[i].name, (char *)"Leaving chat");
	    del_user(CFG.myfqdn, usr_list[i].name);
	    Syslog('c', "Closing chat for pid %d", (int)pid);
	    snprintf(buf, 81, "100:0;");
	    return;
	}
    }

    Syslog('c', "Pid %d was not connected to chatserver", (int)pid);
    snprintf(buf, 81, "100:1,*** ERROR - Not connected to server;");
    return;
}



void chat_put_r(char *data, char *buf)
{
    char	*p, *q, *msg, *cmd, *mbuf, *flags, temp[81];
    int		i, j, k, first, count, owner = FALSE, found;
    pid_t	pid;

    if (IsSema((char *)"upsalarm")) {
	snprintf(buf, 81, "100:2,1,*** Power alarm, running on UPS;");
	return;
    }

    if (s_bbsopen == FALSE) {
	snprintf(buf, 81, "100:2,1,*** The BBS is closed now;");
	return;
    }
	
    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ","));
    msg = xstrcpy(cldecode(strtok(NULL, ";")));
    mbuf = calloc(200, sizeof(char));

    for (i = 0; i < MAXIBC_USR; i++) {
	if (usr_list[i].pid == pid) {
	    if (msg[0] == '/') {
		/*
		 * A command, process this but first see if we are in a channel
		 * and own the channel. This gives us more power.
		 */
		if (strlen(usr_list[i].channel)) {
		    for (j = 0; j < MAXIBC_CHN; j++) {
			if (((strcmp(usr_list[i].nick, chn_list[j].owner) == 0) || 
			     (strcmp(usr_list[i].name, chn_list[j].owner) == 0)) &&
			    (strcmp(usr_list[i].server, chn_list[j].server) == 0)) {
			    owner = TRUE;
			}
		    }
		}
		if (strncasecmp(msg, "/help", 5) == 0) {
		    chat_help(pid, owner);
		    goto ack;
		} else if (strncasecmp(msg, "/echo", 5) == 0) {
		    snprintf(mbuf, 200, "%s", msg);
		    system_msg(usr_list[i].pid, mbuf);
		    goto ack;
		} else if ((strncasecmp(msg, "/exit", 5) == 0) || 
		    (strncasecmp(msg, "/quit", 5) == 0) ||
		    (strncasecmp(msg, "/bye", 4) == 0)) {
		    if (strlen(usr_list[i].channel)) {
			/*
			 * If in a channel, leave channel first
			 */
			part(usr_list[i].pid, (char *)"Quitting");
			snprintf(mbuf, 81, "Goodbye");
			system_msg(usr_list[i].pid, mbuf);
		    }
		    goto hangup;
		} else if ((strncasecmp(msg, "/join", 5) == 0) ||
		    (strncasecmp(msg, "/j ", 3) == 0)) {
		    cmd = strtok(msg, " \0");
		    cmd = strtok(NULL, "\0");
		    if ((cmd == NULL) || (cmd[0] != '#') || (strcmp(cmd, "#") == 0)) {
			snprintf(mbuf, 200, "** Try /join #channel");
			system_msg(usr_list[i].pid, mbuf);
		    } else if (strlen(usr_list[i].channel)) {
			snprintf(mbuf, 200, "** Cannot join while in a channel");
			system_msg(usr_list[i].pid, mbuf);
		    } else {
			join(usr_list[i].pid, cmd, usr_list[i].sysop);
		    }
		    chat_dump();
		    goto ack;
		} else if (strncasecmp(msg, "/list", 5) == 0) {
		    first = TRUE;
		    for (j = 0; j < MAXIBC_CHN; j++) {
			if (strlen(chn_list[j].server)) {
			    if (first) {
				snprintf(mbuf, 200, "Cnt Channel name         Channel topic");
				system_msg(usr_list[i].pid, mbuf);
				snprintf(mbuf, 200, "--- -------------------- ------------------------------------------------------");
				system_msg(usr_list[i].pid, mbuf);
			    }
			    first = FALSE;
			    q = calloc(81, sizeof(char));
			    snprintf(q, 81, "%3d %-20s ", chn_list[j].users, chn_list[j].name);
			    p = xstrcpy(q);
			    p = xstrcat(p, chn_list[j].topic);
			    system_msg(usr_list[i].pid, p);
			    free(p);
			    free(q);
			}
		    }
		    if (first) {
			snprintf(mbuf, 200, "No active channels to list");
			system_msg(usr_list[i].pid, mbuf);
		    }
		    goto ack;
		} else if (strncasecmp(msg, "/names", 6) == 0) {
		    if (strlen(usr_list[i].channel)) {
			snprintf(mbuf, 200, "Present in channel %s:", usr_list[i].channel);
			system_msg(usr_list[i].pid, mbuf);
			snprintf(mbuf, 200, "Nick                                     Real name                      Flags");
			system_msg(usr_list[i].pid, mbuf);
			snprintf(mbuf, 200, "---------------------------------------- ------------------------------ -----");
			system_msg(usr_list[i].pid, mbuf);
			count = 0;
			for (j = 0; j < MAXIBC_USR; j++) {
			    if (strcmp(usr_list[j].channel, usr_list[i].channel) == 0) {
				/*
				 * Get channel flags
				 */
				flags = xstrcpy((char *)"--");
				for (k = 0; k < MAXIBC_CHN; k++) {
				    if (strcmp(chn_list[k].name, usr_list[i].channel) == 0) {
					if (((strcmp(chn_list[k].owner, usr_list[j].nick) == 0) || 
					     (strcmp(chn_list[k].owner, usr_list[j].name) == 0)) &&
					    (strcmp(chn_list[k].server, usr_list[j].server) == 0)) {
					    flags[0] = 'O';
					}
				    }
				}
				if (usr_list[j].sysop)
				    flags[1] = 'S';

				snprintf(temp, 81, "%s@%s", usr_list[j].nick, usr_list[j].server);
				snprintf(mbuf, 200, "%-40s %-30s %s", temp, usr_list[j].realname, flags);
				system_msg(usr_list[i].pid, mbuf);
				count++;
				free(flags);
			    }
			}
			snprintf(mbuf, 200, "%d user%s in this channel", count, (count == 1) ?"":"s");
			system_msg(usr_list[i].pid, mbuf);
		    } else {
			snprintf(mbuf, 200, "** Not in a channel");
			system_msg(usr_list[i].pid, mbuf);
		    }
		    goto ack;
		} else if (strncasecmp(msg, "/nick", 5) == 0) {
		    cmd = strtok(msg, " \0");
		    cmd = strtok(NULL, "\0");
		    if ((cmd == NULL) || (strlen(cmd) == 0) || (strlen(cmd) > 9)) {
			snprintf(mbuf, 200, "** Nickname must be between 1 and 9 characters");
		    } else {
			found = FALSE;
			for (j = 0; j < MAXIBC_USR; j++) {
			    if ((strcmp(usr_list[j].name, cmd) == 0) || (strcmp(usr_list[j].nick, cmd) == 0)) {
				found = TRUE;
			    }
			}

			if (!found ) {
			    strncpy(usr_list[i].nick, cmd, 9);
			    snprintf(mbuf, 200, "Nick set to \"%s\"", cmd);
			    system_msg(usr_list[i].pid, mbuf);
			    send_nick(usr_list[i].nick, usr_list[i].name, usr_list[i].realname);
			    usrchg = TRUE;
			    chat_dump();
			    goto ack;
			}
			snprintf(mbuf, 200, "Can't set nick");
		    }
		    system_msg(usr_list[i].pid, mbuf);
		    chat_dump();
		    goto ack;
		} else if (strncasecmp(msg, "/part", 5) == 0) {
		    cmd = strtok(msg, " \0");
		    Syslog('c', "\"%s\"", cmd);
		    cmd = strtok(NULL, "\0");
		    Syslog('c', "\"%s\"", printable(cmd, 0));
		    if (part(usr_list[i].pid, cmd ? cmd : (char *)"Quitting") == FALSE) {
			snprintf(mbuf, 200, "** Not in a channel");
			system_msg(usr_list[i].pid, mbuf);
		    }
		    chat_dump();
		    goto ack;
		} else if (strncasecmp(msg, "/topic", 6) == 0) {
		    if (strlen(usr_list[i].channel)) {
			snprintf(mbuf, 200, "** Internal system error");
			for (j = 0; j < MAXIBC_CHN; j++) {
			    if (strcmp(usr_list[i].channel, chn_list[j].name) == 0) {
				if (owner) {
				    cmd = strtok(msg, " \0");
				    cmd = strtok(NULL, "\0");
				    if ((cmd == NULL) || (strlen(cmd) == 0) || (strlen(cmd) > 54)) {
					snprintf(mbuf, 200, "** Topic must be between 1 and 54 characters");
				    } else {
					strncpy(chn_list[j].topic, cmd, 54);
					snprintf(mbuf, 200, "Topic set to \"%s\"", cmd);
					p = xstrcpy((char *)"TOPIC ");
					p = xstrcat(p, chn_list[j].name);
					p = xstrcat(p, (char *)" ");
					p = xstrcat(p, chn_list[j].topic);
					p = xstrcat(p, (char *)"\r\n");
					send_all(p);
					free(p);
				    }
				} else {
				    snprintf(mbuf, 200, "** You are not the channel owner");
				}
				break;
			    }
			}
		    } else {
			snprintf(mbuf, 200, "** Not in a channel");
		    }
		    system_msg(usr_list[i].pid, mbuf);
		    chat_dump();
		    goto ack;
		} else {
		    /*
		     * If still here, the command was not recognized.
		     */
		    cmd = strtok(msg, " \t\r\n\0");
		    snprintf(mbuf, 200, "*** \"%s\" :Unknown command", cmd+1);
		    system_msg(usr_list[i].pid, mbuf);
		    goto ack;
		}
	    }
	    if (strlen(usr_list[i].channel) == 0) {
		/*
		 * Trying messages while not in a channel
		 */
		snprintf(mbuf, 200, "** No channel joined. Try /join #channel");
		system_msg(usr_list[i].pid, mbuf);
		goto ack;
	    } else {
		chat_msg(usr_list[i].channel, usr_list[i].nick, msg);
		/*
		 * Send message to all links but not the #sysop channel
		 */
		if (strcmp(usr_list[i].channel, "#sysop")) {
		    p = xstrcpy((char *)"PRIVMSG ");
		    p = xstrcat(p, usr_list[i].channel);
		    p = xstrcat(p, (char *)" <");
		    p = xstrcat(p, usr_list[i].nick);
		    p = xstrcat(p, (char *)"> ");
		    p = xstrcat(p, msg);
		    p = xstrcat(p, (char *)"\r\n");
		    send_all(p);
		    free(p);
		}
	    }
	    goto ack;
	}
    }

    Syslog('+', "Pid %s was not connected to chatserver", pid);
    snprintf(buf, 200, "100:2,1,*** ERROR - Not connected to server;");
    free(msg);
    free(mbuf);
    return;

ack:
    snprintf(buf, 200, "100:0;");
    free(msg);
    free(mbuf);
    return;

hangup:
    snprintf(buf, 200, "100:2,1,Disconnecting;");
    free(msg);
    free(mbuf);
    return;
}



/*
 * Check for a message for the user. Return the message or signal that
 * nothing is there to display.
 */
void chat_get_r(char *data, char *buf)
{
    char    *p;
    pid_t   pid;
    int	    i;
    
    if (IsSema((char *)"upsalarm")) {
	snprintf(buf, 200, "100:2,1,*** Power failure, running on UPS;");
	return;
    }

    if (s_bbsopen == FALSE) {
	snprintf(buf, 200, "100:2,1,*** The BBS is closed now;");
	return;
    }

    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ";"));

    for (i = 0; i < MAXIBC_USR; i++) {
	if (pid == usr_list[i].pid) {
	    while (usr_list[i].pointer != buffer_head) {
		if (usr_list[i].pointer < MAXMESSAGES)
		    usr_list[i].pointer++;
		else
		    usr_list[i].pointer = 0;
		if (usr_list[i].pid == chat_messages[usr_list[i].pointer].topid) {
		    /*
		     * Message is for us
		     */
		    p = xstrcpy((char *)"100:2,0,");
		    p = xstrcat(p, clencode(chat_messages[usr_list[i].pointer].message));
		    p = xstrcat(p, (char *)";");
		    strncpy(buf, p, 200);
		    free(p);
		    return;
		}
	    }
	    snprintf(buf, 200, "100:0;");
	    return;
	}
    }
    snprintf(buf, 200, "100:2,1,*** ERROR - Not connected to server;");
    return;
}



/*
 * Check for sysop present for forced chat
 */
void chat_checksysop_r(char *data, char *buf)
{
    pid_t   pid;
    int	    i;
    
    strtok(data, ",");
    pid = (pid_t)atoi(strtok(NULL, ";"));

    if (reg_ispaging(pid)) {
	Syslog('c', "Check sysopchat for pid %d, user has paged", pid);

        /*
         * Now check if sysop is present in the sysop channel
         */
	for (i = 0; i < MAXIBC_USR; i++) {
	    if (pid != usr_list[i].pid) {
	        if (strlen(usr_list[i].channel) && (strcasecmp(usr_list[i].channel, "#sysop") == 0) && usr_list[i].sysop) {
		    Syslog('c', "Sending ACK on check");
		    snprintf(buf, 20, "100:1,1;");
		    reg_sysoptalk(pid);
		    return;
		}
	    }
	}
    }

    snprintf(buf, 20, "100:1,0;");
    return;
}


