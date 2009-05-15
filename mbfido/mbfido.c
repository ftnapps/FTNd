/*****************************************************************************
 *
 * $Id: mbfido.c,v 1.50 2007/09/02 11:17:32 mbse Exp $
 * Purpose: Process Fidonet style mail and files.
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "../lib/nodelist.h"
#include "../lib/mbsedb.h"
#include "../lib/msg.h"
#include "flock.h"
#include "tosspkt.h"
#include "unpack.h"
#include "orphans.h"
#include "tic.h"
#include "fsort.h"
#include "scan.h"
#include "mbfido.h"
#include "tracker.h"
#include "notify.h"
#include "rollover.h"
#include "hatch.h"
#include "scannews.h"
#include "maketags.h"
#include "makestat.h"
#include "newspost.h"
#include "rnews.h"
#include "mgrutil.h"
#include "backalias.h"
#include "rfc2ftn.h"
#include "dirsession.h"
#include "dirlock.h"
#include "queue.h"
#include "msg.h"
#include "createm.h"


#define	UNPACK_FACTOR 300


int	do_obs_a   = FALSE;		/* Obsolete command used	    */
int	do_areas   = FALSE;		/* Process area taglists	    */
int	do_toss    = FALSE;		/* Toss flag			    */
int	do_scan    = FALSE;		/* Scan flag			    */
int	do_tic     = FALSE;		/* Process .tic files		    */
int	do_notify  = FALSE;		/* Create notify messages	    */
int	do_roll	   = FALSE;		/* Rollover only		    */
int	do_full    = FALSE;		/* Full mailscan		    */
int	do_tags    = FALSE;		/* Create taglists		    */
int	do_stat    = FALSE;		/* Create statistic HTML pages	    */
int	do_test    = FALSE;		/* Test routing			    */
int	do_news    = FALSE;		/* Process NNTP news		    */
int	do_uucp    = FALSE;		/* Process UUCP newsbatch	    */
int	do_mail    = FALSE;		/* Process MTA email message	    */
int	do_unsec   = FALSE;		/* Unsecure tossing		    */
int	do_learn   = FALSE;		/* News articles learnmode	    */
int	check_crc  = TRUE;		/* Check .tic crc values	    */
int	check_dupe = TRUE;		/* Check duplicates		    */
int	do_flush   = FALSE;		/* Flush outbound queue		    */
int	flushed    = FALSE;		/* If anything was flushed	    */
int	areas_changed = FALSE;		/* If echoareas are changed	    */
extern	int do_quiet;			/* Quiet flag			    */
extern	int e_pid;			/* Pid of child process		    */
extern	int show_log;			/* Show logging on screen	    */
int	do_unprot  = FALSE;		/* Unprotected inbound flag	    */
time_t	t_start;			/* Start time			    */
time_t	t_end;				/* End time			    */
int	packets    = 0;			/* Tossed packets		    */
int	packets_ok = 0;			/* Tossed packets Ok.		    */
char	*envptr = NULL;

extern	int net_in, net_imp, net_out, net_bad, net_msgs;
extern	int echo_in, echo_imp, echo_out, echo_bad, echo_dupe;
extern	int email_in, email_imp, email_out, email_bad;
extern	int news_in, news_imp, news_out, news_bad, news_dupe;
extern	int tic_in, tic_imp, tic_out, tic_bad, tic_dup;
extern	int Magics, Hatched;
extern	int notify, filemgr, areamgr;

void editor_configs(void);


/*
 * If we don't know what to type
 */
