/*****************************************************************************
 *
 * $Id: m_fdb.c,v 1.24 2005/10/11 20:49:48 mbse Exp $
 * Purpose ...............: Edit Files DataBase.
 *
 *****************************************************************************
 * Copyright (C) 1999-2005
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
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_global.h"
#include "m_farea.h"
#include "m_fdb.h"


void E_F(int);
void EditFile(void);


void FHeader(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "14. EDIT FILE");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "    FileName");
    mbse_mvprintw( 8, 2, "    Long fn");
    mbse_mvprintw( 9, 2, "    FileSize");
    mbse_mvprintw(10, 2, "    FileDate");
    mbse_mvprintw(11, 2, "    Last DL.");
    mbse_mvprintw(12, 2, "    Upl.Date");
    mbse_mvprintw(13, 2, "    TIC area");
    mbse_mvprintw(14, 2, "    Magic");
    mbse_mvprintw(15, 2, "1.  Uploader");
    mbse_mvprintw(16, 2, "2.  Times DL");
    mbse_mvprintw(17, 2, "3.  Password");

    mbse_mvprintw(15,61, "4.  Deleted");
    mbse_mvprintw(16,61, "5.  No Kill");
    mbse_mvprintw(17,61, "6.  Announced");
}



void EditFile()
{
    FHeader();

    for (;;) {
	set_color(WHITE, BLACK);
	show_str( 7,16,12, fdb.Name);
	show_str( 8,16,64, fdb.LName);
	show_int( 9,16,    fdb.Size);
	mbse_mvprintw(10,16, (char *)"%s %s", StrDateDMY(fdb.FileDate), StrTimeHM(fdb.FileDate));
	mbse_mvprintw(11,16, (char *)"%s %s", StrDateDMY(fdb.LastDL), StrTimeHM(fdb.LastDL));
	mbse_mvprintw(12,16, (char *)"%s %s", StrDateDMY(fdb.UploadDate), StrTimeHM(fdb.UploadDate));
	show_str(13,16,20, fdb.TicArea);
	show_str(14,16,20, fdb.Magic);
	show_str(15,16,36, fdb.Uploader);
	show_int(16,16,    fdb.TimesDL);
	show_str(17,16,15, fdb.Password);

	show_bool(15,75, fdb.Deleted);
	show_bool(16,75, fdb.NoKill);
	show_bool(17,75, fdb.Announced);

	switch(select_menu(6)) {
	    case 0: return;
	    case 1: E_STR( 15,16,35, fdb.Uploader,  "The ^uploader^ of this file")
	    case 2: E_INT( 16,16,    fdb.TimesDL,   "The number of times file is sent with ^download^")
	    case 3: E_STR( 17,16,15, fdb.Password,  "The ^password^ to protect this file with")
	    case 4: E_BOOL(15,75,    fdb.Deleted,   "Should this this file be ^deleted^")
	    case 5: E_BOOL(16,75,    fdb.NoKill,    "File can't be ^killed^ automatic")
	    case 6: E_BOOL(17,75,    fdb.Announced, "File is ^announced^ as new file")
	}
    }
}



void E_F(int areanr)
{
    FILE	    *fil;
    char	    temp[PATH_MAX], help[81];
    int		    i, y, o, records, Ondisk;
    static char	    *menu = (char *)"0";
    int		    offset;
    time_t	    Time;
    struct stat	    statfile;
    unsigned int    crc, crc1;

    clr_index();

    snprintf(temp, PATH_MAX, "%s/var/fdb/file%d.data", getenv("MBSE_ROOT"), areanr);
    if ((fil = fopen(temp, "r+")) == NULL) {
	working(2, 0, 0);
	return;
    }
    if (! check_free())
	return;

    fread(&fdbhdr, sizeof(fdbhdr), 1, fil);
    fseek(fil, 0, SEEK_END);
    records = ((ftell(fil) - fdbhdr.hdrsize) / fdbhdr.recsize);
    o = 0;

    for (;;) {

	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw(5, 4, "14.  EDIT FILES DATABASE");

	y = 8;
	working(1, 0, 0);

	set_color(YELLOW, BLUE);
	mbse_mvprintw(7, 1, "  Nr   Filename           Size Date       Time  Description                   ");
/*                      1234   12345678901234 12345678 12-34-1998 12:45 123456789012345678901234567890*/
	set_color(CYAN, BLACK);

	for (i = 1; i <= 10; i++) {
	    if ((o + i) <= records) {
		offset = (((o + i) - 1) * fdbhdr.recsize) + fdbhdr.hdrsize;
		fseek(fil, offset, SEEK_SET);
		fread(&fdb, fdbhdr.recsize, 1, fil);

		set_color(WHITE, BLACK);
		mbse_mvprintw(y, 1, (char *)"%4d.", o + i);

		snprintf(temp, PATH_MAX, "%s/%s", area.Path, fdb.LName);
		Ondisk = ((stat(temp, &statfile)) != -1);

		if (fdb.Deleted)
		    set_color(LIGHTBLUE, BLACK);
		else if (Ondisk)
		    set_color(CYAN, BLACK);
		else
		    set_color(LIGHTRED, BLACK);
		mbse_mvprintw(y, 8, (char *)"%-14s", fdb.Name);

		if (Ondisk) {
		    if (fdb.Size == statfile.st_size)
			set_color(CYAN, BLACK);
		    else
			set_color(LIGHTRED, BLACK);
		    mbse_mvprintw(y,23, (char *)"%8ld", fdb.Size);

		    if (fdb.FileDate == statfile.st_mtime)
			set_color(CYAN, BLACK);
		    else
			set_color(LIGHTRED, BLACK);
		    Time = fdb.FileDate;
		    mbse_mvprintw(y,32, (char *)"%s %s", StrDateDMY(Time), StrTimeHM(Time));
		}

		set_color(CYAN, BLACK);
		snprintf(temp, 81, "%s", fdb.Desc[0]);
		temp[30] = '\0';
		mbse_mvprintw(y,49, (char *)"%s", temp);
		y++;
	    }
	}

	if (records)
	    if (records > 10)
		snprintf(help, 81, "^1..%d^ Edit, ^-^ Return, ^N^/^P^ Page", records);
	    else
		snprintf(help, 81, "^1..%d^ Edit, ^-^ Return", records); 
	else
	    snprintf(help, 81, "^-^ Return");

	showhelp(help);

	while(TRUE) {
	    mbse_mvprintw(LINES - 4, 6, "Enter your choice >");
	    menu = (char *)"-";
	    menu = edit_field(LINES - 4, 26, 6, '!', menu);
	    mbse_locate(LINES - 4, 6);
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
		offset = ((atoi(menu) - 1) * fdbhdr.recsize) + fdbhdr.hdrsize;
		fseek(fil, offset, SEEK_SET);
		fread(&fdb, fdbhdr.recsize, 1, fil);
		crc = 0xffffffff;
		crc = upd_crc32((char *)&fdb, crc, fdbhdr.recsize);
		o = ((atoi(menu) - 1) / 10) * 10;
				
		snprintf(temp, PATH_MAX, "%s/%s", area.Path, fdb.LName);
		EditFile();

		crc1 = 0xffffffff;
		crc1 = upd_crc32((char *)&fdb, crc1, fdbhdr.recsize);

		if (crc != crc1) {
		    if (yes_no((char *)"Record is changed, save") == 1) {
			working(1, 0, 0);
			fseek(fil, offset, SEEK_SET);
			fwrite(&fdb, fdbhdr.recsize, 1, fil);
			working(6, 0, 0);
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
    int	    offset;

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
    o = 0;

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 4, "14. EDIT FILES DATABSE");
	set_color(CYAN, BLACK);
	if (records != 0) {
	    snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
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
			snprintf(temp, 81, "%3d.  %-32s", o + i, area.Name);
			temp[37] = 0;
			mbse_mvprintw(y, x, temp);
			y++;
		    }
		}
		fclose(fil);
	    }
	}
	strcpy(pick, select_pick(records, 20));

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
	    snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
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



