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

#define	MAXCHANNELS 100		    /* Maximum chat channels		*/
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
 *  List of banned users from a channel. This is a dynamic list.
 */
typedef struct	_banned {
    int		channel;	    /* Channel the user is banned from	*/
    char	user[36];	    /* The user who is banned		*/
} banned_users;


/*
 *  The buffers
 */
_chat_messages	chat_messages[MAXMESSAGES];
_chat_users	chat_users[MAXCLIENT];


int			buffer_head = 0;    /* Messages buffer head	*/
extern struct sysconfig CFG;		    /* System configuration	*/



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



void chat_init(void)
{
    memset(&chat_users, 0, sizeof(chat_users));
    memset(&chat_messages, 0, sizeof(chat_messages));
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
		    sprintf(buf, "100:0;");
		    return buf;
		}
		if (strncasecmp(msg, "/exit", 5) == 0) {
		    /*
		     * Just send messages, the client should later do a
		     * real disconnect.
		     */
		    sprintf(buf, "Goodbye");
		    system_msg(chat_users[i].pid, buf);
		    sprintf(buf, "100:0;");
		    return buf;
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
		    }
		    sprintf(buf, "100:0;");
		    return buf;
		}
		/*
		 * If still here, the command was not recognized.
		 */
		cmd = strtok(msg, " \t\r\n\0");
		sprintf(buf, "%s :Unknown command", cmd+1);
		system_msg(chat_users[i].pid, buf);
		sprintf(buf, "100:0;");
		return buf;
	    }
	    if (chat_users[i].channel == -1) {
		/*
		 * Trying messages while not in a channel
		 */
		sprintf(buf, "No channel joined. Try /join #channel");
		system_msg(chat_users[i].pid, buf);
		sprintf(buf, "100:0;");
		return buf;
	    }
	    sprintf(buf, "100:0;");
	    return buf;
	}
    }
    Syslog('-', "Pid %s was not connected to chatserver");
    sprintf(buf, "100:1,ERROR - Not connected to server;");
    return buf;
}



char *chat_get(char *data)
{
    static char buf[200];
    char	*pid;
    int		i;

    Syslog('-', "CGET:%s", data);
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


