/*****************************************************************************
 *
 * $Id: mbmsg.c,v 1.34 2007/09/02 11:17:32 mbse Exp $
 * Purpose ...............: Message Base Maintenance
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
#include "../lib/msg.h"
#include "../lib/mbsedb.h"
#include "post.h"
#include "mbmsg.h"



int	do_pack = FALSE;	/* Pack flag				    */
int	do_kill = FALSE;	/* Kill flag (age and maxmsgs)		    */
int	do_link = FALSE;	/* Link messages			    */
int	do_post = FALSE;	/* Post a Message			    */
extern	int do_quiet;		/* Quiet flag				    */
extern	int show_log;		/* Show loglines			    */
int	do_area = 0;		/* Do only one area			    */
time_t	t_start;		/* Start time				    */
time_t	t_end;			/* End time				    */
int	are_tot = 0;		/* Total areas				    */
int	are_proc = 0;		/* Areas processed			    */
int	msg_tot = 0;		/* Total messages			    */
int	msg_del = 0;		/* Deleted messages			    */
int	msg_link = 0;		/* Linked messages			    */
int	processed = FALSE;	/* Did process something		    */
int	oldmask;




/*
 * Header, only if not quiet
 */
void ProgName()
{
    if (do_quiet)
	return;

    mbse_colour(WHITE, BLACK);
    printf("\nMBMSG: MBSE BBS %s - Message Base Maintenance Utility\n", VERSION);
    mbse_colour(YELLOW, BLACK);
    printf("       %s\n", COPYRIGHT);
    mbse_colour(LIGHTGRAY, BLACK);
}



int main(int argc, char **argv)
{
    int		    i;
    char	    *cmd, *too = NULL, *subj = NULL, *mfile = NULL, *flavor = NULL;
    struct passwd   *pw;
    int		    tarea = 0;


    InitConfig();
    mbse_TermInit(1, 80, 25);
    oldmask = umask(007);
    t_start = time(NULL);

    /*
     * Catch all signals we can, and ignore or catch them
     */
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGHUP) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM) || (i == SIGIOT))
	    signal(i, (void (*))die);
	else if ((i != SIGKILL) && (i != SIGSTOP))
	    signal(i, SIG_IGN);
    }

    if (argc < 2)
	Help();

    cmd = xstrcpy((char *)"Cmd:");

    for (i = 1; i < argc; i++) {
	cmd = xstrcat(cmd, (char *)" ");
	cmd = xstrcat(cmd, argv[i]);

	if (strncasecmp(argv[i], "l", 1) == 0)
	    do_link = TRUE;
	if (strncasecmp(argv[i], "k", 1) == 0)
	    do_kill = TRUE;
	if (strncasecmp(argv[i], "pa", 2) == 0)
	    do_pack = TRUE;
	if (strncasecmp(argv[i], "po", 2) == 0) {
	    if (((argc - i) < 6) || ((argc - i) > 7))
		Help();
	    do_post = TRUE;
	    too = argv[++i];
	    cmd = xstrcat(cmd, (char *)" \"");
	    cmd = xstrcat(cmd, too);
	    tarea = atoi(argv[++i]);
	    cmd = xstrcat(cmd, (char *)"\" ");
	    cmd = xstrcat(cmd, argv[i]);
	    subj = argv[++i];
	    cmd = xstrcat(cmd, (char *)" \"");
	    cmd = xstrcat(cmd, subj);
	    mfile = argv[++i];
	    cmd = xstrcat(cmd, (char *)"\" ");
	    cmd = xstrcat(cmd, mfile);
	    flavor = argv[++i];
	    cmd = xstrcat(cmd, (char *)" ");
	    cmd = xstrcat(cmd, flavor);
	}
	if (strncasecmp(argv[i], "-a", 2) == 0) {
	    i++;
	    do_area = atoi(argv[i]);
	}
	if (strncasecmp(argv[i], "-q", 2) == 0)
	    do_quiet = TRUE;
    }

    if (!(do_link || do_kill || do_pack || do_post))
	Help();

    ProgName();
    pw = getpwuid(getuid());
    InitClient(pw->pw_name, (char *)"mbmsg", CFG.location, CFG.logfile, 
		CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);

    Syslog(' ', " ");
    Syslog(' ', "MBMSG v%s", VERSION);
    Syslog(' ', cmd);
    free(cmd);

    if (!do_quiet) {
	printf("\n");
	mbse_colour(CYAN, BLACK);
    }

    if (do_link || do_kill || do_pack) {
	memset(&MsgBase, 0, sizeof(MsgBase));
	DoMsgBase();
    }

    if (do_post) {
	if (Post(too, tarea, subj, mfile, flavor))
	    die(MBERR_GENERAL);
    }

    die(MBERR_OK);
    return 0;
}



