/*****************************************************************************
 *
 * File ..................: m_ticareas.c
 * Purpose ...............: TIC Areas Setup Program 
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
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "stlist.h"
#include "grlist.h"
#include "m_global.h"
#include "m_node.h"
#include "m_fgroup.h"
#include "m_farea.h"
#include "m_archive.h"
#include "m_ticarea.h"


int		TicUpdated = 0;
unsigned long	TicCrc;
FILE		*ttfil = NULL;



/*
 * Count nr of tic records in the database.
 * Creates the database if it doesn't exist.
 */
int CountTicarea(void)
{
	FILE	*fil;
	char	ffile[81];
	int	count;

	sprintf(ffile, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created tic.data");
			tichdr.hdrsize = sizeof(tichdr);
			tichdr.recsize = sizeof(tic);
			tichdr.syssize = CFG.tic_systems * sizeof(sysconnect);
			tichdr.lastupd = time(NULL);
			fwrite(&tichdr, sizeof(tichdr), 1, fil);
			fclose(fil);
			return 0;
		} else
			return -1;
	}

	fread(&tichdr, sizeof(tichdr), 1, fil);
	fseek(fil, 0, SEEK_SET);
	fread(&tichdr, tichdr.hdrsize, 1, fil);
	fseek(fil, 0, SEEK_END);
	count = (ftell(fil) - tichdr.hdrsize) / (tichdr.recsize + tichdr.syssize);
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenTicarea(void)
{
	FILE	*fin, *fout;
	char	fnin[81], fnout[81];
	long	oldsize, oldsys;
	struct	_sysconnect syscon;
	int	i, oldsystems;

	sprintf(fnin,  "%s/etc/tic.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/tic.temp", getenv("MBSE_ROOT"));
	if ((fin = fopen(fnin, "r")) != NULL) {
		if ((fout = fopen(fnout, "w")) != NULL) {
			Syslog('+', "Opened \"tic.data\"");
			TicUpdated = 0;
			fread(&tichdr, sizeof(tichdr), 1, fin);
			fseek(fin, 0, SEEK_SET);
			fread(&tichdr, tichdr.hdrsize, 1, fin);
			if (tichdr.hdrsize != sizeof(tichdr)) {
				tichdr.hdrsize = sizeof(tichdr);
				tichdr.lastupd = time(NULL);
				TicUpdated = 1;
			}

			/*
			 * In case we are automatic upgrading the data format
			 * we save the old format. If it is changed, the
			 * database must always be updated.
			 */
			oldsize = tichdr.recsize;
			oldsys  = tichdr.syssize;
			oldsystems = oldsys / sizeof(syscon);
			if ((oldsize != sizeof(tic)) || (CFG.tic_systems != oldsystems)) {
				TicUpdated = 1;
				Syslog('+', "\"tic.data\" nr of systems is changed");
			}
			tichdr.hdrsize = sizeof(tichdr);
			tichdr.recsize = sizeof(tic);
			tichdr.syssize = sizeof(syscon) * CFG.tic_systems;
			fwrite(&tichdr, sizeof(tichdr), 1, fout);

			/*
			 * The datarecord is filled with zero's before each
			 * read, so if the format changed, the new fields
			 * will be empty.
			 */
			memset(&tic, 0, sizeof(tic));
			while (fread(&tic, oldsize, 1, fin) == 1) {
				fwrite(&tic, sizeof(tic), 1, fout);
				memset(&tic, 0, sizeof(tic));
				/*
				 * Copy the existing connections
				 */
				for (i = 1; i <= oldsystems; i++) {
					fread(&syscon, sizeof(syscon), 1, fin);
					/* 
					 * Write only valid records
					 */
					if (i <= CFG.tic_systems) 
						fwrite(&syscon, sizeof(syscon), 1, fout);
				}
				if (oldsystems < CFG.tic_systems) {
					/*
					 * The size is increased, fill with
					 * blank records.
					 */
					memset(&syscon, 0, sizeof(syscon));
					for (i = (oldsystems + 1); i <= CFG.tic_systems; i++)
						fwrite(&syscon, sizeof(syscon), 1, fout);
				}
			}

			fclose(fin);
			fclose(fout);
			Syslog('+', "Opened \"tic.data\"");
			if (TicUpdated)
				Syslog('+', "Updated \"tic.data\" data format");
			return 0;
		} else
			return -1;
	}
	return -1;
}



