/*****************************************************************************
 *
 * $Id: dbmsgs.c,v 1.11 2005/10/11 20:49:42 mbse Exp $
 * Purpose ...............: Message areas record Access
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
#include "mbselib.h"
#include "users.h"
#include "mbsedb.h"


char		msgs_fil[PATH_MAX];	/* Database filename		   */
char		mgrp_fil[PATH_MAX];	/* Group database filename	   */
int		msgs_pos = -1;		/* Current record position	   */
int		mgrp_pos = -1;		/* Current group position	   */
unsigned int	msgs_crc = -1;		/* CRC value of current record     */
unsigned int	mgrp_crc = -1;		/* CRC value of group record	   */
static int	sysstart, sysrecord;



int InitMsgs(void)
{
	FILE	*fil;

	memset(&msgs, 0, sizeof(msgs));
	memset(&mgroup, 0, sizeof(mgroup));
	LoadConfig();
	sysstart = -1;

	snprintf(msgs_fil, PATH_MAX -1, "%s/etc/mareas.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(msgs_fil, "r")) == NULL)
		return FALSE;

	fread(&msgshdr, sizeof(msgshdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	msgs_cnt = (ftell(fil) - msgshdr.hdrsize) / (msgshdr.recsize + msgshdr.syssize);
	fclose(fil);

	snprintf(mgrp_fil, PATH_MAX -1, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	return TRUE;
}



int smsgarea(char *, int, int);
int smsgarea(char *what, int newsgroup, int bad)
{
    FILE    *fil;

    if ((fil = fopen(msgs_fil, "r")) == NULL) {
	return FALSE;
    }

    fread(&msgshdr, sizeof(msgshdr), 1, fil);

    while (fread(&msgs, msgshdr.recsize, 1, fil) == 1) {
	/*
	 * Mark the start of the connected systems records
	 * for later use and skip the system records.
	 */
	msgs_pos = ftell(fil) - msgshdr.recsize;
	sysstart = ftell(fil);
	fseek(fil, msgshdr.syssize, SEEK_CUR);
	if (((!strcasecmp(what, msgs.Tag) && !newsgroup && !bad) || 
	     (!strcmp(what, msgs.Newsgroup) && newsgroup && !bad) ||
	     (!strcmp(CFG.badboard, msgs.Base) && bad)) && msgs.Active) {
	    sysrecord = 0;
	    fclose(fil);
	    msgs_crc = 0xffffffff;
	    msgs_crc = upd_crc32((char *)&msgs, msgs_crc, msgshdr.recsize);
	    mgrp_pos = -1;
	    mgrp_crc = -1;

	    if (strlen(msgs.Group)) {
		if ((fil = fopen(mgrp_fil, "r")) != NULL) {
		    fread(&mgrouphdr, sizeof(mgrouphdr), 1, fil);
		    while ((fread(&mgroup, mgrouphdr.recsize, 1, fil)) == 1) {
			if (!strcmp(msgs.Group, mgroup.Name)) {
			    mgrp_pos = ftell(fil) - mgrouphdr.recsize;
			    mgrp_crc = 0xffffffff;
			    mgrp_crc = upd_crc32((char *)&mgroup, mgrp_crc, mgrouphdr.recsize);
			    break;
			}
		    }
		    fclose(fil);
		}
	    } else
		memset(&mgroup, 0, sizeof(mgroup));

	    return TRUE;
	}
    }
    sysstart = -1;
    msgs_crc = -1;
    msgs_pos = -1;
    fclose(fil);
    return FALSE;
}



int SearchMsgs(char *Area)
{
    return smsgarea(Area, FALSE, FALSE);
}



int SearchMsgsNews(char *Group)
{
    return smsgarea(Group, TRUE, FALSE);
}



int SearchBadBoard(void)
{
    return smsgarea((char *)"xXxX", FALSE, TRUE);
}



/*
 *  Check if system is connected
 */
int MsgSystemConnected(sysconnect Sys)
{
	FILE		*fil;
	sysconnect	T;

	if (sysstart == -1)
		return FALSE;

	if ((fil = fopen(msgs_fil, "r")) == NULL) 
		return FALSE;

	if (fseek(fil, sysstart, SEEK_SET) != 0) {
		fclose(fil);
		return FALSE;
	}

	while (ftell(fil) != (sysstart + msgshdr.syssize)) {
		fread(&T, sizeof(sysconnect), 1, fil);
		if ((T.aka.zone  == Sys.aka.zone) &&
		    (T.aka.net   == Sys.aka.net) &&
		    (T.aka.node  == Sys.aka.node) &&
		    (T.aka.point == Sys.aka.point)) {
			fclose(fil);
			return TRUE;
		}
	}

	fclose(fil);
	return FALSE;
}



/*
 *  Change system's status, if the Read or Write flags are clear,
 *  the connection will be erased, else updated or connected.
 */
int MsgSystemConnect(sysconnect *Sys, int New)
{
	FILE		*fil;
	sysconnect	T;

	if (sysstart == -1)
		return FALSE;

	if ((fil = fopen(msgs_fil, "r+")) == NULL)
		return FALSE;

	if (fseek(fil, sysstart, SEEK_SET) != 0) {
		fclose(fil);
		return FALSE;
	}

	while (ftell(fil) != (sysstart + msgshdr.syssize)) {
		fread(&T, sizeof(sysconnect), 1, fil);

		/*
		 *  For a new connection, search an empty slot.
		 */
		if (New && (!T.aka.zone)) {
			fseek(fil, - sizeof(sysconnect), SEEK_CUR);
			fwrite(Sys, sizeof(sysconnect), 1, fil);
			fclose(fil);
			return TRUE;
		}

		/*
		 *  If not new it is an update
		 */
		if ((!New) && (T.aka.zone == Sys->aka.zone) &&
		    (T.aka.net == Sys->aka.net) && (T.aka.node == Sys->aka.node) &&
		    (T.aka.point == Sys->aka.point)) {
			fseek(fil, - sizeof(sysconnect), SEEK_CUR);
			if ((!Sys->sendto) && (!Sys->receivefrom)) {
				/*
				 * It's a deletion, if the area is mandatory or
				 * the node is cutoff, refuse the deletion.
				 */
				if (msgs.Mandatory || T.cutoff) {
					fclose(fil);
					return FALSE;
				}
				memset(&T, 0, sizeof(sysconnect));
				fwrite(&T, sizeof(sysconnect), 1, fil);
			} else {
				/*
				 * It's a update, refuse it if the node is cutoff.
				 */
				if (T.cutoff) {
					fclose(fil);
					return FALSE;
				}
				fwrite(Sys, sizeof(sysconnect), 1, fil);
			}
			fclose(fil);
			return TRUE;
		}
	}

	fclose(fil);
	return FALSE;
}



int GetMsgSystem(sysconnect * Sys, int First)
{
	FILE	*fil;

	memset(Sys, 0, sizeof(sysconnect));
	if (sysstart == -1)
		return FALSE;

	if (First)
		sysrecord = 0;
	else
		sysrecord++;

	if (sysrecord >= CFG.toss_systems)
		return FALSE;

	if ((fil = fopen(msgs_fil, "r")) == NULL)
		return FALSE;

	if (fseek(fil, sysstart + (sysrecord * sizeof(sysconnect)), SEEK_SET) != 0) {
		fclose(fil);
		return FALSE;
	}

	if (fread(Sys, sizeof(sysconnect), 1, fil) == 1) {
		fclose(fil);
		return TRUE;
	}

	fclose(fil);
	return FALSE;
}



int SearchNetBoard(unsigned short zone, unsigned short net)
{
	FILE	*fil;

	mgrp_pos = -1;
	mgrp_crc = -1;

	if ((fil = fopen(msgs_fil, "r")) == NULL) {
		return FALSE;
	}

	fread(&msgshdr, sizeof(msgshdr), 1, fil);

	while (fread(&msgs, msgshdr.recsize, 1, fil) == 1) {
		fseek(fil, msgshdr.syssize, SEEK_CUR);
		if ((msgs.Type == NETMAIL) && (msgs.Active) && 
		    (zone == msgs.Aka.zone) && (net == msgs.Aka.net)) {
			msgs_pos = ftell(fil) - (msgshdr.recsize + msgshdr.syssize);
			msgs_crc = 0xffffffff;
			msgs_crc = upd_crc32((char *)&msgs, msgs_crc, msgshdr.recsize);
			fclose(fil);
			return TRUE;
		}
	}
	fclose(fil);
	msgs_pos = -1;
	msgs_crc = -1;
	sysstart = -1;
	return FALSE;
}



void UpdateMsgs()
{
	unsigned int	crc = 0xffffffff;
	FILE		*fil;

	if (msgs_pos == -1)
		return;

	crc = upd_crc32((char *)&msgs, crc, msgshdr.recsize);
	if (crc != msgs_crc) {
		if ((fil = fopen(msgs_fil, "r+")) == NULL) {
			msgs_pos = -1;
			return;
		}
		fseek(fil, msgs_pos, SEEK_SET);
		fwrite(&msgs, msgshdr.recsize, 1, fil);
		fclose(fil);
	}
	msgs_pos = -1;
	msgs_crc = -1;

	if (mgrp_pos == -1)
		return;

	crc = 0xffffffff;
	crc = upd_crc32((char *)&mgroup, crc, mgrouphdr.recsize);
	if (crc != mgrp_crc) {
		if ((fil = fopen(mgrp_fil, "r+")) == NULL) {
			mgrp_pos = -1;
			return;
		}
		fseek(fil, mgrp_pos, SEEK_SET);
		fwrite(&mgroup, mgrouphdr.recsize, 1, fil);
		fclose(fil);
	}
	mgrp_pos = -1;
	mgrp_crc = -1;
}


