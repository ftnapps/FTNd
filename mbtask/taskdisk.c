/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Give status of all filesystems
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "taskdisk.h"
#include "taskutil.h"


int		    disk_reread = FALSE;	/* Reread tables	*/
extern int	    T_Shutdown;			/* Program shutdown	*/
int		    disk_run = FALSE;		/* Thread running	*/


typedef struct _mfs_list {
    struct _mfs_list	*next;			/* Linked list		*/
    char		*mountpoint;		/* Mountpoint		*/
    char		*fstype;		/* FS type		*/
    unsigned long	size;			/* Size in MB		*/
    unsigned long	avail;			/* Available in MB	*/
} mfs_list;

mfs_list	    *mfs = NULL;		/* List of filesystems	*/


/*
 * Internal prototypes
 */
void tidy_mfslist(mfs_list **);
int  in_mfslist(char *, mfs_list **);
void fill_mfslist(mfs_list **, char *, char *);
void update_diskstat(void);
void add_path(char *);



/*
 * Reset mounted filesystems array
 */
void tidy_mfslist(mfs_list **fap)
{
    mfs_list	*tmp, *old;

    for (tmp = *fap; tmp; tmp = old) {
	old = tmp->next;
	free(tmp->mountpoint);
	free(tmp);
    }
    *fap = NULL;
}



int in_mfslist(char *mountpoint, mfs_list **fap)
{
    mfs_list	*tmp;

    for (tmp = *fap; tmp; tmp = tmp->next)
	if (strcmp(tmp->mountpoint, mountpoint) == 0)
	    return TRUE;

    return FALSE;
}



void fill_mfslist(mfs_list **fap, char *mountpoint, char *fstype)
{
    mfs_list	*tmp;

    if (in_mfslist(mountpoint, fap))
	return;

    tmp = (mfs_list *)malloc(sizeof(mfs_list));
    tmp->next = *fap;
    tmp->mountpoint = xstrcpy(mountpoint);
    tmp->fstype = xstrcpy(fstype);
    tmp->size = 0L;
    tmp->avail = 0L;
    *fap = tmp;
}



/*
 * This function signals the diskwatcher to reread the tables.
 */
char *disk_reset(void)
{
    static char	buf[10];

    disk_reread = TRUE;
    sprintf(buf, "100:0;");
    return buf;
}



/*
 * Check free diskspace on all by mbse used filesystems.
 * The amount of needed space is given in MBytes.
 */
char *disk_check(char *token)
{
    static char	    buf[SS_BUFSIZE];
    mfs_list	    *tmp;
    unsigned long   needed;

    strtok(token, ",");
    needed = atol(strtok(NULL, ";"));
    Syslog('d', "disk_check(%ld)", needed);

    if (! needed) {
	/*
	 * Answer enough space
	 */
	sprintf(buf, "100:1,1");
	return buf;
    }

    if (mfs == NULL) {
	/*
	 * Answer Error
	 */
	sprintf(buf, "100:1,3");
	return buf;
    }

    for (tmp = mfs; tmp; tmp = tmp->next) {
	if (tmp->avail < needed) {
	    /*
	     * Answer Not enough space
	     */
	    sprintf(buf, "100:1,0");
	    return buf;
	}
    }

    /*
     * Enough space
     */
    sprintf(buf, "100:1,1");
    return buf;
}



/*
 * This function returns the information of all mounted filesystems,
 * but no more then 10 filesystems.
 */
char *disk_getfs()
{
    static char	    buf[SS_BUFSIZE];
    char	    tt[80], *ans = NULL;
    mfs_list	    *tmp;
    int		    i = 0;

    buf[0] = '\0';
    if (mfs == NULL) {
	sprintf(buf, "100:0;");
	return buf;
    }

    for (tmp = mfs; tmp; tmp = tmp->next) {
	i++;
	if (ans == NULL)
	    ans = xstrcpy((char *)",");
	else
	    ans = xstrcat(ans, (char *)",");
	tt[0] = '\0';
	sprintf(tt, "%lu %lu %s %s", tmp->size, tmp->avail, tmp->mountpoint, tmp->fstype);
	ans = xstrcat(ans, tt);
	if (i == 10) /* No more then 10 filesystems */
	    break;
    }

    if (strlen(ans) > (SS_BUFSIZE - 8))
	sprintf(buf, "100:0;");
    else
	sprintf(buf, "100:%d%s;", i, ans);

    if (ans != NULL)
	free(ans);

    return buf;
}



/*
 * Update disk useage status.
 */
