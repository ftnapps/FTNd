/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Setup MGroups.
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
	char	ffile[PATH_MAX];
	int	count;

	sprintf(ffile, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			mgrouphdr.hdrsize = sizeof(mgrouphdr);
			mgrouphdr.recsize = sizeof(mgroup);
			fwrite(&mgrouphdr, sizeof(mgrouphdr), 1, fil);
			memset(&mgroup, 0, sizeof(mgroup));
			sprintf(mgroup.Name, "NOGROUP");
			sprintf(mgroup.Comment, "Dummy group for badmail, dupemail");
			mgroup.Active = TRUE;
			fwrite(&mgroup, sizeof(mgroup), 1, fil);
			memset(&mgroup, 0, sizeof(mgroup));
			sprintf(mgroup.Name, "LOCAL");
			sprintf(mgroup.Comment, "Local mail areas");
			mgroup.Active = TRUE;
			fwrite(&mgroup, sizeof(mgroup), 1, fil);
			fclose(fil);
			chmod(ffile, 0640);
			return 2;
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
int OpenMGroup(void);
int OpenMGroup(void)
{
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX], temp[13];
	long	oldsize;
	int	i;

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

			if (MGrpUpdated)
			    Syslog('+', "Updated %s, format changed", fnin);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&mgroup, 0, sizeof(mgroup));
			while (fread(&mgroup, oldsize, 1, fin) == 1) {
				if (MGrpUpdated && !strlen(mgroup.BasePath)) {
				    memset(&temp, 0, sizeof(temp));
				    strcpy(temp, mgroup.Name);
				    for (i = 0; i < strlen(temp); i++) {
					if (isupper(temp[i]))
					    temp[i] = tolower(temp[i]);
					if (temp[i] == '.')
					    temp[i] = '/';
				    }
				    sprintf(mgroup.BasePath, "%s/var/mail/%s", getenv("MBSE_ROOT"), temp);
				}
				if (MGrpUpdated && !mgroup.LinkSec.level) {
				    mgroup.LinkSec.level = 1;
				    mgroup.LinkSec.flags = 1;
				}
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



void CloseMGroup(int);
void CloseMGroup(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];
	FILE	*fi, *fo;
	st_list	*mgr = NULL, *tmp;

	sprintf(fin, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/mgroups.temp", getenv("MBSE_ROOT"));

	if (MGrpUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
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
			chmod(fin, 0640);
			Syslog('+', "Updated \"mgroups.data\"");
			return;
		}
	}
	chmod(fin, 0640);
	working(1, 0, 0);
	unlink(fout); 
}