void Help(void)
{
    do_quiet = FALSE;
    ProgName();

    mbse_colour(LIGHTCYAN, BLACK);
    printf("\nUsage: mbfido [command(s)] <options>\n\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("	Commands are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	a    areas			Process Areas taglists\n");
    printf("	m    mail <recipient> ...	MTA Mail mode\n");
    printf("	ne   news			Scan for new news\n");
    printf("	no   notify <nodes>		Send notify messages\n");
    printf("	r    roll			Rollover statistic counters\n");
    printf("	s    scan			Scan outgoing Fido mail\n");
    printf("	ta   tag			Create taglists\n");
    printf("	te   test <node>		Do routing test for node\n");
    printf("	ti   tic			Process .tic files\n");
    printf("	to   toss			Toss incoming Fido mail\n");
    printf("	u    uucp	 		Process UUCP batchfile\n");
    printf("	w    web			Create WWW statistics\n\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("	Options are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	-f   -full			Full Mailscan\n");
    printf("	-l   -learn			Learn News dupes\n");
    printf("	-noc -nocrc			Skip CRC checking\n");
    printf("	-nod -nodupe			Skip dupe checking\n");
    printf("	-q   -quiet			Quiet mode\n");
    printf("	-uns -unsecure			Toss unsecure\n");
    printf("	-unp -unprotect			Toss unprotected inbound\n");
    mbse_colour(LIGHTGRAY, BLACK);
    ExitClient(MBERR_COMMANDLINE);
}



/*
 * Header, only if not quiet.
 */
void ProgName(void)
{
    if (do_quiet)
	return;

    mbse_colour(WHITE, BLACK);
    printf("\nMBFIDO: MBSE BBS %s - Fidonet File and Mail processor\n", VERSION);
    mbse_colour(YELLOW, BLACK);
    printf("        %s\n", COPYRIGHT);
}



/*
 * Create ~/etc/msg.txt for MsgEd and ~/etc/golded.inc for GoldED.
 */
void editor_configs(void)
{
    char    *temp;
    FILE    *fp;
    int	    i;

    temp = calloc(PATH_MAX, sizeof(char));

    /*
     * Export ~/etc/msg.txt for MsgEd.
     */
    snprintf(temp, PATH_MAX, "%s/etc/msg.txt", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "w")) != NULL) {
	fprintf(fp, "; msg.txt -- Automatic created by mbsetup %s -- Do not edit!\n;\n", VERSION);
	fprintf(fp, "; Mail areas for MsgEd.\n;\n");
	msged_areas(fp);
	fclose(fp);
	Syslog('+', "Created new %s", temp);
    } else {
	WriteError("$Could not create %s", temp);
    }

    /*
     * Export ~/etc/golded.inc for GoldED
     */
    snprintf(temp, PATH_MAX, "%s/etc/golded.inc", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "w")) != NULL) {
	fprintf(fp, "; GoldED.inc -- Automatic created by mbsetup %s -- Do not edit!\n\n", VERSION);
	fprintf(fp, "; Basic information.\n;\n");
	if (strlen(CFG.sysop_name) && CFG.akavalid[0] && CFG.aka[0].zone) {
	    fprintf(fp, "USERNAME %s\n\n", CFG.sysop_name);
	    fprintf(fp, "ADDRESS %s\n", aka2str(CFG.aka[0]));
	    for (i = 1; i < 40; i++)
		if (CFG.akavalid[i])
		    fprintf(fp, "AKA     %s\n", aka2str(CFG.aka[i]));
		fprintf(fp, "\n");

	    gold_akamatch(fp);
	    fprintf(fp, "; JAM MessageBase Setup\n;\n");
	    fprintf(fp, "JAMPATH %s/tmp/\n", getenv("MBSE_ROOT"));
	    fprintf(fp, "JAMHARDDELETE NO\n\n");

	    fprintf(fp, "; Semaphore files\n;\n");
	    fprintf(fp, "SEMAPHORE NETSCAN    %s/var/sema/mailout\n", getenv("MBSE_ROOT"));
	    fprintf(fp, "SEMAPHORE ECHOSCAN   %s/var/sema/mailout\n\n", getenv("MBSE_ROOT"));

	    gold_areas(fp);
	}
	fclose(fp);
	Syslog('+', "Created new %s", temp);
    } else {
	WriteError("$Could not create %s", temp);
    }

    free(temp);
}



