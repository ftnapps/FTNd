/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Create Message Area
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "createm.h"


#define MCHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"


int create_msgarea(char *marea, faddr *p_from)
{
    char	*temp, *buf, *tag, *desc, *p;
    FILE	*gp, *ap, *mp;
    long	offset;
    int		i, rc = 0;
    sysconnect	System;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
    if ((gp = fopen(temp, "r")) == NULL) {
	WriteError("$Can't open %s", temp);
	free(temp);
	return FALSE;
    }
    fread(&mgrouphdr, sizeof(mgrouphdr), 1, gp);

    fseek(gp, mgrouphdr.hdrsize, SEEK_SET);
    while ((fread(&mgroup, mgrouphdr.recsize, 1, gp)) == 1) {
	if ((mgroup.UpLink.zone  == p_from->zone) && (mgroup.UpLink.net   == p_from->net) &&
	    (mgroup.UpLink.node  == p_from->node) && (mgroup.UpLink.point == p_from->point) &&
	    strlen(mgroup.AreaFile)) {
	    Syslog('m', "Checking echogroup %s %s", mgroup.Name, mgroup.Comment);
	    sprintf(temp, "%s/%s", CFG.alists_path , mgroup.AreaFile);
	    if ((ap = fopen(temp, "r")) == NULL) {
		WriteError("$Can't open %s", temp);
		free(temp);
		fclose(gp);
		return FALSE;
	    } else {
		buf = calloc(4097, sizeof(char));
		while (fgets(buf, 4096, ap)) {
		    tag = strtok(buf, "\t \r\n\0");
		    p = strtok(NULL, "\r\n\0");
		    desc = p;
		    while ((*desc == ' ') || (*desc == '\t'))
			desc++;
		    if (strcmp(tag, marea) == 0) {
			Syslog('m', "Found tag \"%s\" desc \"%s\"", tag, desc);
			sprintf(temp, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
			if ((mp = fopen(temp, "r+")) == NULL) {
			    WriteError("$Can't open %s", temp);
			    fclose(ap);
			    fclose(gp);
			    free(buf);
			    free(temp);
			    return FALSE;
			}
			fread(&msgshdr, sizeof(msgshdr), 1, mp);
			offset = msgshdr.hdrsize + ((mgroup.StartArea -1) * (msgshdr.recsize + msgshdr.syssize));
			if (fseek(mp, offset, SEEK_SET)) {
			    WriteError("$Can't seek in %s", temp);
			    fclose(ap);
			    fclose(gp);
			    fclose(mp);
			    free(buf);
			    free(temp);
			    return FALSE;
			}
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
			strncpy(msgs.Group, mgroup.Name, 12);
			msgs.Aka = mgroup.UseAka;
			strncpy(msgs.Origin, CFG.origin, 50);
			msgs.Aliases = mgroup.Aliases;
			msgs.NetReply = mgroup.NetReply;
			msgs.Active = TRUE;
			msgs.Quotes = mgroup.Quotes;
			msgs.Rfccode = CHRS_DEFAULT_RFC;
			msgs.Ftncode = CHRS_DEFAULT_FTN;
			msgs.MaxArticles = CFG.maxarticles;
			tag = tl(tag);
			for (i = 0; i < strlen(tag); i++)
			    if (tag[i] == '.')
				tag[i] = '/';
			sprintf(msgs.Base, "%s/%s", mgroup.BasePath, tag);
			fwrite(&msgs, sizeof(msgs), 1, mp);

			memset(&System, 0, sizeof(System));
			System.aka = mgroup.UpLink;
			System.sendto = System.receivefrom = TRUE;
			fwrite(&System, sizeof(System), 1, mp);
			memset(&System, 0, sizeof(System));
			for (i = 1; i < (msgshdr.syssize / sizeof(System)); i++)
			    fwrite(&System, sizeof(System), 1, mp);

			fclose(mp);
			fclose(gp);
			fclose(ap);
			free(buf);
			free(temp);
			Syslog('+', "Auto created echo %s, group %s, area %ld, for node %s", 
				msgs.Tag, msgs.Group, offset, ascfnode(p_from, 0x1f));
			return TRUE;
		    } /* if (strcmp(tag, marea) == 0) */
		}
		free(buf);
		fclose(ap);
	    }
	}
    }
    fclose(gp);
    return FALSE;
}