void CloseTicarea(int Force)
{
	char	fin[81], fout[81];
	FILE	*fi, *fo;
	st_list	*tir = NULL, *tmp;
	int	i;
	struct	_sysconnect syscon;

	sprintf(fin, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/tic.temp", getenv("MBSE_ROOT"));

	if (TicUpdated == 1) {
		if (Force || (yes_no((char *)"Tic areas database is changed, save changes")) == 1) {
			working(1, 0, 0);
			fi = fopen(fout, "r");
			fo = fopen(fin,  "w");
			fread(&tichdr, tichdr.hdrsize, 1, fi);
			fwrite(&tichdr, tichdr.hdrsize, 1, fo);

			while (fread(&tic, tichdr.recsize, 1, fi) == 1) {
				if (!tic.Deleted)
					fill_stlist(&tir, tic.Name, ftell(fi) - tichdr.recsize);
				fseek(fi, tichdr.syssize, SEEK_CUR);
			}
			sort_stlist(&tir);

			for (tmp = tir; tmp; tmp = tmp->next) {
				fseek(fi, tmp->pos, SEEK_SET);
				fread(&tic, tichdr.recsize, 1, fi);
				fwrite(&tic, tichdr.recsize, 1, fo);
				for (i = 0; i < (tichdr.syssize / sizeof(syscon)); i++) {
					fread(&syscon, sizeof(syscon), 1, fi);
					fwrite(&syscon, sizeof(syscon), 1, fo);
				}
			}

			tidy_stlist(&tir);
			fclose(fi);
			fclose(fo);
			unlink(fout);
			Syslog('+', "Updated \"tic.data\"");
			return;
		}
	}
	working(1, 0, 0);
	unlink(fout); 
	Syslog('+', "No update of \"tic.data\"");
}



int AppendTicarea(void);
int AppendTicarea(void)
{
	FILE	*fil;
	char	ffile[81];
	struct	_sysconnect syscon;
	int	i;

	sprintf(ffile, "%s/etc/tic.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&tic, 0, sizeof(tic));
		/*
		 * Fill in default values
		 */
		tic.DupCheck   = TRUE;
		tic.Secure     = TRUE;
		tic.VirScan    = TRUE;
		tic.Announce   = TRUE;
		tic.UpdMagic   = TRUE;
		tic.FileId     = TRUE;
		tic.Active     = TRUE;
		tic.AreaStart  = time(NULL);
		fwrite(&tic, sizeof(tic), 1, fil);
		memset(&syscon, 0, sizeof(syscon));
		for (i = 1; i <= CFG.tic_systems; i++)
			fwrite(&syscon, sizeof(syscon), 1, fil);
		fclose(fil);
		TicUpdated = 1;
		return 0;
	} else
		return -1;
}



void EditTicSystem(sysconnect *);
void EditTicSystem(sysconnect *Sys)
{
	sysconnect	S;
	unsigned short	zone = 0;
	int		refresh = TRUE;

	S = (* Sys);
	for (;;) {
		if (refresh) {
			clr_index();
			set_color(WHITE, BLACK);
			mvprintw( 5,6, "10.2.25 EDIT CONNECTION");
			set_color(CYAN, BLACK);
			mvprintw( 7,6, "1.      Aka");
			mvprintw( 8,6, "2.      Send to");
			mvprintw( 9,6, "3.      Recv from");
			mvprintw(10,6, "4.      Pause");
			mvprintw(11,6, "5.      Delete");
			refresh = FALSE;
		}

		set_color(WHITE, BLACK);
		show_str(  7,24,23, aka2str(S.aka));
		show_bool( 8,24, S.sendto);
		show_bool( 9,24, S.receivefrom);
		show_bool(10,24, S.pause);
		zone = S.aka.zone;

		switch(select_menu(5)) {
			case 0:	(* Sys) = S;
				return;
			case 1:	S.aka = PullUplink((char *)"10.2.25");
				refresh = TRUE;
				break;
			case 2: E_BOOL( 8,24, S.sendto,      "^Send^ files ^to^ this node")
			case 3: E_BOOL( 9,24, S.receivefrom, "^Receive^ files ^from^ this node")
			case 4: E_BOOL(10,24, S.pause,       "Is this node ^paused^")
			case 5: if (yes_no((char *)"Delete this entry")) {
					memset(&S, 0, sizeof(sysconnect));
					(* Sys) = S;
					return;
				}
				break;
		}

		/*
		 * Set sendto to on when a new
		 * zone is entered.
		 */
		if ((S.aka.zone) && (!zone)) {
			S.sendto = 1;
		}
	}
}



int EditTicConnections(FILE *);
int EditTicConnections(FILE *fil)
{
	int		systems, o = 0, i, y, x;
	long		offset;
	char		pick[12];
	sysconnect	System;
	char		status[4];
	char		temp[41];

	systems = tichdr.syssize / sizeof(sysconnect);
	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 5, "10.2.25 TIC AREA CONNECTIONS");
		set_color(CYAN, BLACK);
		y = 7;
		x = 2;
		for (i = 1; i <= 20; i++) {
			if ((o+i-1) < systems) {
				if (i == 11) {
					y = 7;
					x = 42;
				}
				offset = (o+i-1) * sizeof(sysconnect);
				if ((fseek(fil, offset, 0)) != 0) {
					working(2, 0, 0);
					return FALSE;
				}
				fread(&System, sizeof(sysconnect), 1, fil);
				memset(&status, 0, 4);
				memset(&status, '-', 3);
				if (System.sendto)
					status[0] = 'S';
				if (System.receivefrom)
					status[1] = 'R';
				if (System.pause)
					status[2] = 'P';
				if (System.aka.zone) {
					set_color(CYAN,BLACK);
					sprintf(temp, "%3d. %s %s", o+i, status, aka2str(System.aka));
				} else {
					set_color(LIGHTBLUE, BLACK);
					sprintf(temp, "%3d.", o+i);
				}
				mvprintw(y, x, temp);
				y++;
			}
		}
		strcpy(pick, select_pick(systems, 20));

		if (strncmp(pick, "-", 1) == 0) {
			return FALSE;
		}

		if (strncmp(pick, "N", 1) == 0) 
			if ((o + 20) < systems)
				o = o + 20;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 20) >= 0)
				o = o - 20;

		if (((atoi(pick) > 0) && (atoi(pick) <= systems))) {
			offset = (atoi(pick) -1) * sizeof(sysconnect);
			fseek(fil, offset, 0);
			fread(&System, sizeof(sysconnect), 1, fil);
			EditTicSystem(&System);
			fseek(fil, offset, 0);
			fwrite(&System, sizeof(sysconnect), 1, fil);
		}
	}
}



