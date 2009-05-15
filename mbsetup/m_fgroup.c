/*****************************************************************************
 *
 * $Id: m_fgroup.c,v 1.42 2006/02/24 20:33:28 mbse Exp $
 * Purpose ...............: Setup FGroups.
 *
 *****************************************************************************
 * Copyright (C) 1997-2006
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

	snprintf(ffile, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
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
	int	oldsize;

	snprintf(fnin,  PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	snprintf(fnout, PATH_MAX, "%s/etc/fgroups.temp", getenv("MBSE_ROOT"));
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
				if (FGrpUpdated && !strlen(fgroup.BasePath)) {
				    fgroup.DupCheck  = TRUE;
				    fgroup.Secure    = TRUE;
				    fgroup.VirScan   = TRUE;
				    fgroup.Announce  = TRUE;
				    fgroup.UpdMagic  = TRUE;
				    fgroup.FileId    = TRUE;
				    memset(&temp, 0, sizeof(temp));
				    strcpy(temp, fgroup.Name);
				    snprintf(fgroup.BasePath, 65, "%s/ftp/pub/%s", getenv("MBSE_ROOT"), tl(temp));
				}
				if (FGrpUpdated && !fgroup.LinkSec.level) {
				    fgroup.LinkSec.level = 1;
				    fgroup.LinkSec.flags = 1;
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

	snprintf(fin,  PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/fgroups.temp", getenv("MBSE_ROOT"));

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
			disk_reset();
			Syslog('+', "Updated \"fgroups.data\"");
			if (!force)
			    working(6, 0, 0);
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

	snprintf(ffile, PATH_MAX, "%s/etc/fgroups.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&fgroup, 0, sizeof(fgroup));
		fgroup.StartDate = time(NULL);
		fgroup.FileGate = TRUE;
		fgroup.Secure = TRUE;
		fgroup.VirScan = TRUE;
		fgroup.Announce = TRUE;
		fgroup.FileId = TRUE;
		fgroup.DupCheck = TRUE;
		fgroup.Replace = TRUE;
		fgroup.LinkSec.level = 1;
		fgroup.LinkSec.flags = 1;
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
	mbse_mvprintw( 4, 2, "10.1 EDIT FILE GROUP");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 6, 2, "1.  Name");
	mbse_mvprintw( 7, 2, "2.  Comment");
	mbse_mvprintw( 8, 2, "3.  Base path");
	mbse_mvprintw( 9, 2, "4.  Use Aka");
	mbse_mvprintw(10, 2, "5.  Uplink");
	mbse_mvprintw(11, 2, "6.  Areas");
	mbse_mvprintw(12, 2, "7.  Filegate");
	mbse_mvprintw(13, 2, "8.  Banner");
	mbse_mvprintw(14, 2, "9.  Convert");
	mbse_mvprintw(15, 2, "10. BBS group");
	mbse_mvprintw(16, 2, "11. New group");
	mbse_mvprintw(17, 2, "12. Active");
	mbse_mvprintw(18, 2, "13. Deleted");
	mbse_mvprintw(19, 2, "14. Start at");

	mbse_mvprintw(12,32, "15. Auto chng");
	mbse_mvprintw(13,32, "16. User chng");
	mbse_mvprintw(14,32, "17. Replace");
	mbse_mvprintw(15,32, "18. Dupecheck");
	mbse_mvprintw(16,32, "19. Secure");
	mbse_mvprintw(17,32, "20. Touch");
	mbse_mvprintw(18,32, "21. Virscan");
	mbse_mvprintw(19,32, "22. Announce");

	mbse_mvprintw(11,56, "23. Upd magic");
	mbse_mvprintw(12,56, "24. File ID");
	mbse_mvprintw(13,56, "25. Conv. all");
	mbse_mvprintw(14,56, "26. Send orig");
	mbse_mvprintw(15,56, "27. DL sec");
	mbse_mvprintw(16,56, "28. UP sec");
	mbse_mvprintw(17,56, "29. LT sec");
	mbse_mvprintw(18,56, "30. Upl. area");
	mbse_mvprintw(19,56, "31. Link sec");
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditFGrpRec(int Area)
{
	FILE		*fil;
	char		mfile[PATH_MAX], temp[13];
	int		offset;
	int		i, j, tmp;
	unsigned int	crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit FileGroup");

	snprintf(mfile, PATH_MAX, "%s/etc/fgroups.temp", getenv("MBSE_ROOT"));
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

		show_bool(12,46,    fgroup.AutoChange);
		show_bool(13,46,    fgroup.UserChange);
		show_bool(14,46,    fgroup.Replace);
		show_bool(15,46,    fgroup.DupCheck);
		show_bool(16,46,    fgroup.Secure);
		show_bool(17,46,    fgroup.Touch);
		show_bool(18,46,    fgroup.VirScan);
		show_bool(19,46,    fgroup.Announce);

		show_bool(11,70,    fgroup.UpdMagic);
		show_bool(12,70,    fgroup.FileId);
		show_bool(13,70,    fgroup.ConvertAll);
		show_bool(14,70,    fgroup.SendOrg);
		show_int( 15,70,    fgroup.DLSec.level);
		show_int( 16,70,    fgroup.UPSec.level);
		show_int( 17,70,    fgroup.LTSec.level);
		show_int( 18,70,    fgroup.Upload);

		j = select_menu(31);
		switch(j) {
		case 0:	if (!fgroup.StartArea && strlen(fgroup.AreaFile)) {
			    errmsg("Areas file defined but no BBS start area");
			    break;
			}
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
					working(6, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		
		case 1: if (CheckFgroup())
				break;
			strcpy(fgroup.Name, edit_ups(6,16,12, fgroup.Name, (char *)"The ^name^ of this file group"));
			if (strlen(fgroup.BasePath) == 0) {
			    memset(&temp, 0, sizeof(temp));
			    strcpy(temp, fgroup.Name);
			    for (i = 0; i < strlen(temp); i++) {
				if (temp[i] == '.')
				    temp[i] = '/';
				if (isupper(temp[i]))
				    temp[i] = tolower(temp[i]);
			    }
			    snprintf(fgroup.BasePath, 65, "%s/%s", CFG.ftp_base, temp);
			}
			if (strlen(fgroup.BbsGroup) == 0)
			    strcpy(fgroup.BbsGroup, fgroup.Name);
			break;
		case 2: E_STR(  7,16,55,fgroup.Comment,    "The ^description^ of this file group")
		case 3: E_PTH(  8,16,64,fgroup.BasePath,   "The ^base path^ for new created file areas", 0775)
		case 4: tmp = PickAka((char *)"10.1.4", TRUE);
			if (tmp != -1)
				fgroup.UseAka = CFG.aka[tmp];
			FgScreen();
			break;
		case 5:	fgroup.UpLink = PullUplink((char *)"10.1.5"); 
			FgScreen();
			break;
		case 6: E_STR( 11,16,12,fgroup.AreaFile,   "The name of the ^Areas File^ from the uplink (case sensitive)")
		case 7: E_BOOL(12,16,   fgroup.FileGate,   "Is the areas file in ^filegate.zxx^ format")
		case 8: E_STR( 13,16,14,fgroup.Banner,     "The ^banner^ to add to the archives")
		case 9: strcpy(fgroup.Convert, PickArchive((char *)"10.1.9", FALSE));
			FgScreen();
			break;
		case 10:strncpy(fgroup.BbsGroup, PickFGroup((char *)"8.4.17"), 12);
			FgScreen();
			break;
		case 11:strncpy(fgroup.AnnGroup, PickNGroup((char *)"8.4.18"), 12);
			FgScreen();
			break;
		case 12:if (fgroup.Active && CheckFgroup())
			    break;
			E_BOOL(17,16,   fgroup.Active,     "Is this file group ^active^")
		case 13:if (CheckFgroup())
				break;
			E_BOOL(18,16,   fgroup.Deleted,    "Is this file group ^Deleted^")
		case 14:E_INT( 19,16,   fgroup.StartArea,  "The ^start area^ to create new BBS areas")

		case 15:E_BOOL(12,46,   fgroup.AutoChange, "^Automatic change areas^ when a new arealist is received")
		case 16:tmp = edit_bool(13,46, fgroup.UserChange, (char *)"Create new areas when ^users^ request new tic areas");
			if (tmp && !fgroup.UpLink.zone)
			    errmsg("It looks like you are at the toplevel, no Uplink defined");
			else
			    fgroup.UserChange = tmp;
			break;
		case 17:E_BOOL(14,46,   fgroup.Replace,    "Set ^Replace^ in new created tic areas")
		case 18:E_BOOL(15,46,   fgroup.DupCheck,   "Set ^Dupe check^ in new created tic areas")
		case 19:E_BOOL(16,46,   fgroup.Secure,     "Set ^Secure^ tic processing in new created tic areas")
		case 20:E_BOOL(17,46,   fgroup.Touch,      "Set ^Touch filedate^ in new created tic areas")
		case 21:E_BOOL(18,46,   fgroup.VirScan,    "Set ^Virus scanner^ in new created tic areas")
		case 22:E_BOOL(19,46,   fgroup.Announce,   "Set ^Announce files^ in new created tic areas")

		case 23:E_BOOL(11,70,   fgroup.UpdMagic,   "Set ^Update magic^ in new created tic areas")
		case 24:E_BOOL(12,70,   fgroup.FileId,     "Set ^FILE_ID.DIZ extract^ in new created tic areas")
		case 25:tmp = edit_bool(13,70, fgroup.ConvertAll, (char *)"Set ^Convert All^ setting in new created tic areas");
			if (tmp && !fgroup.ConvertAll && (strlen(fgroup.Convert) == 0))
			    errmsg("No archiver configured to convert to, edit 9 first");
			else
			    fgroup.ConvertAll = tmp;
			break;
		case 26:E_BOOL(14,70,   fgroup.SendOrg,    "Set ^Send original^ setting in new created tic areas")
		case 27:E_SEC( 15,70,   fgroup.DLSec,      "10.1.27 FILE GROUP DOWNLOAD SECURITY", FgScreen)
		case 28:E_SEC( 16,70,   fgroup.UPSec,      "10.1.28 FILE GROUP UPLOAD SECURITY", FgScreen)
		case 29:E_SEC( 17,70,   fgroup.LTSec,      "10.1.29 FILE GROUP LIST SECURITY", FgScreen)
		case 30:E_INT( 18,70,   fgroup.Upload,     "Set the default ^Upload area^ in new created file areas")
		case 31:fgroup.LinkSec = edit_asec(fgroup.LinkSec, (char *)"10.1.31 DEFAULT NEW TIC AREAS SECURITY");
			FgScreen();
			break;
		}
	}

	return 0;
}



void EditFGroup(void)
{
    int	    records, i, o, x, y;
    char    pick[12], temp[PATH_MAX];
    FILE    *fil;
    int	    offset;

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
    o = 0;
    if (! check_free())
	return;

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 4, "10.1 FILE GROUPS SETUP");
	set_color(CYAN, BLACK);
	if (records != 0) {
	    snprintf(temp, PATH_MAX, "%s/etc/fgroups.temp", getenv("MBSE_ROOT"));
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
			snprintf(temp, 81, "%3d.  %-12s %-18s", o + i, fgroup.Name, fgroup.Comment);
			temp[38] = '\0';
			mbse_mvprintw(y, x, temp);
			y++;
		    }
		}
		fclose(fil);
	    }
	}
	strcpy(pick, select_record(records, 20));
		
	if (strncmp(pick, "-", 1) == 0) {
	    CloseFGroup(FALSE);
	    open_bbs();
	    return;
	}

	if (strncmp(pick, "A", 1) == 0) {
	    if (records < CFG.tic_groups) {
		working(1, 0, 0);
		if (AppendFGroup() == 0) {
		    records++;
		    working(1, 0, 0);
		} else
		    working(2, 0, 0);
	    } else {
		errmsg("Cannot add group, change global setting in menu 1.11.5");
	    }
	}

	if (strncmp(pick, "N", 1) == 0)
	    if ((o + 20) < records)
		o += 20;

	if (strncmp(pick, "P", 1) == 0)
	    if ((o - 20) >= 0)
		o -= 20;

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
	int	offset;


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


	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		snprintf(temp, 81, "%s.  FILE GROUP SELECT", shdr);
		mbse_mvprintw( 5, 4, temp);
		set_color(CYAN, BLACK);
		if (records != 0) {
			snprintf(temp, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
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
						snprintf(temp, 81, "%3d.  %-12s %-18s", o + i, fgroup.Name, fgroup.Comment);
						temp[38] = '\0';
						mbse_mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
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
			snprintf(temp, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
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
    char    *temp, group[13];
    FILE    *ti, *wp, *ip, *no;
    int	    refs, i, First = TRUE;
    time_t  tt;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL) {
	free(temp);
	return page;
    }

    fread(&fgrouphdr, sizeof(fgrouphdr), 1, no);
    fseek(no, 0, SEEK_SET);
    fread(&fgrouphdr, fgrouphdr.hdrsize, 1, no);

    ip = open_webdoc((char *)"filegroup.html", (char *)"File Groups", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<P>\n");
    fprintf(ip, "<TABLE border='1' cellspacing='0' cellpadding='2'>\n");
    fprintf(ip, "<TBODY>\n");
    fprintf(ip, "<TR><TH align='left'>Group</TH><TH align='left'>Comment</TH><TH align='left'>Active</TH></TR>\n");
	    
    while ((fread(&fgroup, fgrouphdr.recsize, 1, no)) == 1) {
	if (First) {
	    addtoc(fp, toc, 10, 1, page, (char *)"File processing groups");
	    First = FALSE;
	    fprintf(fp, "\n");
	} else {
	    page = newpage(fp, page);
	    fprintf(fp, "\n\n");
	}

	snprintf(temp, 81, "filegroup_%s.html", fgroup.Name);
	fprintf(ip, " <TR><TD><A HREF=\"%s\">%s</A></TD><TD>%s</TD><TD>%s</TD></TR>\n", 
		temp, fgroup.Name, fgroup.Comment, getboolean(fgroup.Active));

	if ((wp = open_webdoc(temp, (char *)"File group", fgroup.Comment))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"filegroup.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Group name", fgroup.Name);
	    add_webtable(wp, (char *)"Comment", fgroup.Comment);
	    add_webtable(wp, (char *)"Active", getboolean(fgroup.Active));
	    add_webtable(wp, (char *)"Use Aka", aka2str(fgroup.UseAka));
	    add_webtable(wp, (char *)"Uplink Aka", aka2str(fgroup.UpLink));
	    add_webtable(wp, (char *)"Areas file", fgroup.AreaFile);
	    snprintf(temp, 81, "%d", fgroup.StartArea);
	    add_webtable(wp, (char *)"Start autocreate BBS area", temp);
	    add_webtable(wp, (char *)"Banner file", fgroup.Banner);
	    add_webtable(wp, (char *)"Default archiver", fgroup.Convert);
	    add_webtable(wp, (char *)"Area file in Filegate format", getboolean(fgroup.FileGate));
	    add_webtable(wp, (char *)"Auto change areas", getboolean(fgroup.AutoChange));
	    add_webtable(wp, (char *)"User (downlink) change areas", getboolean(fgroup.UserChange));
	    add_webtable(wp, (char *)"Allow replace", getboolean(fgroup.Replace));
	    add_webtable(wp, (char *)"Dupe checking", getboolean(fgroup.DupCheck));
	    add_webtable(wp, (char *)"Secure processing", getboolean(fgroup.Secure));
	    add_webtable(wp, (char *)"Touch file dates", getboolean(fgroup.Touch));
	    add_webtable(wp, (char *)"Virus scan", getboolean(fgroup.VirScan));
	    add_webtable(wp, (char *)"Announce", getboolean(fgroup.Announce));
	    add_webtable(wp, (char *)"Allow update magics", getboolean(fgroup.UpdMagic));
	    add_webtable(wp, (char *)"Extract FILE_ID.DIZ", getboolean(fgroup.FileId));
	    add_webtable(wp, (char *)"Convert all archives", getboolean(fgroup.ConvertAll));
	    add_webtable(wp, (char *)"Send original file", getboolean(fgroup.SendOrg));
	    add_webtable(wp, (char *)"Base path for new areas", fgroup.BasePath);
	    web_secflags(wp, (char *)"Download security", fgroup.DLSec);
	    web_secflags(wp, (char *)"Upload security", fgroup.UPSec);
	    web_secflags(wp, (char *)"List security", fgroup.LTSec);
	    add_webtable(wp, (char *)"Default tic security", getflag(fgroup.LinkSec.flags, fgroup.LinkSec.notflags));
	    fprintf(wp, "<TR><TH align='left'>BBS (tic) file group</TH><TD><A HREF=\"filegroup_%s.html\">%s</A></TD></TH>\n",
		fgroup.BbsGroup, fgroup.BbsGroup);
	    fprintf(wp, "<TR><TH align='left'>Newfiles announce group</TH><TD><A HREF=\"newgroup.html\">%s</A></TD></TH>\n",
		fgroup.AnnGroup);
	    snprintf(temp, 81, "%d", fgroup.Upload);
	    add_webtable(wp, (char *)"Upload area", temp);
	    tt = (time_t)fgroup.StartDate;
	    add_webtable(wp, (char *)"Start date", ctime(&tt));
	    tt = (time_t)fgroup.LastDate;
	    add_webtable(wp, (char *)"Last active date", ctime(&tt));
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>BBS File Areas Reference</H3>\n");
	    i = refs = 0;
	    snprintf(temp, PATH_MAX, "%s/etc/fareas.data", getenv("MBSE_ROOT"));
	    if ((ti = fopen(temp, "r"))) {
		fread(&areahdr, sizeof(areahdr), 1, ti);
		while ((fread(&area, areahdr.recsize, 1, ti)) == 1) {
		    i++;
		    if (area.Available && (strcmp(fgroup.Name, area.BbsGroup) == 0)) {
			if (refs == 0) {
			    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
			    fprintf(wp, "<COL width='20%%'><COL width='80%%'>\n");
			    fprintf(wp, "<TBODY>\n");
			}
			fprintf(wp, "<TR><TD><A HREF=\"filearea_%d.html\">Area %d</A></TD><TD>%s</TD></TR>\n", i, i, area.Name);
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
	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>TIC Areas Reference</H3>\n");
	    refs = 0;
	    snprintf(temp, PATH_MAX, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	    if ((ti = fopen(temp, "r"))) {
		fread(&tichdr, sizeof(tichdr), 1, ti);
		fseek(ti, 0, SEEK_SET);
		fread(&tichdr, tichdr.hdrsize, 1, ti);
		while ((fread(&tic, tichdr.recsize, 1, ti)) == 1) {
		    if (strcmp(fgroup.Name, tic.Group) == 0) {
			if (refs == 0) {
			    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
			    fprintf(wp, "<COL width='20%%'><COL width='80%%'>\n");
			    fprintf(wp, "<TBODY>\n");
			}
			fprintf(wp, "<TR><TD><A HREF=\"ticarea_%s.html\">%s</A></TD><TD>%s</TD></TR>\n", 
				tic.Name, tic.Name, tic.Comment);
			refs++;
		    }
		    fseek(ti, tichdr.syssize, SEEK_CUR);
		}
		fclose(ti);
	    }
	    if (refs == 0)
		fprintf(wp, "No TIC Areas References\n");
	    else {
		fprintf(wp, "</TBODY>\n");
		fprintf(wp, "</TABLE>\n");
	    }
	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>Nodes Reference</H3>\n");
	    refs = 0;
	    snprintf(temp, PATH_MAX, "%s/etc/nodes.data", getenv("MBSE_ROOT"));
	    if ((ti = fopen(temp, "r"))) {
		fread(&nodeshdr, sizeof(nodeshdr), 1, ti);
		fseek(ti, 0, SEEK_SET);
		fread(&nodeshdr, nodeshdr.hdrsize, 1, ti);
		while ((fread(&nodes, nodeshdr.recsize, 1, ti)) == 1) {
		    for (i = 0; i < nodeshdr.filegrp / sizeof(group); i++) {
			fread(&group, sizeof(group), 1, ti);
			if (strcmp(group, fgroup.Name) == 0) {
			    if (refs == 0) {
				fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
				fprintf(wp, "<COL width='20%%'><COL width='80%%'>\n");
				fprintf(wp, "<TBODY>\n");
			    }
			    fprintf(wp, "<TR><TD><A HREF=\"node_%d_%d_%d_%d_%s.html\">%s</A></TD><TD>%s</TD></TR>\n", 
				    nodes.Aka[0].zone, nodes.Aka[0].net, nodes.Aka[0].node, nodes.Aka[0].point, 
				    nodes.Aka[0].domain, aka2str(nodes.Aka[0]), nodes.Sysop);
			    refs++;
			}
		    }
		    fseek(ti, nodeshdr.mailgrp, SEEK_CUR);
		}
		fclose(ti);
	    }
	    if (refs == 0)
		fprintf(wp, "No Nodes References\n");
	    else {
		fprintf(wp, "</TBODY>\n");
		fprintf(wp, "</TABLE>\n");
	    }
	    fprintf(wp, "<HR>\n");
	    fprintf(wp, "<H3>Group Statistics</H3>\n");
	    add_statcnt(wp, (char *)"processed files", fgroup.Files);
	    add_statcnt(wp, (char *)"KBytes of files", fgroup.KBytes);
	    close_webdoc(wp);
	}
	
	fprintf(fp, "    Group name     %s\n", fgroup.Name);
	fprintf(fp, "    Comment        %s\n", fgroup.Comment);
	fprintf(fp, "    Active         %s\n", getboolean(fgroup.Active));
	fprintf(fp, "    Use Aka        %s\n", aka2str(fgroup.UseAka));
	fprintf(fp, "    Uplink         %s\n", aka2str(fgroup.UpLink));
	fprintf(fp, "    Areas file     %s\n", fgroup.AreaFile);
	fprintf(fp, "    Start area     %d\n", fgroup.StartArea);
	fprintf(fp, "    Banner file    %s\n", fgroup.Banner);
	fprintf(fp, "    Def. archiver  %s\n", fgroup.Convert);
	fprintf(fp, "    Filegate fmt   %s\n", getboolean(fgroup.FileGate));
	fprintf(fp, "    Auto change    %s\n", getboolean(fgroup.AutoChange));
	fprintf(fp, "    User change    %s\n", getboolean(fgroup.UserChange));
	fprintf(fp, "    Allow replace  %s\n", getboolean(fgroup.Replace));
	fprintf(fp, "    Dupe checking  %s\n", getboolean(fgroup.DupCheck));
	fprintf(fp, "    Secure         %s\n", getboolean(fgroup.Secure));
	fprintf(fp, "    Touch dates    %s\n", getboolean(fgroup.Touch));
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
	fprintf(fp, "    Def. tic sec.        %s\n", getflag(fgroup.LinkSec.flags, fgroup.LinkSec.notflags));
	fprintf(fp, "    BBS group      %s\n", fgroup.BbsGroup);
	fprintf(fp, "    Newfiles group %s\n", fgroup.AnnGroup);
	fprintf(fp, "    Upload area    %d\n", fgroup.Upload);
	tt = (time_t)fgroup.StartDate;
	fprintf(fp, "    Start date     %s", ctime(&tt));
	tt = (time_t)fgroup.LastDate;
	fprintf(fp, "    Last date      %s\n", ctime(&tt));
    }

    fprintf(ip, "</TBODY>\n");
    fprintf(ip, "</TABLE>\n");
    close_webdoc(ip);
    
    fclose(no);
    free(temp);
    return page;
}



