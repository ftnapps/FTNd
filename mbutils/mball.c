/*****************************************************************************
 *
 * $Id: mball.c,v 1.9 2007/09/02 11:17:33 mbse Exp $
 * Purpose ...............: Creates allfiles listings
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
#include "../lib/mbse.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "dlcount.h"
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

    mbse_colour(WHITE, BLACK);
    printf("\nMBALL: MBSE BBS %s Allfiles Listing Creator\n", VERSION);
    mbse_colour(YELLOW, BLACK);
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
	mbse_colour(LIGHTGRAY, BLACK);
	printf("\n");
    }
    ExitClient(onsig);
}



void Help()
{
    do_quiet = FALSE;
    ProgName();

    mbse_colour(LIGHTCYAN, BLACK);
    printf("\nUsage:	mball [command] <options>\n\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("	Commands are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	l  list		Create allfiles and newfiles lists\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("\n	Options are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	-q -quiet	Quiet mode\n");
    printf("	-z -zip		Create .zip archives\n");
    mbse_colour(LIGHTGRAY, BLACK);
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
	if ((i == SIGHUP) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM) || (i == SIGIOT))
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
	mbse_colour(CYAN, BLACK);
	printf("\n");
    }

    if (lockprogram((char *)"mball")) {
	if (!do_quiet)
	    printf("Can't lock mball, abort.\n");
	die(MBERR_NO_PROGLOCK);
    }

    if (do_list) {
	dlcount();
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



void MidLine(char *txt, FILE *fp, FILE *up, int doit)
{
    char    temp[256];
    int	    x, y, z;

    if (!doit)
	return;

    z = strlen(txt);
    x = 77 - z;
    x /= 2;
    strcpy(temp, "\xB3");

    for (y = 0; y < x; y++)
	strcat(temp, " ");

    strncat(temp, txt, 255);

    for (y = strlen(temp); y < 78; y++)
	strncat(temp, " ", 255);

    strncat(temp, "\xB3\r\n", 255);
    fprintf(fp, temp);
    fprintf(up, chartran(temp));
}



void TopBox(FILE *fp, FILE *up, int doit)
{
    int		y;
    char	temp[256];
	
    if (!doit)
	return;

    strcpy(temp, "\r\n\xDA");
    for(y = 0; y < 77; y++)
	strncat(temp, "\xC4", 255);
    strncat(temp, "\xBF\r\n", 255);
    fprintf(fp, temp);
    fprintf(up, chartran(temp));
}



void BotBox(FILE *fp, FILE *up, int doit)
{
    int		y;
    char	temp[256];

    if (!doit)
	return;

    strcpy(temp, "\xC0");
    for (y = 0; y < 77; y++)
	strncat(temp, "\xC4", 255);
    strncat(temp, "\xD9\r\n\r\n", 255);
    fprintf(fp, temp);
    fprintf(up, chartran(temp));
}



void WriteFiles(FILE *fp, FILE *fu, FILE *np, FILE *nu, int New, char *temp)
{
    fprintf(fp, "%s\r\n", temp);
    fprintf(fu, "%s\r\n", chartran(temp));
    if (New) {
	fprintf(np, "%s\r\n", temp);
	fprintf(nu, "%s\r\n", chartran(temp));
    }
}



void Masterlist()
{
    FILE	    *fp, *np, *fu, *nu, *pAreas, *pHeader;
    int		    AreaNr = 0, z, x = 0, New;
    unsigned int    AllFiles = 0, AllKBytes = 0, NewFiles = 0, NewKBytes = 0;
    unsigned int    AllAreaFiles, AllAreaBytes, popdown, down, NewAreaFiles, NewAreaBytes;
    char	    *sAreas, temp[PATH_MAX], pop[81];
    struct _fdbarea *fdb_area = NULL;

    sAreas	= calloc(PATH_MAX, sizeof(char));

    IsDoing("Create Allfiles list");

    snprintf(sAreas, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));

    if(( pAreas = fopen (sAreas, "r")) == NULL) {
	WriteError("Can't open File Areas File: %s", sAreas);
	mbse_colour(LIGHTGRAY, BLACK);
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
    if ((fu = fopen("allfiles.ump", "a+")) == NULL) {
	WriteError("$Can't open allfiles.ump");
	fclose(fp);
	fclose(np);
	die(MBERR_GENERAL);
    }
    if ((nu = fopen("newfiles.ump", "a+")) == NULL) {
	WriteError("$Can't open newfiles.ump");
	fclose(fp);
	fclose(np);
	fclose(fu);
	die(MBERR_GENERAL);
    }

    chartran_init((char *)"CP437", (char *)"UTF-8", 'B');

    TopBox(fp, fu, TRUE);
    TopBox(np, nu, TRUE);
    snprintf(temp, 81, "All available files at %s", CFG.bbs_name);
    MidLine(temp, fp, fu, TRUE);
    snprintf(temp, 81, "New available files since %d days at %s", CFG.newdays, CFG.bbs_name);
    MidLine(temp, np, nu, TRUE);
    BotBox(fp, fu, TRUE);
    BotBox(np, nu, TRUE);

    snprintf(temp, PATH_MAX, "%s/etc/header.txt", getenv("MBSE_ROOT"));
    if (( pHeader = fopen(temp, "r")) != NULL) {
	Syslog('+', "Inserting %s", temp);

	while( fgets(temp, 80 ,pHeader) != NULL) {
	    Striplf(temp);
	    fprintf(fp, "%s\r\n", temp);
	    fprintf(np, "%s\r\n", temp);
	    fprintf(fu, "%s\r\n", chartran(temp));
	    fprintf(nu, "%s\r\n", chartran(temp));
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
		WriteError("Can't open Area %d (%s)! Skipping ...", AreaNr, area.Name);
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
			    snprintf(pop, 81, "%s", fdb.Name);
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
		    TopBox(fp, fu, TRUE);
		    TopBox(np, nu, NewAreaFiles);

		    snprintf(temp, 81, "Area %d - %s", AreaNr, area.Name);
		    MidLine(temp, fp, fu, TRUE);
		    MidLine(temp, np, nu, NewAreaFiles);

		    snprintf(temp, 81, "File Requests allowed");
		    MidLine(temp, fp, fu, area.FileReq);
		    MidLine(temp, np, nu, area.FileReq && NewAreaFiles);

		    snprintf(temp, 81, "%d KBytes in %d files", AllAreaBytes / 1024, AllAreaFiles);
		    MidLine(temp, fp, fu, TRUE);
		    snprintf(temp, 81, "%d KBytes in %d files", NewAreaBytes / 1024, NewAreaFiles);
		    MidLine(temp, np, nu, NewAreaFiles);
		    if (popdown) {
			snprintf(temp, 81, "Most popular file is %s", pop);
			MidLine(temp, fp, fu, TRUE);
		    }

		    BotBox(fp, fu, TRUE);
		    BotBox(np, nu, NewAreaFiles);

		    fseek(fdb_area->fp, fdbhdr.hdrsize, SEEK_SET);
		    while (fread(&fdb, fdbhdr.recsize, 1, fdb_area->fp) == 1) {
			if (!fdb.Deleted) {
			    New = (((t_start - fdb.UploadDate) / 84400) <= CFG.newdays);

			    snprintf(temp, 81, "%s", fdb.LName);
			    WriteFiles(fp, fu, np, nu, New, temp);

			    snprintf(temp, 81, "%-12s%10u K %s [%04d] Uploader: %s", 
				fdb.Name, (int)(fdb.Size / 1024), StrDateDMY(fdb.UploadDate), fdb.TimesDL, 	 
				strlen(fdb.Uploader)?fdb.Uploader:"");
			    WriteFiles(fp, fu, np, nu, New, temp);

			    for (z = 0; z < 25; z++) {
				if (strlen(fdb.Desc[z])) {
				    if ((fdb.Desc[z][0] == '@') && (fdb.Desc[z][1] == 'X')) {
					snprintf(temp, 81, "                         %s", fdb.Desc[z]+4);
				    } else {
					snprintf(temp, 81, "                         %s", fdb.Desc[z]);
				    }
				    WriteFiles(fp, fu, np, nu, New, temp);
				}
			    }

			    if (strlen(fdb.Magic)) {
				snprintf(temp, 81, "                         Magic filerequest: %s", fdb.Magic);
				WriteFiles(fp, fu, np, nu, New, temp);
			    }
			    WriteFiles(fp, fu, np, nu, New, (char *)"");
			}
		    }
		}
		mbsedb_CloseFDB(fdb_area);
    		}
	}
    } /* End of While Loop Checking for Areas Done */

    fclose(pAreas);

    TopBox(fp, fu, TRUE);
    TopBox(np, nu, TRUE);
    snprintf(temp, 81, "Total %d files, %d KBytes", AllFiles, AllKBytes);
    MidLine(temp, fp, fu, TRUE);
    snprintf(temp, 81, "Total %d files, %d KBytes", NewFiles, NewKBytes);
    MidLine(temp, np, nu, TRUE);

    MidLine((char *)"", fp, fu, TRUE);
    MidLine((char *)"", np, nu, TRUE);

    snprintf(temp, 81, "Created by MBSE BBS v%s (%s-%s) at %s", VERSION, OsName(), OsCPU(), StrDateDMY(t_start));
    MidLine(temp, fp, fu, TRUE);
    MidLine(temp, np, nu, TRUE);

    BotBox(fp, fu, TRUE);
    BotBox(np, nu, TRUE);

    snprintf(temp, PATH_MAX, "%s/etc/footer.txt", getenv("MBSE_ROOT"));
    if(( pHeader = fopen(temp, "r")) != NULL) {
	Syslog('+', "Inserting %s", temp);

	while( fgets(temp, 80 ,pHeader) != NULL) {
	    Striplf(temp);
	    fprintf(fp, "%s\r\n", temp);
	    fprintf(np, "%s\r\n", temp);
	    fprintf(fu, "%s\r\n", chartran(temp));
	    fprintf(nu, "%s\r\n", chartran(temp));
	}
	fclose(pHeader);
    }

    fclose(fp);
    fclose(np);
    fclose(fu);
    fclose(nu);
    chartran_close();

    if ((rename("allfiles.tmp", "allfiles.txt")) == 0)
	unlink("allfiles.tmp");
    if ((rename("newfiles.tmp", "newfiles.txt")) == 0)
	unlink("newfiles.tmp");
    if ((rename("allfiles.ump", "allfiles.utf")) == 0)
	unlink("allfiles.ump");
    if ((rename("newfiles.ump", "newfiles.utf")) == 0)
	unlink("newfiles.ump");

    Syslog('+', "Allfiles: %ld, %ld MBytes", AllFiles, AllKBytes / 1024);
    Syslog('+', "Newfiles: %ld, %ld MBytes", NewFiles, NewKBytes / 1024);
    free(sAreas);
}



void MakeArc()
{
    char    *cmd;

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
    if (!execute_str(cmd, (char *)"allfiles.zip allfiles.txt allfiles.utf", (char *)NULL, (char *)"/dev/null", 
			(char *)"/dev/null", (char *)"/dev/null") == 0)
	WriteError("Create allfiles.zip failed");

    Nopper();
    if (!do_quiet)
	printf("Creating newfiles.zip\n");
    if (!execute_str(cmd, (char *)"newfiles.zip newfiles.txt newfiles.utf", (char *)NULL, (char *)"/dev/null", 
			(char *)"/dev/null", (char  *)"/dev/null") == 0)
	WriteError("Create newfiles.zip failed");

    free(cmd);
    cmd = NULL;
}


