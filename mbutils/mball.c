/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Creates allfiles listings
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
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "mball.h"


extern int	do_quiet;		/* Suppress screen output	*/
int		do_zip   = FALSE;	/* Create ZIP archives		*/
int		do_list  = FALSE;	/* Create filelist		*/
extern	int	e_pid;			/* Pid of child			*/
extern	int	show_log;		/* Show logging			*/
time_t		t_start;		/* Start time			*/
time_t		t_end;			/* End time			*/
struct		tm *l_date;		/* Structure for Date		*/


void ProgName()
{
	if (do_quiet)
		return;

	mbse_colour(15, 0);
	printf("\nMBALL: MBSE BBS %s Allfiles Listing Creator\n", VERSION);
	mbse_colour(14, 0);
	printf("       %s\n", COPYRIGHT);
}



void die(int onsig)
{
    /*
     * First check if a child is running, if so, kill it.
     */
    if (e_pid) {
	if ((kill(e_pid, SIGTERM)) == 0)
	    Syslog('+', "SIGTERM to pid %d succeeded", e_pid);
	else {
	    if ((kill(e_pid, SIGKILL)) == 0)
		Syslog('+', "SIGKILL to pid %d succeded", e_pid);
	    else
		WriteError("$Failed to kill pid %d", e_pid);
	}

	/*
	 * In case the child had the tty in raw mode...
	 */
	execute_pth((char *)"stty", (char *)"sane", (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
    }

    signal(onsig, SIG_IGN);

    if (onsig) {
	if (onsig <= NSIG)
	    WriteError("Terminated on signal %d (%s)", onsig, SigName[onsig]);
	else
	    WriteError("Terminated with error %d", onsig);
    }

    ulockprogram((char *)"mball");
    t_end = time(NULL);
    Syslog(' ', "MBALL finished in %s", t_elapsed(t_start, t_end));

    if (!do_quiet) {
	mbse_colour(7, 0);
	printf("\n");
    }
    ExitClient(onsig);
}



void Help()
{
	do_quiet = FALSE;
	ProgName();

	mbse_colour(11, 0);
	printf("\nUsage:	mball [command] <options>\n\n");
	mbse_colour(9, 0);
	printf("	Commands are:\n\n");
	mbse_colour(3, 0);
	printf("	l  list		Create allfiles and newfiles lists\n");
	mbse_colour(9, 0);
	printf("\n	Options are:\n\n");
	mbse_colour(3, 0);
	printf("	-q -quiet	Quiet mode\n");
	printf("	-z -zip		Create .zip archives\n");
	mbse_colour(7, 0);
	printf("\n");
	die(MBERR_COMMANDLINE);
}



int main(int argc, char **argv)
{
    int		    i;
    char	    *cmd;
    struct passwd   *pw;

    InitConfig();
    mbse_TermInit(1, 80, 24);
    t_start = time(NULL);
    umask(000);

    /*
     * Catch all signals we can, and ignore the rest.
     */
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGHUP) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM))
	    signal(i, (void (*))die);
	else if (i == SIGCHLD)
	    signal(i, SIG_DFL);
	else if ((i != SIGKILL) && (i != SIGSTOP))
	    signal(i, SIG_IGN);
    }

    if (argc < 2)
	Help();

    cmd = xstrcpy((char *)"Command line: mball");

    for (i = 1; i < argc; i++) {

	cmd = xstrcat(cmd, (char *)" ");
	cmd = xstrcat(cmd, argv[i]);

	if (!strncasecmp(argv[i], "l", 1))
	    do_list = TRUE;
	if (!strncasecmp(argv[i], "-q", 2))
	    do_quiet = TRUE;
	if (!strncasecmp(argv[i], "-z", 2))
	    do_zip = TRUE;
    }

    if (!do_list)
	Help();

    ProgName();
    pw = getpwuid(getuid());
    InitClient(pw->pw_name, (char *)"mball", CFG.location, CFG.logfile, 
	    CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);

    Syslog(' ', " ");
    Syslog(' ', "MBALL v%s", VERSION);
    Syslog(' ', cmd);
    free(cmd);

    if (!do_quiet) {
	mbse_colour(3, 0);
	printf("\n");
    }

    if (lockprogram((char *)"mball")) {
	if (!do_quiet)
	    printf("Can't lock mball, abort.\n");
	die(MBERR_NO_PROGLOCK);
    }

    if (do_list) {
	Masterlist();
	if (do_zip)
	    MakeArc();
	CreateSema((char *)"mailin");
    }

    if (!do_quiet)
	printf("Done!\n");

    die(MBERR_OK);
    return 0;
}



void MidLine(char *txt, FILE *fp, int doit)
{
	char	temp[81];
	int	x, y, z;

	if (!doit)
		return;

	z = strlen(txt);
	x = 77 - z;
	x /= 2;
	strcpy(temp, "");

	for (y = 0; y < x; y++)
		strcat(temp, " ");

	strcat(temp, txt);
	z = strlen(temp);
	x = 77 - z;

	for (y = 0; y < x; y++)
		strcat(temp, " ");

	fprintf(fp, "%c", 179);
	fprintf(fp, "%s", temp);
	fprintf(fp, "%c\r\n", 179);
}



