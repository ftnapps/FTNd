/*****************************************************************************
 *
 * $Id: m_users.c,v 1.34 2007/02/25 20:55:33 mbse Exp $
 * Purpose ...............: Edit Users
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
#include "../lib/users.h"
#include "screen.h"
#include "mutil.h"
#include "ledit.h"
#include "m_lang.h"
#include "m_global.h"
#include "m_archive.h"
#include "m_protocol.h"
#include "m_users.h"



int	UsrUpdated = 0;


/*
 * Count nr of usrconfig records in the database.
 * Creates the database if it doesn't exist.
 */
int CountUsers(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];
	int	count;

	snprintf(ffile, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			usrconfighdr.hdrsize = sizeof(usrconfighdr);
			usrconfighdr.recsize = sizeof(usrconfig);
			fwrite(&usrconfighdr, sizeof(usrconfighdr), 1, fil);
			fclose(fil);
			chmod(ffile, 0660);
			return 0;
		} else
			return -1;
	}

	count = 0;
	fread(&usrconfighdr, sizeof(usrconfighdr), 1, fil);

	while (fread(&usrconfig, usrconfighdr.recsize, 1, fil) == 1) {
		count++;
	}
	fclose(fil);

	return count;
}



/*
 * Open database for editing. The datafile is copied, if the format
 * is changed it will be converted on the fly. All editing must be 
 * done on the copied file.
 */
