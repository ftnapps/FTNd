/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Announce new files and FileFind
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
#include "../lib/msgtext.h"
#include "../lib/diesel.h"
#include "grlist.h"
#include "msgutil.h"
#include "announce.h"


extern int		do_quiet;		/* Suppress screen output   */
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
    char		*fname;
    struct _filerecord	Temp;
    FILE		*tbr;
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
	if ((strcmp(Temp.Name, T_File.Name) == 0) && (Temp.Fdate = T_File.Fdate))
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
    FILE    *pAreas, *pFile;
    char    *sAreas, *fAreas;
    int	    Count = 0, i = 0, j, k;

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
	    msleep(1);

	if ((area.Available) && strlen(area.NewGroup)) {

	    if (!do_quiet) {
		printf("\r  %4d => %-44s", i, area.Name);
		fflush(stdout);
	    }

	    sprintf(fAreas, "%s/fdb/fdb%d.data", getenv("MBSE_ROOT"), i);
	    if ((pFile = fopen(fAreas, "r+")) != NULL) {

		while (fread(&file, sizeof(file), 1, pFile) == 1) {
		    Nopper();
		    if (!file.Announced) {
			Syslog('m', "  %d %s", i, file.Name);
			memset(&T_File, 0, sizeof(T_File));
			sprintf(T_File.Echo, "AREA %d", i);
			sprintf(T_File.Group, "%s", area.NewGroup);
			sprintf(T_File.Comment, "%s", area.Name);
			sprintf(T_File.Name, "%s", file.Name);
			sprintf(T_File.LName, "%s", file.LName);
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



long StartMsg(void);
long StartMsg(void)
{
    if (!Msg_Open(newfiles.Area))
	return -1;

    if (!Msg_Lock(30L)) {
	Msg_Close();
	return -1;
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
    return Msg_Top(newfiles.Template, newfiles.Language, newfiles.UseAka);
}



void FinishMsg(int, long);
void FinishMsg(int Final, long filepos)
{
    char    *temp;
    FILE    *fp, *fi;

    temp = calloc(PATH_MAX, sizeof(char));

    if (Final && ((fi = OpenMacro(newfiles.Template, newfiles.Language, FALSE)) != NULL)) {
	/*
	 * Message footer
	 */
	MacroVars("CD", "dd", TotalFiles, TotalSize);
	fseek(fi, filepos, SEEK_SET);
	Msg_Macro(fi);
	fclose(fi);
	MacroClear();
    }

    if (strlen(newfiles.Origin))
	Msg_Bot(newfiles.UseAka, newfiles.Origin, newfiles.Template);
    else
	Msg_Bot(newfiles.UseAka, CFG.origin, newfiles.Template);

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



/*
 * Report one group block of new files.
 */
long Report(gr_list *, long);
long Report(gr_list *ta, long filepos)
{
    FILE	    *fp, *fi;
    char	    *temp, *line;
    int		    i, Total = 0;
    unsigned long   Size = 0;
    long	    filepos1 = 0, filepos2, filepos3 = 0, finalpos = 0;
    time_t	    ftime;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	WriteError("$Can't open %s", temp);
	return 0;
    }

    MacroVars("GJZ", "ssd", "", "", 0);
    MacroVars("slbkdt", "ssddss", "", "", 0, 0, "", "");
    MacroVars("ABZ", "ddd", 0, 0, 0);

    while (fread(&T_File, sizeof(T_File), 1, fp) == 1) {
	if ((!strcmp(T_File.Echo, ta->echo)) && (!strcmp(T_File.Group, ta->group)))
	    break;
    }

    if ((fi = OpenMacro(newfiles.Template, newfiles.Language, FALSE)) != NULL) {
	/*
	 * Area block header
	 */
	MacroVars("GJZ", "ssd", T_File.Echo, T_File.Comment, 0);
	fseek(fi, filepos, SEEK_SET);
	Msg_Macro(fi);
	filepos1 = ftell(fi);
    } else {
	free(temp);
	return 0;
    }

    fseek(fp, 0, SEEK_SET);
    while (fread(&T_File, sizeof(T_File), 1, fp) == 1) {
	if ((!strcmp(T_File.Echo, ta->echo)) && (!strcmp(T_File.Group, ta->group))) {

	    if (CFG.slow_util && do_quiet)
		msleep(1);

	    /*
	     * Report one newfile, first line.
	     */
	    fseek(fi, filepos1, SEEK_SET);
	    ftime = T_File.Fdate;
	    MacroVars("sl", "ss", T_File.Name, T_File.LName);
	    MacroVars("bk", "dd", T_File.Size, T_File.SizeKb);
	    MacroVars("dt", "ss", rfcdate(ftime), To_Low(T_File.LDesc[0],newfiles.HiAscii));
	    Msg_Macro(fi);
	    filepos2 = ftell(fi);

	    /*
	     * Extra description lines follow
	     */
	    for (i = 1; i < 24; i++) {
		MacroVars("t", "s", To_Low(T_File.LDesc[i],newfiles.HiAscii));
		fseek(fi, filepos2, SEEK_SET);
		if (strlen(T_File.LDesc[i])) {
		    Msg_Macro(fi);
		} else {
		    line = calloc(MAXSTR, sizeof(char));
		    while ((fgets(line, MAXSTR-2, fi) != NULL) && ((line[0]!='@') || (line[1]!='|'))) {}
		    free(line);
		}
		filepos3 = ftell(fi);
	    }
	    Total++;
	    Size += T_File.SizeKb;
	}
    }

    /*
     * Area block footer
     */
    if (Msg.Size > (CFG.new_split * 1024))
	MacroVars("ABZ", "ddd", Total, Size, 1);
    else
	MacroVars("ABZ", "ddd", Total, Size, 0);
    fseek(fi, filepos3, SEEK_SET);
    Msg_Macro(fi);
    finalpos = ftell(fi);
    fclose(fp);
    free(temp);

    /*
     * Split messages if too big.
     */
    if (Msg.Size > (CFG.new_split * 1024)) {
	MsgCount++;
	FinishMsg(FALSE, finalpos);
	StartMsg();
    }

    TotalFiles += Total;
    TotalSize += Size;

    if (fi != NULL) {
	fclose(fi);
    }
    return finalpos;
}



int Announce()
{
    gr_list	*fgr = NULL, *tmp;
    char	*temp;
    FILE	*fp;
    int		Count = 0, rc = FALSE;
    long	filepos, filepos1, filepos2;
    char	group[13];
    int		i, groups, any;

    if (!do_quiet) {
	colour(3, 0);
	printf("Announce new files\n");
    }

    Uploads();
    IsDoing("Announce files");

    temp = calloc(PATH_MAX, sizeof(char));
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
	    if (!do_quiet)
		printf("  %s\n", newfiles.Comment);
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
		if ((filepos1 = StartMsg()) != -1) {
		    filepos2 = 0;
		    while (fread(&group, 13, 1, fp) == 1) {
			for (tmp = fgr; tmp; tmp = tmp->next) {
			    if (!strcmp(tmp->group, group)) {
				filepos2 = Report(tmp, filepos1);
			    }
			}
		    }
		    FinishMsg(TRUE, filepos2);
		}
	    } else {
		if (!do_quiet)
		    printf("    No matching groups\n");
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