void SetTicScreen(void);
void SetTicScreen(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 4, 2, "10.2 EDIT TIC AREA");
	set_color(CYAN, BLACK);
	mvprintw( 6, 2, "1.  Comment");
	mvprintw( 7, 2, "2.  Area tag");
	mvprintw( 8, 2, "3.  BBS area");
	mvprintw( 9, 2, "4.  Message");
	mvprintw(10, 2, "5.  Group");
	mvprintw(11, 2, "6.  Keep #");
	mvprintw(12, 2, "7.  Fido aka");
	mvprintw(13, 2, "8.  Convert");
	mvprintw(14, 2, "9.  Banner");
	mvprintw(15, 2, "10. Replace");

	mvprintw( 7,37, "11. Dupecheck");
	mvprintw( 8,37, "12. Secure");
	mvprintw( 9,37, "13. No touch");
	mvprintw(10,37, "14. Virus sc.");
	mvprintw(11,37, "15. Announce");
	mvprintw(12,37, "16. Upd magic");
	mvprintw(13,37, "17. File_id");
	mvprintw(14,37, "18. Conv.all");
	mvprintw(15,37, "19. Send org.");

	mvprintw( 7,59, "20. Mandatory");
	mvprintw( 8,59, "21. Notified");
	mvprintw( 9,59, "22. Upl discon");
	mvprintw(10,59, "23. Deleted");
	mvprintw(11,59, "24. Active");
	mvprintw(12,59, "25. Systems");
} 



long LoadTicRec(int, int);
long LoadTicRec(int Area, int work)
{
	FILE            *fil;
	char            mfile[81];
	long            offset;
	sysconnect      System;
	int		i;

	if (work) 
		working(1, 0, 0);

	sprintf(mfile, "%s/etc/tic.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r")) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	if ((ttfil = tmpfile()) == NULL) {
		working(2, 0, 0);
		return -1;
	}

	fread(&tichdr, sizeof(tichdr), 1, fil);
	offset = tichdr.hdrsize + (((Area -1) * (tichdr.recsize + tichdr.syssize)));
	if (fseek(fil, offset, 0) != 0) {
		fclose(ttfil);
		working(2, 0, 0);
		return -1;
	}

	fread(&tic, tichdr.recsize, 1, fil);
	TicCrc = 0xffffffff;
	TicCrc = upd_crc32((char *)&tic, TicCrc, tichdr.recsize);
	for (i = 0; i <= (tichdr.syssize / sizeof(sysconnect)); i++) {
		fread(&System, sizeof(sysconnect), 1, fil);
		fwrite(&System, sizeof(sysconnect), 1, ttfil);
		TicCrc = upd_crc32((char *)&System, TicCrc, sizeof(sysconnect));
	}
	fclose(fil);
	if (work)
		working(0, 0, 0);
	return offset;
}



int SaveTicRec(int, int);
int SaveTicRec(int Area, int work)
{
	int             i;
	FILE            *fil;
	long            offset;
	char            mfile[81];
	sysconnect      System;

	if (work)
		working(1, 0, 0);
	sprintf(mfile, "%s/etc/tic.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(mfile, "r+")) == 0) {
		working(2, 0, 0);
		return -1;
	}

	fread(&tichdr, sizeof(tichdr), 1, fil);
	offset = tichdr.hdrsize + (((Area -1) * (tichdr.recsize + tichdr.syssize)));
	if (fseek(fil, offset, SEEK_SET)) {
		fclose(fil);
		working(2, 0, 0);
		return -1;
	}

	fwrite(&tic, tichdr.recsize, 1, fil);
	fseek(ttfil, 0, SEEK_SET);
	for (i = 0; i < (tichdr.syssize / sizeof(sysconnect)); i++) {
		fread(&System, sizeof(sysconnect), 1, ttfil);
		fwrite(&System, sizeof(sysconnect), 1, fil);
	}
	fclose(fil);
	fclose(ttfil);
	ttfil = NULL;
	if (work)
		working(0, 0, 0);
	return 0;
}



