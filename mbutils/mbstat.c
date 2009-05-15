/*****************************************************************************
 *
 * $Id: mbstat.c,v 1.5 2007/09/02 11:17:33 mbse Exp $
 * Purpose ...............: Change BBS status
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
 *   
 * Michiel Broek		FIDO:	2:280/2802
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
#include "mbstat.h"


extern	int	do_quiet;
time_t		t_start, t_end;


void Help(void)
{
    do_quiet = FALSE;
    ProgName();

    mbse_colour(LIGHTCYAN, BLACK);
    printf("\nUsage:	mbstat [command] <options>\n\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("	Commands are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	c  close	Close the BBS for users\n");
    printf("	o  open		Open the BBS for users\n");
    printf("	s  set semafore Set named semafore\n");
    printf("	w  wait		Wait until the BBS is free\n\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("	Semafore's are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("			mailout mailin mbindex newnews msglink\n");
    printf("			reqindex upsalarm upsdown do_inet\n\n");
    mbse_colour(LIGHTBLUE, BLACK);
    printf("	Options are:\n\n");
    mbse_colour(CYAN, BLACK);
    printf("	-q -quiet	Quiet, no screen output\n");
    mbse_colour(LIGHTGRAY, BLACK);
    die(MBERR_COMMANDLINE);
}



void ProgName(void)
{
    if (do_quiet)
	return;

    mbse_colour(WHITE, BLACK);
    printf("\nMBSTAT: MBSE BBS %s Status Changer\n", VERSION);
    mbse_colour(YELLOW, BLACK);
    printf("        %s\n", COPYRIGHT);
    mbse_colour(CYAN, BLACK);
}



void die(int onsig)
{
    signal(onsig, SIG_IGN);

    if (onsig)
	Syslog('+', "Terminated on signal %d", onsig);

    if (!do_quiet) {
	mbse_colour(LIGHTGRAY, BLACK);
	printf("\n");
    }

    t_end = time(NULL);
    Syslog(' ', "MBSTAT finished in %s", t_elapsed(t_start, t_end));

    ExitClient(onsig);
}



int main(int argc, char **argv)
{
    int		    i;
    char	    *cmd, *semafore = NULL;
    int		    do_open  = FALSE;
    int		    do_close = FALSE;
    int		    do_wait  = FALSE;
    int		    do_sema  = FALSE;
    struct passwd   *pw;

    InitConfig();
    mbse_TermInit(1, 80, 24);
    t_start = time(NULL);

    /*
     * Catch or ignore signals
     */
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGHUP) || (i == SIGINT) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM) || (i == SIGIOT))
	    signal(i, (void (*))die);
	else if ((i != SIGKILL) && (i != SIGSTOP))
	    signal(i, SIG_IGN);
    }

    cmd = xstrcpy((char *)"Command line: mbstat");

    for (i = 1; i < argc; i++) {
	cmd = xstrcat(cmd, (char *)" ");
	cmd = xstrcat(cmd, tl(argv[i]));

	if (!strncmp(tl(argv[i]), "w", 1))
	    do_wait = TRUE;
	if (!strncmp(tl(argv[i]), "o", 1))
	    do_open = TRUE;
	if (!strncmp(tl(argv[i]), "c", 1))
	    do_close = TRUE;
	if (!strncmp(tl(argv[i]), "s", 1)) {
	    do_sema = TRUE;
	    i++;
	    semafore = xstrcpy(tl(argv[i]));
	    cmd = xstrcat(cmd, (char *)" ");
	    cmd = xstrcat(cmd, tl(argv[i]));
	}
	if (!strncmp(tl(argv[i]), "-q", 2))
	    do_quiet = TRUE;
    }

    ProgName();
    pw = getpwuid(getuid());
    InitClient(pw->pw_name, (char *)"mbstat", CFG.location, CFG.logfile, 
		CFG.util_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);

    Syslog(' ', " ");
    Syslog(' ', "MBSTAT v%s", VERSION);
    Syslog(' ', cmd);
    free(cmd);

    if (!do_quiet) {
	mbse_colour(CYAN, BLACK);
	printf("\n");
    }

    if (do_close)
	do_open = FALSE;

    if (do_open) {
	Open();
	do_close = FALSE;
	do_wait  = FALSE;
    }

    if (do_close)
	Close();

    if (do_wait)
	Wait();

    if (do_sema)
	Semafore(semafore);

    if (!(do_open || do_close || do_wait || do_sema))
	Help();

    die(MBERR_OK);
    return 0;
}



int Semafore(char *semafore)
{
    char    buf[81];

    strcpy(buf, SockR("SECR:1,%s;", semafore));
    if (strncmp(buf, "100:0;", 6) == 0) {
        Syslog('+', "Semafore \"%s\" is set", semafore);
        if (!do_quiet)
            printf("Semafore \"%s\" is set\n", semafore);
        return TRUE;
    } else {
	Syslog('+', "Failed to set \"%s\" semafore", semafore);
        printf("Failed to set \"%s\" semafore\n", semafore);
	return FALSE;
    }
}



int Close(void)
{
    char	buf[81];

    strcpy(buf, SockR("SCLO:0;"));
    if (strncmp(buf, "100:0;", 6) == 0) {
	Syslog('+', "The BBS is closed");
	if (!do_quiet)
	    printf("The BBS is closed\n");
	return TRUE;
    } else {
	printf("Failed to close the BBS\n");
	return FALSE;
    }
}



int Open(void)
{
    char	buf[81];

    strcpy(buf, SockR("SOPE:0;"));
    if (strncmp(buf, "100:0;", 6) == 0) {
	Syslog('+', "The BBS is open");
	if (!do_quiet)
	    printf("The BBS is open\n");
	return TRUE;
    } else {
	printf("Failed to open the BBS\n");
	return FALSE;
    }
}



int Wait(void)
{
    int	    Waiting = 3600;
    char    buf[PATH_MAX];

    if (IsSema((char *)"upsdown"))
	Waiting = 30;

    Syslog('+', "Waiting for the BBS to become free, timeout %d seconds", Waiting);
    while (Waiting) {
	strcpy(buf, SockR("SFRE:0;"));
	if (strncmp(buf, "100:0;", 6) == 0) {
	    Syslog('+', "The BBS is free");
	    if (!do_quiet)
		printf("The BBS is free.                             \n");
	    return TRUE;
	}
	if (!do_quiet) {
	    buf[strlen(buf) -1] = '\0';
	    printf("\r%s\r", buf+6);
	    fflush(stdout);
	}
	sleep(1);
	Waiting--;
    }

    WriteError("Wait for BBS free timeout, aborting");
    return FALSE;
}


