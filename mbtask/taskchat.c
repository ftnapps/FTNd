/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: mbtask - chat server
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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
#include "libs.h"
#include "../lib/structs.h"
#include "taskutil.h"
#include "taskregs.h"
#include "taskchat.h"

#define	MAXCHANNELS 10		    /* Maximum chat channels		*/
#define	MAXMESSAGES 100		    /* Maximum ringbuffer for messages	*/


typedef enum {CH_FREE, CH_PRIVATE, CH_PUBLIC} CHANNELTYPE;


/*
 *  Users connected to the chatserver.
 */
typedef struct _ch_user_rec {
    pid_t	pid;		    /* User's pid			*/
    char	name[36];	    /* His name used (may become nick)	*/
    time_t	connected;	    /* Time connected			*/
    int		channel;	    /* Connected channel or -1		*/
    int		pointer;	    /* Message pointer			*/
    unsigned	chatting    : 1;    /* Is chatting in a channel		*/
    unsigned	chanop	    : 1;    /* Is a chanop			*/
} _chat_users;



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
 * List of channels
 */
typedef struct _channel_rec {
    char	name[21];	    /* Channel name			*/
    pid_t	owner;		    /* Channel owner			*/
    int		users;		    /* Users in channel			*/
    time_t	created;	    /* Creation time			*/
    unsigned	active	    : 1;    /* Channel active			*/
} _channel;



/*
 *  List of banned users from a channel. This is a dynamic list.
 */
typedef struct	_banned {
    int		channel;	    /* Channel the user is banned from	*/
    char	user[36];	    /* The user who is banned		*/
} banned_users;



/*
 *  The buffers
 */
_chat_messages		chat_messages[MAXMESSAGES];
_chat_users		chat_users[MAXCLIENT];
_channel		chat_channels[MAXCHANNELS];


int			buffer_head = 0;    /* Messages buffer head	*/
extern struct sysconfig CFG;		    /* System configuration	*/


/*
 * Prototypes
 */
void chat_msg(int, char *, char *);




void chat_dump(void)
{
    int	    i;
    
    for (i = 0; i < MAXCLIENT; i++)
	if (chat_users[i].pid)
	    Syslog('u', "%5d %-36s %2d %s", chat_users[i].pid, chat_users[i].name, chat_users[i].channel,
		chat_users[i].chatting?"True":"False");
    for (i = 0; i < MAXCHANNELS; i++)
	if (chat_channels[i].owner)
	    Syslog('c', "%-20s %5d %3d %s", chat_channels[i].name, chat_channels[i].owner, chat_channels[i].users,
		    chat_channels[i].active?"True":"False");
}



/*
 * Put a system message into the chatbuffer
 */
void system_msg(pid_t, char *);
void system_msg(pid_t pid, char *msg)
{
    if (buffer_head < MAXMESSAGES)
	buffer_head++;
    else
	buffer_head = 0;

    Syslog('-', "system_msg(%d, %s) ptr=%d", pid, msg, buffer_head);
    memset(&chat_messages[buffer_head], 0, sizeof(_chat_messages));
    chat_messages[buffer_head].topid = pid;
    sprintf(chat_messages[buffer_head].fromname, "Server");
    strncpy(chat_messages[buffer_head].message, msg, 80);
    chat_messages[buffer_head].posted = time(NULL);
}



/*
 * Show help
 */
void chat_help(pid_t);
void chat_help(pid_t pid)
{
    system_msg(pid, (char *)"Topics available:");
    system_msg(pid, (char *)"");
    system_msg(pid, (char *)"    EXIT        HELP");
}



/*
 * Join a channel
 */
int join(pid_t, char *);
int join(pid_t pid, char *channel)
{
    int	    i, j;
    char    buf[81];

    Syslog('-', "Join pid %d to channel %s", pid, channel);
    for (i = 0; i < MAXCHANNELS; i++) {
	if (strcasecmp(chat_channels[i].name, channel) == 0) {
	    /*
	     * Excisting channel, add user to channel.
	     */
	    chat_channels[i].users++;
	    for (j = 0; j < MAXCLIENT; j++) {
		if (chat_users[j].pid == pid) {
		    chat_users[j].channel = i;
		    chat_users[j].chatting = TRUE;
		    Syslog('-', "Added user %d to channel %d", j, i);
		    chat_dump();
		    sprintf(buf, "%s has joined channel #%s, now %d users", chat_users[j].name, channel, chat_channels[i].users);
		    chat_msg(i, NULL, buf);
		    return TRUE;
		}
	    }
	}
    }

    /*
     * No matching channel found, add a new channel.
     */
    for (i = 0; i < MAXCHANNELS; i++) {
	if (chat_channels[i].active == FALSE) {
	    /*
	     * Got one, register channel.
	     */
	    strncpy(chat_channels[i].name, channel, 20);
	    chat_channels[i].owner = pid;
	    chat_channels[i].users = 1;
	    chat_channels[i].created = time(NULL);
	    chat_channels[i].active = TRUE;
	    Syslog('-', "Created channel %d", i);
	    /*
	     * Register user to channel
	     */
	    for (j = 0; j < MAXCLIENT; j++) {
		if (chat_users[j].pid == pid) {
		    chat_users[j].channel = i;
		    chat_users[j].chatting = TRUE;
		    Syslog('-', "Added user %d to channel %d", j, i);
		    sprintf(buf, "Created channel #%s", channel);
		    chat_msg(i, NULL, buf);
		}
	    }
	    chat_dump();
	    return TRUE;
	}
    }

    /*
     * No matching or free channels
     */
    Syslog('+', "Cannot create chat channel %s, no free channels", channel);
    return FALSE;
}



