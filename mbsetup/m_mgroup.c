/*****************************************************************************
 *
 * File ..................: setup/m_mgroups.c
 * Purpose ...............: Setup MGroups.
 * Last modification date : 12-Jun-2001
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
#include "stlist.h"
#include "m_global.h"
#include "m_node.h"
#include "m_marea.h"
#include "m_mgroup.h"



int	MGrpUpdated = 0;


/*
 * Count nr of mgroup records in the database.
 * Creates the database if it doesn't exist.
 */
int CountMGroup(void)
{
	FILE	*fil;
	char	ffile[81];
	int	count;

	sprintf(ffile, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			mgrouphdr.hdrsize = sizeof(mgrouphdr);
			mgrouphdr.recsize = sizeof(mgroup);
			fwrite(&mgrouphdr, sizeof(mgrouphdr), 1, fil);
			fclose(fil);
			return 0;
		} else
			return -1;
	}

	fread(&mgrouphdr, sizeof(mgrouphdr), 1, fil);
	fseek(fil, 0, SEEK_SET);
	fread(&mgrouphdr, mgrouphdr.hdrsize, 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - mgrouphdr.hdrsize) / mgrouphdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenMGroup(void)
{
	FILE	*fin, *fout;
	char	fnin[81], fnout[81];
	long	oldsize;

	sprintf(fnin,  "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/mgroups.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			MGrpUpdated = 0;
			fread(&mgrouphdr, sizeof(mgrouphdr), 1, fin);
			fseek(fin, 0, SEEK_SET);
			fread(&mgrouphdr, mgrouphdr.hdrsize, 1, fin);
			if (mgrouphdr.hdrsize != sizeof(mgrouphdr)) {
				mgrouphdr.hdrsize = sizeof(mgrouphdr);
				mgrouphdr.lastupd = time(NULL);
				MGrpUpdated = 1;
			}

			/*
			 * In case we are automaitc upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = mgrouphdr.recsize;
			if (oldsize != sizeof(mgroup))
				MGrpUpdated = 1;
			mgrouphdr.hdrsize = sizeof(mgrouphdr);
			mgrouphdr.recsize = sizeof(mgroup);
			fwrite(&mgrouphdr, sizeof(mgrouphdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&mgroup, 0, sizeof(mgroup));
			while (fread(&mgroup, oldsize, 1, fin) == 1) {
				fwrite(&mgroup, sizeof(mgroup), 1, fout);
				memset(&mgroup, 0, sizeof(mgroup));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseMGroup(void)
{
	char	fin[81], fout[81];
	FILE	*fi, *fo;
	st_list	*mgr = NULL, *tmp;

	sprintf(fin, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/mgroups.temp", getenv("MBSE_ROOT"));

	if (MGrpUpdated == 1) {
		if (yes_no((char *)"Database is changed, save changes") == 1) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&mgrouphdr, mgrouphdr.hdrsize, 1, fi);
			fwrite(&mgrouphdr, mgrouphdr.hdrsize, 1, fo);

			while (fread(&mgroup, mgrouphdr.recsize, 1, fi) == 1)
				if (!mgroup.Deleted)
					fill_stlist(&mgr, mgroup.Name, ftell(fi) - mgrouphdr.recsize);
			sort_stlist(&mgr);

			for (tmp = mgr; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&mgroup, mgrouphdr.recsize, 1, fi);
				fwrite(&mgroup, mgrouphdr.recsize, 1, fo);
			}

			tidy_stlist(&mgr);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			Syslog('+', "Updated \"mgroups.data\"");
			return;
		}
	}
	working(1, 0, 0);
	unlink(fout); 
}



int AppendMGroup(void)
{
	FILE	*fil;
	char	ffile[81];

	sprintf(ffile, "%s/etc/mgroups.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&mgroup, 0, sizeof(mgroup));
		time(&mgroup.StartDate);
		fwrite(&mgroup, sizeof(mgroup), 1, fil);
		fclose(fil);
		MGrpUpdated = 1;
		return 0;
	} else
		return -1;
}



void MgScreen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "9.1 EDIT MESSAGE GROUP");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.  Name");
	mvprintw( 8, 6, "2.  Comment");
	mvprintw( 9, 6, "3.  Active");
	mvprintw(10, 6, "4.  Use Aka");
	mvprintw(11, 6, "5.  Uplink");
	mvprintw(12, 6, "6.  Areas");
	mvprintw(13, 6, "7.  Deleted");
}



/*
 *  Check if field can be edited without screwing up the database.
 */
int CheckMgroup(void);
int CheckMgroup(void)
{
	int	ncnt, mcnt;

	working(1, 0, 0);
	ncnt = GroupInNode(mgroup.Name, TRUE);
	mcnt = GroupInMarea(mgroup.Name);
	working(0, 0, 0);
	if (ncnt || mcnt) {
		errmsg((char *)"Error, %d node(s) and/or %d message area(s) connected", ncnt, mcnt);
		return TRUE;
	}
	return FALSE;
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditMGrpRec(int Area)
{
	FILE		*fil;
	static char	mfile[PATH_MAX];
	static long	offset;
	static int	j, tmp;
	unsigned long	crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit MessageGroup");

	sprintf(mfile, "%s/etc/mgroups.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(mgrouphdr) + ((Area -1) * sizeof(mgroup));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&mgroup, sizeof(mgroup), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&mgroup, crc, sizeof(mgroup));
	working(0, 0, 0);
	MgScreen();
	
	for (;;) {
		set_color(WHITE, BLACK);
		show_str(  7,18,12, mgroup.Name);
		show_str(  8,18,55, mgroup.Comment);
		show_bool( 9,18, mgroup.Active);
		show_aka( 10,18, mgroup.UseAka);
		show_aka( 11,18, mgroup.UpLink);
		show_str( 12,18,12, mgroup.AreaFile);
		show_bool(13,18, mgroup.Deleted);

		j = select_menu(7);
		switch(j) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&mgroup, crc1, sizeof(mgroup));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						WriteError("$Can't reopen %s", mfile);
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&mgroup, sizeof(mgroup), 1, fil);
					fclose(fil);
					MGrpUpdated = 1;
					working(1, 0, 0);
					working(0, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1: if (CheckMgroup())
				break;
			E_UPS( 7,18,12,mgroup.Name,"The ^name^ for this message group")
		case 2: E_STR( 8,18,55,mgroup.Comment,"The ^desription^ for this message group")
		case 3: if (CheckMgroup())
				break;
			E_BOOL(9,18,   mgroup.Active, "Is this message group ^active^")
		case 4: tmp = PickAka((char *)"9.1.4", TRUE);
			if (tmp != -1)
				memcpy(&mgroup.UseAka, &CFG.aka[tmp], sizeof(fidoaddr));
			MgScreen();
			break;
		case 5: mgroup.UpLink = PullUplink((char *)"9.1.5");
			MgScreen();
			break;
		case 6: E_STR(12,18,12,mgroup.AreaFile,"The name of the ^Areas File^ from the uplink")
		case 7: if (CheckMgroup())
				break;
			E_BOOL(13,18,  mgroup.Deleted, "Is this group ^Deleted^")
		}
	}

	return 0;
}



void EditMGroup(void)
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

	records = CountMGroup();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenMGroup() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 4, "9.1 MESSAGE GROUPS SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/mgroups.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&mgrouphdr, sizeof(mgrouphdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11 ) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(mgrouphdr) + (((o + i) - 1) * mgrouphdr.recsize);
						fseek(fil, offset, 0);
						fread(&mgroup, mgrouphdr.recsize, 1, fil);
						if (mgroup.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-12s %-18s", o + i, mgroup.Name, mgroup.Comment);
						temp[38] = '\0';
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
			CloseMGroup();
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendMGroup() == 0) {
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
			EditMGrpRec(atoi(pick));
	}
}



char *PickMGroup(char *shdr)
{
	static	char MGrp[21] = "";
	int	records, i, o = 0, y, x;
	char	pick[12];
	FILE	*fil;
	char	temp[81];
	long	offset;


	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return MGrp;
	}

	records = CountMGroup();
	if (records == -1) {
		working(2, 0, 0);
		return MGrp;
	}

	working(0, 0, 0);

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		sprintf(temp, "%s.  MESSAGE GROUP SELECT", shdr);
		mvprintw( 5, 4, temp);
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&mgrouphdr, sizeof(mgrouphdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(mgrouphdr) + (((o + i) - 1) * mgrouphdr.recsize);
						fseek(fil, offset, 0);
						fread(&mgroup, mgrouphdr.recsize, 1, fil);
						if (mgroup.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-12s %-18s", o + i, mgroup.Name, mgroup.Comment);
						temp[38] = '\0';
						mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
		strcpy(pick, select_pick(records, 20));

		if (strncmp(pick, "-", 1) == 0) 
			return MGrp;

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 20) < records)
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
			fil = fopen(temp, "r");
			offset = sizeof(mgrouphdr) + ((atoi(pick) - 1) * mgrouphdr.recsize);
			fseek(fil, offset, 0);
			fread(&mgroup, mgrouphdr.recsize, 1, fil);
			fclose(fil);
			strcpy(MGrp, mgroup.Name);
			return MGrp;
		}
	}
}



