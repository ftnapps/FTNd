/*****************************************************************************
 *
 * $Id$
 * Purpose: File Database Maintenance - utilities
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
#include "mbfutil.h"
#include "mbfile.h"



extern int	do_quiet;		/* Supress screen output	    */
extern	int	e_pid;			/* Pid of external process	    */
extern time_t	t_start;		/* Start time			    */
extern time_t	t_end;			/* End time			    */
int		marker = 0;		/* Marker counter		    */



void ProgName(void)
{
	if (do_quiet)
		return;

	colour(WHITE, BLACK);
	printf("\nMBFILE: MBSE BBS %s File maintenance utility\n", VERSION);
	colour(YELLOW, BLACK);
	printf("        %s\n", COPYRIGHT);
}



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
		if (!do_quiet)
			system("stty sane");
	}

	signal(onsig, SIG_IGN);

	if (onsig) {
		if (onsig <= NSIG)
			WriteError("$Terminated on signal %d (%s)", onsig, SigName[onsig]);
		else
			WriteError("Terminated with error %d", onsig);
	}

	time(&t_end);
	Syslog(' ', "MBFILE finished in %s", t_elapsed(t_start, t_end));

	if (!do_quiet) {
		colour(7, 0);
		printf("\n");
	}
	ExitClient(onsig);
}



void Help(void)
{
	do_quiet = FALSE;
	ProgName();

	colour(LIGHTCYAN, BLACK);
	printf("\nUsage:	mbfile [command] <options>\n\n");
	colour(LIGHTBLUE, BLACK);
	printf("	Commands are:\n\n");
	colour(CYAN, BLACK);
	printf("	a  adopt <area> <file> [desc]	Adopt file to area\n");
	printf("	c  check			Check filebase\n");
//	printf("	d  delete <area> <file>		Mark file in area for deletion\n");
//	printf("        im import <area>		Import files in current dir to area\n");
	printf("	in index			Create filerequest index\n");
	printf("        k  kill				Kill/move old files\n");
	printf("	l  list				List file areas\n");
//	printf("	m  move <from> <to> <file>	Move file from to area\n");
	printf("	p  pack				Pack filebase\n");
//	printf("	r  rearc <area> [file] [arc]	Rearc file(s) in area\n");
	colour(LIGHTBLUE, BLACK);
	printf("\n	Options are:\n\n");
	colour(CYAN, BLACK);
	printf("	-q -quiet			Quiet mode\n");
	colour(LIGHTGRAY, BLACK);
	printf("\n");
	die(0);
}



void Marker(void)
{
	/*
	 * Keep the connection with the server alive
	 */
	Nopper();

	/*
	 * Release system resources when running in the background
	 */
	if (CFG.slow_util && do_quiet)
		usleep(1);

	if (do_quiet)
		return;

	switch (marker) {
		case 0:	printf(">---");
			break;

		case 1:	printf(">>--");
			break;

		case 2:	printf(">>>-");
			break;

		case 3:	printf(">>>>");
			break;

		case 4:	printf("<>>>");
			break;

		case 5:	printf("<<>>");
			break;

		case 6:	printf("<<<>");
			break;

		case 7:	printf("<<<<");
			break;

		case 8: printf("-<<<");
			break;

		case 9: printf("--<<");
			break;

		case 10:printf("---<");
			break;

		case 11:printf("----");
			break;
	}
	printf("\b\b\b\b");
	fflush(stdout);

	if (marker < 11)
		marker++;
	else
		marker = 0;
}



void DeleteVirusWork()
{
        char    *buf, *temp;

        buf  = calloc(PATH_MAX, sizeof(char));
        temp = calloc(PATH_MAX, sizeof(char));
        getcwd(buf, PATH_MAX);
        sprintf(temp, "%s/tmp", getenv("MBSE_ROOT"));

        if (chdir(temp) == 0) {
                Syslog('f', "DeleteVirusWork %s/arc", temp);
                system("rm -r -f arc");
                system("mkdir arc");
        } else
                WriteError("$Can't chdir to %s", temp);

        chdir(buf);
        free(temp);
        free(buf);
}