void die(int onsig)
{
    /*
     * First check if there is a child running, if so, kill it.
     */
    if (e_pid) {
	if ((kill(e_pid, SIGTERM)) == 0)
	    Syslog('+', "SIGTERM to pid %d succeeded", e_pid);
	else {
	    if ((kill(e_pid, SIGKILL)) == 0)
		Syslog('+', "SIGKILL to pid %d succeeded", e_pid);
	    else
		WriteError("Failed to kill pid %d", e_pid);
	}

	/*
	 * In case the child had the tty in raw mode, reset the tty.
	 */
	execute_pth((char *)"stty", (char *)"sane", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
    }

    if (onsig != MBERR_NO_PROGLOCK)
	CloseDupes();

    /*
     * Check for locked and open message base.
     */
    if (MsgBase.Locked)
	Msg_UnLock();
    if (MsgBase.Open)
	Msg_Close();

    signal(onsig, SIG_IGN);
    deinitnl();
    clean_tmpwork();

    if (!do_quiet) {
	show_log = TRUE;
	mbse_colour(CYAN, BLACK);
    }

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    WriteError("Terminated with error %d", onsig);
    } else {
	if (areas_changed)
	    editor_configs();
    }

    if (echo_imp + net_imp + net_out + echo_out)
	CreateSema((char *)"msglink");

    if ((echo_out + net_out + tic_out) || flushed)
	CreateSema((char *)"scanout");

    if (tic_imp)
	CreateSema((char *)"reqindex");

    if (net_in + net_imp + net_out + net_bad + net_msgs) {
	Syslog('+', "Netmail  [%4d] import [%4d] out [%4d] bad [%4d] msgs [%4d]", net_in, net_imp, net_out, net_bad, net_msgs);
	SockS("MSTN:3,%d,%d,%d;", net_in, net_out, net_bad);
    }
    if (email_in + email_imp + email_out + email_bad) {
	Syslog('+', "Email    [%4d] import [%4d] out [%4d] bad [%4d]", email_in, email_imp, email_out, email_bad);
	SockS("MSTI:3,%d,%d,%d;", email_in, email_out, email_bad);
    }
    if (echo_in + echo_imp + echo_out + echo_bad + echo_dupe) {
	Syslog('+', "Echomail [%4d] import [%4d] out [%4d] bad [%4d] dupe [%4d]", echo_in, echo_imp, echo_out, echo_bad, echo_dupe);
	SockS("MSTE:4,%d,%d,%d,%d;", echo_in, echo_out, echo_bad, echo_dupe);
    }
    if (news_in + news_imp + news_out + news_bad + news_dupe) {
	Syslog('+', "News     [%4d] import [%4d] out [%4d] bad [%4d] dupe [%4d]", news_in, news_imp, news_out, news_bad, news_dupe);
	SockS("MSTR:4,%d,%d,%d,%d;", news_in, news_out, news_bad, news_dupe);
    }
    if (tic_in + tic_imp + tic_out + tic_bad + tic_dup)
	Syslog('+', "TICfiles [%4d] import [%4d] out [%4d] bad [%4d] dupe [%4d]", tic_in, tic_imp, tic_out, tic_bad, tic_dup);
    if (Magics + Hatched)
	Syslog('+', "  Magics [%4d]  hatch [%4d]", Magics, Hatched);
    if (tic_in + tic_imp + tic_out + tic_bad + tic_dup + Magics + Hatched)
	SockS("MSTF:6,%d,%d,%d,%d,%d,%d;", tic_in, tic_out, tic_bad, tic_dup, Magics, Hatched);
    if (notify + areamgr + filemgr)
	Syslog('+', "Notify msgs [%4d] AreaMgr [%4d] FileMgr [%4d]", notify, areamgr, filemgr);

    /*
     * There should be no locks anymore, but in case of a crash try to unlock
     * all possible directories. Only if onsig <> 110, this was a lock error
     * and there should be no lock. We prevent removing the lock of another
     * mbfido this way.
     */
    if (onsig != MBERR_NO_PROGLOCK) {
	ulockdir(CFG.inbound);
	ulockdir(CFG.pinbound);
	ulockdir(CFG.out_queue);
    }

    t_end = time(NULL);
    Syslog(' ', "MBFIDO finished in %s", t_elapsed(t_start, t_end));

    if (!do_quiet)
	mbse_colour(LIGHTGRAY, BLACK);
    ExitClient(onsig);
}