/*
 * Part from a channel
 */
int part(pid_t, char*);
int part(pid_t pid, char *reason)
{
    int	    i;
    char    buf[81];

    Syslog('-', "Part pid %d from channel, reason %s", pid, reason);

    for (i = 0; i < MAXCLIENT; i++) {
	if ((chat_users[i].pid == pid) && chat_users[i].chatting) {
	    chat_channels[chat_users[i].channel].users--;

	    /*
	     * Inform other users
	     */
	    if (reason != NULL)
		chat_msg(chat_users[i].channel, chat_users[i].name, reason);
	    sprintf(buf, "%s has left channel #%s, %d users left", chat_users[i].name, chat_channels[chat_users[i].channel].name,
		    chat_channels[chat_users[i].channel].users);
	    chat_msg(chat_users[i].channel, NULL, buf);
	    
	    /*
	     * First clean channel
	     */
	    Syslog('-', "User leaves channel %s", chat_channels[chat_users[i].channel].name);
	    if (chat_channels[chat_users[i].channel].users == 0) {
		/*
		 * Last user from channel, clear channel
		 */
		Syslog('-', "Remove channel %s, no more users left", chat_channels[chat_users[i].channel].name);
		memset(&chat_channels[chat_users[i].channel], 0, sizeof(_channel));
	    }
	    chat_users[i].channel = -1;
	    chat_users[i].chatting = FALSE;
	    chat_dump();
	    return TRUE;
	}
    }
    Syslog('-', "No channel found");
    return FALSE;
}



void chat_init(void)
{
    memset(&chat_users, 0, sizeof(chat_users));
    memset(&chat_messages, 0, sizeof(chat_messages));
    memset(&chat_channels, 0, sizeof(chat_channels));
}



void chat_cleanuser(pid_t pid)
{
    part(pid, (char *)"I'm hanging up!");
}



/*
 * Send message into channel
 */
void chat_msg(int channel, char *nick, char *msg)
{
    int	    i;
    char    buf[128];

    if (nick == NULL)
	sprintf(buf, "%s", msg);
    else
	sprintf(buf, "<%s> %s", nick, msg);
    buf[79] = '\0';

    for (i = 0; i < MAXCLIENT; i++) {
	if ((chat_users[i].channel == channel) && chat_users[i].chatting) {
	    system_msg(chat_users[i].pid, buf);
	}
    }
}



/*
 * Connect a session to the chatserver.
 */
char *chat_connect(char *data)
{
    char	*pid, *usr;
    static char buf[200];
    int		i, j, count = 0;

    Syslog('-', "CCON:%s", data);
    memset(&buf, 0, sizeof(buf));

    /*
     * Search free userslot
     */
    for (i = 0; i < MAXCLIENT; i++) {
	if (chat_users[i].pid == 0) {
	    /*
	     * Oke, found
	     */
	    pid = strtok(data, ",");	/* Should be 2	    */
	    pid = strtok(NULL, ",");	/* The pid	    */
	    usr = strtok(NULL, ";");	/* Username	    */
	    chat_users[i].pid = atoi(pid);
	    strncpy(chat_users[i].name, usr, 36);
	    chat_users[i].connected = time(NULL);
	    chat_users[i].pointer = buffer_head;
	    chat_users[i].channel = -1;

	    Syslog('-', "Connected user %s (%s) with chatserver, slot %d", usr, pid, i);

	    /*
	     * Now put welcome message into the ringbuffer and report success.
	     */
	    sprintf(buf, "MBSE BBS v%s chat server; type /help for help", VERSION);
	    system_msg(chat_users[i].pid, buf);
	    sprintf(buf, "Welcome to the %s chat network", CFG.bbs_name);
	    system_msg(chat_users[i].pid, buf);
	    for (j = 0; j < MAXCLIENT; j++)
		if (chat_users[j].pid)
		    count++;
	    sprintf(buf, "There %s %d user%s connected", (count != 1)?"are":"is", count, (count != 1)?"s":"");
	    system_msg(chat_users[i].pid, buf);

	    sprintf(buf, "100:0;");
	    return buf;
	}
    }
    sprintf(buf, "100:1,Too many users connected;");
    return buf;
}