void update_diskstat(void)
{
    struct statfs   sfs;
    unsigned long   temp;
    mfs_list        *tmp;

    for (tmp = mfs; tmp; tmp = tmp->next) {
        if (statfs(tmp->mountpoint, &sfs) == 0) {
            temp = (unsigned long)(sfs.f_bsize / 512L);
	    tmp->size  = (unsigned long)(sfs.f_blocks * temp) / 2048L;
	    tmp->avail = (unsigned long)(sfs.f_bavail * temp) / 2048L;
        }
    }
}



/*
 * Add path to table. 
 * Find out if the parameter is a path or a file on a path.
 */
void add_path(char *path)
{
    struct stat	    sb;
    char	    *p, *fsname, *fstype;
#if defined(__linux__)
    char	    *mtab, *fs;
    FILE	    *fp;
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    struct statfs   *mntbuf;
    long	    mntsize;
    int		    i;
#endif

    if (strlen(path) == 0) {
	Syslog('d', "add_path() empty pathname");
	return;
    }

    Syslog('d', "add_path(%s)", path);
    if (lstat(path, &sb) == 0) {
	if (S_ISDIR(sb.st_mode)) {

#if defined(__linux__)
	    if ((fp = fopen((char *)"/etc/mtab", "r"))) {
		mtab = calloc(PATH_MAX, sizeof(char));
		fsname = calloc(PATH_MAX, sizeof(char));
		fstype = calloc(PATH_MAX, sizeof(char));
		fsname[0] = '\0';

		while (fgets(mtab, PATH_MAX -1, fp)) {
		    fs = strtok(mtab, " \t");
		    fs = strtok(NULL, " \t");
		    /*
		     * Overwrite fsname each time a match is found, the last
		     * mounted filesystem that matches must be the one we seek.
		     */
		    if (strncmp(fs, path, strlen(fs)) == 0) {
			Syslog('d', "Found fs %s", fs);
			sprintf(fsname, "%s", fs);
			fs = strtok(NULL, " \t");
			sprintf(fstype, "%s", fs);
		    }
		}
		fclose(fp);
		free(mtab);
		Syslog('d', "Should be on \"%s\" \"%s\"", fsname, fstype);
		fill_mfslist(&mfs, fsname, fstype);
		free(fsname);
		free(fstype);
	    }

#elif defined(__FreeBSD__) || defined(__NetBSD__)

	    if ((mntsize = getmntinfo(&mntbuf, MNT_NOWAIT))) {
		fsname = calloc(PATH_MAX, sizeof(char));
		fstype = calloc(PATH_MAX, sizeof(char));

		for (i = 0; i < mntsize; i++) {
		    Syslog('d', "Check fs %s", mntbuf[i].f_mntonname);
		    if (strncmp(mntbuf[i].f_mntonname, path, strlen(mntbuf[i].f_mntonname)) == 0) {
			Syslog('d', "Found fs %s", mntbuf[i].f_mntonname);
			sprintf(fsname, "%s", mntbuf[i].f_mntonname);
			sprintf(fstype, "%s", mntbuf[i].f_fstypename);
		    }
		}
		Syslog('d', "Should be on \"%s\" \"%s\"", fsname, fstype);
		fill_mfslist(&mfs, fsname);
		free(fsname);
		free(fstype);
	    }

#endif

	} else {
	    Syslog('d', "****  Is not a dir");
	}
    } else {
	/*
	 * Possible cause, we were given a filename that
	 * may not be present because this is a temporary file.
	 * Strip the last part of the name and try again.
	 */
	if ((p = strrchr(path, '/')))
	    *p = '\0';
	Syslog('d', "Recursive add name %s", path);
	add_path(path);
    }
}



/*
 * Diskwatch thread
 */
