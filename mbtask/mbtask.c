/*****************************************************************************
 *
 * Purpose ...............: MBSE BBS Task Manager
 *
 *****************************************************************************
 * Copyright (C) 1997-2011
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../paths.h"
#include "signame.h"
#include "taskstat.h"
#include "taskutil.h"
#include "taskregs.h"
#include "taskcomm.h"
#include "taskdisk.h"
#include "taskibc.h"
#include "callstat.h"
#include "outstat.h"
#include "../lib/nodelist.h"
#include "ports.h"
#include "calllist.h"
#include "ping.h"
#include "taskchat.h"
#include "mbtask.h"



/*
 *  Global variables
 */
static onetask		task[MAXTASKS];		/* Array with tasks	*/
extern tocall		calllist[MAXTASKS];	/* Array with calllist	*/
reg_info		reginfo[MAXCLIENT];	/* Array with clients	*/
static pid_t		pgrp;			/* Pids group		*/
int			sock = -1;		/* Datagram socket	*/
struct sockaddr_un	servaddr;		/* Server address	*/
struct sockaddr_un	from;			/* From address		*/
struct sockaddr_in	myaddr_in;		/* IBC local socket	*/
struct sockaddr_in	clientaddr_in;		/* IBC remote socket	*/
int			ibcsock = -1;		/* IBC socket		*/
socklen_t		fromlen;
char			waitmsg[81];		/* Waiting message	*/
static char		spath[PATH_MAX];	/* Socket path		*/
int			logtrans = 0;		/* Log transactions	*/
struct taskrec		TCFG;			/* Task config record	*/
struct sysconfig	CFG;			/* System config	*/
struct _nodeshdr	nodeshdr;		/* Nodes header record	*/
struct _nodes		nodes;			/* Nodes data record	*/
struct _fidonethdr	fidonethdr;		/* Fidonet header rec.	*/
struct _fidonet		fidonet;		/* Fidonet data record	*/
time_t			tcfg_time;		/* Config record time	*/
time_t			cfg_time;		/* Config record time	*/
time_t			tty_time;		/* TTY config time	*/
char			tcfgfn[PATH_MAX];	/* Config file name	*/
char			cfgfn[PATH_MAX];	/* Config file name	*/
char			ttyfn[PATH_MAX];	/* TTY file name	*/
extern int     		ping_isocket;		/* Ping socket		*/
int			internet = FALSE;	/* Internet is down	*/
double			Load;			/* System Load		*/
int			Processing;		/* Is system running	*/
int			ZMH = FALSE;		/* Zone Mail Hour	*/
int			UPSalarm = FALSE;	/* UPS alarm status	*/
int			UPSdown = FALSE;	/* UPS down status	*/
extern int		s_bbsopen;		/* BBS open semafore	*/
extern int		s_scanout;		/* Scan outbound sema	*/
extern int		s_mailout;		/* Mail out semafore	*/
extern int		s_mailin;		/* Mail in semafore	*/
extern int		s_index;		/* Compile nl semafore	*/
extern int		s_newnews;		/* New news semafore	*/
extern int		s_reqindex;		/* Create req index sem */
extern int		s_msglink;		/* Messages link sem	*/
int			masterinit = FALSE;	/* Master init needed	*/
int			ptimer = PAUSETIME;	/* Pause timer		*/
int			tflags = FALSE;		/* if nodes with Txx	*/
extern int		nxt_hour;		/* Next event hour	*/
extern int		nxt_min;		/* Next event minute	*/
extern _alist_l		*alist;			/* Nodes to call list	*/
int			rescan = FALSE;		/* Master rescan flag	*/
extern int		pots_calls;
extern int		isdn_calls;
extern int		inet_calls;
extern int		pots_lines;		/* POTS lines available	*/
extern int		isdn_lines;		/* ISDN lines available */
extern int		pots_free;		/* POTS lines free	*/
extern int		isdn_free;		/* ISDN lines free	*/
extern pp_list		*pl;			/* List of tty ports	*/
extern int		ipmailers;		/* TCP/IP mail sessions	*/
extern int		tosswait;		/* Toss wait timer	*/
extern pid_t		mypid;			/* Pid of daemon	*/
int			G_Shutdown = FALSE;	/* Global shutdown	*/
int			nodaemon = FALSE;	/* Run in foreground	*/
extern time_t		resettime;		/* IBC reset time	*/
int			Run_IBC = TRUE;		/* Run IBC server	*/


void logtasks(void);


/*
 *  Load main configuration, if it doesn't exist, create it.
 *  This is the case the very first time when you start MBSE BBS.
 */