void ShowTicStatus(sysconnect);
void ShowTicStatus(sysconnect S)
{
	clr_index();
	set_color(CYAN, BLACK);
	mvprintw( 7, 6, "Aka");
	mvprintw( 8, 6, "Send to");
	mvprintw( 9, 6, "Recv from");
	mvprintw(10, 6, "Pause");
	set_color(WHITE, BLACK);
	show_str(  7,16,23, aka2str(S.aka));
	show_bool( 8,16, S.sendto);
	show_bool( 9,16, S.receivefrom);
	show_bool(10,16, S.pause);
}



void TicGlobal(void);
void TicGlobal(void)
{
	gr_list		*mgr = NULL, *tmp;
	char		*p, tfile[128];
	FILE		*fil;
	fidoaddr	a1, a2;
	int		menu = 0, areanr, Areas, akan = 0, Found;
	int		Total, Done;
	long		offset;
	sysconnect	S, Sc;

	/*
	 * Build the groups select array
	 */
	working(1, 0, 0);
	sprintf(tfile, "%s/etc/fgroups.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(tfile, "r")) != NULL) {
		fread(&fgrouphdr, sizeof(fgrouphdr), 1, fil);

		while (fread(&fgroup, fgrouphdr.recsize, 1, fil) == 1)
			fill_grlist(&mgr, fgroup.Name);

		fclose(fil);
		sort_grlist(&mgr);
	}
	working(0, 0, 0);

	/*
	 * Initialize some variables
	 */
	memset(&S, 0, sizeof(sysconnect));
	S.sendto = TRUE;
	S.receivefrom = FALSE;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 6, "10.2 GLOBAL EDIT TIC AREAS");
		set_color(CYAN, BLACK);
		mvprintw( 7, 6, "1.   Delete connection");
		mvprintw( 8, 6, "2.   Add new connection");
		mvprintw( 9, 6, "3.   Replace connection");
		mvprintw(10, 6, "4.   Change connection status");
		mvprintw(11, 6, "5.   Change aka to use");
		mvprintw(12, 6, "6.   Delete TIC area");

		memset(&a1, 0, sizeof(fidoaddr));
		memset(&a2, 0, sizeof(fidoaddr));
	
		menu = select_menu(6);
		switch (menu) {
			case 0: return;
			case 1: a1 = PullUplink((char *)"AKA TO DELETE");
				break;
			case 2: a2 = PullUplink((char *)"AKA TO ADD");
				break;
			case 3: a1 = PullUplink((char *)"AKA TO REPLACE");
				a2 = PullUplink((char *)"NEW AKA");
				break;
			case 4: S.aka = PullUplink((char *)"AKA TO CHANGE STATUS");
				ShowTicStatus(S);
				S.sendto = edit_bool(8,16,S.sendto, (char *)"^Send^ files to this node");
				S.receivefrom = edit_bool(9,16,S.receivefrom, (char *)"^Receive^ files from this node");
				S.pause = edit_bool(10,16,S.pause, (char *)"Is this node ^paused^");
				break;
			case 5: akan = PickAka((char *)"10.2.5", TRUE);
				break;
		}

		E_Group(&mgr, (char *)"SELECT TIC GROUPS TO CHANGE");

		/*
		 * Show settings before proceeding
		 */
		switch (menu) {
			case 1: mvprintw(7, 6, "Delete aka %s", aka2str(a1));
				break;
			case 2: mvprintw(7, 6, "Add aka %s", aka2str(a2));
				break;
			case 3: p = xstrcpy(aka2str(a1));
				mvprintw(7, 6, "Replace aka %s with %s", p, aka2str(a2));
				free(p);
				break;
			case 4: ShowTicStatus(S);
				mvprintw(14, 6, "Change the link status");
			case 5: if (akan != -1)
					mvprintw( 7, 6, "Set %s as new aka to use", aka2str(CFG.aka[akan]));
				break;
			case 6: mvprintw(7, 6, "Delete TIC areas");
				break;
		}

		if (yes_no((char *)"Perform changes")) {
			working(1, 0, 0);
			Areas = CountTicarea();
			Total = Done = 0;

			for (areanr = 1; areanr <= Areas; areanr++) {
				offset = LoadTicRec(areanr, FALSE);
				if (tic.Active && strlen(tic.Group)) {
					for (tmp = mgr; tmp; tmp = tmp->next) {
						if (tmp->tagged && (strcmp(tmp->group, tic.Group) == 0)) {
							Total++;
							switch (menu) {
							case 1: fseek(ttfil, 0, SEEK_SET);
								while (fread(&Sc, sizeof(sysconnect), 1, ttfil) == 1) {
									if ((Sc.aka.zone == a1.zone) &&
									    (Sc.aka.net == a1.net) &&
									    (Sc.aka.node == a1.node) &&
									    (Sc.aka.point == a1.point)) {
										fseek(ttfil, - sizeof(sysconnect), SEEK_CUR);
										memset(&Sc, 0, sizeof(sysconnect));
										fwrite(&Sc, sizeof(sysconnect), 1, ttfil);
										if (SaveTicRec(areanr, FALSE) == 0) {
											Done++;
											Syslog('+', "Deleted %s from %s", aka2str(a1), tic.Name);
										}
										break;
									}
								}
								break;
							case 2: fseek(ttfil, 0, SEEK_SET);
								Found = FALSE;
								while (fread(&Sc, sizeof(sysconnect), 1, ttfil) == 1)
									if ((Sc.aka.zone == a2.zone) &&
									    (Sc.aka.net == a2.net) &&
									    (Sc.aka.node == a2.node) &&
									    (Sc.aka.point == a2.point)) {
										Found = TRUE;
										break;
									}
									if (Found)
										break;
								fseek(ttfil, 0, SEEK_SET);
								while (fread(&Sc, sizeof(sysconnect), 1, ttfil) == 1) {
									if (Sc.aka.zone == 0) {
										fseek(ttfil, - sizeof(sysconnect), SEEK_CUR);
										memset(&Sc, 0, sizeof(sysconnect));
										Sc.aka.zone = a2.zone;
										Sc.aka.net = a2.net;
										Sc.aka.node = a2.node;
										Sc.aka.point = a2.point;
										Sc.sendto = TRUE;
										Sc.receivefrom = FALSE;
										sprintf(Sc.aka.domain, "%s", a2.domain);
										fwrite(&Sc, sizeof(sysconnect), 1, ttfil);
										if (SaveTicRec(areanr, FALSE) == 0) {
											Done++;
											Syslog('+', "Added %s to area %s", aka2str(a2), tic.Name);
										}
										break;
									}
								}
								break;
							case 3: fseek(ttfil, 0, SEEK_SET);
								while (fread(&Sc, sizeof(sysconnect), 1, ttfil) == 1) {
									if ((Sc.aka.zone == a1.zone) &&
									    (Sc.aka.net == a1.net) &&
									    (Sc.aka.node == a1.node) &&
									    (Sc.aka.point == a1.point)) {
										Sc.aka.zone = a2.zone;
										Sc.aka.net = a2.net;
										Sc.aka.node = a2.node;
										Sc.aka.point = a2.point;
										sprintf(Sc.aka.domain, "%s", a2.domain);
										fseek(ttfil, - sizeof(sysconnect), SEEK_CUR);
										fwrite(&Sc, sizeof(sysconnect), 1, ttfil);
										if (SaveTicRec(areanr, FALSE) == 0) {
											Done++;
											p = xstrcpy(aka2str(a1));
											Syslog('+', "Changed %s into %s in area %s", p, aka2str(a2), tic.Name);
											free(p);
										}
										break;
									}
								}
								break;
							case 4: fseek(ttfil, 0, SEEK_SET);
								while (fread(&Sc, sizeof(sysconnect), 1, ttfil) == 1) {
									if ((Sc.aka.zone == S.aka.zone) &&
									    (Sc.aka.net == S.aka.net) &&
									    (Sc.aka.node == S.aka.node) &&
									    (Sc.aka.point == S.aka.point)) {
										Sc.sendto = S.sendto;
										Sc.receivefrom = S.receivefrom;
										Sc.pause = S.pause;
										Sc.cutoff = S.cutoff;
										fseek(ttfil, - sizeof(sysconnect), SEEK_CUR);
										fwrite(&Sc, sizeof(sysconnect), 1, ttfil);
										if (SaveTicRec(areanr, FALSE) == 0) {
											Done++;
											Syslog('+', "Changed status of %s in area %s", aka2str(S.aka), tic.Name);
										}
										break;
									}
								}
								break;
							case 5: if (akan != -1) {
									if ((tic.Aka.zone != CFG.aka[akan].zone) ||
									    (tic.Aka.net != CFG.aka[akan].net) ||
									    (tic.Aka.node != CFG.aka[akan].node) ||
									    (tic.Aka.point != CFG.aka[akan].point)) {
										tic.Aka.zone = CFG.aka[akan].zone;
										tic.Aka.net = CFG.aka[akan].net;
										tic.Aka.node = CFG.aka[akan].node;
										tic.Aka.point = CFG.aka[akan].point;
										sprintf(tic.Aka.domain, "%s", CFG.aka[akan].domain);
										if (SaveTicRec(areanr, FALSE) == 0) {
											Done++;
											Syslog('+', "Area %s now uses aka %s", tic.Name, aka2str(tic.Aka));
										}
									}
								}
								break;
							case 6:	if (tic.Active) {
									tic.Active = FALSE;
									tic.Deleted = TRUE;
									if (SaveTicRec(areanr, FALSE) == 0) {
										Done++;
										Syslog('+', "Deleted TIC area %s", tic.Name);
									}
								}
								break;
							}
						}
					}
				}
				if (ttfil != NULL)
					fclose(ttfil);
			}

			working(0, 0, 0);
			mvprintw(LINES -3, 6,"Made %d changes in %d possible areas", Done, Total);
			(void)readkey(LINES -3, 50, LIGHTGRAY, BLACK);
			if (Done)
				TicUpdated = TRUE;
		}
	}

	tidy_grlist(&mgr);
}