int main(int argc, char **argv)
{
    int		    i, x, Loop, envrecip_count = 0;
    char	    *p, *cmd, *temp, Options[81];
    struct passwd   *pw;
    struct tm	    *t;
    fa_list	    **envrecip, *envrecip_start = NULL;
    faddr	    *taddr = NULL;
    FILE	    *ofp;


    /*
     * The next trick is to supply a fake environment variable
     * MBSE_ROOT in case we are started from UUCP or the MTA.
     * this will setup the variable so InitConfig() will work.
     * The /etc/passwd must point to the correct homedirectory.
     * Some programs can't set uid to mbse, so mbfido is installed
     * setuid mbse.
     */
    if (getenv("MBSE_ROOT") == NULL) {
	pw = getpwuid(getuid());
	if (strcmp(pw->pw_name, "mbse")) {
	    /*
	     *  We are not running as user mbse.
	     */
	    pw = getpwnam("mbse");
	    if (setuid(pw->pw_uid)) {
		printf("Fatal error: can't set uid to user mbse\n");
	    }
	}
	envptr = xstrcpy((char *)"MBSE_ROOT=");
	envptr = xstrcat(envptr, pw->pw_dir);
	putenv(envptr);
    }

    InitConfig();
    memset(&Options, 0, sizeof(Options));

    /*
     * Initialize global variables, data records.
     */
    InitNode();
    InitMsgs();
    InitTic();
    InitUser();
    InitFidonet();
    mbse_TermInit(1, 80, 25);
    t_start = time(NULL);
    t = localtime(&t_start);
    Diw = t->tm_wday;
    Miy = t->tm_mon;
    umask(002);


    /*
     * Catch all the signals we can, and ignore the rest.
     */
    for(i = 0; i < NSIG; i++) {

	if ((i == SIGINT) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM) || (i == SIGIOT))
	    signal(i, (void (*))die);
	else if (i == SIGCHLD)
	    signal(i, SIG_DFL);
	else if ((i != SIGKILL) && (i != SIGSTOP))
	    signal(i, SIG_IGN);
    }

    if ((p = strrchr(argv[0], '/')))
	p++;
    else
	p = argv[0];
    if (!strcmp(p, "mbmail")) {
	do_quiet = TRUE;
	do_mail  = TRUE;
	cmd = xstrcpy((char *)"Cmd: mbmail");
    } else if (!strcmp(p, "mbnews")) {
	do_quiet = TRUE;
	do_uucp  = TRUE;
	cmd = xstrcpy((char *)"Cmd: mbnews");
    } else {
	if (argc < 2)
	    Help();
	cmd = xstrcpy((char *)"Cmd: mbfido");
    }

    envrecip = &envrecip_start;
    for (i = 1; i < argc; i++) {
	cmd = xstrcat(cmd, (char *)" ");
	cmd = xstrcat(cmd, argv[i]);

	if ((strncmp(tl(argv[i]), "ne", 2) == 0) && !do_mail)
	    do_news = TRUE;
	else if ((strncmp(tl(argv[i]), "no", 2) == 0) && !do_mail) {
	    do_notify = TRUE;
	    if (((i + 1) < argc) &&
                            ((strchr(argv[i + 1], ':') != NULL) || 
			     (atoi(argv[i + 1])) ||
			     (strncmp(argv[i + 1], "*", 1) == 0))) {
				snprintf(Options, 81, "%s", argv[i + 1]);
				i++;
	    }
	}
	else if ((strncmp(tl(argv[i]), "r", 1) == 0) && !do_mail)
	    do_roll = TRUE;
	else if ((strncmp(tl(argv[i]), "a", 1) == 0) && !do_mail)
	    do_areas = TRUE;
	else if ((strncmp(tl(argv[i]), "s", 1) == 0) && !do_mail)
	    do_scan = TRUE;
	else if ((strncmp(tl(argv[i]), "ta", 2) == 0) && !do_mail)
	    do_tags = TRUE;
	else if ((strncmp(tl(argv[i]), "ti", 2) == 0) && !do_mail)
	    do_tic = TRUE;
	else if ((strncmp(tl(argv[i]), "te", 2) == 0) && !do_mail) {
	    do_test = TRUE;
	    if ((i + 1) < argc) {
		if ((taddr = parsefaddr(argv[i + 1])) == NULL) {
		    Help();
		}
		i++;
		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[i]);
	    }
	} else if ((strncmp(tl(argv[i]), "to", 2) == 0) && !do_mail)
	    do_toss = TRUE;
	else if ((strncmp(tl(argv[i]), "u", 1) == 0) && !do_mail)
	    do_uucp = TRUE;
	else if ((strncmp(tl(argv[i]), "m", 1) == 0) && !do_mail)
	    do_mail = TRUE;
	else if ((strncmp(tl(argv[i]), "w", 1) == 0) && !do_mail)
	    do_stat = TRUE;
	else if (strncmp(tl(argv[i]), "-f", 2) == 0)
	    do_full = TRUE;
	else if (strncmp(tl(argv[i]), "-l", 2) == 0)
	    do_learn = TRUE;
	else if (strncmp(tl(argv[i]), "-noc", 4) == 0)
	    check_crc = FALSE;
	else if (strncmp(tl(argv[i]), "-nod", 4) == 0)
	    check_dupe = FALSE;
	else if (strncmp(tl(argv[i]), "-q", 2) == 0)
	    do_quiet = TRUE;
	else if (strncmp(tl(argv[i]), "-a", 2) == 0)
	    do_obs_a = TRUE;
	else if (strncmp(tl(argv[i]), "-unp", 4) == 0)
	    do_unprot = TRUE;
	else if (strncmp(tl(argv[i]), "-uns", 4) == 0)
	    do_unsec = TRUE;
	else if (do_mail) {
	    /*
	     *  Possible recipient address(es).
	     */
	    if ((taddr = parsefaddr(argv[i]))) {
		(*envrecip) = (fa_list*)malloc(sizeof(fa_list));
		(*envrecip)->next = NULL;
		(*envrecip)->addr = taddr;
		envrecip = &((*envrecip)->next);
		envrecip_count++;
	    } else {
		cmd = xstrcat(cmd, (char *)" <- unparsable recipient! ");
	    }
	}
    }

    if ((!do_areas) && (!do_toss) && (!do_scan) && (!do_tic) && (!do_notify) && (!do_roll) && 
    	(!do_tags) && (!do_stat) && (!do_test) && (!do_news) && (!do_uucp) && (!do_mail))
	Help();

    ProgName();
    pw = getpwuid(getuid());
    InitClient(pw->pw_name, (char *)"mbfido", CFG.location, CFG.logfile, 
	    CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);

    Syslog(' ', " ");
    Syslog(' ', "MBFIDO v%s", VERSION);
    Syslog(' ', cmd);
    free(cmd);
    if (do_obs_a)
	WriteError("The -a option is obsolete, adjust your setup");

    /*
     * Not yet locked, if anything goes wrong, exit with die(MBERR_NO_PROGLOCK)
     */
    if (enoughspace(CFG.freespace) == 0)
	die(MBERR_DISK_FULL);

    if (do_mail) {
	/*
	 * Try to get a lock for a long time, another mbfido may be legally
	 * running since mbmail is started by the MTA instead of by mbtask.
	 * The timeout is 10 minutes. If mbmail times out, the MTA will
	 * bounce the message. What happens during the time we wait is
	 * unknown, will the MTA be patient enough?
	 */
	i = 30;
	while (TRUE) {
	    if (do_unprot) {
		if (lockdir(CFG.inbound))
		    break;
	    } else {
		if (lockdir(CFG.pinbound))
		    break;
	    }
	    i--;
	    if (! i) {
		WriteError("Lock timeout, aborting");
		die(MBERR_NO_PROGLOCK);
	    }
	    sleep(20);
	    Nopper();
	}
    } else {
	/*
	 * Started under control of mbtask, that means if there is a lock then
	 * there is something wrong; abort.
	 */
	if (do_unprot) {
	    if (! lockdir(CFG.inbound))
		die(MBERR_NO_PROGLOCK);
	} else {
	    if (! lockdir(CFG.pinbound))
		die(MBERR_NO_PROGLOCK);
	}
    }

    /*
     * Locking succeeded, no more abort alowed with die(110)
     */

    if (initnl())
	die(MBERR_INIT_ERROR);
    if (!do_mail && !do_uucp)
	Rollover();
    if (!do_quiet)
	printf("\n");

    /*
     *  Read alias file
     */
    cmd = calloc(PATH_MAX, sizeof(char));
    snprintf(cmd, PATH_MAX, "%s/etc/aliases", getenv("MBSE_ROOT"));
    if ((do_news || do_scan || do_toss || do_mail) && file_exist(cmd, R_OK) == 0)
        readalias(cmd);
    free(cmd);

    if (do_mail) {
	if (!envrecip_count) {
	    WriteError("No valid receipients specified, aborting");
	    die(MBERR_NO_RECIPIENTS);
	}

	umask(066);
	if ((ofp = tmpfile()) == NULL) {
	    WriteError("$Can't open tmpfile for RFC message");
	    die(MBERR_INIT_ERROR);
	}
	temp = calloc(10240, sizeof(char));
	while (fgets(temp, 10240, stdin))
	    fprintf(ofp, temp);
	free(temp);

	for (envrecip = &envrecip_start; *envrecip; envrecip = &((*envrecip)->next)) {
	    Syslog('+', "Message to: %s", ascfnode((*envrecip)->addr, 0x7f));
	    rfc2ftn(ofp, (*envrecip)->addr);
	}

	fclose(ofp);
	flush_queue();
	die(MBERR_OK);
    }

    InitDupes();

    if (do_notify)
	if (Notify(Options)) {
	    do_flush = TRUE;
	}
    if (do_tic || do_toss)
	toss_msgs();
    if (do_tic) {
	if (IsSema((char *)"mailin"))
	    RemoveSema((char *)"mailin");
	/*
	 *  Hatch new files and process .tic files
	 *  until nothing left to do.
	 */
	Loop = TRUE;
	do {
	    toss_msgs();
	    Hatch();
	    switch (Tic()) {
		case -1:    die(MBERR_OK);
			    break;
		case 0:	    Loop = FALSE;
			    break;
		default:    break;
	    }
	} while (Loop);
    }
    if (do_news) {
	ScanNews();
	if (IsSema((char *)"newnews"))
	    RemoveSema((char *)"newnews");
    }
    if (do_scan) {
	toss_msgs();
	ScanMail(do_full);
    }
    if (do_toss) {
	if (IsSema((char *)"mailin"))
	    RemoveSema((char *)"mailin");
	toss_msgs();
	if (TossMail() == FALSE)
	    die(MBERR_OK);
    }
    if (do_tic || do_toss) {
	/*
	 * Do inbound direcory sessions
	 */
	if (dirinbound()) {
	    if (do_tic) {
		/*
		 *  Hatch new files and process .tic files
		 *  until nothing left to do.
		 */
		Loop = TRUE;
		do {
		    toss_msgs();
		    Hatch();
		    switch (Tic()) {
			case -1:    die(MBERR_OK);
				    break;
			case 0:     Loop = FALSE;
				    break;
			default:    break;
		    }
		} while (Loop);
	    }
	    if (do_toss) {
		toss_msgs();
		TossMail();
	    }
	}
    }
    if (!do_uucp)
	newspost();
    if (do_test) {
	if (taddr == NULL)
	    Help();
	TestTracker(taddr);
	tidy_faddr(taddr);
    }
    if (do_tags)
	MakeTags();
    if (do_stat)
	MakeStat();
    if (do_uucp)
	NewsUUCP();
    if (do_areas) {
	if (!do_quiet) {
	    mbse_colour(LIGHTGREEN, BLACK);
	    printf("Are you sure to process all area lists [y/N] ");
	    fflush(stdout);
	    x = mbse_Getone();
	    printf("\r                                             \r");
	    fflush(stdout);
	    if (toupper(x) != 'Y')
		die(MBERR_OK);
	}
	Areas();
    }
    if (do_flush)
	flush_queue();

    die(MBERR_OK);
    return 0;
}



