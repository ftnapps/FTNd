/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Process .tic files
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
#include "../lib/dbftn.h"
#include "../lib/clcomm.h"
#include "unpack.h"
#include "ptic.h"
#include "fsort.h"
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
extern	int do_flush;


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
		do_flush = TRUE;

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
	char	*Temp, *Temp2, *Buf, *Log = NULL;
	int	i, j, rc, bufsize;
	fa_list	*sbl = NULL;
	int	DescCnt = FALSE;

	if (CFG.slow_util && do_quiet)
		usleep(1);

	memset(&TIC, 0, sizeof(TIC));
	memset(&T_File, 0, sizeof(T_File));

	sprintf(TIC.Inbound, "%s", inb);
	strncpy(TIC.TicName, tfn, 12);

	chdir(inb);
	if ((tfp = fopen(tfn, "r")) == NULL) {
		WriteError("$Cannot open %s", tfn);
		return 1;
	}

	/*
	 * Although a TIC line may only be 255 characters long,
	 * nobody seems to care and lines are up to 1024 characters
	 * long.
	 */
	if (PATH_MAX > 1024)
	    bufsize = PATH_MAX;
	else
	    bufsize = 1024;
	Temp = calloc(bufsize+1, sizeof(char));
	Buf  = calloc(bufsize+1, sizeof(char));

	while ((fgets(Buf, bufsize, tfp)) != NULL) {

		if (strlen(Buf) == bufsize)
		    Syslog('!', "Detected a TIC file line of %d characters long", bufsize);

		/*
		 * Remove all garbage from the .TIC file.
		 */
		Temp[0] = '\0';
		j = 0;
		for (i = 0; i < strlen(Buf); i++)
			if (isprint(Buf[i] & 0x7f)) {
				Temp[j] = Buf[i] & 0x7f;
				j++;
			}
		Temp[j] = '\0';

		if (strncasecmp(Temp, "hatch", 5) == 0) {
			TIC.TicIn.Hatch = TRUE;

		} else if (TIC.TicIn.Hatch && (strncasecmp(Temp, "pth ", 4) == 0)) {
			strncpy(TIC.TicIn.Pth, Temp+4, PATH_MAX);

		} else if (strncasecmp(Temp, "area ", 5) == 0) {
			strncpy(TIC.TicIn.Area, Temp+5, 20);
			strncpy(T_File.Echo, Temp+5, 20);

		} else if (strncasecmp(Temp, "origin ", 7) == 0) {
			strncpy(TIC.TicIn.Origin, Temp+7, 80);
			strncpy(T_File.Origin, Temp+7, 80);

		} else if (strncasecmp(Temp, "from ", 5) == 0) {
			strncpy(TIC.TicIn.From, Temp+5, 80);
			strncpy(T_File.From, Temp+5, 80);

		} else if (strncasecmp(Temp, "file ", 5) == 0) {
			strncpy(TIC.TicIn.File, Temp+5, 80);

		} else if (strncasecmp(Temp, "fullname ", 9) == 0) {
			strncpy(TIC.TicIn.FullName, Temp+9, 80);

		} else if (strncasecmp(Temp, "created ", 8) == 0) {
			strncpy(TIC.TicIn.Created, Temp+8, 80);

		} else if (strncasecmp(Temp, "magic ", 6) == 0) {
			strncpy(TIC.TicIn.Magic, Temp+6, 20);
			strncpy(T_File.Magic, Temp+6, 20);

		} else if (strncasecmp(Temp, "crc ", 4) == 0) {
			TIC.Crc_Int = strtoul(Temp+4, (char **)NULL, 16);
			sprintf(TIC.TicIn.Crc, "%08lX", TIC.Crc_Int);
			strcpy(T_File.Crc, TIC.TicIn.Crc);

		} else if (strncasecmp(Temp, "pw ", 3) == 0) {
			strncpy(TIC.TicIn.Pw, Temp+3, 20);

		} else if (strncasecmp(Temp, "replaces ", 9) == 0) {
			strncpy(TIC.TicIn.Replace, Temp+9, 80);
			strncpy(T_File.Replace, Temp+9, 80);

		} else if (strncasecmp(Temp, "desc ", 5) == 0) {
			if (!DescCnt) {
				strncpy(TIC.TicIn.Desc, Temp+5, 1023);
				strncpy(T_File.Desc, TIC.TicIn.Desc, 255);
				DescCnt = TRUE;
			} else {
				Syslog('!', "More than one \"Desc\" line");
			}
		} else if (strncasecmp(Temp, "path ", 5) == 0) {
			strncpy(TIC.TicIn.Path[TIC.TicIn.TotPath], Temp+5, 80);
			TIC.TicIn.TotPath++;
			TIC.Aka.zone = atoi(strtok(Temp+5, ":"));
			TIC.Aka.net  = atoi(strtok(NULL, "/"));
			TIC.Aka.node = atoi(strtok(NULL, "\0"));
			for (i = 0; i < 40; i++)
				if ((CFG.akavalid[i]) && (CFG.aka[i].zone  == TIC.Aka.zone) && (CFG.aka[i].net   == TIC.Aka.net) &&
				    (CFG.aka[i].node  == TIC.Aka.node) && (!CFG.aka[i].point)) {
					TIC.TicIn.PathError = TRUE;
					Syslog('+', "Aka %d: %s in path", i + 1, aka2str(CFG.aka[i]));
				}
		} else if (strncasecmp(Temp, "seenby ", 7) == 0) {
			fill_list(&sbl, Temp+7, NULL);

		} else if (strncasecmp(Temp, "areadesc ", 9) == 0) {
			strncpy(TIC.TicIn.AreaDesc, Temp+9, 60);

		} else if (strncasecmp(Temp, "to ", 3) == 0) {
			/*
			 * Drop this one
			 */
		} else if (strncasecmp(Temp, "size ", 5) == 0) {
			TIC.TicIn.Size = atoi(Temp+5);

		} else if (strncasecmp(Temp, "date ", 5) == 0) {
			/*
			 * Drop this one and log
			 */
			Syslog('f', "Date: %s", Temp+5);

		} else if (strncasecmp(Temp, "cost ", 5) == 0) {
			TIC.TicIn.Cost = atoi(Temp+5);

		} else if (strncasecmp(Temp, "ldesc ", 6) == 0) {
			if (TIC.TicIn.TotLDesc < 25) {
				strncpy(TIC.TicIn.LDesc[TIC.TicIn.TotLDesc], Temp+6, 80);
				TIC.TicIn.TotLDesc++;
			}
		} else if (strncasecmp(Temp, "destination ", 12) == 0) {
			/*
			 * Drop this one
			 */
		} else {
			/*
			 * If we didn't find a matching keyword it is a line we
			 * will just remember and forward if there are downlinks.
			 */
			if (TIC.TicIn.Unknowns < 25) {
				strcpy(TIC.TicIn.Unknown[TIC.TicIn.Unknowns], Temp);
				TIC.TicIn.Unknowns++;
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
			strncpy(T_File.LDesc[i], TIC.TicIn.LDesc[i], 48);
	}

	/*
	 * Show on screen what we are doing
	 */
	if (!do_quiet) {
		colour(3, 0);
		printf("\r");
		for (i = 0; i < 79; i++)
			printf(" ");
		printf("\rTic: %12s  File: %-14s Area: %-12s ", TIC.TicName, TIC.TicIn.File, TIC.TicIn.Area);
		fflush(stdout);
	}

	/*
	 * Show in logfile what we are doing
	 */
	Syslog('+', "Processing %s, %s area %s from %s", TIC.TicName, TIC.TicIn.File, TIC.TicIn.Area, TIC.TicIn.From);
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

	strcpy(Temp, TIC.TicIn.From);
	TIC.Aka.zone = atoi(strtok(Temp, ":"));
	TIC.Aka.net  = atoi(strtok(NULL, "/"));
	TIC.Aka.node = atoi(strtok(NULL, "@\0"));
	if (SearchFidonet(TIC.Aka.zone))
		strcpy(TIC.Aka.domain, fidonet.domain);
	strcpy(Temp, TIC.TicIn.Origin);
	TIC.OrgAka.zone = atoi(strtok(Temp, ":"));
	TIC.OrgAka.net  = atoi(strtok(NULL, "/"));
	TIC.OrgAka.node = atoi(strtok(NULL, "@\0"));
	if (SearchFidonet(TIC.OrgAka.zone))
		strcpy(TIC.OrgAka.domain, fidonet.domain);

	Temp2 = calloc(PATH_MAX, sizeof(char));

	if (TIC.TicIn.Hatch) {
	    /*
	     * Try to move the hatched file to the inbound
	     */
	    sprintf(Temp, "%s/%s", TIC.TicIn.Pth, TIC.TicIn.FullName);
	    if (file_exist(Temp, R_OK) == 0) {
		strcpy(TIC.RealName, TIC.TicIn.FullName);
	    } else {
		WriteError("Can't find %s", Temp);
		tidy_falist(&sbl);
		return 2;
	    }
	    sprintf(Temp2, "%s/%s", TIC.Inbound, TIC.TicIn.FullName);
	    if ((rc = file_mv(Temp, Temp2))) {
		WriteError("Can't move %s to inbound: %s", Temp, strerror(rc));
		tidy_falist(&sbl);
		return 1;
	    }
	    if (!strlen(TIC.TicIn.File)) {
		strcpy(Temp, TIC.TicIn.FullName);
		name_mangle(Temp);
		strncpy(TIC.TicIn.File, Temp, 12);
		Syslog('f', "Local hatch created 8.3 name %s", Temp);
	    }
	} else {
	    /*
	     * Find out what the real name of the file is
	     */
	    sprintf(Temp2, "%s", TIC.TicIn.File);
	    sprintf(Temp, "%s/%s", TIC.Inbound, Temp2);
	    if (file_exist(Temp, R_OK) == 0) {
		strcpy(TIC.RealName, Temp2);
	    } else {
		tu(Temp2);
		sprintf(Temp, "%s/%s", TIC.Inbound, Temp2);
		if (file_exist(Temp, R_OK) == 0) {
		    strcpy(TIC.RealName, Temp2);
		} else {
		    tl(Temp2);
		    sprintf(Temp, "%s/%s", TIC.Inbound, Temp2);
		    if (file_exist(Temp, R_OK) == 0) {
			strcpy(TIC.RealName, Temp2);
		    }
		}
	    }
	    if (strlen(TIC.TicIn.FullName) && (strlen(TIC.RealName) == 0)) {
		sprintf(Temp2, "%s", TIC.TicIn.FullName);
		sprintf(Temp, "%s/%s", TIC.Inbound, Temp2);
		if (file_exist(Temp, R_OK) == 0) {
		    strcpy(TIC.RealName, Temp2);
		} else {
		    tu(Temp2);
		    sprintf(Temp, "%s/%s", TIC.Inbound, Temp2);
		    if (file_exist(Temp, R_OK) == 0) {
			strcpy(TIC.RealName, Temp2);
		    } else {
			tl(Temp2);
			sprintf(Temp, "%s/%s", TIC.Inbound, Temp2);
			if (file_exist(Temp, R_OK) == 0) {
			    strcpy(TIC.RealName, Temp2);
			}
		    }
		}
	    }
	}

	if (strlen(TIC.RealName) == 0) {
	    /*
	     * We leave RealName empty, the ProcessTic function
	     * will handle this orphaned tic file.
	     */
	    WriteError("Can't find file in inbound");
	} else {
	    Syslog('f', "Real filename in inbound is \"%s\"", TIC.RealName);
	    strncpy(TIC.NewName, TIC.RealName, 80);
	}

	free(Temp2);
	free(Temp);
	free(Buf);

	tic_in++;
	rc = ProcessTic(sbl);
	tidy_falist(&sbl);

	return rc;
}



