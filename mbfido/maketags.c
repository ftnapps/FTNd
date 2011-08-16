/*****************************************************************************
 *
 * $Id: maketags.c,v 1.8 2005/08/28 14:52:15 mbse Exp $
 * Purpose ...............: Make tag files
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
 *   
 * Michiel Broek		FIDO:		2:2801/16
 * Beekmansbos 10		Internet:	mbroek@ux123.pttnwb.nl
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
#include "maketags.h"



void MakeTags(void)
{
    FILE    *fg, *fd, *td, *ad;
    char    *gname, *dname, *tname, *aname;

    Syslog('+', "Start making tagfiles");
    gname = calloc(PATH_MAX, sizeof(char));
    dname = calloc(PATH_MAX, sizeof(char));
    tname = calloc(PATH_MAX, sizeof(char));
    aname = calloc(PATH_MAX, sizeof(char));

    snprintf(gname, PATH_MAX, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
    snprintf(dname, PATH_MAX, "%s/etc/mareas.data", getenv("MBSE_ROOT"));

    if (((fg = fopen(gname, "r")) == NULL) || ((fd = fopen(dname, "r")) == NULL)) {
	WriteError("$Can't open data");
    } else {
	fread(&mgrouphdr, sizeof(mgrouphdr), 1, fg);
	fread(&msgshdr, sizeof(msgshdr), 1, fd);

	while ((fread(&mgroup, mgrouphdr.recsize, 1, fg)) == 1) {
	    if (mgroup.Active) {
		snprintf(tname, PATH_MAX, "%s/share/doc/tags/%s.msgs.tag", getenv("MBSE_ROOT"), mgroup.Name);
		mkdirs(tname, 0755);
		td = fopen(tname, "w");
		snprintf(aname, PATH_MAX, "%s/share/doc/tags/%s.msgs.are", getenv("MBSE_ROOT"), mgroup.Name);
		ad = fopen(aname, "w");
		fprintf(ad, "; Mail areas in group %s\n", mgroup.Name);
		fprintf(ad, ";\n");

		fseek(fd, msgshdr.hdrsize, SEEK_SET);
		while ((fread(&msgs, msgshdr.recsize, 1, fd)) == 1) {
		    if (msgs.Active && strlen(msgs.Tag) && strcmp(mgroup.Name, msgs.Group) == 0) {
			fprintf(ad, "%-35s %s\n", msgs.Tag, msgs.Name);
			fprintf(td, "%s\n", msgs.Tag);
		    }

		    fseek(fd, msgshdr.syssize, SEEK_CUR);
		}
		fclose(ad);
		fclose(td);
	    }
	}
	fclose(fg);
	fclose(fd);
    }

    snprintf(gname, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
    snprintf(dname, PATH_MAX, "%s/etc/tic.data", getenv("MBSE_ROOT"));

    if (((fg = fopen(gname, "r")) == NULL) || ((fd = fopen(dname, "r")) == NULL)) {
	WriteError("$Can't open data");
    } else {
	fread(&fgrouphdr, sizeof(fgrouphdr), 1, fg);
	fread(&tichdr, sizeof(tichdr), 1, fd);

	while ((fread(&fgroup, fgrouphdr.recsize, 1, fg)) == 1) {
	    if (fgroup.Active) {
		snprintf(tname, PATH_MAX, "%s/share/doc/tags/%s.file.tag", getenv("MBSE_ROOT"), fgroup.Name);
		td = fopen(tname, "w");
		snprintf(aname, PATH_MAX, "%s/share/doc/tags/%s.file.are", getenv("MBSE_ROOT"), fgroup.Name);
		ad = fopen(aname, "w");
		fprintf(ad, "; TIC file areas in group %s\n", fgroup.Name);
		fprintf(ad, ";\n");

		fseek(fd, tichdr.hdrsize, SEEK_SET);
		while ((fread(&tic, tichdr.recsize, 1, fd)) == 1) {
		    if (tic.Active && strlen(tic.Name) && strcmp(fgroup.Name, tic.Group) == 0) {
			fprintf(ad, "%-21s %s\n", tic.Name, tic.Comment);
			fprintf(td, "%s\n", tic.Name);
		    }

		    fseek(fd, tichdr.syssize, SEEK_CUR);
		}
		fclose(ad);
		fclose(td);
	    }
	}
	fclose(fg);
	fclose(fd);
    }

    free(aname);
    free(tname);
    free(dname);
    free(gname);
}