void Help()
{
    do_quiet = FALSE;
    ProgName();

    mbse_colour(LIGHTCYAN, BLACK);
    printf("\n	Usage: mbmsg [command(s)] <options>\n\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("	Commands are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	l  link					Link messages by subject\n");
    printf("	k  kill					Kill messages (age & count)\n");
    printf("	pa pack					Pack deleted messages\n");
    printf("	po post <to> <#> <subj> <file> <flavor>	Post file in message area #\n\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("	Options are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	-a -area <#>				Process area <#> only\n");
    printf("	-q -quiet				Quiet mode\n");

    printf("\n");
    die(MBERR_COMMANDLINE);
}



void die(int onsig)
{
    if (onsig && (onsig <= NSIG)) {
	signal(onsig, SIG_IGN);
    }

    if (!do_quiet) {
	printf("\r");
	mbse_colour(CYAN, BLACK);
    }

    if (MsgBase.Locked)
	Msg_UnLock();
    if (MsgBase.Open)
	Msg_Close();

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    WriteError("Terminated with error %d", onsig);
    }

    if (are_tot || are_proc || msg_link)
    	Syslog('+', "Areas  [%6d] Processed [%6d] Linked [%6d]", are_tot, are_proc, msg_link);
    if (msg_tot || msg_del)
	Syslog('+', "Msgs   [%6d]   Deleted [%6d]", msg_tot, msg_del);

    t_end = time(NULL);
    Syslog(' ', "MBMSG finished in %s", t_elapsed(t_start, t_end));

    umask(oldmask);
    if (!do_quiet) {
	mbse_colour(LIGHTGRAY, BLACK);
	printf("\r                                                          \n");
    }
    ExitClient(onsig);
}



void DoMsgBase()
{
    FILE    *pAreas;
    char    *sAreas, *Name;
    int	    arearec;
    int	    Del = 0;

    sAreas  = calloc(PATH_MAX, sizeof(char));
    Name    = calloc(PATH_MAX, sizeof(char ));

    IsDoing("Msg Maintenance");

    if (do_area)
	Syslog('+', "Processing message area %ld", do_area);
    else
	Syslog('+', "Processing all message areas");

    if (do_kill) {
	Syslog('-', " Total Max. Days/Killed  Max. Nr/Killed Area name");
	Syslog('-', "------    ------ ------   ------ ------ ----------------------------------");
    }

    snprintf(sAreas, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
    if(( pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("$Can't open %s", sAreas);
	die(MBERR_GENERAL);
    }
    fread(&msgshdr, sizeof(msgshdr), 1, pAreas);

    if (do_area) {
	if (fseek(pAreas, (msgshdr.recsize + msgshdr.syssize) * (do_area - 1), SEEK_CUR) == 0) {
	    fread(&msgs, msgshdr.recsize, 1, pAreas);
	    if (msgs.Active) {

		if (enoughspace(CFG.freespace) == 0)
		    die(MBERR_DISK_FULL);

		if (!do_quiet) {
		    mbse_colour(CYAN, BLACK);
		    printf("\r%5d .. %-40s", do_area, msgs.Name);
		    fflush(stdout);
		}
		are_tot++;
		mkdirs(msgs.Base, 0770);
		if (do_kill)
		    KillArea(msgs.Base, msgs.Name, msgs.DaysOld, msgs.MaxMsgs, do_area);
		if (do_pack || msg_del)
		    PackArea(msgs.Base, do_area);
		if (do_link)
		    LinkArea(msgs.Base, do_area);
		if (processed)
		    are_proc++;
	    }
	}
    } else {
	arearec = 0;
	while (fread(&msgs, msgshdr.recsize, 1, pAreas) == 1) {
	    fseek(pAreas, msgshdr.syssize, SEEK_CUR);
	    arearec++;
	    if (msgs.Active) {

		if (enoughspace(CFG.freespace) == 0)
		    die(MBERR_DISK_FULL);

		Nopper();
		if (!do_quiet) {
		    mbse_colour(CYAN, BLACK);
		    printf("\r%5d .. %-40s", arearec, msgs.Name);
		    fflush(stdout);
		}
		are_tot++;
		mkdirs(msgs.Base, 0770);
		processed = FALSE;
		if (do_kill)
		    KillArea(msgs.Base, msgs.Name, msgs.DaysOld, msgs.MaxMsgs, arearec);
		if (do_pack || (Del != msg_del)) {
		    PackArea(msgs.Base, arearec);
		}
		Del = msg_del;
		if (do_link)
		    LinkArea(msgs.Base, arearec);
		if (processed)
		    are_proc++;
	    }
	}
    }
    fclose(pAreas);

    if (!do_area) {
	snprintf(sAreas, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((pAreas = fopen (sAreas, "r")) == NULL) {
	    WriteError("$Can't open %s", sAreas);
	    die(MBERR_GENERAL);
	}
	fread(&usrconfighdr, sizeof(usrconfighdr), 1, pAreas);

	while (fread(&usrconfig, usrconfighdr.recsize, 1, pAreas) == 1) {
	    if (usrconfig.Email && strlen(usrconfig.Name)) {
		Nopper();
		snprintf(Name, PATH_MAX, "User %s email area: mailbox", usrconfig.Name);
		if (!do_quiet) {
		    mbse_colour(CYAN, BLACK);
		    printf("\r      .. %-40s", Name);
		    fflush(stdout);
		}
		snprintf(sAreas, PATH_MAX, "%s/%s/mailbox", CFG.bbs_usersdir, usrconfig.Name);
		are_tot++;
		processed = FALSE;
		if (do_kill)
		    KillArea(sAreas, Name, 0, CFG.defmsgs, 0);
		if (do_pack || (Del != msg_del)) {
		    PackArea(sAreas, 0);
		}
		Del = msg_del;
		if (do_link)
		    LinkArea(sAreas, 0);
		if (processed)
		    are_proc++;
		snprintf(sAreas, PATH_MAX, "%s/%s/archive", CFG.bbs_usersdir, usrconfig.Name);
		snprintf(Name, 80, "User %s email area: archive", usrconfig.Name);
		are_tot++;
		processed = FALSE;
		if (do_kill)
		    KillArea(sAreas, Name, 0, CFG.defmsgs, 0);
		if (do_pack || (Del != msg_del))
		    PackArea(sAreas, 0);
		Del = msg_del;
		if (do_link)
		    LinkArea(sAreas, 0);
		if (processed)
		    are_proc++;
		snprintf(sAreas, PATH_MAX, "%s/%s/trash", CFG.bbs_usersdir, usrconfig.Name);
		snprintf(Name, 80, "User %s email area: trash", usrconfig.Name);
		are_tot++;
		processed = FALSE;
		if (do_kill)
		    KillArea(sAreas, Name, CFG.defdays, CFG.defmsgs, 0);
		if (do_pack || (Del != msg_del))
		    PackArea(sAreas, 0);
		Del = msg_del;
		if (do_link)
		    LinkArea(sAreas, 0);
		if (processed)
		    are_proc++;

	    }
	}

	fclose(pAreas);
    }

    if (do_link)
	RemoveSema((char *)"msglink");

    free(sAreas);
    free(Name);
    die(MBERR_OK);
}



void LinkArea(char *Path, int Areanr)
{
    int	    rc;

    IsDoing("Linking %ld", Areanr);
    rc = Msg_Link(Path, do_quiet, CFG.slow_util);

    if (rc != -1) {
	msg_link += rc;
	processed = TRUE;
    }
}



/*
 * Kill messages according to age and max messages.
 */
void KillArea(char *Path, char *Name, int DaysOld, int MaxMsgs, int Areanr)
{
    unsigned int    Number, TotalMsgs = 0, Highest, *Active, Counter = 0;
    int		    i, DelCount = 0, DelAge = 0, Done;
    time_t	    Today, MsgDate;

    IsDoing("Killing %ld", Areanr);
    Today = time(NULL) / 86400L;

    if (Msg_Open(Path)) {

	if (!do_quiet) {
	    mbse_colour(LIGHTRED, BLACK);
	    printf(" (Killing)");
	    mbse_colour(LIGHTMAGENTA, BLACK);
	    fflush(stdout);
	}

	if (Msg_Lock(30L)) {

	    TotalMsgs = Msg_Number();

	    if (TotalMsgs) {
		if ((Active = (unsigned int *)malloc((size_t)((TotalMsgs + 100L) * sizeof(unsigned int)))) != NULL) {
		    i = 0;
		    Number = Msg_Lowest();
		    do {
			Active[i++] = Number;
		    } while (Msg_Next(&Number) == TRUE);
		}
	    } else
		Active = NULL;

	    Number  = Msg_Lowest();
	    Highest = Msg_Highest();

	    do {
		if (CFG.slow_util && do_quiet)
		msleep(1);

		if ((!do_quiet) && ((Counter % 10L) == 0)) {
		    printf("%6u / %6u\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", Counter, TotalMsgs);
		    fflush(stdout);
		}
		if ((Counter % 10L) == 0)
		    DoNop();

		Counter++;
		msg_tot++;

		if (Msg_ReadHeader(Number) == TRUE) {
		    Done = FALSE;
		    if (DaysOld) {
			/*
			 * GoldED doesn't fill the Msg.Arrived field, use the
			 * written date instead.
			 */
			if (Msg.Arrived == 0L)
			    MsgDate = Msg.Written / 86400L;
			else
			    MsgDate = Msg.Arrived / 86400L;

			if ((Today - MsgDate) > DaysOld) {
			    Msg_Delete(Number);
			    Done = TRUE;
			    DelAge++;
			    msg_del++;
			    if (Active != NULL) {
				for (i = 0; i < TotalMsgs; i++) {
				    if (Active[i] == Number)
					Active[i] = 0L;
				}
			    }
			}
		    }

		    if (Done == FALSE && (MaxMsgs) && Msg_Number() > MaxMsgs) {
			Msg_Delete(Number);
			DelCount++;
			msg_del++;
			if (Active != NULL) {
			    for (i = 0; i < TotalMsgs; i++) {
				if (Active[i] == Number)
				    Active[i] = 0L;
			    }
			}
		    }
		} 
	    } while (Msg_Next(&Number) == TRUE);

	    if (Active != NULL)
		free(Active);
	    Msg_UnLock();
	} else {
	    Syslog('+', "Can't lock msgbase %s", Path);
	}

	if (!do_quiet) {
	    printf("               \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	    fflush(stdout);
	}

	Msg_Close();
	Syslog('-', "%6d    %6d %6d   %6d %6d %s", TotalMsgs, DaysOld, DelAge, MaxMsgs, DelCount, Name);

	if (!do_quiet) {
	    printf("\b\b\b\b\b\b\b\b\b\b          \b\b\b\b\b\b\b\b\b\b");
	    fflush(stdout);
	}
    } else
	Syslog('+', "Failed to open %s", Path);
}



/*
 * Pack message area if there are deleted messages.
 */
void PackArea(char *Path, int Areanr)
{
    IsDoing("Packing %ld", Areanr);
    if (Msg_Open(Path)) {

	if (!do_quiet) {
	    mbse_colour(LIGHTRED, BLACK);
	    printf(" (Packing)");
	    fflush(stdout);
	}

	if (Msg_Lock(30L)) {
	    Msg_Pack();
	    Msg_UnLock();
	} else
	    Syslog('+', "Can't lock %s", Path);

	Msg_Close();

	if (!do_quiet) {
	    printf("\b\b\b\b\b\b\b\b\b\b          \b\b\b\b\b\b\b\b\b\b");
	    fflush(stdout);
	}
    }

    if (CFG.slow_util && do_quiet)
	msleep(1);
}