void *disk_thread(void)
{
    FILE	*fp;
    char	*temp;
    mfs_list	*tmp;

    Syslog('+', "Start disk thread");
    disk_run = TRUE;
    disk_reread = TRUE;
    
    while (! T_Shutdown) {

	if (disk_reread) {
	    disk_reread = FALSE;
	    Syslog('+', "Reread disk filesystems");
	    tidy_mfslist(&mfs);

	    add_path(getenv("MBSE_ROOT"));
	    add_path(CFG.bbs_menus);
	    add_path(CFG.bbs_txtfiles);
	    add_path(CFG.alists_path);
	    add_path(CFG.req_magic);
	    add_path(CFG.bbs_usersdir);
	    add_path(CFG.nodelists);
	    add_path(CFG.inbound);
	    add_path(CFG.pinbound);
	    add_path(CFG.outbound);
	    add_path(CFG.ftp_base);
	    add_path(CFG.bbs_macros);
	    add_path(CFG.out_queue);
	    add_path(CFG.rulesdir);
	    add_path(CFG.tmailshort);
	    add_path(CFG.tmaillong);
	
	    temp = calloc(PATH_MAX, sizeof(char ));
	    sprintf(temp, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	    if ((fp = fopen(temp, "r"))) {
		Syslog('d', "+ %s", temp);
		fread(&areahdr, sizeof(areahdr), 1, fp);
		fseek(fp, areahdr.hdrsize, SEEK_SET);

		while (fread(&area, areahdr.recsize, 1, fp)) {
		    if (area.Available) { 
			if (area.CDrom)
			    add_path(area.FilesBbs);
			else
			    add_path(area.Path);
		    }
		}
		fclose(fp);
	    }

	    sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	    if ((fp = fopen(temp, "r"))) {
		Syslog('d', "+ %s", temp);
		fread(&msgshdr, sizeof(msgshdr), 1, fp);
		fseek(fp, msgshdr.hdrsize, SEEK_SET);

		while (fread(&msgs, msgshdr.recsize, 1, fp)) {
		    if (msgs.Active)
			add_path(msgs.Base);
		    fseek(fp, msgshdr.syssize, SEEK_CUR);
		}
		fclose(fp);
	    }

	    sprintf(temp, "%s/etc/language.data", getenv("MBSE_ROOT"));
	    if ((fp = fopen(temp, "r"))) {
		Syslog('d', "+ %s", temp);
		fread(&langhdr, sizeof(langhdr), 1, fp);
		fseek(fp, langhdr.hdrsize, SEEK_SET);

		while (fread(&lang, langhdr.recsize, 1, fp)) {
		    if (lang.Available) {
			add_path(lang.MenuPath);
			add_path(lang.TextPath);
			add_path(lang.MacroPath);
		    }
		}
		fclose(fp);
	    }

	    sprintf(temp, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
	    if ((fp = fopen(temp, "r"))) {
		Syslog('d', "+ %s", temp);
		fread(&nodeshdr, sizeof(nodeshdr), 1, fp);
		fseek(fp, nodeshdr.hdrsize, SEEK_SET);

		while (fread(&nodes, nodeshdr.recsize, 1, fp)) {
		    if (nodes.Session_in == S_DIR)
			add_path(nodes.Dir_in_path);
		    if (nodes.Session_out == S_DIR)
			add_path(nodes.Dir_out_path);
		    add_path(nodes.OutBox);
		    fseek(fp, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
		}
		fclose(fp);
	    }

	    sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	    if ((fp = fopen(temp, "r"))) {
		Syslog('d', "+ %s", temp);
		fread(&fgrouphdr, sizeof(fgrouphdr), 1, fp);
		fseek(fp, fgrouphdr.hdrsize, SEEK_SET);

		while (fread(&fgroup, fgrouphdr.recsize, 1, fp)) {
		    if (fgroup.Active)
			add_path(fgroup.BasePath);
		}
		fclose(fp);
	    }
	    
	    sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	    if ((fp = fopen(temp, "r"))) {
		Syslog('d', "+ %s", temp);
		fread(&mgrouphdr, sizeof(mgrouphdr), 1, fp);
		fseek(fp, mgrouphdr.hdrsize, SEEK_SET);

		while (fread(&mgroup, mgrouphdr.recsize, 1, fp)) {
		    if (mgroup.Active)
			add_path(mgroup.BasePath);
		}
		fclose(fp);
	    }

	    sprintf(temp, "%s/etc/hatch.data", getenv("MBSE_ROOT"));
	    if ((fp = fopen(temp, "r"))) {
		Syslog('d', "+ %s", temp);
		fread(&hatchhdr, sizeof(hatchhdr), 1, fp);
		fseek(fp, hatchhdr.hdrsize, SEEK_SET);

		while (fread(&hatch, hatchhdr.recsize, 1, fp)) {
		    if (hatch.Active)
			add_path(hatch.Spec);
		}
		fclose(fp);
	    }

	    sprintf(temp, "%s/etc/magic.data", getenv("MBSE_ROOT"));
	    if ((fp = fopen(temp, "r"))) {
		Syslog('d', "+ %s", temp);
		fread(&magichdr, sizeof(magichdr), 1, fp);
		fseek(fp, magichdr.hdrsize, SEEK_SET);

		while (fread(&magic, magichdr.recsize, 1, fp)) {
		    if (magic.Active && ((magic.Attrib == MG_COPY) || (magic.Attrib == MG_UNPACK)))
			add_path(magic.Path);
		}
		fclose(fp);
	    }
	    free(temp);
	    Syslog('d', "All directories added");

	    for (tmp = mfs; tmp; tmp = tmp->next) {
		Syslog('+', "%s %s %ld %ld", tmp->mountpoint, tmp->fstype, tmp->size, tmp->avail);
	    }
	}

	update_diskstat();
	sleep(1);
    }

    disk_run = FALSE;
    Syslog('+', "Disk thread stopped");
    pthread_exit(NULL);
}