int OpenUsers(void);
int OpenUsers(void)
{
    FILE    *fin, *fout;
    char    fnin[PATH_MAX], fnout[PATH_MAX];
    int	    oldsize;

    snprintf(fnin,  PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
    snprintf(fnout, PATH_MAX, "%s/etc/users.temp", getenv("MBSE_ROOT"));
    if ((fin = fopen(fnin, "r")) != NULL) {
	if ((fout = fopen(fnout, "w")) != NULL) {
	    fread(&usrconfighdr, sizeof(usrconfighdr), 1, fin);
	    /*
	     * In case we are automatic upgrading the data format
	     * we save the old format. If it is changed, the
	     * database must always be updated.
	     */
	    oldsize = usrconfighdr.recsize;
	    if (oldsize != sizeof(usrconfig)) {
		UsrUpdated = 1;
		Syslog('+', "Upgraded %s, format changed", fnin);
	    } else
		UsrUpdated = 0;
	    usrconfighdr.hdrsize = sizeof(usrconfighdr);
	    usrconfighdr.recsize = sizeof(usrconfig);
	    fwrite(&usrconfighdr, sizeof(usrconfighdr), 1, fout);

	    /*
	     * The datarecord is filled with zero's before each
	     * read, so if the format changed, the new fields
	     * will be empty.
	     */
	    memset(&usrconfig, 0, sizeof(usrconfig));
	    while (fread(&usrconfig, oldsize, 1, fin) == 1) {
		/*
		 * Since 0.83.0 there is no more line editor.
		 */
		if ((usrconfig.MsgEditor == X_LINEEDIT) || 
			((usrconfig.MsgEditor == EXTEDIT) && (strlen(CFG.externaleditor) == 0))) {
		    if (strlen(CFG.externaleditor))
			usrconfig.MsgEditor = EXTEDIT;
		    else
			usrconfig.MsgEditor = FSEDIT;
		    UsrUpdated = 1;
		    Syslog('+', "Adjusted editor setting for user %s", usrconfig.sUserName);
		}
		fwrite(&usrconfig, sizeof(usrconfig), 1, fout);
		memset(&usrconfig, 0, sizeof(usrconfig));
	    }

	    fclose(fin);
	    fclose(fout);
	    return 0;
	} else
	    return -1;
    }
    return -1;
}



void CloseUsers(int);
void CloseUsers(int force)
{
	char	fin[PATH_MAX], fout[PATH_MAX];

	snprintf(fin,  PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
	snprintf(fout, PATH_MAX, "%s/etc/users.temp", getenv("MBSE_ROOT"));

	if (UsrUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			if ((rename(fout, fin)) == 0)
				unlink(fout);
			chmod(fin, 0660);
			Syslog('+', "Updated \"users.data\"");
			if (!force)
			    working(6, 0, 0);
			return;
		}
	}
	chmod(fin, 0660);
	unlink(fout); 
}



void Screen1(void)
{
        clr_index();
        set_color(WHITE, BLACK);
        mbse_mvprintw( 4, 2, "15. EDIT USER");
        set_color(CYAN, BLACK);
        mbse_mvprintw( 6, 2, "1.  Full Name");
        mbse_mvprintw( 7, 2, "2.  Security");
        mbse_mvprintw( 8, 2, "3.  Expirydate");
        mbse_mvprintw( 9, 2, "4.  Expiry Sec");
        mbse_mvprintw(10, 2, "    Unix name");
	mbse_mvprintw(11, 2, "    1st login");
	mbse_mvprintw(12, 2, "    Last login");
	mbse_mvprintw(13, 2, "    Pwdchange");
        mbse_mvprintw(14, 2, "5.  Credit");
        mbse_mvprintw(15, 2, "6.  Hidden");
	mbse_mvprintw(16, 2, "7.  Deleted");
	mbse_mvprintw(17, 2, "8.  No Kill");
	mbse_mvprintw(18, 2, "9.  Comment");

        mbse_mvprintw( 6,54, "10. Locked");
        mbse_mvprintw( 7,54, "11. Guest");
        mbse_mvprintw( 8,54, "12. Ext Info");
        mbse_mvprintw( 9,54, "13. Email");
	mbse_mvprintw(10,54, "    Calls");
	mbse_mvprintw(11,54, "    Downlds");
	mbse_mvprintw(12,54, "    Down Kb");
	mbse_mvprintw(13,54, "    Uploads");
	mbse_mvprintw(14,54, "    Upload Kb");
	mbse_mvprintw(15,54, "    Posted");
	mbse_mvprintw(16,54, "14. Time left");
	mbse_mvprintw(17,54, "15. Screen 2");
}



void Fields1(void)
{
        char    Date[30];
        struct  tm *ld;
	time_t	now;

        set_color(WHITE, BLACK);
	show_str( 6,17,35, usrconfig.sUserName);
	show_int( 7,17,    usrconfig.Security.level);
	show_str( 8,17,10, usrconfig.sExpiryDate);
	show_int( 9,17,    usrconfig.ExpirySec.level);
	set_color(LIGHTGRAY, BLACK);
	show_str(10,17, 8, usrconfig.Name);

	now = usrconfig.tFirstLoginDate;
        ld = localtime(&now);
        snprintf(Date, 30, "%02d-%02d-%04d %02d:%02d:%02d", ld->tm_mday,
                ld->tm_mon+1, ld->tm_year + 1900, ld->tm_hour, ld->tm_min, ld->tm_sec);
        show_str(11,17,19, Date);
	now = usrconfig.tLastLoginDate;
        ld = localtime(&now);
        snprintf(Date, 30, "%02d-%02d-%04d %02d:%02d:%02d", ld->tm_mday,
                ld->tm_mon+1, ld->tm_year + 1900, ld->tm_hour, ld->tm_min, ld->tm_sec);
        show_str(12,17,19, Date);
	now = usrconfig.tLastPwdChange;
	ld = localtime(&now);
	snprintf(Date, 30, "%02d-%02d-%04d %02d:%02d:%02d", ld->tm_mday,
		ld->tm_mon+1, ld->tm_year + 1900, ld->tm_hour, ld->tm_min, ld->tm_sec);
	show_str(13,17,19, Date);
	
	set_color(WHITE, BLACK);
        show_int( 14,17,    usrconfig.Credit);
        show_bool(15,17,    usrconfig.Hidden);
	show_bool(16,17,    usrconfig.Deleted);
	show_bool(17,17,    usrconfig.NeverDelete);
	show_str( 18,17,63, usrconfig.sComment);

        show_bool( 6,68, usrconfig.LockedOut);
        show_bool( 7,68, usrconfig.Guest);
        show_bool( 8,68, usrconfig.OL_ExtInfo);
        show_bool( 9,68, usrconfig.Email);
	set_color(LIGHTGRAY, BLACK);
	show_int( 10,68, usrconfig.iTotalCalls);
	show_int( 11,68, usrconfig.Downloads);
	show_int( 12,68, usrconfig.DownloadK);
	show_int( 13,68, usrconfig.Uploads);
	show_int( 14,68, usrconfig.UploadK);
	show_int( 15,68, usrconfig.iPosted);
	set_color(WHITE, BLACK);
	show_int( 16,68, usrconfig.iTimeLeft);
}



void Screen2(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 4, 2, "15. EDIT USER PRIVATE SETTINGS");
	set_color(CYAN, BLACK);
	mbse_mvprintw( 6, 2, "1.  Handle");
	mbse_mvprintw( 7, 2, "2.  Location");
	mbse_mvprintw( 8, 2, "3.  Address 1");
	mbse_mvprintw( 9, 2, "4.  Address 2");
	mbse_mvprintw(10, 2, "5.  Address 3");
	mbse_mvprintw(11, 2, "6.  Voicephone");
	mbse_mvprintw(12, 2, "7.  Dataphone");
	mbse_mvprintw(13, 2, "8.  Birthdate");
	mbse_mvprintw(14, 2, "9.  Password");
	mbse_mvprintw(15, 2, "10. Sex");
	mbse_mvprintw(16, 2, "11. Protocol");
	mbse_mvprintw(17, 2, "12. Archiver");
	mbse_mvprintw(18, 2, "13. Charset");

	mbse_mvprintw( 7,63, "14. Language");
	mbse_mvprintw( 8,63, "15. Hotkeys");
	mbse_mvprintw( 9,63, "16. Silent");
	mbse_mvprintw(10,63, "17. CLS");
	mbse_mvprintw(11,63, "18. More");
	mbse_mvprintw(12,63, "19. Editor");
	mbse_mvprintw(13,63, "20. MailScan");
	mbse_mvprintw(14,63, "21. ShowNews");
	mbse_mvprintw(15,63, "22. NewFiles");
	mbse_mvprintw(16,63, "23. Emacs");
}



