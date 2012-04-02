/*****************************************************************************
 *
 * $Id: m_ff.c,v 1.20 2007/03/05 12:25:20 mbse Exp $
 * Purpose ...............: Filefind Setup
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
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
	char	ffile[PATH_MAX];
	int	count;

	snprintf(ffile, PATH_MAX, "%s/etc/scanmgr.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			scanmgrhdr.hdrsize = sizeof(scanmgrhdr);
			scanmgrhdr.recsize = sizeof(scanmgr);
			fwrite(&scanmgrhdr, sizeof(scanmgrhdr), 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
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
int OpenFilefind(void);
int OpenFilefind(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	int	oldsize;

	snprintf(fnin,  PATH_MAX, "%s/etc/scanmgr.data", getenv("MBSE_ROOT"));
	snprintf(fnout, PATH_MAX, "%s/etc/scanmgr.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&scanmgrhdr, sizeof(scanmgrhdr), 1, fin);
			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize    = scanmgrhdr.recsize;
			if (oldsize != sizeof(scanmgr)) {
				FilefindUpdated = 1;
				Syslog('+', "Updated %s to new format", fnin);
			} else
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
				if (!strlen(scanmgr.template)) {
				    snprintf(scanmgr.template, 15, "filefind");
				    FilefindUpdated = 1;
				}
				if (!scanmgr.keywordlen) {
				    scanmgr.keywordlen = 3;
				    FilefindUpdated = 1;
				}
				if (scanmgr.charset == 0) {
				    scanmgr.charset = FTNC_CP437;
				    FilefindUpdated = 1;
				}
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



void CloseFilefind(int);
void CloseFilefind(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*fff = NULL, *tmp;

	snprintf(fin,  PATH_MAX, "%s/etc/scanmgr.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/scanmgr.temp", getenv("MBSE_ROOT"));

	if (FilefindUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
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
			chmod(fin, 0640);
			Syslog('+', "Updated \"scanmgr.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendFilefind(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	snprintf(ffile, PATH_MAX, "%s/etc/scanmgr.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&scanmgr, 0, sizeof(scanmgr));
		/*
		 * Fill in default values
		 */
		scanmgr.Language = 'E';
		snprintf(scanmgr.template, 15, "filefind");
		strncpy(scanmgr.Origin, CFG.origin, 50);
		scanmgr.keywordlen = 3;
		scanmgr.charset = FTNC_CP437;
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
	mbse_mvprintw( 5, 2, "13. EDIT FILEFIND AREAS");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 2, "1.  Comment");
	mbse_mvprintw( 8, 2, "2.  Origin");
	mbse_mvprintw( 9, 2, "3.  Aka to use");
	mbse_mvprintw(10, 2, "4.  Scan area");
	mbse_mvprintw(11, 2, "5.  Reply area");
	mbse_mvprintw(12, 2, "6.  Language");
	mbse_mvprintw(13, 2, "7.  Template");
	mbse_mvprintw(14, 2, "8.  Active");
	mbse_mvprintw(15, 2, "9.  Deleted");
	mbse_mvprintw(16, 2, "10. Net. reply");
	mbse_mvprintw(17, 2, "11. CHRS kludge");
	mbse_mvprintw(18, 2, "12. Keywrd len");
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditFfRec(int Area)
{
    FILE	    *fil;
    char	    mfile[PATH_MAX], temp1[2];
    int		    offset;
    unsigned int    crc, crc1;
    int		    i;

    clr_index();
    working(1, 0, 0);
    IsDoing("Edit Filefind");

    snprintf(mfile, PATH_MAX, "%s/etc/scanmgr.temp", getenv("MBSE_ROOT"));
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

    for (;;) {
	FFScreen();
	set_color(WHITE, BLACK);
	show_str(  7,18,55, scanmgr.Comment);
	show_str(  8,18,50, scanmgr.Origin);
	show_str(  9,18,35, aka2str(scanmgr.Aka));
	show_str( 10,18,50, scanmgr.ScanBoard);
	show_str( 11,18,50, scanmgr.ReplBoard);
	snprintf(temp1, 2, "%c", scanmgr.Language);
	show_str( 12,18,2,  temp1);
	show_str( 13,18,14, scanmgr.template);
	show_bool(14,18,    scanmgr.Active);
	show_bool(15,18,    scanmgr.Deleted);
	show_bool(16,18,    scanmgr.NetReply);
	show_charset(17,18, scanmgr.charset);
	show_int( 18,18,    scanmgr.keywordlen);
		
	switch(select_menu(12)) {
	    case 0: crc1 = 0xffffffff;
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
			    working(6, 0, 0);
			}
		    }
		    IsDoing("Browsing Menu");
		    return 0;
	    case 1: E_STR(  7,18,55, scanmgr.Comment,   "The ^comment^ for this area")
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
	    case 11:scanmgr.charset = edit_charset(17,18, scanmgr.charset);
		    break;
	    case 12:E_IRC( 18,18,    scanmgr.keywordlen, 3, 8, "Minimum ^keyword length^ to allowed for search")
	}
    }
}



