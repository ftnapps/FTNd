/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Edit BBS menus
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
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
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
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
#include "mutil.h"
#include "screen.h"
#include "ledit.h"
#include "m_lang.h"
#include "m_menu.h"



char *select_menurec(int max)
{
	static	char *menu=(char *)"-";
	char	help[81];
	int	pick;

	if (max > 10)
		sprintf(help, "Rec. (1..%d), ^\"-\"^ Back, ^A^ppend, ^D^elete, ^M^ove, ^P^revious, ^N^ext", max);
	else if (max > 1)
		sprintf(help, "Rec. (1..%d), ^\"-\"^ Back, ^A^ppend, ^D^elete, ^M^ove", max);
	else if (max == 1)
		sprintf(help, "Rec. (1..%d), ^\"-\"^ Back, ^A^ppend, ^D^elete", max);
	else
		sprintf(help, "Select ^\"-\"^ for previous level, ^A^ppend a record");

	showhelp(help);

	for (;;) {
		mvprintw(LINES - 3, 6, "Enter your choice >");
		menu = (char *)"-";
		menu = edit_field(LINES - 3, 26, 6, '!', menu);
		locate(LINES -3, 6);
		clrtoeol();

		if (strncmp(menu, "A", 1) == 0)
			break;
		if (strncmp(menu, "-", 1) == 0)
			break;
		if (strncmp(menu, "D", 1) == 0)
			break;
		if ((max > 1) && (strncmp(menu, "M", 1) == 0))
			break;

		if (max > 10) {
			if (strncmp(menu, "N", 1) == 0)
				break;
			if (strncmp(menu, "P", 1) == 0)
				break;
		}
		pick = atoi(menu);
		if ((pick >= 1) && (pick <= max))
			break;

		working(2, 0, 0);
		working(0, 0, 0);
	}
	return menu;
}



void Show_A_Menu(void);
void Show_A_Menu(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 5, 2, "8.3. EDIT MENU ITEM");
	set_color(CYAN, BLACK);
	mvprintw( 7, 2, "1.  Sel. key");
	mvprintw( 8, 2, "2.  Type nr.");
	mvprintw( 9, 2, "3.  Opt. data");
	mvprintw(11, 2, "4.  Display");
	mvprintw(12, 2, "5.  Security");
	mvprintw(13, 2, "6.  Min. age");
	mvprintw(14, 2, "7.  Max. lvl");
	mvprintw(15, 2, "8.  Password");
	mvprintw(16, 2, "9.  Credit");
	mvprintw(17, 2, "10. Lo-colors");
	mvprintw(18, 2, "11. Hi-colors");
	mvprintw(19, 2, "12. Autoexec");
	if (menus.MenuType == 7) {
		mvprintw(15,42, "13. No door.sys");
		mvprintw(16,42, "14. Y2K style");
		mvprintw(17,42, "15. Use Comport");
		mvprintw(18,42, "16. Run nosuid");
		mvprintw(19,42, "17. No Prompt");
	}

	set_color(WHITE, BLACK);
	show_str( 7,16, 1, menus.MenuKey);
	show_int( 8,16,    menus.MenuType); show_str( 8, 26,29, menus.TypeDesc);
	show_str( 9,16,64, menus.OptionalData);
	show_str(10,16,64,(char *)"1234567890123456789012345678901234567890123456789012345678901234");
	show_str(11,16,64, menus.Display);
	show_sec(12,16,    menus.MenuSecurity);
	show_int(13,16,    menus.Age);
	show_int(14,16,    menus.MaxSecurity);
	if (strlen(menus.Password))
		show_str(15,16,14, (char *)"**************");
	else
		show_str(15,16,14, (char *)"<null>");
	show_int(16,16,    menus.Credit);
	S_COL(17,16, "Normal display color", menus.ForeGnd, menus.BackGnd)
	S_COL(18,16, "Bright display color", menus.HiForeGnd, menus.HiBackGnd)
	set_color(WHITE, BLACK);
	show_bool(19,16,   menus.AutoExec);
	if (menus.MenuType == 7) {
		show_bool(15,58,  menus.NoDoorsys);
		show_bool(16,58,  menus.Y2Kdoorsys);
		show_bool(17,58,  menus.Comport);
		show_bool(18,58,  menus.NoSuid);
		show_bool(19,58,  menus.NoPrompt);
	}
}



