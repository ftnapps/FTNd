/*****************************************************************************
 *
 * $Id: dbtic.c,v 1.9 2005/10/11 20:49:42 mbse Exp $
 * Purpose ...............: Tic areas record Access
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



char		tic_fil[PATH_MAX];	/* Database filename		   */
char		tgrp_fil[PATH_MAX];	/* Group database filename	   */
int		tic_pos = -1;		/* Current record position	   */
int		tgrp_pos = -1;		/* Current group position	   */
unsigned int	tic_crc = -1;		/* CRC value of current record     */
unsigned int	tgrp_crc = -1;		/* CRC value of group record	   */
static int	sysstart, sysrecord;



int InitTic(void)
{
	FILE	*fil;

	memset(&tic, 0, sizeof(tic));
	memset(&fgroup, 0, sizeof(fgroup));
	LoadConfig();
	sysstart = -1;

	snprintf(tic_fil, PATH_MAX -1, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(tic_fil, "r")) == NULL)
		return FALSE;

	fread(&tichdr, sizeof(tichdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	tic_cnt = (ftell(fil) - tichdr.hdrsize) / (tichdr.recsize + tichdr.syssize);
	fclose(fil);

	snprintf(tgrp_fil, PATH_MAX -1, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	return TRUE;
}



int SearchTic(char *Area)
{
	FILE	*fil;

	if ((fil = fopen(tic_fil, "r")) == NULL) {
		return FALSE;
	}

	fread(&tichdr, sizeof(tichdr), 1, fil);

	while (fread(&tic, tichdr.recsize, 1, fil) == 1) {
		/*
		 * Mark the start of the connected systems records
		 * for later use and skip the system records.
		 */
		tic_pos = ftell(fil) - tichdr.recsize;
		sysstart = ftell(fil);
		fseek(fil, tichdr.syssize, SEEK_CUR);
		if (!strcasecmp(Area, tic.Name) && tic.Active) {
			sysrecord = 0;
			fclose(fil);
			tic_crc = 0xffffffff;
			tic_crc = upd_crc32((char *)&tic, tic_crc, tichdr.recsize);
			tgrp_pos = -1;
			tgrp_crc = -1;

			if (strlen(tic.Group)) {
				if ((fil = fopen(tgrp_fil, "r")) != NULL) {
					fread(&fgrouphdr, sizeof(fgrouphdr), 1, fil);
					while ((fread(&fgroup, fgrouphdr.recsize, 1, fil)) == 1) {
						if (!strcmp(tic.Group, fgroup.Name)) {
							tgrp_pos = ftell(fil) - fgrouphdr.recsize;
							tgrp_crc = 0xffffffff;
							tgrp_crc = upd_crc32((char *)&fgroup, tgrp_crc, fgrouphdr.recsize);
							break;
						}
					}
					fclose(fil);
				}
			} else
				memset(&fgroup, 0, sizeof(fgroup));

			return TRUE;
		}
	}
	sysstart = -1;
	tic_crc = -1;
	tic_pos = -1;
	fclose(fil);
	return FALSE;
}



/*
 *  Check if system is connected
 */
int TicSystemConnected(sysconnect Sys)
{
	FILE		*fil;
	sysconnect	T;

	if (sysstart == -1)
		return FALSE;

	if ((fil = fopen(tic_fil, "r")) == NULL)
		return FALSE;

	if (fseek(fil, sysstart, SEEK_SET) != 0) {
		fclose(fil);
		return FALSE;
	}

	while (ftell(fil) != (sysstart + tichdr.syssize)) {
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
 *  Change the system's status, if the Read and Write flags are clear,
 *  the connection will be erased, else updated or connected.
 */
int TicSystemConnect(sysconnect *Sys, int New)
{
	FILE		*fil;
	sysconnect	T;

	if (sysstart == -1)
		return FALSE;

	if ((fil = fopen(tic_fil, "r+")) == NULL)
		return FALSE;

	if (fseek(fil, sysstart, SEEK_SET) != 0) {
		fclose(fil);
		return FALSE;
	}

	while (ftell(fil) != (sysstart + tichdr.syssize)) {
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
		    (T.aka.node  == Sys->aka.node) &&
		    (T.aka.net   == Sys->aka.net) &&
		    (T.aka.point == Sys->aka.point)) {
			fseek(fil, - sizeof(sysconnect), SEEK_CUR);
			if ((!Sys->sendto) && (!Sys->receivefrom)) {
				/*
				 *  It's a deletion
				 */
				if (tic.Mandat) {
					fclose(fil);
					return FALSE;
				}
				memset(&T, 0, sizeof(sysconnect));
				fwrite(&T, sizeof(sysconnect), 1, fil);
			} else {
				/*
				 *  It's a update
				 */
				fwrite(Sys, sizeof(sysconnect), 1, fil);
			}
			fclose(fil);
			return TRUE;
		}
	}

	fclose(fil);
	return FALSE;
}



int GetTicSystem(sysconnect * Sys, int First)
{
	FILE	*fil;

	memset(Sys, 0, sizeof(sysconnect));
	if (sysstart == -1)
		return FALSE;

	if (First)
		sysrecord = 0;
	else
		sysrecord++;

	if (sysrecord >= CFG.tic_systems)
		return FALSE;

	if ((fil = fopen(tic_fil, "r")) == NULL)
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



void UpdateTic()
{
	unsigned int	crc = 0xffffffff;
	FILE		*fil;

	if (tic_pos == -1)
		return;

	crc = upd_crc32((char *)&tic, crc, tichdr.recsize);
	if (crc != tic_crc) {
		if ((fil = fopen(tic_fil, "r+")) == NULL) {
			tic_pos = -1;
			return;
		}
		fseek(fil, tic_pos, SEEK_SET);
		fwrite(&tic, tichdr.recsize, 1, fil);
		fclose(fil);
	}
	tic_pos = -1;
	tic_crc = -1;

	if (tgrp_pos == -1)
		return;

	crc = 0xffffffff;
	crc = upd_crc32((char *)&fgroup, crc, fgrouphdr.recsize);
	if (crc != tgrp_crc) {
		if ((fil = fopen(tgrp_fil, "r+")) == NULL) {
			tgrp_pos = -1;
			return;
		}
		fseek(fil, tgrp_pos, SEEK_SET);
		fwrite(&fgroup, fgrouphdr.recsize, 1, fil);
		fclose(fil);
	}
	tgrp_pos = -1;
	tgrp_crc = -1;
}


