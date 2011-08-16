/*****************************************************************************
 *
 * $Id: mbcico.c,v 1.46 2007/09/02 11:17:31 mbse Exp $
 * Purpose: Fidonet mailer
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/nodelist.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "config.h"
#include "answer.h"
#include "call.h"
#include "lutil.h"
#include "mbcico.h"
#include "session.h"
#include "binkp.h"


#define	IEMSI 1

int		master = 0;
int		immediatecall = FALSE;
char		*forcedphone = NULL;
char		*forcedline = NULL;
char		*inetaddr = NULL;
char		*protocol = NULL;
char		*envptr = NULL;
time_t		t_start;
time_t		t_end;
time_t		c_start;
time_t		c_end;
int		online = 0;
unsigned int	sentbytes = 0;
unsigned int	rcvdbytes = 0;
int		tcp_mode = TCPMODE_NONE;
int		Loaded = FALSE;
int		telnet = FALSE;
int		crashme = FALSE;


extern char	*myname;
char		*inbound;
char		*tempinbound = NULL;
char		*uxoutbound;
char		*name;
char		*phone;
char		*flags;
extern int	gotfiles;
extern int	mypid;
extern unsigned int	report_count;

extern int	session_type;
extern int	session_state;

void usage(void)
{
    fprintf(stderr,"ifcico; (c) Eugene G. Crosser, 1993-1997\n");
    fprintf(stderr,"mbcico ver. %s; (c) %s\n\n", VERSION, SHORTRIGHT);
    fprintf(stderr,"mbcico [-a inetaddr[:port]] [-n phone] [-l tty] [-t ibn|-t ifc] node\n");
    fprintf(stderr,"node   should be in domain form, e.g. f11.n22.z3\n");
    fprintf(stderr,"       (this implies master mode)\n");
    fprintf(stderr," or:\n");
    fprintf(stderr,"mbcico tsync|yoohoo|**EMSI_INQC816|-t ibn|-t ifc\n");
    fprintf(stderr,"       (this implies slave mode)\n");
}



void free_mem(void)
{
    free(inbound);
    if (name)
	free(name);
    if (phone)
	free(phone);
    if (flags)
	free(flags);
    if (uxoutbound)
	free(uxoutbound);
    if (protocol)
	free(protocol);
    if ((nlent) && (nlent->url))
	free(nlent->url);
}



void die(int onsig)
{
    int		    total = 0;
    unsigned int    rcvd = 0, sent = 0;

    signal(onsig, SIG_IGN);

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    Syslog('+', "Terminated with error %d", onsig);
	binkp_abort();
    }

    if (sentbytes || rcvdbytes) {
	total = (int)(c_end - c_start -2);
	if (total < 1)
	    total = 1;
	Syslog('+', "Sent %lu bytes, received %lu bytes, avg %d cps", sentbytes, rcvdbytes, (sentbytes + rcvdbytes) / total);
	sent = sentbytes / 1024;
	if (sentbytes && !sent)
	    sent = 1;	/* If something, at least 1 KByte */
	rcvd = rcvdbytes / 1024;
	if (rcvdbytes && !rcvd)
	    rcvd = 1;
    }

    SockS("MSMS:6,%d,%d,%d,%d,%d,%d;", rcvd, sent, master, session_state, session_type, report_count);

    if (online)
	Syslog('+', "Connected %s", str_time(online));

    if (gotfiles)
	CreateSema((char *)"mailin");

    /*
     * Free memory
     */
    free_mem();
    deinitnl();
    
    t_end = time(NULL);
    Syslog(' ', "MBCICO finished in %s", t_elapsed(t_start, t_end));

    if (envptr)
	free(envptr);
    ExitClient(onsig);
}