/*
 * Toss Fidonet mail
 */
int TossMail(void)
{
    char	    *inbound, *fname;
    DIR		    *dp;
    struct dirent   *de;
    struct stat	    sbuf;
    int		    files = 0, files_ok = 0, rc = 0, maxrc = 0;
    fd_list	    *fdl = NULL;

    if (do_unprot)
	inbound = xstrcpy(CFG.inbound);
    else
	inbound = xstrcpy(CFG.pinbound);

    Syslog('+', "Pass: toss netmail (%s)", inbound);

    if (chdir(inbound) == -1) {
	WriteError("$Can't chdir(%s)", inbound);
	die(MBERR_INIT_ERROR);
    }

    /*
     * First toss any netmail packets.
     */
    maxrc = rc = TossPkts();
    chdir(inbound);

    /*
     * Scan the directory for ARCmail archives. The archive extension
     * numbering doesn't matter, as long as there is something, so
     * all kind of ARCmail naming schemes are recognized.
     */
    if ((dp = opendir(inbound)) == NULL) {
	WriteError("$Can't opendir(%s)", inbound);
	die(MBERR_INIT_ERROR);
    }

    Syslog('+', "Pass: toss ARCmail (%s)", inbound);

    /*
     * Add all ARCmail filenames to the memory array.
     */
    sync();
    while ((de=readdir(dp)))
	if ((strlen(de->d_name) == 12) && ((strncasecmp(de->d_name+8,".su",3) == 0) ||
	    (strncasecmp(de->d_name+8,".mo",3) == 0) || (strncasecmp(de->d_name+8,".tu",3) == 0) ||
	    (strncasecmp(de->d_name+8,".we",3) == 0) || (strncasecmp(de->d_name+8,".th",3) == 0) ||
	    (strncasecmp(de->d_name+8,".fr",3) == 0) || (strncasecmp(de->d_name+8,".sa",3) == 0))) {
	    stat(de->d_name, &sbuf);
	    fill_fdlist(&fdl, de->d_name, sbuf.st_mtime);
	}

    closedir(dp);
    sort_fdlist(&fdl);

    /*
     * Now process the archives, the oldest first.
     */
    while ((fname = pull_fdlist(&fdl)) != NULL) {
	files++;
	IsDoing("Unpack arcmail");

	if (IsSema((char *)"upsalarm")) {
	    Syslog('+', "Detected upsalarm semafore, aborting toss");
	    break;
	}
	if (enoughspace(CFG.freespace) == 0) {
	    Syslog('+', "Low diskspace, aborting toss");
	    rc = MBERR_DISK_FULL;
	    break;
	}

	if (checkspace(inbound, fname, UNPACK_FACTOR))
	    if ((rc = unpack(fname)) == 0) {
		files_ok++;
		rc = TossPkts();
		chdir(inbound);
	    } else 
		WriteError("Error unpacking file %s", fname);
	else
	    Syslog('!', "Insufficient space to unpack file %s", fname);
			
	if (rc > maxrc) 
	    maxrc = rc;
    }

    free(inbound);
    if ((files || packets) && ((files_ok != files) || (packets_ok != packets)))
	Syslog('!', "Processed %d of %d files, %d of %d packets, rc=%d", files_ok, files, packets_ok, packets, maxrc);

    return TRUE;
}