/*
 * Init files database. Since version 0.51.2 the format is changed.
 * Check this and automagic upgrade the database.
 */
void InitFDB(void)
{
    int			    records, i;
    int			    Area = 0;
    char		    *temp, Magic[21];
    FILE		    *fp1, *fp2, *fil, *ft, *fp;
    DIR			    *dp;
    struct dirent	    *de;
    struct OldFILERecord    old;
    struct stat		    sb;
    struct _fdbarea	    *fdb_area = NULL;

    records = CountFilearea();
    if (records <= 0)
	return;
    
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
    if ((fil = fopen(temp, "r")) != NULL) {
	fread(&areahdr, sizeof(areahdr), 1, fil);

	while (fread(&area, areahdr.recsize, 1, fil)) {
	    Area++;
	    if (area.Available) {
		snprintf(temp, PATH_MAX, "%s/var/fdb/fdb%d.data", getenv("MBSE_ROOT"), Area);
		if ((fp1 = fopen(temp, "r")) != NULL) {
		    /*
		     * Old area available, upgrade.
		     */
		    snprintf(temp, PATH_MAX, "%s/var/fdb/file%d.data", getenv("MBSE_ROOT"), Area);
		    if ((fp2 = fopen(temp, "w+")) == NULL) {
			WriteError("$Can't create %s", temp);
		    } else {
			fdbhdr.hdrsize = sizeof(fdbhdr);
			fdbhdr.recsize = sizeof(fdb);
			fwrite(&fdbhdr, sizeof(fdbhdr), 1, fp2);

			while (fread(&old, sizeof(old), 1, fp1)) {
			    Nopper();
			    memset(&fdb, 0, fdbhdr.recsize);
			    strncpy(fdb.Name, old.Name, sizeof(fdb.Name) -1);
			    strncpy(fdb.LName, old.LName, sizeof(fdb.LName) -1);
			    snprintf(temp, PATH_MAX, "%s/etc/tic.data", getenv("MBSE_ROOT"));
			    if ((ft = fopen(temp, "r")) != NULL) {
				fread(&tichdr, sizeof(tichdr), 1, ft);
				while (fread(&tic, tichdr.recsize, 1, ft)) {
				    if (StringCRC32(tic.Name) == old.TicAreaCRC) {
					strncpy(fdb.TicArea, tic.Name, sizeof(fdb.TicArea) -1);
					break;
				    }
				    fseek(ft, tichdr.syssize, SEEK_CUR);
				}
				fclose(ft);
			    }
			    fdb.Size = old.Size;
			    fdb.Crc32 = old.Crc32;
			    strncpy(fdb.Uploader, old.Uploader, sizeof(fdb.Uploader) -1);
			    fdb.UploadDate = old.UploadDate;
			    fdb.FileDate = old.FileDate;
			    fdb.LastDL = old.LastDL;
			    fdb.TimesDL = old.TimesDL + old.TimesFTP + old.TimesReq;
			    strncpy(fdb.Password, old.Password, sizeof(fdb.Password) -1);
			    for (i = 0; i < 25; i++)
				strncpy(fdb.Desc[i], old.Desc[i], 48);

			    /*
			     * Search the magic directory to see if this file is a magic file.
			     */
			    snprintf(temp, 81, "%s", CFG.req_magic);
			    if ((dp = opendir(temp)) != NULL) {
				while ((de = readdir(dp))) {
				    if (de->d_name[0] != '.') {
					snprintf(temp, PATH_MAX, "%s/%s", CFG.req_magic, de->d_name);
					/*
					 * Only regular files without execute permission are magic requests.
					 */
					if ((lstat(temp, &sb) != -1) && (S_ISREG(sb.st_mode)) && (! (sb.st_mode & S_IXUSR))) {
					    if ((fp = fopen(temp, "r"))) {
						fgets(Magic, sizeof(Magic) -1, fp);
						Striplf(Magic);
						if ((strcasecmp(Magic, fdb.Name) == 0) || (strcasecmp(Magic, fdb.LName) == 0)) {
						    strncpy(fdb.Magic, de->d_name, sizeof(fdb.Magic) -1);
						}
						fclose(fp);
					    }
					}
				    }
				}
				closedir(dp);
			    } else {
				WriteError("$Can't open directory %s", temp);
			    }
			    fdb.Deleted = old.Deleted;
			    fdb.NoKill = old.NoKill;
			    fdb.Announced = old.Announced;
			    fdb.Double = old.Double;
			    fwrite(&fdb, fdbhdr.recsize, 1, fp2);
			}
			fclose(fp2);
			Syslog('+', "Upgraded file area database %d", Area);
		    }
		    fclose(fp1);
		    snprintf(temp, PATH_MAX, "%s/var/fdb/fdb%d.data", getenv("MBSE_ROOT"), Area);
		    unlink(temp);
		} // Old area type upgrade.

		/*
		 * Current area, check
		 */
		if ((fdb_area = mbsedb_OpenFDB(Area, 30)) == NULL)
		    WriteError("InitFDB(): database area %d might be corrupt", Area);
		else
		    mbsedb_CloseFDB(fdb_area);
	    }
	}
	fclose(fil);
    }
    free(temp);
}


