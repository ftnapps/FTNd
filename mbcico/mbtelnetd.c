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
#include "mbtelnetd.h"


#define MBT_BUFSIZ  8192 
#define MBT_TIMEOUT 3600


void die(int);
int  init_telnet(void);
void answer(int, int);
int  read0(char *, int);
int  write1(char *, int);
void com_gw(int);


#define WILL		      251
#define WONT		      252
#define DO		      253
#define DONT		      254
#define IAC		      255

#define TN_TRANSMIT_BINARY	0
#define	TN_ECHO			1
#define TN_SUPPRESS_GA		3



static int	tellen;
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

    init_telnet();

    tmp = calloc(81, sizeof(char ));
    sprintf(tmp, "mbtelnetd v%s\r\n", VERSION);
    write1(tmp, strlen(tmp));
    free(tmp);

    com_gw(s);
    
    free(remote_name);
    free(remote_port);
    close(s);
    die(0);
    return 0;
}



/* --- This is an artwork of serge terekhov, 2:5000/13@fidonet :) --- */

void answer (int tag, int opt)
{
    char    buf[3];
    char    *r = (char *)"???";
	
    switch (tag) {
	case WILL:
	    r = (char *)"WILL";
	    break;
	case WONT:
	    r = (char *)"WONT";
	    break;
	case DO:
	    r = (char *)"DO";
	    break;
	case DONT:
	    r = (char *)"DONT";
	    break;
    }
    Syslog('s', "TELNET send %s %d", r, opt);

    buf[0] = IAC;
    buf[1] = tag;
    buf[2] = opt;
    if (write (1, buf, 3) != 3)
	WriteError("$answer cant send");
}



int init_telnet(void)
{
    tellen = 0;
    answer (DO, TN_SUPPRESS_GA);
    answer (WILL, TN_SUPPRESS_GA);
    answer (DO, TN_TRANSMIT_BINARY);
    answer (WILL, TN_TRANSMIT_BINARY);
    answer (DO, TN_ECHO);
    answer (WILL, TN_ECHO);
    return 1;
}



int read0 (char *buf, int len)
{
    int		n = 0, m;
    char	*q, *p;
    static char	telbuf[4];

    while ((n == 0) && (n = read (0, buf + tellen, MBT_BUFSIZ - tellen)) > 0) {
	if (tellen) {
	    memcpy(buf, telbuf, tellen);
	    n += tellen;
	    tellen = 0;
	}

	if (memchr (buf, IAC, n)) {
	    for (p = q = buf; n--; )
		if ((m = (unsigned char)*q++) != IAC)
		    *p++ = m;
		else {
		    if (n < 2) {
			memcpy (telbuf, q - 1, tellen = n + 1);
			break;
		    }
		    --n;
		    switch (m = (unsigned char)*q++) {
			case WILL:  m = (unsigned char)*q++; --n;
				    Syslog('s', "TELNET: recv WILL %d", m);
				    if (m != TN_TRANSMIT_BINARY && m != TN_SUPPRESS_GA && m != TN_ECHO)
					answer (DONT, m);
				    break;
			case WONT:  m = *q++; 
				    --n;
				    Syslog('s', "TELNET: recv WONT %d", m);
				    break;
			case DO:    m = (unsigned char)*q++; 
				    --n;
				    Syslog('s', "TELNET: recv DO %d", m);
				    if (m != TN_TRANSMIT_BINARY && m != TN_SUPPRESS_GA && m != TN_ECHO)
					answer (WONT, m);
				    break;
			case DONT:  m = (unsigned char)*q++; 
				    --n;
				    Syslog('s', "TELNET: recv DONT %d", m);
				    break;
			case IAC:   *p++ = IAC;
				    break;
			default:    Syslog('s', "TELNET: recv IAC %d", m);
				    break;
		    }
		}
	    n = p - buf;
	}
    }

    return n;
}



int write1 (char *buf, int len)
{
    char    *q;
    int	    k, l;
    
    l = len;
    while ((len > 0) && (q = memchr(buf, IAC, len))) {
	k = (q - buf) + 1;
	if ((write (1, buf, k) != k) || (write (1, q, 1) != 1)) {
	    return -1;
	}
	buf += k;
	len -= k;
    }

    if ((len > 0) && write (1, buf, len) != len) {
	return -1;
    }
    return l;
}



void com_gw(int in)
{
    fd_set		    fds;
    int			    n, fdsbits;
    static struct timeval   tout = { MBT_TIMEOUT, 0 };
    unsigned char	    buf[MBT_BUFSIZ];

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
                    if (write1(buf, n) < 0) {
			goto bad;
		    }
                } else {
		    goto bad;
		}
            }
            if (FD_ISSET(0, &fds)) {
                if ((n = read0(buf, sizeof buf)) > 0) {
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


