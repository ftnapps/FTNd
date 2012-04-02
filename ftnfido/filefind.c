/*****************************************************************************
 *
 * $Id: filefind.c,v 1.26 2007/10/15 12:48:11 mbse Exp $
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
#include "fflist.h"
#include "filefind.h"
#include "msgutil.h"

/*
 *  The next constants are to prevent overflowing the echomail areas
 *  with huge replies. MAX_DESC_LINES limits the number of file description
 *  lines, 5 should be enough. The other two are the maximum files to report
 *  if in the same area or different area.
 *  For netmail replies there is a different limit.
 */
#define	MAX_DESC_LINES		5
#define	MAX_FILES_SAMEBOARD	25
#define	MAX_FILES_OTHERBOARD	50
#define MAX_FILES_NETMAIL	100


extern int		do_quiet;		/* Suppress screen output   */
struct _filerecord	T_File;			/* Internal announce record */
int			Requests = 0;		/* Total found request      */
int			Replies = 0;		/* Total generated replies  */


void Back(int);
void Back(int count)
{
    int	i;

    if (do_quiet)
	return;

    for (i = 0; i < count; i++)
	printf("\b");
    fflush(stdout);
}



void Clean(int);
void Clean(int count)
{
    int	i;

    if (do_quiet)
	return;

    for (i = 0; i < count; i++)
	printf(" ");
    Back(count);
}



void ScanArea(ff_list **);
void ScanArea(ff_list **ffl)
{
    unsigned int    Number, Highest;

    if (!do_quiet) {
	mbse_colour(CYAN, BLACK);
	printf("\r  %-40s", scanmgr.Comment);
	mbse_colour(LIGHTRED, BLACK);
	printf(" (Scanning) ");
	mbse_colour(LIGHTMAGENTA, BLACK);
	fflush(stdout);
    }
    Syslog('+', "Scanning %s", scanmgr.Comment);
    if (Msg_Open(scanmgr.ScanBoard)) {
	Number  = Msg_Lowest();
	Highest = Msg_Highest();

	do {
	   if (!do_quiet) {
		printf("%6u / %6u", Number, Highest);
		Back(15);
	    }

	    if (CFG.slow_util && do_quiet)
		msleep(1);

	    if (Msg_ReadHeader(Number) == TRUE) {
		if (((!strcasecmp(Msg.To, "allfix")) || (!strcasecmp(Msg.To, "filefind"))) && (!Msg.Received)) {
		    Syslog('m', "Msg: %s (%u) [%s]", Msg.From, Number, Msg.Subject);
		    Msg.Received = TRUE;
		    Msg.Read = time(NULL);
		    if (Msg_Lock(30L)) {
			Msg_WriteHeader(Number);
			Msg_UnLock();
			Syslog('m', "Marked message received");
		    }
		    if (strlen(Msg.Subject) && strlen(Msg.FromAddress)) {
			fill_fflist(ffl);
			Requests++;
		    }
		}
	    }

	} while (Msg_Next(&Number) == TRUE);

	Msg_Close();
	Clean(15);
    } else
	WriteError("$Can't open %s", scanmgr.ScanBoard);

    Back(54);
    Clean(54);
}



