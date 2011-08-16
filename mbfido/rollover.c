/*****************************************************************************
 *
 * $Id: rollover.c,v 1.13 2005/10/16 11:37:54 mbse Exp $
 * Purpose ...............: Statistic rollover util.
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "rollover.h"


extern int	do_quiet;


void StatAdd(statcnt *S, unsigned int V)
{
    S->total += V;
    S->tweek += V;
    S->tdow[Diw] += V;
    S->month[Miy] += V;
}



void RollWeek(statcnt *);
void RollWeek(statcnt *S)
{
    int	i;

    for (i = 0; i < 7; i++) {
	S->ldow[i] = S->tdow[i];
	S->tdow[i] = 0;
    }

    S->lweek = S->tweek;
    S->tweek = 0;

    if (CFG.slow_util && do_quiet)
	msleep(1);
}



FILE *OpenData(char *);
FILE *OpenData(char *Name)
{
    char    *temp;
    FILE    *fp;

    temp = calloc(PATH_MAX, sizeof(char));

    snprintf(temp, PATH_MAX, "%s/etc/%s", getenv("MBSE_ROOT"), Name);
    if ((fp = fopen(temp, "r+")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return NULL;
    }

    free(temp);
    return fp;
}



/*
 *  Test all files with statistic counters if a new week has started
 *  or a new month has started. All the record counters will be 
 *  updated if one of these is the case.
 */
