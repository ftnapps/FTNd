/*****************************************************************************
 *
 * $Id: m_service.c,v 1.18 2008/02/28 22:05:14 mbse Exp $
 * Purpose ...............: Service Setup
 *
 *****************************************************************************
 * Copyright (C) 1997-2008
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
#include "m_service.h"


int	ServiceUpdated;


/*
 * Count nr of servrec records in the database.
 * Creates the database if it doesn't exist.
 */
int CountService(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	snprintf(ffile, PATH_MAX, "%s/etc/service.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			servhdr.hdrsize = sizeof(servhdr);
			servhdr.recsize = sizeof(servrec);
			servhdr.lastupd = time(NULL);
			fwrite(&servhdr, sizeof(servhdr), 1, fil);
			memset(&servrec, 0, sizeof(servrec));

			servrec.Action = EMAIL;
			servrec.Active = TRUE;
			snprintf(servrec.Service, 16, "UUCP");
			fwrite(&servrec, sizeof(servrec), 1, fil);

			servrec.Action = AREAMGR;
			snprintf(servrec.Service, 16, "areamgr");
			fwrite(&servrec, sizeof(servrec), 1, fil);
			snprintf(servrec.Service, 16, "gecho");
			fwrite(&servrec, sizeof(servrec), 1, fil);
			snprintf(servrec.Service, 16, "fmail");
			fwrite(&servrec, sizeof(servrec), 1, fil);

			servrec.Action = FILEMGR;
			snprintf(servrec.Service, 16, "filemgr");
			fwrite(&servrec, sizeof(servrec), 1, fil);
			snprintf(servrec.Service, 16, "allfix");
			fwrite(&servrec, sizeof(servrec), 1, fil);
			snprintf(servrec.Service, 16, "mbtic");
			fwrite(&servrec, sizeof(servrec), 1, fil);
			snprintf(servrec.Service, 16, "raid");
			fwrite(&servrec, sizeof(servrec), 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
			return 6;
		} else
			return -1;
	}

	fread(&servhdr, sizeof(servhdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - servhdr.hdrsize) / servhdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenService(void);
int OpenService(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	int	oldsize;

	snprintf(fnin,  PATH_MAX, "%s/etc/service.data", getenv("MBSE_ROOT"));
	snprintf(fnout, PATH_MAX, "%s/etc/service.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&servhdr, sizeof(servhdr), 1, fin);
			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize    = servhdr.recsize;
			if (oldsize != sizeof(servrec)) {
				ServiceUpdated = 1;
				Syslog('+', "Upgraded %s, format changed", fnin);
			} else
				ServiceUpdated = 0;
			servhdr.hdrsize = sizeof(servhdr);
			servhdr.recsize = sizeof(servrec);
			fwrite(&servhdr, sizeof(servhdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&servrec, 0, sizeof(servrec));
			while (fread(&servrec, oldsize, 1, fin) == 1) {
				fwrite(&servrec, sizeof(servrec), 1, fout);
				memset(&servrec, 0, sizeof(servrec));
			}
			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseService(int);
void CloseService(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*hat = NULL, *tmp;

	snprintf(fin,  PATH_MAX, "%s/etc/service.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/service.temp", getenv("MBSE_ROOT"));

	if (ServiceUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&servhdr, servhdr.hdrsize, 1, fi);
			fwrite(&servhdr, servhdr.hdrsize, 1, fo);

			while (fread(&servrec, servhdr.recsize, 1, fi) == 1)
				if (!servrec.Deleted)
					fill_stlist(&hat, servrec.Service, ftell(fi) - servhdr.recsize);
			sort_stlist(&hat);

			for (tmp = hat; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&servrec, servhdr.recsize, 1, fi);
				fwrite(&servrec, servhdr.recsize, 1, fo);
			}

			tidy_stlist(&hat);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"servrec.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendService(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	snprintf(ffile, PATH_MAX, "%s/etc/service.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&servrec, 0, sizeof(servrec));
		/*
		 * Fill in default values
		 */
		fwrite(&servrec, sizeof(servrec), 1, fil);
		fclose(fil);
		ServiceUpdated = 1;
		return 0;
	} else
		return -1;
}



void ServiceScreen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 2, "16.  EDIT SERVICES");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 7, 2, "1.  Name");
	mbse_mvprintw( 8, 2, "2.  Type");
	mbse_mvprintw( 9, 2, "3.  Active");
	mbse_mvprintw(10, 2, "4.  Deleted");
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditServiceRec(int Area)
{
	FILE		*fil;
	char		mfile[PATH_MAX];
	int		offset;
	unsigned int	crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Service");

	snprintf(mfile, PATH_MAX, "%s/etc/service.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	fread(&servhdr, sizeof(servhdr), 1, fil);
	offset = servhdr.hdrsize + ((Area -1) * servhdr.recsize);
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&servrec, servhdr.recsize, 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&servrec, crc, servhdr.recsize);

	for (;;) {
		ServiceScreen();
		set_color(WHITE, BLACK);
		show_str(  7,18,15, servrec.Service);
		show_service(8, 18, servrec.Action);
		show_bool( 9,18,    servrec.Active);
		show_bool(10,18,    servrec.Deleted);

		switch(select_menu(4)) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&servrec, crc1, servhdr.recsize);
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&servrec, servhdr.recsize, 1, fil);
					fclose(fil);
					ServiceUpdated = 1;
					working(6, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;

		case 1:	E_STR(  7,18,15, servrec.Service,"Enter the ^name^ of this ^service^.")
		case 2: servrec.Action = edit_service(8,18,servrec.Action);
			break;
		case 3: E_BOOL( 9,18,    servrec.Active,    "If this service is ^active^")
		case 4: E_BOOL(10,18,    servrec.Deleted,   "If this record is ^Deleted^")
		}
	}
}