int StartReply(ff_list *);
int StartReply(ff_list *ffl)
{
    char	    *temp;
    unsigned int    crc = -1;

    if (strlen(scanmgr.ReplBoard)) {
	if (!Msg_Open(scanmgr.ReplBoard))
	    return -1;
	else
	    CountPosted(scanmgr.ReplBoard);
    } else {
	if (!Msg_Open(scanmgr.ScanBoard))
	    return -1;
	else
	    CountPosted(scanmgr.ScanBoard);
    }

    if (!Msg_Lock(30L)) {
	Msg_Close();
	return -1;
    }
    Msg_New();

    chartran_init((char *)"CP437", get_ic_ftn(scanmgr.charset), 'f');
    temp = calloc(PATH_MAX, sizeof(char));

    snprintf(Msg.From, 101, "%s", CFG.sysop_name);
    snprintf(Msg.To, 101, "%s", chartran(ffl->from));
    snprintf(Msg.Subject, 101, "Re: %s", chartran(ffl->subject));
    snprintf(Msg.FromAddress, 101, "%s", aka2str(scanmgr.Aka));
    Msg.Written = time(NULL);
    Msg.Arrived = time(NULL);
    Msg.Local   = TRUE;
    if (scanmgr.NetReply){
	Msg.Netmail = TRUE;
	snprintf(Msg.ToAddress, 101, "%d:%d/%d.%d", ffl->zone, ffl->net, ffl->node, ffl->point);
	Msg.Private = TRUE;
    } else
	Msg.Echomail = TRUE;

    /*
     *  Start message text including kludges
     */
    Msg_Id(scanmgr.Aka);
    snprintf(temp, PATH_MAX, "\001REPLY: %s", ffl->msgid);
    MsgText_Add2(temp);
    Msg.ReplyCRC = upd_crc32(temp, crc, strlen(temp));
    free(temp);
    Msg_Pid(scanmgr.charset);
    return Msg_Top(scanmgr.template, scanmgr.Language, scanmgr.Aka);
}



void FinishReply(int, int, int);
void FinishReply(int Reported, int Total, int filepos)
{
    char    *temp;
    FILE    *fp, *fi;

    temp = calloc(PATH_MAX, sizeof(char));

    if ((fi = OpenMacro(scanmgr.template, scanmgr.Language, FALSE)) != NULL) {
	MacroVars("CD", "dd", Reported, Total);
	fseek(fi, filepos, SEEK_SET);
	Msg_Macro(fi);
	fclose(fi);
	MacroClear();
    }
    
    if (strlen(scanmgr.Origin))
	Msg_Bot(scanmgr.Aka, scanmgr.Origin, scanmgr.template);
    else
	Msg_Bot(scanmgr.Aka, CFG.origin, scanmgr.template);
    Msg_AddMsg();
    Msg_UnLock();
    Syslog('+', "Posted message %ld", Msg.Id);

    chartran_close();

    snprintf(temp, PATH_MAX, "%s/tmp/%smail.jam", getenv("MBSE_ROOT"), scanmgr.NetReply?"net":"echo");
    if ((fp = fopen(temp, "a")) != NULL) {
	if (strlen(scanmgr.ReplBoard))
	    fprintf(fp, "%s %u\n", scanmgr.ReplBoard, Msg.Id);
	else
	    fprintf(fp, "%s %u\n", scanmgr.ScanBoard, Msg.Id);
	fclose(fp);
    }

    Msg_Close();
    free(temp);
}



/*
 *  Scan for files for one request.
 */
