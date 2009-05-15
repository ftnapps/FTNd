/*****************************************************************************
 *
 * $Id: queue.c,v 1.34 2007/08/26 12:21:16 mbse Exp $
 * Purpose ...............: Mail and file queue operations
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
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "fsort.h"
#include "dirsession.h"
#include "queue.h"


static char *dow[] = {(char *)"su", (char *)"mo", (char *)"tu", (char *)"we",
                      (char *)"th", (char *)"fr", (char *)"sa"};


extern	int	do_quiet;
extern	int	flushed;
extern	pid_t	mypid;


/*
 *  Internal version of basename to make this better portable.
 */
char *Basename(char *str)
{
    char *cp = strrchr(str, '/');

    return cp ? cp+1 : str;
}



/*
 * Flush one queue directory of a node. If everything is successfull the
 * directory will become empty.
 */
void flush_dir(char *);
void flush_dir(char *ndir)
{
    struct dirent   *de;
    DIR		    *dp;
    FILE	    *fp, *inf, *ouf;
    faddr	    noden, *bestaka;
    fidoaddr	    nodenr;
    int		    flavor, mode, Attach, fage, first, bread, rc;
    int		    fsize;
    char	    *p, *temp, *fname, *arcfile, *pktfile, *ext, maxnr, nr, oldnr, *buf;
    time_t	    Now;
    struct tm	    *ptm;
    struct stat	    sbuf;
    fd_list	    *fdl = NULL;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/%s", CFG.out_queue, ndir);
    if (chdir(temp) == -1) {
	WriteError("$Error chdir to %s", temp);
	free(temp);
	return;
    }

    /*
     * Get the nodenumber from the filename
     */
    noden.domain = NULL;
    noden.name   = NULL;
    noden.zone   = atoi(strtok(ndir, "."));
    noden.net    = atoi(strtok(NULL, "."));
    noden.node   = atoi(strtok(NULL, "."));
    noden.point  = atoi(strtok(NULL, "\0"));
    if (SearchFidonet(noden.zone))
	noden.domain = xstrcpy(fidonet.domain);

    memset(&nodenr, 0, sizeof(nodenr));
    nodenr.zone  = noden.zone;
    nodenr.net   = noden.net;
    nodenr.node  = noden.node;
    nodenr.point = noden.point;
    snprintf(nodenr.domain, 13, "%s", noden.domain);

    if (!SearchNode(nodenr)) {
	/*
	 * Node not in setup, blank noderecord and fill in some details
	 * so that we are able to send mail crash or immediate.
	 */
	Syslog('+', "Node %s not in setup, using default settings", aka2str(nodenr));
	memset(&nodes, 0, sizeof(nodes));
	nodes.Aka[0].zone  = noden.zone;
	nodes.Aka[0].net   = noden.net;
	nodes.Aka[0].node  = noden.node;
	nodes.Aka[0].point = noden.point;
	snprintf(nodes.Archiver, 6, (char *)"ZIP");
    }

    /*
     * If we route via another aka, change everything.
     */
    if (nodes.RouteVia.zone) {
	p = xstrcpy(aka2str(nodenr));
	Syslog('+', "Route to %s via %s", p, aka2str(nodes.RouteVia));
	free(p);
	noden.zone   = nodes.RouteVia.zone;
	noden.net    = nodes.RouteVia.net;
	noden.node   = nodes.RouteVia.node;
	noden.point  = nodes.RouteVia.point;
	if (noden.domain)
	    free(noden.domain);
	noden.domain = xstrcpy(nodes.RouteVia.domain);
	/*
	 * Load routevia noderecord to get the correct flavor.
	 * If there is no noderecord, reload the old one.
	 */
	if (!SearchNode(nodes.RouteVia))
	    SearchNode(nodenr);
    }

    /*
     * At this point we are ready to add everything to the real outbound.
     * Lock the node, if this fails because the node is busy we abort
     * and try to add at a later time.
     */
    if (nodes.Session_out == S_DIR) {
	if (islocked(nodes.Dir_out_clock, nodes.Dir_out_chklck, nodes.Dir_out_waitclr, 'p')) {
	    Syslog('+', "Mail and files stay in queue, will be added later");
	    if (noden.domain)
		free(noden.domain);
	    free(temp);
	    return;
	} else {
	    if (! setlock(nodes.Dir_out_mlock, nodes.Dir_out_mklck, 'p')) {
		Syslog('+', "Mail and files stay in queue, will be added later");
		if (noden.domain)
		    free(noden.domain);
		free(temp);
		return;
	    }
	}
    } else {
	if (nodelock(&noden, mypid)) {
	    Syslog('+', "Mail and files stay in queue, will be added later");
	    if (noden.domain)
		free(noden.domain);
	    free(temp);
	    return;
	}
    }

    /*
     * Create arcmail filename for this node.
     */
    Now = time(NULL);
    arcfile = calloc(PATH_MAX, sizeof(char));
    if (nodes.Session_out == S_DIR) {

	ptm = localtime(&Now);
	ext = dow[ptm->tm_wday];

	if (!nodes.ARCmailCompat && (nodes.Aka[0].zone != noden.zone)) {
	    /*
	     * Generate ARCfile name from the CRC of the ASCII string of the node address.
	     */
	    snprintf(arcfile, PATH_MAX, "%s/%08x.%s0", nodes.Dir_out_path, StringCRC32(ascfnode(&noden, 0x1f)), ext);
	} else {
	    bestaka = bestaka_s(&noden);

	    if (noden.point) {
		snprintf(arcfile, PATH_MAX, "%s/%04x%04x.%s0", nodes.Dir_out_path, ((bestaka->net) - (noden.net)) & 0xffff,
			((bestaka->node) - (noden.node) + (noden.point)) & 0xffff, ext);
	    } else if (bestaka->point) {
		/*
		 * Inserted the next code for if we are a point,
		 * I hope this is ARCmail 0.60 compliant. 21-May-1999
		 */
		snprintf(arcfile, PATH_MAX, "%s/%04x%04x.%s0", nodes.Dir_out_path, ((bestaka->net) - (noden.net)) & 0xffff,
			((bestaka->node) - (noden.node) - (bestaka->point)) & 0xffff, ext);
	    } else {
		snprintf(arcfile, PATH_MAX, "%s/%04x%04x.%s0", nodes.Dir_out_path, ((bestaka->net) - (noden.net)) & 0xffff,
			((bestaka->node) - (noden.node)) &0xffff, ext);
	    }
	}
    } else {
	snprintf(arcfile, PATH_MAX, "%s", arcname(&noden, nodes.Aka[0].zone, nodes.ARCmailCompat));
    }

    /*
     * If there is a mailpkt.qqq file, close it and rename it.
     */
    pktfile = calloc(PATH_MAX, sizeof(char));
    fname   = calloc(PATH_MAX, sizeof(char));
    snprintf(fname, PATH_MAX, "%s/mailpkt.qqq", temp);
    if (access(fname, W_OK) == 0) {
	snprintf(pktfile, PATH_MAX, "%s/%08x.pkt", temp, sequencer());
	if (rename(fname, pktfile)) {
	    WriteError("$Can't rename %s to %s", fname, pktfile);
	} else {
	    /*
	     * Add zero word to end of .pkt file
	     */
	    if ((fp = fopen(pktfile, "a+")) == NULL) {
		WriteError("$Can't open %s", pktfile);
	    }
	    putc('\0', fp);
	    putc('\0', fp);
	    fsync(fileno(fp));
	    fclose(fp);
	}
    }
    free(fname);

    /*
     * Now all mail pkts are ready to archive
     */
    if ((dp = opendir(temp)) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	free(arcfile);
	if (nodes.Session_out == S_DIR)
	    remlock(nodes.Dir_out_mlock, nodes.Dir_out_mklck, 'p');
	else
	    nodeulock(&noden, mypid);
	if (noden.domain)
	    free(noden.domain);
	return;
    }

    /*
     * Read all .pkt filenames, get the timestamp and add them
     * to the memory array for later sort on filedate.
     */ 
    while ((de = readdir(dp)))
	if ((strlen(de->d_name) == 12) && (strncasecmp(de->d_name+8,".pkt",4) == 0)) {
	    stat(de->d_name, &sbuf);
	    Syslog('p', "Adding %s to filelist", de->d_name);
	    fill_fdlist(&fdl, de->d_name, sbuf.st_mtime);
	}

    closedir(dp);
    sort_fdlist(&fdl);

    if (getarchiver(nodes.Archiver)) {
	flavor = 'f';
	if (nodes.Crash)
	    flavor = 'c';
	if (nodes.Hold)
	    flavor = 'h';
    } else {
	WriteError("Archiver %s not found", nodes.Archiver);
	if (noden.domain)
	    free(noden.domain);
	free(temp);
	free(arcfile);
	return;
    }

    first = TRUE;
    while ((fname = pull_fdlist(&fdl)) != NULL) {
	/*
	 *  Check the size of the existing archive if there is a size limit.
	 *  Change to new archive names if the existing is too large.
	 *  If the archive size is zero, it's an already sent archive, the
	 *  number will be bumped also.
	 *  If the archive is older then 6 days, the name is also bumped.
	 *  Do this until we find a new name or if the last digit is a '9' or 'z'.
	 *  Purge archives older then toss_days.
	 */
	nr = oldnr = '0';
	if (nodes.ARCmailAlpha)
	    maxnr = 'z';
	else
	    maxnr = '9';
	Attach = FALSE;

	for (;;) {
	    fsize = file_size(arcfile);
	    fage  = (int)((Now - file_time(arcfile)) / 86400);

	    if (fsize == -1L) {
		Attach = TRUE;
		break;
	    }

	    if (fsize == 0L) {
		if ((fage > 6) && (nr < maxnr)) {
		    /*
		     * Remove truncated ARCmail files older then 6 days.
		     */
		    unlink(arcfile);
		    fsize = -1L;
		    Attach = TRUE;
		    break;
		}

		/*
		 * Increase filename extension if there is a truncated file of today.
		 */
		if (nr < maxnr) {
		    nr++;
		    if (nr == ('9' +1))
			nr = 'a';
		    arcfile[strlen(arcfile) -1] = nr;
		} else {
		    Syslog('!', "Warning: archive filename extensions exhausted for today");
		    break;
		}
	    } else if (CFG.maxarcsize && (fsize > (CFG.maxarcsize * 1024)) && (nr < maxnr)) {
		/*
		 * Use a new ARCmail file if the last one is too big.
		 */
		nr++;
		if (nr == ('9' +1))
		    nr = 'a';
		arcfile[strlen(arcfile) -1] = nr;
	    }

	    fsize = file_size(arcfile);
	    fage  = (int)((Now - file_time(arcfile)) / 86400);

	    if ((fsize > 0L) && (fage > 6) && (nr < maxnr)) {
		/*
		 * If there is ARCmail of a week old or older, add mail
		 * to a new ARCmail bundle.
		 */
		nr++;
		if (nr == ('9' +1))
		    nr = 'a';
		arcfile[strlen(arcfile) -1] = nr;
	    }

	    if (oldnr == nr)
		break;
	    else
		oldnr = nr;
	}

	fsize = file_size(arcfile);

	/*
	 * If arcfile names were exhausted then the file ending on a z could still
	 * be in the outbound but truncated if it has been sent. Since we are
	 * reusing that filename (not a good solution) we must erase it or the
	 * archiver program will complain.
	 */
	if (fsize == 0L) {
	    unlink(arcfile);
	    Attach = TRUE;
	}

	if (first) {
	    Syslog('+', "Pack ARCmail for %s via %s with %s", aka2str(nodenr), ascfnode(&noden, 0x1f), nodes.Archiver);
	    if (!do_quiet) {
		printf("\rAdding ARCmail for %s                      ", ascfnode(&noden, 0x1f));
		fflush(stdout);
	    }
	    first = FALSE;
	    flushed = TRUE;
	}

	if (execute_str(archiver.marc, arcfile, fname, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null") == 0) {
	    unlink(fname);
	} else {
	    WriteError("Can't add %s to ARCmail archive", fname);
	    Attach = FALSE;
	}

	/*
	 * Change filemode so downlink has rights to the file.
	 */
	if (nodes.Session_out == S_DIR)
	    chmod(arcfile, 0660);
	
	/*
	 * Attach file to .flo, not for FTP or Directory sessions.
	 */
	if (Attach && nodes.Session_out == S_DIRECT)
	    attach(noden, arcfile, TFS, flavor);
    }

    /*
     * Open directory again.
     */
    if ((dp = opendir(temp)) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	free(arcfile);
	if (nodes.Session_out == S_DIR)
	    remlock(nodes.Dir_out_mlock, nodes.Dir_out_mklck, 'p');
	else
	    nodeulock(&noden, mypid);
	if (noden.domain)
	    free(noden.domain);
	return;
    }

    /*
     * Read all other mail filenames, get the timestamp and add them
     * to the memory array for later sort on filedate.
     */ 
    while ((de = readdir(dp)))
	if ((strlen(de->d_name) == 11) && (strncasecmp(de->d_name,"mailpkt.",8) == 0)) {
	    stat(de->d_name, &sbuf);
	    Syslog('p', "Adding %s to filelist", de->d_name);
	    fill_fdlist(&fdl, de->d_name, sbuf.st_mtime);
	}

    closedir(dp);
    sort_fdlist(&fdl);

    first = TRUE;
    while ((fname = pull_fdlist(&fdl)) != NULL) {
	if (first) {
	    Syslog('+', "Pack unpacked mail for %s via %s", aka2str(nodenr), ascfnode(&noden, 0x1f));
	    if (!do_quiet) {
		printf("\rAdding netmail for %s                      ", ascfnode(&noden, 0x1f));
		fflush(stdout);
	    }
	    first = FALSE;
	    flushed = TRUE;
	}

	snprintf(pktfile, PATH_MAX, "%s/%s", temp, fname);

	if (strstr(fname, ".ddd"))
	    flavor = 'd';
	else if (strstr(fname, ".ccc"))
	    flavor = 'c';
	else if (strstr(fname, ".hhh"))
	    flavor = 'h';
	else
	    flavor = 'o';

	if (nodes.Session_out == S_DIR) {
	    snprintf(arcfile, PATH_MAX, "%s/%08x.pkt", nodes.Dir_out_path, sequencer());
	} else {
	    snprintf(arcfile, PATH_MAX, "%s", pktname(&noden, flavor));
	}

	/*
	 *  Now we must see if there is already mail in the outbound.
	 *  If that's the case, we must skip the .pkt header from the queue
	 *  because there is already a .pkt header also, append to the
	 *  outbound 2 bytes before the end of file, this is the zero word.
	 */
	if ((inf = fopen(pktfile, "r")) != NULL) {
	    if (access(arcfile, R_OK) == -1) {
		ouf = fopen(arcfile, "w");  /* create new  */
		Syslog('p', "Create new %s", arcfile);
	    } else {
		ouf = fopen(arcfile, "r+"); /* open R/W    */
		fseek(ouf, -2, SEEK_END);   /* b4 0 word   */
		fseek(inf, 58, SEEK_SET);   /* skip header */
		Syslog('p', "Append to %s", arcfile);
	    }

	    if (ouf != NULL) {
		buf = malloc(16384);

		do {
		    bread = fread(buf, 1, 16384, inf);
		    fwrite(buf, 1, bread, ouf);
		} while (bread);

		free(buf);
		putc('\0', ouf);
		putc('\0', ouf);
		fsync(fileno(ouf));
		fclose(ouf);
		fclose(inf);
		unlink(pktfile);
	    } else {
		WriteError("$Can't open %s", arcfile);
		fclose(inf);
	    }
	}

	/*
	 * Change filemode so downlink has rights to the file.
	 */
	if (nodes.Session_out == S_DIR)
	    chmod(arcfile, 0660);
    }

    /*
     * Now add the files for the node, information is in the .filelist
     * file, this tells the location of the file and what to do with
     * it after it is sent.
     */
    snprintf(pktfile, PATH_MAX, "%s/.filelist", temp);
    if ((fp = fopen(pktfile, "r")) != NULL) {

	Syslog('+', "Adding files for %s via %s", aka2str(nodenr), ascfnode(&noden, 0x1f));
	if (!do_quiet) {
	    printf("\rAdding files for %s                        ", ascfnode(&noden, 0x1f));
	    fflush(stdout);
	    flushed = TRUE;
	}

	buf = calloc(PATH_MAX + 1, sizeof(char));

	while (fgets(buf, PATH_MAX, fp)) {
	    Striplf(buf);
	    Syslog('p', ".filelist: %s", buf);
	    flavor = buf[0];
	    p = strchr(buf, ' ');
	    p++;
	    if (strncmp(p, "LEAVE ", 6) == 0)
		mode = LEAVE;
	    else if (strncmp(p, "KFS ", 4) == 0)
		mode = KFS;
	    else if (strncmp(p, "TFS ", 4) == 0)
		mode = TFS;
	    else {
		WriteError("Syntax error in filelist \"%s\"", buf);
		mode = LEAVE;
	    }
	    p = strchr(p, ' ');
	    p++;
	    // Here is a extra now unused keyword.
	    p = strchr(p, ' ');
	    p++;

	    Syslog('p', "File attach %s to %s", p, ascfnode(&noden, 0x1f));
	    if (nodes.Session_out == S_DIRECT) {
		attach(noden, p, mode, flavor);
	    } else if (nodes.Session_out == S_DIR) {
		snprintf(arcfile, PATH_MAX, "%s/%s", nodes.Dir_out_path, Basename(p));
		if (mode == LEAVE) {
		    /*
		     * LEAVE file, so we copy this one.
		     */
		    rc = file_cp(p, arcfile);
		    Syslog('p', "file_cp(%s, %s) rc=%d", p, arcfile, rc);
		} else {
		    /*
		     * KFS or TFS, move file to node directory
		     */
		    rc = file_mv(p, arcfile);
		    Syslog('p', "file_mv(%s, %s) rc=%d", p, arcfile, rc);
		}
		chmod(arcfile, 0660);
	    }
	}

	free(buf);
	fclose(fp);
	unlink(pktfile);
    }

    /*
     * We are done, the queue is flushed, unlock the node.
     */
    if (nodes.Session_out == S_DIR)
	remlock(nodes.Dir_out_mlock, nodes.Dir_out_mklck, 'p');
    else
	nodeulock(&noden, mypid);

    if (noden.domain)
	free(noden.domain);
    free(temp);
    free(arcfile);
    free(pktfile);
    return;
}



/*
 * Flush outbound queue to real outbound.
 */
void flush_queue(void)
{
    char	    *temp;
    struct dirent   *de;
    DIR		    *dp;
    
    Syslog('+', "Flushing outbound queue");
    if (enoughspace(CFG.freespace) == 0) {
	Syslog('+', "Low diskspace, not flushing outbound queue");
	return;
    }

    IsDoing("Flush queue");
    if (!do_quiet) {
	mbse_colour(LIGHTBLUE, BLACK);
	printf("Flushing outbound queue\n");
	mbse_colour(CYAN, BLACK);
    }
    
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/foobar", CFG.out_queue);
    mkdirs(temp, 0750);

    if ((dp = opendir(CFG.out_queue)) == 0) {
	WriteError("$Can't open %s", CFG.out_queue);
	free(temp);
	return;
    }
    
    /*
     * The outbound queue contains subdirectories which actuallly hold the
     * queue outbound files. Process each found subdirectory.
     */
    while ((de = readdir(dp))) {
	if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
	    snprintf(temp, PATH_MAX, "%s/%s", CFG.out_queue, de->d_name);
	    Syslog('p', "Queue directory %s", temp);
	    flush_dir(de->d_name);
	    if (chdir(CFG.out_queue))
		WriteError("$Can't chdir to %s", CFG.out_queue);
	    if (rmdir(temp))
		WriteError("$Can't rmdir %s", temp);
	}
    }
    closedir(dp);
    free(temp);

    if (!do_quiet) {
	printf("\r                                              \r");
	fflush(stdout);
    }
}


