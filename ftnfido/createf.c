/*****************************************************************************
 *
 * $Id: createf.c,v 1.25 2005/10/11 20:49:47 mbse Exp $
 * Purpose ...............: Create TIC Area and BBS file area.
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
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "mgrutil.h"
#include "createf.h"


#define MCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"


int create_ticarea(char *farea, faddr *p_from)
{
    char        *temp;
    FILE        *gp;

    Syslog('f', "create_ticarea(%s)", farea);
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
    if ((gp = fopen(temp, "r")) == NULL) {
        WriteError("Can't open %s", temp);
        free(temp);
        return FALSE;
    }
    fread(&fgrouphdr, sizeof(fgrouphdr), 1, gp);
    free(temp);

    fseek(gp, fgrouphdr.hdrsize, SEEK_SET);
    while ((fread(&fgroup, fgrouphdr.recsize, 1, gp)) == 1) {
        if ((fgroup.UpLink.zone  == p_from->zone) && (fgroup.UpLink.net   == p_from->net) &&
            (fgroup.UpLink.node  == p_from->node) && (fgroup.UpLink.point == p_from->point) &&
            strlen(fgroup.AreaFile)) {
            if (CheckTicGroup(farea, FALSE, p_from) == 0) {
                fclose(gp);
                return TRUE;
            }
        }
    }
    fclose(gp);
    return FALSE;
}



/*
 * Check TIC group AREAS file if requested area exists.
 * If so, create tic area and if SendUplink is TRUE,
 * send the uplink a FileMgr request to connect this area.
 * The tic group record (fgroup) must be in memory.
 * Return codes:
 *  0  - All Seems Well
 *  1  - Some error
 *
 * The current nodes record may be destroyed after this,
 * make sure it is saved.
 */