int mail_group_doc(FILE *fp, FILE *toc, int page)
{
	char	temp[81];
	FILE	*no;
	int	j;

	sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	addtoc(fp, toc, 9, 1, page, (char *)"Mail processing groups");
	j = 0;
	fprintf(fp, "\n\n");

	fread(&mgrouphdr, sizeof(mgrouphdr), 1, no);
	fseek(no, 0, SEEK_SET);
	fread(&mgrouphdr, mgrouphdr.hdrsize, 1, no);

	while ((fread(&mgroup, mgrouphdr.recsize, 1, no)) == 1) {
		if (j == 3) {
			page = newpage(fp, page);
			fprintf(fp, "\n");
			j = 0;
		}

		fprintf(fp, "    Name       %s\n", mgroup.Name);
		fprintf(fp, "    Comment    %s\n", mgroup.Comment);
		fprintf(fp, "    Active     %s\n", getboolean(mgroup.Active));
		fprintf(fp, "    Use Aka    %s\n", aka2str(mgroup.UseAka));
		fprintf(fp, "    Uplink     %s\n", aka2str(mgroup.UpLink));
		fprintf(fp, "    Areas file %s\n", mgroup.AreaFile);
		fprintf(fp, "    Start date %s",   ctime(&mgroup.StartDate));
		fprintf(fp, "    Last date  %s\n", ctime(&mgroup.LastDate));

//		fprintf(fp, "               Total      This month Last month\n");
//		fprintf(fp, "               ---------- ---------- ----------\n");
//		fprintf(fp, "    Messages   %-10ld %-10ld %-10ld\n", mgroup.Total.files, mgroup.ThisMonth.files, mgroup.LastMonth.files);
//		fprintf(fp, "    Kilobytes  %-10ld %-10ld %-10ld\n", mgroup.Total.kbytes, mgroup.ThisMonth.kbytes, mgroup.LastMonth.kbytes);
		fprintf(fp, "\n\n\n");
		j++;
	}

	fclose(no);
	return page;
}


