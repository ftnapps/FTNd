/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Setup FGroups.
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

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "m_global.h"
#include "m_node.h"
#include "m_ticarea.h"
#include "m_fgroup.h"



int	FGrpUpdated = 0;


/*
 * Count nr of fgroup records in the database.
 * Creates the database if it doesn't exist.
 */
int CountFGroup(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			fgrouphdr.hdrsize = sizeof(fgrouphdr);
			fgrouphdr.recsize = sizeof(fgroup);
			fwrite(&fgrouphdr, sizeof(fgrouphdr), 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
			return 0;
		} else
			return -1;
	}

	fread(&fgrouphdr, sizeof(fgrouphdr), 1, fil);
	fseek(fil, 0, SEEK_SET);
	fread(&fgrouphdr, fgrouphdr.hdrsize, 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - fgrouphdr.hdrsize) / fgrouphdr.recsize;
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenFGroup(void);
int OpenFGroup(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize;

	sprintf(fnin,  "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/fgroups.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			FGrpUpdated = 0;
			fread(&fgrouphdr, sizeof(fgrouphdr), 1, fin);
			fseek(fin, 0, SEEK_SET);
			fread(&fgrouphdr, fgrouphdr.hdrsize, 1, fin);
			if (fgrouphdr.hdrsize != sizeof(fgrouphdr)) {
				fgrouphdr.hdrsize = sizeof(fgrouphdr);
				fgrouphdr.lastupd = time(NULL);
				FGrpUpdated = 1;
			}

			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = fgrouphdr.recsize;
			if (oldsize != sizeof(fgroup)) 
				FGrpUpdated = 1;
			fgrouphdr.hdrsize = sizeof(fgrouphdr);
			fgrouphdr.recsize = sizeof(fgroup);
			fwrite(&fgrouphdr, sizeof(fgrouphdr), 1, fout);

			if (FGrpUpdated)
			    Syslog('+', "Updated %s, format changed", fnin);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&fgroup, 0, sizeof(fgroup));
			while (fread(&fgroup, oldsize, 1, fin) == 1) {
				/*
				 * Now set defaults
				 */
				if (FGrpUpdated) {
				    fgroup.DupCheck  = TRUE;
				    fgroup.Secure    = TRUE;
				    fgroup.VirScan   = TRUE;
				    fgroup.Announce  = TRUE;
				    fgroup.UpdMagic  = TRUE;
				    fgroup.FileId    = TRUE;
				    sprintf(fgroup.BasePath, "%s/ftp/pub/%s", getenv("MBSE_ROOT"), tl(fgroup.Name));
				}
				
				fwrite(&fgroup, sizeof(fgroup), 1, fout);
				memset(&fgroup, 0, sizeof(fgroup));
			}

			fclose(fin);
			fclose(fout);
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseFGroup(int);
void CloseFGroup(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*fgr = NULL, *tmp;

	sprintf(fin, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/fgroups.temp", getenv("MBSE_ROOT"));

	if (FGrpUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&fgrouphdr, fgrouphdr.hdrsize, 1, fi);
			fwrite(&fgrouphdr, fgrouphdr.hdrsize, 1, fo);

			while (fread(&fgroup, fgrouphdr.recsize, 1, fi) == 1)
				if (!fgroup.Deleted)
					fill_stlist(&fgr, fgroup.Name, ftell(fi) - fgrouphdr.recsize);
			sort_stlist(&fgr);

			for (tmp = fgr; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&fgroup, fgrouphdr.recsize, 1, fi);
				fwrite(&fgroup, fgrouphdr.recsize, 1, fo);
			}

			tidy_stlist(&fgr);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			chmod(fin, 0640);
			Syslog('+', "Updated \"fgroups.data\"");
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendFGroup(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	sprintf(ffile, "%s/etc/fgroups.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&fgroup, 0, sizeof(fgroup));
		fgroup.StartDate = time(NULL);
		fgroup.DivideCost = TRUE;
		fwrite(&fgroup, sizeof(fgroup), 1, fil);
		fclose(fil);
		FGrpUpdated = 1;
		return 0;
	} else
		return -1;
}



/*
 *  Check if field can be edited without screwing up the database
 */
int CheckFgroup(void);
int CheckFgroup(void)
{
	int	ncnt, tcnt;

	working(1, 0, 0);
	ncnt = GroupInNode(fgroup.Name, FALSE);
	tcnt = GroupInTic(fgroup.Name);
	working(0, 0, 0);
	if (ncnt || tcnt) {
		errmsg((char *)"Error, %d node(s) and/or %d tic area(s) connected", ncnt, tcnt);
		return TRUE;
	}
	return FALSE;
}



void FgScreen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 6, "10.1 EDIT FILE GROUP");
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "1.  Name");
	mvprintw( 8, 6, "2.  Comment");
	mvprintw( 9, 6, "3.  Active");
	mvprintw(10, 6, "4.  Use Aka");
	mvprintw(11, 6, "5.  Uplink");
	mvprintw(12, 6, "6.  Areas");
	mvprintw(13, 6, "7.  Unit Cost");
	mvprintw(14, 6, "8.  Unit Size");
	mvprintw(15, 6, "9.  Add Prom.");
	mvprintw(16, 6, "10. Divide");
	mvprintw(17, 6, "11. Deleted");
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditFGrpRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX];
	long	offset;
	int	j, tmp;
	unsigned long crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit FileGroup");

	sprintf(mfile, "%s/etc/fgroups.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	offset = sizeof(fgrouphdr) + ((Area -1) * sizeof(fgroup));
	if (fseek(fil, offset, 0) != 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&fgroup, sizeof(fgroup), 1, fil);
	fclose(fil);
	crc = 0xffffffff;
	crc = upd_crc32((char *)&fgroup, crc, sizeof(fgroup));
	working(0, 0, 0);
	FgScreen();

	for (;;) {
		set_color(WHITE, BLACK);
		show_str(  7,20,12, fgroup.Name);
		show_str(  8,20,55, fgroup.Comment);
		show_bool( 9,20, fgroup.Active);
		show_aka( 10,20, fgroup.UseAka);
		show_aka( 11,20, fgroup.UpLink);
		show_str( 12,20,12, fgroup.AreaFile);
		show_int( 13,20,    fgroup.UnitCost);
		show_int( 14,20,    fgroup.UnitSize);
		show_int( 15,20,    fgroup.AddProm);
		show_bool(16,20,    fgroup.DivideCost);
		show_bool(17,20,    fgroup.Deleted);

		j = select_menu(11);
		switch(j) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&fgroup, crc1, sizeof(fgroup));
			if (crc != crc1) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					if ((fil = fopen(mfile, "r+")) == NULL) {
						working(2, 0, 0);
						return -1;
					}
					fseek(fil, offset, 0);
					fwrite(&fgroup, sizeof(fgroup), 1, fil);
					fclose(fil);
					FGrpUpdated = 1;
					working(1, 0, 0);
					working(0, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		
		case 1: if (CheckFgroup())
				break;
			E_UPS(  7,20,12,fgroup.Name,       "The ^name^ of this file group")
		case 2: E_STR(  8,20,55,fgroup.Comment,    "The ^description^ of this file group")
		case 3: if (CheckFgroup())
				break;
			E_BOOL( 9,20,   fgroup.Active,     "Is this file group ^active^")
		case 4: tmp = PickAka((char *)"10.1.4", TRUE);
			if (tmp != -1)
				fgroup.UseAka = CFG.aka[tmp];
			FgScreen();
			break;
		case 5:	fgroup.UpLink = PullUplink((char *)"10.1.5"); 
			FgScreen();
			break;
		case 6: E_STR( 12,20,12,fgroup.AreaFile,   "The name of the ^Areas File^ from the uplink")
		case 7: E_INT( 13,20,   fgroup.UnitCost,   "The ^cost per size unit^ for this file")
		case 8: E_INT( 14,20,   fgroup.UnitSize,   "The ^unit size^ in KBytes, 0 means cost per file")
		case 9: E_INT( 15,20,   fgroup.AddProm,    "The ^Promillage^ to add or substract of the filecost")
		case 10:E_BOOL(16,20,   fgroup.DivideCost, "^Divide^ the cost over all downlinks or charge each link full cost")
		case 11:if (CheckFgroup())
				break;
			E_BOOL(17,20,   fgroup.Deleted,    "Is this file group ^Deleted^")
		}
	}

	return 0;
}