int GetSubmenu(int, int);
int GetSubmenu(int Base, int Max)
{
	int	i, x, y;
	char	temp[81];

	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 4, 2, "8.3 EDIT MENU - SELECT MENUTYPE");
	set_color(CYAN, BLACK);
	y = 6;
	x = 2;

	for (i = 1; i <= Max; i++) {
		sprintf(temp, "%2d. %s", i, getmenutype(i - 1 + Base));
		mvprintw(y, x, temp);
		y++;
		if ((i % 13) == 0) {
			y = 6;
			x = 42;
		}
	}
	i = select_menu(Max);

	if (i)
		return (i + Base - 1);
	else
		return 0;
}



int GetMenuType(void);
int GetMenuType(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mvprintw( 5, 6, "8.3 EDIT MENU - SELECT MENUTYPE");
    set_color(CYAN, BLACK);
    mvprintw( 7, 6, "1.  Global system menus");
    mvprintw( 8, 6, "2.  File areas menus");
    mvprintw( 9, 6, "3.  Message areas menus");
    mvprintw(10, 6, "4.  User setting menus");
    mvprintw(11, 6, "5.  Oneliner menus");
    mvprintw(12, 6, "6.  BBS List menus");

    switch (select_menu(6)) {
	case 1:	    return GetSubmenu(1, 25);
	case 2:	    return GetSubmenu(101, 19);
	case 3:	    return GetSubmenu(201, 20);
	case 4:	    return GetSubmenu(301, 19);
	case 5:	    return GetSubmenu(401, 5);
	case 6:	    return GetSubmenu(501, 6);
	default:    return 0; 
    }
}



void Edit_A_Menu(void);
void Edit_A_Menu(void)
{
	int fg, bg;

	Show_A_Menu();

	for (;;) {
		switch(select_menu(17)) {
		case 0: return;
			break;
		case 1:	E_UPS( 7,16, 1, menus.MenuKey,   "The ^key^ to select this menu item")
		case 2: menus.MenuType = GetMenuType();
			memset(&menus.TypeDesc, 0, sizeof(menus.TypeDesc));
			if (menus.MenuType)
				strcpy(menus.TypeDesc, getmenutype(menus.MenuType));
			if (menus.MenuType == 21)
				menus.AutoExec = TRUE;
			Show_A_Menu();
			break;
		case 3: E_STR( 9,16,64, menus.OptionalData, "The ^optional data^ for this menu item")
		case 4: E_STR(11,16,64, menus.Display,      "The text to ^display^ for this menu")
		case 5: E_SEC(12,16,    menus.MenuSecurity, "8.3.5 MENU ACCESS SECURITY", Show_A_Menu)
		case 6: E_INT(13,16,    menus.Age,          "The minimum ^Age^ to select this menu, 0 is don't care")
		case 7: E_INT(14,16,    menus.MaxSecurity,  "The maximum ^Security level^ to access this menu")
		case 8: E_STR(15,16,14, menus.Password,     "The ^password^ to access this menu item")
		case 9: E_INT(16,16,    menus.Credit,       "The ^credit cost^ for this menu item")
		case 10:fg = menus.ForeGnd;
			bg = menus.BackGnd;
			edit_color(&fg, &bg, (char *)"8.3.10 EDIT COLOR", (char *)"normal");
			menus.ForeGnd = fg;
			menus.BackGnd = bg;
			Show_A_Menu();
			break;
		case 11:fg = menus.HiForeGnd;
			bg = menus.HiBackGnd;
			edit_color(&fg, &bg, (char *)"8.3.11 EDIT COLOR", (char *)"bright");
			menus.HiForeGnd = fg;
			menus.HiBackGnd = bg;
			Show_A_Menu();
			break;
		case 12:E_BOOL(19,16,   menus.AutoExec,     "Is this an ^Autoexecute^ menu item")
		case 13:if (menus.MenuType == 7) {
				E_BOOL(15,58,   menus.NoDoorsys,    "Suppress writing ^door.sys^ dropfile")
			} else
				break;
		case 14:if (menus.MenuType == 7) {
				E_BOOL(16,58,   menus.Y2Kdoorsys,   "Create ^door.sys^ with 4 digit yearnumbers")
			} else
				break;
		case 15:if (menus.MenuType == 7) {
				E_BOOL(17,58,   menus.Comport,      "Write real ^COM port^ in door.sys for Vmodem patch")
			} else
				break;
		case 16:if (menus.MenuType == 7) {
				E_BOOL(18,58,   menus.NoSuid,       "Run the door as ^real user (nosuid)^")
			} else
				break;
		case 17:if (menus.MenuType == 7) {
				E_BOOL(19,58,   menus.NoPrompt,     "^Don't display prompt^ when door is finished")
			} else
				break;
		}
	}
}