int main(int argc, char *argv[])
{
    faddr	    *addr = NULL;
    int		    i, c, uid, rc, maxrc;
    char	    *answermode = NULL, *p = NULL, *cmd = NULL;
    struct passwd   *pw;
    FILE	    *fp;
#ifdef IEMSI
    char	    temp[PATH_MAX];
#endif

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

    if (argc < 2) {
	usage();
	if (envptr)
	    free(envptr);
	exit(MBERR_COMMANDLINE);
    }

    InitConfig();
    InitNode();
    InitFidonet();
    mbse_TermInit(1, 80, 25);
    t_start = c_start = c_end = time(NULL);

    InitClient(pw->pw_name, (char *)"mbcico", CFG.location, CFG.logfile, 
	    CFG.cico_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);
    Syslog(' ', " ");
    Syslog(' ', "MBCICO v%s", VERSION);

    /*
     * Catch all signals we can, and ignore the rest.
     */
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGINT) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM) || (i == SIGIOT)) {
	    signal(i, (void (*))die);
	} else if ((i != SIGKILL) && (i != SIGSTOP)) {
	    signal(i, SIG_IGN);
	}
    }

    /*
     *  Check if history file exists, if not create a new one.
     */
    cmd = calloc(PATH_MAX, sizeof(char));
    snprintf(cmd, PATH_MAX -1, "%s/var/mailer.hist", getenv("MBSE_ROOT"));
    if ((fp = fopen(cmd, "r")) == NULL) {
	if ((fp = fopen(cmd, "a")) == NULL) {
	    WriteError("$Can't create %s", cmd);
	} else {
	    memset(&history, 0, sizeof(history));
	    history.online = (int)time(NULL);
	    history.offline = (int)time(NULL);
	    fwrite(&history, sizeof(history), 1, fp);
	    fclose(fp);
	    Syslog('+', "Created new %s", cmd);
	}
    } else {
	fclose(fp);
    }
    free(cmd);
    memset(&history, 0, sizeof(history));

    cmd = xstrcpy((char *)"Cmd: mbcico");
    for (i = 1; i < argc; i++) {
	cmd = xstrcat(cmd, (char *)" ");
	cmd = xstrcat(cmd, argv[i]);
    }
    Syslog(' ', cmd);
    free(cmd);

    setmyname(argv[0]);

    while ((c = getopt(argc,argv,"r:n:l:t:a:ch")) != -1) {
	switch (c) {
	    case 'r':	WriteError("commandline option -r is obsolete");
			break;

	    case 'l':	forcedline = optarg; 
			break;

	    case 't':	p = xstrcpy(optarg);
			if (strncmp(p, "ifc", 3) == 0) {
			    tcp_mode = TCPMODE_IFC;
			    protocol = xstrcpy((char *)"fido");
			} else if (strncmp(p, "ibn", 3) == 0) {
			    tcp_mode = TCPMODE_IBN;
			    protocol = xstrcpy((char *)"binkp");
			} else if (strncmp(p, "itn", 3) == 0) {
			    tcp_mode = TCPMODE_ITN;
			    protocol = xstrcpy((char *)"telnet");
			    telnet = TRUE;
			} else {
			    usage();
			    die(MBERR_COMMANDLINE);
			}
			free(p);
			RegTCP();
			break;

	    case 'a':	inetaddr = optarg; 
			break;

	    case 'n':	forcedphone = optarg; 
			break;

	    case 'c':	crashme = TRUE;
			Syslog('+', "crashme is set");
			break;

	    default:	usage(); 
			die(MBERR_COMMANDLINE);
	}
    }

    /*
     * Load rest of the configuration
     */
    inbound	= xstrcpy(CFG.inbound);
    uxoutbound	= xstrcpy(CFG.uxpath);
    name	= xstrcpy(CFG.bbs_name);
    phone	= xstrcpy(CFG.IP_Phone);
    flags	= xstrcpy(CFG.IP_Flags);

    while (argv[optind]) {

	for (p = argv[optind]; (*p) && (*p == '*'); p++);
#ifdef IEMSI
        if (strncasecmp(p, "EMSI_NAKEEC3", 12) == 0) {

	    Syslog('+', "Detected IEMSI client, starting mblogin");
            snprintf(temp, PATH_MAX -1, "%s/bin/mblogin", getenv("MBSE_ROOT"));
            socket_shutdown(mypid);

            if (execl(temp, "mblogin", (char *)NULL) == -1)
                perror("FATAL: Error loading BBS!");

	        InitClient(pw->pw_name, (char *)"mbcico", CFG.location, CFG.logfile,
			            CFG.cico_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);
            /*
             * If this happens, nothing is logged!
             */
            printf("\n\nFATAL: Loading of the BBS failed!\n\n");
            sleep(3);
            free_mem();
            if (envptr)
                free(envptr);
            exit(MBERR_EXEC_FAILED);
        }
#endif
	if ((strcasecmp(argv[optind],"tsync") == 0) ||
	    (strcasecmp(argv[optind],"yoohoo") == 0) ||
	    (strcasecmp(argv[optind],"ibn") == 0) ||
	    (strncasecmp(p,"EMSI_",5) == 0)) {
	    master = 0;
	    answermode = argv[optind];
	} else {
	    if ((addr = parsefaddr(argv[optind]))) {
		immediatecall = TRUE;
		master = 1;
	    } else {
		WriteError("Unrecognizable address \"%s\"", argv[optind]);
	    }
	}
	optind++;
    }

    /*
       The following witchkraft about uid-s is necessary to make
       access() work right.  Unforunately, access() checks the real
       uid, if mbcico is invoked with supervisor real uid (as when
       called by uugetty) it returns X_OK for the magic files that
       even do not have `x' bit set.  Therefore, `reference' magic
       file requests are taken for `execute' requests (and the
       actual execution natually fails).  Here we set real uid equal
       to effective.  If real uid is not zero, all these fails, but
       in this case it is not necessary anyway.
    */

    uid=geteuid();
    seteuid(0);
    setuid(uid);
    seteuid(uid);

    umask(066);	/* packets may contain confidential information */

    maxrc=0;
    if (master) {
	/*
	 * Don't do outbound calls if low diskspace
	 */
	if (enoughspace(CFG.freespace) == 0)
	    die(MBERR_DISK_FULL);

	if (addr == NULL) {
	    WriteError("Calling mbcico without node address not supported anymore");
	    die(MBERR_COMMANDLINE);
	}

	rc = call(addr);
	Syslog('+', "Call to %s %s (rc=%d)", ascfnode(addr, 0x1f), rc?"failed":"successful", rc);
	if (rc > maxrc) 
	    maxrc=rc;
    } else {
	/*
	 * Slave (answer) mode
	 */
	if (!answermode && tcp_mode == TCPMODE_IBN)
	    answermode = xstrcpy((char *)"ibn");
	rc = maxrc = answer(answermode);
	Syslog('+', "Incoming call %s (rc=%d)", rc?"failed":"successful", rc);
    }

    tidy_faddr(addr);
    if (maxrc)
	die(maxrc);
    else
	die(MBERR_OK);
    return 0;
}


