/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Edit Files DataBase.
 *
 *****************************************************************************
 * Copyright (C) 1999-2002
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
#include "../lib/clcomm.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_global.h"
#include "m_farea.h"
#include "m_fdb.h"


void E_F(long);
void EditFile(void);


void FHeader(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "14. EDIT FILE");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "    FileName");
	mvprintw( 8, 2, "    Long fn");
	mvprintw( 9, 2, "    FileSize");
	mvprintw(10, 2, "    FileDate");
	mvprintw(11, 2, "    Last DL.");
	mvprintw(12, 2, "    Upl.Date");
	mvprintw(13, 2, "1.  Uploader");
	mvprintw(14, 2, "2.  Times DL");
	mvprintw(15, 2, "3.  Times FTP");
	mvprintw(16, 2, "4.  Times Req");
	mvprintw(17, 2, "5.  Password");
	mvprintw(18, 2, "6.  Cost");

	mvprintw(14,42, "7.  Free");
	mvprintw(15,42, "8.  Deleted");
	mvprintw(16,42, "    Missing");
	mvprintw(17,42, "9.  No Kill");
	mvprintw(18,42, "10. Announced");
}



void EditFile()
{
	FHeader();

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 7,16,12, file.Name);
		show_str( 8,16,64, file.LName);
		show_int( 9,16,    file.Size);
		mvprintw(10,16, (char *)"%s %s", StrDateDMY(file.FileDate), StrTimeHM(file.FileDate));
		mvprintw(11,16, (char *)"%s %s", StrDateDMY(file.LastDL), StrTimeHM(file.LastDL));
		mvprintw(12,16, (char *)"%s %s", StrDateDMY(file.UploadDate), StrTimeHM(file.UploadDate));
		show_str(13,16,36, file.Uploader);
		show_int(14,16,    file.TimesDL);
		show_int(15,16,    file.TimesFTP);
		show_int(16,16,    file.TimesReq);
		show_str(17,16,15, file.Password);
		show_int(18,16,    file.Cost);

		show_bool(14,56, file.Free);
		show_bool(15,56, file.Deleted);
		show_bool(16,56, file.Missing);
		show_bool(17,56, file.NoKill);
		show_bool(18,56, file.Announced);

		switch(select_menu(10)) {
			case 0: return;
			case 1: E_STR( 13,16,35, file.Uploader,  "The ^uploader^ of this file")
			case 2: E_INT( 14,16,    file.TimesDL,   "The number of times file is sent with ^download^")
			case 3: E_INT( 15,16,    file.TimesFTP,  "The number of times file is sent with ^FTP or WWW^")
			case 4: E_INT( 16,16,    file.TimesReq,  "The number of times file is sent with ^filerequest^")
			case 5: E_STR( 17,16,15, file.Password,  "The ^password^ to protect this file with")
			case 6: E_INT( 18,16,    file.Cost,      "The ^cost^ of this file")
			case 7: E_BOOL(14,56,    file.Free,      "If this file is a ^free^ download")
			case 8:	E_BOOL(15,56,    file.Deleted,   "Should this this file be ^deleted^")
			case 9: E_BOOL(17,56,    file.NoKill,    "File can't be ^killed^ automatic")
			case 10:E_BOOL(18,56,    file.Announced, "File is ^announced^ as new file")
		}
	}
}