void Fields2(void)
{
	char	temp[4];

	set_color(WHITE, BLACK);
	show_str( 6,17,35, usrconfig.sHandle);
	show_str( 7,17,27, usrconfig.sLocation);
	show_str( 8,17,40, usrconfig.address[0]);
	show_str( 9,17,40, usrconfig.address[1]);
	show_str(10,17,40, usrconfig.address[2]);
	show_str(11,17,19, usrconfig.sVoicePhone);
	show_str(12,17,19, usrconfig.sDataPhone);
	show_str(13,17,10, usrconfig.sDateOfBirth);
	show_str(14,17,Max_passlen, (char *)"**************");
	show_str( 15,17, 7,usrconfig.sSex);
	show_str( 16,17,12,usrconfig.sProtocol);
	show_str( 17,17, 5,usrconfig.Archiver);
	show_charset(18,17,usrconfig.Charset);

	snprintf(temp, 4, "%c",usrconfig.iLanguage);
	show_str(  7,76,1, temp);
	show_bool( 8,76,   usrconfig.HotKeys);
	show_bool( 9,76,   usrconfig.DoNotDisturb);
	show_bool(10,76,   usrconfig.Cls);
	show_bool(11,76,   usrconfig.More);
	show_msgeditor(12,76, usrconfig.MsgEditor);
	show_bool(13,76,   usrconfig.MailScan);
	show_bool(14,76,   usrconfig.ieNEWS);
	show_bool(15,76,   usrconfig.ieFILE);
	show_bool(16,76,   usrconfig.FSemacs);
}



