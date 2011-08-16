/*****************************************************************************
 *
 * $Id: taskdisk.c,v 1.35 2007/08/26 14:02:28 mbse Exp $
 * Purpose ...............: Give status of all filesystems
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


typedef struct _mfs_list {
    struct _mfs_list	*next;			/* Linked list		*/
    char		*mountpoint;		/* Mountpoint		*/
    char		*fstype;		/* FS type		*/
    unsigned int	size;			/* Size in MB		*/
    unsigned int	avail;			/* Available in MB	*/
    unsigned		ro		: 1;	/* Read-Only fs.	*/
} mfs_list;

mfs_list	    *mfs = NULL;		/* List of filesystems	*/
int                 disk_reread = TRUE;		/* Reread tables        */
int		    recursecount = 0;		/* Recurse counter	*/



/*
 * Internal prototypes
 */
char *mbt_realpath(const char *, char *);
void tidy_mfslist(mfs_list **);
int  in_mfslist(char *, mfs_list **);
void fill_mfslist(mfs_list **, char *, char *);
void update_diskstat(void);
void add_path(char *);


/*
 * realpath.c -- canonicalize pathname by removing symlinks
 * Copyright (C) 1993 Rick Sladkey <jrs@world.std.com>
 */

#define HAVE_GETCWD

/*
 * This routine is part of libc.  We include it nevertheless,
 * since the libc version has some security flaws.
 */
#define MAX_READLINKS 32

char *mbt_realpath(const char *path, char *resolved_path)
{
    char copy_path[PATH_MAX], link_path[PATH_MAX], *new_path = resolved_path, *max_path;
    int readlinks = 0, n;

    /* Make a copy of the source path since we may need to modify it. */
    if (strlen(path) >= PATH_MAX) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    strcpy(copy_path, path);
    path = copy_path;
    max_path = copy_path + PATH_MAX - 2;

    /* If it's a relative pathname use getwd for starters. */
    if (*path != '/') {
        getcwd(new_path, PATH_MAX - 1);
        new_path += strlen(new_path);
        if (new_path[-1] != '/')
            *new_path++ = '/';
    } else {
        *new_path++ = '/';
        path++;
    }
    /* Expand each slash-separated pathname component. */
    while (*path != '\0') {
	/* Ignore stray "/". */
        if (*path == '/') {
            path++;
            continue;
        }
        if (*path == '.') {
            /* Ignore ".". */
            if (path[1] == '\0' || path[1] == '/') {
                path++;
                continue;
            }
            if (path[1] == '.') {
                if (path[2] == '\0' || path[2] == '/') {
                    path += 2;
                    /* Ignore ".." at root. */
                    if (new_path == resolved_path + 1)
                        continue;
                    /* Handle ".." by backing up. */
                    while ((--new_path)[-1] != '/')
                        ;
                    continue;
                }
            }
        }
        /* Safely copy the next pathname component. */
        while (*path != '\0' && *path != '/') {
            if (path > max_path) {
                errno = ENAMETOOLONG;
                return NULL;
            }
            *new_path++ = *path++;
        }
#ifdef S_IFLNK
        /* Protect against infinite loops. */
        if (readlinks++ > MAX_READLINKS) {
            errno = ELOOP;
            return NULL;
        }
        /* See if latest pathname component is a symlink. */
        *new_path = '\0';
        n = readlink(resolved_path, link_path, PATH_MAX - 1);
        if (n < 0) {
            /* EINVAL means the file exists but isn't a symlink. */
            if (errno != EINVAL)
                return NULL;
        } else {
            /* Note: readlink doesn't add the null byte. */
            link_path[n] = '\0';
            if (*link_path == '/')
                /* Start over for an absolute symlink. */
                new_path = resolved_path;
            else
                /* Otherwise back up over this component. */
                while (*(--new_path) != '/')
                    ;
            /* Safe sex check. */
            if (strlen(path) + n >= PATH_MAX) {
                errno = ENAMETOOLONG;
                return NULL;
	    }
	    /* Insert symlink contents into path. */
            strcat(link_path, path);
            strcpy(copy_path, link_path);
            path = copy_path;
        }
#endif /* S_IFLNK */
        *new_path++ = '/';
    }
    /* Delete trailing slash but don't whomp a lone slash. */
    if (new_path != resolved_path + 1 && new_path[-1] == '/')
        new_path--;
    /* Make sure it's null terminated. */
    *new_path = '\0';
    return resolved_path;
}