void EditFilefind(void)
{
	int	records, i, o, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	int	offset;

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
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 4, "13. FILEFIND AREAS");
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/scanmgr.temp", getenv("MBSE_ROOT"));
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
						snprintf(temp, 81, "%3d.  %-32s", o + i, scanmgr.Comment);
						temp[37] = 0;
						mbse_mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		strcpy(pick, select_record(records, 20));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseFilefind(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendFilefind() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
		}

		if (strncmp(pick, "N", 1) == 0) 
			if ((o + 20) < records) 
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			EditFfRec(atoi(pick));
			o = ((atoi(pick) - 1) / 20) * 20;
		}
	}
}



void InitFilefind(void)
{
    CountFilefind();
    OpenFilefind();
    CloseFilefind(TRUE);
}



int ff_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *ti, *wp, *ip, *no;
    int	    refs, nr, i = 0, j;

    snprintf(temp, PATH_MAX, "%s/etc/scanmgr.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 13, 0, page, (char *)"Filefind areas");
    j = 0;

    fprintf(fp, "\n\n");
    fread(&scanmgrhdr, sizeof(scanmgrhdr), 1, no);

    ip = open_webdoc((char *)"filefind.html", (char *)"Filefind Areas", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<UL>\n");

    while ((fread(&scanmgr, scanmgrhdr.recsize, 1, no)) == 1) {

	i++;
	if (j == 4) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}

	snprintf(temp, 81, "filefind_%d.html", i);
	fprintf(ip, " <LI><A HREF=\"%s\">%3d %s</A></LI>\n", temp, i, scanmgr.Comment);
	if ((wp = open_webdoc(temp, (char *)"Filefind Area", scanmgr.Comment))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"filefind.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Area comment", scanmgr.Comment);
	    add_webtable(wp, (char *)"Origin line", scanmgr.Origin);
	    add_webtable(wp, (char *)"Aka to use", aka2str(scanmgr.Aka));
	    add_webtable(wp, (char *)"Scan msg board", scanmgr.ScanBoard);
	    add_webtable(wp, (char *)"Reply msg board", scanmgr.ReplBoard);
	    snprintf(temp, 81, "%c", scanmgr.Language);
	    add_webtable(wp, (char *)"Language", temp);
	    add_webtable(wp, (char *)"Template file", scanmgr.template);
	    add_webtable(wp, (char *)"Active", getboolean(scanmgr.Active));
	    add_webtable(wp, (char *)"Netmail reply", getboolean(scanmgr.NetReply));
	    add_webtable(wp, (char *)"CHRS kludge", getftnchrs(scanmgr.charset));
	    add_webdigit(wp, (char *)"Keyword length", scanmgr.keywordlen);
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");

	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>BBS File Areas Reference</H3>\n");
	    nr = refs = 0;
	    snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	    if ((ti = fopen(temp, "r"))) {
		fread(&areahdr, sizeof(areahdr), 1, ti);
		while ((fread(&area, areahdr.recsize, 1, ti)) == 1) {
		    nr++;
		    if (area.Available) {
			if (refs == 0) {
			    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
			    fprintf(wp, "<COL width='15%%'><COL width='15%%'><COL width='70%%'>\n");
			    fprintf(wp, "<TBODY>\n");
			    fprintf(wp, "<TR><TH align='left'>Area</TH><TH align='left'>Search</TH><TH align='left'>Description</TH></TD>\n");
			}
			fprintf(wp, "<TR><TD><A HREF=\"filearea_%d.html\">Area %d</A></TD><TD>%s</TD><TD>%s</TD></TR>\n", 
				nr, nr, getboolean(area.FileFind), area.Name);
			refs++;
		    }
		}
		fclose(ti);
	    }
	    if (refs == 0)
		fprintf(wp, "No BBS File Areas References\n");
	    else {
		fprintf(wp, "</TBODY>\n");
		fprintf(wp, "</TABLE>\n");
	    }
	    close_webdoc(wp);
	}

	fprintf(fp, "     Area comment      %s\n", scanmgr.Comment);
	fprintf(fp, "     Origin line       %s\n", scanmgr.Origin);
	fprintf(fp, "     Aka to use        %s\n", aka2str(scanmgr.Aka));
	fprintf(fp, "     Scan msg board    %s\n", scanmgr.ScanBoard);
	fprintf(fp, "     Reply msg board   %s\n", scanmgr.ReplBoard);
	fprintf(fp, "     Language          %c\n", scanmgr.Language);
	fprintf(fp, "     Template file     %s\n", scanmgr.template);
	fprintf(fp, "     Active            %s\n", getboolean(scanmgr.Active));
	fprintf(fp, "     Netmail reply     %s\n", getboolean(scanmgr.NetReply));
	fprintf(fp, "     CHRS kludge       %s\n", getftnchrs(scanmgr.charset));
	fprintf(fp, "     Keyword length    %d\n", scanmgr.keywordlen);
	fprintf(fp, "\n\n");
	j++;
    }

    fprintf(ip, "</UL>\n");
    close_webdoc(ip);

    fclose(no);
    return page;
}


