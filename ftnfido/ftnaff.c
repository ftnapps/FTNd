/*****************************************************************************
 *
 * $Id: mbaff.c,v 1.22 2007/09/02 11:17:32 mbse Exp $
 * Purpose ...............: Announce new files and FileFind
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
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "../lib/msg.h"
#include "announce.h"
#include "filefind.h"
#include "mbaff.h"



int			do_announce = FALSE;	/* Announce flag	    */
int			do_filefind = FALSE;	/* FileFind flag	    */
extern	int		do_quiet;		/* Suppress screen output   */
extern	int		show_log;		/* Show logging		    */
time_t			t_start;		/* Start time		    */
time_t			t_end;			/* End time		    */



void ProgName(void)
{
    if (do_quiet)
	return;

    mbse_colour(WHITE, BLACK);
    printf("\nMBAFF: MBSE BBS %s Announce new files and FileFind\n", VERSION);
    mbse_colour(YELLOW, BLACK);
    printf("       %s\n", COPYRIGHT);
}



void die(int onsig)
{
    signal(onsig, SIG_IGN);

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    WriteError("Terminated with error %d", onsig);
    }

    ulockprogram((char *)"mbaff");
    t_end = time(NULL);
    Syslog(' ', "MBAFF finished in %s", t_elapsed(t_start, t_end));

    if (!do_quiet) {
	mbse_colour(LIGHTGRAY, BLACK);
	printf("\n");
    }
    ExitClient(onsig);
}



int main(int argc, char **argv)
{
    int		    i, Mail = FALSE;
    char	    *cmd;
    struct passwd   *pw;
    struct tm	    *t;

    InitConfig();
    mbse_TermInit(1, 80, 25);
    t_start = time(NULL);
    t = localtime(&t_start);
    Diw = t->tm_wday;
    Miy = t->tm_mon;
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

    if (argc < 2)
	Help();

    cmd = xstrcpy((char *)"Command line: mbaff");

    for (i = 1; i < argc; i++) {

	cmd = xstrcat(cmd, (char *)" ");
	cmd = xstrcat(cmd, tl(argv[i]));

	if (!strncmp(argv[i], "a", 1))
	    do_announce = TRUE;
	if (!strncmp(argv[i], "f", 1))
	    do_filefind = TRUE;
	if (!strncmp(argv[i], "-q", 2))
	    do_quiet = TRUE;

    }

    ProgName();
    pw = getpwuid(getuid());
    InitClient(pw->pw_name, (char *)"mbaff", CFG.location, CFG.logfile, 
	    CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);

    Syslog(' ', " ");
    Syslog(' ', "MBAFF v%s", VERSION);
    Syslog(' ', cmd);
    free(cmd);

    if (!do_quiet)
	printf("\n");

    if (enoughspace(CFG.freespace) == 0)
	die(MBERR_DISK_FULL);

    if (lockprogram((char *)"mbaff")) {
	if (!do_quiet)
	    printf("Can't lock mbaff, abort.\n");
	die(MBERR_NO_PROGLOCK);
    }

    memset(&MsgBase, 0, sizeof(MsgBase));

    if (do_announce)
	if (Announce())
	    Mail = TRUE;

    if (do_filefind)
	if (Filefind())
	    Mail = TRUE;

    if (Mail) {
	CreateSema((char *)"mailout");
	CreateSema((char *)"msglink");
    }

    die(MBERR_OK);
    return 0;
}



void Help(void)
{
	do_quiet = FALSE;
	ProgName();

	mbse_colour(LIGHTMAGENTA, BLACK);
	printf("\nUsage:	mbaff [command] <options>\n\n");
	mbse_colour(LIGHTBLUE, BLACK);
	printf("	Commands are:\n\n");
	mbse_colour(CYAN, BLACK);
	printf("	a  announce	Announce new files\n");
	printf("	f  filefind	FileFind service\n");
	mbse_colour(LIGHTBLUE, BLACK);
	printf("\n	Options are:\n\n");
	mbse_colour(CYAN, BLACK);
	printf("	-q -quiet	Quiet mode\n");
	mbse_colour(LIGHTGRAY, BLACK);
	printf("\n");
	die(MBERR_COMMANDLINE);
}


