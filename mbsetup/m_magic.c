/*****************************************************************************
 *
 * $Id: m_magic.c,v 1.22 2005/10/11 20:49:49 mbse Exp $
 * Purpose ...............: Edit Magics
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
#include "m_ticarea.h"
#include "m_global.h"
#include "m_magic.h"



int	MagicUpdated = 0;


/*
 * Count nr of magic records in the database.
 * Creates the database if it doesn't exist.
 */
int CountMagics(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	snprintf(ffile, PATH_MAX, "%s/etc/magic.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			magichdr.hdrsize = sizeof(magichdr);
			magichdr.recsize = sizeof(magic);
			fwrite(&magichdr, sizeof(magichdr), 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
			return 0;
		} else
			return -1;
	}

	fread(&magichdr, sizeof(magichdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - magichdr.hdrsize) / magichdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenMagics(void);
int OpenMagics(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	int	oldsize;
	int	FieldPatch = FALSE;

	snprintf(fnin,  PATH_MAX, "%s/etc/magic.data", getenv("MBSE_ROOT"));
	snprintf(fnout, PATH_MAX, "%s/etc/magic.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&magichdr, sizeof(magichdr), 1, fin);
			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = magichdr.recsize;
			if (oldsize != sizeof(magic)) {
				MagicUpdated = 1;
				Syslog('+', "Updated %s, format changed", fnin);
				if ((oldsize + 16) == sizeof(magic)) {
					FieldPatch = TRUE;
					Syslog('?', "Magic: performing FieldPatch");
				}
			} else
				MagicUpdated = 0;
			magichdr.hdrsize = sizeof(magichdr);
			magichdr.recsize = sizeof(magic);
			fwrite(&magichdr, sizeof(magichdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&magic, 0, sizeof(magic));
			while (fread(&magic, oldsize, 1, fin) == 1) {
				if (FieldPatch) {
					memmove(&magic.Path, &magic.From[13], oldsize-12);
					memset(&magic.From[13], 0, 8);
				}
				fwrite(&magic, sizeof(magic), 1, fout);
				memset(&magic, 0, sizeof(magic));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseMagics(int);
void CloseMagics(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*mag = NULL, *tmp;

	snprintf(fin,  PATH_MAX, "%s/etc/magic.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/magic.temp", getenv("MBSE_ROOT"));

	if (MagicUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&magichdr, magichdr.hdrsize, 1, fi);
			fwrite(&magichdr, magichdr.hdrsize, 1, fo);

			while (fread(&magic, magichdr.recsize, 1, fi) == 1)
				if (!magic.Deleted)
					fill_stlist(&mag, magic.Mask, ftell(fi) - magichdr.recsize);
			sort_stlist(&mag);

			for (tmp = mag; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&magic, magichdr.recsize, 1, fi);
				fwrite(&magic, magichdr.recsize, 1, fo);
			}

			fclose(fi);
			fclose(fo);
			tidy_stlist(&mag);
			unlink(fout);
			chmod(fin, 0640);
			disk_reset();
			Syslog('+', "Updated \"magic.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendMagics(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	snprintf(ffile, PATH_MAX, "%s/etc/magic.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&magic, 0, sizeof(magic));
		fwrite(&magic, sizeof(magic), 1, fil);
		fclose(fil);
		MagicUpdated = 1;
		return 0;
	} else
		return -1;
}



void ScreenM(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 2, "10.4. EDIT MAGIC");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 2, "1.   Magic");
	mbse_mvprintw( 8, 2, "2.   Filemask");
	mbse_mvprintw( 9, 2, "3.   Active");
	mbse_mvprintw(10, 2, "4.   Deleted");
	mbse_mvprintw(11, 2, "5.   Area");

	switch(magic.Attrib) {
		case MG_ADOPT:
		case MG_MOVE:
				mbse_mvprintw(12, 2, "6.   To Area");
				break;

		case MG_EXEC:
				mbse_mvprintw(12, 2, "6.   Command");
				mbse_mvprintw(13, 2, "7.   Compile");
				break;

		case MG_COPY:
		case MG_UNPACK:
				mbse_mvprintw(12, 2, "6.   To path");
				mbse_mvprintw(13, 2, "7.   Compile");
				break;

		case MG_KEEPNUM:
				mbse_mvprintw(12, 2, "6.   Keep Num");
				break;
	}
}



void FieldsM(void)
{
	set_color(WHITE, BLACK);
	show_str( 7,16,20, getmagictype(magic.Attrib));
	show_str( 8,16,14, magic.Mask);
	show_str( 9,16, 3, getboolean(magic.Active));
	show_str(10,16, 3, getboolean(magic.Deleted));
	show_str(11,16,20, magic.From);

	switch(magic.Attrib) {
		case MG_ADOPT:
		case MG_MOVE:
				show_str(12,16,20, magic.ToArea);
				break;
		case MG_EXEC:
				show_str(12,16,64, magic.Cmd);
				show_bool(13,16, magic.Compile);
				break;
		case MG_UNPACK:
		case MG_COPY:
				show_bool(13,16, magic.Compile);
				show_str(12,16,64, magic.Path);
				break;
		case MG_KEEPNUM:
				show_int(12,16, magic.KeepNum);
				break;
	}
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditMagicRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX];
	int	offset;
	int	j, choices;
	unsigned int	crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Magics");

	snprintf(mfile, PATH_MAX, "%s/etc/magic.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(magichdr) + ((Area -1) * sizeof(magic));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&magic, sizeof(magic), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&magic, crc, sizeof(magic));

	for (;;) {
		ScreenM();
		FieldsM();

		switch(magic.Attrib) {
			case MG_UPDALIAS:
			case MG_DELETE:
					choices = 5;
					break;
			case MG_EXEC:
			case MG_COPY:
			case MG_UNPACK:
					choices = 7;
					break;
			default:
					choices = 6;
		}

		j = select_menu(choices);
		switch(j) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&magic, crc1, sizeof(magic));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&magic, sizeof(magic), 1, fil);
					fclose(fil);
					MagicUpdated = 1;
					working(6, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;

		case 1:	magic.Attrib = edit_magictype(7,16, magic.Attrib);
			break;

		case 2:	E_STR(  8,16,14, magic.Mask,   "File ^mask^ to test for this magic")
		case 3: E_BOOL( 9,16,    magic.Active, "Is this magic ^active^")
		case 4: E_BOOL(10,16,    magic.Deleted,"Is this record ^deleted^")
		case 5: strcpy(magic.From, PickTicarea((char *)"10.4.5"));
			break;
		case 6: switch(magic.Attrib) {
				case MG_ADOPT:
				case MG_MOVE:
						strcpy(magic.ToArea, PickTicarea((char *)"10.4.6"));
						break;

				case MG_COPY:
				case MG_UNPACK:
						E_PTH(12,16,64, magic.Path,   "The ^path^ to use", 0750)

				case MG_EXEC:
						E_STR(12,16,64, magic.Cmd,    "The ^command^ to execute")

				case MG_KEEPNUM:
						E_INT(12,16,    magic.KeepNum,"The number of files to ^keep^")
			}
			break;

		case 7:	E_BOOL(13,16, magic.Compile, "Trigger the ^compile nodelist^ flag")
		}
	}	
	return 0;
}



