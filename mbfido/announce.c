/*****************************************************************************
 *
 * File ..................: mbaff/announce.c
 * Purpose ...............: Announce new files and FileFind
 * Last modification date : 25-Aug-2000
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
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
#include "../lib/msg.h"
#include "../lib/msgtext.h"
#include "grlist.h"
#include "msgutil.h"
#include "announce.h"


extern int		do_quiet;		/* Supress screen output    */
struct _filerecord	T_File;			/* Internal announce record */
int			TotalFiles;		/* Total announced files    */
unsigned long		TotalSize;		/* Total size in KBytes.    */
int			MsgCount;		/* Message counter	    */



/*
 *  Add a file whos data is in T_File to the toberep.data file.
 */
int Add_ToBeRep(void);
int Add_ToBeRep()
{
	char			*fname;
	struct _filerecord	Temp;
	FILE			*tbr;
	int			Found = FALSE;

	fname = calloc(PATH_MAX, sizeof(char));
	sprintf(fname, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
	if ((tbr = fopen(fname, "a+")) == NULL) {
		WriteError("$Can't create %s", fname);
		free(fname);
		return FALSE;
	}
	free(fname);

	fseek(tbr, 0, SEEK_SET);
	while (fread(&Temp, sizeof(Temp), 1, tbr) == 1) {
		if ((strcmp(Temp.Name, T_File.Name) == 0) &&
		    (Temp.Fdate = T_File.Fdate))
			Found = TRUE;
	}

	if (Found) {
		Syslog('!', "File %s already in toberep.data", T_File.Name);
		fclose(tbr);
		return FALSE;
	}

	fwrite(&T_File, sizeof(T_File), 1, tbr);
	fclose(tbr);
	return TRUE;
}



/*
 *  Check for uploads, these are files in the files database with
 *  the announced flag not yet set.
 */
void Uploads(void);
void Uploads()
{
	FILE	*pAreas, *pFile;
	char	*sAreas, *fAreas;
	int	Count = 0, i = 0, j, k;

	sAreas = calloc(PATH_MAX, sizeof(char));
	fAreas = calloc(PATH_MAX, sizeof(char));

	Syslog('+', "Checking for uploads");
	IsDoing("Check uploads");

	if (!do_quiet) {
		colour(3, 0);
		printf("  Checking uploads...\n");
	}

	sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	if ((pAreas = fopen(sAreas, "r")) == NULL) {
		WriteError("$Can't open %s", sAreas);
		free(sAreas);
		free(fAreas);
		return;
	}
	fread(&areahdr, sizeof(areahdr), 1, pAreas);

	while (fread(&area, areahdr.recsize, 1, pAreas) == 1) {

		i++;

		if (CFG.slow_util && do_quiet)
			usleep(1);

		if ((area.Available) && strlen(area.NewGroup)) {

			if (!do_quiet) {
				printf("\r  %4d => %-44s", i, area.Name);
				fflush(stdout);
			}

			sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), i);
			if ((pFile = fopen(fAreas, "r+")) != NULL) {

				while (fread(&file, sizeof(file), 1, pFile) == 1) {
					if (!file.Announced) {
						Syslog('m', "  %d %s", i, file.Name);
						memset(&T_File, 0, sizeof(T_File));
						sprintf(T_File.Echo, "AREA %d", i);
						sprintf(T_File.Group, "%s", area.NewGroup);
						sprintf(T_File.Comment, "%s", area.Name);
						sprintf(T_File.Name, "%s", file.Name);
						T_File.Size = file.Size;
						T_File.SizeKb = file.Size / 1024;
						T_File.Fdate = file.FileDate;
						sprintf(T_File.Crc, "%08lx", file.Crc32);
						sprintf(T_File.Desc, "%s %s %s %s", file.Desc[0], file.Desc[1], file.Desc[2], file.Desc[3]);
						k = 0;
						for (j = 0; j < 25; j++) {
							if (strlen(file.Desc[j])) {
								sprintf(T_File.LDesc[k], "%s", file.Desc[j]);
								T_File.LDesc[k][49] = '\0';
								k++;
							}
						}
						T_File.TotLdesc = k;
						T_File.Cost = file.Cost;
						T_File.Announce = TRUE;
						if (Add_ToBeRep())
							Count++;
						/*
						 * Mark file is announced.
						 */
						file.Announced = TRUE;
						fseek(pFile, - sizeof(file), SEEK_CUR);
						fwrite(&file, sizeof(file), 1, pFile);
					}
				}

				fclose(pFile);
			}
		}
	}

	if (!do_quiet) {
		printf("\r                                                      \r");
		if (Count)
			printf("  %d new uploads\n", Count);
		fflush(stdout);
	}

	if (Count)
		Syslog('+', "%d new uploads", Count);
	
	fclose(pAreas);
	free(sAreas);
	free(fAreas);
}