void EditMenu(char *);
void EditMenu(char *Name)
{
	char		mtemp[PATH_MAX], temp[PATH_MAX];
	FILE		*fil, *tmp;
	int		records = 0, i, o, y;
	char		pick[12];
	long		offset;
	unsigned long	crc, crc1;
	int		MenuUpdated = FALSE, from, too;
	struct menufile	tmenus;

	clr_index();
	IsDoing("Edit Menu");
	working(1, 0, 0);

	sprintf(mtemp, "%s/%s.tmp", lang.MenuPath, Name);
	tmp = fopen(mtemp, "w+");

	sprintf(temp, "%s/%s.mnu", lang.MenuPath, Name);
	if ((fil = fopen(temp, "r")) != NULL) {
		while (fread(&menus, sizeof(menus), 1, fil) == 1) {
			fwrite(&menus, sizeof(menus), 1, tmp);
			records++;
		}
		fclose(fil);
	}

	o = 0;
	for (;;) {
		clr_index();
		working(1, 0, 0);
		sprintf(temp, "8.3 EDIT MENU \"%s\" (%s)", Name, lang.Name);
		mvprintw( 5, 6, tu(temp));
		set_color(CYAN, BLACK);
		fseek(tmp, 0, SEEK_SET);

		if (records) {
			y = 7;
			for (i = 1; i <= 10; i++) {
				if ((o + i) <= records) {
					offset = ((o + i) - 1) * sizeof(menus);
					fseek(tmp, offset, SEEK_SET);
					fread(&menus, sizeof(menus), 1, tmp);
					if (menus.MenuKey[0] || menus.AutoExec) {
						set_color(CYAN, BLACK);
						mvprintw(y, 5, "%3d. ", o + i);
						if (menus.AutoExec) {
							set_color(LIGHTRED, BLACK);
							mvprintw(y, 10,  "a");
							set_color(CYAN, BLACK);
						} else
							mvprintw(y, 10, "%1s", menus.MenuKey);
						if (menus.MenuType == 999 ){
							mvprintw(y, 12, "%-29s %5d %s", menus.TypeDesc, 
								menus.MenuSecurity.level, menus.Display);
						} else
							mvprintw(y, 12, "%-29s %5d %s", menus.TypeDesc, 
								menus.MenuSecurity.level, menus.OptionalData);
					} else {
						set_color(LIGHTBLUE, BLACK);
						mvprintw(y, 5, "%3d.", o + i);
					}
					y++;
				}
			}
		}

		working(0, 0, 0);
		strcpy(pick, select_menurec(records));

		if (strncmp(pick, "-", 1) == 0) {
			if (MenuUpdated) {
				if (yes_no((char *)"Menu is changed, save changes") == 1) {
					working(1, 0, 0);
					sprintf(temp, "%s/%s.mnu", lang.MenuPath, Name);
					if ((fil = fopen(temp, "w+")) == NULL) {
						working(2, 0, 0);
					} else {
						Syslog('+', "Updated menu %s (%s)", temp, lang.Name);
						fseek(tmp, 0, SEEK_SET);
						while (fread(&menus, sizeof(menus), 1, tmp) == 1) {
							if (menus.MenuKey[0] || menus.AutoExec)
								fwrite(&menus, sizeof(menus), 1, fil);
						}
						fclose(fil);
						chmod(temp, 0640);
					}
					working(0, 0, 0);
				}
			}
			fclose(tmp);
			unlink(mtemp);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			memset(&menus, 0, sizeof(menus));
			menus.ForeGnd = LIGHTGRAY;
			menus.HiForeGnd = WHITE;
			fseek(tmp, 0, SEEK_END);
			fwrite(&menus, sizeof(menus), 1, tmp);
			records++;
			working(0, 0, 0);
		}

		if (strncmp(pick, "D", 1) == 0) {
			mvprintw(LINES -3, 6, "Enter menu number (1..%d) to delete >", records);
			y = 0;
			y = edit_int(LINES -3, 44, y, (char *)"Enter record number");
			if ((y > 0) && (y <= records) && yes_no((char *)"Remove record")) {
				offset = (y - 1) * sizeof(menus);
				fseek(tmp, offset, SEEK_SET);
				fread(&menus, sizeof(menus), 1, tmp);
				menus.MenuKey[0] = '\0';
				menus.AutoExec = FALSE;
				fseek(tmp, offset, SEEK_SET);
				fwrite(&menus, sizeof(menus), 1, tmp);
				MenuUpdated = TRUE;
			}
		}

		if (strncmp(pick, "M", 1) == 0) {
			from = too = 0;
			mvprintw(LINES -3, 6, "Enter menu number (1..%d) to move >", records);
			from = edit_int(LINES -3, 42, from, (char *)"Enter record number");
			locate(LINES -3, 6);
			clrtoeol();
 			mvprintw(LINES -3, 6, "Enter new position (1..%d) >", records);
			too = edit_int(LINES -3, 36, too, (char *)"Enter destination record number, other will move away");
			if ((from == too) || (from == 0) || (too == 0) || (from > records) || (too > records)) {
				errmsg("That makes no sense");
			} else if (yes_no((char *)"Proceed move")) {
				fseek(tmp, (from -1) * sizeof(menus), SEEK_SET);
				fread(&tmenus, sizeof(menus), 1, tmp);
				if (from > too) {
					for (i = from; i > too; i--) {
						fseek(tmp, (i -2) * sizeof(menus), SEEK_SET);
						fread(&menus, sizeof(menus), 1, tmp);
						fseek(tmp, (i -1) * sizeof(menus), SEEK_SET);
						fwrite(&menus, sizeof(menus), 1, tmp);
					}
				} else {
					for (i = from; i < too; i++) {
						fseek(tmp, i * sizeof(menus), SEEK_SET);
						fread(&menus, sizeof(menus), 1, tmp);
						fseek(tmp, (i -1) * sizeof(menus), SEEK_SET);
						fwrite(&menus, sizeof(menus), 1, tmp);
					}
				}
				fseek(tmp, (too -1) * sizeof(menus), SEEK_SET);
				fwrite(&tmenus, sizeof(menus), 1, tmp);
				MenuUpdated = TRUE;
			}
		}

		if (strncmp(pick, "N", 1) == 0)
			if ((o + 10) < records)
				o += 10;

		if (strncmp(pick, "P", 1) == 0)
			if ((o - 10) >= 0)
				o -= 10;

		if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
			offset = (atoi(pick) - 1) * sizeof(menus);
			fseek(tmp, offset, SEEK_SET);
			fread(&menus, sizeof(menus), 1, tmp);
			crc = 0xffffffff;
			crc = upd_crc32((char *)&menus, crc, sizeof(menus));
			Edit_A_Menu();
			crc1 = 0xffffffff;
			crc1 = upd_crc32((char *)&menus, crc1, sizeof(menus));
			if (crc1 != crc) {
				if (yes_no((char *)"Record is changed, save") == 1) {
					working(1, 0, 0);
					fseek(tmp, offset, SEEK_SET);
					fwrite(&menus, sizeof(menus), 1, tmp);
					MenuUpdated = TRUE;
					working(0, 0, 0);
				}
			}
		}
	}
}