void EditFGroup(void)
{
	int	records, i, o, x, y;
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

	records = CountFGroup();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenFGroup() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 4, "10.1 FILE GROUPS SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/fgroups.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&fgrouphdr, sizeof(fgrouphdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11 ) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(fgrouphdr) + (((o + i) - 1) * fgrouphdr.recsize);
						fseek(fil, offset, 0);
						fread(&fgroup, fgrouphdr.recsize, 1, fil);
						if (fgroup.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-12s %-18s", o + i, fgroup.Name, fgroup.Comment);
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
			CloseFGroup(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendFGroup() == 0) {
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
			EditFGrpRec(atoi(pick));
			o = ((atoi(pick) - 1) / 20) * 20;
		}
	}
}



void InitFGroup(void)
{
    CountFGroup();
    OpenFGroup();
    CloseFGroup(TRUE);
}



char *PickFGroup(char *shdr)
{
	static	char FGrp[21] = "";
	int	records, i, o = 0, x, y;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
	long	offset;


	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return FGrp;
	}

	records = CountFGroup();
	if (records == -1) {
		working(2, 0, 0);
		return FGrp;
	}

	working(0, 0, 0);

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		sprintf(temp, "%s.  FILE GROUP SELECT", shdr);
		mvprintw( 5, 4, temp);
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&fgrouphdr, sizeof(fgrouphdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 20; i++) {
					if (i == 11) {
						x = 42;
						y = 7;
					}
					if ((o + i) <= records) {
						offset = sizeof(fgrouphdr) + (((o + i) - 1) * fgrouphdr.recsize);
						fseek(fil, offset, 0);
						fread(&fgroup, fgrouphdr.recsize, 1, fil);
						if (fgroup.Active)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-12s %-18s", o + i, fgroup.Name, fgroup.Comment);
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

		if (strncmp(pick, "-", 1) == 0) {
			return FGrp;
		}

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 20) < records)
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
			fil = fopen(temp, "r");
			offset = fgrouphdr.hdrsize + ((atoi(pick) - 1) * fgrouphdr.recsize);
			fseek(fil, offset, 0);
			fread(&fgroup, fgrouphdr.recsize, 1, fil);
			fclose(fil);
			strcpy(FGrp, fgroup.Name);
			return (FGrp);
		}
	}
}