void EditMagics(void)
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

	records = CountMagics();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenMagics() == -1) {
		working(2, 0, 0);
		return;
	}
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 2, "10.4.  MAGICS EDITOR");
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/magic.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&magichdr, sizeof(magichdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(magichdr) + (((i + o) - 1) * magichdr.recsize);
						fseek(fil, offset, 0);
						fread(&magic, magichdr.recsize, 1, fil);
						if (magic.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						snprintf(temp, 81, "%3d.  %s %s", o + i, getmagictype(magic.Attrib), magic.Mask);
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
			CloseMagics(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendMagics() == 0) {
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
			EditMagicRec(atoi(pick));
			o = ((atoi(pick) - 1) / 20) * 20;
		}
	}
}



void InitMagics(void)
{
    CountMagics();
    OpenMagics();
    CloseMagics(TRUE);
}



int tic_magic_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *wp, *ip, *no;
    int	    nr = 0i, j;

    snprintf(temp, PATH_MAX, "%s/etc/magic.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 10, 4, page, (char *)"File Magic processing");
    j = 0;
    fprintf(fp, "\n\n");
    fread(&magichdr, sizeof(magichdr), 1, no);

    ip = open_webdoc((char *)"magic.html", (char *)"File Magic Processing", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<P>\n");
    fprintf(ip, "<TABLE border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Nr</TH><TH align='left'>Mask</TH><TH align='left'>Type</TH><TH align='left'>Active</TH></TR>\n");
	    
    while (fread(&magic, magichdr.recsize, 1, no) == 1) {
	if (j == 6) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    j = 0;
	}

	nr++;
	snprintf(temp, 81, "magic_%d.html", nr);
	fprintf(ip, " <TR><TD><A HREF=\"%s\">%d</A></TD><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>\n", 
		temp, nr, magic.Mask, getmagictype(magic.Attrib), getboolean(magic.Active));
	if ((wp = open_webdoc(temp, (char *)"File Magic", magic.Mask))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"magic.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Filemask", magic.Mask);
	    add_webtable(wp, (char *)"Magic type", getmagictype(magic.Attrib));
	    add_webtable(wp, (char *)"Active", getboolean(magic.Active));
	    fprintf(wp, "<TR><TH align='left'>TIC Area</TH><TD><A HREF=\"ticarea_%s.html\">%s</A></TD></TR>\n",
				                    magic.From, magic.From);
	    fprintf(fp, "   Filemask     %s\n", magic.Mask);
	    fprintf(fp, "   Type         %s\n", getmagictype(magic.Attrib));
	    fprintf(fp, "   Active       %s\n", getboolean(magic.Active));
	    fprintf(fp, "   Area         %s\n", magic.From);

	    switch (magic.Attrib) {
		case MG_ADOPT:
		case MG_MOVE:	    fprintf(fp, "   To area      %s\n", magic.ToArea);
				    fprintf(wp, "<TR><TH align='left'>TIC Area</TH><TD><A HREF=\"ticarea_%s.html\">%s</A></TD></TR>\n",
					    magic.ToArea, magic.ToArea);
				    break;
		case MG_EXEC:	    add_webtable(wp, (char *)"Command", magic.Cmd);
				    add_webtable(wp, (char *)"Compile nodelist", getboolean(magic.Compile));
				    fprintf(fp, "   Command      %s\n", magic.Cmd);
				    fprintf(fp, "   Compile NL   %s\n", getboolean(magic.Compile));
				    break;
		case MG_UNPACK:
		case MG_COPY:	    add_webtable(wp, (char *)"Compile nodelist", getboolean(magic.Compile));
				    add_webtable(wp, (char *)"Destination path", magic.Path);
				    fprintf(fp, "   Compile NL   %s\n", getboolean(magic.Compile));
				    fprintf(fp, "   Path         %s\n", magic.Path);
				    break;
		case MG_KEEPNUM:    add_webdigit(wp, (char *)"Keep # files", magic.KeepNum);
				    fprintf(fp, "   Keep # file  %d\n", magic.KeepNum);
				    break;
	    }
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    close_webdoc(wp);
	}
	
	fprintf(fp, "\n\n");
	j++;
    }

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
	
    fclose(no);
    return page;
}


