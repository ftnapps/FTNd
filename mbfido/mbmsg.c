/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Message Base Maintenance
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
#include "../lib/msg.h"
#include "../lib/dbcfg.h"
#include "../lib/mberrors.h"
#include "post.h"
#include "mbmsg.h"



int	do_pack = FALSE;	/* Pack flag				    */
int	do_kill = FALSE;	/* Kill flag (age and maxmsgs)		    */
int	do_index = FALSE;	/* Index flag				    */
int	do_link = FALSE;	/* Link messages			    */
int	do_post = FALSE;	/* Post a Message			    */
extern	int do_quiet;		/* Quiet flag				    */
extern	int show_log;		/* Show loglines			    */
long	do_area = 0;		/* Do only one area			    */
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

	colour(15, 0);
	printf("\nMBMSG: MBSE BBS %s - Message Base Maintenance Utility\n", VERSION);
	colour(14, 0);
	printf("       %s\n", COPYRIGHT);
	colour(7, 0);
}



int main(int argc, char **argv)
{
	int	i;
	char	*cmd, *too = NULL, *subj = NULL, *mfile = NULL, *flavor = NULL;
	struct	passwd *pw;
	long	tarea = 0;

#ifdef MEMWATCH
        mwInit();
#endif

	InitConfig();
	TermInit(1);
	oldmask = umask(007);
	t_start = time(NULL);

	/*
	 * Catch all signals we can, and ignore or catch them
	 */
	for (i = 0; i < NSIG; i++) {
		if ((i == SIGHUP) || (i == SIGBUS) || (i == SIGILL) ||
		    (i == SIGSEGV) || (i == SIGTERM) || (i == SIGKILL))
			signal(i, (void (*))die);
		else
			signal(i, SIG_IGN);
	}

	if (argc < 2)
		Help();

	cmd = xstrcpy((char *)"Cmd:");

	for (i = 1; i < argc; i++) {
		cmd = xstrcat(cmd, (char *)" ");
		cmd = xstrcat(cmd, argv[i]);

		if (strncasecmp(argv[i], "i", 1) == 0)
			do_index = TRUE;
		if (strncasecmp(argv[i], "l", 1) == 0)
			do_link = TRUE;
		if (strncasecmp(argv[i], "k", 1) == 0)
			do_kill = TRUE;
		if (strncasecmp(argv[i], "pa", 2) == 0)
			do_pack = TRUE;
		if (strncasecmp(argv[i], "po", 2) == 0) {
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

	if (!(do_index || do_link || do_kill || do_pack || do_post))
		Help();

	ProgName();
	pw = getpwuid(getuid());
	InitClient(pw->pw_name, (char *)"mbmsg", CFG.location, CFG.logfile, CFG.util_loglevel, CFG.error_log, CFG.mgrlog);

	Syslog(' ', " ");
	Syslog(' ', "MBMSG v%s", VERSION);
	Syslog(' ', cmd);
	free(cmd);

	if (!do_quiet) {
		printf("\n");
		colour(3, 0);
	}

	if (do_index || do_link || do_kill || do_pack) {
		memset(&MsgBase, 0, sizeof(MsgBase));
		DoMsgBase();
	}

	if (do_post) {
		Post(too, tarea, subj, mfile, flavor);
	}

	die(MBERR_OK);
	return 0;
}



void Help()
{
	do_quiet = FALSE;
	ProgName();

	colour(12, 0);
	printf("\n	Usage: mbmsg [command(s)] <options>\n\n");
	colour(9, 0);
	printf("	Commands are:\n\n");
	colour(3, 0);
//	printf("	i  index				Create new index files\n");
	printf("	l  link					Link messages by subject\n");
	printf("	k  kill					Kill messages (age & count)\n");
	printf("	pa pack					Pack deleted messages\n");
	printf("	po post <to> <#> <subj> <file> <flavor>	Post file in message area #\n\n");
	colour(9, 0);
	printf("	Options are:\n\n");
	colour(3, 0);
	printf("	-a -area <#>				Process area <#> only\n");
	printf("	-q -quiet				Quiet mode\n");

	printf("\n");
	die(MBERR_COMMANDLINE);
}



void die(int onsig)
{
	signal(onsig, SIG_IGN);
	if (!do_quiet) {
		printf("\r");
		colour(3, 0);
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

	if (onsig == SIGSEGV)
		Syslog('+', "Last msg area %s", msgs.Name);

	if (are_tot || are_proc || msg_link)
		Syslog('+', "Areas  [%5d] Processed [%5d] Linked [%5d]", are_tot, are_proc, msg_link);
	if (msg_tot || msg_del)
		Syslog('+', "Msgs   [%5d]   Deleted [%5d]", msg_tot, msg_del);

	t_end = time(NULL);
	Syslog(' ', "MBMSG finished in %s", t_elapsed(t_start, t_end));

	umask(oldmask);
	if (!do_quiet) {
		colour(7, 0);
		printf("\r                                                          \n");
	}
	ExitClient(onsig);
}



void DoMsgBase()
{
	FILE	*pAreas;
	char	*sAreas, *Name;
	long	arearec;
	int	Del = 0;

	sAreas  = calloc(81, sizeof(char));
	Name	= calloc(81, sizeof(char ));

	IsDoing("Msg Maintenance");

	if (do_area)
		Syslog('+', "Processing message area %ld", do_area);
	else
		Syslog('+', "Processing all message areas");

	if (do_kill) {
		Syslog('-', " Total Max. Days/Killed  Max. Nr/Killed Area name");
		Syslog('-', "------    ------ ------   ------ ------ ----------------------------------");
	}

	sprintf(sAreas, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if(( pAreas = fopen (sAreas, "r")) == NULL) {
		WriteError("$Can't open Messages Areas File.");
		die(MBERR_INIT_ERROR);
	}
	fread(&msgshdr, sizeof(msgshdr), 1, pAreas);

	if (do_area) {
		if (fseek(pAreas, (msgshdr.recsize + msgshdr.syssize) * (do_area - 1), SEEK_CUR) == 0) {
			fread(&msgs, msgshdr.recsize, 1, pAreas);
			if (msgs.Active) {

				if (!diskfree(CFG.freespace))
					die(MBERR_DISK_FULL);

				if (!do_quiet) {
					colour(3, 0);
					printf("\r%5ld .. %-40s", do_area, msgs.Name);
					fflush(stdout);
				}
				are_tot++;
				mkdirs(msgs.Base, 0770);
				if (do_kill)
					KillArea(msgs.Base, msgs.Name, msgs.DaysOld, msgs.MaxMsgs, do_area);
				if (do_pack || msg_del)
					PackArea(msgs.Base, do_area);
				if (do_index)
					IndexArea(msgs.Base, do_area);
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

				if (!diskfree(CFG.freespace))
					die(MBERR_DISK_FULL);

				Nopper();
				if (!do_quiet) {
					colour(3, 0);
					printf("\r%5ld .. %-40s", arearec, msgs.Name);
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
				if (do_index)
					IndexArea(msgs.Base, arearec);
				if (do_link)
					LinkArea(msgs.Base, arearec);
				if (processed)
					are_proc++;
			}
		}
	}
	fclose(pAreas);

	if (!do_area) {
		sprintf(sAreas, "%s/etc/users.data", getenv("MBSE_ROOT"));
		if ((pAreas = fopen (sAreas, "r")) == NULL) {
			WriteError("$Can't open Messages Areas File.");
			die(SIGILL);
		}
		fread(&usrconfighdr, sizeof(usrconfighdr), 1, pAreas);

		while (fread(&usrconfig, usrconfighdr.recsize, 1, pAreas) == 1) {
			if (usrconfig.Email && strlen(usrconfig.Name)) {
				Nopper();
				sprintf(Name, "User %s email area: mailbox", usrconfig.Name);
				if (!do_quiet) {
					colour(3, 0);
					printf("\r      .. %-40s", Name);
					fflush(stdout);
				}
				sprintf(sAreas, "%s/%s/mailbox", CFG.bbs_usersdir, usrconfig.Name);
				are_tot++;
				processed = FALSE;
				if (do_kill)
					KillArea(sAreas, Name, 0, CFG.defmsgs, 0);
				if (do_pack || (Del != msg_del)) {
					PackArea(sAreas, 0);
				}
				Del = msg_del;
				if (do_index)
					IndexArea(sAreas, 0);
				if (do_link)
					LinkArea(sAreas, 0);
				if (processed)
					are_proc++;
				sprintf(sAreas, "%s/%s/archive", CFG.bbs_usersdir, usrconfig.Name);
				sprintf(Name, "User %s email area: archive", usrconfig.Name);
				are_tot++;
				processed = FALSE;
				if (do_kill)
					KillArea(sAreas, Name, 0, CFG.defmsgs, 0);
				if (do_pack || (Del != msg_del))
					PackArea(sAreas, 0);
				Del = msg_del;
				if (do_index)
					IndexArea(sAreas, 0);
				if (do_link)
					LinkArea(sAreas, 0);
				if (processed)
					are_proc++;
				sprintf(sAreas, "%s/%s/trash", CFG.bbs_usersdir, usrconfig.Name);
				sprintf(Name, "User %s email area: trash", usrconfig.Name);
				are_tot++;
				processed = FALSE;
				if (do_kill)
					KillArea(sAreas, Name, CFG.defdays, CFG.defmsgs, 0);
				if (do_pack || (Del != msg_del))
					PackArea(sAreas, 0);
				Del = msg_del;
				if (do_index)
					IndexArea(sAreas, 0);
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



typedef struct {
	unsigned long	Subject;
	unsigned long	Number;
} MSGLINK;



void LinkArea(char *Path, long Areanr)
{
	int		i, m;
	unsigned long	Number, Prev, Next, Crc, Total;
	char		Temp[128], *p;
	MSGLINK		*Link;
	
	IsDoing("Linking %ld", Areanr);

	if (Msg_Open(Path)) {
		if (!do_quiet) {
			colour(12, 0);
			printf(" (linking)");
			colour(13, 0);
			fflush(stdout);
		}

		if ((Total = Msg_Number()) != 0L) {
			if (Msg_Lock(30L)) {
				if ((Link = (MSGLINK *)malloc(Total * sizeof(MSGLINK))) != NULL) {
					memset(Link, 0, Total * sizeof(MSGLINK));
					Number = Msg_Lowest();
					i = 0;
					do {
						Msg_ReadHeader(Number);
						strcpy(Temp, Msg.Subject);
						p = strupr(Temp);
						if (!strncmp(p, "RE:", 3)) {
							p += 3;
							if (*p == ' ')
								p++;
						}
						Link[i].Subject = StringCRC32(p);
						Link[i].Number = Number;
						i++;

						if (CFG.slow_util && do_quiet && ((i % 5) == 0))
							usleep(1);

						if (((i % 10) == 0) && (!do_quiet)) {
							printf("%6d / %6lu\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
							fflush(stdout);
						}
					} while(Msg_Next(&Number) == TRUE);

					if (!do_quiet) {
						printf("%6d / %6lu\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
						fflush(stdout);
					}

					Number = Msg_Lowest();
					i = 0;
					do {
						Msg_ReadHeader(Number);
						Prev = Next = 0;
						Crc = Link[i].Subject;
	
						for (m = 0; m < Total; m++) {
							if (m == i)
								continue;
							if (Link[m].Subject == Crc) {
								if (m < i)
									Prev = Link[m].Number;
								else if (m > i) {
									Next = Link[m].Number;
									break;
								}
							}
						}

						if (CFG.slow_util && do_quiet && ((i % 5) == 0))
							usleep(1);

						if (((i % 10) == 0) && (!do_quiet)) {
							printf("%6d / %6lu\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
							fflush(stdout);
						}

						if (Msg.Original != Prev || Msg.Reply != Next) {
							Msg.Original = Prev;
							Msg.Reply = Next;
							Msg_WriteHeader(Number);
							processed = TRUE;
							msg_link++;
						}

						i++;
					} while(Msg_Next(&Number) == TRUE);
	
					if (!do_quiet) {
						printf("%6d / %6lu\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", i, Total);
						fflush(stdout);
					}

					free(Link);
				}

				if (!do_quiet) {
					printf("               \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
					fflush(stdout);
				}
				Msg_UnLock();
			} else {
				Syslog('+', "Can't lock %s", Path);
			}
		}

		Msg_Close();

		if (!do_quiet) {
			printf("\b\b\b\b\b\b\b\b\b\b          \b\b\b\b\b\b\b\b\b\b");
			fflush(stdout);
		}
	}
}



/*
 * Kill messages according to age and max messages.
 */
void KillArea(char *Path, char *Name, int DaysOld, int MaxMsgs, long Areanr)
{
	unsigned long	Number, TotalMsgs = 0, Highest, *Active, Counter = 0;
	int		i, DelCount = 0, DelAge = 0, Done;
	time_t		Today, MsgDate;

	IsDoing("Killing %ld", Areanr);
	Today = time(NULL) / 86400L;

	if (Msg_Open(Path)) {

		if (!do_quiet) {
			colour(12, 0);
			printf(" (Killing)");
			colour(13, 0);
			fflush(stdout);
		}

		if (Msg_Lock(30L)) {

			TotalMsgs = Msg_Number();

			if (TotalMsgs) {
				if ((Active = (unsigned long *)malloc((size_t)((TotalMsgs + 100L) * sizeof(unsigned long)))) != NULL) {
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
					usleep(1);

				if ((!do_quiet) && ((Counter % 10L) == 0)) {
					printf("%6lu / %6lu\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", Counter, TotalMsgs);
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



void IndexArea(char *Path, long Areanr)
{
}



/*
 * Pack message area if there are deleted messages.
 */
void PackArea(char *Path, long Areanr)
{
	IsDoing("Packing %ld", Areanr);
	if (Msg_Open(Path)) {

		if (!do_quiet) {
			colour(12, 0);
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
		usleep(1);
}


