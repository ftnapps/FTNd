/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: File Transfers
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
#include "transfer.h"
#include "change.h"
#include "whoson.h"
#include "funcs.h"
#include "term.h"
#include "ttyio.h"
#include "filesub.h"
#include "language.h"
#include "openport.h"
#include "timeout.h"


/*
 
   Design remarks.

   This entry must accept up and downloads at the same time in case a users
   wants to download but uses a bidirectional protocol and has something to
   upload.

   First design fase: support the now used external protocols and link it to
   the rest of the bbs.

   Second design fase: add build-in zmodem and ymodem-1k.

   To think of:
    - Drop bidirectional support, is this still in use. No bimodem specs and
      Hydra seems not implemented in any terminal emulator.
    - Add sliding kermit? Can still be used externally.
    - Add special internet protocol and make a module for minicom. No, this
      would mean only a few users will use it.

*/

char *transfertime(struct timeval, struct timeval, long bytes, int);


int ForceProtocol()
{
    /*
     * If user has no default protocol, make sure he has one.
     */
    if (strcmp(sProtName, "") == 0) {
	Chg_Protocol();

	/*
	 * If the user didn't pick a protocol, quit.
	 */
	if (strcmp(sProtName, "") == 0) {
	    return FALSE;
	}
    }
    return TRUE;
}



/*
 * Download files to the user.
 * Returns:
 *  0 - All seems well
 *  1 - No transfer protocol selected
 *  2 - No files to download.
 */
