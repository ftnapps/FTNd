/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Main startup
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
#include "../lib/msg.h"
#include "mbsebbs.h"
#include "user.h"
#include "dispfile.h"
#include "language.h"
#include "menu.h"
#include "misc.h"
#include "bye.h"
#include "timeout.h"
#include "funcs.h"
#include "term.h"
#include "ttyio.h"
#include "openport.h"



extern	int	do_quiet;	/* Logging quiet flag */
time_t		t_start;



int main(int argc, char **argv)
{
    FILE	*pTty;
    char	*p, *tty, temp[PATH_MAX];
    int		i, rc;
    struct stat	sb;

    pTTY = calloc(15, sizeof(char));
    tty = ttyname(1);

    /*
     * Find username from the environment
     */
    sUnixName[0] = '\0';
    if (getenv("LOGNAME") != NULL) {
        strncpy(sUnixName, getenv("LOGNAME"), 8);
    } else if (getenv("USER") != NULL) {
	strcpy(sUnixName, getenv("USER"));
    } else {
        fprintf(stderr, "No username in environment\n");
        Quick_Bye(MBERR_OK);
    }

    /*
     * Get MBSE_ROOT Path and load Config into Memory
     */
    FindMBSE();

    /* 
     * Set local time and statistic indexes.
     */
    Time_Now = t_start = time(NULL); 
    l_date = localtime(&Time_Now); 
    Diw = l_date->tm_wday;
    Miy = l_date->tm_mon;
    ltime = time(NULL);  

    /*
     * Initialize this client with the server. 
     */
    do_quiet = TRUE;
    InitClient(sUnixName, (char *)"mbsebbs", (char *)"Unknown", CFG.logfile, 
	    CFG.bbs_loglevel, CFG.error_log, CFG.mgrlog, CFG.debuglog);
    IsDoing("Loging in");
    Syslog(' ', " ");
    Syslog(' ', "MBSEBBS v%s", VERSION);

    if ((rc = rawport()) != 0) {
	WriteError("Unable to set raw mode");
	Quick_Bye(MBERR_OK);;
    }

    PUTSTR((char *)"Loading MBSE BBS ...");
    Enter(1);

    if ((p = getenv("CONNECT")) != NULL)
	Syslog('+', "CONNECT %s", p);
    if ((p = getenv("CALLER_ID")) != NULL)
	if (strncmp(p, "none", 4))
	    Syslog('+', "CALLER_ID  %s", p);

    /*
     * Initialize 
     */
    InitLanguage();
    InitMenu();
    memset(&MsgBase, 0, sizeof(MsgBase));
		
    i = getpid();

    if ((tty = ttyname(0)) == NULL) {
	WriteError("Not at a tty");
	Free_Language();
	Quick_Bye(MBERR_OK);
    }

    if (strncmp("/dev/", tty, 5) == 0)
	sprintf(pTTY, "%s", tty+5);
    else if (*tty == '/') {
	tty = strrchr(ttyname(0), '/');
	++tty;
	sprintf(pTTY, "%s", tty);
    }

    umask(007);

    /* 
     * Trap signals
     */
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGHUP) || (i == SIGPIPE) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM))
	    signal(i, (void (*))die);
	else if (i == SIGCHLD)
	    signal(i, SIG_DFL);
    }

    /*
     * Default set the terminal to ANSI mode. If your logo
     * is in color, the user will see color no mather what.
     */
    TermInit(1, 80, 24);

    /*
     * Now it's time to check if the bbs is open. If not, we 
     * log the user off.
     */
    if (CheckStatus() == FALSE) {
	Syslog('+', "Kicking user out, the BBS is closed");
	Free_Language();
	Quick_Bye(MBERR_OK);
    }

    clear();
    DisplayLogo();

    sprintf(temp, "MBSE BBS v%s (Release: %s) on %s/%s", VERSION, ReleaseDate, OsName(), OsCPU());
    poutCR(YELLOW, BLACK, temp);
    pout(WHITE, BLACK, (char *)COPYRIGHT);
    Enter(2);
 
    /*
     * Check users homedirectory, some *nix systems let users in if no
     * homedirectory exists
     */
    sprintf(temp, "%s/%s", CFG.bbs_usersdir, sUnixName);
    if (stat(temp, &sb)) {
	sprintf(temp, "No homedirectory\r\n\r\n");
	PUTSTR(temp);
	WriteError("homedirectory %s doesn't exist", temp);
	Quick_Bye(MBERR_OK);
    }
    
    /*
     * Check if this port is available. In iNode we set a fake
     * line number, this will be used by doors.
     */
    sprintf(temp, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
    if ((pTty = fopen(temp, "r")) == NULL) {
	WriteError("Can't read %s", temp);	
    } else {
	fread(&ttyinfohdr, sizeof(ttyinfohdr), 1, pTty);

	while (fread(&ttyinfo, ttyinfohdr.recsize, 1, pTty) == 1) {
	    if (strcmp(ttyinfo.tty, pTTY) == 0) 
		break;
	}
	fclose(pTty);

	if ((strcmp(ttyinfo.tty, pTTY) != 0) || (!ttyinfo.available)) {
	    Syslog('+', "No BBS allowed on port \"%s\"", pTTY);
	    sprintf(temp, "No BBS on this port allowed!\r\n\r\n");
	    PUTSTR(temp);
	    Free_Language();
	    Quick_Bye(MBERR_OK);
	}

	/* 
	 * Display Connect String if turned on.
	 */
	if (CFG.iConnectString) {
	    /* Connected on port */
	    sprintf(temp, "%s\"%s\" ", (char *) Language(348), ttyinfo.comment);
	    pout(CYAN, BLACK, temp);
	    /* on */
	    sprintf(temp, "%s %s", (char *) Language(135), ctime(&ltime));
	    PUTSTR(temp);
	    Enter(1);
	}
    }

    /*
     * Some debugging for me
     */
    Syslog('b', "setlocale(LC_ALL, NULL) returns \"%s\"", printable(setlocale(LC_ALL, NULL), 0));
    /* Next is not usefull */
    Syslog('b', "nl_langinfo(LC_CTYPE) returns \"%s\"", printable(nl_langinfo(LC_CTYPE), 0));

    sprintf(sMailbox, "mailbox");
    colour(LIGHTGRAY, BLACK);
    user();
    return 0;
}

