/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Create Message Area
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
#include "../lib/msg.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "mgrutil.h"
#include "createm.h"


#define MCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"


int create_msgarea(char *marea, faddr *p_from)
{
    char	*temp;
    FILE	*gp;

    Syslog('m', "create_msgarea(%s)", marea);
    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
    if ((gp = fopen(temp, "r")) == NULL) {
	WriteError("Can't open %s", temp);
	free(temp);
	return FALSE;
    }
    fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);
    free(temp);

    fseek(gp, mgrouphdr.hdrsize, SEEK_SET);
    while ((fread(&mgroup, mgrouphdr.recsize, 1, gp)) == 1) {
	if ((mgroup.UpLink.zone  == p_from->zone) && (mgroup.UpLink.net   == p_from->net) &&
	    (mgroup.UpLink.node  == p_from->node) && (mgroup.UpLink.point == p_from->point) &&
	    strlen(mgroup.AreaFile)) {
	    if (CheckEchoGroup(marea, FALSE, p_from) == 0) {
		fclose(gp);
		return TRUE;
	    }
	}
    }
    fclose(gp);
    return FALSE;
}



/*
 * Check echomail group AREAS file if requested area exists.
 * If so, create echomail area and if SendUplink is TRUE,
 * send the uplink a AreaMgr request to connect this area.
 * The echomail group record (mgroup) must be in memory.
 * Return codes:
 *  0  - All Seems Well
 *  1  - Some error
 *
 * The current nodes record may be destroyed after this,
 * make sure it is saved.
 */