void ScanFiles(ff_list *);
void ScanFiles(ff_list *tmp)
{
    char	    *temp, *kwd, *BigDesc, *line;
    FILE	    *pAreas, *fi;
    unsigned int    areanr = 0, found = 0, SubSize = 0;
    int		    i, j, k, keywrd, Found;
    rf_list	    *rfl = NULL, *rft;
    int		    Rep = 0, Sub = 0, Stop = FALSE;
    int		    filepos, filepos1 = 0, filepos2 = 0, filepos3 = 0, filepos4 = 0;
    struct _fdbarea *fdb_area = NULL;

    kwd  = calloc(81, sizeof(char));
    temp = calloc(PATH_MAX, sizeof(char));

    snprintf(temp, PATH_MAX, "%s (%d:%d/%d.%d)", tmp->from, tmp->zone, tmp->net, tmp->node, tmp->point);
    Syslog('+', "Search [%s] for %s", tmp->subject, temp);

    if (!do_quiet) {
	mbse_colour(CYAN, BLACK);
	temp[40] = '\0';
	printf("\r  %-40s", temp);
	mbse_colour(LIGHTRED, BLACK);
	printf(" (Searching)");
	mbse_colour(LIGHTMAGENTA, BLACK);
	fflush(stdout);
    }

    snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((pAreas = fopen(temp, "r")) != NULL) {
	fread(&areahdr, sizeof(areahdr), 1, pAreas);

	while (fread(&area, areahdr.recsize, 1, pAreas) == 1) {
	    areanr++;
	    Nopper();

	    if (CFG.slow_util && do_quiet)
		msleep(1);

	    if (!do_quiet) {
		printf("%6u / %6u", areanr, found);
		Back(15);
	    }
	    if (area.Available && area.FileFind) {
		if ((fdb_area = mbsedb_OpenFDB(areanr, 30))) {
		    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
			BigDesc = xstrcpy((char *)"");
			for (i = 0; i < 25; i++)
			    BigDesc = xstrcat(BigDesc, fdb.Desc[i]);
			snprintf(temp, PATH_MAX, "%s", tmp->subject);

			Found = FALSE;
			while (strlen(temp) && (!Found)) {
			    /*
			     *  Split the search request in separate words.
			     */
			    k = strlen(temp);
			    for (i = 0; i < k; i++)
				if (temp[i] == ' ')
				    break;
			    if (i < k) {
				strncpy(kwd, temp, i);
				kwd[i] = '\0';
				for (j = 0; j < (k - i -1); j++)
				    temp[j] = temp[j+i+1];
				temp[j] = '\0';
			    } else {
				snprintf(kwd, 80, "%s", temp);
				temp[0] = '\0';
			    }

			    /*
			     *  Check if it's a filename search or a keyword search.
			     */
			    keywrd = FALSE;
			    if ((kwd[0] == '/') || (kwd[0] == '\\')) {
				keywrd = TRUE;
				for (i = 1; i < strlen(kwd); i++)
				    kwd[i-1] = kwd[i];
				kwd[i-1] = '\0';
			    }

			    if (strlen(kwd) > scanmgr.keywordlen) {
				if ((strcasestr(fdb.Name, kwd) != NULL) || (strcasestr(fdb.LName, kwd) != NULL)) {
				    Found = TRUE;
				    Syslog('m', " Found '%s' in %s in filename", kwd, fdb.LName);
				}
				if (keywrd && (strcasestr(BigDesc, kwd) != NULL)) {
				    Found = TRUE;
				    Syslog('m', " Found '%s' in %s in description", kwd, fdb.LName);
				}
			    }
			} /* while (strlen(temp) && (!Found)) */
			if (Found) {
			    found++;
			    Syslog('+', " Found %s in area %d", fdb.LName, areanr);
			    fill_rflist(&rfl, fdb.LName, areanr);
			}
			free(BigDesc);
			BigDesc = NULL;
		    }
		    mbsedb_CloseFDB(fdb_area);
		} else
		    WriteError("$Can't open %s", temp);
	    }
	}
	fclose(pAreas);
	Clean(15);
    } else
	WriteError("$Can't open %s", temp);

    Back(12);
    Clean(12);

    if (found) {
	if (!do_quiet) {
	    mbse_colour(YELLOW, BLACK);
	    printf(" (Replying)");
	    fflush(stdout);
	}

	if (((filepos = StartReply(tmp)) != -1) && ((fi = OpenMacro(scanmgr.template, scanmgr.Language, FALSE)) != NULL)) {
	    areanr = 0;

	    snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	    if ((pAreas = fopen(temp, "r")) != NULL) {
		fread(&areahdr, sizeof(areahdr), 1, pAreas);

		for (rft = rfl; rft; rft = rft->next) {

		    /*
		     * Area footer
		     */
		    if ((areanr != rft->area) && (Sub)) {
			fseek(fi, filepos3, SEEK_SET);
			MacroVars("AB", "dd", Sub, SubSize / 1024);
			Msg_Macro(fi);
			filepos4 = ftell(fi);
			Sub = 0;
			SubSize = 0;
		    }

		    /*
		     * New area header
		     */
		    if (areanr != rft->area) {
			fseek(pAreas, ((rft->area - 1) * areahdr.recsize) + areahdr.hdrsize, SEEK_SET);
			fread(&area, areahdr.recsize, 1, pAreas);
			MacroVars("GJ", "ds", rft->area, area.Name);
			fseek(fi, filepos, SEEK_SET);
			Msg_Macro(fi);
			filepos1 = ftell(fi);
			areanr = rft->area;
		    }

		    Syslog('m', "rp: %d %s", rft->area, rft->filename);

		    if ((fdb_area = mbsedb_OpenFDB(rft->area, 30))) {
			while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1)
			    if (!strcmp(rft->filename, fdb.LName))
				break;
			mbsedb_CloseFDB(fdb_area);
			MacroVars("slbkdt", "ssddss", fdb.Name, fdb.LName, fdb.Size, fdb.Size / 1024, " ",
					chartran(fdb.Desc[0]));
			fseek(fi, filepos1, SEEK_SET);
			Msg_Macro(fi);
			filepos2 = ftell(fi);
			SubSize += fdb.Size;

			/*
			 * We add no more then 5 description lines
			 * to prevent unnecesary long messages.
			 */
			for (i = 1; i < MAX_DESC_LINES; i++) {
			    MacroVars("t", "s", chartran(fdb.Desc[i]));
			    fseek(fi, filepos2, SEEK_SET);
			    if (strlen(fdb.Desc[i])) {
				Msg_Macro(fi);
			    } else {
				line = calloc(255, sizeof(char));
				while ((fgets(line, 254, fi) != NULL) && ((line[0]!='@') || (line[1]!='|'))) {}
				free(line);
			    }
			    filepos3 = ftell(fi);
			}
		    }
		    Rep++;
		    Sub++;

		    if (!scanmgr.NetReply) {
			if (strlen(scanmgr.ReplBoard)) {
			    if (Rep >= MAX_FILES_OTHERBOARD) 
				Stop = TRUE;
			} else {
			    if (Rep >= MAX_FILES_SAMEBOARD)
				Stop = TRUE;
			}
		    } else {
			if (Rep >= MAX_FILES_NETMAIL)
			    Stop = TRUE;
		    }
		    if (Stop)
			break;
		}

		if (Sub) {
		    fseek(fi, filepos3, SEEK_SET);
		    MacroVars("AB", "dd", Sub, SubSize / 1024);
		    Msg_Macro(fi);
		    filepos4 = ftell(fi);
		}

		fclose(pAreas);
	    }
	    FinishReply(Rep, found, filepos4);
	    Replies++;
	}

	Back(11);
	Clean(11);
    }

    Back(42);
    Clean(42);

    tidy_rflist(&rfl);
    free(BigDesc);
    free(temp);
    free(kwd);
}



