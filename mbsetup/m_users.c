/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Edit Users
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

	sprintf(ffile, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
			Syslog('+', "Created new %s", ffile);
			usrconfighdr.hdrsize = sizeof(usrconfighdr);
			usrconfighdr.recsize = sizeof(usrconfig);
			fwrite(&usrconfighdr, sizeof(usrconfighdr), 1, fil);
			fclose(fil);
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
	FILE	*fin, *fout;
	char	fnin[PATH_MAX], fnout[PATH_MAX];
	long	oldsize;

	sprintf(fnin,  "%s/etc/users.data", getenv("MBSE_ROOT"));
	sprintf(fnout, "%s/etc/users.temp", getenv("MBSE_ROOT"));
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

	sprintf(fin, "%s/etc/users.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/users.temp", getenv("MBSE_ROOT"));

	if (UsrUpdated == 1) {
		if (force || (yes_no((char *)"Database is changed, save changes") == 1)) {
			working(1, 0, 0);
			if ((rename(fout, fin)) == 0)
				unlink(fout);
			Syslog('+', "Updated \"users.data\"");
			return;
		}
	}
	working(1, 0, 0);
	unlink(fout); 
}



int AppendUsers(void)
{
	FILE	*fil;
	char	ffile[PATH_MAX];

	sprintf(ffile, "%s/etc/users.temp", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "a")) != NULL) {
		memset(&usrconfig, 0, sizeof(usrconfig));
		usrconfig.MailScan = TRUE;
		usrconfig.ieFILE = TRUE;
		fwrite(&usrconfig, sizeof(usrconfig), 1, fil);
		fclose(fil);
		UsrUpdated = 1;
		return 0;
	} else
		return -1;
}



void Screen1(void)
{
        clr_index();
        set_color(WHITE, BLACK);
        mvprintw( 4, 2, "15. EDIT USER");
        set_color(CYAN, BLACK);
        mvprintw( 6, 2, "1.  Full Name");
        mvprintw( 7, 2, "2.  Security");
        mvprintw( 8, 2, "3.  Expirydate");
        mvprintw( 9, 2, "4.  Expiry Sec");
        mvprintw(10, 2, "5.  Unix name");
	mvprintw(11, 2, "    1st login");
	mvprintw(12, 2, "    Last login");
	mvprintw(13, 2, "    Pwdchange");
        mvprintw(14, 2, "6.  Credit");
        mvprintw(15, 2, "7.  Hidden");
	mvprintw(16, 2, "8.  Deleted");
	mvprintw(17, 2, "9.  No Kill");
	mvprintw(18, 2, "10. Comment");

        mvprintw( 6,54, "11. Locked");
        mvprintw( 7,54, "12. Guest");
        mvprintw( 8,54, "13. Ext Info");
        mvprintw( 9,54, "14. Email");
	mvprintw(10,54, "    Calls");
	mvprintw(11,54, "    Downlds");
	mvprintw(12,54, "    Down Kb");
	mvprintw(13,54, "    Uploads");
	mvprintw(14,54, "    Upload Kb");
	mvprintw(15,54, "    Posted");
	mvprintw(16,54, "15. Screen 2");
}



void Fields1(void)
{
        char    Date[30];
        struct  tm *ld;

        set_color(WHITE, BLACK);
	show_str( 6,17,35, usrconfig.sUserName);
	show_int( 7,17,    usrconfig.Security.level);
	show_str( 8,17,10, usrconfig.sExpiryDate);
	show_int( 9,17,    usrconfig.ExpirySec.level);
	show_str(10,17, 8, usrconfig.Name);
	
        ld = localtime(&usrconfig.tFirstLoginDate);
        sprintf(Date, "%02d-%02d-%04d %02d:%02d:%02d", ld->tm_mday,
                ld->tm_mon+1, ld->tm_year + 1900, ld->tm_hour, ld->tm_min, ld->tm_sec);
        show_str(11,17,19, Date);
        ld = localtime(&usrconfig.tLastLoginDate);
        sprintf(Date, "%02d-%02d-%04d %02d:%02d:%02d", ld->tm_mday,
                ld->tm_mon+1, ld->tm_year + 1900, ld->tm_hour, ld->tm_min, ld->tm_sec);
        show_str(12,17,19, Date);
	ld = localtime(&usrconfig.tLastPwdChange);
	sprintf(Date, "%02d-%02d-%04d %02d:%02d:%02d", ld->tm_mday,
		ld->tm_mon+1, ld->tm_year + 1900, ld->tm_hour, ld->tm_min, ld->tm_sec);
	show_str(13,17,19, Date);
	
        show_int( 14,17,    usrconfig.Credit);
        show_bool(15,17,    usrconfig.Hidden);
	show_bool(16,17,    usrconfig.Deleted);
	show_bool(17,17,    usrconfig.NeverDelete);
	show_str( 18,17,63, usrconfig.sComment);

        show_bool( 6,68, usrconfig.LockedOut);
        show_bool( 7,68, usrconfig.Guest);
        show_bool( 8,68, usrconfig.OL_ExtInfo);
        show_bool( 9,68, usrconfig.Email);
	show_int( 10,68, usrconfig.iTotalCalls);
	show_int( 11,68, usrconfig.Downloads);
	show_int( 12,68, usrconfig.DownloadK);
	show_int( 13,68, usrconfig.Uploads);
	show_int( 14,68, usrconfig.UploadK);
	show_int( 15,68, usrconfig.iPosted);
}



