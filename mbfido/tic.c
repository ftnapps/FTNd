/*****************************************************************************
 *
 * File ..................: mbfido/tic.c
 * Purpose ...............: Process .tic files
 * Last modification date : 15-Oct-2001
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
#include "../lib/dbftn.h"
#include "../lib/clcomm.h"
#include "ulock.h"
#include "ptic.h"
#include "fsort.h"
#include "pack.h"
#include "tic.h"

#define	UNPACK_FACTOR 300

int	tic_in  = 0;				/* .tic files received	     */
int	tic_imp = 0;				/* imported .tic files	     */
int	tic_out = 0;				/* .tic files sent	     */
int	tic_bad = 0;				/* bad .tic files	     */
int	tic_dup = 0;				/* dupe .tic files	     */


extern	int do_unprot;
extern	int do_quiet;
extern	int tic_in;


/*
 *  returns:	-1 = Errors.
 *		0  = No files processed
 *		1  = Processed file(s)
 */
int Tic()
{
	char	*inbound, *fname;
	DIR	*dp;
	struct	dirent *de;
	struct	stat sbuf;
	int	i, rc = 0;
	fd_list	*fdl = NULL;

	IsDoing("Process .tic files");
	CompileNL = FALSE;

	if (do_unprot) {
		inbound = xstrcpy(CFG.inbound);
	} else {
		inbound = xstrcpy(CFG.pinbound);
	}
	Syslog('+', "Pass: process ticfiles (%s)", inbound);

	if (!diskfree(CFG.freespace)) {
		free(inbound);
		return -1;
	}

	if (chdir(inbound) == -1) {
		WriteError("$Can't chdir(%s)", inbound);
		free(inbound);
		return -1;
	}

	if ((dp = opendir(inbound)) == NULL) {
		WriteError("$Can't opendir(%s)", inbound);
		free(inbound);
		return -1;
	}

	while ((de = readdir(dp)))
		if ((strlen(de->d_name) == 12) &&
		    (strncasecmp(de->d_name+11, "c", 1) == 0)) {
			if ((strncasecmp(de->d_name+8, ".a", 2) == 0) ||
			    (strncasecmp(de->d_name+8, ".c", 2) == 0) ||
			    (strncasecmp(de->d_name+8, ".z", 2) == 0) ||
			    (strncasecmp(de->d_name+8, ".l", 2) == 0) ||
			    (strncasecmp(de->d_name+8, ".r", 2) == 0) ||
			    (strncasecmp(de->d_name+8, ".0", 2) == 0)) {
				if (checkspace(inbound, de->d_name, UNPACK_FACTOR)) {
					if ((unpack(de->d_name)) != 0) {
						WriteError("Error unpacking %s", de->d_name);
					}
				} else
					Syslog('+', "Insufficient space to unpack file %s", de->d_name);
			}
		}

	rewinddir(dp);
	while ((de = readdir(dp)))
		if ((strlen(de->d_name) == 12) && (strncasecmp(de->d_name+8, ".tic", 4) == 0)) {
			stat(de->d_name, &sbuf);
			fill_fdlist(&fdl, de->d_name, sbuf.st_mtime);
		}

	closedir(dp);
	sort_fdlist(&fdl);

	while ((fname = pull_fdlist(&fdl)) != NULL) {
		if (LoadTic(inbound, fname) == 0)
			rc = 1;
		if (IsSema((char *)"upsalarm")) {
			rc = 0;
			Syslog('+', "Detected upsalarm semafore, aborting tic processing");
			break;
		}
		if (!diskfree(CFG.freespace)) {
			rc = 0;
			break;
		}
	}

	if (!do_quiet) {
		printf("\r");
		for (i = 0; i < 79; i++)
			printf(" ");
		printf("\r");
		fflush(stdout);
	}

	if (rc)
		packmail();

	if (CompileNL) 
		CreateSema((char *)"mbindex");

	free(inbound);
	return rc;
}



/*
 * Returns 1 if error, 0 if ok.
 */
