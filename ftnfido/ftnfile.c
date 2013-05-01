/*****************************************************************************
 *
 * ftnfile.c
 * Purpose: File Database Maintenance
 *
 *****************************************************************************
 * Copyright (C) 1997-2007 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2013   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/ftndlib.h"
#include "../lib/users.h"
#include "../lib/ftnddb.h"
#include "ftnfkill.h"
#include "ftnfadopt.h"
#include "ftnfindex.h"
#include "ftnfcheck.h"
#include "ftnfpack.h"
#include "ftnflist.h"
#include "ftnfimport.h"
#include "ftnftoberep.h"
#include "ftnfmove.h"
#include "ftnfdel.h"
#include "ftnfutil.h"
#include "ftnfsort.h"
#include "ftnfile.h"
#include "ftnfrearc.h"


extern int	do_quiet;		/* Suppress screen output	    */
int		do_annon = FALSE;	/* Suppress announce on new files   */
int		do_novir = FALSE;	/* Suppress virus check		    */
int		do_adopt = FALSE;	/* Adopt a file			    */
int		do_pack  = FALSE;	/* Pack filebase		    */
int		do_check = FALSE;	/* Check filebase		    */
int		do_kill  = FALSE;	/* Kill/move old files		    */
int		do_index = FALSE;	/* Create request index		    */
int		do_import= FALSE;	/* Import files in area		    */
int		do_list  = FALSE;	/* List fileareas		    */
int		do_tobe  = FALSE;	/* List toberep database	    */
int		do_move  = FALSE;	/* Move a file			    */
int		do_del   = FALSE;	/* Delete/undelete a file	    */
int		do_sort  = FALSE;	/* Sort a filebase		    */
int		do_rearc = FALSE;	/* ReArc a file			    */
int		do_force = FALSE;	/* Force file overwrite		    */
extern	int	e_pid;			/* Pid of external process	    */
extern	int	show_log;		/* Show logging			    */
time_t		t_start;		/* Start time			    */
time_t		t_end;			/* End time			    */


