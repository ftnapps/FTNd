/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbcfg.h"
#include "mbfkill.h"
#include "mbfadopt.h"
#include "mbfindex.h"
#include "mbfcheck.h"
#include "mbfpack.h"
#include "mbflist.h"
#include "mbfutil.h"
#include "mbfile.h"



extern int	do_quiet;		/* Supress screen output	    */
int		do_adopt = FALSE;	/* Adopt a file			    */
int		do_pack  = FALSE;	/* Pack filebase		    */
int		do_check = FALSE;	/* Check filebase		    */
int		do_kill  = FALSE;	/* Kill/move old files		    */
int		do_index = FALSE;	/* Create request index		    */
int		do_list  = FALSE;	/* List fileareas		    */
extern	int	e_pid;			/* Pid of external process	    */
extern	int	show_log;		/* Show logging			    */
time_t		t_start;		/* Start time			    */
time_t		t_end;			/* End time			    */



int main(int argc, char **argv)
{
	int	i, Area = 0;
	char	*cmd, *FileName = NULL, *Description = NULL;
	struct	passwd *pw;

#ifdef MEMWATCH
	mwInit();
#endif
	InitConfig();
	TermInit(1);
	time(&t_start);
	umask(002);

	/*
	 * Catch all signals we can, and ignore the rest.
	 */
	for (i = 0; i < NSIG; i++) {
		if ((i == SIGHUP) || (i == SIGBUS) || (i == SIGKILL) ||
		    (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM))
			signal(i, (void (*))die);
		else
			signal(i, SIG_IGN);
	}

	if(argc < 2)
		Help();

	cmd = xstrcpy((char *)"Command line: mbfile");

	for (i = 1; i < argc; i++) {

		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, tl(argv[i]));

		if (!strncmp(argv[i], "a", 1)) {
			do_adopt = TRUE;
			i++;
			Area = atoi(argv[i]);
			cmd = xstrcat(cmd, (char *)" ");
			cmd = xstrcat(cmd, argv[i]);
			i++;
			FileName = xstrcpy(argv[i]);
			cmd = xstrcat(cmd, (char *)" ");
			cmd = xstrcat(cmd, argv[i]);
			if (argc > (i + 1)) {
				i++;
				Description = xstrcpy(argv[i]);
				cmd = xstrcat(cmd, (char *)" ");
				cmd = xstrcat(cmd, argv[i]);
			}
		}
		if (!strncmp(argv[i], "in", 2))
			do_index = TRUE;
		if (!strncmp(argv[i], "l", 1))
			do_list  = TRUE;
		if (!strncmp(argv[i], "p", 1))
			do_pack = TRUE;
		if (!strncmp(argv[i], "c", 1))
			do_check = TRUE;
		if (!strncmp(argv[i], "k", 1))
			do_kill = TRUE;
		if (!strncmp(argv[i], "-q", 2))
			do_quiet = TRUE;
	}

	if (!(do_pack || do_check || do_kill || do_index || do_list || do_adopt))
		Help();

	ProgName();
	pw = getpwuid(getuid());
	InitClient(pw->pw_name, (char *)"mbfile", CFG.location, CFG.logfile, CFG.util_loglevel, CFG.error_log);

	Syslog(' ', " ");
	Syslog(' ', "MBFILE v%s", VERSION);
	Syslog(' ', cmd);
	free(cmd);

	if (!do_quiet)
		printf("\n");

	if (!diskfree(CFG.freespace))
		die(101);

	if (do_adopt)
		AdoptFile(Area, FileName, Description);

	if (do_kill)
		Kill();

	if (do_check)
		Check();

	if (do_pack)
		PackFileBase();

	if (do_index)
		Index();

	if (do_list)
		ListFileAreas();

	die(0);
	return 0;
}




