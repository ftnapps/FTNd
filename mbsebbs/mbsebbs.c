/*****************************************************************************
 *
 * $Id: mbsebbs.c,v 1.59 2007/10/14 13:15:34 mbse Exp $
 * Purpose ...............: Main startup
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
#include "input.h"



extern	int	do_quiet;	/* Logging quiet flag	*/
time_t		t_start;
int		cols = 80;	/* Screen columns	*/
int		rows = 24;	/* Screen rows		*/


#ifdef  HAVE_GEOIP_H

extern void _GeoIP_setup_dbfilename(void);

void geoiplookup(GeoIP* gi, char *hostname, int i)
{
    const char * country_code;
    const char * country_name;
    const char * country_continent;
    int country_id;

    if (GEOIP_COUNTRY_EDITION == i) {
	country_id = GeoIP_id_by_name(gi, hostname);
	Syslog('b', "geoiplookup '%s', id=%d", hostname, country_id);
	country_code = GeoIP_country_code[country_id];
	country_name = GeoIP_country_name[country_id];
	country_continent = GeoIP_country_continent[country_id];
	if (country_code == NULL) {
	    Syslog('+', "%s: IP Address not found\n", GeoIPDBDescription[i]);
	} else { 
	    Syslog('+', "GeoIP location: %s, %s %s\n", country_name, country_code, country_continent);
	}
    }
}
#endif