int EditUsrRec2(void)
{
    int	    j = 0, ch;
    char    temp[PATH_MAX], *args[16];

    Screen2();
    for (;;) {
        Fields2();
        j = select_menu(23);
        switch(j) {
            case 0: return 0;
            case 1: E_STR( 6,17,35,usrconfig.sHandle,  "The ^Handle^ of this user")
            case 2: E_STR( 7,17,27,usrconfig.sLocation,"The users ^Location^")
            case 3:
            case 4:
            case 5: E_STR(j+5,17,40,usrconfig.address[j-3],"^Address^")
            case 6: E_STR(11,17,16, usrconfig.sVoicePhone, "The ^Voice Phone^ number of this user")
            case 7: E_STR(12,17,16, usrconfig.sDataPhone,  "The ^Data Phone^ number of this user")
            case 8: E_STR(13,17,10, usrconfig.sDateOfBirth,"The ^Date of Birth^ in DD-MM-YYYY format")
            case 9: strcpy(temp,edit_str(14,17,Max_passlen,usrconfig.Password,(char *)"Enter the ^password^ for this user"));
                    if (strlen(temp)) {
			if (strcasecmp(usrconfig.Password, temp)) {
			    /*
			     * Only do something if password really changed.
			     */
			    working(1,0,0);
                            memset(&usrconfig.Password, 0, sizeof(usrconfig.Password));
                            strcpy(usrconfig.Password, temp);
			    usrconfig.tLastPwdChange = time(NULL);
			    Syslog('+', "%s/bin/mbpasswd %s ******", getenv("MBSE_ROOT"), usrconfig.Name);
			    snprintf(temp, PATH_MAX, "%s/bin/mbpasswd", getenv("MBSE_ROOT"));
			    memset(args, 0, sizeof(args));
			    args[0] = temp;
			    args[1] = usrconfig.Name;
			    args[2] = usrconfig.Password;
			    args[3] = NULL;

			    if (execute(args, (char *)"/dev/null", (char *)"/dev/null", (char *)"/dev/null")!= 0) {
			        WriteError("$Failed to set new Unix password");
			    } else {
			        Syslog('+', "Password changed for %s (%s)", usrconfig.sUserName, usrconfig.Name);
			    }
			}
                    } else {
		        working(2, 0, 0);
		    }
                    break;
            case 10:showhelp((char *)"Toggle ^Sex^ with spacebar, press <Enter> when done.");
		    do {
			set_color(YELLOW, BLUE);
			show_str(15,17,7, usrconfig.sSex);
			ch = readkey(15, 17, YELLOW, BLUE);
			if (ch == ' ') {
			    if (strcmp(usrconfig.sSex, "Male") == 0)
				strcpy(usrconfig.sSex, "Female");
			    else {
				strcpy(usrconfig.sSex, "Male\0\0");
			    }
			}
		    } while (ch != KEY_ENTER && ch != '\012');
		    set_color(WHITE, BLACK);
		    show_str(15,17,7, usrconfig.sSex);
                    break;
            case 11:strcpy(temp, PickProtocol(15));
                    if (strlen(temp) != 0)
                        strcpy(usrconfig.sProtocol, temp);
                    clr_index();
                    Screen2();
                    break;
            case 12:strcpy(temp, PickArchive((char *)"15", TRUE));
                    if (strlen(temp) != 0)
                        strcpy(usrconfig.Archiver, temp);
                    clr_index();
                    Screen2();
                    break;
	    case 13:usrconfig.Charset = edit_charset(18,17, usrconfig.Charset); break;

	    case 14:usrconfig.iLanguage = PickLanguage((char *)"15.14");
		    clr_index();
		    Screen2();
		    break;
            case 15:E_BOOL( 8,76,usrconfig.HotKeys,      "Is user using ^HotKeys^ for menus")
            case 16:E_BOOL( 9,76,usrconfig.DoNotDisturb, "User will not be ^disturbed^")
            case 17:E_BOOL(10,76,usrconfig.Cls,          "Send ^ClearScreen code^ to users terminal")
            case 18:E_BOOL(11,76,usrconfig.More,         "User uses the ^More prompt^")
            case 19:usrconfig.MsgEditor = edit_msgeditor(12,76,usrconfig.MsgEditor);
		    break;
            case 20:E_BOOL(13,76,usrconfig.MailScan,     "Don't check for ^new mail^")
            case 21:E_BOOL(14,76,usrconfig.ieNEWS,       "Show ^News Bulletins^ when logging in")
            case 22:E_BOOL(15,76,usrconfig.ieFILE,       "Show ^New Files^ when logging in")
            case 23:E_BOOL(16,76,usrconfig.FSemacs,      "Use ^Emacs^ or Wordstart shorcut keys in FS editor")
        }
    }
}