void EditMenus(void)
{
	int	Lang, mcount, err, i, x, y;
	DIR	*dp;
	FILE	*fil;
	struct	dirent *de;
	char	menuname[50][11];
	char	temp[81], pick[12], *p;

	Syslog('+', "Start menu edit");
	memset(&menuname, 0, sizeof(menuname));
	Lang = PickLanguage((char *)"8.3");
	if (Lang == '\0')
		return;

	for (;;) {
		clr_index();
		mcount = 0;
		if ((dp = opendir(lang.MenuPath)) != NULL) {
			working(1, 0, 0);
	
			while ((de = readdir(dp))) {
				if (de->d_name[0] != '.') {
					strcpy(menuname[mcount], strtok(de->d_name, "."));
					mcount++;
				}
			}
			closedir(dp);
			working(0, 0, 0);
		}

		set_color(WHITE, BLACK);
		mvprintw( 5, 6, "8.3 MENU EDIT: %s", lang.Name);
		set_color(CYAN, BLACK);
		
		if (mcount) {
			x = 6;
			y = 7;
			set_color(CYAN, BLACK);
			for (i = 1; i <= mcount; i++) {
				sprintf(temp, "%2d. %s", i, menuname[i-1]);
				mvprintw(y, x, temp);
				y++;
				if ((i % 10) == 0) {
					x+=15;
					y = 7;
				}
			}
		}
		strcpy(pick, select_record(mcount, 50));

		if (strncmp(pick, "-", 1) == 0) {
			Syslog('+', "Finished menu edit");
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			set_color(WHITE, BLACK);
			mvprintw(LINES -3, 6, (char *)"New menu name >");
			memset(&temp, 0, sizeof(temp));
			strcpy(temp, edit_str(LINES -3, 22, 10, temp, (char *)"Enter a new ^menu^ name without extension"));
			if (strlen(temp)) {
				p = xstrcpy(lang.MenuPath);
				p = xstrcat(p, (char *)"/");
				p = xstrcat(p, temp);
				p = xstrcat(p, (char *)".mnu");
				if ((err = file_exist(p, F_OK))) {
					if ((fil = fopen(p, "a")) == NULL) {
						errmsg("Can't create menu %s", temp);
					} else {
						fclose(fil);
						chmod(p, 0640);
						Syslog('+', "Created menufile %s", p);
					}
				} else {
					errmsg("Menu %s already exists", temp);
				}
				free(p);
			}
		}

		if ((atoi(pick) >= 1) && (atoi(pick) <= mcount))
			EditMenu(menuname[atoi(pick) -1]);
	}
}