/*
 * Reset mounted filesystems array
 */
void tidy_mfslist(mfs_list **fap)
{
    mfs_list	*tmp, *old;

    for (tmp = *fap; tmp; tmp = old) {
	old = tmp->next;
	free(tmp->mountpoint);
	free(tmp->fstype);
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
    tmp->ro = TRUE; // Read-only, statfs will set real value.
    *fap = tmp;
}



/*
 * This function signals the diskwatcher to reread the tables.
 */
char *disk_reset(void)
{
    disk_reread = TRUE;
    return (char *)"100:0;";
}



/*
 * Check free diskspace on all by mbse used filesystems.
 * The amount of needed space is given in MBytes.
 */
void disk_check_r(char *token, char *buf)
{
    mfs_list	    *tmp;
    unsigned int    needed, lowest = 0xffffffff;

    strtok(token, ",");
    needed = atol(strtok(NULL, ";"));

    if (mfs == NULL) {
	/*
	 * Answer Error
	 */
	snprintf(buf, SS_BUFSIZE, "100:1,3");
	return;
    }

    for (tmp = mfs; tmp; tmp = tmp->next) {
	if (!tmp->ro && (tmp->avail < lowest))
	    lowest = tmp->avail;
    }

    if (lowest < needed) {
	snprintf(buf, SS_BUFSIZE, "100:2,0,%d;", lowest);
    } else {
	snprintf(buf, SS_BUFSIZE, "100:2,1,%d;", lowest);
    }
    return;
}



/*
 * This function returns the information of all mounted filesystems,
 * but no more then 10 filesystems.
 */
void disk_getfs_r(char *buf)
{
    char	    tt[80], *ans = NULL;
    mfs_list	    *tmp;
    int		    i = 0;

    buf[0] = '\0';
    if (mfs == NULL) {
	snprintf(buf, SS_BUFSIZE, "100:0;");
	return;
    }

    for (tmp = mfs; tmp; tmp = tmp->next) {
	i++;
	if (ans == NULL)
	    ans = xstrcpy((char *)",");
	else
	    ans = xstrcat(ans, (char *)",");
	tt[0] = '\0';
	snprintf(tt, 80, "%u %u %s %s %d", tmp->size, tmp->avail, tmp->mountpoint, tmp->fstype, tmp->ro);
	ans = xstrcat(ans, tt);
	if (i == 10) /* No more then 10 filesystems */
	    break;
    }

    if (strlen(ans) > (SS_BUFSIZE - 8))
	snprintf(buf, SS_BUFSIZE, "100:0;");
    else
	snprintf(buf, SS_BUFSIZE, "100:%d%s;", i, ans);

    if (ans != NULL)
	free(ans);
	ans = NULL;

    return;
}



/*
 * Update disk useage status.
 */
void update_diskstat(void)
{
#ifdef	__NetBSD__
    struct statvfs   sfs;
#else
    struct statfs   sfs;
#endif
    unsigned int    temp;
    mfs_list        *tmp;

    for (tmp = mfs; tmp; tmp = tmp->next) {
#ifdef	__NetBSD__
        if (statvfs(tmp->mountpoint, &sfs) == 0) {
#else
        if (statfs(tmp->mountpoint, &sfs) == 0) {
#endif
            temp = (unsigned int)(sfs.f_bsize / 512L);
	    tmp->size  = (unsigned int)(sfs.f_blocks * temp) / 2048L;
	    tmp->avail = (unsigned int)(sfs.f_bavail * temp) / 2048L;
#if defined(__linux__)
	    /*
	     * The struct statfs (or statvfs) seems to have no information
	     * about read-only filesystems, so we guess it based on fstype.
	     * See man 2 statvfs about what approximately is defined.
	     */
	    tmp->ro = (strstr(tmp->fstype, "iso") != NULL);
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	    /*
	     * XxxxBSD has the info in the statfs structure.
	     */
	    tmp->ro = (sfs.f_flags & MNT_RDONLY);
#elif defined(__NetBSD__)
	    /*
	     * NetBSD 3.1 is a bit different again
	     */
	    tmp->ro = (sfs.f_flag & ST_RDONLY);
#else
#error "Don't know how to get sfs read-only status"
#endif
        }
    }
}



/*
 * Add path to table. 
 * Find out if the parameter is a path or a file on a path.
 */
void add_path(char *lpath)
{
    struct stat	    sb;
    char	    *p, fsname[PATH_MAX], fstype[PATH_MAX], *rpath;
#if defined(__linux__)
    char	    *mtab, *fs;
    FILE	    *fp;
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
    struct statfs   *mntbuf;
    int		    mntsize;
    int		    i;
#elif defined(__NetBSD__)
    struct statvfs  *mntbuf;
    int             mntsize;
    int             i;
#else
#error "Don't know how to get mount paths"
#endif

    if (strlen(lpath) == 0) {
	return;
    }

    /*
     * Safety recursive call count.
     */
    recursecount++;
    if (recursecount > 2) {
	Syslog('!', "add_path(%s), too many recursive calls", lpath);
	recursecount--;
	return;
    }

    rpath = calloc(PATH_MAX, sizeof(char));
    mbt_realpath(lpath, rpath);

    if (lstat(rpath, &sb) == 0) {

	if (S_ISDIR(sb.st_mode)) {

#if defined(__linux__)

	    if ((fp = fopen((char *)"/etc/mtab", "r"))) {
		mtab = calloc(PATH_MAX, sizeof(char));
		fsname[0] = '\0';

		while (fgets(mtab, PATH_MAX -1, fp)) {
		    fs = strtok(mtab, " \t");
		    fs = strtok(NULL, " \t");
		    /*
		     * Overwrite fsname each time a match is found, the last
		     * mounted filesystem that matches must be the one we seek.
		     */
		    if (strncmp(fs, rpath, strlen(fs)) == 0) {
			snprintf(fsname, PATH_MAX, "%s", fs);
			fs = strtok(NULL, " \t");
			snprintf(fstype, PATH_MAX, "%s", fs);
		    }
		}
		fclose(fp);
		free(mtab);
		fill_mfslist(&mfs, fsname, fstype);
	    }

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)

	    if ((mntsize = getmntinfo(&mntbuf, MNT_NOWAIT))) {

		for (i = 0; i < mntsize; i++) {
		    if (strncmp(mntbuf[i].f_mntonname, rpath, strlen(mntbuf[i].f_mntonname)) == 0) {
			snprintf(fsname, PATH_MAX, "%s", mntbuf[i].f_mntonname);
			snprintf(fstype, PATH_MAX, "%s", mntbuf[i].f_fstypename);
		    }
		}
		fill_mfslist(&mfs, fsname, fstype);
	    }

#else
#error "Unknown OS - don't know what to do"
#endif

	} else {
	    /*
	     * Not a directory, maybe a file.
	     */
	    if ((p = strrchr(rpath, '/')))
		*p = '\0';
	    add_path(rpath);
	}
    } else {
	/*
	 * Possible cause, we were given a filename that
	 * may not be present because this is a temporary file.
	 * Strip the last part of the name and try again.
	 */
	if ((p = strrchr(rpath, '/')))
	    *p = '\0';
	add_path(rpath);
    }
    recursecount--;
	free(rpath);
}



/*
 * Diskwatch 
 */
void diskwatch(void)
{
    FILE	*fp;
    char	*temp = NULL;
    mfs_list	*tmp;

    if (disk_reread) {
        disk_reread = FALSE;
        Syslog('+', "Reread disk filesystems");

        tidy_mfslist(&mfs);

        add_path(getenv("MBSE_ROOT"));
        add_path(CFG.alists_path);
        add_path(CFG.req_magic);
        add_path(CFG.bbs_usersdir);
        add_path(CFG.nodelists);
        add_path(CFG.inbound);
        add_path(CFG.pinbound);
        add_path(CFG.outbound);
        add_path(CFG.ftp_base);
        add_path(CFG.out_queue);
        add_path(CFG.rulesdir);
        add_path(CFG.tmailshort);
        add_path(CFG.tmaillong);
	
        temp = calloc(PATH_MAX, sizeof(char ));
        snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
        if ((fp = fopen(temp, "r"))) {
	    fread(&areahdr, sizeof(areahdr), 1, fp);
	    fseek(fp, areahdr.hdrsize, SEEK_SET);

	    while (fread(&area, areahdr.recsize, 1, fp)) {
	        if (area.Available) { 
		    add_path(area.Path);
		}
	    }
	    fclose(fp);
	}

	snprintf(temp, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
        if ((fp = fopen(temp, "r"))) {
	    fread(&msgshdr, sizeof(msgshdr), 1, fp);
	    fseek(fp, msgshdr.hdrsize, SEEK_SET);

	    while (fread(&msgs, msgshdr.recsize, 1, fp)) {
	        if (msgs.Active)
		    add_path(msgs.Base);
		fseek(fp, msgshdr.syssize, SEEK_CUR);
	    }
	    fclose(fp);
	}

	snprintf(temp, PATH_MAX, "%s/etc/language.data", getenv("MBSE_ROOT"));
        if ((fp = fopen(temp, "r"))) {
	    fread(&langhdr, sizeof(langhdr), 1, fp);
	    fseek(fp, langhdr.hdrsize, SEEK_SET);

	    while (fread(&lang, langhdr.recsize, 1, fp)) {
	        if (lang.Available) {
		    snprintf(temp, PATH_MAX, "%s/share/int/menus/%s", getenv("MBSE_ROOT"), lang.lc);
		    add_path(temp);
		    snprintf(temp, PATH_MAX, "%s/share/int/txtfiles/%s", getenv("MBSE_ROOT"), lang.lc);
		    add_path(temp);
		    snprintf(temp, PATH_MAX, "%s/share/int/macro/%s", getenv("MBSE_ROOT"), lang.lc);
		    add_path(temp);
		}
	    }
	    fclose(fp);
	}

	snprintf(temp, PATH_MAX, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
        if ((fp = fopen(temp, "r"))) {
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

	snprintf(temp, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
        if ((fp = fopen(temp, "r"))) {
	    fread(&fgrouphdr, sizeof(fgrouphdr), 1, fp);
	    fseek(fp, fgrouphdr.hdrsize, SEEK_SET);

	    while (fread(&fgroup, fgrouphdr.recsize, 1, fp)) {
	        if (fgroup.Active)
		    add_path(fgroup.BasePath);
	    }
	    fclose(fp);
	}
	    
	snprintf(temp, PATH_MAX, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
        if ((fp = fopen(temp, "r"))) {
	    fread(&mgrouphdr, sizeof(mgrouphdr), 1, fp);
	    fseek(fp, mgrouphdr.hdrsize, SEEK_SET);

	    while (fread(&mgroup, mgrouphdr.recsize, 1, fp)) {
	        if (mgroup.Active)
		    add_path(mgroup.BasePath);
	    }
	    fclose(fp);
	}

	snprintf(temp, PATH_MAX, "%s/etc/hatch.data", getenv("MBSE_ROOT"));
	if ((fp = fopen(temp, "r"))) {
	    fread(&hatchhdr, sizeof(hatchhdr), 1, fp);
	    fseek(fp, hatchhdr.hdrsize, SEEK_SET);

	    while (fread(&hatch, hatchhdr.recsize, 1, fp)) {
	        if (hatch.Active)
		    add_path(hatch.Spec);
	    }
	    fclose(fp);
	}

	snprintf(temp, PATH_MAX, "%s/etc/magic.data", getenv("MBSE_ROOT"));
        if ((fp = fopen(temp, "r"))) {
	    fread(&magichdr, sizeof(magichdr), 1, fp);
	    fseek(fp, magichdr.hdrsize, SEEK_SET);

	    while (fread(&magic, magichdr.recsize, 1, fp)) {
	        if (magic.Active && ((magic.Attrib == MG_COPY) || (magic.Attrib == MG_UNPACK)))
		    add_path(magic.Path);
	    }
	    fclose(fp);
	}
	free(temp);
	temp = NULL;

	/*
         * Now update the new table with filesystems information.
         */
        update_diskstat();
	for (tmp = mfs; tmp; tmp = tmp->next) {
	    Syslog('+', "Found filesystem: %s type: %s status: %s", tmp->mountpoint, tmp->fstype, tmp->ro ?"RO":"RW");
	}
    } else {
	update_diskstat();
    }
}



void deinit_diskwatch()
{
    Syslog('d', "De-init diskwatch done");
    tidy_mfslist(&mfs);
}