void load_maincfg(void)
{
    FILE	    *fp;
    struct utsname  un;
    int		    i;

    if ((fp = fopen(cfgfn, "r")) == NULL) {
	masterinit = TRUE;
        memset(&CFG, 0, sizeof(CFG));

        /*
         * Fill Registration defaults
         */
        snprintf(CFG.bbs_name, 36, "MBSE BBS");
        uname((struct utsname *)&un); 
#if defined(__USE_GNU)
        snprintf(CFG.sysdomain, 36, "%s.%s", un.nodename, un.domainname); 
#elif defined(__linux__)
        snprintf(CFG.sysdomain, 36, "%s.%s", un.nodename, un.__domainname);
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	snprintf(CFG.sysdomain, 36, "%s", un.nodename);	/* No domain in FreeBSD */
#else
#error "Don't know un.domainname on this OS"
#endif
        snprintf(CFG.comment, 56,  "MBSE BBS development");
        snprintf(CFG.origin, 51,   "MBSE BBS. Made in the Netherlands");
        snprintf(CFG.location, 36, "Earth");

        /*
         * Fill Filenames defaults
         */
        snprintf(CFG.logfile, 15, "system.log");
        snprintf(CFG.error_log, 15, "error.log");
        snprintf(CFG.default_menu, 15, "main.mnu");
        snprintf(CFG.deflang, 10, "en");
        snprintf(CFG.chat_log, 15, "chat.log");
        snprintf(CFG.welcome_logo, 15, "logo.ans");
	snprintf(CFG.mgrlog, 15, "manager.log");
	snprintf(CFG.debuglog, 15, "debug.log");

        /*
         * Fill Global defaults
         */
        snprintf(CFG.bbs_usersdir, 65, "%s/home", getenv("MBSE_ROOT"));
        snprintf(CFG.nodelists, 65, "%s/var/nodelist", getenv("MBSE_ROOT"));
        snprintf(CFG.inbound, 65, "%s/var/unknown", getenv("MBSE_ROOT"));
        snprintf(CFG.pinbound, 65, "%s/var/inbound", getenv("MBSE_ROOT"));
        snprintf(CFG.outbound, 65, "%s/var/bso/outbound", getenv("MBSE_ROOT"));
	snprintf(CFG.msgs_path, 65, "%s/var/msgs", getenv("MBSE_ROOT"));
        snprintf(CFG.uxpath, 65, "%s", getenv("MBSE_ROOT"));
        snprintf(CFG.badtic, 65, "%s/var/badtic", getenv("MBSE_ROOT"));
        snprintf(CFG.ticout, 65, "%s/var/ticqueue", getenv("MBSE_ROOT"));
        snprintf(CFG.req_magic, 65, "%s/var/magic", getenv("MBSE_ROOT"));
	snprintf(CFG.alists_path, 65, "%s/var/arealists", getenv("MBSE_ROOT"));
	snprintf(CFG.out_queue, 65, "%s/var/queue", getenv("MBSE_ROOT"));
	snprintf(CFG.rulesdir, 65, "%s/var/rules", getenv("MBSE_ROOT"));
	CFG.leavecase = TRUE;

        /*
         * Newfiles reports
         */
        snprintf(CFG.ftp_base, 65, "%s/ftp/pub", getenv("MBSE_ROOT"));
        CFG.newdays = 30;
        CFG.security.level = 20;
        CFG.new_split = 27;
        CFG.new_force = 30;

        /*
         * BBS Globals
         */
        CFG.CityLen = 6;
        CFG.exclude_sysop = TRUE;
        CFG.iConnectString = FALSE;
        CFG.iAskFileProtocols = FALSE;
        CFG.sysop_access = 32000;
        CFG.password_length = 4;
        CFG.iPasswd_Char = '.';
        CFG.idleout = 3;
        CFG.iQuota = 10;
        CFG.iCRLoginCount = 10;
        CFG.bbs_loglevel = DLOG_ALLWAYS | DLOG_ERROR | DLOG_ATTENT | DLOG_NORMAL | DLOG_VERBOSE;
        CFG.util_loglevel = DLOG_ALLWAYS | DLOG_ERROR | DLOG_ATTENT | DLOG_NORMAL | DLOG_VERBOSE;
        CFG.OLR_NewFileLimit = 30;
        CFG.OLR_MaxReq = 25;
        CFG.slow_util = TRUE;
        CFG.iCrashLevel = 100;
        CFG.iAttachLevel = 100;
        CFG.new_groups = 25;
	CFG.max_logins = 1;
	CFG.AskNewmail = YES;
	CFG.AskNewfiles = YES;

        CFG.slow_util = TRUE;
        CFG.iCrashLevel = 100;
        CFG.iAttachLevel = 100;
        CFG.new_groups = 25;
        snprintf(CFG.startname, 9, "bbs");
        CFG.freespace = 10;

        /*
         * New Users
         */
        CFG.newuser_access.level = 20;
        CFG.iCapUserName = TRUE;
        CFG.iDataPhone = TRUE;
        CFG.iVoicePhone = TRUE;
        CFG.iDOB = TRUE;
        CFG.iTelephoneScan = TRUE;
        CFG.iLocation = TRUE;
        CFG.iHotkeys = TRUE;
        CFG.iCapLocation = FALSE;
        CFG.AskAddress = TRUE;
        CFG.GiveEmail = TRUE;

        /*
         * Colors
         */
        CFG.TextColourF         = CYAN;
        CFG.TextColourB         = BLACK;
        CFG.UnderlineColourF    = YELLOW;
        CFG.UnderlineColourB    = BLACK;
        CFG.InputColourF        = LIGHTCYAN;
        CFG.InputColourB        = BLACK;
        CFG.CRColourF           = WHITE;
        CFG.CRColourB           = BLACK;
        CFG.MoreF               = LIGHTMAGENTA;
        CFG.MoreB               = BLACK;
        CFG.HiliteF             = WHITE;
        CFG.HiliteB             = BLACK;
        CFG.FilenameF           = YELLOW;
        CFG.FilenameB           = BLACK;
        CFG.FilesizeF           = LIGHTMAGENTA;
        CFG.FilesizeB           = BLACK;
        CFG.FiledateF           = LIGHTGREEN;
        CFG.FiledateB           = BLACK;
        CFG.FiledescF           = CYAN;
        CFG.FiledescB           = BLACK;
        CFG.MsgInputColourF     = CYAN;
        CFG.MsgInputColourB     = BLACK;

        /*
         * Paging
         */
        CFG.iPageLength         = 30;
        CFG.iMaxPageTimes       = 5;
        CFG.iAskReason          = TRUE;
        CFG.iSysopArea          = 1;
        CFG.iAutoLog            = TRUE;
        CFG.iChatPromptChk      = TRUE;
        CFG.iStopChatTime       = TRUE;

        /*
         * Fill ticconf defaults
         */
        CFG.ct_PlusAll = TRUE;
        CFG.ct_Notify = TRUE;
        CFG.ct_Message = TRUE;
        CFG.ct_TIC = TRUE;
        CFG.tic_days = 30;
        snprintf(CFG.hatchpasswd, 21, "DizIzMyBIGseeKret");
        CFG.tic_systems = 10;
        CFG.tic_groups  = 25;
        CFG.tic_dupes   = 16000;

        /*
         * Fill Mail defaults
         */
        CFG.maxpktsize = 150;
        CFG.maxarcsize = 300;
        snprintf(CFG.badboard, 65, "%s/var/mail/badmail", getenv("MBSE_ROOT"));
        snprintf(CFG.dupboard, 65, "%s/var/mail/dupemail", getenv("MBSE_ROOT"));
        snprintf(CFG.popnode, 65, "localhost");
        snprintf(CFG.smtpnode, 65, "localhost");
        snprintf(CFG.nntpnode, 65, "localhost");
        CFG.toss_days = 30;
        CFG.toss_dupes = 16000;
        CFG.toss_old = 60;
        CFG.defmsgs = 500;
        CFG.defdays = 90;
        CFG.toss_systems = 10;
        CFG.toss_groups = 25;
        CFG.UUCPgate.zone = 2;
        CFG.UUCPgate.net  = 292;
        CFG.UUCPgate.node = 875;
        snprintf(CFG.UUCPgate.domain, 13, "fidonet");
        CFG.nntpdupes = 16000;
	CFG.ca_PlusAll = TRUE;
	CFG.ca_Notify = TRUE;
	CFG.ca_Passwd = TRUE;
	CFG.ca_Pause = TRUE;
	CFG.ca_Check = TRUE;

        for (i = 0; i < 32; i++) {
	    snprintf(CFG.fname[i], 17, "Flag %d", i+1);
	    snprintf(CFG.aname[i], 17, "Flag %d", i+1);
	}
	snprintf(CFG.aname[0], 17, "Everyone");


        /*
         * Fido mailer defaults
         */
        CFG.timeoutreset = 3L;
        CFG.timeoutconnect = 60L;
        snprintf(CFG.phonetrans[0].match, 21, "31-255");
        snprintf(CFG.phonetrans[1].match, 21, "31-");
        snprintf(CFG.phonetrans[1].repl, 21, "0");
        snprintf(CFG.phonetrans[2].repl, 21, "00");
        CFG.IP_Speed = 256000;
        CFG.dialdelay = 60;
        snprintf(CFG.IP_Flags, 31, "ICM,XX,IBN");
        CFG.cico_loglevel = DLOG_ALLWAYS | DLOG_ERROR | DLOG_ATTENT | DLOG_NORMAL | DLOG_VERBOSE;

	/*
	 *  WWW defaults
	 */
        snprintf(CFG.www_root, 81, "/var/www/htdocs");
        snprintf(CFG.www_link2ftp, 21, "files");
        snprintf(CFG.www_url, 41, "http://%s", CFG.sysdomain);
        snprintf(CFG.www_charset, 21, "ISO 8859-1");
        snprintf(CFG.www_author, 41, "Your Name");
	if (strlen(_PATH_CONVERT))
	    snprintf(CFG.www_convert, 81, "%s -geometry x100", _PATH_CONVERT);
        CFG.www_files_page = 10;

	CFG.maxarticles = 500;

	CFG.priority = 15;
#ifdef __linux__
	CFG.do_sync = TRUE;
#endif
	CFG.is_upgraded = TRUE;

        if ((fp = fopen(cfgfn, "a+")) == NULL) {
	    perror("");
            fprintf(stderr, "Can't create %s\n", cfgfn);
            exit(MBERR_INIT_ERROR);
        }
        fwrite(&CFG, sizeof(CFG), 1, fp);
        fclose(fp);
	chmod(cfgfn, 0640);
    } else {
        fread(&CFG, sizeof(CFG), 1, fp);
        fclose(fp);
	if (strlen(CFG.debuglog) == 0)
	    snprintf(CFG.debuglog, 15, "debug.log");
    }

    cfg_time = file_time(cfgfn);
}



