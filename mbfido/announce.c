/*****************************************************************************
 *
 * $Id: announce.c,v 1.37 2007/03/02 13:23:36 mbse Exp $
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
#include "../lib/msgtext.h"
#include "../lib/diesel.h"
#include "grlist.h"
#include "msgutil.h"
#include "toberep.h"
#include "announce.h"


extern int		do_quiet;		/* Suppress screen output   */
struct _filerecord	T_File;			/* Internal announce record */
int			TotalFiles;		/* Total announced files    */
unsigned int		TotalSize;		/* Total size in KBytes.    */
int			MsgCount;		/* Message counter	    */



/*
 *  Check for uploads, these are files in the files database with
 *  the announced flag not yet set.
 */
void Uploads(void);
void Uploads()
{
    FILE    *pAreas;
    char    *sAreas;
    int	    Count = 0, i = 0, j, k;
    struct _fdbarea *fdb_area = NULL;

    sAreas = calloc(PATH_MAX, sizeof(char));

    Syslog('+', "Checking for uploads");
    IsDoing("Check uploads");

    if (!do_quiet) {
	mbse_colour(CYAN, BLACK);
	printf("  Checking uploads...\n");
    }

    snprintf(sAreas, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen(sAreas, "r")) == NULL) {
	WriteError("$Can't open %s", sAreas);
	free(sAreas);
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

	    if ((fdb_area = mbsedb_OpenFDB(i, 30))) {
		while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
		    Nopper();
		    if (!fdb.Announced) {
			Syslog('m', "  %d %s", i, fdb.Name);
			memset(&T_File, 0, sizeof(T_File));
			if (strlen(fdb.TicArea))
			    strncpy(T_File.Echo, fdb.TicArea, sizeof(T_File.Echo) -1);
			else
			    snprintf(T_File.Echo, 21, "AREA %d", i);
			strncpy(T_File.Group, area.NewGroup, sizeof(T_File.Group) -1);
			strncpy(T_File.Comment, area.Name, sizeof(T_File.Comment) -1);
			strncpy(T_File.Name, fdb.Name, sizeof(T_File.Name) -1);
			strncpy(T_File.LName, fdb.LName, sizeof(T_File.LName) -1);
			if (strlen(fdb.Magic))
			    strncpy(T_File.Magic, fdb.Magic, sizeof(T_File.Magic) -1);
			T_File.Size = fdb.Size;
			T_File.SizeKb = fdb.Size / 1024;
			T_File.Fdate = fdb.FileDate;
			snprintf(T_File.Crc, 9, "%08x", fdb.Crc32);
			snprintf(T_File.Desc, 256, "%s %s %s %s", fdb.Desc[0], fdb.Desc[1], fdb.Desc[2], fdb.Desc[3]);
			k = 0;
			for (j = 0; j < 25; j++) {
			    if (strlen(fdb.Desc[j])) {
				snprintf(T_File.LDesc[k], 49, "%s", fdb.Desc[j]);
				T_File.LDesc[k][49] = '\0';
				k++;
			    }
			}
			T_File.TotLdesc = k;
			T_File.Announce = TRUE;
			if (Add_ToBeRep(T_File))
			    Count++;
			/*
			 * Mark file is announced.
			 */
			fdb.Announced = TRUE;
			if (mbsedb_LockFDB(fdb_area, 30)) {
			    fseek(fdb_area->fp, - fdbhdr.recsize, SEEK_CUR);
			    fwrite(&fdb, fdbhdr.recsize, 1, fdb_area->fp);
			    mbsedb_UnlockFDB(fdb_area);
			}
		    }
		}

		mbsedb_CloseFDB(fdb_area);
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
}