int tic_group_doc(FILE *fp, FILE *toc, int page)
{
	char	temp[PATH_MAX];
	FILE	*no;
	int	j;

	sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	addtoc(fp, toc, 10, 1, page, (char *)"File processing groups");
	j = 0;
	fprintf(fp, "\n\n");

	fread(&fgrouphdr, sizeof(fgrouphdr), 1, no);
	fseek(no, 0, SEEK_SET);
	fread(&fgrouphdr, fgrouphdr.hdrsize, 1, no);

	while ((fread(&fgroup, fgrouphdr.recsize, 1, no)) == 1) {
		if (j == 3) {
			page = newpage(fp, page);
			fprintf(fp, "\n");
			j = 0;
		}

		fprintf(fp, "    Name       %s\n", fgroup.Name);
		fprintf(fp, "    Comment    %s\n", fgroup.Comment);
		fprintf(fp, "    Active     %s\n", getboolean(fgroup.Active));
		fprintf(fp, "    Use Aka    %s\n", aka2str(fgroup.UseAka));
		fprintf(fp, "    Uplink     %s\n", aka2str(fgroup.UpLink));
		fprintf(fp, "    Areas file %s\n", fgroup.AreaFile);
		fprintf(fp, "    Start date %s", ctime(&fgroup.StartDate));
		fprintf(fp, "    Last date  %s\n", ctime(&fgroup.LastDate));

//		fprintf(fp, "               Total      This month Last month\n");
//		fprintf(fp, "               ---------- ---------- ----------\n");
//		fprintf(fp, "    Files      %-10ld %-10ld %-10ld\n", fgroup.Total.files, fgroup.ThisMonth.files, fgroup.LastMonth.files);
//		fprintf(fp, "    KBytes     %-10ld %-10ld %-10ld\n", fgroup.Total.kbytes, fgroup.ThisMonth.kbytes, fgroup.LastMonth.kbytes);
		fprintf(fp, "\n\n\n");
		j++;
	}

	fclose(no);
	return page;
}