/*
 *  Load task configuration data.
 */
void load_taskcfg(void)
{
    FILE    *fp;

    if ((fp = fopen(tcfgfn, "r")) == NULL) {
	memset(&TCFG, 0, sizeof(TCFG));
	TCFG.maxload = 1.50;
	snprintf(TCFG.zmh_start, 6, "02:30");
	snprintf(TCFG.zmh_end, 6, "03:30");
	snprintf(TCFG.cmd_mailout,  81, "%s/bin/mbfido scan web -quiet", getenv("MBSE_ROOT"));
	snprintf(TCFG.cmd_mailin,   81, "%s/bin/mbfido tic toss web -quiet", getenv("MBSE_ROOT"));
	snprintf(TCFG.cmd_newnews,  81, "%s/bin/mbfido news web -quiet", getenv("MBSE_ROOT"));
	snprintf(TCFG.cmd_mbindex1, 81, "%s/bin/mbindex -quiet", getenv("MBSE_ROOT"));
	if (strlen(_PATH_GOLDNODE))
	    snprintf(TCFG.cmd_mbindex2, 81, "%s -f -q", _PATH_GOLDNODE);
	snprintf(TCFG.cmd_msglink,  81, "%s/bin/mbmsg link -quiet", getenv("MBSE_ROOT"));
	    snprintf(TCFG.cmd_reqindex, 81, "%s/bin/mbfile index -quiet", getenv("MBSE_ROOT"));
	TCFG.max_tcp  = 0;
	snprintf(TCFG.isp_ping1, 41, "192.168.1.1");
	snprintf(TCFG.isp_ping2, 41, "192.168.1.1");
	if ((fp = fopen(tcfgfn, "a+")) == NULL) {
	    Syslog('?', "$Can't create %s", tcfgfn);
	    die(MBERR_INIT_ERROR);
	}
	fwrite(&TCFG, sizeof(TCFG), 1, fp);
	fclose(fp);
	chmod(tcfgfn, 0640);
	Syslog('+', "Created new %s", tcfgfn);
    } else {
	fread(&TCFG, sizeof(TCFG), 1, fp);
	fclose(fp);
    }

    tcfg_time = file_time(tcfgfn);
}



/*
 * Milliseconds timer, returns 0 on success.
 */
int msleep(int msecs)
{
    int             rc;
    struct timespec req, rem;

    rem.tv_sec = 0;
    rem.tv_nsec = 0;
    req.tv_sec = msecs / 1000;
    req.tv_nsec = (msecs % 1000) * 1000000;

    while (TRUE) {

	rc = nanosleep(&req, &rem);
	if (rc == 0)
	    break;
	if ((errno == EINVAL) || (errno == EFAULT)) {
	    WriteError("$msleep(%d)", msecs);
	    break;
	}

	/*
	 * Error was EINTR, run timer again to complete.
	 */
	req.tv_sec = rem.tv_sec;
	req.tv_nsec = rem.tv_nsec;
	rem.tv_sec = 0;
	rem.tv_nsec = 0;
    }

    return rc;
}



/*
 *  Launch an external program in the background.
 *  On success add it to the tasklist and return
 *  the pid. Set the pause timer.
 */
pid_t launch(char *cmd, char *opts, char *name, int tasktype)
{
    static char buf[PATH_MAX]; 
    static char	*vector[16];
    int		i, rc = 0;
    pid_t	pid = 0;

    if (checktasks(0) >= MAXTASKS) {
	Syslog('?', "Launch: can't execute %s, maximum tasks reached", cmd);
	return 0;
    }

    for (i = 0; i < 16; i++)
	vector[i] = NULL;
    
    if (opts == NULL)
	snprintf(buf, PATH_MAX, "%s", cmd);
    else
	snprintf(buf, PATH_MAX, "%s %s", cmd, opts);

    i = 0;
    vector[i++] = strtok(buf," \t\n\0");
    while ((vector[i++] = strtok(NULL," \t\n")) && (i<16));
    vector[15] = NULL;

    if (file_exist(vector[0], X_OK)) {
	WriteError("Launch: can't execute %s, command not found", vector[0]);
	return 0;
    }

    switch (pid = fork()) {
	case -1:
		WriteError("$Launch: error, can't fork grandchild");
		return 0;
	case 0:
		/*
		 * A delay in the child process to prevent it returns
		 * before the main process sees it ever started.
		 */
		msleep(150);

		/* From Paul Vixies cron: */
		rc = setsid(); /* It doesn't seem to help */
		if (rc == -1)
		    WriteError("$Launch: setsid()");

		close(0);
		if (open("/dev/null", O_RDONLY) != 0) {
		    WriteError("$Launch: \"%s\": reopen of stdin to /dev/null failed", buf);
		    _exit(MBERR_EXEC_FAILED);
		}
		close(1);
		if (open("/dev/null", O_WRONLY | O_APPEND | O_CREAT,0600) != 1) {
		    WriteError("$Launch: \"%s\": reopen of stdout to /dev/null failed", buf);
		    _exit(MBERR_EXEC_FAILED);
		}
		close(2);
		if (open("/dev/null", O_WRONLY | O_APPEND | O_CREAT,0600) != 2) {
		    WriteError("$Launch: \"%s\": reopen of stderr to /dev/null failed", buf);
		    _exit(MBERR_EXEC_FAILED);
		}
		errno = 0;
		rc = execv(vector[0],vector);
		WriteError("$Launch: execv \"%s\" failed, returned %d", cmd, rc);
		_exit(MBERR_EXEC_FAILED);
	default:
		/* grandchild's daddy's process */
		break;
    }

    /*
     *  Add it to the tasklist.
     */
    for (i = 0; i < MAXTASKS; i++) {
	if (strlen(task[i].name) == 0) {
	    strcpy(task[i].name, name);
	    strcpy(task[i].cmd, cmd);
	    if (opts)
		strcpy(task[i].opts, opts);
	    task[i].pid = pid;
	    task[i].status = 0;
	    task[i].running = TRUE;
	    task[i].rc = 0;
	    task[i].tasktype = tasktype;
	    logtasks();
	    break;
	}
    }

    ptimer = PAUSETIME;

    if (opts)
	Syslog('+', "Launch: task %d \"%s %s\" success, pid=%d", i, cmd, opts, pid);
    else
	Syslog('+', "Launch: task %d \"%s\" success, pid=%d", i, cmd, pid);
    return pid;
}