int main(int argc, char **argv)
{
    int	    i, Area = 0, ToArea = 0, UnDel = FALSE;
    char    *cmd, *FileName = NULL, *Description = NULL;
    struct  passwd *pw;

    InitConfig();
    ftnd_TermInit(1, 80, 25);
    t_start = time(NULL);
    umask(002);

    /*
     * Catch all signals we can, and ignore the rest.
     */
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGHUP) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM) || (i == SIGIOT))
	    signal(i, (void (*))die);
	else if (i == SIGCHLD)
	    signal(i, SIG_DFL);
	else if ((i != SIGKILL) && (i != SIGSTOP))
	    signal(i, SIG_IGN);
    }

    if(argc < 2)
	Help();

    cmd = xstrcpy((char *)"Command line: ftnfile");

    for (i = 1; i < argc; i++) {

	cmd = xstrcat(cmd, (char *)" ");
	cmd = xstrcat(cmd, argv[i]);

	if (!strncasecmp(argv[i], "a", 1)) {
	    i++;
	    Area = atoi(argv[i]);
	    cmd = xstrcat(cmd, (char *)" ");
	    cmd = xstrcat(cmd, argv[i]);
	    if (Area) {
		do_adopt = TRUE;
		i++;
		FileName = xstrcpy(argv[i]);
		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[i]);
		if (argc > (i + 1)) {
		    i++;
		    cmd = xstrcat(cmd, (char *)" ");
		    cmd = xstrcat(cmd, argv[i]);
		    if (!strncasecmp(argv[i], "-a", 2)) {
			do_annon = TRUE;
		    } else {
			Description = xstrcpy(argv[i]);
		    }
		}
		break;
	    }
	} else if ((!strncasecmp(argv[i], "d", 1)) || (!strncasecmp(argv[i], "u", 1))) {
	    if (!strncasecmp(argv[i], "u", 1))
		UnDel = TRUE;
	    if (argc > (i + 1)) {
		i++;
		Area = atoi(argv[i]);
		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[i]);
		if (argc > (i + 1)) {
		    i++;
		    FileName = xstrcpy(argv[i]);
		    cmd = xstrcat(cmd, (char *)" ");
		    cmd = xstrcat(cmd, argv[i]);
		    do_del = TRUE;
		}
	    }
	} else if (!strncasecmp(argv[i], "r", 1)) {
	    if (argc > (i + 1)) {
		i++;
		Area = atoi(argv[i]);
		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[i]);
		if (argc > (i + 1)) {
		    i++;
		    FileName = xstrcpy(argv[i]);
		    cmd = xstrcat(cmd, (char *)" ");
		    cmd = xstrcat(cmd, argv[i]);
		    do_rearc = TRUE;
		}
	    }
	} else if (!strncasecmp(argv[i], "in", 2)) {
	    do_index = TRUE;
	} else if (!strncasecmp(argv[i], "im", 2)) {
	    if (argc > (i + 1)) {
		do_import = TRUE;
		i++;
		Area = atoi(argv[i]);
		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[i]);
	    }
	} else if (!strcasecmp(argv[i], "i")) {
	    Help();
	} else if (!strncasecmp(argv[i], "l", 1)) {
	    do_list  = TRUE;
	    if (argc > (i + 1)) {
		i++;
		Area = atoi(argv[i]);
		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[i]);
	    }
	} else if (!strncasecmp(argv[i], "m", 1)) {
	    if (argc > (i + 1)) {
		i++;
		Area = atoi(argv[i]);
		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[i]);
		if (argc > (i + 1)) {
		    i++;
		    ToArea = atoi(argv[i]);
		    cmd = xstrcat(cmd, (char *)" ");
		    cmd = xstrcat(cmd, argv[i]);
		    if (argc > (i + 1)) {
			i++;
			FileName = xstrcpy(argv[i]);
			cmd = xstrcat(cmd, (char *)" ");
			cmd = xstrcat(cmd, argv[i]);
			do_move = TRUE;
		    }
		}
	    }
	} else if (!strncasecmp(argv[i], "s", 1)) {
	    if (argc > (i + 1)) {
		i++;
		Area = atoi(argv[i]);
		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[i]);
		do_sort = TRUE;
	    }
	} else if (!strncasecmp(argv[i], "p", 1)) {
	    do_pack = TRUE;
	} else if (!strncasecmp(argv[i], "c", 1)) {
	    do_check = TRUE;
	    if (argc > (i + 1)) {
		Area = atoi(argv[i+1]);
		if (Area) {
		    i++;
		    cmd = xstrcat(cmd, (char *)" ");
		    cmd = xstrcat(cmd, argv[i]);
		}
	    }
	} else if (!strncasecmp(argv[i], "k", 1)) {
	    do_kill = TRUE;
	} else if (!strncasecmp(argv[i], "t", 1)) {
	    do_tobe = TRUE;
	} else if (!strncasecmp(argv[i], "-q", 2)) {
	    do_quiet = TRUE;
	} else if (!strncasecmp(argv[i], "-a", 2)) {
	    do_annon = TRUE;
	} else if (!strncasecmp(argv[i], "-v", 2)) {
	    do_novir = TRUE;
	} else if (!strncasecmp(argv[i], "-f", 2)) {
	    do_force = TRUE;
	}
    }

    if (!(do_pack || do_sort || do_check || do_kill || do_index || do_import || 
		do_list || do_adopt || do_del || do_move || do_tobe || do_rearc))
	Help();

    ProgName();
    pw = getpwuid(getuid());
    InitClient(pw->pw_name, (char *)"ftnfile", CFG.location, CFG.logfile, 
	    CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);

    Syslog(' ', " ");
    Syslog(' ', "FTNDFILE v%s", VERSION);
    Syslog(' ', cmd);
    if (do_novir)
	Syslog('!', "WARNING: running without virus checking");
    free(cmd);

    if (!do_quiet)
	printf("\n");

    if (enoughspace(CFG.freespace) == 0)
	die(FTNERR_DISK_FULL);

    if (lockprogram((char *)"ftnfile")) {
	if (!do_quiet)
	    printf("Can't lock ftnfile, abort.\n");
	die(FTNERR_NO_PROGLOCK);
    }

    if (do_adopt) {
	AdoptFile(Area, FileName, Description);
	die(FTNERR_OK);
    }

    if (do_import) {
	ImportFiles(Area);
	die(FTNERR_OK);
    }

    if (do_kill)
	Kill();

    if (do_sort)
	SortFileBase(Area);

    if (do_check) {
	Check(Area);
    }

    if (do_rearc) {
	ReArc(Area, FileName);
	if (do_index)
	    Index();
	die(FTNERR_OK);
    }

    if (do_pack)
	PackFileBase();

    if (do_index)
	Index();

    if (do_move) {
	Move(Area, ToArea, FileName);
	die(FTNERR_OK);
    }

    if (do_del) {
	Delete(UnDel, Area, FileName);
	die(FTNERR_OK);
    }

    if (do_list) {
	ListFileAreas(Area);
	die(FTNERR_OK);
    }

    if (do_tobe)
	ToBeRep();
    die(FTNERR_OK);
    return 0;
}