/*
 *  Toss all packets currently in the inbound. Tossing is sorted by
 *  age of the files.
 */
int TossPkts(void)
{
    char	    *inbound = NULL, *fname;
    DIR		    *dp;
    struct dirent   *de;
    struct stat	    sbuf;
    int		    rc = 0, maxrc = 0;
    fd_list	    *fdl = NULL;

    IsDoing("Tossing mail");

    if (do_unprot)
	inbound = xstrcpy(CFG.inbound);
    else
	inbound = xstrcpy(CFG.pinbound);

    if ((dp = opendir(inbound)) == NULL) {
	WriteError("$Can't opendir(%s)", inbound);
	return FALSE;
    }

    /*
     * Read all .pkt filenames, get the timestamp and add them
     * to the memory array for later sort on filedate.
     */ 
    while ((de = readdir(dp)))
	if ((strlen(de->d_name) == 12) && (strncasecmp(de->d_name+8,".pkt",4) == 0)) {
	    stat(de->d_name, &sbuf);
	    fill_fdlist(&fdl, de->d_name, sbuf.st_mtime);
	}

    closedir(dp);
    sort_fdlist(&fdl);

    /*
     * Get the filenames, the oldest first until nothing left.
     */
    while ((fname = pull_fdlist(&fdl)) != NULL) {

	if (enoughspace(CFG.freespace) == 0) {
	    Syslog('+', "Low diskspace, abort tossing packet");
	    return FALSE;
	}
	packets++;

	/*
	 * See if "pktdate" from Tobias Ernst (or another preprocessor) is installed.
	 */
	if (strlen(CFG.pktdate)) {
	    rc = execute_str(CFG.pktdate, fname, (char *)NULL, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
	    if (rc)
		Syslog('+', "%s preprocessing rc=%d", fname, rc);
	}

	if ((rc = toss(fname)) == 0) 
	    packets_ok++;
	else 
	    WriteError("Error tossing packet %s", fname);
	if (rc > maxrc) 
	    maxrc = rc;
    }

    free(inbound);
    do_flush = TRUE;
    return maxrc;
}



/*
 * Toss one packet 
 */
int toss(char *fn)
{
    int	    rc = 0, ld;
    char    newname[16];

    /*
     * Lock the packet
     */
    if ((ld = f_lock(fn)) == -1) 
	return 1; 

    rc = TossPkt(fn);
    if (rc == 0) {
	unlink(fn); 
    } else {
	strncpy(newname,fn,sizeof(newname)-1);
	strcpy(newname+8,".bad");
	rename(fn,newname);
    }

    funlock(ld); 
    return rc;
}