void TopBox(FILE *fp, int doit)
{
	int	y;
	
	if (!doit)
		return;

	fprintf(fp, "\r\n%c", 213);
	for(y = 0; y < 77; y++)
		fprintf(fp, "%c", 205);
	fprintf(fp, "%c\r\n", 184);
}



void BotBox(FILE *fp, int doit)
{
	int	y;

	if (!doit)
		return;

	fprintf(fp, "%c", 212);
	for (y = 0; y < 77; y++)
		fprintf(fp, "%c", 205);
	fprintf(fp, "%c\r\n\r\n", 190);
}



void Masterlist()
{
    FILE	    *fp, *np, *pAreas, *pHeader;
    int		    AreaNr = 0, z, x = 0, New;
    unsigned long   AllFiles = 0, AllKBytes = 0, NewFiles = 0, NewKBytes = 0;
    unsigned long   AllAreaFiles, AllAreaBytes, popdown, down;
    unsigned long   NewAreaFiles, NewAreaBytes;
    char	    *sAreas;
    char	    temp[81], pop[81];
    struct _fdbarea *fdb_area = NULL;

    sAreas	= calloc(PATH_MAX, sizeof(char));

    IsDoing("Create Allfiles list");

    sprintf(sAreas, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

    if(( pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("Can't open File Areas File: %s", sAreas);
	mbse_colour(7,0);
	die(MBERR_GENERAL);
    }
    fread(&areahdr, sizeof(areahdr), 1, pAreas);

    if (!do_quiet)
	printf("Processing file areas\n");

    if ((fp = fopen("allfiles.tmp", "a+")) == NULL) {
 	WriteError("$Can't open allfiles.tmp");
	die(MBERR_GENERAL);
    }
    if ((np = fopen("newfiles.tmp", "a+")) == NULL) {
	WriteError("$Can't open newfiles.tmp");
	fclose(fp);
	die(MBERR_GENERAL);
    }

    TopBox(fp, TRUE);
    TopBox(np, TRUE);
    sprintf(temp, "All available files at %s", CFG.bbs_name);
    MidLine(temp, fp, TRUE);
    sprintf(temp, "New available files since %d days at %s", CFG.newdays, CFG.bbs_name);
    MidLine(temp, np, TRUE);
    BotBox(fp, TRUE);
    BotBox(np, TRUE);

    sprintf(temp, "%s/etc/header.txt", getenv("MBSE_ROOT"));
    if (( pHeader = fopen(temp, "r")) != NULL) {
	Syslog('+', "Inserting %s", temp);

	while( fgets(temp, 80 ,pHeader) != NULL) {
	    Striplf(temp);
	    fprintf(fp, "%s\r\n", temp);
	    fprintf(np, "%s\r\n", temp);
	}
	fclose(pHeader);
    }

    while (fread(&area, areahdr.recsize, 1, pAreas) == 1) {
	AreaNr++;
	AllAreaFiles = 0;
	AllAreaBytes = 0;
	NewAreaFiles = 0;
	NewAreaBytes = 0;

	if (area.Available && (area.LTSec.level <= CFG.security.level)) {

	    Nopper();

	    if ((fdb_area = mbsedb_OpenFDB(AreaNr, 30)) == 0) {
		WriteError("$Can't open Area %d (%s)! Skipping ...", AreaNr, area.Name);
	    } else {
		popdown = 0;
		while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
		    if (!fdb.Deleted) {
			/*
			 * The next is to reduce system load.
			 */
			x++;
			if (CFG.slow_util && do_quiet && ((x % 3) == 0))
			    msleep(1);
			AllFiles++;
			AllAreaFiles++;
			AllAreaBytes += fdb.Size;
			down = fdb.TimesDL;
			if (down > popdown) {
			    popdown = down;
			    sprintf(pop, "%s", fdb.Name);
			}
			if (((t_start - fdb.UploadDate) / 84400) <= CFG.newdays) {
			    NewFiles++;
			    NewAreaFiles++;
			    NewAreaBytes += fdb.Size;
			}
		    }
		}

		AllKBytes += AllAreaBytes / 1024;
		NewKBytes += NewAreaBytes / 1024;
				
		/*
		 * If there are files to report do it.
		 */
		if (AllAreaFiles) {
		    TopBox(fp, TRUE);
		    TopBox(np, NewAreaFiles);

		    sprintf(temp, "Area %d - %s", AreaNr, area.Name);
		    MidLine(temp, fp, TRUE);
		    MidLine(temp, np, NewAreaFiles);

		    sprintf(temp, "File Requests allowed");
		    MidLine(temp, fp, area.FileReq);
		    MidLine(temp, np, area.FileReq && NewAreaFiles);

		    sprintf(temp, "%ld KBytes in %ld files", AllAreaBytes / 1024, AllAreaFiles);
		    MidLine(temp, fp, TRUE);
		    sprintf(temp, "%ld KBytes in %ld files", NewAreaBytes / 1024, NewAreaFiles);
		    MidLine(temp, np, NewAreaFiles);
		    if (popdown) {
			sprintf(temp, "Most popular file is %s", pop);
			MidLine(temp, fp, TRUE);
		    }

		    BotBox(fp, TRUE);
		    BotBox(np, NewAreaFiles);

		    fseek(fdb_area->fp, fdbhdr.hdrsize, SEEK_SET);
		    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
			if (!fdb.Deleted) {
			    New = (((t_start - fdb.UploadDate) / 84400) <= CFG.newdays);
			    sprintf(temp, "%-12s%10lu K %s [%04ld] Uploader: %s",
				fdb.Name, (long)(fdb.Size / 1024), StrDateDMY(fdb.UploadDate), fdb.TimesDL, 
				strlen(fdb.Uploader)?fdb.Uploader:"");
			    fprintf(fp, "%s\r\n", temp);
			    if (New)
				fprintf(np, "%s\r\n", temp);
	
			    for (z = 0; z < 25; z++) {
				if (strlen(fdb.Desc[z])) {
				    if ((fdb.Desc[z][0] == '@') && (fdb.Desc[z][1] == 'X')) {
					fprintf(fp, "                         %s\r\n",fdb.Desc[z]+4);
					if (New)
					    fprintf(np, "                         %s\r\n",fdb.Desc[z]+4);
				    } else {
					fprintf(fp, "                         %s\r\n",fdb.Desc[z]);
					if (New)
					    fprintf(np, "                         %s\r\n",fdb.Desc[z]);
				    }
				}
			    }
			    if (strlen(fdb.Magic)) {
				fprintf(fp, "                         Magic filerequest: %s\r\n", fdb.Magic);
				if (New)
				    fprintf(np, "                         Magic filerequest: %s\r\n", fdb.Magic);
			    }
			}
		    }
		}
		mbsedb_CloseFDB(fdb_area);
    		}
	}
    } /* End of While Loop Checking for Areas Done */

    fclose(pAreas);

    TopBox(fp, TRUE);
    TopBox(np, TRUE);
    sprintf(temp, "Total %ld files, %ld KBytes", AllFiles, AllKBytes);
    MidLine(temp, fp, TRUE);
    sprintf(temp, "Total %ld files, %ld KBytes", NewFiles, NewKBytes);
    MidLine(temp, np, TRUE);

    MidLine((char *)"", fp, TRUE);
    MidLine((char *)"", np, TRUE);

    sprintf(temp, "Created by MBSE BBS v%s (%s-%s) at %s", VERSION, OsName(), OsCPU(), StrDateDMY(t_start));
    MidLine(temp, fp, TRUE);
    MidLine(temp, np, TRUE);

    BotBox(fp, TRUE);
    BotBox(np, TRUE);

    sprintf(temp, "%s/etc/footer.txt", getenv("MBSE_ROOT"));
    if(( pHeader = fopen(temp, "r")) != NULL) {
	Syslog('+', "Inserting %s", temp);

	while( fgets(temp, 80 ,pHeader) != NULL) {
	    Striplf(temp);
	    fprintf(fp, "%s\r\n", temp);
	    fprintf(np, "%s\r\n", temp);
	}
	fclose(pHeader);
    }

    fclose(fp);
    fclose(np);

    if ((rename("allfiles.tmp", "allfiles.txt")) == 0)
	unlink("allfiles.tmp");
    if ((rename("newfiles.tmp", "newfiles.txt")) == 0)
	unlink("newfiles.tmp");

    Syslog('+', "Allfiles: %ld, %ld MBytes", AllFiles, AllKBytes / 1024);
    Syslog('+', "Newfiles: %ld, %ld MBytes", NewFiles, NewKBytes / 1024);
    free(sAreas);
}



void MakeArc()
{
	char	*cmd;

	if (!getarchiver((char *)"ZIP")) {
		WriteError("ZIP Archiver not available");
		return;
	}

	cmd = xstrcpy(archiver.farc);

	if (cmd == NULL) {
		WriteError("ZIP archive command not available");
		return;
	}

	Nopper();
	if (!do_quiet)
		printf("Creating allfiles.zip\n");
	if (!execute_str(cmd, (char *)"allfiles.zip allfiles.txt", (char *)NULL, (char *)"/dev/null", 
			(char *)"/dev/null", (char *)"/dev/null") == 0)
		WriteError("Create allfiles.zip failed");

	Nopper();
	if (!do_quiet)
		printf("Creating newfiles.zip\n");
	if (!execute_str(cmd, (char *)"newfiles.zip newfiles.txt", (char *)NULL, (char *)"/dev/null", 
			(char *)"/dev/null", (char  *)"/dev/null") == 0)
		WriteError("Create newfiles.zip failed");
	free(cmd);
	cmd = NULL;
}

