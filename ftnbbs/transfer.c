/*****************************************************************************
 *
 * $Id: transfer.c,v 1.21 2005/10/11 20:49:48 mbse Exp $
 * Purpose ...............: File Transfers
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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


int	sentbytes;
int	rcvdbytes;

extern int  zmodem_requested;


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



void add_download(down_list **lst, char *local, char *remote, int Area, unsigned int size, int kfs)
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
    unsigned int    Size = 0;
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
	    snprintf(symFrom, PATH_MAX, "%s/%s/tag/%s", CFG.bbs_usersdir, exitinfo.Name, tmpf->remote);
	    Syslog('b', "test \"%s\" \"%s\"", symFrom, tmpf->local);
	    if (strcmp(symFrom, tmpf->local)) {
		Syslog('b', "different, need a symlink");
		unlink(tmpf->remote);
		snprintf(symFrom, PATH_MAX, "%s", tmpf->remote);
		snprintf(symTo, PATH_MAX, "%s", tmpf->local);
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
	    snprintf(symFrom, PATH_MAX, "%s", tmpf->remote);
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
    pout(YELLOW, BLACK, (char *) Language(349)); snprintf(temp, PATH_MAX, "%d", Count);     PUTSTR(temp); Enter(1);
    /* Size        : */
    pout(  CYAN, BLACK, (char *) Language(350)); snprintf(temp, PATH_MAX, "%u", Size);      PUTSTR(temp); Enter(1);
    /* Protocol    : */
    pout(  CYAN, BLACK, (char *) Language(351)); snprintf(temp, PATH_MAX, "%s", sProtName); PUTSTR(temp); Enter(1);

    Syslog('+', "Download files start, protocol: %s", sProtName);

    PUTSTR(sProtAdvice);
    Enter(2);

    /*
     * Wait a while before download
     */
    sleep(2);
    
    if (uProtInternal) {
	snprintf(temp, PATH_MAX, "%s/%s/tag", CFG.bbs_usersdir, exitinfo.Name);
	chdir(temp);
	if (strncasecmp(sProtName, "zmodem-8k", 9) == 0) {
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

	snprintf(temp, PATH_MAX, "%s/%s/tag", CFG.bbs_usersdir, exitinfo.Name);
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

	purgeline(200); /* Wait a while, some Wintendo programs ignore input for a few seconds */
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
		snprintf(symTo, PATH_MAX, "./tag/%s", tmpf->remote);
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
	Syslog('+', "Download %s in %d file(s)", transfertime(starttime, endtime, (unsigned int)Size, TRUE), Count);
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
    unsigned int    Size = 0;
    int		    err, Count = 0, rc = 0, want1k = FALSE, wantg = FALSE;
    up_list	    *tmp, *ta;

    /*
     * If user has no default protocol, make sure he has one.
     */
    if (!ForceProtocol()) {
	return 1;
    }

    temp  = calloc(PATH_MAX, sizeof(char));

    /* Please start your upload now */
    snprintf(temp, PATH_MAX, "%s, %s", sProtAdvice, (char *) Language(283));
    pout(CFG.HiliteF, CFG.HiliteB, temp);
    Enter(2);
    Syslog('+', "Upload using %s", sProtName);

    snprintf(temp, PATH_MAX, "%s/%s/upl", CFG.bbs_usersdir, exitinfo.Name);

    if (chdir(temp)) {
	WriteError("$Can't chdir to %s", temp);
	free(temp);
	return 1;
    }
    sleep(2);

    if (uProtInternal) {
	if ((strncasecmp(sProtName, "zmodem", 6) == 0) || 
	    (strncasecmp(sProtName, "ymodem", 6) == 0) || 
	    (strncasecmp(sProtName, "xmodem", 6) == 0)) {

	    if (strncasecmp(sProtName, "zmodem", 6) == 0) {
		zmodem_requested = TRUE;
		protocol = ZM_ZMODEM;
	    } else {
		zmodem_requested = FALSE;
	    }
	    if (strncasecmp(sProtName, "ymodem", 6) == 0)
		protocol = ZM_YMODEM;
	    if (strncasecmp(sProtName, "xmodem", 6) == 0)
		protocol = ZM_XMODEM;

	    if (strstr(sProtName, "-1K") && (protocol != ZM_ZMODEM)) {
		Syslog('x', "%s: want 1K blocks", protname());
		want1k = TRUE;
	    }
	    if ((strstr(sProtName, "-G")) && (protocol == ZM_YMODEM)) {
		Syslog('x', "%s: want Ymodem-G", protname());
		wantg = TRUE;
	    }
	    rc = zmrcvfiles(want1k, wantg);

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
			    snprintf(temp, PATH_MAX, "%s/%s/upl/%s", CFG.bbs_usersdir, exitinfo.Name, dp->d_name);
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
		    snprintf(temp, PATH_MAX, "%s/%s/upl/%s", CFG.bbs_usersdir, exitinfo.Name, dp->d_name);
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
	    Syslog('+', "Upload %s in %d file(s)", transfertime(starttime, endtime, (unsigned int)Size, FALSE), Count);
	}
    }
    free(temp);
    Syslog('b', "Done, return rc=%d", rc);

    return rc;
}



char *transfertime(struct timeval start, struct timeval end, int bytes, int sent)
{
    static char     resp[81];
    double long	    startms, endms, elapsed;

    startms = (start.tv_sec * 1000) + (start.tv_usec / 1000);
    endms = (end.tv_sec * 1000) + (end.tv_usec / 1000);
    elapsed = endms - startms;
    memset(&resp, 0, sizeof(resp));
    if (!elapsed)
	elapsed = 1L;
    if (bytes > 1000000)
	snprintf(resp, 81, "%d bytes %s in %0.3Lf seconds (%0.3Lf Kb/s)",
		bytes, sent?"sent":"received", elapsed / 1000.000, ((bytes / elapsed) * 1000) / 1024);
    else
	snprintf(resp, 81, "%d bytes %s in %0.3Lf seconds (%0.3Lf Kb/s)", 
		bytes, sent?"sent":"received", elapsed / 1000.000, ((bytes * 1000) / elapsed) / 1024);   
    return resp;
}