int LoadTic(char *inb, char *tfn)
{
	FILE	*tfp;
	char	*Temp, *Buf, *Log = NULL, *Realname;
	int	i, j, rc;
	fa_list	*sbl = NULL;
	int	Kwd, DescCnt = FALSE;

	if (CFG.slow_util && do_quiet)
		usleep(1);

	memset(&TIC, 0, sizeof(TIC));
	memset(&T_File, 0, sizeof(T_File));

	sprintf(TIC.Inbound, "%s/", inb);
	sprintf(TIC.FilePath, "%s/", inb);
	strcpy(TIC.TicName, tfn);

	chdir(inb);
	if ((tfp = fopen(tfn, "r")) == NULL) {
		WriteError("$Cannot open %s", tfn);
		return 1;
	}

	Temp = calloc(256, sizeof(char));
	Buf  = calloc(256, sizeof(char));
	Realname = calloc(PATH_MAX, sizeof(char));

	while ((fgets(Buf, 256, tfp)) != NULL) {

		/*
		 * Remove all garbage from the .TIC file.
		 */
		Temp[0] = '\0';
		j = 0;
		for (i = 0; i < strlen(Buf); i++)
			if ((Buf[i] >= ' ') || (Buf[i] < 0)) {
				Temp[j] = Buf[i];
				j++;
			}
		Temp[j] = '\0';

		Kwd = FALSE;

		if (strncasecmp(Temp, "hatch", 5) == 0)
			Kwd = TIC.Hatch = TRUE;

		if (TIC.Hatch) {
			if (strncasecmp(Temp, "pth ", 4) == 0) {
				sprintf(TIC.FilePath, "%s/", Temp+4);
				Kwd = TRUE;
			}

			if (strncasecmp(Temp, "nomove", 6) == 0)
				Kwd = TIC.NoMove = TRUE;

			if (strncasecmp(Temp, "hatchnew", 8) == 0)
				Kwd = TIC.HatchNew = TRUE;
		}

		if (strncasecmp(Temp, "area ", 5) == 0) {
			strcpy(TIC.TicIn.Area, Temp+5);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "origin ", 7) == 0) {
			strcpy(TIC.TicIn.Origin, Temp+7);
			strcpy(T_File.Origin, Temp+7);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "from ", 5) == 0) {
			strcpy(TIC.TicIn.From, Temp+5);
			strcpy(T_File.From, Temp+5);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "file ", 5) == 0) {
			sprintf(Realname, "%s", Temp+5);  /* Moved here for test Michiel/Redy */
			if (TIC.Hatch)
				strcpy(TIC.TicIn.OrgName, Temp+5);
			else
				strcpy(TIC.TicIn.OrgName, tl(Temp+5));
//			sprintf(Realname, "%s", Temp+5); /* Temp removed */
			strcpy(TIC.NewName, TIC.TicIn.OrgName);
			strcpy(T_File.Name, TIC.TicIn.OrgName);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "fullname", 8) == 0) {
			strcpy(TIC.TicIn.LName, Temp+8);
			Syslog('f', "Long filename: %s", TIC.TicIn.LName);
		}
 
		if (strncasecmp(Temp, "created ", 8) == 0) {
			strcpy(TIC.TicIn.Created, Temp+8);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "magic ", 6) == 0) {
			strcpy(TIC.TicIn.Magic, Temp+6);
			strcpy(T_File.Magic, Temp+6);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "crc ", 4) == 0) {
			TIC.Crc_Int = strtoul(Temp+4, (char **)NULL, 16);
			sprintf(TIC.TicIn.Crc, "%08lX", TIC.Crc_Int);
			strcpy(T_File.Crc, TIC.TicIn.Crc);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "pw ", 3) == 0) {
			strcpy(TIC.TicIn.Pw, Temp+3);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "replaces ", 9) == 0) {
			strcpy(TIC.TicIn.Replace, Temp+9);
			strcpy(T_File.Replace, Temp+9);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "desc ", 5) == 0) {
			if (!DescCnt) {
				strcpy(TIC.TicIn.Desc, Temp+5);
				strcpy(T_File.Desc, TIC.TicIn.Desc);
				Kwd = TRUE;
				DescCnt = TRUE;
			} else {
				Syslog('!', "More than one \"Desc\" line");
			}
		}

		if (strncasecmp(Temp, "path ", 5) == 0) {
			strcpy(TIC.TicIn.Path[TIC.TicIn.TotPath], Temp+5);
			TIC.TicIn.TotPath++;
			TIC.Aka.zone = atoi(strtok(Temp+5, ":"));
			TIC.Aka.net  = atoi(strtok(NULL, "/"));
			TIC.Aka.node = atoi(strtok(NULL, "\0"));
			for (i = 0; i < 40; i++)
				if ((CFG.akavalid[i]) &&
				    (CFG.aka[i].zone  == TIC.Aka.zone) &&
				    (CFG.aka[i].net   == TIC.Aka.net) &&
				    (CFG.aka[i].node  == TIC.Aka.node) &&
				    (!CFG.aka[i].point))
					TIC.PathErr = TRUE;
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "seenby ", 7) == 0) {
			fill_list(&sbl, Temp+7, NULL);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "areadesc ", 9) == 0) {
			strcpy(TIC.TicIn.AreaDesc, Temp+9);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "to ", 3) == 0) {
			/*
			 * Drop this one
			 */
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "size ", 5) == 0) {
			TIC.TicIn.Size = atoi(Temp+5);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "date ", 5) == 0) {
			Syslog('f', "Date: %s", Temp+5);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "cost ", 5) == 0) {
			TIC.TicIn.UplinkCost = atoi(Temp+5);
			Kwd = TRUE;
		}

		if (strncasecmp(Temp, "ldesc ", 6) == 0) {
			if (TIC.TicIn.TotLDesc < 25) {
				Temp[86] = '\0';
				strcpy(TIC.TicIn.LDesc[TIC.TicIn.TotLDesc], Temp+6);
				TIC.TicIn.TotLDesc++;
			}
			Kwd = TRUE;
		}

		/*
		 * If we didn't find a matching keyword it is a line we
		 * will just remember and forward if there are downlinks.
		 */
		if (!Kwd) {
			/*
			 * Consider Destination keyword not as a passthru
			 * line and drop it.
			 */
			if (strncasecmp(Temp, "destination ", 12) != 0) {
				if (TIC.TicIn.Unknowns < 25) {
					strcpy(TIC.TicIn.Unknown[TIC.TicIn.Unknowns], Temp);
					TIC.TicIn.Unknowns++;
				}
			}
		}
	}

	fclose(tfp);

	if (TIC.TicIn.TotLDesc) {
		/*
		 * First check for a bug in Harald Harms Allfix program that
		 * lets Allfix forward dummy Ldesc lines with the contents:
		 * "Long description not available"
		 */
		if (strstr(TIC.TicIn.LDesc[0], "ion not avail") != NULL) {
			Syslog('!', "Killing invalid Ldesc line(s)");
			TIC.TicIn.TotLDesc = 0;
		}
	}
	if (TIC.TicIn.TotLDesc) {
		T_File.TotLdesc = TIC.TicIn.TotLDesc;
		for (i = 0; i <= TIC.TicIn.TotLDesc; i++)
			strcpy(T_File.LDesc[i], TIC.TicIn.LDesc[i]);
	}

	/*
	 * Show on screen what we are doing
	 */
	if (!do_quiet) {
		colour(3, 0);
		printf("\r");
		for (i = 0; i < 79; i++)
			printf(" ");
		printf("\rTic: %12s  File: %-14s Area: %-12s ", TIC.TicName, TIC.TicIn.OrgName, TIC.TicIn.Area);
		fflush(stdout);
	}

	/*
	 * Show in logfile what we are doing
	 */
	Syslog('+', "Processing %s, %s area %s from %s", TIC.TicName, TIC.TicIn.OrgName, TIC.TicIn.Area, TIC.TicIn.From);
	Syslog('+', "+- %s", TIC.TicIn.Created);
	Log = NULL;

	if (strlen(TIC.TicIn.Replace)) {
		Log = xstrcpy((char *)"Replace ");
		Log = xstrcat(Log, TIC.TicIn.Replace);
	}
	if (strlen(TIC.TicIn.Magic)) {
		if (Log != NULL)
			Log = xstrcat(Log, (char *)", Magic ");
		else
			Log = xstrcpy((char *)"Magic ");
		Log = xstrcat(Log, TIC.TicIn.Magic);
	}
	if (Log != NULL) {
		Syslog('+', "%s", Log);
		free(Log);
		Log = NULL;
	}

	sprintf(Temp, "%s", TIC.TicIn.From);
	TIC.Aka.zone = atoi(strtok(Temp, ":"));
	TIC.Aka.net  = atoi(strtok(NULL, "/"));
	TIC.Aka.node = atoi(strtok(NULL, "@\0"));
	if (SearchFidonet(TIC.Aka.zone))
		strcpy(TIC.Aka.domain, fidonet.domain);
	sprintf(Temp, "%s", TIC.TicIn.Origin);
	TIC.OrgAka.zone = atoi(strtok(Temp, ":"));
	TIC.OrgAka.net  = atoi(strtok(NULL, "/"));
	TIC.OrgAka.node = atoi(strtok(NULL, "@\0"));
	if (SearchFidonet(TIC.OrgAka.zone))
		strcpy(TIC.OrgAka.domain, fidonet.domain);
	free(Temp);
	free(Buf);

	tic_in++;
	rc = ProcessTic(sbl, Realname);
	tidy_falist(&sbl);
	free(Realname);
	
	return rc;
}