/*
 *  Count specific running tasks
 */
int runtasktype(int tasktype)
{
    int	    i, count = 0;

    for (i = 0; i < MAXTASKS; i++) {
	if (strlen(task[i].name) && task[i].running && (task[i].tasktype == tasktype))
	    count++;
    }
    return count;
}



void logtasks(void)
{
    int	    i, first = TRUE;
    
    if (first) {
	first = FALSE;
	Syslog('t', "Task             Type      pid stat    rc");
	Syslog('t', "---------------- ------- ----- ---- -----");
	for (i = 0; i < MAXTASKS; i++)
	    if (strlen(task[i].name))
		Syslog('t', "%-16s %s %5d %s %5d", task[i].name, callmode(task[i].tasktype),
			    task[i].pid, task[i].running?"runs":"stop", task[i].rc);
    }
}



/*
 *  Check all running tasks registered in the tasklist.
 *  Report programs that are stopped. If signal is set
 *  then send that signal.
 */
int checktasks(int onsig)
{
    int	i, j, rc, count = 0, status;

    for (i = 0; i < MAXTASKS; i++) {
	if (strlen(task[i].name)) {

	    if (onsig) {
		if (kill(task[i].pid, onsig) == 0)
		    Syslog('+', "%s to %s (pid %d) succeeded", SigName[onsig], task[i].name, task[i].pid);
		else
		    Syslog('+', "%s to %s (pid %d) failed", SigName[onsig], task[i].name, task[i].pid);
	    }

	    task[i].rc = wait4(task[i].pid, &status, WNOHANG | WUNTRACED, NULL);
	    task[i].status = status;

	    /*
	     * If task was set not running, handle status
	     */
	    if (task[i].rc) {
		task[i].running = FALSE;
		
		/*
		 * If a mailer call is finished, set the global rescan flag.
		 */
		if (task[i].tasktype == CM_POTS || task[i].tasktype == CM_ISDN || task[i].tasktype == CM_INET)
		    rescan = TRUE;
		ptimer = PAUSETIME;

		/*
		 * If a nodelist compiler is ready, reload the nodelists configuration
		 */
		if (task[i].tasktype == MBINDEX) {
		    deinitnl();
		    initnl();
		}

		logtasks();
	    }
	    
	    switch (task[i].rc) {
	        case -1:
		        if (errno == ECHILD)
			    Syslog('+', "Task %d \"%s\" is ready", i, task[i].name);
			else
			    Syslog('+', "Task %d \"%s\" is ready, error: %s", i, task[i].name, strerror(errno));
			break;
		case 0:
		        /*	 
		         * Update last known status when running.	 
		         */	     
		        task[i].status = status;    
		        count++;
		        break;
		default:
		        if (WIFEXITED(task[i].status)) {
			    rc = WEXITSTATUS(task[i].status);
			    if (rc)
			        Syslog('+', "Task %s is ready, error=%d", task[i].name, rc);
			    else
			        Syslog('+', "Task %s is ready", task[i].name);
			} else if (WIFSIGNALED(task[i].status)) {
			    rc = WTERMSIG(task[i].status);
			    /*
			     * Here we don't report an error number, on FreeBSD WIFSIGNALED
			     * seems true while there's nothing wrong.
			     */
			    Syslog('+', "Task %s terminated", task[i].name);
			} else if (WIFSTOPPED(task[i].status)) {
			    rc = WSTOPSIG(task[i].status);
			    Syslog('+', "Task %s stopped on signal %s (%d)", task[i].name, SigName[rc], rc);
			}
			break;
	    }

	    /*
	     * Remove finished task from the list
	     */
	    if (!task[i].running) {
	        for (j = 0; j < MAXTASKS; j++) {
		   if (calllist[j].taskpid == task[i].pid) {
		        calllist[j].calling = FALSE;
		        calllist[j].taskpid = 0;
		        rescan = TRUE;
		    }
		}
		memset(&task[i], 0, sizeof(onetask));
	    }
	}
    }

    return count;
}



/*
 * This function triggers the shutdown and is only installed for SIGTERM
 * and SIGINT. On NetBSD the threads signal handlers cannot be disabled,
 * so in fact all threads call this function as soon as one of these
 * signals is received. The first one arrived will initiate the shutdown.
 */
void start_shutdown(int onsig)
{
    Syslog('+', "Trigger shutdown on signal %s", SigName[onsig]);
    signal(onsig, SIG_IGN);
    G_Shutdown = TRUE;
}



/*
 * Normal fatal signal handler, but also used during shutdown.
 */
void die(int onsig)
{
    int	    i, count;
    char    temp[80];

    signal(onsig, SIG_IGN);

    if (onsig < NSIG) {
	if ((onsig == SIGTERM) || (nodaemon && (onsig == SIGINT))) {
	    Syslog('+', "Starting normal shutdown (%s)", SigName[onsig]);
	} else {
	    Syslog('+', "Abnormal shutdown on signal %s", SigName[onsig]);
	}
    } else {
	Syslog('+', "Shutdown on error %d", onsig);
    }

    /*
     *  First check if there are tasks running, if so try to stop them
     */
    if ((count = checktasks(0))) {
	Syslog('+', "There are %d tasks running, sending SIGTERM", count);
	checktasks(SIGTERM);
	for (i = 0; i < 15; i++) {
	    sleep(1);
	    count = checktasks(0);
	    if (count == 0)
		break;
	}
	if (count) {
	    /*
	     *  There are some diehards running...
	     */
	    Syslog('+', "There are %d tasks running, sending SIGKILL", count);
	    count = checktasks(SIGKILL);
	}
	if (count) {
	    sleep(1);
	    count = checktasks(0);
	    if (count)
		Syslog('?', "Still %d tasks running, giving up", count);
	}
    }

    if ((count = checktasks(0)))
	Syslog('?', "Shutdown with %d tasks still running", count);
    else
	Syslog('+', "Good, no more tasks running");

    /*
     * Now check for users online and other programs not started
     * under control of mbtask.
     */
    count = 30;
    while (count) {
	reg_fre_r(temp);
	if (strcmp(temp, "100:0;") == 0) {
	    Syslog('+', "Good, no more other programs running");
	    break;
	}
	Syslog('+', "%s", temp+6);
	sleep(1);
	count--;
    }
    if ((count == 0) && strcmp(temp, "100:0;")) {
	Syslog('?', "Continue shutdown with other programs running");
    }

    /*
     * Disconnect chatservers
     */
    if (Run_IBC)
	ibc_shutdown();

    /*
     * Free memory
     */
    deinit_ping();
    deinitnl();
    deinit_diskwatch();
    unload_ports();
    ulocktask();
    printable(NULL, 0);

    /*
     * Close socket
     */
    if (sock != -1)
	close(sock);
    if (!file_exist(spath, R_OK)) {
	unlink(spath);
    }

    Syslog(' ', "MBTASK finished");
    exit(onsig);
}



/*
 *  Put a lock on this program.
 */
