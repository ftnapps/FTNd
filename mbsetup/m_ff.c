/*****************************************************************************
 *
 * File ..................: m_ff.c
 * Purpose ...............: Filefind Setup
 * Last modification date : 29-Oct-2000
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
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
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "m_global.h"
#include "m_ff.h"
#include "m_lang.h"
#include "m_marea.h"


int	FilefindUpdated = 0;


/*
 * Count nr of scanmgr records in the database.
 * Creates the database if it doesn't exist.
 */
int CountFilefind(void)
{
	FILE	*fil;
	char	ffile[81];
	int	count;

	sprintf(ffile, "%s/etc/scanmgr.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			scanmgrhdr.hdrsize = sizeof(scanmgrhdr);
			scanmgrhdr.recsize = sizeof(scanmgr);
			fwrite(&scanmgrhdr, sizeof(scanmgrhdr), 1, fil);
			fclose(fil);
			return 0;
		} else
			return -1;
	}

	fread(&scanmgrhdr, sizeof(scanmgrhdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - scanmgrhdr.hdrsize) / scanmgrhdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenFilefind(void)
{
	FILE	*fin, *fout;
	char	fnin[81], fnout[81];
	long	oldsize;

	sprintf(fnin,  "%s/etc/scanmgr.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/scanmgr.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&scanmgrhdr, sizeof(scanmgrhdr), 1, fin);
			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize    = scanmgrhdr.recsize;
			if (oldsize != sizeof(scanmgr))
				FilefindUpdated = 1;
			else
				FilefindUpdated = 0;
			scanmgrhdr.hdrsize = sizeof(scanmgrhdr);
			scanmgrhdr.recsize = sizeof(scanmgr);
			fwrite(&scanmgrhdr, sizeof(scanmgrhdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&scanmgr, 0, sizeof(scanmgr));
			while (fread(&scanmgr, oldsize, 1, fin) == 1) {
				fwrite(&scanmgr, sizeof(scanmgr), 1, fout);
				memset(&scanmgr, 0, sizeof(scanmgr));
			}
			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseFilefind(void)
{
	char	fin[81], fout[81];
	FILE	*fi, *fo;
	st_list	*fff = NULL, *tmp;

	sprintf(fin, "%s/etc/scanmgr.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/scanmgr.temp", getenv("MBSE_ROOT"));

	if (FilefindUpdated == 1) {
		if (yes_no((char *)"Database is changed, save changes") == 1) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&scanmgrhdr, scanmgrhdr.hdrsize, 1, fi);
			fwrite(&scanmgrhdr, scanmgrhdr.hdrsize, 1, fo);

			while (fread(&scanmgr, scanmgrhdr.recsize, 1, fi) == 1)
				if (!scanmgr.Deleted)
					fill_stlist(&fff, scanmgr.Comment, ftell(fi) - scanmgrhdr.recsize);
			sort_stlist(&fff);

			for (tmp = fff; tmp; tmp=tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&scanmgr, scanmgrhdr.recsize, 1, fi);
				fwrite(&scanmgr, scanmgrhdr.recsize, 1, fo);
			}

			fclose(fi);
			fclose(fo);
			tidy_stlist(&fff);
			unlink(fout);
			Syslog('+', "Updated \"scanmgr.data\"");
			return;
		}
	}
	working(1, 0, 0);
	unlink(fout); 
}



int AppendFilefind(void)
{
	FILE	*fil;
	char	ffile[81];

	sprintf(ffile, "%s/etc/scanmgr.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&scanmgr, 0, sizeof(scanmgr));
		/*
		 * Fill in default values
		 */
		fwrite(&scanmgr, sizeof(scanmgr), 1, fil);
		fclose(fil);
		FilefindUpdated = 1;
		return 0;
	} else
		return -1;
}



void FFScreen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "13. EDIT FILEFIND AREAS");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.  Comment");
	mvprintw( 8, 2, "2.  Origin");
	mvprintw( 9, 2, "3.  Aka to use");
	mvprintw(10, 2, "4.  Scan area");
	mvprintw(11, 2, "5.  Reply area");
	mvprintw(12, 2, "6.  Language");
	mvprintw(13, 2, "7.  Template");
	mvprintw(14, 2, "8.  Active");
	mvprintw(15, 2, "9.  Deleted");
	mvprintw(16, 2, "10. Net. reply");
	mvprintw(17, 2, "11. Hi Ascii");
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditFfRec(int Area)
{
	FILE		*fil;
	char		mfile[81], temp1[2];
	long		offset;
	unsigned long	crc, crc1;
	int		i;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Filefind");

	sprintf(mfile, "%s/etc/scanmgr.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	fread(&scanmgrhdr, sizeof(scanmgrhdr), 1, fil);
	offset = scanmgrhdr.hdrsize + ((Area -1) * scanmgrhdr.recsize);
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&scanmgr, scanmgrhdr.recsize, 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&scanmgr, crc, scanmgrhdr.recsize);
	working(0, 0, 0);

	for (;;) {
		FFScreen();
		set_color(WHITE, BLACK);
		show_str(  7,18,55, scanmgr.Comment);
		show_str(  8,18,50, scanmgr.Origin);
		show_str(  9,18,35, aka2str(scanmgr.Aka));
		show_str( 10,18,50, scanmgr.ScanBoard);
		show_str( 11,18,50, scanmgr.ReplBoard);
		sprintf(temp1, "%c", scanmgr.Language);
		show_str( 12,18,2,  temp1);
		show_str( 13,18,14, scanmgr.template);
		show_bool(14,18,    scanmgr.Active);
		show_bool(15,18,    scanmgr.Deleted);
		show_bool(16,18,    scanmgr.NetReply);
		show_bool(17,18,    scanmgr.HiAscii);

		switch(select_menu(11)) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&scanmgr, crc1, scanmgrhdr.recsize);
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&scanmgr, scanmgrhdr.recsize, 1, fil);
					fclose(fil);
					FilefindUpdated = 1;
					working(0, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;

		case 1:	E_STR(  7,18,55, scanmgr.Comment,   "The ^comment^ for this area")
		case 2: E_STR(  8,18,50, scanmgr.Origin,    "The ^origin^ line to append, leave blank for random lines")
		case 3: i = PickAka((char *)"13.3", TRUE);
			if (i != -1)
				scanmgr.Aka = CFG.aka[i];
			break;
		case 4: strcpy(scanmgr.ScanBoard, PickMsgarea((char *)"13.4"));
			break;
		case 5: strcpy(scanmgr.ReplBoard, PickMsgarea((char *)"13.5"));
			break;
		case 6: scanmgr.Language = PickLanguage((char *)"13.6");
			break;
		case 7: E_STR( 13,18,14, scanmgr.template,  "The ^template^ file to use for the report")
		case 8: E_BOOL(14,18,    scanmgr.Active,    "If this report is ^active^")
		case 9: E_BOOL(15,18,    scanmgr.Deleted,   "If this record is ^deleted^")
		case 10:E_BOOL(16,18,    scanmgr.NetReply,  "If reply's via ^netmail^ instead of echomail")
		case 11:E_BOOL(17,18,    scanmgr.HiAscii,   "Allow ^Hi ASCII^ in this area")
		}
	}
}



