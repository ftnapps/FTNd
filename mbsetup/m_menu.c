/*****************************************************************************
 *
 * $Id: m_menu.c,v 1.38 2007/02/17 12:14:27 mbse Exp $
 * Purpose ...............: Edit BBS menus
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "mutil.h"
#include "screen.h"
#include "ledit.h"
#include "m_lang.h"
#include "m_global.h"
#include "m_menu.h"




char *select_menurec(int max)
{
    static char	*menu=(char *)"-";
    char	help[81];
    int		pick;

    if (max > 10)
	snprintf(help, 81, "Rec. (1..%d), ^\"-\"^ Back, ^A^ppend, ^D^elete, ^M^ove, ^P^revious, ^N^ext", max);
    else if (max > 1)
	snprintf(help, 81, "Rec. (1..%d), ^\"-\"^ Back, ^A^ppend, ^D^elete, ^M^ove", max);
    else if (max == 1)
	snprintf(help, 81, "Rec. (1..%d), ^\"-\"^ Back, ^A^ppend, ^D^elete", max);
    else
	snprintf(help, 81, "Select ^\"-\"^ for previous level, ^A^ppend a record");

    showhelp(help);

    for (;;) {
	mbse_mvprintw(LINES - 3, 6, "Enter your choice >");
	menu = (char *)"-";
	menu = edit_field(LINES - 3, 26, 6, '!', menu);
	mbse_locate(LINES -3, 6);
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
    }
    return menu;
}



void Show_A_Menu(void);
void Show_A_Menu(void)
{
    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 5, 2, "8.3. EDIT MENU ITEM");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 2, "1.  Sel. key");
    mbse_mvprintw( 8, 2, "2.  Type nr.");
    mbse_mvprintw( 9, 2, "3.  Opt. data");
    mbse_mvprintw(11, 2, "4.  Display");
    mbse_mvprintw(12, 2, "5.  Security");
    mbse_mvprintw(13, 2, "6.  Min. age");
    mbse_mvprintw(14, 2, "7.  Lo-colors");
    mbse_mvprintw(15, 2, "8.  Hi-colors");
    mbse_mvprintw(16, 2, "9.  Autoexec");
    if (le_int(menus.MenuType) == 7) {
	mbse_mvprintw(17, 2, "10. Door Name");
	mbse_mvprintw(18, 2, "11. Y2K style");
	mbse_mvprintw(13,42, "12. No door.sys");
	mbse_mvprintw(14,42, "13. Use COMport");
	mbse_mvprintw(15,42, "14. Run nosuid");
	mbse_mvprintw(16,42, "15. No Prompt");
	mbse_mvprintw(17,42, "16. Single User");
	mbse_mvprintw(18,42, "17. Hidden door");
    }

    set_color(WHITE, BLACK);
    show_str( 7,16, 1, menus.MenuKey);
    show_int( 8,16,    le_int(menus.MenuType)); show_str( 8, 26,29, menus.TypeDesc);
    show_str( 9,16,64, menus.OptionalData);
    show_str(10,16,64,(char *)"1234567890123456789012345678901234567890123456789012345678901234");
    show_str(11,16,64, menus.Display);
    show_sec(12,16,    menus.MenuSecurity);
    show_int(13,16,    le_int(menus.Age));
    S_COL(14,16, "Normal display color", le_int(menus.ForeGnd), le_int(menus.BackGnd))
    S_COL(15,16, "Bright display color", le_int(menus.HiForeGnd), le_int(menus.HiBackGnd))
    set_color(WHITE, BLACK);
    show_bool(16,16,  menus.AutoExec);
    if (le_int(menus.MenuType) == 7) {
	show_str(17,16,14, menus.DoorName);
	show_bool(18,16,  menus.Y2Kdoorsys);
	show_bool(13,58,  menus.NoDoorsys);
	show_bool(14,58,  menus.Comport);
	show_bool(15,58,  menus.NoSuid);
	show_bool(16,58,  menus.NoPrompt);
	show_bool(17,58,  menus.SingleUser);
	show_bool(18,58,  menus.HideDoor);
    }
}



int GetSubmenu(int, int);
int GetSubmenu(int Base, int Max)
{
    int	    i, x, y;
    char    temp[81];

    clr_index();
    set_color(WHITE, BLACK);
    mbse_mvprintw( 4, 2, "8.3 EDIT MENU - SELECT MENUTYPE");
    set_color(CYAN, BLACK);
    y = 6;
    x = 2;

    for (i = 1; i <= Max; i++) {
	snprintf(temp, 81, "%2d. %s", i, getmenutype(i - 1 + Base));
	mbse_mvprintw(y, x, temp);
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
    mbse_mvprintw( 5, 6, "8.3 EDIT MENU - SELECT MENUTYPE");
    set_color(CYAN, BLACK);
    mbse_mvprintw( 7, 6, "1.  Global system menus");
    mbse_mvprintw( 8, 6, "2.  File areas menus");
    mbse_mvprintw( 9, 6, "3.  Message areas menus");
    mbse_mvprintw(10, 6, "4.  User setting menus");
    mbse_mvprintw(11, 6, "5.  Oneliner menus");

    switch (select_menu(5)) {
	case 1:	    return GetSubmenu(1, 22);
	case 2:	    return GetSubmenu(101, 19);
	case 3:	    return GetSubmenu(201, 21);
	case 4:	    return GetSubmenu(301, 22);
	case 5:	    return GetSubmenu(401, 5);
	default:    return 0; 
    }
}



void Edit_A_Menu(void);
void Edit_A_Menu(void)
{
    int	    temp, fg, bg;

    Show_A_Menu();

    for (;;) {
	switch(select_menu(17)) {
	    case 0: return;
		    break;
	    case 1: E_UPS( 7,16, 1, menus.MenuKey,   "The ^key^ to select this menu item")
	    case 2: temp = GetMenuType();
		    memset(&menus.TypeDesc, 0, sizeof(menus.TypeDesc));
		    if (temp)
			strcpy(menus.TypeDesc, getmenutype(temp));
		    if (temp == 21)
			menus.AutoExec = TRUE;
		    menus.MenuType = le_int(temp);
		    Show_A_Menu();
		    break;
	    case 3: E_STR( 9,16,64, menus.OptionalData, "The ^optional data^ for this menu item")
	    case 4: E_STR(11,16,64, menus.Display,      "The text to ^display^ for this menu")
	    case 5: menus.MenuSecurity.level = le_int(menus.MenuSecurity.level);
		    menus.MenuSecurity = edit_sec(12,16, menus.MenuSecurity, (char *)"8.3.5 MENU ACCESS SECURITY");
		    menus.MenuSecurity.level = le_int(menus.MenuSecurity.level);
		    Show_A_Menu();
		    break;
	    case 6: temp = le_int(menus.Age);
		    temp = edit_int(13,16, temp, (char *)"The minimum ^Age^ to select this menu, 0 is don't care");
		    menus.Age = le_int(temp);
		    break;
	    case 7: fg = le_int(menus.ForeGnd);
		    bg = le_int(menus.BackGnd);
		    edit_color(&fg, &bg, (char *)"8.3.7  EDIT COLOR", (char *)"normal");
		    menus.ForeGnd = le_int(fg);
		    menus.BackGnd = le_int(bg);
		    Show_A_Menu();
		    break;
	    case 8: fg = le_int(menus.HiForeGnd);
		    bg = le_int(menus.HiBackGnd);
		    edit_color(&fg, &bg, (char *)"8.3.8  EDIT COLOR", (char *)"bright");
		    menus.HiForeGnd = le_int(fg);
		    menus.HiBackGnd = le_int(bg);
		    Show_A_Menu();
		    break;
	    case 9: menus.AutoExec = edit_bool(16,16, menus.AutoExec, (char *)"Is this an ^Autoexecute^ menu item");
		    break;
	    case 10:if (le_int(menus.MenuType) == 7) {
			E_STR(17,16,14, menus.DoorName, (char *)"The ^name^ of the door to show to the users")
		    } else {
			working(2, 0, 0);
		    }
		    break;
	    case 11:if (le_int(menus.MenuType) == 7) {
			menus.Y2Kdoorsys = edit_bool(18,16, menus.Y2Kdoorsys, (char *)"Create ^door.sys^ with 4 digit yearnumbers");
		    } else {
			working(2, 0, 0);
		    }
		    break;
	    case 12:if (le_int(menus.MenuType) == 7) {
			menus.NoDoorsys = edit_bool(13,58, menus.NoDoorsys, (char *)"Suppress writing ^door.sys^ dropfile");
		    } else {
			working(2, 0, 0);
		    }
		    break;
	    case 13:if (le_int(menus.MenuType) == 7) {
			menus.Comport = edit_bool(14,58, menus.Comport, (char *)"Write real ^COM port^ in door.sys for Vmodem patch");
		    } else {
			working(2, 0, 0);
		    }
		    break;
	    case 14:if (le_int(menus.MenuType) == 7) {
			menus.NoSuid = edit_bool(15,58, menus.NoSuid, (char *)"Run the door as ^real user (nosuid)^");
		    } else {
			working(2, 0, 0);
		    }
		    break;
	    case 15:if (le_int(menus.MenuType) == 7) {
			menus.NoPrompt = edit_bool(16,58, menus.NoPrompt, (char *)"^Don't display prompt^ when door is finished");
		    } else {
			working(2, 0, 0);
		    }
		    break;
	    case 16:if (le_int(menus.MenuType) == 7) {
			menus.SingleUser = edit_bool(17,58, menus.SingleUser, (char *)"Set if door is for ^single user^ only");
		    } else {
			working(2, 0, 0);
		    }
		    break;
	    case 17:if (le_int(menus.MenuType) == 7) {
			menus.HideDoor = edit_bool(18,58, menus.HideDoor, (char *)"^Hide door^ from user display lists");
		    } else {
			working(2, 0, 0);
		    }
		    break;
	}
    }
}



void EditMenu(char *);
void EditMenu(char *Name)
{
    char	    mtemp[PATH_MAX], temp[PATH_MAX], pick[12];
    FILE	    *fil, *tmp;
    int		    records = 0, i, o, y, MenuUpdated = FALSE, from, too;
    int		    offset;
    unsigned int    crc, crc1;
    struct menufile tmenus;

    clr_index();
    IsDoing("Edit Menu");
    working(1, 0, 0);

    snprintf(mtemp, PATH_MAX, "%s/share/int/menus/%s/%s.tmp", getenv("MBSE_ROOT"), lang.lc, Name);
    tmp = fopen(mtemp, "w+");

    snprintf(temp, PATH_MAX, "%s/share/int/menus/%s/%s.mnu", getenv("MBSE_ROOT"), lang.lc, Name);
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
	snprintf(temp, 81, "8.3 EDIT MENU \"%s\" (%s)", Name, lang.Name);
	mbse_mvprintw( 5, 6, tu(temp));
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
			mbse_mvprintw(y, 5, "%3d. ", o + i);
			if (menus.AutoExec) {
			    set_color(LIGHTRED, BLACK);
			    mbse_mvprintw(y, 10,  "a");
			    set_color(CYAN, BLACK);
			} else {
			    mbse_mvprintw(y, 10, "%1s", menus.MenuKey);
			}
			if (le_int(menus.MenuType) == 999 ) {
			    snprintf(temp, 81, "%-29s %5d %s", menus.TypeDesc, 
				    le_int(menus.MenuSecurity.level), menus.Display);
			} else {
			    snprintf(temp, 81, "%-29s %5d %s", menus.TypeDesc, 
				    le_int(menus.MenuSecurity.level), menus.OptionalData);
			}
			temp[68] = '\0';
			mbse_mvprintw(y, 12, temp);
		    } else {
			set_color(LIGHTBLUE, BLACK);
			mbse_mvprintw(y, 5, "%3d.", o + i);
		    }
		    y++;
		}
	    }
	}

	strcpy(pick, select_menurec(records));

	if (strncmp(pick, "-", 1) == 0) {
	    if (MenuUpdated) {
		if (yes_no((char *)"Menu is changed, save changes") == 1) {
		    working(1, 0, 0);
		    snprintf(temp, PATH_MAX, "%s/share/int/menus/%s/%s.mnu", getenv("MBSE_ROOT"), lang.lc, Name);
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
			working(6, 0, 0);
		    }
		}
	    }
	    fclose(tmp);
	    unlink(mtemp);
	    return;
	}

	if (strncmp(pick, "A", 1) == 0) {
	    working(1, 0, 0);
	    memset(&menus, 0, sizeof(menus));
	    menus.ForeGnd = le_int(LIGHTGRAY);
	    menus.HiForeGnd = le_int(WHITE);
	    fseek(tmp, 0, SEEK_END);
	    fwrite(&menus, sizeof(menus), 1, tmp);
	    records++;
	}

	if (strncmp(pick, "D", 1) == 0) {
	    mbse_mvprintw(LINES -3, 6, "Enter menu number (1..%d) to delete >", records);
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
	    mbse_mvprintw(LINES -3, 6, "Enter menu number (1..%d) to move >", records);
	    from = edit_int(LINES -3, 42, from, (char *)"Enter record number");
	    mbse_locate(LINES -3, 6);
	    clrtoeol();
 	    mbse_mvprintw(LINES -3, 6, "Enter new position (1..%d) >", records);
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
		    working(6, 0, 0);
		}
	    }
	}
    }
}



void EditMenus(void)
{
    int	    Lang, mcount, err, i, x, y;
    DIR	    *dp;
    FILE    *fil;
    struct  dirent *de;
    char    menuname[50][11], temp[PATH_MAX], pick[12], *p;

    Syslog('+', "Start menu edit");
    memset(&menuname, 0, sizeof(menuname));
    Lang = PickLanguage((char *)"8.3");
    if (Lang == '\0')
	return;

    for (;;) {
	clr_index();
	mcount = 0;

	snprintf(temp, PATH_MAX, "%s/share/int/menus/%s", getenv("MBSE_ROOT"), lang.lc);
	if ((dp = opendir(temp)) != NULL) {
	    working(1, 0, 0);
	
	    while ((de = readdir(dp))) {
		if (de->d_name[0] != '.') {
		    strcpy(menuname[mcount], strtok(de->d_name, "."));
		    mcount++;
		}
	    }
	    closedir(dp);
	}

	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 6, "8.3 MENU EDIT: %s", lang.Name);
	set_color(CYAN, BLACK);
		
	if (mcount) {
	    x = 6;
	    y = 7;
	    set_color(CYAN, BLACK);
	    for (i = 1; i <= mcount; i++) {
		snprintf(temp, 81, "%2d. %s", i, menuname[i-1]);
		mbse_mvprintw(y, x, temp);
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
	    mbse_mvprintw(LINES -3, 6, (char *)"New menu name >");
	    memset(&temp, 0, sizeof(temp));
	    strcpy(temp, edit_str(LINES -3, 22, 10, temp, (char *)"Enter a new ^menu^ name without extension"));
	    if (strlen(temp)) {
		p = xstrcpy(getenv("MBSE_ROOT"));
		p = xstrcat(p, (char *)"/share/int/menus/");
		p = xstrcat(p, lang.lc);
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
			working(3, 0, 0);
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
    char	    *temp;
    FILE	    *wp, *ip, *no, *mn;
    DIR		    *dp;
    struct dirent   *de;
    int		    j;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/language.data", getenv("MBSE_ROOT"));
    if ((no = fopen(temp, "r")) == NULL) {
	free(temp);
	return page;
    }

    page = newpage(fp, page);
    addtoc(fp, toc, 8, 3, page, (char *)"BBS Menus");

    fread(&langhdr, sizeof(langhdr), 1, no);
    j =0;

    ip = open_webdoc((char *)"menus.html", (char *)"BBS Menus", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");

    while ((fread(&lang, langhdr.recsize, 1, no)) == 1) {
	fprintf(ip, "<H3>BBS Menus for %s</H3>\n", lang.Name);
	fprintf(ip, "<UL>\n");

	snprintf(temp, PATH_MAX, "%s/share/int/menus/%s", getenv("MBSE_ROOT"), lang.lc);
	if ((dp = opendir(temp)) != NULL) {
	    while ((de = readdir(dp))) {
		if (de->d_name[0] != '.') {
		    j = 0;
		    fprintf(ip, "<LI><A HREF=\"menu_%s_%s.html\">%s</A></LI>\n", lang.LangKey, de->d_name, de->d_name);
		    snprintf(temp, PATH_MAX, "%s/share/int/menus/%s/%s", getenv("MBSE_ROOT"), lang.lc, de->d_name);
		    fprintf(fp, "\n    MENU %s (%s)\n\n", de->d_name, lang.Name);
		    if ((mn = fopen(temp, "r")) != NULL) {
			snprintf(temp, 81, "menu_%s_%s.html", lang.LangKey, de->d_name);
			if ((wp = open_webdoc(temp, lang.Name, de->d_name))) {
			    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"menus.html\">Back</A>\n");
			    while (fread(&menus, sizeof(menus), 1, mn) == 1) {
				fprintf(wp, "<P>\n");
				fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
				fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
				fprintf(wp, "<TBODY>\n");
				if (menus.MenuKey[0]) {
				    fprintf(fp, "    Menu select   %s\n", menus.MenuKey);
				    add_webtable(wp, (char *)"Menu select", menus.MenuKey);
				}
				if (menus.AutoExec) {
				    fprintf(fp, "    Menu select   Autoexec\n");
				    add_webtable(wp, (char *)"Menu select", (char *)"Autoexec");
				}
				snprintf(temp, 81, "%d %s", le_int(menus.MenuType), menus.TypeDesc);
				add_webtable(wp, (char *)"Menu type", temp);
				add_webtable(wp, (char *)"Optional data", menus.OptionalData);
				add_webtable(wp, (char *)"Display", menus.Display);
				web_secflags(wp, (char *)"Security level", menus.MenuSecurity);
				add_webdigit(wp, (char *)"Minimum age", menus.Age);
				add_colors(wp, (char *)"Normal colors", le_int(menus.ForeGnd), le_int(menus.BackGnd));
				add_colors(wp, (char *)"Bright colors", le_int(menus.HiForeGnd), le_int(menus.HiBackGnd));
				fprintf(fp, "    Type          %d %s\n", le_int(menus.MenuType), menus.TypeDesc);
				fprintf(fp, "    Opt. data     %s\n", menus.OptionalData);
				fprintf(fp, "    Display       %s\n", menus.Display);
				fprintf(fp, "    Security      %s\n", get_secstr(menus.MenuSecurity));
				fprintf(fp, "    Minimum age   %d\n", menus.Age);
				fprintf(fp, "    Lo-colors     %s on %s\n", 
						get_color(le_int(menus.ForeGnd)), get_color(le_int(menus.BackGnd)));
				fprintf(fp, "    Hi-colors     %s on %s\n", 
						get_color(le_int(menus.HiForeGnd)), get_color(le_int(menus.HiBackGnd)));
				if (le_int(menus.MenuType) == 7) {
				    add_webtable(wp, (char *)"Door Name", menus.DoorName);
				    add_webtable(wp, (char *)"No door.sys file", getboolean(menus.NoDoorsys));
				    add_webtable(wp, (char *)"Y2K format in door.sys", getboolean(menus.Y2Kdoorsys));
				    add_webtable(wp, (char *)"Use COM port", getboolean(menus.Comport));
				    add_webtable(wp, (char *)"Run nosuid", getboolean(menus.NoSuid));
				    add_webtable(wp, (char *)"No Prompt after door", getboolean(menus.NoPrompt));
				    add_webtable(wp, (char *)"Single user door", getboolean(menus.SingleUser));
				    add_webtable(wp, (char *)"Hidden door", getboolean(menus.HideDoor));
				    fprintf(fp, "    Door Name     %s\n", menus.DoorName);
				    fprintf(fp, "    No door.sys   %s", getboolean(menus.NoDoorsys));
				    fprintf(fp, "    Y2K door.sys  %s", getboolean(menus.Y2Kdoorsys));
				    fprintf(fp, "    Use COM port  %s\n", getboolean(menus.Comport));
				    fprintf(fp, "    Run nosuid    %s", getboolean(menus.NoSuid));
				    fprintf(fp, "    No Prompt     %s", getboolean(menus.NoPrompt));
				    fprintf(fp, "    Single user   %s\n", getboolean(menus.SingleUser));
				    fprintf(fp, "    Hidden door   %s\n", getboolean(menus.HideDoor));
				}
				fprintf(fp, "\n\n");
				j++;
				if (j == 4) {
				    j = 0;
				    page = newpage(fp, page);
				    fprintf(fp, "\n");
				}
				fprintf(wp, "</TBODY>\n");
				fprintf(wp, "</TABLE>\n");
			    }
			    close_webdoc(wp);
			}
			fclose(mn);
		    }
		    if (j)
			page = newpage(fp, page);
		}
	    }
	    closedir(dp);
	}
	fprintf(ip, "</UL>\n");
	fprintf(ip, "<HR>\n");
    }

    close_webdoc(ip);
	
    free(temp);
    fclose(no);
    return page;
}

