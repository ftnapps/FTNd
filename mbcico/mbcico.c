/*****************************************************************************
 *
 * $Id$
 * Purpose: Fidonet mailer
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

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/nodelist.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "../lib/dbnode.h"
#include "../lib/dbftn.h"
#include "../lib/mberrors.h"
#include "config.h"
#include "answer.h"
#include "call.h"
#include "lutil.h"
#include "mbcico.h"
#include "session.h"



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
unsigned long	sentbytes = 0;
unsigned long	rcvdbytes = 0;
int		tcp_mode = TCPMODE_NONE;
int		Loaded = FALSE;


extern char	*myname;
char		*inbound;
char		*tempinbound = NULL;
char		*uxoutbound;
char		*name;
char		*phone;
char		*flags;
extern int	gotfiles;
extern int	mypid;


void usage(void)
{
    fprintf(stderr,"ifcico; (c) Eugene G. Crosser, 1993-1997\n");
    fprintf(stderr,"mbcico ver. %s; (c) %s\n\n", VERSION, SHORTRIGHT);
    fprintf(stderr,"-a<inetaddr> <node>\n");
    fprintf(stderr,"-n<phone>	forced phone number\n");
    fprintf(stderr,"-l<ttydevice>	forced tty device\n");
    fprintf(stderr,"-t<tcpmode>	must be one of ifc|itn|ibn, forces TCP/IP\n");
    fprintf(stderr,"-a<inetaddr>	supply internet hostname if not in nodelist\n");
    fprintf(stderr,"  <node>	should be in domain form, e.g. f11.n22.z3\n");
    fprintf(stderr,"		(this implies master mode)\n");
    fprintf(stderr,"\n or: %s tsync|yoohoo|**EMSI_INQC816|-t ibn|-t ifc|-t itn\n",myname);
    fprintf(stderr,"		(this implies slave mode)\n");
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
}



void die(int onsig)
{
    int	total = 0;

    signal(onsig, SIG_IGN);

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    Syslog('+', "Terminated with error %d", onsig);
    }

    if (sentbytes || rcvdbytes) {
	total = (int)(c_end - c_start);
	if (!total)
	    total = 1;
	Syslog('+', "Sent %lu bytes, received %lu bytes, avg %d cps", sentbytes, rcvdbytes, (sentbytes + rcvdbytes) / total);
    }

    if (online)
	Syslog('+', "Connected %s", str_time(online));

    if (gotfiles)
	CreateSema((char *)"mailin");

    t_end = time(NULL);
    Syslog(' ', "MBCICO finished in %s", t_elapsed(t_start, t_end));
    free_mem();
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
    TermInit(1);
    t_start = c_start = c_end = time(NULL);

    InitClient(pw->pw_name, (char *)"mbcico", CFG.location, CFG.logfile, CFG.cico_loglevel, CFG.error_log, CFG.mgrlog);
    Syslog(' ', " ");
    Syslog(' ', "MBCICO v%s", VERSION);

    /*
     * Catch all signals we can, and handle the rest.
     */
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGINT) || (i == SIGBUS) || (i == SIGFPE) || (i == SIGSEGV)) {
	    signal(i, (void (*))die);
	} else {
	    signal(i, SIG_DFL);
	}
    }

    /*
     *  Check if history file exists, if not create a new one.
     */
    cmd = calloc(PATH_MAX, sizeof(char));
    sprintf(cmd, "%s/var/mailer.hist", getenv("MBSE_ROOT"));
    if ((fp = fopen(cmd, "r")) == NULL) {
	if ((fp = fopen(cmd, "a")) == NULL) {
	    WriteError("$Can't create %s", cmd);
	} else {
	    memset(&history, 0, sizeof(history));
	    history.online = time(NULL);
	    history.offline = time(NULL);
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

    while ((c = getopt(argc,argv,"r:n:l:t:a:I:h")) != -1) {
	switch (c) {
	    case 'r':	WriteError("commandline option -r is obsolete");
			break;

	    case 'l':	forcedline = optarg; 
			break;

	    case 't':	p = xstrcpy(optarg);
			if (strncmp(p, "ifc", 3) == 0) {
			    tcp_mode = TCPMODE_IFC;
			    protocol = xstrcpy((char *)"fido");
			} else if (strncmp(p, "itn", 3) == 0) {
			    tcp_mode = TCPMODE_ITN;
			    protocol = xstrcpy((char *)"telnet");
			} else if (strncmp(p, "ibn", 3) == 0) {
			    tcp_mode = TCPMODE_IBN;
			    protocol = xstrcpy((char *)"binkp");
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
    phone	= xstrcpy(CFG.Phone);
    flags	= xstrcpy(CFG.Flags);

    while (argv[optind]) {

	for (p = argv[optind]; (*p) && (*p == '*'); p++);
#ifdef IEMSI
        if (strncasecmp(p, "EMSI_NAKEEC3", 12) == 0) {

	    Syslog('+', "Detected IEMSI client, starting BBS");
            sprintf(temp, "%s/bin/mbsebbs", getenv("MBSE_ROOT"));
            socket_shutdown(mypid);

            if (execl(temp, "mbsebbs", (char *)NULL) == -1)
                perror("FATAL: Error loading BBS!");

            /*
             * If this happens, nothing is logged!
             */
            printf("\n\nFATAL: Loading of the BBS failed!\n\n");
            sleep(3);
            free_mem();
            if (envptr)
                free(envptr);
            exit(MBERROR_EXEC_FAILED);
        }
#endif
	if ((strcasecmp(argv[optind],"tsync") == 0) ||
	    (strcasecmp(argv[optind],"yoohoo") == 0) ||
	    (strcasecmp(argv[optind],"ibn") == 0) ||
	    (strncasecmp(p,"EMSI_",5) == 0)) {
	    master = 0;
	    answermode = argv[optind];
	    Syslog('S', "Inbound \"%s\" mode", MBSE_SS(answermode));
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
	if (!diskfree(CFG.freespace))
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
	/* slave */
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