int CheckEchoGroup(char *Area, int SendUplink, faddr *f)
{
    char	*temp, *buf, *tag, *desc, *p;
    FILE	*ap, *mp;
    long	offset;
    int		i, rc = 0;
    sysconnect	System;
    faddr	*From, *To;

    temp = calloc(PATH_MAX, sizeof(char));
    Syslog('m', "Checking echogroup %s %s", mgroup.Name, mgroup.Comment);
    sprintf(temp, "%s/%s", CFG.alists_path , mgroup.AreaFile);
    if ((ap = fopen(temp, "r")) == NULL) {
	WriteError("Echogroup %s: area taglist %s not found", mgroup.Name, temp);
	free(temp);
	return 1;
    }

    buf = calloc(4097, sizeof(char));
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
		/*
		 * Make sure the tag is uppercase
		 */
		for (i = 0; i < strlen(tag); i++)
		    tag[i] = toupper(tag[i]);

		Syslog('m', "Found tag \"%s\" desc \"%s\"", tag, desc);

		/*
		 * Area is in AREAS file, now create area.
		 * If needed, connect at uplink.
		 */
		if (SendUplink) {
		    sprintf(temp, "+%s", tag);
		    From = fido2faddr(mgroup.UseAka);
		    To   = fido2faddr(mgroup.UpLink);
		    if (UplinkRequest(To, From, FALSE, temp)) {
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

		sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
		if ((mp = fopen(temp, "r+")) == NULL) {
		    WriteError("$Can't open %s", temp);
		    fclose(ap);
		    free(buf);
		    free(temp);
		    return 1;
		}
		fread(&msgshdr, sizeof(msgshdr), 1, mp);

		/*
		 * Verify the file is large enough
		 */
		fseek(mp, 0, SEEK_END);
		offset = msgshdr.hdrsize + ((mgroup.StartArea -1) * (msgshdr.recsize + msgshdr.syssize));
		Syslog('m', "file end at %ld, start area offset at %ld", ftell(mp), offset);

		if (ftell(mp) < offset) {
		    /*
		     * Start area record not in database, expand until start record exists.
		     */
		    Syslog('m', "Database too small, expanding...");
		    memset(&msgs, 0, sizeof(msgs));
		    memset(&System, 0, sizeof(System));
		    while (TRUE) {
			fwrite(&msgs, sizeof(msgs), 1, mp);
			for (i = 0; i < (msgshdr.syssize / sizeof(System)); i++)
			    fwrite(&System, sizeof(System), 1, mp);
			if (ftell(mp) >= msgshdr.hdrsize + ((mgroup.StartArea -1) * (msgshdr.recsize + msgshdr.syssize)))
			    break;
		    }
		}

		if (fseek(mp, offset, SEEK_SET)) {
		    WriteError("$Can't seek in %s to position %ld", temp, offset);
		    fclose(ap);
		    fclose(mp);
		    free(buf);
		    free(temp);
		    return 1;
		}

		/*
		 * Search a free record
		 */
		while (fread(&msgs, sizeof(msgs), 1, mp) == 1) {
		    if (!msgs.Active) {
			fseek(mp, - msgshdr.recsize, SEEK_CUR);
			offset = ((ftell(mp) - msgshdr.hdrsize) / (msgshdr.recsize + msgshdr.syssize)) + 1;
			Syslog('m', "Found free slot at %ld", offset);
			rc = 1;
			break;
		    }
		    /*
		     * Skip systems
		     */
		    fseek(mp, msgshdr.syssize, SEEK_CUR);
		}

		if (!rc) {
		    Syslog('m', "No free slot, append after last record");
		    fseek(mp, 0, SEEK_END);
		    if (ftell(mp) < msgshdr.hdrsize + ((mgroup.StartArea -1) * (msgshdr.recsize + msgshdr.syssize))) {
			Syslog('m', "Database too small, expanding...");
			memset(&msgs, 0, sizeof(msgs));
			memset(&System, 0, sizeof(System));
			while (TRUE) {
			    fwrite(&msgs, sizeof(msgs), 1, mp);
			    for (i = 0; i < (msgshdr.syssize / sizeof(System)); i++)
				fwrite(&System, sizeof(System), 1, mp);
			    if (ftell(mp) >= msgshdr.hdrsize + ((mgroup.StartArea -1) * (msgshdr.recsize + msgshdr.syssize)))
				break;
			}
		    }
		    rc = 1;
		}

		/*
		 * Create the record with the defaults from the group record.
		 */
		offset = ((ftell(mp) - msgshdr.hdrsize) / (msgshdr.recsize + msgshdr.syssize)) + 1;
		memset(&msgs, 0, sizeof(msgs));
		strncpy(msgs.Tag, tag, 50);
		strncpy(msgs.Name, desc, 40);
		strncpy(msgs.QWKname, tag, 20);
		msgs.MsgKinds = PUBLIC;
		msgs.Type = ECHOMAIL;
		msgs.DaysOld = CFG.defdays;
		msgs.MaxMsgs = CFG.defmsgs;
		msgs.UsrDelete = mgroup.UsrDelete;
		msgs.RDSec = mgroup.RDSec;
		msgs.WRSec = mgroup.WRSec;
		msgs.SYSec = mgroup.SYSec;
		msgs.LinkSec = mgroup.LinkSec;
		strncpy(msgs.Group, mgroup.Name, 12);
		msgs.Aka = mgroup.UseAka;
		strncpy(msgs.Origin, CFG.origin, 50);
		msgs.Aliases = mgroup.Aliases;
		msgs.NetReply = mgroup.NetReply;
		msgs.Active = TRUE;
		msgs.Quotes = mgroup.Quotes;
		msgs.Charset = mgroup.Charset;
		msgs.MaxArticles = CFG.maxarticles;
		msgs.Created = time(NULL);
		tag = tl(tag);
		for (i = 0; i < strlen(tag); i++)
		    if (tag[i] == '.')
			tag[i] = '/';
		sprintf(msgs.Base, "%s/%s", mgroup.BasePath, tag);
		sprintf(msgs.Newsgroup, "%s.%s", GetFidoDomain(msgs.Aka.zone), tag);
		for (i = 0; i < strlen(msgs.Newsgroup); i++) {
		    msgs.Newsgroup[i] = tolower(msgs.Newsgroup[i]);
		    if (msgs.Newsgroup[i] == '/')
			msgs.Newsgroup[i] = '.';
		    if (msgs.Newsgroup[i] == '_')
			msgs.Newsgroup[i] = '.';
		}
		fwrite(&msgs, sizeof(msgs), 1, mp);
		mkdirs(msgs.Base, 0770);
		if (Msg_Open(msgs.Base))
		    Msg_Close();
		
		memset(&System, 0, sizeof(System));
		System.aka = mgroup.UpLink;
		System.sendto = System.receivefrom = TRUE;
		fwrite(&System, sizeof(System), 1, mp);
		memset(&System, 0, sizeof(System));
		for (i = 1; i < (msgshdr.syssize / sizeof(System)); i++)
		    fwrite(&System, sizeof(System), 1, mp);
		
		fclose(mp);
		fclose(ap);
		free(buf);
		free(temp);
		if (f == NULL)
		    Mgrlog("Auto created echo %s, group %s, area %ld", msgs.Tag, msgs.Group, offset);
		else
		    Mgrlog("Auto created echo %s, group %s, area %ld, for node %s",
			msgs.Tag, msgs.Group, offset, ascfnode(f , 0x1f));
		return 0;
	    } /* if (strcmp(tag, Area) == 0) */
	} /* if (strlen(buf) && isalnum(buf[0])) */
    } /* while (fgets(buf, 4096, ap)) */

    Syslog('m', "Area %s not found in taglist", Area);
    free(buf);
    fclose(ap);
    free(temp);
    return 1;
}