void Screen2(void)
{
	clr_index();
	set_color(WHITE, BLACK);
	mvprintw( 4, 2, "15. EDIT USER PRIVATE SETTINGS");
	set_color(CYAN, BLACK);
	mvprintw( 6, 2, "1.  Handle");
	mvprintw( 7, 2, "2.  Location");
	mvprintw( 8, 2, "3.  Address 1");
	mvprintw( 9, 2, "4.  Address 2");
	mvprintw(10, 2, "5.  Address 3");
	mvprintw(11, 2, "6.  Voicephone");
	mvprintw(12, 2, "7.  Dataphone");
	mvprintw(13, 2, "8.  Birthdate");
	mvprintw(14, 2, "9.  Password");
	mvprintw(15, 2, "10. Sex");
	mvprintw(16, 2, "11. Protocol");
	mvprintw(17, 2, "12. Archiver");
	mvprintw(18, 2, "13. Screenlen");

	mvprintw( 6,63, "14. Language");
	mvprintw( 7,63, "15. Hotkeys");
	mvprintw( 8,63, "16. Color");
	mvprintw( 9,63, "17. Fs Chat");
	mvprintw(10,63, "18. Silent");
	mvprintw(11,63, "19. CLS");
	mvprintw(12,63, "20. More");
	mvprintw(13,63, "21. Fs Edit");
	mvprintw(14,63, "22. MailScan");
	mvprintw(15,63, "23. ShowNews");
	mvprintw(16,63, "24. NewFiles");
	mvprintw(17,63, "25. Emacs");
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

	if (usrconfig.iPassword == 0)
		show_str(14,17,12, (char *)"Invalid");
	else
		show_str(14,17,12, (char *)"********");
	show_str( 15,17, 7,usrconfig.sSex);
	show_str( 16,17,12,usrconfig.sProtocol);
	show_str( 17,17, 5,usrconfig.Archiver);
	show_int( 18,17,   usrconfig.iScreenLen);

	sprintf(temp, "%c",usrconfig.iLanguage);
	show_str(  6,76,1, temp);
	show_bool( 7,76,   usrconfig.HotKeys);
	show_bool( 8,76,   usrconfig.GraphMode);
	show_bool( 9,76,   usrconfig.Chat);
	show_bool(10,76,   usrconfig.DoNotDisturb);
	show_bool(11,76,   usrconfig.Cls);
	show_bool(12,76,   usrconfig.More);
	show_bool(13,76,   usrconfig.FsMsged);
	show_bool(14,76,   usrconfig.MailScan);
	show_bool(15,76,   usrconfig.ieNEWS);
	show_bool(16,76,   usrconfig.ieFILE);
	show_bool(17,76,   usrconfig.FSemacs);
}



