/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: MBSE NNTP Server
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
 *   
 * Michiel Broek		FIDO:		2:280/2802
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
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "../lib/msg.h"
#include "openport.h"
#include "ttyio.h"
#include "auth.h"
#include "commands.h"
#include "mbnntp.h"

time_t		    t_start;
time_t		    t_end;
char		    *envptr = NULL;
struct sockaddr_in  peeraddr;
pid_t		    mypid;
unsigned long	    rcvdbytes = 0L;
unsigned long	    sentbytes = 0L;

extern char	    *ttystat[];
extern int	    authorized;


void die(int onsig)
{
    signal(onsig, SIG_IGN);
    CloseDupes();
    Msg_Close();

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    WriteError("Terminated with error %d", onsig);
    }

    t_end = time(NULL);
    Syslog('+', "Send [%6lu] Received [%6lu]", sentbytes, rcvdbytes);
    Syslog(' ', "MBNNTP finished in %s", t_elapsed(t_start, t_end));

    if (envptr)
	free(envptr);

    ExitClient(onsig);
}



int main(int argc, char *argv[])
{
    struct passwd   *pw;
    int		    i, rc, addrlen = sizeof(struct sockaddr_in);

    /*
     * The next trick is to supply a fake environment variable
     * MBSE_ROOT because this program is started from inetd.
     * This will setup the variable so InitConfig() will work.
     * The /etc/passwd must point to the correct homedirectory.
     */
    pw = getpwuid(geteuid());
    if (getenv("MBSE_ROOT") == NULL) {
	envptr = xstrcpy((char *)"MBSE_ROOT=");
	envptr = xstrcat(envptr, pw->pw_dir);
	putenv(envptr);
    }
    mypid = getpid();

    /*
     * Read the global configuration data, registrate connection
     */
    InitConfig();
    InitMsgs();
    InitUser();
    InitFidonet();
    umask(002);
    memset(&usrconfig, 0, sizeof(usrconfig));

    t_start = time(NULL);
    InitClient(pw->pw_name, (char *)"mbnntp", CFG.location, CFG.logfile, 
	    CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);
    Syslog(' ', "MBNNTP v%s", VERSION);
    IsDoing("Loging in");

    /*
     * Catch all the signals we can, and ignore the rest.
     */
    for(i = 0; i < NSIG; i++) {

	if ((i == SIGINT) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM))
	    signal(i, (void (*))die);
	else if (i == SIGCHLD)
	    signal(i, SIG_DFL);
	else if ((i != SIGKILL) && (i != SIGSTOP))
	    signal(i, SIG_IGN);
    }

    if ((rc = rawport()) != 0)
	WriteError("Unable to set raw mode");
    else {
	if (getpeername(0,(struct sockaddr*)&peeraddr,&addrlen) == 0) {
	    Syslog('s', "TCP connection: len=%d, family=%hd, port=%hu, addr=%s",
		    addrlen,peeraddr.sin_family, peeraddr.sin_port, inet_ntoa(peeraddr.sin_addr));
	    Syslog('+', "Incoming connection from %s", inet_ntoa(peeraddr.sin_addr));
	    send_nntp("200 MBNNTP v%s server ready -- no posting allowed", VERSION);
	    nntp();
	}
    }

    cookedport();

    die(0);
    return 0;
}



/*
 * Get command from the client.
 * return  < 0: error
 * return >= 0: size of buffer
 */
int get_nntp(char *buf, int max)
{
    int	    c, len;

    len = 0;
    memset(buf, 0, sizeof(buf));
    while (TRUE) {
	c = tty_getc(180);
	if (c <= 0) {
	    if (c == -2) {
		/*
		 * Timeout
		 */
		send_nntp("400 Service discontinued");
	    }
	    Syslog('+', "Receiver status %s", ttystat[- c]);
	    return c;
	}
	if ((c == '\r') || (c == '\n')) {
	    rcvdbytes += (len + 1);
	    return len;
	}
	else {
	    buf[len] = c;
	    len++;
	    buf[len] = '\0';
	}
	if (len >= max) {
	    WriteError("Input buffer full");
	    return len;
	}
    }

    return 0;	    /* Not reached */
}