int download(down_list *download_list)
{
    down_list	    *tmpf;
    int		    err, maxrc = 0, Count = 0;
    char	    *temp, *symTo, *symFrom;
    unsigned long   Size = 0;
    struct dirent   *dp;
    DIR		    *dirp;
    struct timeval  starttime, endtime;
    struct timezone tz;

    /*
     * If user has no default protocol, make sure he has one.
     */
    if (!ForceProtocol()) {
	return 1;
    }

    /*
     * Clean users tag directory.
     */
    WhosDoingWhat(DOWNLOAD, NULL);
    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "-rf %s/%s/tag", CFG.bbs_usersdir, exitinfo.Name);
    execute_pth((char *)"rm", temp, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null");
    sprintf(temp, "%s/%s/tag", CFG.bbs_usersdir, exitinfo.Name);
    CheckDir(temp);

    symTo   = calloc(PATH_MAX, sizeof(char));
    symFrom = calloc(PATH_MAX, sizeof(char));
    
    /*
     * Build symlinks into the users tag directory.
     */
    chdir("./tag");
    for (tmpf = download_list; tmpf; tmpf = tmpf->next) {
        if (!tmpf->sent && !tmpf->failed) {
	    unlink(tmpf->remote);
	    sprintf(symFrom, "%s", tmpf->remote);
	    sprintf(symTo, "%s", tmpf->local);
	    if (symlink(symTo, symFrom)) {
	        WriteError("$Can't create symlink %s %s %d", symTo, symFrom, errno);
	        tmpf->failed = TRUE;
	    } else {
	        Syslog('b', "Created symlink %s -> %s", symFrom, symTo);
	    }
	    if ((access(symFrom, R_OK)) != 0) {
	        /*
	         * Extra check, is symlink really there?
	         */
	        WriteError("Symlink %s check failed, unmarking download", symFrom);
	        tmpf->failed = TRUE;
	    } else {
	        Count++;
		Size += tmpf->size;
	    }
	}
    }
    Home();

    if (!Count) {
        /*
         * Nothing to download
         */
        free(temp);
        free(symTo);
        free(symFrom);
        return 2;
    }

    clear();
    /* File(s)     : */
    pout(YELLOW, BLACK, (char *) Language(349)); sprintf(temp, "%d", Count);     PUTSTR(temp); Enter(1);
    /* Size        : */
    pout(  CYAN, BLACK, (char *) Language(350)); sprintf(temp, "%lu", Size);     PUTSTR(temp); Enter(1);
    /* Protocol    : */
    pout(  CYAN, BLACK, (char *) Language(351)); sprintf(temp, "%s", sProtName); PUTSTR(temp); Enter(1);

    Syslog('+', "Download tagged files start, protocol: %s", sProtName);

    PUTSTR(sProtAdvice);
    Enter(2);

    /*
     * Wait a while before download
     */
    sleep(2);
    
    if (uProtInternal) {
	for (tmpf = download_list; tmpf && (maxrc < 2); tmpf = tmpf->next) {
	}
    } else {
	gettimeofday(&starttime, &tz);

	/*
	 * Transfer the files. Set the Client/Server time at the maximum
	 * time the user has plus 10 minutes. The overall timer 10 seconds
	 * less. Not a nice but working solution.
	 */
	alarm_set(((exitinfo.iTimeLeft + 10) * 60) - 10);
	Altime((exitinfo.iTimeLeft + 10) * 60);

	sprintf(temp, "%s/%s/tag", CFG.bbs_usersdir, exitinfo.Name);
	if ((dirp = opendir(temp)) == NULL) {
	    WriteError("$Download: Can't open dir: %s", temp);
	    free(temp);
	} else {
	    chdir(temp);
	    free(temp);
	    temp = NULL;
	    while ((dp = readdir(dirp)) != NULL ) {
		if (*(dp->d_name) != '.') {
		    if (temp != NULL) {
			temp = xstrcat(temp, (char *)" ");
			temp = xstrcat(temp, dp->d_name);
		    } else {
			temp = xstrcpy(dp->d_name);
		    }
		}
	    }
	    if (temp != NULL) {
		if ((err = execute_str(sProtDn, temp, NULL, NULL, NULL, NULL))) {
		    WriteError("$Download error %d, prot: %s", err, sProtDn);
		}
		/*
		 * Restore rawport
		 */
		rawport();
		free(temp);
	    } else {
		WriteError("No filebatch created");
	    }
	    closedir(dirp);
	}
	Altime(0);
	alarm_off();
	alarm_on();
	Home();
	gettimeofday(&endtime, &tz);

	/*
	 * Checking the successfull sent files, they are missing from
	 * the ./tag directory. Failed files are still there.
	 */
	Count = Size = 0;

	for (tmpf = download_list; tmpf && (maxrc < 2); tmpf = tmpf->next) {
	    if (!tmpf->sent && !tmpf->failed) {
		sprintf(symTo, "./tag/%s", tmpf->remote);
		/*
		 * If symlink is gone the file is sent.
		 */
		if ((access(symTo, R_OK)) != 0) {
		    Syslog('+', "File %s from area %d sent ok", tmpf->remote, tmpf->area);
		    tmpf->sent = TRUE;
		    Size += tmpf->size;
		    Count++;
		} else {
		    Syslog('+', "Failed to sent %s from area %d", Tag.LFile, Tag.Area);
		}
	    }
	}

	/*
	 * Work out transfer rate in seconds by dividing the
	 * Size of the File by the amount of time it took to download 
	 * the file.
	 */
	Syslog('+', "Download %s, %d", transfertime(starttime, endtime, (unsigned long)Size, TRUE), Count);
    }
    
    free(symTo);
    free(symFrom);

    return maxrc;
}



/*
 * Upload files from the user.
 * Returns:
 *  0 - All seems well
 *  1 - No transfer protocol selected.
 */
int upload(up_list *upload_list)
{
    /*
     * If user has no default protocol, make sure he has one.
     */
    if (!ForceProtocol()) {
	return 1;
    }

    return 0;
}



char *transfertime(struct timeval start, struct timeval end, long bytes, int sent)
{
    static char     resp[81];
    double long     startms, endms, elapsed;

    startms = (start.tv_sec * 1000) + (start.tv_usec / 1000);
    endms = (end.tv_sec * 1000) + (end.tv_usec / 1000);
    elapsed = endms - startms;
    memset(&resp, 0, sizeof(resp));
    if (!elapsed)
	elapsed = 1L;
    if (bytes > 1000000)
	sprintf(resp, "%ld bytes %s in %0.3Lf seconds (%0.3Lf Kb/s)",
		bytes, sent?"sent":"received", elapsed / 1000.000, ((bytes / elapsed) * 1000) / 1024);
    else
	sprintf(resp, "%ld bytes %s in %0.3Lf seconds (%0.3Lf Kb/s)", 
		bytes, sent?"sent":"received", elapsed / 1000.000, ((bytes * 1000) / elapsed) / 1024);   
    return resp;
}