void EditService(void)
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

	records = CountService();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenService() == -1) {
		working(2, 0, 0);
		return;
	}
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mbse_mvprintw( 5, 4, "16.   SERVICE MANAGER");
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/service.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&servhdr, sizeof(servhdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(servhdr) + (((o + i) - 1) * servhdr.recsize);
						fseek(fil, offset, 0);
						fread(&servrec, servhdr.recsize, 1, fil);
						if (servrec.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						snprintf(temp, 81, "%3d.  %-15s %s", o+i, servrec.Service, getservice(servrec.Action));
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
			CloseService(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendService() == 0) {
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
			EditServiceRec(atoi(pick));
			o = ((atoi(pick) - 1) / 20) * 20;
		}
	}
}



void InitService(void)
{
    CountService();
    OpenService();
    CloseService(TRUE);
}



int service_doc(FILE *fp, FILE *toc, int page)
{
    char    temp[PATH_MAX];
    FILE    *wp, *no;
    int	    j;

    snprintf(temp, PATH_MAX, "%s/etc/service.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL)
	return page;

    page = newpage(fp, page);
    addtoc(fp, toc, 16, 0, page, (char *)"Service manager");
    j = 0;

    fprintf(fp, "\n");
    fprintf(fp, "     Service           Action     Active\n");
    fprintf(fp, "     ---------------   --------   ------\n");
    fread(&servhdr, sizeof(servhdr), 1, no);

    wp = open_webdoc((char *)"service.html", (char *)"Mail Service Manager", NULL);
    fprintf(wp, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(wp, "<UL>\n");
    fprintf(wp, "<TABLE width='400' border='0' cellspacing='0' cellpadding='2'>\n");
    fprintf(wp, "<TBODY>\n");
    fprintf(wp, "<TR><TH align='left'>Service</TH><TH align='left'>Action</TH><TH align='left'>Active</TH></TR>\n");

    while ((fread(&servrec, servhdr.recsize, 1, no)) == 1) {

	if (j == 50) {
	    page = newpage(fp, page);
	    fprintf(fp, "\n");
	    fprintf(fp, "     Service           Action     Active\n");
	    fprintf(fp, "     ---------------   --------   ------\n");
	    j = 0;
	}

	fprintf(wp, "<TR><TD>%s</TD><TD>%s</TD><TD>%s</TD></TR>\n",
		servrec.Service, getservice(servrec.Action), getboolean(servrec.Active));
	fprintf(fp, "     %-15s   %-8s   %s\n", servrec.Service, getservice(servrec.Action), getboolean(servrec.Active));
	j++;
    }

    fclose(no);
    return page;
}


