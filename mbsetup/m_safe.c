/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Setup Safecracker Data
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
 *   
 * Michiel Broek		FIDO:		2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE safe.
 *
 * This safe is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MB safe is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB safe; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/memwatch.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_global.h"
#include "m_safe.h"



int	SafeUpdated = 0;


/*
 * Count nr of safecracker records in the database.
 * Creates the database if it doesn't exist.
 */
int CountSafe(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/safe.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "created new %s", ffile);
			safehdr.hdrsize = sizeof(safehdr);
			safehdr.recsize = sizeof(safe);
			fwrite(&safehdr, sizeof(safehdr), 1, fil);
			fclose(fil);
			chmod(ffile, 0660);
			return 0;
		} else
			return -1;
	}

	count = 0;
	fread(&safehdr, sizeof(safehdr), 1, fil);

	while (fread(&safe, safehdr.recsize, 1, fil) == 1) {
		count++;
	}
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenSafe(void);
int OpenSafe(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize;

	sprintf(fnin,  "%s/etc/safe.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/safe.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&safehdr, sizeof(safehdr), 1, fin);
			/*
			 * In case we are automaic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = safehdr.recsize;
			if (oldsize != sizeof(safe)) {
				SafeUpdated = 1;
				Syslog('+', "Upgraded %s, format changed", fnin);
			} else
				SafeUpdated = 0;
			safehdr.hdrsize = sizeof(safehdr);
			safehdr.recsize = sizeof(safe);
			fwrite(&safehdr, sizeof(safehdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&safe, 0, sizeof(safe));
			while (fread(&safe, oldsize, 1, fin) == 1) {
				fwrite(&safe, sizeof(safe), 1, fout);
				memset(&safe, 0, sizeof(safe));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseSafe(int);
void CloseSafe(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];

	sprintf(fin, "%s/etc/safe.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/safe.temp", getenv("MBSE_ROOT"));

	if (SafeUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			if ((rename(fout, fin)) == 0)
				unlink(fout);
			chmod(fin, 0660);
			Syslog('+', "Updated \"safe.data\"");
			return;
		}
	}
	chmod(fin, 0660);
	working(1, 0, 0);
	unlink(fout); 
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditSafeRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX];
	long	offset;
	int	j, Open;
	unsigned long crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Safe");

	sprintf(mfile, "%s/etc/safe.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(safehdr) + ((Area -1) * sizeof(safe));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&safe, sizeof(safe), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&safe, crc, sizeof(safe));
	working(0, 0, 0);

	set_color(WHITE, BLACK);
	mvprintw( 5, 5, "8.8     EDIT safe");
	set_color(CYAN, BLACK);

	mvprintw( 7, 5, "1. Name");
	mvprintw( 8, 5, "2. Date");
	mvprintw( 9, 5, "3. Tries");
	mvprintw(10, 5, "4. Opended");

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 7,17,35, safe.Name);
		show_str( 8,17,10, safe.Date);
		show_int( 9,17,    safe.Trys);
		show_bool(10,17,   safe.Opened);

		j = select_menu(4);
		switch(j) {
		case 0:	crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&safe, crc1, sizeof(safe));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&safe, sizeof(safe), 1, fil);
					fclose(fil);
					SafeUpdated = 1;
					working(1, 0, 0);
					working(0, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1:	E_STR( 7,17,35, safe.Name,     "The Safecrackers ^Username^")
		case 2: E_STR( 8,17,10, safe.Date,     "The ^last date^ attempt to open the safe (DD-MM-YYYY)")
		case 3: E_INT( 9,17,    safe.Trys,     "The ^number of tries^ for today to open the safe")
		case 4: Open = edit_bool(10,17, safe.Opened, (char *)"Set or reset the safe ^cracked^ status");
			if (safe.Opened && !Open) {
			    safe.Opened = FALSE;
			    safe.Trys = 0;
			}
			break;
		}
	}

	return 0;
}



void EditSafe(void)
{
	int	records, i, x, y, o;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	long	offset;

	clr_index();
	working(1, 0, 0);
	IsDoing("Browsing Menu");
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	records = CountSafe();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenSafe() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 5, "8.8  SAFE CRACKER SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/safe.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&safehdr, sizeof(safehdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(safehdr) + (((o + i) - 1) * safehdr.recsize);
						fseek(fil, offset, 0);
						fread(&safe, safehdr.recsize, 1, fil);
						if (safe.Opened)
							set_color(LIGHTRED, BLACK);
						else
							set_color(CYAN, BLACK);
						sprintf(temp, "%3d.  %-32s", o + i, safe.Name);
						temp[38] = '\0';
						mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
		strcpy(pick, select_pick(records,20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseSafe(FALSE);
			return;
		}

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 20) < records)
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			EditSafeRec(atoi(pick));
			o = ((atoi(pick) - 1) / 20) * 20;
		}
	}
}



void InitSafe(void)
{
    CountSafe();
    OpenSafe();
    CloseSafe(TRUE);
}