int StartMsg(void);
int StartMsg(void)
{
	if (!Msg_Open(newfiles.Area))
		return FALSE;

	if (!Msg_Lock(30L)) {
		Msg_Close();
		return FALSE;
	}
	Msg_New();

	CountPosted(newfiles.Area);

	sprintf(Msg.From, "%s", newfiles.From);
	sprintf(Msg.To, "%s", newfiles.Too);
	if (MsgCount == 1) {
		sprintf(Msg.Subject, "%s", newfiles.Subject);
		TotalSize = TotalFiles = 0;
	} else
		sprintf(Msg.Subject, "%s #%d", newfiles.Subject, MsgCount);
	sprintf(Msg.FromAddress, "%s", aka2str(newfiles.UseAka));
	Msg.Written = time(NULL);
	Msg.Arrived = time(NULL);
	Msg.Local = TRUE;
	Msg.Echomail = TRUE;

	/*
	 * Start message text including kludges
	 */
	Msg_Id(newfiles.UseAka);
	Msg_Pid();
	Msg_Top();
	return TRUE;
}



void FinishMsg(int);
void FinishMsg(int Final)
{
	char	*temp;
	FILE	*fp;

	temp = calloc(PATH_MAX, sizeof(char));

	if (Final) {
		MsgText_Add2((char *)"");
		sprintf(temp, "This is a total of %d files, %lu Kbytes", TotalFiles, TotalSize);
		MsgText_Add2(temp);
	} else {
		MsgText_Add2((char *)"");
		MsgText_Add2((char *)"to be continued...");
		MsgCount++;
	}

	if (strlen(newfiles.Origin))
		Msg_Bot(newfiles.UseAka, newfiles.Origin);
	else
		Msg_Bot(newfiles.UseAka, CFG.origin);

	Msg_AddMsg();
	Msg_UnLock();
	Syslog('+', "Posted message %ld, %d bytes", Msg.Id, Msg.Size);

	sprintf(temp, "%s/tmp/echomail.jam", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "a")) != NULL) {
		fprintf(fp, "%s %lu\n", newfiles.Area, Msg.Id);
		fclose(fp);
	}
	Msg_Close();

	free(temp);
}



void Report(gr_list *);
void Report(gr_list *ta)
{
	FILE		*fp;
	char		*temp;
	int		i, Total = 0;
	unsigned long	Size = 0;

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r")) == NULL) {
		WriteError("$Can't open %s", temp);
		return;
	}

	while (fread(&T_File, sizeof(T_File), 1, fp) == 1) {
		if ((!strcmp(T_File.Echo, ta->echo)) && 
		    (!strcmp(T_File.Group, ta->group)))
			break;
	}

	sprintf(temp, "Area %s - %s", T_File.Echo, T_File.Comment);
	MsgText_Add2(temp);

	fseek(fp, 0, SEEK_SET);
	MsgText_Add2((char *)"------------------------------------------------------------------------");
	while (fread(&T_File, sizeof(T_File), 1, fp) == 1) {
		if ((!strcmp(T_File.Echo, ta->echo)) &&
		    (!strcmp(T_File.Group, ta->group))) {

			if (CFG.slow_util && do_quiet)
				usleep(1);

			sprintf(temp, "%-12s %5lu Kb. %s", tu(T_File.Name), T_File.SizeKb, To_Low(T_File.LDesc[0],newfiles.HiAscii));
			MsgText_Add2(temp);
			if (T_File.TotLdesc > 0)
				for (i = 1; i < T_File.TotLdesc; i++) {
					sprintf(temp, "                       %s", To_Low(T_File.LDesc[i],newfiles.HiAscii));
					MsgText_Add2(temp);
				}
			Total++;
			Size += T_File.SizeKb;

			/*
			 * Split message the hard way.
			 */
			if (Msg.Size > (CFG.new_force * 1024)) {
				FinishMsg(FALSE);
				StartMsg();
			}
		}
	}
	MsgText_Add2((char *)"------------------------------------------------------------------------");

	sprintf(temp, "%d files, %lu Kb", Total, Size);
	MsgText_Add2(temp);
	MsgText_Add2((char *)"");
	MsgText_Add2((char *)"");
	fclose(fp);
	free(temp);

	/*
	 * Split messages the gently way.
	 */
	if (Msg.Size > (CFG.new_split * 1024)) {
		FinishMsg(FALSE);
		StartMsg();
	}

	TotalFiles += Total;
	TotalSize += Size;
}



