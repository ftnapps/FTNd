/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Announce new files and FileFind
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
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

	colour(15, 0);
	printf("\nMBAFF: MBSE BBS %s Announce new files and FileFind\n", VERSION);
	colour(14, 0);
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

	t_end = time(NULL);
	Syslog(' ', "MBAFF finished in %s", t_elapsed(t_start, t_end));

	if (!do_quiet) {
		colour(7, 0);
		printf("\n");
	}
	ExitClient(onsig);
}



int main(int argc, char **argv)
{
	int	i, Mail = FALSE;
	char	*cmd;
	struct	passwd *pw;
	struct	tm *t;

#ifdef MEMWATCH
        mwInit();
#endif
	InitConfig();
	TermInit(1);
	t_start = time(NULL);
	t = localtime(&t_start);
	Diw = t->tm_wday;
	Miy = t->tm_mon;
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
	InitClient(pw->pw_name, (char *)"mbaff", CFG.location, CFG.logfile, CFG.util_loglevel, CFG.error_log, CFG.mgrlog);

	Syslog(' ', " ");
	Syslog(' ', "MBAFF v%s", VERSION);
	Syslog(' ', cmd);
	free(cmd);

	if (!do_quiet)
		printf("\n");

	if (!diskfree(CFG.freespace))
		die(101);

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

	die(0);
	return 0;
}



void Help(void)
{
	do_quiet = FALSE;
	ProgName();

	colour(11, 0);
	printf("\nUsage:	mbaff [command] <options>\n\n");
	colour(9, 0);
	printf("	Commands are:\n\n");
	colour(3, 0);
	printf("	a  announce	Announce new files\n");
	printf("	f  filefind	FileFind service\n");
	colour(9, 0);
	printf("\n	Options are:\n\n");
	colour(3, 0);
	printf("	-q -quiet	Quiet mode\n");
	colour(7, 0);
	printf("\n");
	die(0);
}