void E_F(long areanr)
{
	FILE		*fil;
	char		temp[PATH_MAX];
	int		i, y, o, records, Ondisk;
	char		help[81];
	static	char	*menu = (char *)"0";
	long		offset;
	time_t		Time;
	struct	stat	statfile;
	unsigned long	crc, crc1;

	clr_index();

	sprintf(temp, "%s/fdb/fdb%ld.data", getenv("MBSE_ROOT"), areanr);
	if ((fil = fopen(temp, "r+")) == NULL) {
		working(2, 0, 0);
		return;
	}
        if (! check_free())
	    return;

	fseek(fil, 0, SEEK_END);
	records = ftell(fil) / sizeof(file);
	o = 0;

	for (;;) {

		clr_index();
		set_color(WHITE, BLACK);
		mvprintw(5, 4, "14.  EDIT FILES DATABASE");

		y = 8;
		working(1, 0, 0);

		set_color(YELLOW, BLUE);
		mvprintw(7, 1, "  Nr   Filename           Size Date       Time  Description                   ");
/*                              1234   12345678901234 12345678 12-34-1998 12:45 123456789012345678901234567890*/
		set_color(CYAN, BLACK);

		for (i = 1; i <= 10; i++) {
			if ((o + i) <= records) {
				offset = ((o + i) - 1) * sizeof(file);
				fseek(fil, offset, SEEK_SET);
				fread(&file, sizeof(file), 1, fil);

				set_color(WHITE, BLACK);
				mvprintw(y, 1, (char *)"%4d.", o + i);

				sprintf(temp, "%s/%s", area.Path, file.LName);
				Ondisk = ((stat(temp, &statfile)) != -1);

				if (Ondisk)
					set_color(CYAN, BLACK);
				else
					set_color(LIGHTRED, BLACK);
				mvprintw(y, 8, (char *)"%-14s", file.Name);

				if (Ondisk) {
					if (file.Size == statfile.st_size)
						set_color(CYAN, BLACK);
					else
						set_color(LIGHTRED, BLACK);
					mvprintw(y,23, (char *)"%8ld", file.Size);

					if (file.FileDate == statfile.st_mtime)
						set_color(CYAN, BLACK);
					else
						set_color(LIGHTRED, BLACK);
					Time = file.FileDate;
					mvprintw(y,32, (char *)"%s %s", StrDateDMY(Time), StrTimeHM(Time));
				}

				set_color(CYAN, BLACK);
				sprintf(temp, "%s", file.Desc[0]);
				temp[30] = '\0';
				mvprintw(y,49, (char *)"%s", temp);
				y++;
			}
		}
		working(0, 0, 0);

		if (records)
			if (records > 10)
				sprintf(help, "^1..%d^ Edit, ^-^ Return, ^N^/^P^ Page", records);
			else
				sprintf(help, "^1..%d^ Edit, ^-^ Return", records); 
		else
			sprintf(help, "^-^ Return");

		showhelp(help);

		while(TRUE) {
			mvprintw(LINES - 4, 6, "Enter your choice >");
			menu = (char *)"-";
			menu = edit_field(LINES - 4, 26, 6, '!', menu);
			locate(LINES - 4, 6);
			clrtoeol();

			if (strncmp(menu, "-", 1) == 0) {
				fclose(fil);
				open_bbs();
				return;
			}

			if (records > 10) {
				if (strncmp(menu, "N", 1) == 0)
					if ((o + 10) < records) {
						o += 10;
						break;
					}

				if (strncmp(menu, "P", 1) == 0)
					if ((o - 10) >= 0) {
						o -= 10;
						break;
					}
			}

			if ((atoi(menu) > 0) && (atoi(menu) <= records)) {
				working(1, 0, 0);
				offset = (atoi(menu) - 1) * sizeof(file);
				fseek(fil, offset, SEEK_SET);
				fread(&file, sizeof(file), 1, fil);
				crc = 0xffffffff;
				crc = upd_crc32((char *)&file, crc, sizeof(file));

				sprintf(temp, "%s/%s", area.Path, file.LName);
				if (stat(temp, &statfile) == -1)
					file.Missing = TRUE;

				EditFile();

				crc1 = 0xffffffff;
				crc1 = upd_crc32((char *)&file, crc1, sizeof(file));

				if (crc != crc1) {
					if (yes_no((char *)"Record is changed, save") == 1) {
						working(1, 0, 0);
						fseek(fil, offset, SEEK_SET);
						fwrite(&file, sizeof(file), 1, fil);
					}
				}
				break;
			}
		}

	}
}



void EditFDB()
{
    int     records, i, o, x, y;
    char    pick[12];
    FILE    *fil;
    char    temp[PATH_MAX];
    long    offset;

    clr_index();
    working(1, 0, 0);
    IsDoing("Browsing Menu");
    if (config_read() == -1) {
	working(2, 0, 0);
	return;
    }

    records = CountFilearea();
    if (records == -1) {
	working(2, 0, 0);
	return;
    }
    working(0, 0, 0);
    o = 0;

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 4, "14. EDIT FILES DATABSE");
	set_color(CYAN, BLACK);
	if (records != 0) {
	    sprintf(temp, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	    working(1, 0, 0);
	    if ((fil = fopen(temp, "r")) != NULL) {
		fread(&areahdr, sizeof(areahdr), 1, fil);
		x = 2;
		y = 7;
		set_color(CYAN, BLACK);
		for (i = 1; i <= 20; i++) {
		    if (i == 11) {
			x = 42;
			y = 7;
		    }
		    if ((o + i) <= records) {
			offset = sizeof(areahdr) + (((o + i) - 1) * areahdr.recsize);
			fseek(fil, offset, SEEK_SET);
			fread(&area, areahdr.recsize, 1, fil);
			if (area.Available)
			    set_color(CYAN, BLACK);
			else
			    set_color(LIGHTBLUE, BLACK);
			sprintf(temp, "%3d.  %-32s", o + i, area.Name);
			temp[37] = 0;
			mvprintw(y, x, temp);
			y++;
		    }
		}
		fclose(fil);
	    }
	}
	working(0, 0, 0);
	strcpy(pick, select_record(records, 20));

	if (strncmp(pick, "-", 1) == 0) {
	    return;
	}

	if (strncmp(pick, "N", 1) == 0) 
	    if ((o + 20) < records) 
		o = o + 20;

	if (strncmp(pick, "P", 1) == 0)
	    if ((o - 20) >= 0)
		o = o - 20;

	if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
	    sprintf(temp, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	    if ((fil = fopen(temp, "r")) != NULL) {
		offset = areahdr.hdrsize + ((atoi(pick) - 1) * areahdr.recsize);
		fseek(fil, offset, SEEK_SET);
		fread(&area, areahdr.recsize, 1, fil);
		fclose(fil);
		if (area.Available) {
		    E_F(atoi(pick));
		}
		o = ((atoi(pick) - 1) / 20) * 20;
	    }
	}
    }
}


