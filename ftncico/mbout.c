/*****************************************************************************
 *
 * Purpose: MBSE BBS Outbound Manager
 *
 *****************************************************************************
 * Copyright (C) 1997-2010
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
#include "outstat.h"
#include "nlinfo.h"

extern int most_debug;

extern int	do_quiet;		/* Suppress screen output	    */
int		do_attach = FALSE;	/* Send file attaches		    */
int		do_node   = FALSE;	/* Query the nodelist		    */
int		do_poll	  = FALSE;	/* Poll a node			    */
int		do_req	  = FALSE;	/* Request files from a node	    */
int		do_stat	  = FALSE;	/* Show outbound status		    */
int		do_stop   = FALSE;	/* Stop polling a node		    */
int		do_reset  = FALSE;	/* Reset node's try counter	    */
extern	pid_t	e_pid;			/* Pid of child			    */
extern	int	show_log;		/* Show logging			    */
time_t		t_start;		/* Start time			    */
time_t		t_end;			/* End time			    */



void ProgName(void);
void ProgName()
{
    if (do_quiet)
	return;

    mbse_colour(WHITE, BLACK);
    printf("\nMBOUT: MBSE BBS %s Outbound Manager\n", VERSION);
    mbse_colour(YELLOW, BLACK);
    printf("       %s\n", COPYRIGHT);
}



void die(int);
void die(int onsig)
{
    deinitnl();
    
    /*
     * First check if a child is running, if so, kill it.
     */
    if (e_pid) {
	if ((kill(e_pid, SIGTERM)) == 0)
	    Syslog('+', "SIGTERM to pid %d succeeded", e_pid);
	else {
	    if ((kill(e_pid, SIGKILL)) == 0)
		Syslog('+', "SIGKILL to pid %d succeded", e_pid);
	    else
		WriteError("$Failed to kill pid %d", e_pid);
	}

	/*
	 * In case the child had the tty in raw mode...
	 */
	execute_pth((char *)"stty", (char *)"sane", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
    }

    signal(onsig, SIG_IGN);

    if (show_log)
	do_quiet = FALSE;

    if (!do_quiet)
	mbse_colour(CYAN, BLACK);

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    WriteError("Terminated with error %d", onsig);
    }

    t_end = time(NULL);
    Syslog(' ', "MBOUT finished in %s", t_elapsed(t_start, t_end));

    if (!do_quiet) {
	mbse_colour(LIGHTGRAY, BLACK);
	printf("\n");
    }
    ExitClient(onsig);
}



