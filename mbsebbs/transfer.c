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
#include "zmmisc.h"
#include "zmsend.h"
#include "zmrecv.h"
#include "ymsend.h"
#include "ymrecv.h"


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


int	sentbytes;
int	rcvdbytes;



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



void add_download(down_list **lst, char *local, char *remote, long Area, unsigned long size, int kfs)
{
    down_list	*tmp, *ta;
    int		i;

    Syslog('b', "add_download(\"%s\",\"%s\",%ld,%ld,%d)", MBSE_SS(local), MBSE_SS(remote), Area, size, kfs);

    tmp = (down_list *)malloc(sizeof(down_list));
    tmp->next = NULL;
    tmp->local = xstrcpy(local);
    tmp->remote = xstrcpy(remote);
    tmp->cps = 0;
    tmp->area = Area;
    tmp->size = size;
    tmp->kfs = kfs;
    tmp->sent = FALSE;
    tmp->failed = FALSE;

    /*
     * Most prottocols don't allow spaces in filenames, so we modify the remote name.
     * However, they should not exist on a bbs.
     */
    for (i = 0; i < strlen(tmp->remote); i++) {
	if (tmp->remote[i] == ' ')
	    tmp->remote[i] = '_';
    }

    if (*lst == NULL) {
	*lst = tmp;
    } else {
	for (ta = *lst; ta; ta = ta->next) {
	    if (ta->next == NULL) {
		ta->next = (down_list *)tmp;
		break;
	    }
	}
    }
    
    return;
}



