/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Fidonet telnet proxy
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/


/*
  Simple proxy daemon for ifcico to work with SIO/VMODEM.
  ========================================================
  Written by:  Vadim Zaliva, lord@crocodile.kiev.ua, 2:463/80

  This software is provided ``as is'' without express or implied warranty.
  Feel free to distribute it.
  
  Feel free to contact me with improovments request
  and bug reports.
  
  Some parts of this code are taken from
  1. serge terekhov, 2:5000/13@fidonet path for ifmail
  2. Vadim Kurland, vadim@gu.kiev.ua transl daemon.
  Thanks them.
 */

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "../lib/mberrors.h"
#include "hydra.h"
#include "telnio.h"
#include "mbtelnetd.h"


#define MBT_TIMEOUT 3600


void die(int);
void com_gw(int);


char		*envptr = NULL;
time_t		t_start, t_end;


void die(int onsig)
{
    signal(onsig, SIG_IGN);

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    Syslog('+', "Terminated with error %d", onsig);
    }

    t_end = time(NULL);
    Syslog(' ', "MBTELNETD finished in %s", t_elapsed(t_start, t_end));
    if (envptr)
	free(envptr);
    ExitClient(onsig);
}



int main(int ac, char **av)
{
    struct sockaddr_in	peeraddr;
    int			addrlen = sizeof(struct sockaddr_in);
    char		*remote_name = NULL;
    char		*remote_port = NULL;
    int			s; /* socket to remote*/
    int			i;
    struct hostent	*hp;
    struct servent	*sp;
    struct sockaddr_in	server;
    char		*tmp = NULL;
    struct passwd	*pw;

    /*
     * The next trick is to supply a fake environment variable
     * MBSE_ROOT in case we are started from inetd or mgetty,
     * this will setup the variable so InitConfig() will work.
     * The /etc/passwd must point to the correct homedirectory.
     */
     pw = getpwuid(getuid());
	if (getenv("MBSE_ROOT") == NULL) {
	envptr = xstrcpy((char *)"MBSE_ROOT=");
	envptr = xstrcat(envptr, pw->pw_dir);
	putenv(envptr);
    }

    InitConfig();
    t_start = time(NULL);

    InitClient(pw->pw_name, (char *)"mbtelnetd", CFG.location, CFG.logfile, 
	    CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);

    Syslog(' ', " ");
    Syslog(' ', "MBTELNETDv%s", VERSION);

    /*
     * Catch all signals we can, and ignore the rest.
     */
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGINT) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM) || (i == SIGKILL)) {
	    signal(i, (void (*))die);
	} else {
	    signal(i, SIG_IGN);
	}
    }

    remote_name = xstrcpy((char *)"localhost");
    remote_port = xstrcpy((char *)"fido");

    if (getpeername(0,(struct sockaddr*)&peeraddr,&addrlen) == 0) {
	tmp = strdup(inet_ntoa(peeraddr.sin_addr));
	Syslog('+', "Incoming TCP connection from %s", tmp ? tmp : "Unknown");
	Syslog('+', "Rerouting to %s:%s", remote_name, remote_port);
    }

    if (tmp) 
	free(tmp);
    
    if ((sp = getservbyname(remote_port, "tcp")) == NULL) {
	WriteError("Can't find service: %s", remote_port);
	free(remote_name);
	free(remote_port);
	die(MBERR_INIT_ERROR);
    }

    if ((s = socket(AF_INET,SOCK_STREAM,0)) == -1) {
	WriteError("Can't create Internet domain socket");
	free(remote_name);
	free(remote_port);
	die(MBERR_INIT_ERROR);
    }

    if ((hp = gethostbyname(remote_name)) == NULL) {
	WriteError("%s - Unknown host", remote_name);
	free(remote_name);
	free(remote_port);
	die(MBERR_INIT_ERROR);
    }
  
    memset(&server,0,sizeof(server));
    memcpy((char *)&server.sin_addr,hp->h_addr,hp->h_length);
    
    server.sin_family=hp->h_addrtype;
    server.sin_port  = sp->s_port;
    
    if (connect(s,(struct sockaddr *)&server,sizeof(server)) == -1) {
	WriteError("Can't connect %s", remote_name);
	free(remote_name);
	free(remote_port);
	die(MBERR_INIT_ERROR);
    }

    telnet_init();

    tmp = calloc(81, sizeof(char ));
    sprintf(tmp, "mbtelnetd v%s\r\n", VERSION);
    telnet_write(tmp, strlen(tmp));
    free(tmp);

    com_gw(s);
    
    free(remote_name);
    free(remote_port);
    close(s);
    die(0);
    return 0;
}



void com_gw(int in)
{
    fd_set		    fds;
    int			    n, fdsbits;
    static struct timeval   tout = { MBT_TIMEOUT, 0 };
    unsigned char	    buf[H_ZIPBUFLEN];

    alarm(0);
    fdsbits = in + 1;

    while (TRUE) {
        FD_ZERO(&    fds);
        FD_SET (in, &fds);
        FD_SET (0 , &fds);
        FD_SET (1 , &fds);
	
        tout.tv_sec  = MBT_TIMEOUT;
        tout.tv_usec = 0;
        if ((n = select(fdsbits, &fds, NULL, NULL, &tout)) > 0) {
            if (FD_ISSET(in, &fds)) {
                if ((n = read(in, buf, sizeof buf)) > 0) {
                    if (telnet_write(buf, n) < 0) {
			goto bad;
		    }
                } else {
		    goto bad;
		}
            }
            if (FD_ISSET(0, &fds)) {
                if ((n = telnet_read(buf, sizeof buf)) > 0) {
                    if (write(in, buf, n) < 0) goto bad;
                } else {
		    goto bad;
		}
            }
        } else {
	    goto bad;
	}
    }
bad: ;
}