void Help(void);
void Help()
{
    do_quiet = FALSE;
    ProgName();

    mbse_colour(LIGHTCYAN, BLACK);
    printf("\nUsage:	mbout [command] <params> <options>\n\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("	Commands are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	a   att   <node> <flavor> <file>	Attach a file to a node\n");
    printf("	n   node  <node>			Show nodelist information\n");
    printf("	p   poll  <node> [node..node]		Poll node(s) (always crash)\n");
    printf("	req req   <node> <file> [file..file]	Request file(s) from node\n");
    printf("	res reset <node> [node..node]		Reset node(s) \"try\" counter\n");
    printf("	sta stat				Show outbound status\n");
    printf("	sto stop  <node> [node..node]		Stop polling node(s)\n");
    printf("\n");
    printf("	<node>	  Should be in domain form, e.g. f16.n2801.z2.domain\n");
    printf("	<flavor>  Flavor's are: crash | immediate | normal | hold\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("\n	Options are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	-quiet					Quiet mode\n");
    mbse_colour(LIGHTGRAY, BLACK);
    die(MBERR_OK);
}



void Fatal(char *, int);
void Fatal(char *msg, int error)
{
    show_log = TRUE;
    if (!do_quiet) {
	mbse_colour(LIGHTRED, BLACK);
	printf("%s\n", msg);
    }
    WriteError(msg);
    die(error);
}



int main(int argc, char *argv[])
{
    char	    *cmd, flavor = 'x';
    int		    i, j, rc = 0;
    struct passwd   *pw;
    faddr	    *addr = NULL;
    node	    *nlent;
    FILE	    *fl;
    
    most_debug = TRUE;

    InitConfig();
    InitNode();
    InitFidonet();
    mbse_TermInit(1, 80, 25);
    t_start = time(NULL);
    umask(002);

    /*
     * Catch all signals we can, and ignore the rest.
     */
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGHUP) || (i == SIGINT) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM) || (i == SIGIOT))
	    signal(i, (void (*))die);
	else if ((i != SIGKILL) && (i != SIGSTOP))
	    signal(i, SIG_IGN);
    }

    if(argc < 2)
	Help();

    cmd = xstrcpy((char *)"Command line: mbout");

    if (argc > 1) {
	cmd = xstrcat(cmd, (char *)" ");
	cmd = xstrcat(cmd, argv[1]);

	if (!strncasecmp(argv[1], "a", 1))
	    do_attach = TRUE;
	if (!strncasecmp(argv[1], "n", 1))
	    do_node = TRUE;
	if (!strncasecmp(argv[1], "p", 1))
	    do_poll = TRUE;
	if (!strncasecmp(argv[1], "req", 3))
	    do_req = TRUE;
	if (!strncasecmp(argv[1], "res", 3))
	    do_reset = TRUE;
	if (!strncasecmp(argv[1], "sta", 3))
	    do_stat = TRUE;
	if (!strncasecmp(argv[1], "sto", 3))
	    do_stop = TRUE;
    }

    for (i = 2; i < argc; i++) {

	cmd = xstrcat(cmd, (char *)" ");
	cmd = xstrcat(cmd, argv[i]);

	if (!strncasecmp(argv[i], "-q", 2))
	    do_quiet = TRUE;
    }

    ProgName();
    pw = getpwuid(getuid());
    InitClient(pw->pw_name, (char *)"mbout", CFG.location, CFG.logfile, CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);
    Syslog(' ', " ");
    Syslog(' ', "MBOUT v%s", VERSION);
    Syslog(' ', cmd);
    free(cmd);

    if (!do_quiet) {
	mbse_colour(CYAN, BLACK);
	printf("\n");
    }

    if (strcmp(pw->pw_name, "mbse"))
	Fatal((char *)"You are not user 'mbse'", MBERR_COMMANDLINE);
    
    if (do_stat) {
	rc = outstat();
	die(rc);
    }

    /*
     * Get node number from commandline
     */
    if (do_attach || do_node || do_poll || do_stop || do_req || do_reset) {
	if (argc < 3)
	    Fatal((char *)"Not enough parameters", MBERR_COMMANDLINE);
    }

    if (do_attach || do_node || do_req || do_reset) {
	if ((addr = parsefaddr(argv[2])) == NULL)
	    Fatal((char *)"Unrecognizable address", MBERR_COMMANDLINE);
    }

    if (do_node) {
	rc = nlinfo(addr);
	tidy_faddr(addr);
	die(rc);
    }

    if (do_poll || do_stop) {
	tidy_faddr(addr);
	for (i = 3; i <= argc; i++) {
	    if (strncasecmp(argv[i-1], "-q", 2)) {
		if ((addr = parsefaddr(argv[i-1])) == NULL)
		    Fatal((char *)"Unrecognizable address", MBERR_COMMANDLINE);
		j = pollnode(addr, do_stop);
		tidy_faddr(addr);
		if (j)
		    rc = j;
	    }
	}
	die(rc);
    }

    if (do_reset) {
	tidy_faddr(addr);
	for (i = 3; i <= argc; i++) {
	    if (strncasecmp(argv[i-1], "-q", 2)) {
		if ((addr = parsefaddr(argv[i-1])) == NULL)
		    Fatal((char *)"Unrecognizable address", MBERR_COMMANDLINE);
		j = reset(addr);
		tidy_faddr(addr);
		if (j)
		    rc = j;
	    }
	}
	die(rc);
    }

    if (do_attach) {
	if (argc < 5)
	    Fatal((char *)"Not enough parameters", MBERR_COMMANDLINE);
	flavor = tolower(argv[3][0]);
	switch (flavor) {
	    case 'n' : 	flavor = 'f';	break;
	    case 'i' :	flavor = 'i';	break;
	    case 'c' :	flavor = 'c';	break;
	    case 'h' :	flavor = 'h';	break;
	    default  :	Fatal((char *)"Invalid flavor, must be: immediate, crash, normal or hold", MBERR_COMMANDLINE);
	}

	nlent = getnlent(addr);
	if (nlent->addr.domain)
	    free(nlent->addr.domain);
	nlent->addr.domain = NULL;
	if (nlent->url)
	    free(nlent->url);
	nlent->url = NULL;

	if (nlent->pflag == NL_DUMMY)
	    Fatal((char *)"Node is not in nodelist", MBERR_NODE_NOT_IN_LIST);
	if (nlent->pflag == NL_DOWN)
	    Fatal((char *)"Node has status Down", MBERR_NODE_MAY_NOT_CALL);
	if (nlent->pflag == NL_HOLD)
	    Fatal((char *)"Node has status Hold", MBERR_NODE_MAY_NOT_CALL);
	if (((nlent->can_pots && nlent->is_cm) == FALSE) && ((nlent->can_ip && nlent->is_icm) == FALSE) && (flavor == 'c'))
	    Fatal((char *)"Node is not CM, must use Immediate, Normal or Hold flavor", MBERR_NODE_MAY_NOT_CALL);

	if (argv[4][0] == '-')
	    Fatal((char *)"Invalid filename given", MBERR_COMMANDLINE);
	if (argv[4][0] != '/')
	    Fatal((char *)"Must use absolute path/filename (or ~/path/filename)", MBERR_COMMANDLINE);
	if (file_exist(argv[4], R_OK) != 0)
	    Fatal((char *)"File doesn't exist", MBERR_COMMANDLINE);

	cmd = calloc(PATH_MAX, sizeof(char));
	snprintf(cmd, PATH_MAX -1, "%s/%d.%d.%d.%d/.filelist", CFG.out_queue, addr->zone, addr->net, addr->node, addr->point);
	mkdirs(cmd, 0750);
	if ((fl = fopen(cmd, "a+")) == NULL) {
	    Fatal((char *)"File attach failed", MBERR_ATTACH_FAILED);
	} else {
	    fprintf(fl, "%c LEAVE NOR %s\n", flavor, argv[4]);
	    Syslog('+', "File attach %s is successfull", argv[4]);
	    if (!do_quiet)
		printf("File attach %s is successfull", argv[4]);
	    CreateSema((char *)"mailin");
	    tidy_faddr(addr);
	    fsync(fileno(fl));
	    fclose(fl);
	    free(cmd);
	    die(MBERR_OK);
	}
	free(cmd);
    }

    if (do_req) {
	if (argc < 4)
	    Fatal((char *)"Not enough parameters", MBERR_COMMANDLINE);
	for (i = 4; i <= argc; i++) {
	    if (strncasecmp(argv[i-1], "-q", 2)) {
		rc = freq(addr, argv[i-1]);
		if (rc) 
		    break;
	    }
	}
	tidy_faddr(addr);
	die(rc);
    }

    Help();
    return MBERR_OK;
}


