/*****************************************************************************
 *
 * File ..................: setup/m_users.c
 * Purpose ...............: Edit Users
 * Last modification date : 29-Jul-2000
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
	char	ffile[81];
	int	count;

	sprintf(ffile, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((fil = fopen(ffile, "r")) == NULL) {
		if ((fil = fopen(ffile, "a+")) != NULL) {
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
int OpenUsers(void)
{
	FILE	*fin, *fout;
	char	fnin[81], fnout[81];
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
			if (oldsize != sizeof(usrconfig))
				UsrUpdated = 1;
			else
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



void CloseUsers(void)
{
	char	fin[81], fout[81];

	sprintf(fin, "%s/etc/users.data", getenv("MBSE_ROOT"));
	sprintf(fout,"%s/etc/users.temp", getenv("MBSE_ROOT"));

	if (UsrUpdated == 1) {
		if (yes_no((char *)"Database is changed, save changes") == 1) {
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
	char	ffile[81];

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
	mvprintw( 4,21, "First login");
	mvprintw( 5,21, "Last login");
	mvprintw( 6, 2, "1.  Full Name");
	mvprintw( 7, 2, "2.  Handle");
	mvprintw( 8, 2, "3.  Location");
	mvprintw( 9, 2, "4.  Address 1");
	mvprintw(10, 2, "5.  Address 2");
	mvprintw(11, 2, "6.  Address 3");
	mvprintw(12, 2, "7.  Voicephone");
	mvprintw(13, 2, "8.  Dataphone");
	mvprintw(14, 2, "9.  Security");
	mvprintw(15, 2, "10. Birthdate");
	mvprintw(16, 2, "11. Expirydate");
	mvprintw(17, 2, "12. Expiry Sec");
	mvprintw(18, 2, "13. Unix uid");
	mvprintw(19, 2, "14. Comment");

	mvprintw(12,37, "15. Password");
	mvprintw(13,37, "16. Sex");
	mvprintw(14,37, "17. Credit");
	mvprintw(15,37, "18. Protocol");
	mvprintw(16,37, "19. Archiver");
	mvprintw(17,37, "20. Hidden");
	mvprintw(18,37, "21. Hotkeys");

	mvprintw( 4,63, "22. Color");
	mvprintw( 5,63, "23. Deleted");
	mvprintw( 6,63, "24. No Kill");
	mvprintw( 7,63, "25. Fs Chat");
	mvprintw( 8,63, "26. Locked");
	mvprintw( 9,63, "27. Silent");
	mvprintw(10,63, "28. CLS");
	mvprintw(11,63, "29. More");
	mvprintw(12,63, "30. Fs Edit");
	mvprintw(13,63, "31. MailScan");
	mvprintw(14,63, "32. Guest");
	mvprintw(15,63, "33. ShowNews");
	mvprintw(16,63, "34. NewFiles");
	mvprintw(17,63, "35. Ext Info");
	mvprintw(18,63, "36. Email");
}



void Fields1(void)
{
	char	Date[30];
	struct	tm *ld;

	set_color(WHITE, BLACK);
	ld = localtime(&usrconfig.tFirstLoginDate);
	sprintf(Date, "%02d-%02d-%04d %02d:%02d:%02d", ld->tm_mday, 
		ld->tm_mon+1, ld->tm_year + 1900, ld->tm_hour, ld->tm_min, ld->tm_sec);
	show_str( 4,33,19, Date);
	ld = localtime(&usrconfig.tLastLoginDate);
	sprintf(Date, "%02d-%02d-%04d %02d:%02d:%02d", ld->tm_mday, 
		ld->tm_mon+1, ld->tm_year + 1900, ld->tm_hour, ld->tm_min, ld->tm_sec);
	show_str( 5,33,19, Date);
	show_str( 6,17,35, usrconfig.sUserName);
	show_str( 7,17,35, usrconfig.sHandle);
	show_str( 8,17,27, usrconfig.sLocation);
	show_str( 9,17,40, usrconfig.address[0]);
	show_str(10,17,40, usrconfig.address[1]);
	show_str(11,17,40, usrconfig.address[2]);
	show_str(12,17,19, usrconfig.sVoicePhone);
	show_str(13,17,19, usrconfig.sDataPhone);
	show_int(14,17,    usrconfig.Security.level);
	show_str(15,17,10, usrconfig.sDateOfBirth);
	show_str(16,17,10, usrconfig.sExpiryDate);
	show_int(17,17,    usrconfig.ExpirySec.level);
	show_str(18,17, 8, usrconfig.Name);
	show_str(19,17,63, usrconfig.sComment);

	if (usrconfig.iPassword == 0)
		show_str(12,50,12, (char *)"Invalid");
	else
		show_str(12,50,12, (char *)"********");
	show_str( 13,50, 7,usrconfig.sSex);
	show_int( 14,50,   usrconfig.Credit);
	show_str( 15,50,12,usrconfig.sProtocol);
	show_str( 16,50, 5,usrconfig.Archiver);
	show_bool(17,50,   usrconfig.Hidden);
	show_bool(18,50,   usrconfig.HotKeys);

	show_bool( 4,76, usrconfig.GraphMode);
	show_bool( 5,76, usrconfig.Deleted);
	show_bool( 6,76, usrconfig.NeverDelete);
	show_bool( 7,76, usrconfig.Chat);
	show_bool( 8,76, usrconfig.LockedOut);
	show_bool( 9,76, usrconfig.DoNotDisturb);
	show_bool(10,76, usrconfig.Cls);
	show_bool(11,76, usrconfig.More);
	show_bool(12,76, usrconfig.FsMsged);
	show_bool(13,76, usrconfig.MailScan);
	show_bool(14,76, usrconfig.Guest);
	show_bool(15,76, usrconfig.ieNEWS);
	show_bool(16,76, usrconfig.ieFILE);
	show_bool(17,76, usrconfig.OL_ExtInfo);
	show_bool(18,76, usrconfig.Email);
}



/*
 * Edit one record, return -1 if there are errors, 0 if ok.
 */
