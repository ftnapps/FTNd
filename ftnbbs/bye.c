/*****************************************************************************
 *
 * bye.c
 * Purpose ...............: Hangup functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2006 Michiel Broek <mbse@mbse.eu>
 * Copyright (C)    2012   Robert James Clay <jame@rocasa.us>
 *
 * This file is part of FTNd.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * FTNd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with FTNd; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/ftndlib.h"
#include "../lib/ftnd.h"
#include "../lib/users.h"
#include "../lib/nodelist.h"
#include "dispfile.h"
#include "misc.h"
#include "language.h"
#include "bye.h"
#include "term.h"
#include "openport.h"
#include "ttyio.h"
#include "mib.h"


extern	pid_t		mypid;
extern	time_t		t_start;
extern	char		*StartTime;
extern	int		hanged_up;

extern	unsigned int	mib_sessions;
extern	unsigned int	mib_minutes;


int			do_mailout = FALSE;


void Good_Bye(int onsig)
{
    FILE    *pUsrConfig, *pExitinfo;
    char    *temp;
    int	    offset;
    time_t  t_end;
    int	    i;

    IsDoing("Hangup");
    temp = calloc(PATH_MAX, sizeof(char));
    Syslog('+', "Good_Bye(%d)", onsig);

    /*
     * Don't display goodbye screen on SIGHUP and idle timeout.
     * With idle timeout this will go into a loop.
     */
    if ((onsig != SIGALRM) && (onsig != FTNERR_TIMEOUT) && (hanged_up == 0))
	DisplayFile((char *)"goodbye");

    SaveLastCallers();

    /*
     * Update the users database record.
     */
    snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("FTND_ROOT"));
    if ((pUsrConfig = fopen(temp,"r+")) != NULL) {
	snprintf(temp, PATH_MAX, "%s/%s/exitinfo", CFG.bbs_usersdir, exitinfo.Name);
	if ((pExitinfo = fopen(temp,"rb")) != NULL) {
	    fread(&usrconfighdr, sizeof(usrconfighdr), 1, pUsrConfig);
	    fread(&exitinfo, sizeof(exitinfo), 1, pExitinfo);

	    usrconfig = exitinfo;
	    fclose(pExitinfo);
	    usrconfig.iLastFileArea = iAreaNumber;

	    /* If time expired, do not say say successful logoff */
	    if (!iExpired && !hanged_up)
		Syslog('+', "User successfully logged off BBS");

	    usrconfig.iLastMsgArea = iMsgAreaNumber;

	    offset = usrconfighdr.hdrsize + (grecno * usrconfighdr.recsize);
	    if (fseek(pUsrConfig, offset, SEEK_SET) != 0) {
		WriteError("$Can't move pointer in file %s/etc/users.data", getenv("FTND_ROOT"));
	    } else {
	        fwrite(&usrconfig, sizeof(usrconfig), 1, pUsrConfig);
	    }
	    fclose(pUsrConfig);
	}
    }

    /*
     * Update mib counters
     */
    t_end = time(NULL);
    mib_minutes = (unsigned int) ((t_end - t_start) / 60);
    mib_sessions++;

    sendmibs();

    /*
     * Flush all data to the user, wait 5 seconds to
     * be sure the user received all data.
     */
    if ((onsig != SIGALRM) && (onsig != FTNERR_TIMEOUT) && (hanged_up == 0)) {
	colour(LIGHTGRAY, BLACK);
	sleep(4);
    }

    for (i = 0; i < NSIG; i++) {
	if (i == SIGCHLD)
	    signal(i, SIG_DFL);
	else if ((i != SIGKILL) && (i != SIGSTOP))
	    signal(i, SIG_DFL);
    }

    if ((onsig != SIGALRM) && (onsig != FTNERR_TIMEOUT) && (hanged_up == 0)) {
    	cookedport();
    }

    /*
     * Ignore SIGHUP during hangup.
     */
    signal(SIGHUP, SIG_IGN);
    hangup();

    for (i = 0; i < NSIG; i++) {
	if ((i == SIGHUP) || (i == SIGPIPE) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM))
	    signal(i, SIG_DFL);
    }
    
    if (do_mailout)
	CreateSema((char *)"mailout");

    t_end = time(NULL);
    Syslog(' ', "FTNDBBS finished in %s", t_elapsed(t_start, t_end));
    sleep(1);

    /*
     * Start shutting down this session, cleanup some files.
     */
    socket_shutdown(mypid);
    snprintf(temp, PATH_MAX, "%s/tmp/ftnd%d", getenv("FTND_ROOT"), getpid());
    unlink(temp);

    snprintf(temp, PATH_MAX, "%s/%s/.quote", CFG.bbs_usersdir, exitinfo.Name);
    unlink(temp);

    snprintf(temp, PATH_MAX, "%s/%s/data.msg", CFG.bbs_usersdir, exitinfo.Name);
    unlink(temp);

    snprintf(temp, PATH_MAX, "%s/%s/door.sys", CFG.bbs_usersdir, exitinfo.Name);
    unlink(temp);

    snprintf(temp, PATH_MAX, "%s/%s/door32.sys", CFG.bbs_usersdir, exitinfo.Name);
    unlink(temp);

    snprintf(temp, PATH_MAX, "%s/%s/exitinfo", CFG.bbs_usersdir, exitinfo.Name);
    unlink(temp);
    free(temp);
    unlink("taglist");

    Free_Language();
    free(pTTY);
    if (StartTime)
	free(StartTime);
    deinitnl();
    exit(onsig);
}



void Quick_Bye(int onsig)
{
    char    *temp;
    int	    i;

    temp = calloc(PATH_MAX, sizeof(char));
    Syslog('+', "Quick_Bye");
    socket_shutdown(mypid);
    snprintf(temp, PATH_MAX, "%s/tmp/ftnd%d", getenv("FTND_ROOT"), getpid());
    unlink(temp);
    free(temp);
    colour(LIGHTGRAY, BLACK);
    sleep(3);

    if ((onsig != SIGALRM) && (onsig != FTNERR_TIMEOUT) && (hanged_up == 0)) {
        cookedport();
    }

    /*
     * Ignore SIGHUP during hangup
     */
    signal(SIGHUP, SIG_IGN);
    hangup();

    /*
     * Prevent that we call die() if something goes wrong next
     */
    for (i = 0; i < NSIG; i++)
        if ((i == SIGHUP) || (i == SIGPIPE) || (i == SIGBUS) || (i == SIGILL) || (i == SIGSEGV) || (i == SIGTERM))
            signal(i, SIG_DFL);

    free(pTTY);
    if (StartTime)
	free(StartTime);
    exit(FTNERR_OK);
}



