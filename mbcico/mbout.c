/*****************************************************************************
 *
 * $Id$
 * Purpose: MBSE BBS Outbound Manager
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "../lib/dbnode.h"
#include "../lib/dbftn.h"
#include "outstat.h"
#include "nlinfo.h"


extern int	do_quiet;		/* Supress screen output	    */
int		do_attach = FALSE;	/* Send file attaches		    */
int		do_node   = FALSE;	/* Query the nodelist		    */
int		do_poll	  = FALSE;	/* Poll a node			    */
int		do_req	  = FALSE;	/* Request files from a node	    */
int		do_stat	  = FALSE;	/* Show outbound status		    */
int		do_stop   = FALSE;	/* Stop polling a node		    */
int		e_pid = 0;		/* Pid of child			    */
extern	int	show_log;		/* Show logging			    */
time_t		t_start;		/* Start time			    */
time_t		t_end;			/* End time			    */



void ProgName(void);
void ProgName()
{
	if (do_quiet)
		return;

	colour(15, 0);
	printf("\nMBOUT: MBSE BBS %s Outbound Manager\n", VERSION);
	colour(14, 0);
	printf("       %s\n", COPYRIGHT);
}



void die(int);
void die(int onsig)
{
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
		system("stty sane");
	}

	signal(onsig, SIG_IGN);

	if (show_log)
		do_quiet = FALSE;

	if (!do_quiet)
		colour(3, 0);

	if (onsig) {
		if (onsig <= NSIG)
			WriteError("$Terminated on signal %d (%s)", onsig, SigName[onsig]);
		else
			WriteError("Terminated with error %d", onsig);
	}

	t_end = time(NULL);
	Syslog(' ', "MBOUT finished in %s", t_elapsed(t_start, t_end));

	if (!do_quiet) {
		colour(7, 0);
		printf("\n");
	}
	ExitClient(onsig);
}



void Help(void);
void Help()
{
	do_quiet = FALSE;
	ProgName();

	colour(11, 0);
	printf("\nUsage:	mbout [command] <params> <options>\n\n");
	colour(9, 0);
	printf("	Commands are:\n\n");
	colour(3, 0);
	printf("	a   att  <node> <flavor> <file>		Attach a file to a node\n");
	printf("	n   node <node>				Show nodelist information\n");
	printf("	p   poll <node> [node..node]		Poll node(s) (always crash)\n");
	printf("	r   req  <node> <file> [file..file]	Request file(s) from node\n");
	printf("	sta stat				Show outbound status\n");
	printf("	sto stop <node> [node..node]		Stop polling node(s)\n");
	printf("\n");
	printf("	<node>	 Should be in domain form, e.g. f16.n2801.z2.domain\n");
	printf("	<flavor> Flavor's are: crash | immediate | normal | hold\n");
	colour(9, 0);
	printf("\n	Options are:\n\n");
	colour(3, 0);
	printf("	-quiet					Quiet mode\n");
	colour(7, 0);
	die(0);
}



void Fatal(char *);
void Fatal(char *msg)
{
	show_log = TRUE;
	if (!do_quiet) {
		colour(12, 0);
		printf("%s\n", msg);
	}
	WriteError(msg);
	die(100);
}



int main(int argc, char *argv[])
{
	char		*cmd, flavor = 'x';
	int		i, j, rc = 0;
	struct passwd	*pw;
	faddr		*addr = NULL;
	node		*nlent;

#ifdef MEMWATCH
	mwInit();
#endif
	InitConfig();
	InitNode();
	InitFidonet();
	TermInit(1);
	t_start = time(NULL);
	umask(002);

	/*
	 * Catch all signals we can, and ignore the rest.
	 */
	for (i = 0; i < NSIG; i++) {
		if ((i == SIGHUP) || (i == SIGINT) || (i == SIGBUS) ||
		    (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM) ||
		    (i == SIGKILL))
			signal(i, (void (*))die);
		else
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
		if (!strncasecmp(argv[1], "r", 1))
			do_req = TRUE;
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
	InitClient(pw->pw_name, (char *)"mbout", CFG.location, CFG.logfile, CFG.util_loglevel, CFG.error_log);
	Syslog(' ', " ");
	Syslog(' ', "MBOUT v%s", VERSION);
	Syslog(' ', cmd);
	free(cmd);

	if (!do_quiet) {
		colour(3, 0);
		printf("\n");
	}

	if (do_stat) {
		rc = outstat();
		if (rc)
			rc += 100;
		die(rc);
	}

	/*
	 * Get node number from commandline
	 */
	if (do_attach || do_node || do_poll || do_stop || do_req) {
		if (argc < 3)
			Fatal((char *)"Not enough parameters");
	}

	if (do_attach || do_node || do_req) {
		if ((addr = parsefaddr(argv[2])) == NULL)
			Fatal((char *)"Unrecognizable address");
	}

	if (do_node) {
		rc = nlinfo(addr);
		tidy_faddr(addr);
		if (rc)
			rc += 100;
		die(rc);
	}

	if (do_poll || do_stop) {
		for (i = 3; i <= argc; i++) {
			if (strncasecmp(argv[i-1], "-q", 2)) {
				if ((addr = parsefaddr(argv[i-1])) == NULL)
					Fatal((char *)"Unrecognizable address");
				j = poll(addr, do_stop);
				tidy_faddr(addr);
				if (j > rc)
					rc = j;
			}
		}
		if (rc)
			rc = 100;
		die(rc);
	}

	if (do_attach) {
		if (argc < 5)
			Fatal((char *)"Not enough parameters");
		flavor = tolower(argv[3][0]);
		switch (flavor) {
			case 'n' : 	flavor = 'f';	break;
			case 'i' :	flavor = 'i';	break;
			case 'c' :	flavor = 'c';	break;
			case 'h' :	flavor = 'h';	break;
			default  :	Fatal((char *)"Invalid flavor, must be: immediate, crash, normal or hold");
		}

		nlent = getnlent(addr);
		if (nlent->pflag == NL_DUMMY)
			Fatal((char *)"Node is not in nodelist");
		if (nlent->pflag == NL_DOWN)
			Fatal((char *)"Node has status Down");
		if (nlent->pflag == NL_HOLD)
			Fatal((char *)"Node has status Hold");
		if (((nlent->oflags & OL_CM) == 0) && (flavor == 'c'))
			Fatal((char *)"Node is not CM, must use Immediate, Normal or Hold flavor");

		if (argv[4][0] == '-')
			Fatal((char *)"Invalid filename given");
		if (file_exist(argv[4], R_OK) != 0)
			Fatal((char *)"File doesn't exist");

		if (attach(*addr, argv[4], LEAVE, flavor)) {
			Syslog('+', "File attach %s is successfull", argv[4]);
			if (!do_quiet)
				printf("File attach %s is successfull", argv[4]);
			CreateSema((char *)"scanout");
			die(0);
		} else {
			Fatal((char *)"File attach failed");
		}
	}

	if (do_req) {
		if (argc < 4)
			Fatal((char *)"Not enough parameters");
		for (i = 4; i <= argc; i++) {
			if (strncasecmp(argv[i-1], "-q", 2)) {
				rc = freq(addr, argv[i-1]);
				if (rc) 
					break;
			}
		}
		if (rc)
			rc += 100;
		die(rc);
	}

	Help();
#ifdef MEMWATCH
	mwTerm();
#endif
	return 0;
}