int Filefind()
{
    char	*temp;
    int		rc = FALSE;
    FILE	*fp;
    ff_list	*ffl = NULL, *tmp;

    IsDoing("FileFind");

    if (!do_quiet) {
	mbse_colour(CYAN, BLACK);
	printf("Processing FileFind requests\n");
    }
    Syslog('+', "Processing FileFind requests");
    temp = calloc(PATH_MAX, sizeof(char));

    snprintf(temp, PATH_MAX, "%s/etc/scanmgr.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return FALSE;
    }
    fread(&scanmgrhdr, sizeof(scanmgrhdr), 1, fp);

    while (fread(&scanmgr, scanmgrhdr.recsize, 1, fp) == 1) {
	if (scanmgr.Active) {
	    ScanArea(&ffl);
	    Nopper();
	    for (tmp = ffl; tmp; tmp = tmp->next) {
		ScanFiles(tmp);
	    }
	    tidy_fflist(&ffl);
	}
    }
    fclose(fp);
    free(temp);

    if (Requests) {
	Syslog('+', "Processed %d requests, created %d replies", Requests, Replies);
	if (Replies)
	    rc = TRUE;
	if (!do_quiet) {
	    mbse_colour(CYAN, BLACK);
	    printf("Processed %d requests, created %d replies\n", Requests, Replies);
	}
    }

    return rc;
}