int EditUsrRec(int Area)
{
	FILE	*fil;
	char	mfile[81];
	long	offset;
	int	j = 0;
	unsigned long crc, crc1;
	char	temp[81];

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
		j = select_menu(36);
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
		
		case 1:	E_STR( 6,17,35,usrconfig.sUserName,"The ^First and Last name^ of this user")
		case 2:	E_STR( 7,17,35,usrconfig.sHandle,  "The ^Handle^ of this user")
		case 3:	E_STR( 8,17,27,usrconfig.sLocation,"The users ^Location^")
		case 4:
		case 5:
		case 6:	E_STR(j+5,17,40,usrconfig.address[j-4],"^Address^")
		case 7:	E_STR(12,17,16, usrconfig.sVoicePhone, "The ^Voice Phone^ number of this user")
		case 8:	E_STR(13,17,16, usrconfig.sDataPhone,  "The ^Data Phone^ number of this user")
		case 9:	E_USEC(14,17,    usrconfig.Security,    "15.9   EDIT USER SECURITY", Screen1)
			break;
		case 10:E_STR(15,17,10, usrconfig.sDateOfBirth,"The ^Date of Birth^ in DD-MM-YYYY format")
		case 11:E_STR(16,17,10, usrconfig.sExpiryDate, "The ^Expiry Date^ in DD-MM-YYYY format, 00-00-0000 is no expire")
		case 12:E_INT(17,17, usrconfig.ExpirySec.level,"The ^Expiry Level^ for this user")
		case 13:E_STR(18,17,8,  usrconfig.Name,        "The ^Unix username^ for this user")
		case 14:E_STR(19,17,62, usrconfig.sComment,    "A ^Comment^ for this user")
		case 15:strcpy(temp,  edit_str(12,50,12, usrconfig.Password, (char *)"Enter the ^password^ for this user"));
			if (strlen(temp)) {
				memset(&usrconfig.Password, 0, sizeof(usrconfig.Password));
				strcpy(usrconfig.Password, temp);
				usrconfig.iPassword = StringCRC32(tu(temp));
			} else {
				working(2, 0, 0);
				working(0, 0, 0);
			}
			break;
		case 16:strcpy(usrconfig.sSex, tl(edit_str(13,50,7, usrconfig.sSex, (char *)"^Male^ or ^Female^")));
			break;
		case 17:E_INT(14,50, usrconfig.Credit, "Users ^Credit^")
		case 18:strcpy(temp, PickProtocol(15));
			if (strlen(temp) != 0)
				strcpy(usrconfig.sProtocol, temp);
			clr_index();
			Screen1();
			Fields1();
			break;
		case 19:strcpy(temp, PickArchive((char *)"15"));
			if (strlen(temp) != 0)
				strcpy(usrconfig.Archiver, temp);
			clr_index();
			Screen1();
			Fields1();
			break;
		case 20:E_BOOL(17,50,usrconfig.Hidden,       "Is user ^hidden^ on the BBS")
		case 21:E_BOOL(18,50,usrconfig.HotKeys,      "Is user using ^HotKeys^ for menus")
		case 22:E_BOOL( 4,76,usrconfig.GraphMode,    "Is user using ^ANSI^ colors")
		case 23:E_BOOL( 5,76,usrconfig.Deleted,      "Is user marked for ^deletion^")
		case 24:E_BOOL( 6,76,usrconfig.NeverDelete,  "^Never delete^ this user")
		case 25:E_BOOL( 7,76,usrconfig.Chat,         "User has ^IEMSI Chat^ capability")
		case 26:E_BOOL( 8,76,usrconfig.LockedOut,    "User is ^Locked Out^ of this BBS")
		case 27:E_BOOL( 9,76,usrconfig.DoNotDisturb, "User will not be ^disturbed^")
		case 28:E_BOOL(10,76,usrconfig.Cls,          "Send ^ClearScreen code^ to users terminal")
		case 29:E_BOOL(11,76,usrconfig.More,         "User uses the ^More prompt^")
		case 30:E_BOOL(12,76,usrconfig.FsMsged,      "User uses the ^Fullscreen editor^")
		case 31:E_BOOL(13,76,usrconfig.MailScan,     "Don't check for ^new mail^")
		case 32:E_BOOL(14,76,usrconfig.Guest,        "This is a ^Guest^ account")
		case 33:E_BOOL(15,76,usrconfig.ieNEWS,       "Show ^News Bulletins^ when logging in")
		case 34:E_BOOL(16,76,usrconfig.ieFILE,       "Show ^New Files^ when logging in")
		case 35:E_BOOL(17,76,usrconfig.OL_ExtInfo,   "Add ^Extended Message Info^ in OLR download")
		case 36:E_BOOL(18,76,usrconfig.Email,        "User has a ^private email^ mailbox")
		}
	}

	return 0;
}



void EditUsers(void)
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
			CloseUsers();
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


