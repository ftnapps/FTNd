/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Domain Setup
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
#include "m_menu.h"
#include "m_domain.h"


int	DomainUpdated;


/*
 * Count nr of domtrans records in the database.
 * Creates the database if it doesn't exist.
 */
int CountDomain(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/domain.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			domainhdr.hdrsize = sizeof(domainhdr);
			domainhdr.recsize = sizeof(domtrans);
			domainhdr.lastupd = time(NULL);
			fwrite(&domainhdr, sizeof(domainhdr), 1, fil);
			memset(&domtrans, 0, sizeof(domtrans));
			domtrans.Active = TRUE;
			sprintf(domtrans.ftndom, ".z1.fidonet");
			sprintf(domtrans.intdom, ".z1.fidonet.org");
			fwrite(&domtrans, sizeof(domtrans), 1, fil);
			sprintf(domtrans.ftndom, ".z2.fidonet");
			sprintf(domtrans.intdom, ".z2.fidonet.org");
			fwrite(&domtrans, sizeof(domtrans), 1, fil);
			sprintf(domtrans.ftndom, ".z3.fidonet");
			sprintf(domtrans.intdom, ".z3.fidonet.org");
			fwrite(&domtrans, sizeof(domtrans), 1, fil);
			sprintf(domtrans.ftndom, ".z4.fidonet");
			sprintf(domtrans.intdom, ".z4.fidonet.org");
			fwrite(&domtrans, sizeof(domtrans), 1, fil);
			sprintf(domtrans.ftndom, ".z5.fidonet");
			sprintf(domtrans.intdom, ".z5.fidonet.org");
			fwrite(&domtrans, sizeof(domtrans), 1, fil);
			sprintf(domtrans.ftndom, ".z6.fidonet");
			sprintf(domtrans.intdom, ".z6.fidonet.org");
			fwrite(&domtrans, sizeof(domtrans), 1, fil);
			sprintf(domtrans.ftndom, ".fidonet");
			sprintf(domtrans.intdom, ".ftn");
			fwrite(&domtrans, sizeof(domtrans), 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
			return 7;
		} else
			return -1;
	}

	fread(&domainhdr, sizeof(domainhdr), 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - domainhdr.hdrsize) / domainhdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenDomain(void);
