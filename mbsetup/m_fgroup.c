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
#include "m_archive.h"
#include "m_ngroup.h"
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
	char	fnin[PATH_MAX], fnout[PATH_MAX], temp[13];
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
				    memset(&temp, 0, sizeof(temp));
				    strcpy(temp, fgroup.Name);
				    sprintf(fgroup.BasePath, "%s/ftp/pub/%s", getenv("MBSE_ROOT"), tl(temp));
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
		fgroup.FileGate = TRUE;
		fgroup.Secure = TRUE;
		fgroup.VirScan = TRUE;
		fgroup.Announce = TRUE;
		fgroup.FileId = TRUE;
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
	mvprintw( 4, 2, "10.1 EDIT FILE GROUP");
	set_color(CYAN, BLACK);
	mvprintw( 6, 2, "1.  Name");
	mvprintw( 7, 2, "2.  Comment");
	mvprintw( 8, 2, "3.  Base path");
	mvprintw( 9, 2, "4.  Use Aka");
	mvprintw(10, 2, "5.  Uplink");
	mvprintw(11, 2, "6.  Areas");
	mvprintw(12, 2, "7.  Filegate");
	mvprintw(13, 2, "8.  Banner");
	mvprintw(14, 2, "9.  Convert");
	mvprintw(15, 2, "10. BBS group");
	mvprintw(16, 2, "11. New group");
	mvprintw(17, 2, "12. Active");
	mvprintw(18, 2, "13. Deleted");
	mvprintw(19, 2, "14. Start at");

	mvprintw(11,32, "15. Unit Cost");
	mvprintw(12,32, "16. Unit Size");
	mvprintw(13,32, "17. Add Prom.");
	mvprintw(14,32, "18. Divide");
	mvprintw(15,32, "19. Auto chng");
	mvprintw(16,32, "20. User chng");
	mvprintw(17,32, "21. Replace");
	mvprintw(18,32, "22. Dupecheck");
	mvprintw(19,32, "23. Secure");

	mvprintw( 9,56, "24. No touch");
	mvprintw(10,56, "25. Virscan");
	mvprintw(11,56, "26. Announce");
	mvprintw(12,56, "27. Upd magic");
	mvprintw(13,56, "28. File ID");
	mvprintw(14,56, "29. Conv. all");
	mvprintw(15,56, "30. Send orig");
	mvprintw(16,56, "31. DL sec");
	mvprintw(17,56, "32. UP sec");
	mvprintw(18,56, "33. LT sec");
	mvprintw(19,56, "34. Upl. area");
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
		show_str(  6,16,12, fgroup.Name);
		show_str(  7,16,55, fgroup.Comment);
		show_str(  8,16,64, fgroup.BasePath);
		show_aka(  9,16,    fgroup.UseAka);
		show_aka( 10,16,    fgroup.UpLink);
		show_str( 11,16,12, fgroup.AreaFile);
		show_bool(12,16,    fgroup.FileGate);
		show_str( 13,16,14, fgroup.Banner);
		show_str( 14,16,5,  fgroup.Convert);
		show_str( 15,16,12, fgroup.BbsGroup);
		show_str( 16,16,12, fgroup.AnnGroup);
		show_bool(17,16,    fgroup.Active);
		show_bool(18,16,    fgroup.Deleted);
		show_int( 19,16,    fgroup.StartArea);

		show_int( 11,46,    fgroup.UnitCost);
		show_int( 12,46,    fgroup.UnitSize);
		show_int( 13,46,    fgroup.AddProm);
		show_bool(14,46,    fgroup.DivideCost);
		show_bool(15,46,    fgroup.AutoChange);
		show_bool(16,46,    fgroup.UserChange);
		show_bool(17,46,    fgroup.Replace);
		show_bool(18,46,    fgroup.DupCheck);
		show_bool(19,46,    fgroup.Secure);

		show_bool( 9,70,    fgroup.NoTouch);
		show_bool(10,70,    fgroup.VirScan);
		show_bool(11,70,    fgroup.Announce);
		show_bool(12,70,    fgroup.UpdMagic);
		show_bool(13,70,    fgroup.FileId);
		show_bool(14,70,    fgroup.ConvertAll);
		show_bool(15,70,    fgroup.SendOrg);
		show_int( 16,70,    fgroup.DLSec.level);
		show_int( 17,70,    fgroup.UPSec.level);
		show_int( 18,70,    fgroup.LTSec.level);
		show_int( 19,70,    fgroup.Upload);

		j = select_menu(34);
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
			E_UPS(  6,16,12,fgroup.Name,       "The ^name^ of this file group")
		case 2: E_STR(  7,16,55,fgroup.Comment,    "The ^description^ of this file group")
		case 3: E_PTH(  8,16,64,fgroup.BasePath,   "The ^base path^ for new created file areas")
		case 4: tmp = PickAka((char *)"10.1.4", TRUE);
			if (tmp != -1)
				fgroup.UseAka = CFG.aka[tmp];
			FgScreen();
			break;
		case 5:	fgroup.UpLink = PullUplink((char *)"10.1.5"); 
			FgScreen();
			break;
		case 6: E_STR( 11,16,12,fgroup.AreaFile,   "The name of the ^Areas File^ from the uplink")
		case 7: E_BOOL(12,16,   fgroup.FileGate,   "Is the areas file in ^filegate.zxx^ format")
		case 8: E_STR( 13,16,14,fgroup.Banner,     "The ^banner^ to add to the archives")
		case 9: strcpy(fgroup.Convert, PickArchive((char *)"10.1.9"));
			FgScreen();
			break;
		case 10:strcpy(fgroup.BbsGroup, PickFGroup((char *)"8.4.17"));
			FgScreen();
			break;
		case 11:strcpy(fgroup.AnnGroup, PickNGroup((char *)"8.4.18"));
			FgScreen();
			break;
		case 12:if (CheckFgroup())
			    break;
			E_BOOL(17,16,   fgroup.Active,     "Is this file group ^active^")
		case 13:if (CheckFgroup())
				break;
			E_BOOL(18,16,   fgroup.Deleted,    "Is this file group ^Deleted^")
		case 14:E_INT( 19,16,   fgroup.StartArea,  "The ^start area^ to create new BBS areas")

		case 15:E_INT( 11,46,   fgroup.UnitCost,   "The ^cost per size unit^ files received in this tic group")
		case 16:E_INT( 12,46,   fgroup.UnitSize,   "The ^unit size^ in KBytes, 0 means cost per file in this tic group")
		case 17:E_INT( 13,46,   fgroup.AddProm,    "The ^Promillage^ to add or substract of the filecost")
		case 18:E_BOOL(14,46,   fgroup.DivideCost, "^Divide^ the cost over all downlinks or charge each link full cost")
		case 19:E_BOOL(15,46,   fgroup.AutoChange, "^Automatic change areas^ when a new arealist is received")
		case 20:E_BOOL(16,46,   fgroup.UserChange, "Create new areas when ^users^ request new tic areas")
		case 21:E_BOOL(17,46,   fgroup.Replace,    "Set ^Replace^ in new created tic areas")
		case 22:E_BOOL(18,46,   fgroup.DupCheck,   "Set ^Dupe check^ in new created tic areas")
		case 23:E_BOOL(19,46,   fgroup.Secure,     "Set ^Secure^ tic processing in new created tic areas")

		case 24:E_BOOL( 9,70,   fgroup.NoTouch,    "Set ^Don't touch filedate^ in new created tic areas")
		case 25:E_BOOL(10,70,   fgroup.VirScan,    "Set ^Virus scanner^ in new created tic areas")
		case 26:E_BOOL(11,70,   fgroup.Announce,   "Set ^Announce files^ in new created tic areas")
		case 27:E_BOOL(12,70,   fgroup.UpdMagic,   "Set ^Update magic^ in new created tic areas")
		case 28:E_BOOL(13,70,   fgroup.FileId,     "Set ^FILE_ID.DIZ extract^ in new created tic areas")
		case 29:E_BOOL(14,70,   fgroup.ConvertAll, "Set ^Convert All^ setting in new created tic areas")
		case 30:E_BOOL(15,70,   fgroup.SendOrg,    "Set ^Send original^ setting in new created tic areas")
		case 31:E_SEC( 16,70,   fgroup.DLSec,      "10.1.31 FILE GROUP DOWNLOAD SECURITY", FgScreen)
		case 32:E_SEC( 17,70,   fgroup.UPSec,      "10.1.32 FILE GROUP UPLOAD SECURITY", FgScreen)
		case 33:E_SEC( 18,70,   fgroup.LTSec,      "10.1.33 FILE GROUP LIST SECURITY", FgScreen)
		case 34:E_INT( 19,70,   fgroup.Upload,     "Set the default ^Upload area^ in new created file areas")
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
	char	*temp;
	FILE	*no;
	int	First = TRUE;;

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL) {
	    free(temp);
	    return page;
	}
	free(temp);

	fread(&fgrouphdr, sizeof(fgrouphdr), 1, no);
	fseek(no, 0, SEEK_SET);
	fread(&fgrouphdr, fgrouphdr.hdrsize, 1, no);

	while ((fread(&fgroup, fgrouphdr.recsize, 1, no)) == 1) {
		if (First) {
		    addtoc(fp, toc, 10, 1, page, (char *)"File processing groups");
		    First = FALSE;
		    fprintf(fp, "\n");
		} else {
		    page = newpage(fp, page);
		    fprintf(fp, "\n\n");
		}

		fprintf(fp, "    Group name     %s\n", fgroup.Name);
		fprintf(fp, "    Comment        %s\n", fgroup.Comment);
		fprintf(fp, "    Active         %s\n", getboolean(fgroup.Active));
		fprintf(fp, "    Use Aka        %s\n", aka2str(fgroup.UseAka));
		fprintf(fp, "    Uplink         %s\n", aka2str(fgroup.UpLink));
		fprintf(fp, "    Areas file     %s\n", fgroup.AreaFile);
		fprintf(fp, "    Divice cost    %s\n", getboolean(fgroup.DivideCost));
		fprintf(fp, "    Unit cost      %ld\n", fgroup.UnitCost);
		fprintf(fp, "    Unit size      %ld\n", fgroup.UnitSize);
		fprintf(fp, "    Add promille   %ld\n", fgroup.AddProm);
		fprintf(fp, "    Start area     %ld\n", fgroup.StartArea);
		fprintf(fp, "    Banner file    %s\n", fgroup.Banner);
		fprintf(fp, "    Def. archiver  %s\n", fgroup.Convert);
		fprintf(fp, "    Filegate fmt   %s\n", getboolean(fgroup.FileGate));
		fprintf(fp, "    Auto change    %s\n", getboolean(fgroup.AutoChange));
		fprintf(fp, "    User change    %s\n", getboolean(fgroup.UserChange));
		fprintf(fp, "    Allow replace  %s\n", getboolean(fgroup.Replace));
		fprintf(fp, "    Dupe checking  %s\n", getboolean(fgroup.DupCheck));
		fprintf(fp, "    Secure         %s\n", getboolean(fgroup.Secure));
		fprintf(fp, "    No touch dates %s\n", getboolean(fgroup.NoTouch));
		fprintf(fp, "    Virus scan     %s\n", getboolean(fgroup.VirScan));
		fprintf(fp, "    Announce       %s\n", getboolean(fgroup.Announce));
		fprintf(fp, "    Update magics  %s\n", getboolean(fgroup.UpdMagic));
		fprintf(fp, "    FILE_ID.DIZ    %s\n", getboolean(fgroup.FileId));
		fprintf(fp, "    Convert all    %s\n", getboolean(fgroup.ConvertAll));
		fprintf(fp, "    Send original  %s\n", getboolean(fgroup.SendOrg));
		fprintf(fp, "    Base path      %s\n", fgroup.BasePath);
		fprintf(fp, "    Download sec.  %s\n", get_secstr(fgroup.DLSec));
		fprintf(fp, "    Upload sec.    %s\n", get_secstr(fgroup.UPSec));
		fprintf(fp, "    List security  %s\n", get_secstr(fgroup.LTSec));
		fprintf(fp, "    BBS group      %s\n", fgroup.BbsGroup);
		fprintf(fp, "    Announce group %s\n", fgroup.AnnGroup);
		fprintf(fp, "    Upload area    %d\n", fgroup.Upload);
		fprintf(fp, "    Start date     %s", ctime(&fgroup.StartDate));
		fprintf(fp, "    Last date      %s\n", ctime(&fgroup.LastDate));
	}

	fclose(no);
	return page;
}