int locktask(char *root)
{
    char    *tempfile, *lockfile;
    FILE    *fp;
    pid_t   oldpid;

    tempfile = calloc(PATH_MAX, sizeof(char));
    lockfile = calloc(PATH_MAX, sizeof(char));

    snprintf(tempfile, PATH_MAX, "%s/var/run/mbtask.tmp", root);
    snprintf(lockfile, PATH_MAX, "%s/var/run/mbtask", root);

    if ((fp = fopen(tempfile, "w")) == NULL) {
	perror("mbtask");
	printf("Can't create lockfile \"%s\"\n", tempfile);
	free(tempfile);
	free(lockfile);
	return 1;
    }
    fprintf(fp, "%10u\n", getpid());
    fclose(fp);

    while (TRUE) {
	if (link(tempfile, lockfile) == 0) {
	    unlink(tempfile);
	    free(tempfile);
	    free(lockfile);
	    return 0;
	}
	if ((fp = fopen(lockfile, "r")) == NULL) {
	    perror("mbtask");
	    printf("Can't open lockfile \"%s\"\n", tempfile);
	    unlink(tempfile);
	    free(tempfile);
	    free(lockfile);
	    return 1;
	}
	if (fscanf(fp, "%u", &oldpid) != 1) {
	    perror("mbtask");
	    printf("Can't read old pid from \"%s\"\n", tempfile);
	    fclose(fp);
	    unlink(tempfile);
	    free(tempfile);
	    free(lockfile);
	    return 1;
	}
	fclose(fp);
	if (kill(oldpid,0) == -1) {
	    if (errno == ESRCH) {
		printf("Stale lock found for pid %u\n", oldpid);
		unlink(lockfile);
		/* no return, try lock again */  
	    } else {
		perror("mbtask");
		printf("Kill for %u failed\n",oldpid);
		unlink(tempfile);
		free(tempfile);
		free(lockfile);
		return 1;
	    }
	} else {
	    printf("Another mbtask is already running, pid=%u\n", oldpid);
	    unlink(tempfile);
	    free(tempfile);
	    free(lockfile);
	    return 1;
	}
    }
}



void ulocktask(void)
{
    char	    *lockfile;
    pid_t	    oldpid;
    FILE	    *fp;
    struct passwd   *pw;

    pw = getpwnam((char *)"mbse");
    lockfile = calloc(PATH_MAX, sizeof(char));
    snprintf(lockfile, PATH_MAX, "%s/var/run/mbtask", pw->pw_dir);

    if ((fp = fopen(lockfile, "r")) == NULL) {
	WriteError("$Can't open lockfile \"%s\"", lockfile);
	free(lockfile);
	return;
    }

    if (fscanf(fp, "%u", &oldpid) != 1) {
	WriteError("$Can't read old pid from \"%s\"", lockfile);
	fclose(fp);
	unlink(lockfile);
	free(lockfile);
	return;
    }

    fclose(fp);

    if (oldpid == getpid()) {
	(void)unlink(lockfile);
    }

    free(lockfile);
}



/*
 *  External Semafore Checks
 */
void test_sema(char *);
void test_sema(char *sema)
{
    if (IsSema(sema)) {
	RemoveSema(sema);
	Syslog('s', "Semafore %s detected", sema);
	sem_set(sema, TRUE);
    }
}



/*
 *  Check semafore's, system status flags etc. This is called
 *  each second to test for condition changes.
 */
void check_sema(void);
void check_sema(void)
{
    /*
     * Check UPS status.
     */
    if (IsSema((char *)"upsalarm")) {
        if (!UPSalarm)
            Syslog('!', "UPS: power failure");
	UPSalarm = TRUE;
    } else {
        if (UPSalarm)
            Syslog('!', "UPS: the power is back");
	UPSalarm = FALSE; 
    }
    if (IsSema((char *)"upsdown")) {
	Syslog('!', "UPS: power failure, starting shutdown");
	UPSdown = TRUE;
	/*
	 *  Since the upsdown semafore is permanent, the system WILL go down
	 *  there is no point for this program to stay. Signal all tasks and stop.
	 */
	die(MBERR_UPS_ALARM);
    }

    /*
     *  Check Zone Mail Hour
     */
    get_zmh();

    /*
     *  Semafore's that still can be detected, usefull for
     *  external programs that create them.
     */
    test_sema((char *)"newnews");
    test_sema((char *)"mailout");
    test_sema((char *)"mailin");
    test_sema((char *)"scanout");
}



