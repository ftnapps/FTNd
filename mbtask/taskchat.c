/*****************************************************************************
 *
 * $Id$
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
extern srv_list		*servers;	    /* Connected servers	*/
extern usr_list		*users;		    /* Connected users		*/
extern chn_list		*channels;	    /* Connected channels	*/
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
    int		first;
    usr_list	*tmpu;
    
    first = TRUE;
    for (tmpu = users; tmpu; tmpu = tmpu->next) {
	if (tmpu->pid) {
	    if (first) {
		Syslog('c', "  pid username                             nick      channel              sysop");
		Syslog('c', "----- ------------------------------------ --------- -------------------- -----");
		first = FALSE;
	    }
	    Syslog('c', "%5d %-36s %-9s %-20s %s", tmpu->pid, tmpu->realname, tmpu->nick,
		tmpu->channel, tmpu->sysop?"True ":"False");
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
    usr_list	*tmpu;

    buf = calloc(512, sizeof(char));
    va_start(va_ptr, format);
    vsnprintf(buf, 512, format, va_ptr);
    va_end(va_ptr);

    for (tmpu = users; tmpu; tmpu = tmpu->next)
	if (tmpu->pid) {
	    system_msg(tmpu->pid, buf);
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
    chn_list	*tmp;
    usr_list	*tmpu;

    Syslog('c', "Join pid %d to channel %s", pid, channel);

    if (channels) {
	for (tmp = channels; tmp; tmp = tmp->next) {
	    if (strcmp(tmp->name, channel) == 0) {
		for (tmpu = users; tmpu; tmpu = tmpu->next) {
		    if (tmpu->pid == pid) {

			strncpy(tmpu->channel, channel, 20);
			tmp->users++;
			Syslog('+', "IBC: user %s has joined channel %s", tmpu->nick, channel);
			usrchg = TRUE;
			srvchg = TRUE;
			chnchg = TRUE;

			chat_dump();
			snprintf(buf, 81, "%s has joined channel %s, now %d users", tmpu->nick, channel, tmp->users);
			chat_msg(channel, NULL, buf);

			/*
			 * The sysop channel is private to the system, no broadcast
			 */
			if (strcasecmp(channel, "#sysop"))
			    send_at((char *)"JOIN", tmpu->nick, channel);
			return TRUE;
		    }
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
    for (tmpu = users; tmpu; tmpu = tmpu->next) {
	if (tmpu->pid == pid) {
	    if (add_channel(&channels, channel, tmpu->nick, CFG.myfqdn) == 0) {

		strncpy(tmpu->channel, channel, 20);
		Syslog('+', "IBC: user %s created and joined channel %s", tmpu->nick, channel);
		usrchg = TRUE;
		chnchg = TRUE;
		srvchg = TRUE;

		snprintf(buf, 81, "* Created channel %s", channel);
		chat_msg(channel, NULL, buf);
		chat_dump();
		if (strcasecmp(channel, "#sysop"))
		    send_at((char *)"JOIN", tmpu->nick, channel);

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
    char	buf[81], *p;
    chn_list	*tmp;
    usr_list	*tmpu;

    if (strlen(reason) > 54)
	reason[54] = '\0';

    Syslog('c', "Part pid %d from channel, reason %s", pid, reason);

    for (tmpu = users; tmpu; tmpu = tmpu->next) {
	if ((tmpu->pid == pid) && strlen(tmpu->channel)) {
	    for (tmp = channels; tmp; tmp = tmp->next) {
		if (strcmp(tmp->name, tmpu->channel) == 0) {
		    /*
		     * Inform other users
		     */
		    if (reason != NULL) {
			chat_msg(tmpu->channel, tmpu->nick, reason);
		    }
		    snprintf(buf, 81, "%s has left channel %s, %d users left", tmpu->nick, tmp->name, tmp->users -1);
		    chat_msg(tmpu->channel, NULL, buf);
		    if (strcasecmp(tmp->name, (char *)"#sysop")) {
			p = xstrcpy(tmp->name);
			if (reason && strlen(reason)) {
			    p = xstrcat(p, (char *)" ");
			    p = xstrcat(p, reason);
			}
			send_at((char *)"PART", tmpu->nick, p);
			free(p);
		    }

		    /*
		     * Clean channel
		     */
		    if (tmp->users > 0)
			tmp->users--;
		    Syslog('+', "IBC: nick %s leaves channel %s, %d user left", tmpu->nick, tmp->name, tmp->users);
		    if (tmp->users == 0) {
			/*
			 * Last user in channel, remove channel
			 */
			snprintf(buf, 81, "* Removed channel %s", tmp->name);
			system_msg(pid, buf);
			Syslog('+', "IBC: removed channel %s, no more users left", tmp->name);
			del_channel(&channels, tmp->name);
		    }

		    /*
		     * Update user data
		     */
		    tmpu->channel[0] = '\0';
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
    char	*p;
    usr_list	*tmpu;

    if (nick == NULL)
	p = xstrcpy(msg);
    else {
	p = xstrcpy((char *)"<");
	p = xstrcat(p, nick);
	p = xstrcat(p, (char *)"> ");
	p = xstrcat(p, msg);
    }
    Chatlog((char *)"+", channel, p);

    for (tmpu = users; tmpu; tmpu = tmpu->next) {
	if (strlen(tmpu->channel) && (strcmp(tmpu->channel, channel) == 0)) {
	    system_msg(tmpu->pid, p);
	}
    }

    free(p);
}



/*
 * Connect a session to the chatserver.
 */
void chat_connect_r(char *data, char *buf)
{
    char	*pid, *realname, *nick, *temp;
    int		count = 0, sys = FALSE;
    srv_list	*sl;
    usr_list	*tmpu;

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
    pid = strtok(data, ",");			    /* Should be 3  */
    pid = strtok(NULL, ",");			    /* The pid      */
    realname = xstrcpy(cldecode(strtok(NULL, ",")));/* Username     */
    nick = xstrcpy(cldecode(strtok(NULL, ",")));    /* Nickname     */
    sys = atoi(strtok(NULL, ";"));		    /* Sysop flag   */

    add_user(&users, CFG.myfqdn, nick, realname);
    send_at((char *)"USER", nick, realname);

    /*
     * Now search the added entry to update the data
     */
    for (tmpu = users; tmpu; tmpu = tmpu->next) {
	if ((strcmp(tmpu->server, CFG.myfqdn) == 0) && (strcmp(tmpu->name, nick) == 0) && (strcmp(tmpu->realname, realname) == 0)) {
	    /*
	     * Oke, found
	     */
	    tmpu->pid = atoi(pid);
	    tmpu->pointer = buffer_head;
	    tmpu->sysop = sys;
	    usrchg = TRUE;
	    srvchg = TRUE;
	    Syslog('c', "Connected user %s (%s) with chatserver, sysop %s", realname, pid, sys ? "True":"False");

            /*
	     * Now put welcome message into the ringbuffer and report success.
	     */
	    temp = calloc(81, sizeof(char));
	    snprintf(temp, 200, "MBSE BBS v%s chat server; type /help for help", VERSION);
	    system_msg(tmpu->pid, temp);
	    snprintf(temp, 200, "Welcome to the Internet BBS Chat Network");
	    system_msg(tmpu->pid, temp);
	    snprintf(temp, 200, "Current connected servers:");
	    system_msg(tmpu->pid, temp);
	    for (sl = servers; sl; sl = sl->next) {
		snprintf(temp, 200, "  %s (%d user%s)", sl->fullname, sl->users, (sl->users == 1) ? "":"s");
		system_msg(tmpu->pid, temp);
		count += sl->users;
	    }
	    snprintf(temp, 200, "There %s %d user%s connected", (count != 1)?"are":"is", count, (count != 1)?"s":"");
	    system_msg(tmpu->pid, temp);
	    snprintf(buf, 200, "100:0;");
	    free(realname);
	    free(nick);
	    free(temp);
	    return;
	}
    }

    free(realname);
    free(nick);
    snprintf(buf, 200, "100:1,Too many users connected;");
    return;
}



void chat_close_r(char *data, char *buf)
{
    char	*pid;
    usr_list	*tmpu;

    Syslog('c', "CCLO:%s", data);
    pid = strtok(data, ",");
    pid = strtok(NULL, ";");
 
    for (tmpu = users; tmpu; tmpu = tmpu->next) {
	if (tmpu->pid == atoi(pid)) {
	    /*
	     * Remove from IBC network
	     */
	    send_at((char *)"QUIT", tmpu->name, (char *)"Leaving chat");
	    del_user(&users, CFG.myfqdn, tmpu->name);
	    Syslog('c', "Closing chat for pid %s", pid);
	    snprintf(buf, 81, "100:0;");
	    return;
	}
    }

    Syslog('c', "Pid %s was not connected to chatserver");
    snprintf(buf, 81, "100:1,*** ERROR - Not connected to server;");
    return;
}



void chat_put_r(char *data, char *buf)
{
    char	*p, *pid, *msg, *cmd, *mbuf, *flags, temp[81];
    int		first, count, owner = FALSE, found;
    usr_list	*tmpu, *tmp;
    chn_list	*tmpc;

    if (IsSema((char *)"upsalarm")) {
	snprintf(buf, 81, "100:2,1,*** Power alarm, running on UPS;");
	return;
    }

    if (s_bbsopen == FALSE) {
	snprintf(buf, 81, "100:2,1,*** The BBS is closed now;");
	return;
    }
	
    pid = strtok(data, ",");
    pid = strtok(NULL, ",");
    msg = xstrcpy(cldecode(strtok(NULL, ";")));
    mbuf = calloc(200, sizeof(char));

    for (tmpu = users; tmpu; tmpu = tmpu->next) {
	if (tmpu->pid == atoi(pid)) {
	    if (msg[0] == '/') {
		/*
		 * A command, process this but first se if we are in a channel
		 * and own the channel. This gives us more power.
		 */
		if (strlen(tmpu->channel)) {
		    for (tmpc = channels; tmpc; tmpc = tmpc->next) {
			if (((strcmp(tmpu->nick, tmpc->owner) == 0) || (strcmp(tmpu->name, tmpc->owner) == 0)) &&
			    (strcmp(tmpu->server, tmpc->server) == 0)) {
			    owner = TRUE;
			}
		    }
		    Syslog('c', "IBC: process command, in channel %s and we are %sthe owner", tmpu->channel, owner ? "":"not ");
		}
		if (strncasecmp(msg, "/help", 5) == 0) {
		    chat_help(atoi(pid), owner);
		    goto ack;
		} else if (strncasecmp(msg, "/echo", 5) == 0) {
		    snprintf(mbuf, 200, "%s", msg);
		    system_msg(tmpu->pid, mbuf);
		    goto ack;
		} else if ((strncasecmp(msg, "/exit", 5) == 0) || 
		    (strncasecmp(msg, "/quit", 5) == 0) ||
		    (strncasecmp(msg, "/bye", 4) == 0)) {
		    if (strlen(tmpu->channel)) {
			/*
			 * If in a channel, leave channel first
			 */
			part(tmpu->pid, (char *)"Quitting");
			snprintf(mbuf, 81, "Goodbye");
			system_msg(tmpu->pid, mbuf);
		    }
		    goto hangup;
		} else if ((strncasecmp(msg, "/join", 5) == 0) ||
		    (strncasecmp(msg, "/j ", 3) == 0)) {
		    cmd = strtok(msg, " \0");
		    cmd = strtok(NULL, "\0");
		    if ((cmd == NULL) || (cmd[0] != '#') || (strcmp(cmd, "#") == 0)) {
			snprintf(mbuf, 200, "** Try /join #channel");
			system_msg(tmpu->pid, mbuf);
		    } else if (strlen(tmpu->channel)) {
			snprintf(mbuf, 200, "** Cannot join while in a channel");
			system_msg(tmpu->pid, mbuf);
		    } else {
			join(tmpu->pid, cmd, tmpu->sysop);
		    }
		    chat_dump();
		    goto ack;
		} else if (strncasecmp(msg, "/list", 5) == 0) {
		    first = TRUE;
		    for (tmpc = channels; tmpc; tmpc = tmpc->next) {
			if (first) {
			    snprintf(mbuf, 200, "Cnt Channel name         Channel topic");
			    system_msg(tmpu->pid, mbuf);
			    snprintf(mbuf, 200, "--- -------------------- ------------------------------------------------------");
			    system_msg(tmpu->pid, mbuf);
			}
			first = FALSE;
			snprintf(mbuf, 200, "%3d %-20s %-54s", tmpc->users, tmpc->name, tmpc->topic);
			system_msg(tmpu->pid, mbuf);
		    }
		    if (first) {
			snprintf(mbuf, 200, "No active channels to list");
			system_msg(tmpu->pid, mbuf);
		    }
		    goto ack;
		} else if (strncasecmp(msg, "/names", 6) == 0) {
		    if (strlen(tmpu->channel)) {
			snprintf(mbuf, 200, "Present in channel %s:", tmpu->channel);
			system_msg(tmpu->pid, mbuf);
			snprintf(mbuf, 200, "Nick                                     Real name                      Flags");
			system_msg(tmpu->pid, mbuf);
			snprintf(mbuf, 200, "---------------------------------------- ------------------------------ -----");
			system_msg(tmpu->pid, mbuf);
			count = 0;
			for (tmp = users; tmp; tmp = tmp->next) {
			    if (strcmp(tmp->channel, tmpu->channel) == 0) {
				/*
				 * Get channel flags
				 */
				flags = xstrcpy((char *)"--");
				for (tmpc = channels; tmpc; tmpc = tmpc->next) {
				    if (strcmp(tmpc->name, tmpu->channel) == 0) {
					if (((strcmp(tmpc->owner, tmp->nick) == 0) || (strcmp(tmpc->owner, tmp->name) == 0)) &&
					    (strcmp(tmpc->server, tmp->server) == 0)) {
					    flags[0] = 'O';
					}
				    }
				}
				if (tmp->sysop)
				    flags[1] = 'S';

				snprintf(temp, 81, "%s@%s", tmp->nick, tmp->server);
				snprintf(mbuf, 200, "%-40s %-30s %s", temp, tmp->realname, flags);
				system_msg(tmpu->pid, mbuf);
				count++;
				free(flags);
			    }
			}
			snprintf(mbuf, 200, "%d user%s in this channel", count, (count == 1) ?"":"s");
			system_msg(tmpu->pid, mbuf);
		    } else {
			snprintf(mbuf, 200, "** Not in a channel");
			system_msg(tmpu->pid, mbuf);
		    }
		    goto ack;
		} else if (strncasecmp(msg, "/nick", 5) == 0) {
		    cmd = strtok(msg, " \0");
		    cmd = strtok(NULL, "\0");
		    if ((cmd == NULL) || (strlen(cmd) == 0) || (strlen(cmd) > 9)) {
			snprintf(mbuf, 200, "** Nickname must be between 1 and 9 characters");
		    } else {
			found = FALSE;
			for (tmp = users; tmp; tmp = tmp->next) {
			    if ((strcmp(tmp->name, cmd) == 0) || (strcmp(tmp->nick, cmd) == 0)) {
				found = TRUE;
			    }
			}

			if (!found ) {
			    strncpy(tmpu->nick, cmd, 9);
			    snprintf(mbuf, 200, "Nick set to \"%s\"", cmd);
			    system_msg(tmpu->pid, mbuf);
			    send_nick(tmpu->nick, tmpu->name, tmpu->realname);
			    usrchg = TRUE;
			    chat_dump();
			    goto ack;
			}
			snprintf(mbuf, 200, "Can't set nick");
		    }
		    system_msg(tmpu->pid, mbuf);
		    chat_dump();
		    goto ack;
		} else if (strncasecmp(msg, "/part", 5) == 0) {
		    cmd = strtok(msg, " \0");
		    Syslog('c', "\"%s\"", cmd);
		    cmd = strtok(NULL, "\0");
		    Syslog('c', "\"%s\"", printable(cmd, 0));
		    if (part(tmpu->pid, cmd ? cmd : (char *)"Quitting") == FALSE) {
			snprintf(mbuf, 200, "** Not in a channel");
			system_msg(tmpu->pid, mbuf);
		    }
		    chat_dump();
		    goto ack;
		} else if (strncasecmp(msg, "/topic", 6) == 0) {
		    if (strlen(tmpu->channel)) {
			snprintf(mbuf, 200, "** Internal system error");
			for (tmpc = channels; tmpc; tmpc = tmpc->next) {
			    if (strcmp(tmpu->channel, tmpc->name) == 0) {
				if (owner) {
				    cmd = strtok(msg, " \0");
				    cmd = strtok(NULL, "\0");
				    if ((cmd == NULL) || (strlen(cmd) == 0) || (strlen(cmd) > 54)) {
					snprintf(mbuf, 200, "** Topic must be between 1 and 54 characters");
				    } else {
					strncpy(tmpc->topic, cmd, 54);
					snprintf(mbuf, 200, "Topic set to \"%s\"", cmd);
					p = xstrcpy((char *)"TOPIC ");
					p = xstrcat(p, tmpc->name);
					p = xstrcat(p, (char *)" ");
					p = xstrcat(p, tmpc->topic);
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
		    system_msg(tmpu->pid, mbuf);
		    chat_dump();
		    goto ack;
		} else {
		    /*
		     * If still here, the command was not recognized.
		     */
		    cmd = strtok(msg, " \t\r\n\0");
		    snprintf(mbuf, 200, "*** \"%s\" :Unknown command", cmd+1);
		    system_msg(tmpu->pid, mbuf);
		    goto ack;
		}
	    }
	    if (strlen(tmpu->channel) == 0) {
		/*
		 * Trying messages while not in a channel
		 */
		snprintf(mbuf, 200, "** No channel joined. Try /join #channel");
		system_msg(tmpu->pid, mbuf);
		goto ack;
	    } else {
		chat_msg(tmpu->channel, tmpu->nick, msg);
		/*
		 * Send message to all links but not the #sysop channel
		 */
		if (strcmp(tmpu->channel, "#sysop")) {
		    p = xstrcpy((char *)"PRIVMSG ");
		    p = xstrcat(p, tmpu->channel);
		    p = xstrcat(p, (char *)" <");
		    p = xstrcat(p, tmpu->nick);
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
    char	*pid, *p;
    usr_list	*tmpu;

    if (IsSema((char *)"upsalarm")) {
	snprintf(buf, 200, "100:2,1,*** Power failure, running on UPS;");
	return;
    }

    if (s_bbsopen == FALSE) {
	snprintf(buf, 200, "100:2,1,*** The BBS is closed now;");
	return;
    }

    pid = strtok(data, ",");
    pid = strtok(NULL, ";");

    for (tmpu = users; tmpu; tmpu = tmpu->next) {
	if (atoi(pid) == tmpu->pid) {
	    while (tmpu->pointer != buffer_head) {
		if (tmpu->pointer < MAXMESSAGES)
		    tmpu->pointer++;
		else
		    tmpu->pointer = 0;
		if (tmpu->pid == chat_messages[tmpu->pointer].topid) {
		    /*
		     * Message is for us
		     */
		    p = xstrcpy((char *)"100:2,0,");
		    p = xstrcat(p, clencode(chat_messages[tmpu->pointer].message));
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
    char	*pid;
    usr_list	*tmpu;

    pid = strtok(data, ",");
    pid = strtok(NULL, ";");


    if (reg_ispaging(pid)) {
	Syslog('c', "Check sysopchat for pid %s, user has paged", pid);

        /*
         * Now check if sysop is present in the sysop channel
         */
        for (tmpu = users; tmpu; tmpu = tmpu->next) {
	    if (atoi(pid) != tmpu->pid) {
	        if (strlen(tmpu->channel) && (strcasecmp(tmpu->channel, "#sysop") == 0) && tmpu->sysop) {
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