int Announce()
{
	gr_list		*fgr = NULL, *tmp;
	char		*temp;
	FILE		*fp;
	int		Count = 0, rc = FALSE;
	long		filepos;
	char		group[13];
	int		i, groups, any;

	if (!do_quiet) {
		colour(3, 0);
		printf("Announce new files\n");
	}

	Uploads();

	IsDoing("Announce files");

	temp = calloc(128, sizeof(char));
	sprintf(temp, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r")) == NULL) {
		Syslog('+', "No new files to announce");
		free(temp);
		if (!do_quiet) {
			printf("  No new files.\n");
		}
		return FALSE;
	}

	if (!do_quiet)
		printf("  Preparing reports...\n");

	while (fread(&T_File, sizeof(T_File), 1, fp) == 1) {
		if (T_File.Announce) {
			fill_grlist(&fgr, T_File.Group, T_File.Echo);
			Count++;
		}
	}

	fclose(fp);

	if (Count == 0) {
		unlink(temp);
		if (!do_quiet) 
			printf("  No new files.\n");
		Syslog('+', "No new files to announce");
		free(temp);
		return FALSE;
	}

	if (!do_quiet) 
		printf("  %d new files found\n", Count);

	sort_grlist(&fgr);

	/*
	 *  At this point we have a sorted list of groups with a counter
	 *  indicating howmany files to report in each group.
	 */
	sprintf(temp, "%s/etc/newfiles.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r")) == NULL) {
		WriteError("$Can't open %s", temp);
		if (!do_quiet)
			printf("  No newfile reports defined\n");
		free(temp);
		return FALSE;
	}
	fread(&newfileshdr, sizeof(newfileshdr), 1, fp);
	groups = newfileshdr.grpsize / 13;

	while (fread(&newfiles, newfileshdr.recsize, 1, fp) == 1) {
		if (newfiles.Active) {
			filepos = ftell(fp);
			if (!do_quiet) {
				printf("  %s\n", newfiles.Comment);
			}
			any = FALSE;

			for (i = 0; i < groups; i++) {
				fread(&group, 13, 1, fp);
				for (tmp = fgr; tmp; tmp = tmp->next)
					if (strcmp(tmp->group, group) == 0)
						any = TRUE;
			}
			if (any) {
				fseek(fp, filepos, SEEK_SET);
				rc = TRUE;
				Syslog('+', "Create report: %s", newfiles.Comment);
				MsgCount = 1;
				if (StartMsg()) {
					while (fread(&group, 13, 1, fp) == 1) {
						for (tmp = fgr; tmp; tmp = tmp->next)
							if (!strcmp(tmp->group, group)) {
								Report(tmp);
							}
					}
				}
				FinishMsg(TRUE);
			}

			fseek(fp, filepos, SEEK_SET);
		}

		fseek(fp, newfileshdr.grpsize, SEEK_CUR);
	}
	fclose(fp);

	tidy_grlist(&fgr);

	if (rc) {
		sprintf(temp, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
		unlink(temp);
	}

	free(temp);
	return rc;
}