char *chat_close(char *data)
{
    static char buf[200];
    char	*pid;
    int		i;

    Syslog('-', "CCLO:%s", data);
    memset(&buf, 0, sizeof(buf));
    pid = strtok(data, ",");
    pid = strtok(NULL, ";");
    
    for (i = 0; i < MAXCLIENT; i++) {
	if (chat_users[i].pid == atoi(pid)) {
	    Syslog('-', "Closing chat for pid %s, slot %d", pid, i);
	    memset(&chat_users[i], 0, sizeof(_chat_users));
	    sprintf(buf, "100:0;");
	    return buf;
	}
    }
    Syslog('-', "Pid %s was not connected to chatserver");
    sprintf(buf, "100:1,ERROR - Not connected to server;");
    return buf;
}



char *chat_put(char *data)
{
    static char buf[200];
    char	*pid, *msg, *cmd;
    int		i;

    Syslog('-', "CPUT:%s", data);
    memset(&buf, 0, sizeof(buf));

    pid = strtok(data, ",");
    pid = strtok(NULL, ",");
    msg = strtok(NULL, "\0");
    msg[strlen(msg)-1] = '\0';

    for (i = 0; i < MAXCLIENT; i++) {
	if (chat_users[i].pid == atoi(pid)) {
	    /*
	     * We are connected and known, first send the input back to ourself.
	     */
	    system_msg(chat_users[i].pid, msg);

	    if (msg[0] == '/') {
		/*
		 * A command, process this
		 */
		if (strncasecmp(msg, "/help", 5) == 0) {
		    chat_help(atoi(pid));
		    goto ack;
		}
		if ((strncasecmp(msg, "/exit", 5) == 0) || 
		    (strncasecmp(msg, "/quit", 5) == 0) ||
		    (strncasecmp(msg, "/bye", 4) == 0)) {
		    part(chat_users[i].pid, (char *)"Quitting");
		    /*
		     * Just send messages, the client should later do a
		     * real disconnect.
		     */
		    sprintf(buf, "Goodbye");
		    system_msg(chat_users[i].pid, buf);
		    goto ack;
		}
		if (strncasecmp(msg, "/join", 5) == 0) {
		    cmd = strtok(msg, " \0");
		    Syslog('-', "\"%s\"", cmd);
		    cmd = strtok(NULL, "\0");
		    Syslog('-', "\"%s\"", cmd);
		    if ((cmd == NULL) || (cmd[0] != '#') || (strcmp(cmd, "#") == 0)) {
			sprintf(buf, "Try /join #channel");
			system_msg(chat_users[i].pid, buf);
		    } else {
			Syslog('-', "Trying to join channel %s", cmd);
			join(chat_users[i].pid, cmd+1);
		    }
		    goto ack;
		}
		if (strncasecmp(msg, "/part", 5) == 0) {
		    cmd = strtok(msg, " \0");
		    Syslog('-', "\"%s\"", cmd);
		    cmd = strtok(NULL, "\0");
		    Syslog('-', "\"%s\"", cmd);
		    if (part(chat_users[i].pid, cmd) == FALSE) {
			sprintf(buf, "Not in a channel");
			system_msg(chat_users[i].pid, buf);
		    }
		    goto ack;
		}
		/*
		 * If still here, the command was not recognized.
		 */
		cmd = strtok(msg, " \t\r\n\0");
		sprintf(buf, "%s :Unknown command", cmd+1);
		system_msg(chat_users[i].pid, buf);
		goto ack;
	    }
	    if (chat_users[i].channel == -1) {
		/*
		 * Trying messages while not in a channel
		 */
		sprintf(buf, "No channel joined. Try /join #channel");
		system_msg(chat_users[i].pid, buf);
		chat_dump();
		goto ack;
	    } else {
		chat_msg(chat_users[i].channel, chat_users[i].name, msg);
		chat_dump();
	    }
	    goto ack;
	}
    }
    Syslog('-', "Pid %s was not connected to chatserver");
    sprintf(buf, "100:1,ERROR - Not connected to server;");
    return buf;

ack:
    sprintf(buf, "100:0;");
    return buf;
}



char *chat_get(char *data)
{
    static char buf[200];
    char	*pid;
    int		i;

//    Syslog('-', "CGET:%s", data);
    memset(&buf, 0, sizeof(buf));
    pid = strtok(data, ",");
    pid = strtok(NULL, ";");

    for (i = 0; i < MAXCLIENT; i++) {
	if (atoi(pid) == chat_users[i].pid) {
	    while (chat_users[i].pointer != buffer_head) {
		if (chat_users[i].pointer < MAXMESSAGES)
		    chat_users[i].pointer++;
		else
		    chat_users[i].pointer = 0;
		if (chat_users[i].pid == chat_messages[chat_users[i].pointer].topid) {
		    /*
		     * Message is for us.
		     */
		    sprintf(buf, "100:1,%s;", chat_messages[chat_users[i].pointer].message);
		    Syslog('-', "%s", buf);
		    return buf;
		}
	    }
	    sprintf(buf, "100:0;");
	    return buf;
	}
    }
    sprintf(buf, "100:1,ERROR - Not connected to server;");
    return buf;
}