/**
 *   Edit one record, return -1 if record doesn't exist, 0 if ok.
 */
int EditTicRec(int);
int EditTicRec(int Area)
{
	unsigned long	crc1;
	int		tmp, i, changed = FALSE;
	sysconnect	System;

	clr_index();
	IsDoing("Edit Tic Area");

	if (LoadTicRec(Area, TRUE) == -1)
		return -1;

	SetTicScreen();

	for (;;) {
		set_color(WHITE, BLACK);
		show_str( 6,16,55, tic.Comment);
		show_str( 7,16,20, tic.Name);
		show_int( 8,16,    tic.FileArea);
		show_str( 9,16,14, tic.Message);
		show_str(10,16,12, tic.Group);
		show_int(11,16,    tic.KeepLatest);
		show_str(12,16,20, aka2str(tic.Aka));
		show_str(13,16,5,  tic.Convert);
		show_str(14,16,14, tic.Banner);
		show_bool(15,16,   tic.Replace);

		show_bool( 7,51,   tic.DupCheck);
		show_bool( 8,51,   tic.Secure);
		show_bool( 9,51,   tic.NoTouch);
		show_bool(10,51,   tic.VirScan);
		show_bool(11,51,   tic.Announce);
		show_bool(12,51,   tic.UpdMagic);
		show_bool(13,51,   tic.FileId);
		show_bool(14,51,   tic.ConvertAll);
		show_bool(15,51,   tic.SendOrg);

		show_bool( 7,73,   tic.Mandat);
		show_bool( 8,73,   tic.Notified);
		show_bool( 9,73,   tic.UplDiscon);
		show_bool(10,73,   tic.Deleted);
		show_bool(11,73,   tic.Active);

		switch(select_menu(25)) {
		case 0:
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&tic, crc1, tichdr.recsize);
			fseek(ttfil, 0, 0);
			for (i = 0; i <= (tichdr.syssize / sizeof(sysconnect)); i++) {
				fread(&System, sizeof(sysconnect), 1, ttfil);
				crc1 = upd_crc32((char *)&System, crc1, sizeof(sysconnect));
			}
			if ((TicCrc != crc1) || (changed)) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					if (SaveTicRec(Area, TRUE) == -1)
						return -1;
					TicUpdated = 1;
					Syslog('+', "Saved tic record %d", Area);
				}
			}
			IsDoing("Browsing Menu");
			return 0;

		case 1: E_STR( 6,16,55, tic.Comment, "The ^description^ for this area.");
		case 2:	E_STR( 7,16,20, tic.Name,    "The ^name^ of this ^TIC^ area.");
		case 3: tmp = PickFilearea((char *)"10.2.3");
			if (tmp != 0)
				tic.FileArea = tmp;
			SetTicScreen();
			break;
		case 4:	E_STR( 9,16,14, tic.Message, "The ^message^ to include with the .tic files.");
		case 5:	strcpy(tic.Group, PickFGroup((char *)"10.2.5"));
			if (strlen(tic.Group)) {
				tic.Aka = fgroup.UseAka;
				/*
				 * If there is an uplink defined in the group,
				 * and the first connected system is empty,
				 * copy the uplink as default connection.
				 */
				if (fgroup.UpLink.zone) {
					fseek(ttfil, 0, SEEK_SET);
					fread(&System, sizeof(sysconnect), 1, ttfil);
					if (!System.aka.zone) {
						memset(&System, 0, sizeof(sysconnect));
						System.aka = fgroup.UpLink;
						System.receivefrom = TRUE;
						fseek(ttfil, 0, SEEK_SET);
						fwrite(&System, sizeof(sysconnect), 1, ttfil);
					}
				}
			}
			SetTicScreen();
			break;
		case 6:	E_INT(11,16,    tic.KeepLatest, "^Keep^ the ^latest^ number of files.");
		case 7:	tmp = PickAka((char *)"10.2.7", TRUE);
			if (tmp != -1)
				tic.Aka = CFG.aka[tmp];
			SetTicScreen();
			break;
		case 8:	strcpy(tic.Convert, PickArchive((char *)"10.2.8"));
			SetTicScreen();
			break;
		case 9:	E_STR(14,16,14, tic.Banner,   "The ^banner^ to put in the file archives");
		case 10:E_BOOL(15,16, tic.Replace,    "Allow ^Replace^ files command");
		case 11:E_BOOL( 7,51, tic.DupCheck,   "Check for ^duplicates^ in received files");
		case 12:E_BOOL( 8,51, tic.Secure,     "Check for ^secure^ systems");
		case 13:E_BOOL( 9,51, tic.NoTouch,    "Don't ^touch^ filedate");
		case 14:E_BOOL(10,51, tic.VirScan,    "Check received files for ^virusses^");
		case 15:E_BOOL(11,51, tic.Announce,   "^Announce^ received files");
		case 16:E_BOOL(12,51, tic.UpdMagic,   "Update files ^magic^ names");
		case 17:E_BOOL(13,51, tic.FileId,     "Extract ^FILE_ID.DIZ^ from received files");
		case 18:E_BOOL(14,51, tic.ConvertAll, "^Convert^ archive always");
		case 19:E_BOOL(15,51, tic.SendOrg,    "^Send original^ file to downlinks");
		case 20:E_BOOL( 7,73, tic.Mandat,     "Is this area ^mandatory^");
		case 21:E_BOOL( 8,73, tic.Notified,   "Is the sysop ^notified^ if this area is (dis)connected");
		case 22:E_BOOL( 9,73, tic.UplDiscon,  "Is the uplink ^disconnected^ from this area");
		case 23:E_BOOL(10,73, tic.Deleted,    "Is this area ^deleted^");
		case 24:E_BOOL(11,73, tic.Active,     "Is this area ^active^");
		case 25:
			if (EditTicConnections(ttfil))
				changed = TRUE;
			SetTicScreen();
			break;

		}
	}
}