int bbs_menu_doc(FILE *fp, FILE *toc, int page)
{
	char	*temp;
	FILE	*no, *mn;
	DIR	*dp;
	struct	dirent *de;
	int	j;

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/language.data", getenv("MBSE_ROOT"));
	if ((no = fopen(temp, "r")) == NULL) {
		free(temp);
		return page;
	}

	page = newpage(fp, page);
	addtoc(fp, toc, 8, 3, page, (char *)"BBS Menus");

	fread(&langhdr, sizeof(langhdr), 1, no);
	j =0;

	while ((fread(&lang, langhdr.recsize, 1, no)) == 1) {
		if ((dp = opendir(lang.MenuPath)) != NULL) {
			while ((de = readdir(dp))) {
				if (de->d_name[0] != '.') {
					j = 0;
					sprintf(temp, "%s/%s", lang.MenuPath, de->d_name);
					fprintf(fp, "\n    MENU %s (%s)\n\n", de->d_name, lang.Name);
					if ((mn = fopen(temp, "r")) != NULL) {
						while (fread(&menus, sizeof(menus), 1, mn) == 1) {
							if (menus.MenuKey[0])
								fprintf(fp, "    Menu select   %s\n", menus.MenuKey);
							if (menus.AutoExec)
								fprintf(fp, "    Menu select   Autoexec\n");
							fprintf(fp, "    Type          %d %s\n", menus.MenuType, menus.TypeDesc);
							fprintf(fp, "    Opt. data     %s\n", menus.OptionalData);
							fprintf(fp, "    Display       %s\n", menus.Display);
							fprintf(fp, "    Security      %s\n", get_secstr(menus.MenuSecurity));
							fprintf(fp, "    Minimum age   %d\n", menus.Age);
							fprintf(fp, "    Maximum level %d\n", menus.MaxSecurity);
							fprintf(fp, "    Password      %s\n", menus.Password);
							fprintf(fp, "    Credits       %ld\n", menus.Credit);
							fprintf(fp, "    Lo-colors     %s on %s\n", 
								get_color(menus.ForeGnd), get_color(menus.BackGnd));
							fprintf(fp, "    Hi-colors     %s on %s\n", 
								get_color(menus.HiForeGnd), get_color(menus.HiBackGnd));
							if (menus.MenuType == 7) {
								fprintf(fp, "    No door.sys   %s\n", getboolean(menus.NoDoorsys));
								fprintf(fp, "    Y2K door.sys  %s\n", getboolean(menus.Y2Kdoorsys));
								fprintf(fp, "    Use COM port  %s\n", getboolean(menus.Comport));
								fprintf(fp, "    No setuid     %s\n", getboolean(menus.NoSuid));
								fprintf(fp, "    No Prompt     %s\n", getboolean(menus.NoPrompt));
							}
							fprintf(fp, "\n\n");
							j++;
							if (j == 4) {
								j = 0;
								page = newpage(fp, page);
								fprintf(fp, "\n");
							}
						}
						fclose(mn);
					}
					if (j)
						page = newpage(fp, page);
				}
			}
			closedir(dp);
		}
	}

	free(temp);
	fclose(no);
	return page;
}