int OpenDomain(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize;

	sprintf(fnin,  "%s/etc/domain.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/domain.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			fread(&domainhdr, sizeof(domainhdr), 1, fin);
			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize    = domainhdr.recsize;
			if (oldsize != sizeof(domtrans)) {
				DomainUpdated = 1;
				Syslog('+', "Updated %s to new format", fnin);
			} else
				DomainUpdated = 0;
			domainhdr.hdrsize = sizeof(domainhdr);
			domainhdr.recsize = sizeof(domtrans);
			fwrite(&domainhdr, sizeof(domainhdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&domtrans, 0, sizeof(domtrans));
			while (fread(&domtrans, oldsize, 1, fin) == 1) {
				fwrite(&domtrans, sizeof(domtrans), 1, fout);
				memset(&domtrans, 0, sizeof(domtrans));
			}
			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseDomain(int);
void CloseDomain(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;

	sprintf(fin, "%s/etc/domain.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/domain.temp", getenv("MBSE_ROOT"));

	if (DomainUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&domainhdr, domainhdr.hdrsize, 1, fi);
			fwrite(&domainhdr, domainhdr.hdrsize, 1, fo);

			while (fread(&domtrans, domainhdr.recsize, 1, fi) == 1) {
				if (!domtrans.Deleted)
					fwrite(&domtrans, domainhdr.recsize, 1, fo);
			}

			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"domtrans.data\"");
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendDomain(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	sprintf(ffile, "%s/etc/domain.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&domtrans, 0, sizeof(domtrans));
		/*
		 * Fill in default values
		 */
		fwrite(&domtrans, sizeof(domtrans), 1, fil);
		fclose(fil);
		DomainUpdated = 1;
		return 0;
	} else
		return -1;
}



void DomainScreen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "17.  EDIT DOMAINS");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.  Fidonet");
	mvprintw( 8, 2, "2.  Internet");
	mvprintw( 9, 2, "3.  Active");
	mvprintw(10, 2, "4.  Deleted");
}



/*
 * Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditDomainRec(int Area)
{
	FILE		*fil;
	char		mfile[PATH_MAX];
	long		offset;
	unsigned long	crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Domain");

	sprintf(mfile, "%s/etc/domain.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	fread(&domainhdr, sizeof(domainhdr), 1, fil);
	offset = domainhdr.hdrsize + ((Area -1) * domainhdr.recsize);
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&domtrans, domainhdr.recsize, 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&domtrans, crc, domainhdr.recsize);
	working(0, 0, 0);

	for (;;) {
		DomainScreen();
		set_color(WHITE, BLACK);
		show_str(  7,18,60, domtrans.ftndom);
		show_str(  8,18,60, domtrans.intdom);
		show_bool( 9,18,    domtrans.Active);
		show_bool(10,18,    domtrans.Deleted);

		switch(select_menu(4)) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&domtrans, crc1, domainhdr.recsize);
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&domtrans, domainhdr.recsize, 1, fil);
					fclose(fil);
					DomainUpdated = 1;
					working(0, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;

		case 1:	E_STR(  7,18,60, domtrans.ftndom,    "Enter the ^fidonet^ side of this ^domain^.")
		case 2: E_STR(  8,18,60, domtrans.intdom,    "Enter the ^internet^ side of this ^domain^.")
		case 3: E_BOOL( 9,18,    domtrans.Active,    "If this domain is ^active^")
		case 4: E_BOOL(10,18,    domtrans.Deleted,   "If this record is ^Deleted^")
		}
	}
}



void EditDomain(void)
{
	int		records, i, o, y, from, too;
	char		pick[12];
	FILE		*fil;
	char		temp[PATH_MAX];
	long		offset;
	struct domrec	tdomtrans;

	if (! check_free())
	    return;
		
	clr_index();
	working(1, 0, 0);
	IsDoing("Browsing Menu");
	if (config_read() == -1) {
		working(2, 0, 0);
		return;
	}

	records = CountDomain();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenDomain() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 4, "17.   DOMAIN MANAGER");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/domain.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&domainhdr, sizeof(domainhdr), 1, fil);
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 10; i++) {
					if ((o + i) <= records) {
						offset = sizeof(domainhdr) + (((o + i) - 1) * domainhdr.recsize);
						fseek(fil, offset, 0);
						fread(&domtrans, domainhdr.recsize, 1, fil);
						if (domtrans.Deleted)
							set_color(LIGHTRED, BLACK);
						else if (domtrans.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-31s  %-31s", o+i, domtrans.ftndom, domtrans.intdom);
						temp[75] = 0;
						mvprintw(y, 3, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
		strcpy(pick, select_menurec(records));
		
		if (strncmp(pick, "-", 1) == 0) {
			open_bbs();
			CloseDomain(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendDomain() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
			working(0, 0, 0);
		}

		if (strncmp(pick, "D", 1) == 0) {
			mvprintw(LINES -3, 6, "Enter domain number (1..%d) to delete >", records);
			y = 0;
			y = edit_int(LINES -3, 44, y, (char *)"Enter record number");
			if ((y > 0) && (y <= records) && yes_no((char *)"Remove record")) {
				sprintf(temp, "%s/etc/domain.temp", getenv("MBSE_ROOT"));
				if ((fil = fopen(temp, "r+")) != NULL) {
					offset = ((y - 1) * domainhdr.recsize) + domainhdr.hdrsize;
					fseek(fil, offset, SEEK_SET);
					fread(&domtrans, domainhdr.recsize, 1, fil);
					domtrans.Deleted = TRUE;
					fseek(fil, offset, SEEK_SET);
					fwrite(&domtrans, domainhdr.recsize, 1, fil);
					DomainUpdated = TRUE;
					fclose(fil);
				}
			}
		}

		if (strncmp(pick, "M", 1) == 0) {
			from = too = 0;
			mvprintw(LINES -3, 6, "Enter domain number (1..%d) to move >", records);
			from = edit_int(LINES -3, 42, from, (char *)"Enter record number");
			locate(LINES -3, 6);
			clrtoeol();
			mvprintw(LINES -3, 6, "Enter new position (1..%d) >", records);
			too = edit_int(LINES -3, 36, too, (char *)"Enter destination record number, other will move away");
			if ((from == too) || (from == 0) || (too == 0) || (from > records) || (too > records)) {
				errmsg("That makes no sense");
			} else if (yes_no((char *)"Proceed move")) {
				sprintf(temp, "%s/etc/domain.temp", getenv("MBSE_ROOT"));
				if ((fil = fopen(temp, "r+")) != NULL) {
					fseek(fil, ((from -1) * domainhdr.recsize) + domainhdr.hdrsize, SEEK_SET);
					fread(&tdomtrans, domainhdr.recsize, 1, fil);
					if (from > too) {
						for (i = from; i > too; i--) {
							fseek(fil, ((i -2) * domainhdr.recsize) + domainhdr.hdrsize, SEEK_SET);
							fread(&domtrans, domainhdr.recsize, 1, fil);
							fseek(fil, ((i -1) * domainhdr.recsize) + domainhdr.hdrsize, SEEK_SET);
							fwrite(&domtrans, domainhdr.recsize, 1, fil);
						}
					} else {
						for (i = from; i < too; i++) {
							fseek(fil, (i * domainhdr.recsize) + domainhdr.hdrsize, SEEK_SET);
							fread(&domtrans, domainhdr.recsize, 1, fil);
							fseek(fil, ((i -1) * domainhdr.recsize) + domainhdr.hdrsize, SEEK_SET);
							fwrite(&domtrans, domainhdr.recsize, 1, fil);
						}
					}
					fseek(fil, ((too -1) * domainhdr.recsize) + domainhdr.hdrsize, SEEK_SET);
					fwrite(&tdomtrans, domainhdr.recsize, 1, fil);
					fclose(fil);
					DomainUpdated = TRUE;
				}
			}
		}

		if (strncmp(pick, "N", 1) == 0) 
			if ((o + 10) < records) 
				o = o + 10;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 10) >= 0)
				o = o - 10;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			EditDomainRec(atoi(pick));
			o = ((atoi(pick) - 1) / 10) * 10;
		}
	}
}



void InitDomain(void)
{
    CountDomain();
    OpenDomain();
    CloseDomain(TRUE);
}



int domain_doc(FILE *fp, FILE *toc, int page)
{
	char		temp[PATH_MAX];
	FILE		*no;
	int		j;

	sprintf(temp, "%s/etc/domain.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	page = newpage(fp, page);
	addtoc(fp, toc, 15, 0, page, (char *)"Domain manager");
	j = 0;

	fprintf(fp, "\n");
	fprintf(fp, "     Fidonet                         Internet                        Active\n");
	fprintf(fp, "     ------------------------------  ------------------------------  ------\n");
	fread(&domainhdr, sizeof(domainhdr), 1, no);

	while ((fread(&domtrans, domainhdr.recsize, 1, no)) == 1) {

		if (j == 50) {
			page = newpage(fp, page);
			fprintf(fp, "\n");
			fprintf(fp, "     Fidonet                         Internet                        Active\n");
			fprintf(fp, "     ------------------------------  ------------------------------  ------\n");
			j = 0;
		}

		fprintf(fp, "     %-30s  %-30s  %s\n", domtrans.ftndom, domtrans.intdom, getboolean(domtrans.Active));
		j++;
	}

	fclose(no);
	return page;
}


