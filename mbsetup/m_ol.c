/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Setup Oneliners.
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_global.h"
#include "m_ol.h"



int	OnelUpdated = 0;


/*
 * Count nr of oneline records in the database.
 * Creates the database if it doesn't exist.
 */
int CountOneline(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/oneline.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "created new %s", ffile);
			olhdr.hdrsize = sizeof(olhdr);
			olhdr.recsize = sizeof(ol);
			fwrite(&olhdr, sizeof(olhdr), 1, fil);
			fclose(fil);
			chmod(ffile, 0660);
			return 0;
		} else
			return -1;
	}

	count = 0;
	fread(&olhdr, sizeof(olhdr), 1, fil);

	while (fread(&ol, olhdr.recsize, 1, fil) == 1) {
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
int OpenOneline(void);
int OpenOneline(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize;

	sprintf(fnin,  "%s/etc/oneline.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/oneline.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&olhdr, sizeof(olhdr), 1, fin);
			/*
			 * In case we are automaic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = olhdr.recsize;
			if (oldsize != sizeof(ol)) {
				OnelUpdated = 1;
				Syslog('+', "Upgraded %s, format changed", fnin);
			} else
				OnelUpdated = 0;
			olhdr.hdrsize = sizeof(olhdr);
			olhdr.recsize = sizeof(ol);
			fwrite(&olhdr, sizeof(olhdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&ol, 0, sizeof(ol));
			while (fread(&ol, oldsize, 1, fin) == 1) {
				fwrite(&ol, sizeof(ol), 1, fout);
				memset(&ol, 0, sizeof(ol));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseOneline(int);
void CloseOneline(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];

	sprintf(fin, "%s/etc/oneline.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/oneline.temp", getenv("MBSE_ROOT"));

	if (OnelUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			if ((rename(fout, fin)) == 0)
				unlink(fout);
			chmod(fin, 0660);
			Syslog('+', "Updated \"oneline.data\"");
			return;
		}
	}
	working(1, 0, 0);
	unlink(fout); 
}



int AppendOneline(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	sprintf(ffile, "%s/etc/oneline.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&ol, 0, sizeof(ol));
		fwrite(&ol, sizeof(ol), 1, fil);
		fclose(fil);
		OnelUpdated = 1;
		return 0;
	} else
		return -1;
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditOnelRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX];
	long	offset;
	int	j;
	unsigned long crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Oneline");

	sprintf(mfile, "%s/etc/oneline.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(olhdr) + ((Area -1) * sizeof(ol));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&ol, sizeof(ol), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&ol, crc, sizeof(ol));
	working(0, 0, 0);

	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "8.8.1   EDIT ONELINER");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.  Text");
	mvprintw( 8, 2, "2.  User");
	mvprintw( 9, 2, "3.  Date");
	mvprintw(10, 2, "4.  Avail");

	for (;;) {
		set_color(WHITE, BLACK);
		show_str(  7,12,68, ol.Oneline);
		show_str(  8,12,35, ol.UserName);
		show_str(  9,12,10, ol.DateOfEntry);
		show_bool(10,12,    ol.Available);

		j = select_menu(4);
		switch(j) {
		case 0:	crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&ol, crc1, sizeof(ol));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&ol, sizeof(ol), 1, fil);
					fclose(fil);
					OnelUpdated = 1;
					working(1, 0, 0);
					working(0, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1:	E_STR(  7,12,68,ol.Oneline,    "The ^Oneline^ text to show")
		case 2:	E_STR(  8,12,30,ol.UserName,   "The ^Username^ of the owner of this oneline")
		case 3:	E_STR(  9,12,10,ol.DateOfEntry,"The ^Date^ this oneliner is added, format: ^DD-MM-YYYY^")
		case 4:	E_BOOL(10,12,   ol.Available,  "Is this oneline ^available^")
		}
	}

	return 0;
}



void EditOneline(void)
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

	records = CountOneline();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenOneline() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 2, "8.8.1 ONELINERS SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/oneline.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&olhdr, sizeof(olhdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(olhdr) + (((o + i) - 1) * olhdr.recsize);
						fseek(fil, offset, 0);
						fread(&ol, olhdr.recsize, 1, fil);
						if (ol.Available)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-32s", o + i, ol.Oneline);
						temp[38] = '\0';
						mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
		strcpy(pick, select_record(records,20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseOneline(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendOneline() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
			working(0, 0, 0);
		}

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 20) < records)
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records))
			EditOnelRec(atoi(pick));
	}
}



void InitOneline(void)
{
    CountOneline();
    OpenOneline();
    CloseOneline(TRUE);
}



void PurgeOneline(void)
{
	FILE	*pOneline, *fp;
	int	recno = 0;
	int	iCount = 0;
	char	sFileName[PATH_MAX];
	char	temp[81];

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "7.8.2   ONELINERS PURGE");
	set_color(CYAN, BLACK);
	working(1, 0, 0);

	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	IsDoing("Purge Oneliners");

	sprintf(sFileName,"%s/etc/oneline.data", getenv("MBSE_ROOT"));

	if ((pOneline = fopen(sFileName, "r")) == NULL) {
		return;
	}

	fread(&olhdr, sizeof(olhdr), 1, pOneline);
	while (fread(&ol, olhdr.recsize, 1, pOneline) == 1) {
		recno++;
		if (!ol.Available) 
			iCount++;
	}
	working(0, 0, 0);

	sprintf(temp, "%d records, %d records to purge", recno, iCount);
	mvprintw(7, 6, temp);
	if (iCount == 0) {
		mvprintw(9, 6, "Press any key");
		readkey(9, 20, LIGHTGRAY, BLACK);
		return;
	}

	if (yes_no((char *)"Purge deleted records") == TRUE) {
		working(1, 0, 0);
		fseek(pOneline, olhdr.hdrsize, 0);
		fp = fopen("tmp.1", "a");
		fwrite(&olhdr, sizeof(olhdr), 1, fp);
		while (fread(&ol, olhdr.recsize, 1, pOneline) == 1) {
			if (ol.Available)
				fwrite(&ol, olhdr.recsize, 1, fp);
		}
		fclose(fp);
		fclose(pOneline);
		if ((rename("tmp.1", sFileName)) != 0)
			working(2, 0, 0);
		unlink("tmp.1");
		working(0, 0, 0);	
	}
}



void ImportOneline(void)
{
	FILE	*Imp, *pOneline;
	char	temp[PATH_MAX];
	int	recno = 0;
	struct	tm *l_date;
	char	buf[12];
	time_t	Time;

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw(5, 6, "8.8.3  IMPORT ONELINERS");
	set_color(CYAN, BLACK);
	memset(&temp, 0, sizeof(temp));
	strcpy(temp, edit_str(21, 6,64, temp, (char *)"The ^full path and filename^ of the file to import"));
	if (strlen(temp) == 0)
		return;

	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	if ((Imp = fopen(temp, "r")) == NULL) {
		working(2, 0, 0);
		working(0, 0, 0);
		mvprintw(21, 6, temp);
		readkey(22, 6, LIGHTGRAY, BLACK);
		return;
	}

	sprintf(temp, "%s/etc/oneline.data", getenv("MBSE_ROOT"));

	/*
	 * Check if database exists, if not create a new one
	 */
	if ((pOneline = fopen(temp, "r" )) == NULL) {
		if ((pOneline = fopen(temp, "w")) != NULL) {
			olhdr.hdrsize = sizeof(olhdr);
			olhdr.recsize = sizeof(ol);
			fwrite(&olhdr, sizeof(olhdr), 1, pOneline);
			fclose(pOneline);
		}
	} else
		fclose(pOneline);

	/*
	 * Open database for appending
	 */
	if ((pOneline = fopen(temp, "a+")) == NULL) {
		working(2, 0, 0);
		working(0, 0, 0);
		fclose(Imp);
		mvprintw(21, 6, temp);
		readkey(22, 6, LIGHTGRAY, BLACK);
		return;
	}

	time(&Time);
	l_date = localtime(&Time);
	sprintf(buf, "%02d-%02d-%04d", l_date->tm_mday, l_date->tm_mon+1, l_date->tm_year+1900);

	while ((fgets(temp, 80, Imp)) != NULL) {
		Striplf(temp);
		if ((strlen(temp) > 0) && (strlen(temp) < 78)) {
			memset(&ol, 0, sizeof(ol));
			strcpy(ol.Oneline, temp);
			strcpy(ol.UserName, CFG.sysop_name);
			strcpy(ol.DateOfEntry, buf);
			ol.Available = TRUE;
			fwrite(&ol, sizeof(ol), 1, pOneline);
			recno++;
		}
	}

	fclose(Imp);
	fclose(pOneline);
	working(0, 0, 0);

	sprintf(temp, "Imported %d records", recno);
	mvprintw(21, 6, temp);
	readkey(21, 27, LIGHTGRAY, BLACK);
}



void ol_menu(void)
{
	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 6, "8.8   ONELINER SETUP");
		set_color(CYAN, BLACK);
		mvprintw( 7, 6, "1.    Edit Oneliners");
		mvprintw( 8, 6, "2.    Purge Oneliners");
		mvprintw( 9, 6, "3.    Import Oneliners");

		switch(select_menu(3)) {
		case 0:
			return;

		case 1:
			EditOneline();
			break;

		case 2:
			PurgeOneline();
			break;

		case 3:
			ImportOneline();
			break;

		}
	}
}


