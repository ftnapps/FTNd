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
#include "openport.h"
#include "ttyio.h"
#include "mbnntp.h"

time_t		t_start;
time_t		t_end;
char		*envptr = NULL;
struct sockaddr_in  peeraddr;


void die(int onsig)
{
    signal(onsig, SIG_IGN);
    CloseDupes();

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    WriteError("Terminated with error %d", onsig);
    }

    t_end = time(NULL);
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


    /*
     * Read the global configuration data, registrate connection
     */
    InitConfig();
    InitMsgs();
    InitUser();
    InitFidonet();
    umask(002);

    t_start = time(NULL);
    InitClient(pw->pw_name, (char *)"mbnntpd", CFG.location, CFG.logfile, 
	    CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);
    Syslog(' ', "MBNNTP v%s", VERSION);

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
//	nolocalport();

	if (getpeername(0,(struct sockaddr*)&peeraddr,&addrlen) == 0) {
	    Syslog('s', "TCP connection: len=%d, family=%hd, port=%hu, addr=%s",
		    addrlen,peeraddr.sin_family, peeraddr.sin_port, inet_ntoa(peeraddr.sin_addr));
	    Syslog('+', "Incoming connection from %s", inet_ntoa(peeraddr.sin_addr));
	    PUTSTR((char *)"Hello\r\n");
	}
    }

    cookedport();

    die(0);
    return 0;
}