void Rollover()
{
    time_t	    Now, Temp;
    struct tm	    *t;
    FILE	    *fp, *ft;
    int		    do_week, do_month, Day, i;
    char	    *temp, *temp1;
    struct _history history;

    Now = time(NULL);
    t = localtime(&Now);

    Diw = t->tm_wday;
    Miy = t->tm_mon;
    Day = t->tm_yday;

    if ((fp = OpenData((char *)"nodes.data")) != NULL) {
	fread(&nodeshdr, sizeof(nodeshdr), 1, fp);
	Temp = (time_t)nodeshdr.lastupd;
	t = localtime(&Temp);

	/*
	 *  Test if it's sunday, and the last update wasn't today.
	 *  If it's not sunday, and the last update was more then
	 *  7 days ago, we maybe missed last sunday and the update
	 *  is still done.
	 */
	if (((Diw == 0) && (Day != t->tm_yday)) || ((Day - t->tm_yday) > 7))
	    do_week = TRUE;
	else
	    do_week = FALSE;

	/*
	 *  If the month is different then the last update, we must
	 *  be in a new month.
	 */
	if (Miy != t->tm_mon)
	    do_month = TRUE;
	else
	    do_month = FALSE;

	if (do_week || do_month) {
	    IsDoing("Date rollover");
	    Syslog('+', "Rollover nodes.data");

	    while (fread(&nodes, nodeshdr.recsize, 1, fp) == 1) {
		if (do_week) {
		    RollWeek(&nodes.FilesSent);
		    RollWeek(&nodes.FilesRcvd);
		    RollWeek(&nodes.F_KbSent);
		    RollWeek(&nodes.F_KbRcvd);
		    RollWeek(&nodes.MailSent);
		    RollWeek(&nodes.MailRcvd);
		}
		if (do_month) {
		    nodes.FilesSent.month[Miy] = 0;
		    nodes.FilesRcvd.month[Miy] = 0;
		    nodes.F_KbSent.month[Miy] = 0;
		    nodes.F_KbRcvd.month[Miy] = 0;
		    nodes.MailSent.month[Miy] = 0;
		    nodes.MailRcvd.month[Miy] = 0;
		    if (CFG.slow_util && do_quiet)
			msleep(1);
		}
		fseek(fp, - nodeshdr.recsize, SEEK_CUR);
		fwrite(&nodes, nodeshdr.recsize, 1, fp);
		fseek(fp, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
	    }

	    fseek(fp, 0, SEEK_SET);
	    nodeshdr.lastupd = (int)time(NULL);
	    fwrite(&nodeshdr, nodeshdr.hdrsize, 1, fp);
	}

	fclose(fp);
    }

    if ((fp = OpenData((char *)"mareas.data")) != NULL) {
	fread(&msgshdr, sizeof(msgshdr), 1, fp);
	Temp = (time_t)msgshdr.lastupd;
	t = localtime(&Temp);

	if (((Diw == 0) && (Day != t->tm_yday)) || ((Day - t->tm_yday) > 7))
	    do_week = TRUE;
	else
	    do_week = FALSE;
	if (Miy != t->tm_mon)
	    do_month = TRUE;
	else
	    do_month = FALSE;

	if (do_week || do_month) {
	    Syslog('+', "Rollover mareas.data");

	    while (fread(&msgs, msgshdr.recsize, 1, fp) == 1) {
		if (do_week) {
		    RollWeek(&msgs.Received);
		    RollWeek(&msgs.Posted);
		}
		if (do_month) {
		    msgs.Received.month[Miy] = 0;
		    msgs.Posted.month[Miy] = 0;
		    if (CFG.slow_util && do_quiet)
			msleep(1);
		}
		fseek(fp, - msgshdr.recsize, SEEK_CUR);
		fwrite(&msgs, msgshdr.recsize, 1, fp);
		fseek(fp, msgshdr.syssize, SEEK_CUR);
	    }

	    msgshdr.lastupd = time(NULL);
	    fseek(fp, 0, SEEK_SET);
	    fwrite(&msgshdr, msgshdr.hdrsize, 1, fp);
	}
	fclose(fp);
    }

    if ((fp = OpenData((char *)"mgroups.data")) != NULL) {
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, fp);
	Temp = mgrouphdr.lastupd;
	t = localtime(&Temp);

	if (((Diw == 0) && (Day != t->tm_yday)) || ((Day - t->tm_yday) > 7))
	    do_week = TRUE;
	else
	    do_week = FALSE;
	if (Miy != t->tm_mon)
	    do_month = TRUE;
	else
	    do_month = FALSE;

	if (do_week || do_month) {
	    Syslog('+', "Rollover mgroups.data");

	    while (fread(&mgroup, mgrouphdr.recsize, 1, fp) == 1) {
		if (do_week) {
		    RollWeek(&mgroup.MsgsRcvd);
		    RollWeek(&mgroup.MsgsSent);
		}
		if (do_month) {
		    mgroup.MsgsRcvd.month[Miy] = 0;
		    mgroup.MsgsSent.month[Miy] = 0;
		    if (CFG.slow_util && do_quiet)
			msleep(1);
		}
		fseek(fp, - mgrouphdr.recsize, SEEK_CUR);
		fwrite(&mgroup, mgrouphdr.recsize, 1, fp);
	    }

	    mgrouphdr.lastupd = (int)time(NULL);
	    fseek(fp, 0, SEEK_SET);
	    fwrite(&mgrouphdr, mgrouphdr.hdrsize, 1, fp);
	}
	fclose(fp);
    }

    if ((fp = OpenData((char *)"tic.data")) != NULL) {
	fread(&tichdr, sizeof(tichdr), 1, fp);
	Temp = (time_t)tichdr.lastupd;
	t = localtime(&Temp);

	if (((Diw == 0) && (Day != t->tm_yday)) || ((Day - t->tm_yday) > 7))
	    do_week = TRUE;
	else
	    do_week = FALSE;
	if (Miy != t->tm_mon)
	    do_month = TRUE;
	else
	    do_month = FALSE;

	if (do_week || do_month) {
	    Syslog('+', "Rollover tic.data");

	    while (fread(&tic, tichdr.recsize, 1, fp) == 1) {
		if (do_week) {
		    RollWeek(&tic.Files);
		    RollWeek(&tic.KBytes);
		}
		if (do_month) {
		    tic.Files.month[Miy] = 0;
		    tic.KBytes.month[Miy] = 0;
		    if (CFG.slow_util && do_quiet)
			msleep(1);
		}
		fseek(fp, - tichdr.recsize, SEEK_CUR);
		fwrite(&tic, tichdr.recsize, 1, fp);
		fseek(fp, tichdr.syssize, SEEK_CUR);
	    }

	    tichdr.lastupd = (int)time(NULL);
	    fseek(fp, 0, SEEK_SET);
	    fwrite(&tichdr, tichdr.hdrsize, 1, fp);
	}
	fclose(fp);
    }

    if ((fp = OpenData((char *)"fgroups.data")) != NULL) {
	fread(&fgrouphdr, sizeof(fgrouphdr), 1, fp);
	Temp = (time_t)fgrouphdr.lastupd;
	t = localtime(&Temp);

	if (((Diw == 0) && (Day != t->tm_yday)) || ((Day - t->tm_yday) > 7))
	    do_week = TRUE;
	else
	    do_week = FALSE;
	if (Miy != t->tm_mon)
	    do_month = TRUE;
	else
	    do_month = FALSE;

	if (do_week || do_month) {
	    Syslog('+', "Rollover fgroups.data");

	    while (fread(&fgroup, fgrouphdr.recsize, 1, fp) == 1) {
		if (do_week) {
		    RollWeek(&fgroup.Files);
		    RollWeek(&fgroup.KBytes);
		}
		if (do_month) {
		    fgroup.Files.month[Miy] = 0;
		    fgroup.KBytes.month[Miy] = 0;
		    if (CFG.slow_util && do_quiet)
			msleep(1);
		}
		fseek(fp, - fgrouphdr.recsize, SEEK_CUR);
		fwrite(&fgroup, fgrouphdr.recsize, 1, fp);
	    }

	    fgrouphdr.lastupd = (int)time(NULL);
	    fseek(fp, 0, SEEK_SET);
	    fwrite(&fgrouphdr, fgrouphdr.hdrsize, 1, fp);
	}
	fclose(fp);
    }

    if ((fp = OpenData((char *)"hatch.data")) != NULL) {
	fread(&hatchhdr, sizeof(hatchhdr), 1, fp);
	Temp = (time_t)hatchhdr.lastupd;
	t = localtime(&Temp);

	if (((Diw == 0) && (Day != t->tm_yday)) || ((Day - t->tm_yday) > 7))
	    do_week = TRUE;
	else
	    do_week = FALSE;
	if (Miy != t->tm_mon)
	    do_month = TRUE;
	else
	    do_month = FALSE;

	if (do_week || do_month) {
	    Syslog('+', "Rollover hatch.data");

	    while (fread(&hatch, hatchhdr.recsize, 1, fp) == 1) {
		if (do_week)
		    RollWeek(&hatch.Hatched);
		if (do_month) {
		    hatch.Hatched.month[Miy] = 0;
		    if (CFG.slow_util && do_quiet)
			msleep(1);
		}
		fseek(fp, - hatchhdr.recsize, SEEK_CUR);
		fwrite(&hatch, hatchhdr.recsize, 1, fp);
	    }

	    hatchhdr.lastupd = (int)time(NULL);
	    fseek(fp, 0, SEEK_SET);
	    fwrite(&hatchhdr, hatchhdr.hdrsize, 1, fp);
	}
	fclose(fp);
    }

    temp  = calloc(PATH_MAX, sizeof(char));
    temp1 = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/var/mailer.hist", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r"))) {
	fread(&history, sizeof(history), 1, fp);
	Temp = (time_t)history.online;
	t = localtime(&Temp);
	if (t->tm_mon != Miy) {
	    /*
	     *  Calculate date/time for records to delete
	     */
	    t = localtime(&Now);
	    if (t->tm_mon == 0) {
		t->tm_mon = 11;
		t->tm_year--;
	    } else {
		t->tm_mon--;
	    }
	    t->tm_mday = 1;
	    t->tm_hour = 0;
	    t->tm_min = 0;
	    t->tm_sec = 0;
	    Now = mktime(t);
	    Syslog('+', "Packing mailer history since %s", rfcdate(Now));
	    snprintf(temp1, PATH_MAX, "%s/var/mailer.temp", getenv("MBSE_ROOT"));
	    if ((ft = fopen(temp1, "a")) == NULL) {
		WriteError("$Can't create %s", temp1);
		fclose(fp);
	    } else {
		memset(&history, 0, sizeof(history));
		history.online = (int)time(NULL);
		history.offline = (int)time(NULL);
		fwrite(&history, sizeof(history), 1, ft);

		i = 0;
		while (fread(&history, sizeof(history), 1, fp)) {
		    if (history.online >= (int)Now) {
			fwrite(&history, sizeof(history), 1, ft);
			i++;
		    }
		}
		fclose(ft);	
		fclose(fp);
		unlink(temp);
		rename(temp1, temp);
		Syslog('+', "Written %d records", i);
	    }
	} else {
	    fclose(fp);
	}
    }
    free(temp);
    free(temp1);
}