int CheckTicGroup(char *Area, int SendUplink, faddr *f)
{
    char        *temp, *buf, *tag = NULL, *desc = NULL, *p, *raid = NULL, *flow = NULL;
    FILE        *ap, *mp, *fp;
    int		offset, AreaNr;
    int         i, rc = 0, Found = FALSE;
    sysconnect  System;
    faddr	*From, *To;
    struct _fdbarea *fdb_area = NULL;

    temp = calloc(PATH_MAX, sizeof(char));
    Syslog('f', "Checking file group \"%s\" \"%s\"", fgroup.Name, fgroup.Comment);
    snprintf(temp, PATH_MAX, "%s/%s", CFG.alists_path , fgroup.AreaFile);
    if ((ap = fopen(temp, "r")) == NULL) {
        WriteError("Filegroup %s: area taglist %s not found", fgroup.Name, temp);
        free(temp);
        return 1;
    }

    buf = calloc(4097, sizeof(char));

    if (fgroup.FileGate) {
	/*
	 * filegate.zxx format
	 */
	while (fgets(buf, 4096, ap)) {
	    /*
	     * Each filegroup starts with "% FDN:      Filegroup Description"
	     */
	    if (strlen(buf) && !strncmp(buf, "% FDN:", 6)) {
		tag = strtok(buf, "\t \r\n\0");
		p = strtok(NULL, "\t \r\n\0");
		p = strtok(NULL, "\r\n\0");
		desc = p;
		while ((*desc == ' ') || (*desc == '\t'))
		    desc++;
		if (!strcmp(desc, fgroup.Comment)) {
		    while (fgets(buf, 4096, ap)) {
			if (!strncasecmp(buf, "Area ", 5)) {
			    tag = strtok(buf, "\t \r\n\0");
			    tag = strtok(NULL, "\t \r\n\0");
			    if (!strcasecmp(tag, Area)) {
				raid = strtok(NULL, "\t \r\n\0");
				flow = strtok(NULL, "\t \r\n\0");
				p = strtok(NULL, "\r\n\0");
				desc = p;
				while ((*desc == ' ') || (*desc == '\t'))
				    desc++;
				Syslog('f', "Found area \"%s\" \"%s\" \"%s\" \"%s\"", tag, raid, flow, desc);
				Found = TRUE;
				break;
			    }
			}
			if (strlen(buf) && !strncmp(buf, "% FDN:", 6)) {
			    /*
			     * All entries in group are seen, the area wasn't there.
			     */
			    break;
			}
		    }
		}
		if (Found)
		    break;
	    }
	}
    } else {
	/*
	 * Normal taglist format
	 */
	while (fgets(buf, 4096, ap)) {
	    if (strlen(buf) && isalnum(buf[0])) {
		tag = strtok(buf, "\t \r\n\0");
		p = strtok(NULL, "\r\n\0");
		if (p == NULL)
		    p = tag; /* If no description after the TAG, use TAG as description */
		desc = p;
		while ((*desc == ' ') || (*desc == '\t'))
		    desc++;
		if (strcasecmp(tag, Area) == 0) {
		    Syslog('f', "Found tag \"%s\" desc \"%s\"", tag, desc);
		    Found = TRUE;
		    break;
		}
	    }
	}
    }
    if (!Found) {
	Syslog('f', "Area %s not found in taglist", Area);
	free(buf);
	fclose(ap);
	free(temp);
	return 1;
    }

    /*
     * Some people write taglists with lowercase tagnames...
     */
    for (i = 0; i < strlen(tag); i++)
	tag[i] = toupper(tag[i]);

    Syslog('m', "Found tag \"%s\" desc \"%s\"", tag, desc);

    /*
     * Area is in AREAS file, now create area.
     * If needed, connect at uplink.
     */
    if (SendUplink && SearchNode(fgroup.UpLink)) {
	if (nodes.UplFmgrBbbs)
	    snprintf(temp, PATH_MAX, "file +%s", tag);
	else
	    snprintf(temp, PATH_MAX, "+%s", tag);

	From = fido2faddr(fgroup.UseAka);
	To   = fido2faddr(fgroup.UpLink);
	if (UplinkRequest(To, From, TRUE, temp)) {
	    WriteError("Can't send netmail to uplink");
	    fclose(ap);
	    free(buf);
	    free(temp);
	    tidy_faddr(From);
	    tidy_faddr(To);
	    return 1;
	}
	tidy_faddr(From);
	tidy_faddr(To);
    }

    /*
     * Open tic area and set filepointer to the end to append
     * a new record.
     */
    snprintf(temp, PATH_MAX, "%s/etc/tic.data", getenv("MBSE_ROOT"));
    if ((mp = fopen(temp, "r+")) == NULL) {
	WriteError("$Can't open %s", temp);
	fclose(ap);
	free(buf);
	free(temp);
	return 1;
    }
    fread(&tichdr, sizeof(tichdr), 1, mp);
    fseek(mp, 0, SEEK_END);
    memset(&tic, 0, sizeof(tic));
    Syslog('f', "TIC area open, filepos %ld", ftell(mp));

    /*
     * Open files area, and find a free slot
     */
    snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r+")) == NULL) {
	WriteError("$Can't open %s", temp);
	fclose(ap);
	fclose(mp);
	free(buf);
	free(temp);
	return 1;
    }
    fread(&areahdr, sizeof(areahdr), 1, fp);
    Syslog('f', "File area is open");

    /*
     * Verify the file is large enough
     */
    fseek(fp, 0, SEEK_END);
    offset = areahdr.hdrsize + ((fgroup.StartArea -1) * (areahdr.recsize));
    Syslog('+', "file end at %ld, offset needed %ld", ftell(fp), offset);

    if (ftell(fp) < offset) {
	Syslog('f', "Database too small, expanding...");
	memset(&area, 0, sizeof(area));
	while (TRUE) {
	    fwrite(&area, sizeof(area), 1, fp);
	    if (ftell(fp) >= areahdr.hdrsize + ((fgroup.StartArea -1) * (areahdr.recsize)))
		break;
	}
    } 

    if (fseek(fp, offset, SEEK_SET)) {
	WriteError("$Can't seek in %s to position %ld", temp, offset);
	fclose(ap);
	fclose(mp);
	fclose(fp);
	free(buf);
	free(temp);
	return 1;
    }

    /*
     * Search a free record
     */
    Syslog('f', "Start search record");
    while (fread(&area, sizeof(area), 1, fp) == 1) {
	if (!area.Available) {
	    fseek(fp, - areahdr.recsize, SEEK_CUR);
	    rc = 1;
	    break;
	}
    }
 
    if (!rc) {
	Syslog('f', "No free slot, append after last record");
	fseek(fp, 0, SEEK_END);
	if (ftell(fp) < areahdr.hdrsize + ((fgroup.StartArea -1) * (areahdr.recsize))) {
	    Syslog('f', "Database too small, expanding...");
	    memset(&area, 0, sizeof(area));
	    while (TRUE) {
		fwrite(&area, sizeof(area), 1, fp);
		if (ftell(fp) >= areahdr.hdrsize + ((fgroup.StartArea -1) * (areahdr.recsize)))
		    break;
	    }
	}
	rc = 1;
    }
    AreaNr = ((ftell(fp) - areahdr.hdrsize) / (areahdr.recsize)) + 1;
    Syslog('f', "Found free slot at %ld", AreaNr);

    /*
     * Create the records
     */
    memset(&area, 0, sizeof(area));
    strncpy(area.Name, desc, 44);
    strcpy(temp, tag);
    temp = tl(temp);
    for (i = 0; i < strlen(temp); i++)
	if (temp[i] == '.')
	    temp[i] = '/';
    snprintf(area.Path, 81, "%s/%s", fgroup.BasePath, temp);
    area.DLSec = fgroup.DLSec;
    area.UPSec = fgroup.UPSec;
    area.LTSec = fgroup.LTSec;
    area.New = area.Dupes = area.Free = area.AddAlpha = area.FileFind = area.Available = area.FileReq = TRUE;
    strncpy(area.BbsGroup, fgroup.BbsGroup, 12);
    strncpy(area.NewGroup, fgroup.AnnGroup, 12);
    strncpy(area.Archiver, fgroup.Convert, 5);
    area.Upload = fgroup.Upload;
    fwrite(&area, sizeof(area), 1, fp);
    fclose(fp);

    /*
     * Create download path
     */
    snprintf(temp, PATH_MAX, "%s/foobar", area.Path);
    if (!mkdirs(temp, 0775))
	WriteError("Can't create %s", temp);

    /*
     * Create download database
     */
    if ((fdb_area = mbsedb_OpenFDB(AreaNr, 30)))
	mbsedb_CloseFDB(fdb_area);

    /*
     * Setup new TIC area.
     */
    strncpy(tic.Name, tag, 20);
    strncpy(tic.Comment, desc, 55);
    tic.FileArea = AreaNr;
    strncpy(tic.Group, fgroup.Name, 12);
    tic.AreaStart = time(NULL);
    tic.Aka = fgroup.UseAka;
    strncpy(tic.Convert, fgroup.Convert, 5);
    strncpy(tic.Banner, fgroup.Banner, 14);
    tic.Replace = fgroup.Replace;
    tic.DupCheck = fgroup.DupCheck;
    tic.Secure = fgroup.Secure;
    tic.Touch = fgroup.Touch;
    tic.VirScan = fgroup.VirScan;
    tic.Announce = fgroup.Announce;
    tic.UpdMagic = fgroup.UpdMagic;
    tic.FileId = fgroup.FileId;
    tic.ConvertAll = fgroup.ConvertAll;
    tic.SendOrg = fgroup.SendOrg;
    tic.Active = TRUE;
    tic.LinkSec.level = fgroup.LinkSec.level;
    tic.LinkSec.flags = fgroup.LinkSec.flags;
    tic.LinkSec.notflags = fgroup.LinkSec.notflags;
    fwrite(&tic, sizeof(tic), 1, mp);

    memset(&System, 0, sizeof(System));
    System.aka = fgroup.UpLink;
    if (flow && !strcmp(flow, "*&"))
	/*
	 * Areas direction HQ's go the other way
	 */
	System.sendto = TRUE;
    else
	/*
	 * Normal distribution areas.
	 */
	System.receivefrom = TRUE;
    fwrite(&System, sizeof(System), 1, mp);
    memset(&System, 0, sizeof(System));
    for (i = 1; i < (tichdr.syssize / sizeof(System)); i++)
	fwrite(&System, sizeof(System), 1, mp);   

    fclose(mp);
    fclose(ap);
    free(buf);
    free(temp);
    if (f == NULL)
	Mgrlog("Auto created TIC area %s, group %s, bbs area %ld", tic.Name, tic.Group, AreaNr);
    else
	Mgrlog("Auto created TIC area %s, group %s, bbs area %ld, for node %s",
	    tic.Name, tic.Group, AreaNr, ascfnode(f, 0x1f));

    return 0;
}