void EditTicarea(void)
{
	int	records, i, o, y;
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

	records = CountTicarea();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenTicarea() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 3, "10.2 TIC AREA SETUP");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/tic.temp", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&tichdr, sizeof(tichdr), 1, fil);
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 10; i++) {
					if ((o + i) <= records) {
						offset = sizeof(tichdr) + (((o + i) - 1) * (tichdr.recsize + tichdr.syssize));
						fseek(fil, offset, 0);
						fread(&tic, tichdr.recsize, 1, fil);
						if (tic.Active) {
							set_color(CYAN, BLACK);
							sprintf(temp, "%3d.  %-20s %-40s", o + i, tic.Name, tic.
Comment);
						} else {
							set_color(LIGHTBLUE, BLACK);
							sprintf(temp, "%3d.", o + i);
						}
						mvprintw(y, 2, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
		strcpy(pick, select_area(records, 10));
		
		if (strncmp(pick, "-", 1) == 0) {
			CloseTicarea(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendTicarea() == 0) {
				records++;
				working(1, 0, 0);
			} else
				working(2, 0, 0);
			working(0, 0, 0);
		}

		if (strncmp(pick, "G", 1) == 0) {
			TicGlobal();
		}

		if (strncmp(pick, "N", 1) == 0) 
			if ((o + 10) < records)
				o = o + 10;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 10) >= 0)
				o = o - 10;

		if (strncmp(pick, "M", 1) == 0)
			errmsg((char *)"This has no use here");

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			EditTicRec(atoi(pick));
			o = ((atoi(pick) - 1) / 10) * 10;
		}
	}
}