void tidy_download(down_list **fdp)
{
    down_list	*tmp, *old;

    for (tmp = *fdp; tmp; tmp = old) {
	old = tmp->next;
	if (tmp->local)
	    free(tmp->local);
	if (tmp->remote)
	    free(tmp->remote);
	free(tmp);
    }

    *fdp = NULL;
    return;
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

    Syslog('b', "download()");
    for (tmpf = download_list; tmpf; tmpf = tmpf->next) {
	Syslog('b', "%s,%s,%ld,%ld,%ld,%s,%s,%s", tmpf->local, tmpf->remote, tmpf->cps, tmpf->area, tmpf->size,
		tmpf->kfs ?"KFS":"KEEP", tmpf->sent ?"SENT":"N/A", tmpf->failed ?"FAILED":"N/A");
    }

    /*
     * If user has no default protocol, make sure he has one.
     */
    if (!ForceProtocol()) {
	return 1;
    }

    symTo   = calloc(PATH_MAX, sizeof(char));
    symFrom = calloc(PATH_MAX, sizeof(char));
    temp    = calloc(PATH_MAX, sizeof(char));

    /*
     * Build symlinks into the users tag directory.
     */
    chdir("./tag");
    for (tmpf = download_list; tmpf; tmpf = tmpf->next) {
        if (!tmpf->sent && !tmpf->failed) {
	    sprintf(symFrom, "%s/%s/tag/%s", CFG.bbs_usersdir, exitinfo.Name, tmpf->remote);
	    Syslog('b', "test \"%s\" \"%s\"", symFrom, tmpf->local);
	    if (strcmp(symFrom, tmpf->local)) {
		Syslog('b', "different, need a symlink");
		unlink(tmpf->remote);
		sprintf(symFrom, "%s", tmpf->remote);
		sprintf(symTo, "%s", tmpf->local);
		if (symlink(symTo, symFrom)) {
		    WriteError("$Can't create symlink %s %s %d", symTo, symFrom, errno);
		    tmpf->failed = TRUE;
		} else {
		    Syslog('b', "Created symlink %s -> %s", symFrom, symTo);
		}
		tmpf->kfs = FALSE;
	    } else {
		Syslog('b', "the same, file is in tag directory");
	    }

	    /*
	     * Check if file or symlink is really there.
	     */
	    sprintf(symFrom, "%s", tmpf->remote);
	    if ((access(symFrom, F_OK)) != 0) {
	        WriteError("File or symlink %s check failed, unmarking download", symFrom);
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

    Syslog('+', "Download files start, protocol: %s", sProtName);

    PUTSTR(sProtAdvice);
    Enter(2);

    /*
     * Wait a while before download
     */
    sleep(2);
    
    if (uProtInternal) {
	sprintf(temp, "%s/%s/tag", CFG.bbs_usersdir, exitinfo.Name);
	chdir(temp);
	if (strncasecmp(sProtName, "zmodem 8k", 9) == 0) {
	    maxrc = zmsndfiles(download_list, TRUE);
	    Home();
	} else if (strncasecmp(sProtName, "zmodem", 6) == 0) {
	    maxrc = zmsndfiles(download_list, FALSE);
	    Home();
	} else if ((strncasecmp(sProtName, "xmodem", 6) == 0) || (strncasecmp(sProtName, "ymodem", 6) == 0)) {
	    if (strncasecmp(sProtName, "xmodem", 6) == 0)
		protocol = ZM_XMODEM;
	    else
		protocol = ZM_YMODEM;
	    if (strstr(sProtName, "1K") || strstr(sProtName, "1k"))
		maxrc = ymsndfiles(download_list, TRUE);
	    else
		maxrc = ymsndfiles(download_list, FALSE);
	    Home();
	} else {
	    Syslog('!', "Warning internal protocol %s not supported", sProtName);
	    maxrc = 1;
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
	Syslog('+', "Download %s in %d file(s)", transfertime(starttime, endtime, (unsigned long)Size, TRUE), Count);
    }
    
    free(symTo);
    free(symFrom);

    for (tmpf = download_list; tmpf; tmpf = tmpf->next) {
	Syslog('b', "%s,%s,%ld,%ld,%ld,%s,%s,%s", tmpf->local, tmpf->remote, tmpf->cps, tmpf->area, tmpf->size,
		tmpf->kfs ?"KFS":"KEEP", tmpf->sent ?"SENT":"N/A", tmpf->failed ?"FAILED":"N/A");
    }

    Syslog('b', "download() rc=%d", maxrc);
    return maxrc;
}



void tidy_upload(up_list **fdp)
{
    up_list   *tmp, *old;

    for (tmp = *fdp; tmp; tmp = old) {
	old = tmp->next;
	if (tmp->filename)
	    free(tmp->filename);
	free(tmp);
    }

    *fdp = NULL;
    return;
}



/*
 * Upload files from the user.
 * Returns:
 *  0 - All seems well
 *  1 - No transfer protocol selected.
 *  2 - Transfer failed
 */
int upload(up_list **upload_list)
{
    char	    *temp;
    struct dirent   *dp;
    DIR		    *dirp;
    struct stat	    statfile;
    struct timeval  starttime, endtime;
    struct timezone tz;
    unsigned long   Size = 0;
    int		    err, Count = 0, rc = 0;
    up_list	    *tmp, *ta;

    /*
     * If user has no default protocol, make sure he has one.
     */
    if (!ForceProtocol()) {
	return 1;
    }

    temp  = calloc(PATH_MAX, sizeof(char));

    /* Please start your upload now */
    sprintf(temp, "%s, %s", sProtAdvice, (char *) Language(283));
    pout(CFG.HiliteF, CFG.HiliteB, temp);
    Enter(2);
    Syslog('+', "Upload using %s", sProtName);

    sprintf(temp, "%s/%s/upl", CFG.bbs_usersdir, exitinfo.Name);

    if (chdir(temp)) {
	WriteError("$Can't chdir to %s", temp);
	free(temp);
	return 1;
    }
    sleep(2);

    if (uProtInternal) {
	if (strncasecmp(sProtName, "zmodem", 6) == 0) {
	    rc = zmrcvfiles();
	    Syslog('b', "Begin dir processing");
	    if ((dirp = opendir(".")) == NULL) {
	        WriteError("$Upload: can't open ./upl");
	        Home();
	        rc = 1;
	    } else {
	        while ((dp = readdir(dirp)) != NULL) {
		    if (*(dp->d_name) != '.') {
			if (rc == 0) {
			    stat(dp->d_name, &statfile);
			    Syslog('b', "Uploaded \"%s\", %ld bytes", dp->d_name, statfile.st_size);
			    sprintf(temp, "%s/%s/upl/%s", CFG.bbs_usersdir, exitinfo.Name, dp->d_name);
			    chmod(temp, 0660);

			    /*
			     * Add uploaded file to the list
			     */
			    tmp = (up_list *)malloc(sizeof(up_list));
			    tmp->next = NULL;
			    tmp->filename = xstrcpy(temp);
			    tmp->size = Size;
			    if (*upload_list == NULL) {
				*upload_list = tmp;
			    } else {
				for (ta = *upload_list; ta; ta = ta->next) {
				    if (ta->next == NULL) {
					ta->next = (up_list *)tmp;
					break;
				    }
				}
			    }
			} else {
			    Syslog('+', "Remove failed %s result %d", dp->d_name, unlink(dp->d_name));
			}
		    }
		}
		closedir(dirp);
	    }
	} else {
	    Syslog('!', "Internal protocol %s not supported", sProtName);
	    free(temp);
	    return 1;
	}
    } else {
	/*
	 * External protocol
	 */
	gettimeofday(&starttime, &tz);
    
	Altime(7200);
	alarm_set(7190);
	err = execute_str(sProtUp, (char *)"", NULL, NULL, NULL, NULL);
	if (rawport() != 0) {
	    WriteError("Unable to set raw mode");
	}
	if (err) {
	    WriteError("$Upload error %d, prot: %s", err, sProtUp);
	    rc = 2;
	}
	Altime(0);
	alarm_off();
	alarm_on();

	gettimeofday(&endtime, &tz);

	/*
	 * With external protocols we don't know anything about what we got.
	 * Just check the files in the users upload directory.
	 */
	if ((dirp = opendir(".")) == NULL) {
	    WriteError("$Upload: can't open ./upl");
	    Home();
	    rc = 1;
	} else {
	    while ((dp = readdir(dirp)) != NULL) {
		if (*(dp->d_name) != '.') {
		    stat(dp->d_name, &statfile);
		    Syslog('+', "Uploaded \"%s\", %ld bytes", dp->d_name, statfile.st_size);
		    Count++;
		    Size += statfile.st_size;
		    sprintf(temp, "%s/%s/upl/%s", CFG.bbs_usersdir, exitinfo.Name, dp->d_name);
		    chmod(temp, 0660);

		    /*
		     * Add uploaded file to the list
		     */
		    tmp = (up_list *)malloc(sizeof(up_list));
		    tmp->next = NULL;
		    tmp->filename = xstrcpy(temp);
		    tmp->size = Size;

		    if (*upload_list == NULL) {
			*upload_list = tmp;
		    } else {
			for (ta = *upload_list; ta; ta = ta->next) {
			    if (ta->next == NULL) {
				ta->next = (up_list *)tmp;
				break;
			    }
			}
		    }
		}
	    }
	    closedir(dirp);
	    Syslog('+', "Upload %s in %d file(s)", transfertime(starttime, endtime, (unsigned long)Size, FALSE), Count);
	}
    }
    free(temp);
    Syslog('b', "Done, return rc=%d", rc);

    return rc;
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