void Reset_Time(void);
void Reset_Time(void)
{
    char    *temp;
    FILE    *pLimits;

    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s/etc/limits.data", getenv("MBSE_ROOT"));
    if ((pLimits = fopen(temp,"r")) == NULL) {
	WriteError("$Can't open %s", temp);
    } else {
	fread(&LIMIThdr, sizeof(LIMIThdr), 1, pLimits);
	while (fread(&LIMIT, sizeof(LIMIT), 1, pLimits) == 1) {
	    if (LIMIT.Security == usrconfig.Security.level) {
		if (LIMIT.Time)
		    usrconfig.iTimeLeft = LIMIT.Time;
		else
		    usrconfig.iTimeLeft = 86400;
		usrconfig.iTimeUsed = 0;
		break;
	    }
	}
	fclose(pLimits);
    }
    free(temp);
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditUsrRec(int Area)
{
    FILE	    *fil;
    char	    mfile[PATH_MAX];
    int		    offset;
    int		    j = 0;
    unsigned int    crc, crc1, level;

    clr_index();
    working(1, 0, 0);
    IsDoing("Edit Users");

    snprintf(mfile, PATH_MAX, "%s/etc/users.temp", getenv("MBSE_ROOT"));
    if ((fil = fopen(mfile, "r")) == NULL) {
	working(2, 0, 0);
	return -1;
    }

    offset = sizeof(usrconfighdr) + ((Area -1) * sizeof(usrconfig));
    if (fseek(fil, offset, 0) != 0) {
	working(2, 0, 0);
	return -1;
    }

    fread(&usrconfig, sizeof(usrconfig), 1, fil);
    fclose(fil);

    if (strlen(usrconfig.sUserName) == 0) {
	errmsg((char *)"You cannot edit an empty record");
	return -1;
    }

    crc = 0xffffffff;
    crc = upd_crc32((char *)&usrconfig, crc, sizeof(usrconfig));
    Screen1();

    for (;;) {
	Fields1();
	j = select_menu(15);
	switch(j) {
	case 0: crc1 = 0xffffffff;
		crc1 = upd_crc32((char *)&usrconfig, crc1, sizeof(usrconfig));
		if (crc != crc1) {
		    if (yes_no((char *)"Record is changed, save") == 1) {
			working(1, 0, 0);
			if ((fil = fopen(mfile, "r+")) == NULL) {
			    working(2, 0, 0);
			    return -1;
			}
			fseek(fil, offset, 0);
			fwrite(&usrconfig, sizeof(usrconfig), 1, fil);
			fclose(fil);
			UsrUpdated = 1;
			working(6, 0, 0);
		    }
		}
		IsDoing("Browsing Menu");
		return 0;
	case 1:	E_STR(  6,17,35,usrconfig.sUserName,      "The ^First and Last name^ of this user")
	case 2:	level = usrconfig.Security.level;
		usrconfig.Security = edit_usec(7,17,usrconfig.Security, (char *)"15.2   EDIT USER SECURITY");
		Screen1();
		Fields1();
		if (level != usrconfig.Security.level) {
		    if (yes_no((char *)"Set time left for new level") == 1) {
			Reset_Time();
		    }
		}
		break;
	case 3 :E_STR(  8,17,10,usrconfig.sExpiryDate,    "The ^Expiry Date^ in DD-MM-YYYY format, 00-00-0000 is no expire")
	case 4 :E_INT(  9,17,   usrconfig.ExpirySec.level,"The ^Expiry Level^ for this user")
	case 5 :E_INT( 14,17,   usrconfig.Credit,         "Users ^Credit^")
	case 6 :E_BOOL(15,17,   usrconfig.Hidden,         "Is user ^hidden^ on the BBS")
	case 7 :E_BOOL(16,17,   usrconfig.Deleted,        "Is user marked for ^deletion^")
	case 8 :E_BOOL(17,17,   usrconfig.NeverDelete,    "^Never delete^ this user")
	case 9 :E_STR( 18,17,62,usrconfig.sComment,       "A ^Comment^ for this user")

	case 10:E_BOOL( 6,68,   usrconfig.LockedOut,      "User is ^Locked Out^ of this BBS")
	case 11:E_BOOL( 7,68,   usrconfig.Guest,          "This is a ^Guest^ account")
	case 12:E_BOOL( 8,68,   usrconfig.OL_ExtInfo,     "Add ^Extended Message Info^ in OLR download")
	case 13:E_BOOL( 9,68,   usrconfig.Email,          "User has a ^private email^ mailbox")
	case 14:if (yes_no((char *)"Reset time left for today") == 1) {
		    Reset_Time();
		}
		break;
	case 15:EditUsrRec2();
		clr_index();
		Screen1();
		Fields1();
		break;
	}
    }
    return 0;
}



void EditUsers(void)
{
    int	    records, i, o, x, y;
    char    pick[12];
    FILE    *fil;
    char    temp[PATH_MAX];
    int	    offset;

    clr_index();
    working(1, 0, 0);
    IsDoing("Browsing Menu");
    if (config_read() == -1) {
	working(2, 0, 0);
	return;
    }

    records = CountUsers();
    if (records == -1) {
	working(2, 0, 0);
	return;
    }

    if (OpenUsers() == -1) {
	working(2, 0, 0);
	return;
    }
    o = 0;
    if (! check_free())
	return;

    for (;;) {
	clr_index();
	set_color(WHITE, BLACK);
	mbse_mvprintw( 5, 3, "15.  USERS EDITOR");
	set_color(CYAN, BLACK);
	if (records != 0) {
	    snprintf(temp, PATH_MAX, "%s/etc/users.temp", getenv("MBSE_ROOT"));
	    working(1, 0, 0);
	    if ((fil = fopen(temp, "r")) != NULL) {
		fread(&usrconfighdr, sizeof(usrconfighdr), 1, fil);
		x = 2;
		y = 7;
		set_color(CYAN, BLACK);
		for (i = 1; i <= 20; i++) {
		    if (i == 11) {
			x = 42;
			y = 7;
		    }
		    if ((o + i) <= records) {
			offset = sizeof(usrconfighdr) + (((i + o) - 1) * usrconfighdr.recsize);
			fseek(fil, offset, 0);
			fread(&usrconfig, usrconfighdr.recsize, 1, fil);
			if ((!usrconfig.Deleted) && strlen(usrconfig.sUserName))
			    set_color(CYAN, BLACK);
			else
			    set_color(LIGHTBLUE, BLACK);
			snprintf(temp, 81, "%3d.  %-32s", o + i, usrconfig.sUserName);
			temp[37] = 0;
			mbse_mvprintw(y, x, temp);
			y++;
		    }
		}
		fclose(fil);
	    }
	}
	strcpy(pick, select_pick(records, 20));
		
	if (strncmp(pick, "-", 1) == 0) {
	    CloseUsers(FALSE);
	    open_bbs();
	    return;
	}

	if (strncmp(pick, "N", 1) == 0) 
	    if ((o + 20) < records)
		o = o + 20;

	if (strncmp(pick, "P", 1) == 0)
	    if ((o - 20) >= 0)
		o = o - 20;

	if ((atoi(pick) >= 1) && (atoi(pick) <= records)) {
	    EditUsrRec(atoi(pick));
	    o = ((atoi(pick) - 1) / 20) * 20;
	}
    }
}



void InitUsers(void)
{
    CountUsers();
    OpenUsers();
    CloseUsers(TRUE);
}



void users_doc(void)
{
    char    temp[PATH_MAX];
    FILE    *wp, *ip, *fp;
    int	    nr = 0;
    time_t  tt;

    snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp, "r")) == NULL)
	return;

    fread(&usrconfighdr, sizeof(usrconfighdr), 1, fp);

    ip = open_webdoc((char *)"users.html", (char *)"BBS Users", NULL);
    fprintf(ip, "<A HREF=\"index.html\">Main</A>\n");
    fprintf(ip, "<UL>\n");
		    
    while (fread(&usrconfig, usrconfighdr.recsize, 1, fp) == 1) {
	nr++;
	snprintf(temp, 81, "user_%d.html", nr);
	fprintf(ip, "<LI><A HREF=\"%s\">%s</A></LI>\n", temp, usrconfig.sUserName);
	if ((wp = open_webdoc(temp, (char *)"BBS User", usrconfig.sUserName))) {
	    fprintf(wp, "<A HREF=\"index.html\">Main</A>&nbsp;<A HREF=\"users.html\">Back</A>\n");
	    fprintf(wp, "<P>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Fidonet Name", usrconfig.sUserName);
	    add_webtable(wp, (char *)"Unix Name", usrconfig.Name);
	    web_secflags(wp, (char *)"Security level", usrconfig.Security);
	    add_webtable(wp, (char *)"Expiry date", usrconfig.sExpiryDate);
	    web_secflags(wp, (char *)"Expiry security level", usrconfig.ExpirySec);
	    tt = (time_t)usrconfig.tFirstLoginDate;
	    add_webtable(wp, (char *)"First login date", ctime(&tt));
	    tt = (time_t)usrconfig.tLastLoginDate;
	    add_webtable(wp, (char *)"Last login date", ctime(&tt));
	    tt = (time_t)usrconfig.tLastPwdChange;
	    add_webtable(wp, (char *)"Last password change", ctime(&tt));
	    add_webdigit(wp, (char *)"Credit", usrconfig.Credit);
	    add_webtable(wp, (char *)"Hidden from lists", getboolean(usrconfig.Hidden));
	    add_webtable(wp, (char *)"Never delete", getboolean(usrconfig.NeverDelete));
	    add_webtable(wp, (char *)"Comment", usrconfig.sComment);
	    add_webtable(wp, (char *)"Locked out", getboolean(usrconfig.LockedOut));
	    add_webtable(wp, (char *)"Guest user", getboolean(usrconfig.Guest));
	    add_webtable(wp, (char *)"OLR Extended info", getboolean(usrconfig.OL_ExtInfo));
	    add_webtable(wp, (char *)"Has e-mail", getboolean(usrconfig.Email));
	    add_webdigit(wp, (char *)"Total calls", usrconfig.iTotalCalls);
	    add_webdigit(wp, (char *)"total downloads", usrconfig.Downloads);
	    add_webdigit(wp, (char *)"Downloaded KBytes", usrconfig.DownloadK);
	    add_webdigit(wp, (char *)"Total uploads", usrconfig.Uploads);
	    add_webdigit(wp, (char *)"Uploaded KBytes", usrconfig.UploadK);
	    add_webdigit(wp, (char *)"Posted messages", usrconfig.iPosted);
	    add_webdigit(wp, (char *)"Minutes left today", usrconfig.iTimeLeft);
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    fprintf(wp, "<H3>User personal settings</H3>\n");
	    fprintf(wp, "<TABLE width='600' border='0' cellspacing='0' cellpadding='2'>\n");
	    fprintf(wp, "<COL width='30%%'><COL width='70%%'>\n");
	    fprintf(wp, "<TBODY>\n");
	    add_webtable(wp, (char *)"Handle", usrconfig.sHandle);
	    add_webtable(wp, (char *)"Location", usrconfig.sLocation);
	    add_webtable(wp, (char *)"Address", usrconfig.address[0]);
	    add_webtable(wp, (char *)"Address", usrconfig.address[1]);
	    add_webtable(wp, (char *)"Address", usrconfig.address[2]);
	    add_webtable(wp, (char *)"Voice phone", usrconfig.sVoicePhone);
	    add_webtable(wp, (char *)"Data phone", usrconfig.sDataPhone);
	    add_webtable(wp, (char *)"Date of birth", usrconfig.sDateOfBirth);
	    add_webtable(wp, (char *)"Password", usrconfig.Password);
	    add_webtable(wp, (char *)"Sex", usrconfig.sSex);
	    add_webtable(wp, (char *)"Protocol", usrconfig.sProtocol);
	    add_webtable(wp, (char *)"Archiver", usrconfig.Archiver);
	    add_webtable(wp, (char *)"Character set", getftnchrs(usrconfig.Charset));
	    snprintf(temp, 4, "%c", usrconfig.iLanguage);
	    add_webtable(wp, (char *)"Language", temp);
	    add_webtable(wp, (char *)"Use hotkeys", getboolean(usrconfig.HotKeys));
	    add_webtable(wp, (char *)"Do not disturb", getboolean(usrconfig.DoNotDisturb));
	    add_webtable(wp, (char *)"Clear Screen", getboolean(usrconfig.Cls));
	    add_webtable(wp, (char *)"More prompt", getboolean(usrconfig.More));
	    add_webtable(wp, (char *)"Message editor", getmsgeditor(usrconfig.MsgEditor));
	    add_webtable(wp, (char *)"Scan new mail", getboolean(usrconfig.MailScan));
	    add_webtable(wp, (char *)"Display news", getboolean(usrconfig.ieNEWS));
	    add_webtable(wp, (char *)"Display newfiles", getboolean(usrconfig.ieFILE));
	    add_webtable(wp, (char *)"Emacs editor keys", getboolean(usrconfig.FSemacs));
	    fprintf(wp, "</TBODY>\n");
	    fprintf(wp, "</TABLE>\n");
	    close_webdoc(wp);
	}
    }

    fprintf(ip, "</UL>\n");
    close_webdoc(ip);
	    
    fclose(fp);
}