void EditFilefind(void)
{
	int	records, i, o, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[81];
	long	offset;

	clr_index();
	working(1, 0, 0);
	IsDoing("Browsing Menu");
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	records = CountFilefind();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenFilefind() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 4, "13. FILEFIND AREAS");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/scanmgr.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&scanmgrhdr, sizeof(scanmgrhdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(scanmgrhdr) + (((o + i) - 1) * scanmgrhdr.recsize);
						fseek(fil, offset, 0);
						fread(&scanmgr, scanmgrhdr.recsize, 1, fil);
						if (scanmgr.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-32s", o + i, scanmgr.Comment);
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
			CloseFilefind();
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendFilefind() == 0) {
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
			EditFfRec(atoi(pick));
	}
}



int ff_doc(FILE *fp, FILE *toc, int page)
{
	char		temp[81];
	FILE		*no;
	int		j;

	sprintf(temp, "%s/etc/scanmgr.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	page = newpage(fp, page);
	addtoc(fp, toc, 13, 0, page, (char *)"Filefind areas");
	j = 0;

	fprintf(fp, "\n\n");
	fread(&scanmgrhdr, sizeof(scanmgrhdr), 1, no);

	while ((fread(&scanmgr, scanmgrhdr.recsize, 1, no)) == 1) {

		if (j == 5) {
			page = newpage(fp, page);
			fprintf(fp, "\n");
			j = 0;
		}

		fprintf(fp, "     Area comment      %s\n", scanmgr.Comment);
		fprintf(fp, "     Origin line       %s\n", scanmgr.Origin);
		fprintf(fp, "     Aka to use        %s\n", aka2str(scanmgr.Aka));
		fprintf(fp, "     Scan msg board    %s\n", scanmgr.ScanBoard);
		fprintf(fp, "     Reply msg board   %s\n", scanmgr.ReplBoard);
		fprintf(fp, "     Language          %c\n", scanmgr.Language);
		fprintf(fp, "     Active            %s\n", getboolean(scanmgr.Active));
		fprintf(fp, "     Netmail reply     %s\n", getboolean(scanmgr.NetReply));
		fprintf(fp, "\n\n");
		j++;
	}

	fclose(no);
	return page;
}