char *PickTicarea(char *shdr)
{
	int		records, i, o = 0, x, y;
	char		pick[12];
	FILE		*fil;
	char		temp[81];
	long		offset;
	static char	Buf[81];

	clr_index();
	working(1, 0, 0);
	if (config_read() == -1) {
		working(2, 0, 0);
		return '\0';
	}

	records = CountTicarea();
	if (records == -1) {
		working(2, 0, 0);
		return '\0';
	}

	working(0, 0, 0);

	for(;;) {
		clr_index();
		set_color(WHITE, BLACK);
		sprintf(temp, "%s.  TIC AREA SELECT", shdr);
		mvprintw(5, 3, temp);
		set_color(CYAN, BLACK);

		if (records) {
			sprintf(temp, "%s/etc/tic.data", getenv("MBSE_ROOT"));
			working(1, 0, 0);
			if ((fil = fopen(temp, "r")) != NULL) {
				fread(&tichdr, sizeof(tichdr), 1, fil);
				x = 2;
				y = 7;
				set_color(CYAN, BLACK);
				for (i = 1; i <= 10; i++) {
					if ((o + i) <= records) {
						offset = tichdr.hdrsize + (((o + i) - 1) * (tichdr.recsize + tichdr.syssize));
						fseek(fil, offset, SEEK_SET);
						fread(&tic, tichdr.recsize, 1, fil);
						if (tic.Active)	
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-20s %-40s", o + i, tic.Name, tic.Comment);
						mvprintw(y, x, temp);
						y++;
					}
				}
				fclose(fil);
			}
		}
		working(0, 0, 0);
		strcpy(pick, select_pick(records, 10));

		if (strncmp(pick, "-", 1) == 0)
			return '\0';

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 10) < records) 
				o += 10;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 10) >= 0)
				o -= 10;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			sprintf(temp, "%s/etc/tic.data", getenv("MBSE_ROOT"));
			if ((fil = fopen(temp, "r")) != NULL) {
				offset = tichdr.hdrsize + ((atoi(pick) -1) * (tichdr.recsize + tichdr.syssize));
				fseek(fil, offset, SEEK_SET);
				fread(&tic, tichdr.recsize, 1, fil);
				fclose(fil);
				if (tic.Active) {
					memset(&Buf, 0, sizeof(Buf));
					sprintf(Buf, "%s", tic.Name);
					return Buf;
				}
			}
		}
	}
}