void send_nntp(const char *format, ...)
{
    char    *out;
    va_list va_ptr;

    out = calloc(4096, sizeof(char));

    va_start(va_ptr, format);
    vsprintf(out, format, va_ptr);
    va_end(va_ptr);

    Syslog('n', "> \"%s\"", printable(out, 0));
    PUTSTR(out);
    PUTSTR((char *)"\r\n");
    FLUSHOUT();
    sentbytes += (strlen(out) + 2);
    free(out);
}



void nntp(void)
{
    char    buf[4096];
    int	    len;
    
    while (TRUE) {

	IsDoing("Waiting");
	len = get_nntp(buf, sizeof(buf) -1);
	if (len < 0)
	    return;
	if (len == 0)
	    continue;

	Syslog('n', "< \"%s\"", printable(buf, 0));

	/*
	 * Process received command
	 */
	if (strncasecmp(buf, "QUIT", 4) == 0) {
	    send_nntp("205 Goodbye\r\n");
	    return;
	} else if (strncasecmp(buf, "AUTHINFO USER", 13) == 0) {
	    auth_user(buf);
	} else if (strncasecmp(buf, "AUTHINFO PASS", 13) == 0) {
	    auth_pass(buf);
	} else if (strncasecmp(buf, "ARTICLE", 7) == 0) {
	    if (check_auth(buf))
		command_abhs(buf);
	} else if (strncasecmp(buf, "BODY", 4) == 0) {
	    if (check_auth(buf))
		command_abhs(buf);
	} else if (strncasecmp(buf, "LIST", 4) == 0) {
	    if (check_auth(buf))
		command_list(buf);
	} else if (strncasecmp(buf, "GROUP", 5) == 0) {
	    if (check_auth(buf))
		command_group(buf);
	} else if (strncasecmp(buf, "HEAD", 4) == 0) {
	    if (check_auth(buf))
		command_abhs(buf);
	} else if (strncasecmp(buf, "IHAVE", 5) == 0) {
	    send_nntp("435 Article not wanted - do not send it");
	} else if (strncasecmp(buf, "NEWGROUPS", 9) == 0) {
	    send_nntp("235 Warning: NEWGROUPS not implemented, returning empty list");
	    send_nntp(".");
	} else if (strncasecmp(buf, "NEWNEWS", 7) == 0) {
	    send_nntp("230 Warning: NEWNEWS not implemented, returning empty list");
	    send_nntp(".");
	} else if (strncasecmp(buf, "SLAVE", 5) == 0) {
	    send_nntp("202 Slave status noted");
	} else if (strncasecmp(buf, "STAT", 4) == 0) {
	    if (check_auth(buf))
		command_abhs(buf);
	} else if (strncasecmp(buf, "MODE READER", 11) == 0) {
	    if (check_auth(buf)) {
		if (authorized)
		    send_nntp("200 Server ready, posting allowed");
		else
		    send_nntp("201 Server ready, no posting allowed");
	    }
	} else if (strncasecmp(buf, "XOVER", 5) == 0) {
	    if (check_auth(buf))
		command_xover(buf);
	} else if (strncasecmp(buf, "HELP", 4) == 0) {
	    send_nntp("100 Help text follows");
	    send_nntp("Recognized commands:");
	    send_nntp("");
	    send_nntp("ARTICLE");
	    send_nntp("AUTHINFO");
	    send_nntp("BODY");
	    send_nntp("GROUP");
	    send_nntp("HEAD");
	    send_nntp("IHAVE (not implemented, messages are always rejected)");
	    send_nntp("LIST");
	    send_nntp("NEWGROUPS (not implemented, always returns an empty list)");
	    send_nntp("NEWNEWS (not implemented, always returns an empty list)");
	    send_nntp("QUIT");
	    send_nntp("SLAVE (has no effect)");
	    send_nntp("STAT");
	    send_nntp("XOVER");
	    send_nntp("");
	    send_nntp("MBNNTP supports most of RFC-977 and also has support for AUTHINFO and");
	    send_nntp("limited XOVER support (RFC-2980)");
	    send_nntp(".");
	} else {
	    send_nntp("500 Unknown command");
	}
    }
}


