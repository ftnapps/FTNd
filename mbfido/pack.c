/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Pack mail
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/dbftn.h"
#include "../lib/clcomm.h"
#include "../lib/dbnode.h"
#include "dirsession.h"
#include "pack.h"


extern	int do_quiet;			/* Quiet flag		     */


static char *dow[] = {(char *)"su", (char *)"mo", (char *)"tu", (char *)"we", 
                      (char *)"th", (char *)"fr", (char *)"sa"};


/*
 *  Pack queued arcmail mail for a node. If the node is locked, the mail won't
 *  be packed, and the queue stays as it is. The mail will then be packed
 *  on a next run.
 */
int pack_queue(char *name)
{
    FILE	*fp;
    faddr	noden, *bestaka;
    fidoaddr	nodenr;
    char	flavor, nr, oldnr, maxnr, *ext, srcfile[128], *arcfile, *pktfile;
    int		Attach, fage;
    long	fsize;
    time_t	Now;
    struct tm	*ptm;

    sprintf(srcfile, "%s", name);

    /*
     * Get the nodenumber from the filename
     */
    noden.domain = NULL;
    noden.name   = NULL;
    noden.zone   = atoi(strtok(name, "."));
    noden.net    = atoi(strtok(NULL, "."));
    noden.node   = atoi(strtok(NULL, "."));
    noden.point  = atoi(strtok(NULL, "."));
    if (SearchFidonet(noden.zone))
	noden.domain = xstrcpy(fidonet.domain);

    memset(&nodenr, 0, sizeof(nodenr));
    nodenr.zone  = noden.zone;
    nodenr.net   = noden.net;
    nodenr.node  = noden.node;
    nodenr.point = noden.point;
    sprintf(nodenr.domain, "%s", noden.domain);

    if (!SearchNode(nodenr)) {
	WriteError("Downlink %s not found", aka2str(nodenr));
	if (noden.domain)
	    free(noden.domain);
	return FALSE;
    }

    /*
     * If we route via another aka, change everything.
     */
    if (nodes.RouteVia.zone) {
	Syslog('p', "Route Via %s", aka2str(nodes.RouteVia));
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

    Syslog('+', "Pack ARCmail for %s, via %s", aka2str(nodenr), ascfnode(&noden, 0x1f));

    if (!do_quiet) {
	printf("\rAdding ARCmail for %s                      ", ascfnode(&noden, 0x1f));
	fflush(stdout);
    }

    if (getarchiver((char *)"ZIP")) {
	flavor = 'f';
	if (nodes.Crash)
	    flavor = 'c';
	if (nodes.Hold)
	    flavor = 'h';
    } else {
	WriteError("Archiver ZIP not found");
	if (noden.domain)
	    free(noden.domain);
	return FALSE;
    }

    /*
     * Generate ARCmail filename and .PKT filename,
     */
    arcfile = calloc(PATH_MAX, sizeof(char));
    if (nodes.Session_out == S_DIR) {

	Now = time(NULL);
	ptm = localtime(&Now);
	ext = dow[ptm->tm_wday];

	if (!nodes.ARCmailCompat && (nodes.Aka[0].zone != noden.zone)) {
	    /*
	     * Generate ARCfile name from the CRC of the ASCII string
	     * of the node address.
	     */
	    sprintf(arcfile, "%s/%08lx.%s0", nodes.Dir_out_path, StringCRC32(ascfnode(&noden, 0x1f)), ext);
	} else {
	    bestaka = bestaka_s(&noden);

	    if (noden.point) {
		sprintf(arcfile, "%s/%04x%04x.%s0", nodes.Dir_out_path, ((bestaka->net) - (noden.net)) & 0xffff,
			((bestaka->node) - (noden.node) + (noden.point)) & 0xffff, ext);
	    } else if (bestaka->point) {
		/*
		 * Inserted the next code for if we are a point,
		 * I hope this is ARCmail 0.60 compliant. 21-May-1999
		 */
		sprintf(arcfile, "%s/%04x%04x.%s0", nodes.Dir_out_path, ((bestaka->net) - (noden.net)) & 0xffff,
			    ((bestaka->node) - (noden.node) - (bestaka->point)) & 0xffff, ext);
	    } else {
		sprintf(arcfile, "%s/%04x%04x.%s0", nodes.Dir_out_path, ((bestaka->net) - (noden.net)) & 0xffff,
			    ((bestaka->node) - (noden.node)) &0xffff, ext);
	    }
	}
	Syslog('m', "Arcmail file %s", arcfile);
    } else {
	sprintf(arcfile, "%s", arcname(&noden, nodes.Aka[0].zone, nodes.ARCmailCompat));
    }
    pktfile = calloc(40, sizeof(char));
    sprintf(pktfile, "%08lx.pkt", sequencer());

    if (nodes.Session_out == S_DIR) {
	if (islocked(nodes.Dir_out_clock, nodes.Dir_out_chklck, nodes.Dir_out_waitclr)) {
	    Syslog('+', "Mail stays in queue, will be added later");
	    if (noden.domain)
		free(noden.domain);
	    free(arcfile);
	    free(pktfile);
	    return FALSE;
	} else {
	    if (! setlock(nodes.Dir_out_mlock, nodes.Dir_out_mklck)) {
		Syslog('+', "Mail stays in queue, will be added later");
		if (noden.domain)
		    free(noden.domain);
		free(arcfile);
		free(pktfile);
		return FALSE;
	    }
	}
    } else {
	if (nodelock(&noden)) {
	    Syslog('+', "Mail stays in queue, will be added later");
	    if (noden.domain)
		free(noden.domain);
	    free(arcfile);
	    free(pktfile);
	    return FALSE;
	}
    }

    if (rename(srcfile, pktfile)) {
	WriteError("$Can't rename %s to %s", srcfile, pktfile);
	if (nodes.Session_out == S_DIR)
	    remlock(nodes.Dir_out_mlock, nodes.Dir_out_mklck);
	else
	    nodeulock(&noden);
	if (noden.domain)
	    free(noden.domain);
	free(arcfile);
	free(pktfile);
	return FALSE;
    }

    /*
     * Add zero word at the end of the .pkt file
     */
    if ((fp = fopen(pktfile, "a+")) == NULL) {
	WriteError("$Can't open %s", pktfile);
	if (nodes.Session_out == S_DIR)
	    remlock(nodes.Dir_out_mlock, nodes.Dir_out_mklck);
	else
	    nodeulock(&noden);
	if (noden.domain)
	    free(noden.domain);
	free(arcfile);
	free(pktfile);
	return FALSE;
    }
    putc('\0', fp);
    putc('\0', fp);
    fsync(fileno(fp));
    fclose(fp);

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
    Now = time(NULL);
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
	    nr++;
	    if (nr == ('9' +1))
		nr = 'a';
	    arcfile[strlen(arcfile) -1] = nr;

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
    if (execute(archiver.marc, arcfile, pktfile, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null") == 0) {
	unlink(pktfile);
    } else {
	sync();
	sleep(1);
	Syslog('+', "Create ARCmail failed, trying again after sync()"); 
	if (execute(archiver.marc, arcfile, pktfile, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null") == 0) {
	    unlink(pktfile);
	} else {
	    WriteError("Can't add %s to ARCmail archive", pktfile);
	}
    }

    if (nodes.Session_out == S_DIR) {
	/*
	 * Change filemode so downlink has rights to the file.
	 */
	chmod(arcfile, 0660);
    }

    /*
     * Attach file to .flo, not for FTP or Directory sessions.
     */
    if (Attach && nodes.Session_out == S_DIRECT)
	attach(noden, arcfile, TFS, flavor);

    if (nodes.Session_out == S_DIR)
	remlock(nodes.Dir_out_mlock, nodes.Dir_out_mklck);
    else
	nodeulock(&noden);
    if (noden.domain)
	free(noden.domain);
    free(arcfile);
    free(pktfile);
    return TRUE;
}



/*
 *  Add queued unpacked mail for a node. If the node is locked, the mail
 *  stays in the queue.
 */
int add_queue(char *name)
{
    faddr   noden;
    char    flavor, srcfile[128], *outfile, *buf;
    FILE    *inf, *ouf;
    int	bread;

    sprintf(srcfile, "%s", name);

    /*
     * Get the nodenumber from the filename
     */
    noden.domain = NULL;
    noden.name   = NULL;
    noden.zone   = atoi(strtok(name, "."));
    noden.net    = atoi(strtok(NULL, "."));
    noden.node   = atoi(strtok(NULL, "."));
    noden.point  = atoi(strtok(NULL, "."));
    if (SearchFidonet(noden.zone))
	noden.domain = xstrcpy(fidonet.domain);

    Syslog('+', "Add Netmail for %s", ascfnode(&noden, 0x1f));
    if (!do_quiet) {
	printf("\rAdding Netmail for %s                      ", ascfnode(&noden, 0x1f));
	fflush(stdout);
    }

    outfile = calloc(PATH_MAX, sizeof(char));
    if (strstr(srcfile, ".iii"))
	flavor = 'i';
    else if (strstr(srcfile, ".ccc"))
	flavor = 'c';
    else if (strstr(srcfile, ".hhh"))
	flavor = 'h';
    else
	flavor = 'f';
    if (nodes.Session_out == S_DIR) {
	if (flavor == 'f')
	    flavor = 'o';
	if (noden.point)
	    sprintf(outfile, "%s/%08x.%cut", nodes.Dir_out_path, noden.point, flavor);
	else
	    sprintf(outfile, "%s/%04x%04x.%cut", nodes.Dir_out_path, noden.net, noden.node, flavor);
    } else {
	sprintf(outfile, "%s", pktname(&noden, flavor));
    }
    Syslog('p', "Outfile: %s", outfile);

    if (nodes.Session_out == S_DIR) {
	if (islocked(nodes.Dir_out_clock, nodes.Dir_out_chklck, nodes.Dir_out_waitclr)) {
	    Syslog('+', "Mail stays in queue, will be added later");
	    if (noden.domain)
		free(noden.domain);
	    return FALSE;
	} else {
	    if (! setlock(nodes.Dir_out_mlock, nodes.Dir_out_mklck)) {
		Syslog('+', "Mail stays in queue, will be added later");
		if (noden.domain)
		    free(noden.domain);
		return FALSE;
	    }
	}
    } else {
	if (nodelock(&noden)) {
	    Syslog('+', "Mail stays in queue, will be added later");
	    free(outfile);
	    if (noden.domain)
		free(noden.domain);
	    return FALSE;
	}
    }

    /*
     *  Now we must see if there is already mail in the outbound.
     *  If that's the case, we must skip the .pkt header from the queue
     *  because there is already a .pkt header also, append to the
     *  outbound 2 bytes before the end of file, this is the zero word.
     */
    if ((inf = fopen(srcfile, "r")) != NULL) {
	if (access(outfile, R_OK) == -1) {
	    ouf = fopen(outfile, "w");  /* create new  */
	    Syslog('p', "Create new %s", outfile);
	} else {
	    ouf = fopen(outfile, "r+"); /* open R/W    */
	    fseek(ouf, -2, SEEK_END);   /* b4 0 word   */
	    fseek(inf, 58, SEEK_SET);   /* skip header */
	    Syslog('p', "Append to %s", outfile);
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
	    unlink(srcfile);
	} else {
	    WriteError("$Can't open %s", outfile);
	    fclose(inf);
	}
    }
	
    if (nodes.Session_out == S_DIR) {
	/*
	 * Change filemode so downlink has rights to the file.
	 */
	chmod(outfile, 0660);
    }

    if (nodes.Session_out == S_DIR)
	remlock(nodes.Dir_out_mlock, nodes.Dir_out_mklck);
    else
	nodeulock(&noden);
    if (noden.domain)
	free(noden.domain);
    free(outfile);
    return TRUE;
}



/*
 *  Pack mailqueue file(s) in the $MBSE_ROOT/tmp directory.
 */
void packmail()
{
    char	    *temp;
    struct dirent   *de;
    DIR		    *dp;

    if (!diskfree(CFG.freespace))
	return;

    IsDoing("Packing mail");
    if (!do_quiet) {
	colour(9, 0);
	printf("Packing mail\n");
	colour(3, 0);
    }

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/tmp", getenv("MBSE_ROOT"));

    if (chdir(temp) == -1) {
	WriteError("$Error chdir to %s", temp);
	free(temp);
	return;
    }

    if ((dp = opendir(temp)) == NULL) {
	WriteError("$Error opendir %s", temp);
	free(temp);
	return;
    }

    /*
     * Scan the $MBSE_ROOT/tmp directory for .qqq or .nnn files
     */
    while ((de = readdir(dp))) {
	if (strstr(de->d_name, ".qqq"))
	    pack_queue(de->d_name);
	if (strstr(de->d_name, ".nnn") || strstr(de->d_name, ".iii") || strstr(de->d_name, ".ccc") || strstr(de->d_name, ".hhh"))
	    add_queue(de->d_name);
    }

    closedir(dp);
    free(temp);

    if (!do_quiet) {
	printf("\r                                              \r");
	fflush(stdout);
    }
}