int GroupInTic(char *Group)
{
        char            temp[81];
        FILE            *no;
        int             systems, Area = 0, RetVal = 0;

        sprintf(temp, "%s/etc/tic.data", getenv("MBSE_ROOT"));
        if ((no = fopen(temp, "r")) == NULL)
                return 0;

        fread(&tichdr, sizeof(tichdr), 1, no);
        fseek(no, 0, SEEK_SET);
        fread(&tichdr, tichdr.hdrsize, 1, no);
        systems = tichdr.syssize / sizeof(sysconnect);

        while ((fread(&tic, tichdr.recsize, 1, no)) == 1) {
		Area++;

		if (!strcmp(tic.Group, Group) && strlen(tic.Group)) {
			RetVal++;
			Syslog('-', "Group %s in tic area %d: %s", Group, Area, tic.Name);
		}

		fseek(no, tichdr.syssize, SEEK_CUR);
        }

        fclose(no);
        return RetVal;
}



int NodeInTic(fidoaddr A)
{
	int		i, Area = 0, RetVal = 0, systems;
	FILE		*no;
	char		temp[128];
	sysconnect	S;

	sprintf(temp, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return 0;

	fread(&tichdr, sizeof(tichdr), 1, no);
	fseek(no, 0, SEEK_SET);
	fread(&tichdr, tichdr.hdrsize, 1, no);
	systems = tichdr.syssize / sizeof(sysconnect);

	while ((fread(&tic, tichdr.recsize, 1, no)) == 1) {
		Area++;
		for (i = 0; i < systems; i++) {
			fread(&S, sizeof(sysconnect), 1, no);
			if (S.aka.zone && (S.aka.zone == A.zone) && (S.aka.net == A.net) &&
			    (S.aka.node == A.node) && (S.aka.point == A.point) && tic.Active) {
				RetVal++;
				Syslog('-', "Node %s connected to tic area %d: %s", aka2str(A), Area, tic.Name);
			}
		}
	}

	fclose(no);
	return RetVal;
}



int tic_areas_doc(FILE *fp, FILE *toc, int page)
{
	char		temp[81], status[4];
	FILE		*no;
	int		i, systems, First = TRUE;
	sysconnect	System;

	sprintf(temp, "%s/etc/tic.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL)
		return page;

	fread(&tichdr, sizeof(tichdr), 1, no);
	fseek(no, 0, SEEK_SET);
	fread(&tichdr, tichdr.hdrsize, 1, no);
	systems = tichdr.syssize / sizeof(sysconnect);

	while ((fread(&tic, tichdr.recsize, 1, no)) == 1) {

		page = newpage(fp, page);

		if (First) {
			addtoc(fp, toc, 10, 2, page, (char *)"File processing areas");
			First = FALSE;
			fprintf(fp, "\n");
		} else
			fprintf(fp, "\n\n");

		fprintf(fp, "    Area tag    %s\n", tic.Name);
		fprintf(fp, "    Active      %s\n", getboolean(tic.Active));
		fprintf(fp, "    Comment     %s\n", tic.Comment);
		fprintf(fp, "    BBS area    %ld\n", tic.FileArea);
		fprintf(fp, "    Message     %s\n", tic.Message);
		fprintf(fp, "    Group       %s\n", tic.Group);
		fprintf(fp, "    Keep Numbe  %d\n", tic.KeepLatest);
		fprintf(fp, "    Fido Aka    %s\n", aka2str(tic.Aka));
		fprintf(fp, "    Convert to  %s\n", tic.Convert);
		fprintf(fp, "    Convert all %s\n", getboolean(tic.ConvertAll));
		fprintf(fp, "    Banner file %s\n", tic.Banner);
		fprintf(fp, "    Replace ok. %s\n", getboolean(tic.Replace));
		fprintf(fp, "    Dupe check  %s\n", getboolean(tic.DupCheck));
		fprintf(fp, "    Secure      %s\n", getboolean(tic.Secure));
		fprintf(fp, "    No touch    %s\n", getboolean(tic.NoTouch));
		fprintf(fp, "    Virus scan  %s\n", getboolean(tic.VirScan));
		fprintf(fp, "    Announce    %s\n", getboolean(tic.Announce));
		fprintf(fp, "    Upd. magic  %s\n", getboolean(tic.UpdMagic));
		fprintf(fp, "    FILE_ID.DIZ %s\n", getboolean(tic.FileId));
		fprintf(fp, "    Mandatory   %s\n", getboolean(tic.Mandat));
		fprintf(fp, "    Upl. discon %s\n", getboolean(tic.UplDiscon));
		fprintf(fp, "    Notified    %s\n\n", getboolean(tic.Notified));

		for (i = 0; i < systems; i++) {
			fread(&System, sizeof(sysconnect), 1, no);
			if (System.aka.zone) {
				memset(&status, 0, 4);
				memset(&status, '-', 3);
				if (System.sendto)
					status[0] = 'S';
				if (System.receivefrom)
					status[1] = 'R';
				if (System.pause)
					status[2] = 'P';
				fprintf(fp, "    Link %2d     %s %s\n", i+1, status, aka2str(System.aka));
			}
		}
	}

	fclose(no);
	return page;
}