int AppendMGroup(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	sprintf(ffile, "%s/etc/mgroups.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&mgroup, 0, sizeof(mgroup));
		mgroup.StartDate = time(NULL);
		mgroup.LinkSec.level = 1;
		mgroup.LinkSec.flags = 1;
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
	mvprintw( 5, 2, "9.1 EDIT MESSAGE GROUP");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.  Name");
	mvprintw( 8, 2, "2.  Comment");
	mvprintw( 9, 2, "3.  Base path");
	mvprintw(10, 2, "4.  Read sec");
	mvprintw(11, 2, "5.  Write sec");
	mvprintw(12, 2, "6.  Sysop sec");
	mvprintw(13, 2, "7.  Link sec");
	mvprintw(14, 2, "8.  Start at");
	mvprintw(15, 2, "9.  Net reply");
	mvprintw(16, 2, "10. Users del");
	mvprintw(17, 2, "11. Aliases");
	mvprintw(18, 2, "12. Quotes");
	mvprintw(19, 2, "13. Active");

	mvprintw(14,41, "14. Deleted");
	mvprintw(15,41, "15. Auto change");
	mvprintw(16,41, "16. User change");
	mvprintw(17,41, "17. Use Aka");
	mvprintw(18,41, "18. Uplink");
	mvprintw(19,41, "19. Areas");
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
	static char	mfile[PATH_MAX], temp[13];
	static long	offset;
	static int	i, j, tmp;
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
		show_str(  7,16,12, mgroup.Name);
		show_str(  8,16,55, mgroup.Comment);
		show_str(  9,16,64, mgroup.BasePath);
		show_sec( 10,16,    mgroup.RDSec);
		show_sec( 11,16,    mgroup.WRSec);
		show_sec( 12,16,    mgroup.SYSec);
		mvprintw( 13,22,    getflag(mgroup.LinkSec.flags, mgroup.LinkSec.notflags));
		show_int( 14,16,    mgroup.StartArea);
		show_int( 15,16,    mgroup.NetReply);
		show_bool(16,16,    mgroup.UsrDelete);
		show_bool(17,16,    mgroup.Aliases);
		show_bool(18,16,    mgroup.Quotes);
		show_bool(19,16,    mgroup.Active);

		show_bool(14,57,    mgroup.Deleted);
		show_bool(15,57,    mgroup.AutoChange);
		show_bool(16,57,    mgroup.UserChange);
		show_aka( 17,57,    mgroup.UseAka);
		show_aka( 18,57,    mgroup.UpLink);
		show_str( 19,57,12, mgroup.AreaFile);

		j = select_menu(19);
		switch(j) {
		case 0:	if (!mgroup.StartArea && strlen(mgroup.AreaFile)) {
			    errmsg("Areas file defined but no BBS start area");
			    break;
			}
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
			strcpy(mgroup.Name, edit_str(7,16,12, mgroup.Name, (char *)"The ^name^ for this message group"));
			if (strlen(mgroup.BasePath) == 0) {
			    memset(&temp, 0, sizeof(temp));
			    strcpy(temp, mgroup.Name);
			    for (i = 0; i < strlen(temp); i++) {
				if (temp[i] == '.')
				    temp[i] = '/';
				if (isupper(temp[i]))
				    temp[i] = tolower(temp[i]);
			    }
			    sprintf(mgroup.BasePath, "%s/var/mail/%s", getenv("MBSE_ROOT"), temp);
			}
			break;
		case 2: E_STR(  8,16,55, mgroup.Comment,    "The ^desription^ for this message group")
		case 3: E_PTH(  9,16,64, mgroup.BasePath,   "The ^Base path^ where new JAM areas are created", 0770)
		case 4: E_SEC( 10,16,    mgroup.RDSec,      "9.1.4 MESSAGE GROUP READ SECURITY", MgScreen)
		case 5: E_SEC( 11,16,    mgroup.WRSec,      "9.1.5 MESSAGE GROUP WRITE SECURITY", MgScreen)
		case 6: E_SEC( 12,16,    mgroup.SYSec,      "9.1.6 MESSAGE GROUP SYSOP SECURITY", MgScreen)
		case 7: mgroup.LinkSec = edit_asec(mgroup.LinkSec, (char *)"9.1.7 DEFAULT SECURITY FOR NEW AREAS");
			MgScreen();
			break;
		case 8: E_INT( 14,16,    mgroup.StartArea,  "The ^Start area number^ from where to add areas")
		case 9: E_INT( 15,16,    mgroup.NetReply,   "The ^Area Number^ for netmail replies")
		case 10:E_BOOL(16,16,    mgroup.UsrDelete,  "Allow users to ^Delete^ their messages")
		case 11:E_BOOL(17,16,    mgroup.Aliases,    "Allow ^Aliases^ or real names only")
		case 12:E_BOOL(18,16,    mgroup.Quotes,     "Allow random ^quotes^ to new messages")
		case 13:if (CheckMgroup())
			    break;
			E_BOOL(19,16,    mgroup.Active,     "Is this message group ^active^")
		case 14:if (CheckMgroup())
			    break;
			E_BOOL(14,57,    mgroup.Deleted,    "Is this group ^Deleted^")
		case 15:E_BOOL(15,57,    mgroup.AutoChange, "^Auto change^ areas from new areas lists")
		case 16:tmp = edit_bool(16,57, mgroup.UserChange, (char *)"^Auto add/delete^ areas from downlinks requests");
			if (tmp && !mgroup.UpLink.zone)
			    errmsg("It looks like you are the toplevel, no Uplink defined");
			else
			    mgroup.UserChange = tmp;
			break;
		case 17:tmp = PickAka((char *)"9.1.17", TRUE);
			if (tmp != -1)
				memcpy(&mgroup.UseAka, &CFG.aka[tmp], sizeof(fidoaddr));
			MgScreen();
			break;
		case 18:mgroup.UpLink = PullUplink((char *)"9.1.18");
			MgScreen();
			break;
		case 19:E_STR( 19,57,12, mgroup.AreaFile,   "The name of the ^Areas File^ from the uplink")
		}
	}

	return 0;
}