int main(int argc, char **argv)
{
    FILE	    *pTty;
    char	    *p, *tty, temp[PATH_MAX];
    int		    i, rc, Fix;
    struct stat	    sb;
    struct winsize  ws;
    unsigned char   ch = 0;
#ifdef  HAVE_GEOIP_H
    char    	    *hostname;
    GeoIP   	    *gi;
#endif

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

    if (ioctl(1, TIOCGWINSZ, &ws) != -1 && (ws.ws_col > 0) && (ws.ws_row > 0)) {
	cols = ws.ws_col;
	rows = ws.ws_row;
	PUTSTR((char *)"Loading MBSE BBS ...");
	Enter(1);
    } else {
	Syslog('b', "Could not get screensize using ioctl() call");
	if (getenv("LINES") != NULL) {
	    rows = atoi(getenv("LINES"));
	    PUTSTR((char *)"Loading MBSE BBS ...");
	    Enter(1);
	} else {
	    Syslog('b', "Could not get screensize from environment too");
	
	    /*
	     * Next idea borrowed from Synchronet BBS
	     */
	    PUTSTR((char *)"\r\n");		    /* Locate cursor at column 1 */
	    PUTSTR((char *)"\x1b[s");		    /* Save cursor position (necessary for HyperTerm auto-ANSI) */
	    PUTSTR((char *)"\x1b[99B_");	    /* locate cursor as far down as possible */
	    PUTSTR((char *)"\x1b[6n");		    /* Get cursor position */
	    PUTSTR((char *)"\x1b[u");		    /* Restore cursor position */
//	    PUTSTR((char *)"\x1b[!_");		    /* RIP? */
//	    PUTSTR((char *)"\x1b[0t_");		    /* WIP? */
//	    PUTSTR((char *)"\2\2?HTML?");	    /* HTML? */
	    PUTSTR((char *)"\x1b[0m_");		    /* "Normal" colors */
	    PUTSTR((char *)"\x1b[2J");		    /* clear screen */
	    PUTSTR((char *)"\x1b[H");		    /* home cursor */
	    PUTSTR((char *)"\xC");		    /* clear screen (in case not ANSI) */
	    PUTSTR((char *)"\r");		    /* Move cursor left (in case previous char printed) */
	    PUTSTR((char *)"Loading MBSE BBS ..."); /* Let the user think something is happening	*/
	    Enter(1);

	    memset(&temp, 0, sizeof(temp));
	    for (i = 0; i < 10; i++) {
		rc = Waitchar(&ch, 200);    /* 4 seconds timeout */
		if (rc == 1) {
		    temp[i] = ch;
		    if (ch > 31)
			Syslog('b', "detect rc=%d ch=%d ch=%c", rc, ch, ch);
		    else
			Syslog('b', "detect rc=%d ch=%d ch=^%c", rc, ch, ch+64);
		}
		if (rc == TIMEOUT)
		    break;
	    }
	    if ((temp[0] == '\x1b') && (temp[1] == '[') && strchr(temp, ';') /*&& strchr(temp, 'R') */) {
		rows = atoi(strtok(temp +2, ";"));
		Syslog('b', "detected ANSI cursor position at row %d", rows);
		if (rows < 24)
		    rows = 24;
	    }
	}
    }

    if ((p = getenv("CONNECT")) != NULL)
	Syslog('+', "CONNECT %s", p);
    if ((p = getenv("CALLER_ID")) != NULL)
	if (strncmp(p, "none", 4))
	    Syslog('+', "CALLER_ID  %s", p);
    if ((p = getenv("TERM")) != NULL)
	Syslog('+', "TERM=%s %dx%d", p, cols, rows);
    else
	Syslog('+', "TERM=invalid %dx%d", cols, rows);
    if ((p = getenv("LANG")) != NULL)
	Syslog('+', "LANG=%s", p);
    if ((p = getenv("LC_ALL")) != NULL)
	Syslog('+', "LC_ALL=%s", p);

    /*
     * Initialize 
     */
    snprintf(current_language, 10, "%s", CFG.deflang);
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
	snprintf(pTTY, 15, "%s", tty+5);
    else if (*tty == '/') {
	tty = strrchr(ttyname(0), '/');
	++tty;
	snprintf(pTTY, 15, "%s", tty);
    }

    umask(007);

    /* 
     * Trap signals
     */
    for (i = 0; i < NSIG; i++) {
	if ((i == SIGHUP) || (i == SIGPIPE) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM) || (i == SIGIOT))
	    signal(i, (void (*))die);
	else if (i == SIGCHLD)
	    signal(i, SIG_DFL);
    }

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

    snprintf(temp, 81, "MBSE BBS v%s (Release: %s) on %s/%s", VERSION, ReleaseDate, OsName(), OsCPU());
    poutCR(YELLOW, BLACK, temp);
    pout(WHITE, BLACK, (char *)COPYRIGHT);

    /*
     * Use 80 wide screens
     */
    if (cols > 80)
	cols = 80;

    /*
     * Check and report screens that are too small
     */
    if ((cols < 80) || (rows < 24)) {
	snprintf(temp, 81, "Your screen is set to %dx%d, we use 80x24 at least", cols, rows);
	poutCR(LIGHTRED, BLACK, temp);
	Enter(1);
	cols = 80;
	rows = 24;
    } else {
	Enter(2);
    }
 
    /*
     * Check users homedirectory, some *nix systems let users in if no
     * homedirectory exists. Then check the permissions.
     */
    snprintf(temp, PATH_MAX, "%s/%s", CFG.bbs_usersdir, sUnixName);
    if (stat(temp, &sb)) {
	snprintf(temp, 81, "No homedirectory\r\n\r\n");
	PUTSTR(temp);
	WriteError("homedirectory %s doesn't exist", temp);
	Quick_Bye(MBERR_OK);
    }
    Fix = FALSE;
    if ((sb.st_mode & S_IRUSR) == 0) {
	Fix = TRUE;
	WriteError("No owner read access in %s, mode is %04o", temp, sb.st_mode & 0x1ff);
    }
    if ((sb.st_mode & S_IWUSR) == 0) {
	Fix = TRUE;
	WriteError("No owner write access in %s, mode is %04o", temp, sb.st_mode & 0x1ff);
    }
    if ((sb.st_mode & S_IXUSR) == 0) {
	Fix = TRUE;
	WriteError("No owner execute access in %s, mode is %04o", temp, sb.st_mode & 0x1ff);
    }
    if ((sb.st_mode & S_IRGRP) == 0) {
	Fix = TRUE;
	WriteError("No group read access in %s, mode is %04o", temp, sb.st_mode & 0x1ff);
    }
    if ((sb.st_mode & S_IWGRP) == 0) {
	Fix = TRUE;
	WriteError("No group write access in %s, mode is %04o", temp, sb.st_mode & 0x1ff);
    }
    if ((sb.st_mode & S_IXGRP) == 0) {
	Fix = TRUE;
	WriteError("No group execute access in %s, mode is %04o", temp, sb.st_mode & 0x1ff);
    }
    if ((sb.st_mode & S_IROTH)) {
	Fix = TRUE;
	WriteError("Others have read access in %s, mode is %04o", temp, sb.st_mode & 0x1ff);
    }
    if ((sb.st_mode & S_IWOTH)) {
	Fix = TRUE;
	WriteError("Others have write access in %s, mode is %04o", temp, sb.st_mode & 0x1ff);
    }
    if ((sb.st_mode & S_IXOTH)) {
	Fix = TRUE;
	WriteError("Others have execute access in %s, mode is %04o", temp, sb.st_mode & 0x1ff);
    }
    if (Fix) {
	if (chmod(temp, 0770)) {
	    WriteError("Could not set home directory mode to 0770");
	    snprintf(temp, 81, "Internal error, the sysop is notified");
	    poutCR(LIGHTRED, BLACK, temp);
	    Enter(1);
	    Quick_Bye(MBERR_OK);
	} else {
	    Syslog('+', "Corrected home directory mode to 0770");
	}
    }

    if (((p = getenv("REMOTEHOST")) != NULL) || ((p = getenv("SSH_CLIENT")) != NULL)) {
	/*
	 * Network connection, no tty checking but fill a ttyinfo record.
	 */
#ifdef  HAVE_GEOIP_H
	hostname = xstrcpy(p);
	_GeoIP_setup_dbfilename();
	if (GeoIP_db_avail(GEOIP_COUNTRY_EDITION)) {
	    if ((gi = GeoIP_open_type(GEOIP_COUNTRY_EDITION, GEOIP_STANDARD)) != NULL) {
		geoiplookup(gi, hostname, GEOIP_COUNTRY_EDITION);
	    }
	    GeoIP_delete(gi);
	}
#endif
	memset(&ttyinfo, 0, sizeof(ttyinfo));
	snprintf(ttyinfo.comment, 41, "%s", p);
	snprintf(ttyinfo.tty,      7, "%s", pTTY);
	snprintf(ttyinfo.speed,   21, "10 mbit");
	snprintf(ttyinfo.flags,   31, "IBN,IFC,XX");
	ttyinfo.type = NETWORK;
	ttyinfo.available = TRUE;
	ttyinfo.honor_zmh = FALSE;
	snprintf(ttyinfo.name,    36, "Network port #%d", iNode);
    } else {
	/*
	 * Check if this port is available.
	 */
	snprintf(temp, PATH_MAX, "%s/etc/ttyinfo.data", getenv("MBSE_ROOT"));
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
		snprintf(temp, 81, "No BBS on this port allowed!\r\n\r\n");
		PUTSTR(temp);
		Free_Language();
		Quick_Bye(MBERR_OK);
	    }
	}
    }

    /* 
     * Display Connect String if turned on.
     */
    if (CFG.iConnectString) {
	/* Connected from */
	snprintf(temp, 81, "%s\"%s\" ", (char *) Language(348), ttyinfo.comment);
	pout(CYAN, BLACK, temp);
	/* line */
	snprintf(temp, 81, "%s%d ", (char *) Language(31), iNode);
	pout(CYAN, BLACK, temp);
	/* on */
	snprintf(temp, 81, "%s %s", (char *) Language(135), ctime(&ltime));
	PUTSTR(temp);
	Enter(1);
    }

    /*
     * Some debugging for me
     */
    Syslog('b', "setlocale(LANG, NULL) returns \"%s\"", printable(setlocale(LANG, NULL), 0));
    Syslog('b', "setlocale(LC_CTYPE, NULL) returns \"%s\"", printable(setlocale(LC_CTYPE, NULL), 0));
    Syslog('b', "setlocale(LC_ALL, NULL) returns \"%s\"", printable(setlocale(LC_ALL, NULL), 0));
#ifndef __OpenBSD__
    Syslog('b', "nl_langinfo(CODESET) returns \"%s\"", nl_langinfo(CODESET));
#endif

    snprintf(sMailbox, 21, "mailbox");
    colour(LIGHTGRAY, BLACK);
    user();
    return 0;
}