void start_scheduler(int port)
{
    struct passwd   *pw;
    char            *cmd = NULL, ipstr[INET6_ADDRSTRLEN];
    
    if (nodaemon)
	printf("init fidonet\n");

    InitFidonet();
    if (nodaemon)
        printf("done\n");

    /*
     * Registrate this server for mbmon in slot 0.
     */
    reginfo[0].pid = getpid();
    strcpy(reginfo[0].tty,   "-");
    strcpy(reginfo[0].uname, "mbse");
    strcpy(reginfo[0].prg,   "mbtask");
    strcpy(reginfo[0].city,  "localhost");
    strcpy(reginfo[0].doing, "Start");
    reginfo[0].started = (int)time(NULL);
    if (nodaemon)
        printf("reginfo filled\n");

    Processing = TRUE;
    TouchSema((char *)"mbtask.last");

    /*
     * Setup UNIX Datagram socket
     */
    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
	WriteError("$Can't create socket");
	die(MBERR_INIT_ERROR);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, spath);

    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
	close(sock);
	sock = -1;
	WriteError("$Can't bind socket %s", spath);
	die(MBERR_INIT_ERROR);
    }
    if (nodaemon)
        printf("sockets created\n");

    /*
     * Setup IBC socket
     */
    if (Run_IBC) {
	void	*addr;

	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = port;
	addr = &(myaddr_in.sin_addr);
	inet_ntop(AF_INET, addr, ipstr, sizeof ipstr);
	Syslog('+', "IBC: listen on %s, port %d", ipstr, ntohs(myaddr_in.sin_port));

	ibcsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (ibcsock == -1) {
	    WriteError("$IBC: can't create listen socket");
	    die(MBERR_INIT_ERROR);
	}

	if (bind(ibcsock, (struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1) {
	    WriteError("$IBC: can't bind listen socket");
	    die(MBERR_INIT_ERROR);
	}

	srand(getpid());
	resettime = time(NULL) + (time_t)86400;
    }

    /*
     * The flag masterinit is set if a new config.data is created, this
     * is true if mbtask is started the very first time. Then we run
     * mbsetup init to create the default databases.
     */
    if (masterinit) {
	pw = getpwuid(getuid());
	cmd = xstrcpy(pw->pw_dir);
	cmd = xstrcat(cmd, (char *)"/bin/mbsetup");
	launch(cmd, (char *)"init", (char *)"mbsetup", MBINIT);
	free(cmd);
	sleep(2);
	masterinit = FALSE;
    }

    if (nodaemon)
        printf("init nodelists\n");

    initnl();
    sem_set((char *)"scanout", TRUE);
    if (!TCFG.max_tcp && !pots_lines && !isdn_lines) {
	Syslog('?', "WARNING: this system cannot connect to other systems, check setup");
    }

    /*
     * Run the scheduler
     */
    scheduler();
    die(SIGTERM);
}



/*
 * Scheduler loop
 */
void scheduler(void)
{
    struct passwd	*pw;
    int			rlen, rc, running = 0, i, found, call_work = 0, len;
    static int		LOADhi = FALSE, oldmin = 70, olddo = 70, oldsec = 70, call_entry = MAXTASKS;
    char		*cmd = NULL, *opts, port[21], crbuf[512];
    static char		doing[32], buf[2048];
    time_t		now;
    struct tm		tm, utm;
#if defined(__linux__)
    FILE		*fp;
#endif
    double		loadavg[3];
    pp_list		*tpl;
    struct pollfd	pfd[3];
    socklen_t		sl;
    struct sockaddr_in	ffrom;

    Syslog('+', "Starting scheduler loop");
    pw = getpwuid(getuid());

    /*
     * Enter the mainloop (forever)
     */
    do {
	/*
	 * Poll UNIX Datagram socket and IBC UDP socket until the defined timeout of one second.
	 * This means we listen if a MBSE BBS client program has something to tell us.
	 * Timeout is one second, after the timeout the rest of the mainloop is executed.
	 */
	pfd[0].fd = sock;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;
	pfd[1].fd = ping_isocket;
	pfd[1].events = POLLIN;
	pfd[1].revents = 0;
	if (Run_IBC) {
	    pfd[2].fd = ibcsock;
	    pfd[2].events = POLLIN;
	    pfd[2].revents = 0;
	}

	if (Run_IBC)
	    rc = poll(pfd, 3, 1000);
	else
	    rc = poll(pfd, 2, 1000);

        if (rc == -1) {
	    /* 
	     *  Poll can be interrupted by a finished child so that's not a real error.
	     */
	    if (errno != EINTR) {
		Syslog('?', "$poll() rc=%d sock=%d, events=%04x", rc, sock, pfd[0].revents);
		Syslog('?', "$poll() rc=%d sock=%d, events=%04x", rc, ibcsock, pfd[1].revents);
		if (Run_IBC)
		    Syslog('?', "$poll() rc=%d sock=%d, events=%04x", rc, ping_isocket, pfd[2].revents);
	    }
        } else if (rc) {
	    if (pfd[0].revents & POLLIN) {
		/*
		 * Process the clients request for mbtask commands.
		 */
		memset(&buf, 0, sizeof(buf));
		fromlen = sizeof(from);
		rlen = recvfrom(sock, buf, sizeof(buf) -1, 0, (struct sockaddr *)&from, &fromlen);
		if (rlen == -1) {
		    WriteError("$recvfrom() for command receiver");
		} else {
		    do_cmd(buf);
		}
	    }
	    if (pfd[1].revents & POLLIN || pfd[1].revents & POLLERR || pfd[1].revents & POLLHUP || pfd[1].revents & POLLNVAL) {
		/*
		 * Ping reply received.
		 */
		sl = sizeof(ffrom);
		if ((len = recvfrom(ping_isocket, &buf, sizeof(buf)-1, 0,(struct sockaddr *)&ffrom, &sl)) != -1) {
		    ping_receive(buf, len);
		} else {
		    WriteError("$recvfrom() for ping receiver");
		}
	    } 
	    if (Run_IBC && (pfd[2].revents & POLLIN || pfd[2].revents & POLLERR || 
		 pfd[2].revents & POLLHUP || pfd[2].revents & POLLNVAL)) {
		/*
		 * IBC chat command received.
		 */
		sl = sizeof(myaddr_in);
		memset(&clientaddr_in, 0, sizeof(struct sockaddr_in));
		memset(&crbuf, 0, sizeof(crbuf));
		if ((len = recvfrom(ibcsock, &crbuf, sizeof(crbuf)-1, 0,(struct sockaddr *)&clientaddr_in, &sl)) != -1) {
		    ibc_receiver(crbuf);
		} else {
		    WriteError("$recvfrom() for IBC receiver");
		}
	    }
	}

	if (G_Shutdown)
	    break;

	/*
	 * Check all registered connections and semafore's
	 */
	if (Run_IBC)
	    check_servers();

	reg_check();
	check_sema();
	check_ports();
	diskwatch();
	check_ping();

	/*
	 * Check the systems load average.
	 */
	Load = loadavg[0] = loadavg[1] = loadavg[2] = 0.0;
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	if (getloadavg(loadavg, 3) == 3) {
	    Load = loadavg[0];
	}
#elif defined(__linux__)
	if ((fp = fopen((char *)"/proc/loadavg", "r"))) {
	    if (fscanf(fp, "%lf %lf %lf", &loadavg[0], &loadavg[1], &loadavg[2]) == 3) {
		Load = loadavg[0];
	    } else {
		Syslog('-', "error");
	    }
	    fclose(fp);
	}
#else
#error "Don't know how to get the loadaverage on this OS"
#endif
	if (Load >= TCFG.maxload) {
	    if (!LOADhi) {
		Syslog('!', "System load too high: %2.2f (%2.2f)", Load, TCFG.maxload);
		LOADhi = TRUE;
	    }
	} else {
	    if (LOADhi) {
		Syslog('!', "System load normal: %2.2f (%2.2f)", Load, TCFG.maxload);
		LOADhi = FALSE;
	    }
	}

	/*
	 * Report to the system monitor. 
	 */
	memset(&doing, 0, sizeof(doing));
	if ((running = checktasks(0)))
	    snprintf(doing, 32, "Run %d tasks", running);
	else if (UPSdown)
	    snprintf(doing, 32, "UPS shutdown");
	else if (UPSalarm)
	    snprintf(doing, 32, "UPS alarm");
	else if (!s_bbsopen)
	    snprintf(doing, 32, "BBS is closed");
	else if (Processing)
	    snprintf(doing, 32, "%s", waitmsg);
	else
	    snprintf(doing, 32, "Overload %2.2f", Load);

	snprintf(reginfo[0].doing, 36, "%s", doing);
	reginfo[0].lastcon = (int)time(NULL);

	/*
	 *  Touch the mbtask.last semafore to prove this daemon
	 *  is actually running.
	 *  Reload configuration data if some file is changed.
	 */
	now = time(NULL);
	localtime_r(&now, &tm);
	gmtime_r(&now, &utm);

	if (tm.tm_min != olddo) {
	    /*
	     * Each minute we execute this part
	     */
	    if (tosswait)
		tosswait--;
	    olddo = tm.tm_min;
	    TouchSema((char *)"mbtask.last");
	    if (file_time(tcfgfn) != tcfg_time) {
		Syslog('+', "Task configuration changed, reloading");
		load_taskcfg();
		deinitnl();
		initnl();
		sem_set((char *)"scanout", TRUE);
	    }
	    if (file_time(cfgfn) != cfg_time) {
		Syslog('+', "Main configuration changed, reloading");
		load_maincfg();
		deinitnl();
		initnl();
		sem_set((char *)"scanout", TRUE);
	    }
	    if (file_time(ttyfn) != tty_time) {
		Syslog('+', "Ports configuration changed, reloading");
		load_ports();
		check_ports();
		deinitnl();
		initnl();
		sem_set((char *)"scanout", TRUE);
	    }

	    /*
	     * If the next event time is reached, rescan the outbound
	     */
	    if ((utm.tm_hour == nxt_hour) && (utm.tm_min == nxt_min)) {
		Syslog('+', "It is now %02d:%02d UTC, starting new event", utm.tm_hour, utm.tm_min);
		sem_set((char *)"scanout", TRUE);
	    }
	}

	/*
	 * Check system processing state
	 */
	if (s_bbsopen && !UPSalarm && !LOADhi) {
	    if (!Processing) {
		Syslog('+', "Resuming normal operations");
		Processing = TRUE;
	    }
        } else {
	    if (Processing) {
		Syslog('+', "Suspending operations");
		Processing = FALSE;
	    }
	}

	/*
	 * Check Pause Timer, make sure it's only checked
	 * once each second. Also do pingcheck.
	 */
	if (tm.tm_sec != oldsec) {
	    oldsec = tm.tm_sec;
	    if (ptimer)
		ptimer--;
	}

	if (Processing) {
	    /*
	     *  Here we run all normal operations.
	     */
	    running = checktasks(0);

	    if (s_mailout && (!ptimer) && (!runtasktype(MBFIDO))) {
	        launch(TCFG.cmd_mailout, NULL, (char *)"mailout", MBFIDO);
	        running = checktasks(0);
		s_mailout = FALSE;
	    }

	    if (s_mailin && (!ptimer) && (!runtasktype(MBFIDO))) {
		/*
		 * Here we should check if no mailers are running. Only start
		 * processing the inbound if no mailers are running, but on busy
		 * systems this may hardly be the case. So we wait for 30 minutes
		 * and if there are still mailers running, mbfido is started
		 * anyway.
		 */
		if ((ipmailers + runtasktype(CM_ISDN) + runtasktype(CM_POTS)) == 0) {
		    Syslog('i', "Mailin, no mailers running, start direct");
		    tosswait = TOSSWAIT_TIME;
		    launch(TCFG.cmd_mailin, NULL, (char *)"mailin", MBFIDO);
		    running = checktasks(0);
		    s_mailin = FALSE;
		} else {
		    Syslog('i', "Mailin, tosswait=%d", tosswait);
		    if (tosswait == 0) {
		        launch(TCFG.cmd_mailin, NULL, (char *)"mailin", MBFIDO);
		        running = checktasks(0);
		        s_mailin = FALSE;
		    }
		}
	    }

	    if (s_newnews && (!ptimer) && (!runtasktype(MBFIDO))) {
	        launch(TCFG.cmd_newnews, NULL, (char *)"newnews", MBFIDO);
	        running = checktasks(0);
	        s_newnews = FALSE;
	    }

	    /*
	     *  Only run the nodelist compiler if nothing else
	     *  is running. There's no hurry to compile the
	     *  new lists. If more then one compiler is defined,
	     *  start them in parallel.
	     */
	    if (s_index && (!ptimer) && (!running)) {
		if (strlen(TCFG.cmd_mbindex1)) {
		    launch(TCFG.cmd_mbindex1, NULL, (char *)"compiler 1", MBINDEX);
		}
		if (strlen(TCFG.cmd_mbindex2)) {
		    launch(TCFG.cmd_mbindex2, NULL, (char *)"compiler 2", MBINDEX);
		}
		if (strlen(TCFG.cmd_mbindex3)) {
		    launch(TCFG.cmd_mbindex3, NULL, (char *)"compiler 3", MBINDEX);
		}
		running = checktasks(0);
		s_index = FALSE;
	    }

	    /*
	     *  Linking messages is also only done when there is
	     *  nothing else to do.
	     */
	    if (s_msglink && (!ptimer) && (!running)) {
		launch(TCFG.cmd_msglink, NULL, (char *)"msglink", MBFIDO);
		running = checktasks(0);
		s_msglink = FALSE;
	    }

	    /*
	     *  Creating filerequest indexes, also only if nothing to do.
	     */
	    if (s_reqindex && (!ptimer) && (!running)) {
		launch(TCFG.cmd_reqindex, NULL, (char *)"reqindex", MBFILE);
		running = checktasks(0);
		s_reqindex = FALSE;
	    }

	} /* if (Processing) */

	if ((tm.tm_sec / SLOWRUN) != oldmin) {

	    /*
	     *  These tasks run once per 20 seconds.
	     */
	    oldmin = tm.tm_sec / SLOWRUN;

	    if (Processing) {

		/*
		 * Update outbound status if needed.
		 */
		if (rescan) {
		    rescan = FALSE;
		    outstat();
		    call_work = check_calllist();
		}

		/*
		 * Launch the systems to call, start one system each time.
		 * Set the safety counter to MAXTASKS + 1, this forces that
		 * the counter really will advance to the next node in case
		 * of failing sessions.
		 */
		i = MAXTASKS + 1;
		found = FALSE;
		if (call_work) {
		    while (TRUE) {
			/*
			 * Rotate the call entries
			 */
			if (call_entry == MAXTASKS)
			    call_entry = 0;
			else
			    call_entry++;

			/*
			 * If a valid entry, and not yet calling, and the retry time is reached,
			 * then launch a callprocess for this node.
			 */
			if (calllist[call_entry].addr.zone && !calllist[call_entry].calling && 
				(calllist[call_entry].cst.trytime < (int)now)) {
			    if ((calllist[call_entry].callmode == CM_INET) && (ipmailers < TCFG.max_tcp) && internet) {
				found = TRUE;
				break;
			    }
			    if ((calllist[call_entry].callmode == CM_ISDN) && (runtasktype(CM_ISDN) < isdn_free)) {
				found = TRUE;
				break;
			    }
			    if ((calllist[call_entry].callmode == CM_POTS) && (runtasktype(CM_POTS) < pots_free)) {
				found = TRUE;
				break;
			    }
			}

			/*
			 * Safety counter, if all systems are already calling, we should
			 * never break out of this loop anymore.
			 */
			i--;
			if (!i)
			    break;
		    }
		    if (found) {
			cmd = xstrcpy(pw->pw_dir);
			cmd = xstrcat(cmd, (char *)"/bin/mbcico");
			/*
			 * For ISDN or POTS, select a free tty device.
			 */
			switch (calllist[call_entry].callmode) {
			    case CM_ISDN:   for (tpl = pl; tpl; tpl = tpl->next) {
						if (!tpl->locked && (tpl->dflags  & calllist[call_entry].diflags)) {
						    snprintf(port, 21, "-l %s ", tpl->tty);
						    break;
						}
					    }
					    break;
			    case CM_POTS:   for (tpl = pl; tpl; tpl = tpl->next) {
						if (!tpl->locked && (tpl->mflags  & calllist[call_entry].moflags)) {
						    snprintf(port, 21, "-l %s ", tpl->tty);
						    break;
						}
					    }
					    break;
			    default:	    port[0] = '\0';
					    break;
			}
			opts = calloc(41, sizeof(char));
			if (calllist[call_entry].addr.point) {
			    snprintf(opts, 41, "%sp%u.f%u.n%u.z%u.%s", port, calllist[call_entry].addr.point, 
				    calllist[call_entry].addr.node, calllist[call_entry].addr.net, 
				    calllist[call_entry].addr.zone, calllist[call_entry].addr.domain);
			} else {
			    snprintf(opts, 41, "%sf%u.n%u.z%u.%s", port, calllist[call_entry].addr.node, 
				    calllist[call_entry].addr.net,
				    calllist[call_entry].addr.zone, calllist[call_entry].addr.domain);
			}
			calllist[call_entry].taskpid = launch(cmd, opts, (char *)"mbcico", calllist[call_entry].callmode);
			if (calllist[call_entry].taskpid)
			    calllist[call_entry].calling = TRUE;
			running = checktasks(0);
			rescan = TRUE;
			free(opts);
			free(cmd);
			cmd = NULL;
		    }
		}
	    } /* if (Processing) */
	} /* if ((tm->tm_sec / SLOWRUN) != oldmin) */

    } while (! G_Shutdown);

    Syslog('+', "Scheduler stopped");
}



int main(int argc, char **argv)
{
    struct passwd   *pw;
    char	    *lockfile;
    int             i, chatport = 0;
    pid_t           frk;
    FILE            *fp;
    struct servent  *se;

    /*
     * Print copyright notices and setup logging.
     */
    printf("MBTASK: MBSE BBS v%s Task Manager Daemon\n", VERSION);
    printf("        %s\n\n", COPYRIGHT);

    /*
     *  Catch all the signals we can, and ignore the rest. Note that SIGKILL can't be ignored
     *  but that's live. This daemon should only be stopped by SIGTERM.
     */
    for (i = 0; i < NSIG; i++) {
        if ((i == SIGHUP) || (i == SIGPIPE) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGIOT))
            signal(i, (void (*))die);
	else if ((i == SIGINT) || (i == SIGTERM))
	    signal(i, (void (*))start_shutdown);
	else
	    signal(i, SIG_IGN);
    }

    /*
     * Secret undocumented startup switch, ie: no help available.
     */
    if ((argc == 2) && (strcmp(argv[1], "-nd") == 0)) {
	nodaemon = TRUE;
    }

    init_pingsocket();

    /*
     *  mbtask is setuid root, drop privileges to user mbse.
     *  This will stay forever like this, no need to become
     *  root again. The child can't even become root anymore.
     */
    pw = getpwnam((char *)"mbse");
    if (setuid(pw->pw_uid)) {
	perror("");
	fprintf(stderr, "can't setuid to mbse\n");
	close(ping_isocket);
	exit(MBERR_INIT_ERROR);
    }

    /*
     * If running in the foreground under valgrind the next call fails.
     * Developers should know what they are doing so we are not bailing out.
     */
    if (setgid(pw->pw_gid)) {
	perror("");
	fprintf(stderr, "can't setgid to bbs\n");
	if (! nodaemon) {
	    close(ping_isocket);
	    exit(MBERR_INIT_ERROR);
	}
    }

    umask(007);
    if (locktask(pw->pw_dir)) {
	close(ping_isocket);
        exit(MBERR_NO_PROGLOCK);
    }

    snprintf(cfgfn, PATH_MAX, "%s/etc/config.data", getenv("MBSE_ROOT"));
    load_maincfg();
    if (nodaemon)
	printf("main config loaded\n");

    mypid = getpid();
    if (nodaemon)
	printf("my pid is %d\n", mypid);

    Syslog(' ', " ");
    Syslog(' ', "MBTASK v%s", VERSION);

    if (nodaemon)
	Syslog('+', "Starting in no-daemon mode");

    snprintf(tcfgfn, PATH_MAX, "%s/etc/task.data", getenv("MBSE_ROOT"));
    load_taskcfg();

    status_init();

    if ((se = getservbyname("fido", "udp")) == NULL) {
	WriteError("IBC: no fido udp entry in /etc/services, cannot start Internet BBS Chat");
	Run_IBC = FALSE;
    } else {
	chatport = se->s_port;
	if (strlen(CFG.bbs_name) == 0) {
	    WriteError("IBC: mbsetup 1.2.1 is empty, cannot start Internet BBS Chat");
	    Run_IBC = FALSE;
	} else if (strlen(CFG.myfqdn) == 0) {
	    Run_IBC = FALSE;
	    WriteError("IBC: mbsetup 1.2.10 is empty, cannot start Internet BBS Chat");
	}
    }

    memset(&task, 0, sizeof(task));
    memset(&reginfo, 0, sizeof(reginfo));
    memset(&calllist, 0, sizeof(calllist));
    snprintf(spath, PATH_MAX, "%s/tmp/mbtask", getenv("MBSE_ROOT"));
    snprintf(ttyfn, PATH_MAX, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
    initnl();
    load_ports();
    check_ports();
    chat_init();
    ibc_init();

    /*
     * Now that init is complete and this program is locked, it is
     * safe to remove a stale socket if it is there after a crash.
     */
    if (!file_exist(spath, R_OK))
        unlink(spath);

    if (nodaemon) {
	/*
	 * For debugging, run in foreground mode
	 */
	mypid = getpid();
	printf("init complete, starting scheduler ...\n");
	start_scheduler(chatport);
    } else {
	/*
	 * Server initialization is complete. Now we can fork the 
	 * daemon and return to the user. We need to do a setpgrp
	 * so that the daemon will no longer be assosiated with the
	 * users control terminal. This is done before the fork, so
	 * that the child will not be a process group leader. Otherwise,
	 * if the child were to open a terminal, it would become
	 * associated with that terminal as its control terminal.
	 */
	if ((pgrp = setpgid(0, 0)) == -1) {
	    Syslog('t', "$setpgid failed");
	    /*
	     * Just log this, some SElinux versions don't allow it.
	     */
//	    die(MBERR_INIT_ERROR);
	}

	frk = fork();
	switch (frk) {
	case -1:
		Syslog('?', "$Unable to fork daemon");
		die(MBERR_INIT_ERROR);
	case 0:
		/*
		 *  Starting the deamon child process here. 
		 */
		fclose(stdin);
		if (open("/dev/null", O_RDONLY) != 0) {
		    Syslog('?', "$Reopen of stdin to /dev/null failed");
		    _exit(MBERR_EXEC_FAILED);
		}
		fclose(stdout);
		if (open("/dev/null", O_WRONLY | O_APPEND | O_CREAT,0600) != 1) {
		    Syslog('?', "$Reopen of stdout to /dev/null failed");
		    _exit(MBERR_EXEC_FAILED);
		}
		fclose(stderr);
		if (open("/dev/null", O_WRONLY | O_APPEND | O_CREAT,0600) != 2) {
		    Syslog('?', "$Reopen of stderr to /dev/null failed");
		    _exit(MBERR_EXEC_FAILED);
		}
		mypid = getpid();
		start_scheduler(chatport);
		/* Not reached */
	default:
		/*
		 * Here we detach this process and let the child
		 * run the deamon process. Put the child's pid
		 * in the lockfile before leaving.
		 */
		lockfile = calloc(PATH_MAX, sizeof(char));
		snprintf(lockfile, PATH_MAX, "%s/var/run/mbtask", pw->pw_dir);
		if ((fp = fopen(lockfile, "w"))) {
		    fprintf(fp, "%10u\n", frk);
		    fclose(fp);
		}
		free(lockfile);
		Syslog('+', "Starting daemon with pid %d", frk);
		exit(MBERR_OK);
	}
    }

    /*
     *  Not reached in daemon mode.
     */
    return 0;
}