void EditMGroup(void)
{
    int	    records, i, o, x, y;
    char    pick[12], temp[PATH_MAX];
    FILE    *fil;
    long    offset;

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
    if (! check_free())
	return;

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
	    CloseMGroup(FALSE);
	    open_bbs();
	    return;
	}

	if (strncmp(pick, "A", 1) == 0) {
	    if (records < CFG.toss_groups) {
		working(1, 0, 0);
		if (AppendMGroup() == 0) {
		    records++;
		    working(1, 0, 0);
		} else
		    working(2, 0, 0);
		working(0, 0, 0);
	    } else {
		errmsg("Cannot add group, change global setting in menu 1.14.12");
	    }
	}
	

	if (strncmp(pick, "N", 1) == 0)
	    if ((o + 20) < records)
		o = o + 20;

	if (strncmp(pick, "P", 1) == 0)
	    if ((o - 20) >= 0)
		o = o - 20;

	if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
	    EditMGrpRec(atoi(pick));
	    o = ((atoi(pick) - 1) / 20) * 20;
	}
    }
}



void InitMGroup(void)
{
    CountMGroup();
    OpenMGroup();
    CloseMGroup(TRUE);
}



char *PickMGroup(char *shdr)
{
	static	char MGrp[21] = "";
	int	records, i, o = 0, y, x;
	char	pick[12];
	FILE	*fil;
	char	temp[PATH_MAX];
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
	char	temp[PATH_MAX];
	FILE	*no;
	int	j;

	sprintf(temp, "%s/etc/mgroups.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	addtoc(fp, toc, 9, 1, page, (char *)"Mail processing groups");
	j = 0;
	fprintf(fp, "\n");

	fread(&mgrouphdr, sizeof(mgrouphdr), 1, no);
	fseek(no, 0, SEEK_SET);
	fread(&mgrouphdr, mgrouphdr.hdrsize, 1, no);

	while ((fread(&mgroup, mgrouphdr.recsize, 1, no)) == 1) {
		if (j == 2) {
			page = newpage(fp, page);
			fprintf(fp, "\n");
			j = 0;
		}

		fprintf(fp, "    Group name         %s\n", mgroup.Name);
		fprintf(fp, "    Comment            %s\n", mgroup.Comment);
		fprintf(fp, "    Active             %s\n", getboolean(mgroup.Active));
		fprintf(fp, "    Use Aka            %s\n", aka2str(mgroup.UseAka));
		fprintf(fp, "    Uplink             %s\n", aka2str(mgroup.UpLink));
		fprintf(fp, "    Areas file         %s\n", mgroup.AreaFile);
		fprintf(fp, "    Base path          %s\n", mgroup.BasePath);
		fprintf(fp, "    Netmail reply area %d\n", mgroup.NetReply);
		fprintf(fp, "    Start new areas at %d\n", mgroup.StartArea);
		fprintf(fp, "    Read security      %s\n", get_secstr(mgroup.RDSec));
		fprintf(fp, "    Write security     %s\n", get_secstr(mgroup.WRSec));
		fprintf(fp, "    Sysop security     %s\n", get_secstr(mgroup.SYSec));
		fprintf(fp, "    Def. link security       %s\n", getflag(mgroup.LinkSec.flags, mgroup.LinkSec.notflags));
		fprintf(fp, "    Use aliases        %s\n", getboolean(mgroup.Aliases));
		fprintf(fp, "    Add quotes         %s\n", getboolean(mgroup.Quotes));
		fprintf(fp, "    Auto add/del areas %s\n", getboolean(mgroup.AutoChange));
		fprintf(fp, "    user add/del areas %s\n", getboolean(mgroup.UserChange));
		fprintf(fp, "    Start area date    %s",   ctime(&mgroup.StartDate));
		fprintf(fp, "    Last active date   %s\n", ctime(&mgroup.LastDate));
		fprintf(fp, "\n\n");
		j++;
	}

	fclose(no);
	return page;
}