int StartMsg(void);
int StartMsg(void)
{
    Syslog('m', "StartMsg()");
    if (!Msg_Open(newfiles.Area))
	return -1;

    if (!Msg_Lock(30L)) {
	Msg_Close();
	return -1;
    }
    Msg_New();

    CountPosted(newfiles.Area);

    chartran_init((char *)"CP437", get_ic_ftn(newfiles.charset), 'f');

    snprintf(Msg.From, 101, "%s", chartran(newfiles.From));
    snprintf(Msg.To, 101, "%s", chartran(newfiles.Too));
    if (MsgCount == 1) {
	snprintf(Msg.Subject, 101, "%s", chartran(newfiles.Subject));
	TotalSize = TotalFiles = 0;
    } else
	snprintf(Msg.Subject, 101, "%s #%d", chartran(newfiles.Subject), MsgCount);
    snprintf(Msg.FromAddress, 101, "%s", aka2str(newfiles.UseAka));
    Msg.Written = time(NULL);
    Msg.Arrived = time(NULL);
    Msg.Local = TRUE;
    Msg.Echomail = TRUE;

    /*
     * Start message text including kludges
     */
    Msg_Id(newfiles.UseAka);
    Msg_Pid(newfiles.charset);
    return Msg_Top(newfiles.Template, newfiles.Language, newfiles.UseAka);
}



void FinishMsg(int, int);
void FinishMsg(int Final, int filepos)
{
    char    *temp;
    FILE    *fp, *fi;

    Syslog('m', "FinishMsg(%s, %d)", Final ? "TRUE":"FALSE", filepos);

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

    chartran_close();

    snprintf(temp, PATH_MAX, "%s/tmp/echomail.jam", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "a")) != NULL) {
	fprintf(fp, "%s %u\n", newfiles.Area, Msg.Id);
	fclose(fp);
    }
    Msg_Close();

    free(temp);
}



/*
 * Report one group block of new files.
 */
int Report(gr_list *, int);
int Report(gr_list *ta, int filepos)
{
    FILE	    *fp, *fi;
    char	    *temp, *line;
    int		    i, Total = 0;
    unsigned int    Size = 0;
    int		    filepos1 = 0, filepos2, filepos3 = 0, finalpos = 0;
    time_t	    ftime;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
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

	Syslog('m', "Announce %s %s %s", T_File.Echo, T_File.Name, chartran(T_File.LName));
    if ((fi = OpenMacro(newfiles.Template, newfiles.Language, FALSE)) != NULL) {
	/*
	 * Area block header
	 */
	MacroVars("GJZ", "ssd", T_File.Echo, chartran(T_File.Comment), 0);
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
	    MacroVars("dt", "ss", rfcdate(ftime), chartran(T_File.LDesc[0]));
	    Msg_Macro(fi);
	    filepos2 = ftell(fi);

	    /*
	     * Extra description lines follow
	     */
	    for (i = 1; i < 24; i++) {
		fseek(fi, filepos2, SEEK_SET);
		if (strlen(T_File.LDesc[i])) {
		    MacroVars("t", "s", chartran(T_File.LDesc[i]));
		    Msg_Macro(fi);
		} else {
		    line = calloc(MAXSTR, sizeof(char));
		    while ((fgets(line, MAXSTR-2, fi) != NULL) && ((line[0]!='@') || (line[1]!='|'))) {}
		    free(line);
		}
		filepos3 = ftell(fi);
	    }

	    /*
	     * Magic request
	     */
	    if (strlen(T_File.Magic)) {
		MacroVars("u", "s", T_File.Magic);
		Msg_Macro(fi);
	    } else {
		line = calloc(MAXSTR, sizeof(char));
		while ((fgets(line, MAXSTR-2, fi) != NULL) && ((line[0]!='@') || (line[1]!='|'))) {}
		free(line);
	    }
	    filepos3 = ftell(fi);
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
	Syslog('m', "Report() splitting report");
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
    int		filepos, filepos1, filepos2;
    char	group[13];
    int		i, groups, any;

    if (!do_quiet) {
	mbse_colour(CYAN, BLACK);
	printf("Announce new files\n");
    }

    Uploads();
    IsDoing("Announce files");

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
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
    snprintf(temp, PATH_MAX, "%s/etc/newfiles.data", getenv("MBSE_ROOT"));
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
	snprintf(temp, PATH_MAX, "%s/etc/toberep.data", getenv("MBSE_ROOT"));
	unlink(temp);
    }

    free(temp);
    return rc;
}