int EditUsrRec2(void)
{
	int	j = 0;
	char	temp[PATH_MAX];

        Screen2();
        for (;;) {
                Fields2();
                j = select_menu(25);
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
                case 9: strcpy(temp,  edit_str(14,17,12, usrconfig.Password, (char *)"Enter the ^password^ for this user"));
                        if (strlen(temp)) {
			    if (strcasecmp(usrconfig.Password, temp)) {
				/*
				 * Only do something if password really changed.
				 */
				working(1,0,0);
                                memset(&usrconfig.Password, 0, sizeof(usrconfig.Password));
                                strcpy(usrconfig.Password, temp);
                                usrconfig.iPassword = StringCRC32(tu(temp));
				usrconfig.tLastPwdChange = time(NULL);
				Syslog('+', "%s/bin/mbpasswd -n %s ******", getenv("MBSE_ROOT"), usrconfig.Name);
				sprintf(temp, "%s/bin/mbpasswd -n %s %s", getenv("MBSE_ROOT"), usrconfig.Name, usrconfig.Password);
				if (system(temp) != 0) {
				    WriteError("$Failed to set new Unix password");
				} else {
				    Syslog('+', "Password changed for %s (%s)", usrconfig.sUserName, usrconfig.Name);
				}
			    }
                        } else {
			    working(2, 0, 0);
			}
                        working(0, 0, 0);
                        break;
                case 10:strcpy(usrconfig.sSex, tl(edit_str(15,17,7, usrconfig.sSex, (char *)"^Male^ or ^Female^")));
                        break;
                case 11:strcpy(temp, PickProtocol(15));
                        if (strlen(temp) != 0)
                                strcpy(usrconfig.sProtocol, temp);
                        clr_index();
                        Screen2();
                        break;
                case 12:strcpy(temp, PickArchive((char *)"15"));
                        if (strlen(temp) != 0)
                                strcpy(usrconfig.Archiver, temp);
                        clr_index();
                        Screen2();
                        break;
		case 13:E_INT( 18,17,usrconfig.iScreenLen,   "Users ^Screen length^ in lines (about 24)")

		case 14:usrconfig.iLanguage = PickLanguage((char *)"15.14");
			clr_index();
			Screen2();
			break;
                case 15:E_BOOL( 7,76,usrconfig.HotKeys,      "Is user using ^HotKeys^ for menus")
                case 16:E_BOOL( 8,76,usrconfig.GraphMode,    "Is user using ^ANSI^ colors")
                case 17:E_BOOL( 9,76,usrconfig.Chat,         "User has ^IEMSI Chat^ capability")
                case 18:E_BOOL(10,76,usrconfig.DoNotDisturb, "User will not be ^disturbed^")
                case 19:E_BOOL(11,76,usrconfig.Cls,          "Send ^ClearScreen code^ to users terminal")
                case 20:E_BOOL(12,76,usrconfig.More,         "User uses the ^More prompt^")
                case 21:E_BOOL(13,76,usrconfig.FsMsged,      "User uses the ^Fullscreen editor^")
                case 22:E_BOOL(14,76,usrconfig.MailScan,     "Don't check for ^new mail^")
                case 23:E_BOOL(15,76,usrconfig.ieNEWS,       "Show ^News Bulletins^ when logging in")
                case 24:E_BOOL(16,76,usrconfig.ieFILE,       "Show ^New Files^ when logging in")
                case 25:E_BOOL(17,76,usrconfig.FSemacs,      "Use ^Emacs^ or Wordstart shorcut keys in FS editor")
                }
        }
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditUsrRec(int Area)
{
	FILE	*fil;
	char	mfile[PATH_MAX];
	long	offset;
	int	j = 0;
	unsigned long crc, crc1;

	clr_index();
	working(1, 0, 0);
	IsDoing("Edit Users");

	sprintf(mfile, "%s/etc/users.temp", getenv("MBSE_ROOT"));
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
	crc = 0xffffffff;
	crc = upd_crc32((char *)&usrconfig, crc, sizeof(usrconfig));
	working(0, 0, 0);
	Screen1();

	for (;;) {
		Fields1();
		j = select_menu(15);
		switch(j) {
		case 0:
			crc1 = 0xffffffff;
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
					working(0, 0, 0);
				}
			}
			IsDoing("Browsing Menu");
			return 0;
		case 1:	E_STR(  6,17,35,usrconfig.sUserName,      "The ^First and Last name^ of this user")
		case 2:	E_USEC( 7,17,   usrconfig.Security,       "15.2   EDIT USER SECURITY", Screen1)
			break;
		case 3 :E_STR(  8,17,10,usrconfig.sExpiryDate,    "The ^Expiry Date^ in DD-MM-YYYY format, 00-00-0000 is no expire")
		case 4 :E_INT(  9,17,   usrconfig.ExpirySec.level,"The ^Expiry Level^ for this user")
		case 5 :E_STR( 10,17,8, usrconfig.Name,           "The ^Unix username^ for this user")
		case 6 :E_INT( 14,17,   usrconfig.Credit,         "Users ^Credit^")
		case 7 :E_BOOL(15,17,   usrconfig.Hidden,         "Is user ^hidden^ on the BBS")
		case 8 :E_BOOL(16,17,   usrconfig.Deleted,        "Is user marked for ^deletion^")
		case 9 :E_BOOL(17,17,   usrconfig.NeverDelete,    "^Never delete^ this user")
		case 10:E_STR( 18,17,62,usrconfig.sComment,       "A ^Comment^ for this user")

		case 11:E_BOOL( 6,68,   usrconfig.LockedOut,      "User is ^Locked Out^ of this BBS")
		case 12:E_BOOL( 7,68,   usrconfig.Guest,          "This is a ^Guest^ account")
		case 13:E_BOOL( 8,68,   usrconfig.OL_ExtInfo,     "Add ^Extended Message Info^ in OLR download")
		case 14:E_BOOL( 9,68,   usrconfig.Email,          "User has a ^private email^ mailbox")
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

	records = CountUsers();
	if (records == -1) {
		working(2, 0, 0);
		return;
	}

	if (OpenUsers() == -1) {
		working(2, 0, 0);
		return;
	}
	working(0, 0, 0);
	o = 0;

	for (;;) {
		clr_index();
		set_color(WHITE, BLACK);
		mvprintw( 5, 3, "15.  USERS EDITOR");
		set_color(CYAN, BLACK);
		if (records != 0) {
			sprintf(temp, "%s/etc/users.temp", getenv("MBSE_ROOT"));
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
						if (!usrconfig.Deleted)
							set_color(CYAN, BLACK);
						else
							set_color(LIGHTBLUE, BLACK);
						sprintf(temp, "%3d.  %-32s", o + i, usrconfig.sUserName);
						temp[37] = 0;
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
			CloseUsers(FALSE);
			return;
		}

		if (strncmp(pick, "A", 1) == 0) {
			working(1, 0, 0);
			if (AppendUsers() == 0) {
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
			EditUsrRec(atoi(pick));
	}
}



void InitUsers(void)
{
    CountUsers();
    OpenUsers();
    CloseUsers(TRUE);
}


