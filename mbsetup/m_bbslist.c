/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Setup BBS lists.
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
 * MB BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MB BBS; see the file COPYING.  If not, write to the Free
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
#include "m_bbslist.h"



int	BBSlistUpdated = 0;


/*
 * Count nr of bbslist records in the database.
 * Creates the database if it doesn't exist.
 */
int CountBBSlist(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/bbslist.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "created new %s", ffile);
			bbshdr.hdrsize = sizeof(bbshdr);
			bbshdr.recsize = sizeof(bbs);
			fwrite(&bbshdr, sizeof(bbshdr), 1, fil);
			fclose(fil);
			chmod(ffile, 0660);
			return 0;
		} else
			return -1;
	}

	count = 0;
	fread(&bbshdr, sizeof(bbshdr), 1, fil);

	while (fread(&bbs, bbshdr.recsize, 1, fil) == 1) {
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
int OpenBBSlist(void);
int OpenBBSlist(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize;

	sprintf(fnin,  "%s/etc/bbslist.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/bbslist.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&bbshdr, sizeof(bbshdr), 1, fin);
			/*
			 * In case we are automaic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = bbshdr.recsize;
			if (oldsize != sizeof(bbs)) {
				BBSlistUpdated = 1;
				Syslog('+', "Upgraded %s, format changed", fnin);
			} else
				BBSlistUpdated = 0;
			bbshdr.hdrsize = sizeof(bbshdr);
			bbshdr.recsize = sizeof(bbs);
			fwrite(&bbshdr, sizeof(bbshdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&bbs, 0, sizeof(bbs));
			while (fread(&bbs, oldsize, 1, fin) == 1) {
				fwrite(&bbs, sizeof(bbs), 1, fout);
				memset(&bbs, 0, sizeof(bbs));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseBBSlist(int);
void CloseBBSlist(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];

	sprintf(fin, "%s/etc/bbslist.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/bbslist.temp", getenv("MBSE_ROOT"));

	if (BBSlistUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			if ((rename(fout, fin)) == 0)
				unlink(fout);
			chmod(fin, 0660);
			Syslog('+', "Updated \"bbslist.data\"");
			return;
		}
	}
	chmod(fin, 0660);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendBBSlist(void)
{
    FILE	*fil;
    char	ffile[PATH_MAX], buf[12];
    struct tm	*l_date;
    time_t	Time;

    Time = time(NULL);
    l_date = localtime(&Time);
    sprintf(buf, "%02d-%02d-%04d", l_date->tm_mday, l_date->tm_mon+1, l_date->tm_year+1900);

    sprintf(ffile, "%s/etc/bbslist.temp", getenv("MBSE_ROOT"));
    if ((fil = fopen(ffile, "a")) != NULL) {
	memset(&bbs, 0, sizeof(bbs));
	strcpy(bbs.DateOfEntry, buf);
	strcpy(bbs.UserName, CFG.sysop_name);
	fwrite(&bbs, sizeof(bbs), 1, fil);
	fclose(fil);
	BBSlistUpdated = 1;
	return 0;
    } else
	return -1;
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditBBSlistRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX], *temp, *aka;
	long	offset;
	int	j;
	unsigned long crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit bbslist");

	sprintf(mfile, "%s/etc/bbslist.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(bbshdr) + ((Area -1) * sizeof(bbs));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&bbs, sizeof(bbs), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&bbs, crc, sizeof(bbs));
	working(0, 0, 0);

	set_color(WHITE, BLACK);
	mvprintw( 4, 1, "8.6.1   EDIT BBSLIST");
	set_color(CYAN, BLACK);

	mvprintw( 6, 1, " 1. BBS");
	mvprintw( 7, 1, " 2. Desc.");
	mvprintw( 8, 1, " 3. Desc.");
	mvprintw( 9, 1, " 4. Phn 1");
	mvprintw(10, 1, " 5. Phn 2");
	mvprintw(11, 1, " 6. Phn 3");
	mvprintw(12, 1, " 7. Phn 4");
	mvprintw(13, 1, " 8. Phn 5");
	mvprintw(14, 1, " 9. TCPIP");
	mvprintw(15, 1, "10. Name");
	mvprintw(16, 1, "11. Sysop");
	mvprintw(17, 1, "12. Date");
	mvprintw(18, 1, "13. Lines");
	mvprintw(19, 1, "14. Open");
	
	mvprintw(16,46, "15. Verified");
	mvprintw(17,46, "16. Available");
	mvprintw(18,46, "17. Software");
	mvprintw(19,46, "18. Storage");

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 6,11,40, bbs.BBSName);
		show_str( 7,11,65, bbs.Desc[0]);
		show_str( 8,11,65, bbs.Desc[1]);
		show_str( 9,11,20, bbs.Phone[0]); show_str( 9,32,30, bbs.Speeds[0]); show_str( 9,63,16, aka2str(bbs.FidoAka[0]));
		show_str(10,11,20, bbs.Phone[1]); show_str(10,32,30, bbs.Speeds[1]); show_str(10,63,16, aka2str(bbs.FidoAka[1]));
		show_str(11,11,20, bbs.Phone[2]); show_str(11,32,30, bbs.Speeds[2]); show_str(11,63,16, aka2str(bbs.FidoAka[2]));
		show_str(12,11,20, bbs.Phone[3]); show_str(12,32,30, bbs.Speeds[3]); show_str(12,63,16, aka2str(bbs.FidoAka[3]));
		show_str(13,11,20, bbs.Phone[4]); show_str(13,32,30, bbs.Speeds[4]); show_str(13,63,16, aka2str(bbs.FidoAka[4]));
		show_str(14,11,50, bbs.IPaddress);
		show_str(15,11,36, bbs.UserName);
		show_str(16,11,36, bbs.Sysop);
		show_str(17,11,10, bbs.DateOfEntry);
		show_int(18,11,    bbs.Lines);
		show_str(19,11,20, bbs.Open);

		show_str(16,60,10, bbs.Verified);
		show_bool(17,60,   bbs.Available);
		show_str(18,60,20, bbs.Software);
		show_int(19,60,    bbs.Storage);

		j = select_menu(18);
		switch(j) {
		case 0:	crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&bbs, crc1, sizeof(bbs));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&bbs, sizeof(bbs), 1, fil);
					fclose(fil);
					BBSlistUpdated = 1;
					working(1, 0, 0);
					working(0, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1:	E_STR( 6,11,40, bbs.BBSName,     "The ^name^ of the BBS")
		case 2: E_STR( 7,11,65, bbs.Desc[0],     "The ^description^ of the BBS")
		case 3: E_STR( 8,11,65, bbs.Desc[1],     "The ^description^ of the BBS")
		case 4:
		case 5:
		case 6:
		case 7:
		case 8: temp = calloc(81, sizeof(char));
			aka  = calloc(31, sizeof(char));
			sprintf(temp, "The ^phone number^ for line %d", j-3);
			strncpy(bbs.Phone[j-4], edit_str(j+5,11,20,bbs.Phone[j-4], temp), 20);
			sprintf(temp, "The ^connect speeds^ for line %d", j-3);
			strncpy(bbs.Speeds[j-4], edit_str(j+5,32,30,bbs.Speeds[j-4], temp), 30);
			sprintf(aka, aka2str(bbs.FidoAka[j-4]));
			sprintf(temp, "The ^Fidonet aka^ for line %d", j-3);
			strncpy(aka, edit_str(j+5,63,16, aka, temp), 16);
			bbs.FidoAka[j-4] = str2aka(aka);
			free(aka);
			free(temp);
			break;
		case 9:	E_STR(14,11,50, bbs.IPaddress,   "The ^IP address^ or ^hostname^ of this BBS")
		case 10:E_STR(15,11,36, bbs.UserName,    "The ^username^ of the person who entered this BBS")
		case 11:E_STR(16,11,36, bbs.Sysop,       "The ^sysop name^ for this BBS")
		case 12:E_STR(17,11,10, bbs.DateOfEntry, "The ^date of entry^ for this BBS (DD-MM-JJJJ)")
		case 13:E_INT(18,11,    bbs.Lines,       "The ^number of lines^ for this BBS")
		case 14:E_STR(19,11,20, bbs.Open,        "The ^opening hours^ for this BBS")
		case 15:E_STR(16,60,10, bbs.Verified,    "The last ^verify date^ for this BBS (DD-MM-JJJJ)")
		case 16:E_BOOL(17,60,   bbs.Available,   "Is this BBS entry ^available^")
		case 17:E_STR(18,60,20, bbs.Software,    "The ^software^ this BBS is using")
		case 18:E_INT(19,60,    bbs.Storage,     "The ^files storage^ capacity in GBytes")
		}
	}

	return 0;
}



void EditBBSlist(void)
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

	records = CountBBSlist();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenBBSlist() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 2, "8.6.1 BBS LISTS SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/bbslist.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&bbshdr, sizeof(bbshdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(bbshdr) + (((o + i) - 1) * bbshdr.recsize);
						fseek(fil, offset, 0);
						fread(&bbs, bbshdr.recsize, 1, fil);
						if (bbs.Available)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-32s", o + i, bbs.BBSName);
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
			CloseBBSlist(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendBBSlist() == 0) {
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

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			EditBBSlistRec(atoi(pick));
			o = ((atoi(pick) - 1) / 20) * 20;
		}
	}
}



void InitBBSlist(void)
{
    CountBBSlist();
    OpenBBSlist();
    CloseBBSlist(TRUE);
}



void PurgeBBSlist(void)
{
	FILE	*pbbslist, *fp;
	int	recno = 0;
	int	iCount = 0;
	char	sFileName[PATH_MAX];
	char	temp[81];

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "8.6.2   BBS LISTS PURGE");
	set_color(CYAN, BLACK);
	working(1, 0, 0);

	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	IsDoing("Purge bbslists");

	sprintf(sFileName,"%s/etc/bbslist.data", getenv("MBSE_ROOT"));

	if ((pbbslist = fopen(sFileName, "r")) == NULL) {
		return;
	}

	fread(&bbshdr, sizeof(bbshdr), 1, pbbslist);
	while (fread(&bbs, bbshdr.recsize, 1, pbbslist) == 1) {
		recno++;
		if (!bbs.Available) 
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
		fseek(pbbslist, bbshdr.hdrsize, 0);
		fp = fopen("tmp.1", "a");
		fwrite(&bbshdr, sizeof(bbshdr), 1, fp);
		while (fread(&bbs, bbshdr.recsize, 1, pbbslist) == 1) {
			if (bbs.Available)
				fwrite(&bbs, bbshdr.recsize, 1, fp);
		}
		fclose(fp);
		fclose(pbbslist);
		if ((rename("tmp.1", sFileName)) != 0)
			working(2, 0, 0);
		unlink("tmp.1");
		working(0, 0, 0);	
	}
}



void bbslist_menu(void)
{
	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 6, "8.6   BBS LISTS SETUP");
		set_color(CYAN, BLACK);
		mvprintw( 7, 6, "1.    Edit BBS lists");
		mvprintw( 8, 6, "2.    Purge BBS lists");

		switch(select_menu(2)) {
		case 0:
			return;

		case 1:
			EditBBSlist();
			break;

		case 2:
			PurgeBBSlist();
			break;
		}
	}
}


