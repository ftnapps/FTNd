/*****************************************************************************
 *
 * File ..................: mbsebbs/newuser.c
 * Purpose ...............: New User login under Unix, creates both
 *			    BBS and unix accounts.
 * Last modification date : 28-Jun-2001
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
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "funcs4.h"
#include "pwcheck.h"
#include "newuser.h"
#include "language.h"
#include "timeout.h"
#include "change.h"
#include "bye.h"


extern	int	do_quiet;		/* No logging to the screen	*/
extern	pid_t	mypid;			/* Pid of this program		*/
char		UnixName[9];		/* Unix Name			*/
extern	int	ieLogin;		/* IEMSI Login Successfull	*/
extern	int	ieRows;			/* Rows on screen		*/
extern	int	ieHOT;			/* Use Hotkeys			*/
extern	char	*ieHandle;		/* Users Handle			*/
extern	char	*ieLocation;		/* Users Location		*/
extern	char	*Passwd;		/* Plain password		*/


int newuser(char *FullName)
{
	FILE		*pUsrConfig;
	int		i, x, Found, iLang, recno = 0;
	unsigned long	crc;
	char		temp[PATH_MAX];
	char		*temp1, *temp2;
	char		*Phone1, *Phone2;
	long		offset;
	struct userrec	us;

	IsDoing("New user login");
	Syslog('+', "Newuser registration");
	clear();
	iLang = Chg_Language(TRUE);

	Enter(1);
	/* MBSE BBS - NEW USER REGISTRATION */
	language(3, 0, 37);
	Enter(2);

	Syslog('+', "Name entered: %s", FullName);

	memset(&usrconfig, 0, sizeof(usrconfig));
	memset(&exitinfo, 0, sizeof(exitinfo));

	temp1  = calloc(81, sizeof(char));
	temp2  = calloc(81, sizeof(char));
	Phone1 = calloc(81, sizeof(char));
	Phone2 = calloc(81, sizeof(char));

	usrconfig.iLanguage = iLang;
	usrconfig.FsMsged = TRUE;

	while (TRUE) {
		do {
			alarm_on();
			Enter(1);
			/* Use this name: */
			language(14, 0, 38); 
			printf("%s [Y/n]? ", FullName);
			fflush(stdout);
			fflush(stdin);
			GetstrC(temp, 80);

			if ((strcasecmp(temp, "y") == 0) || (strcmp(temp, "") == 0))
				sprintf(temp, "%s", FullName);
			else {
				do {
					Syslog('+', "User chose to use a different name");
					Enter(1);
					/* Please enter your First and Last name: */
					language(3, 0, 0);
					fflush(stdout);
					alarm_on();
					Getname(temp, 35);
					if (CheckName(temp))
						printf("\n%s\n", (char *) Language(149));
					/*
				 	 * Do a check to see if name exists
					 */
				} while ((CheckName(temp) || strchr(temp, ' ') == NULL));
			}
		} while (BadNames(temp) || *(temp) == '\n');

		/*
		 * Used to get users full name for other functions
		 */
		strcpy(FullName, tlcap(temp));
		UserCity(mypid, FullName, (char *)"Unknown");

		while (1) {
			Enter(1);
			/* Please enter your BBS password, this can be the same as the unix password */
			printf("%s\n\n", (char *) Language(388));
			/* Please enter new password   : */
			language(11, 0, 39);
			fflush(stdout);
			alarm_on();
		  	Getpass(temp1);
			if((x = strlen(temp1)) >= CFG.password_length) {
				Enter(1);
				/* Please enter password again : */
				language(11, 0, 40);
				fflush(stdout);
				alarm_on();
			  	Getpass(temp2);
				if((i = strcmp(temp1,temp2)) != 0) {
					Enter(2);
					/* Your passwords do not match! Try again. */
					language(12, 0, 41);
					Enter(1);
				} else {
					crc = StringCRC32(tu(temp1));
					break;
		  		}
		  	} else {
				Enter(2);
				/* Your password must contain at least */
				language(12, 0, 42);
		  		printf("%d ", CFG.password_length);
				/* characters! Try again. */
				language(15, 0, 43);
				Enter(1);
			}
		}
		memset(Passwd, 0, 16);
		sprintf(Passwd, "%s", temp2);
		memset(&usrconfig.Password, 0, sizeof(usrconfig.Password));
		sprintf(usrconfig.Password, "%s", temp2);
		usrconfig.iPassword = crc;
		alarm_on();
		sprintf(UnixName, "%s", (char *) NameCreate(NameGen(FullName), FullName, temp2));
		break;
	}

	strcpy(usrconfig.sUserName, FullName);
	strcpy(usrconfig.Name, UnixName);
	Time_Now = time(NULL);
	l_date = localtime(&Time_Now);
	ltime = time(NULL);

	if(CFG.iAnsi) {
		Enter(2);
		/* Do you want ANSI and graphics mode [Y/n]: */
		language(7, 0, 44);

		alarm_on();
 		i = toupper(getchar());

		if (i == Keystroke(44, 0) || i == '\n')
			usrconfig.GraphMode = TRUE;
		else
			usrconfig.GraphMode = FALSE;
	} else {
		usrconfig.GraphMode = TRUE; /* Default set it to ANSI */
		Enter(1);
	}
	exitinfo.GraphMode = usrconfig.GraphMode;
	TermInit(exitinfo.GraphMode);

	if (CFG.iVoicePhone) {
		while (1) {
			Enter(1);
			/* Please enter you Voice Number */
			language(10, 0, 45);
			Enter(1);

			pout(10, 0, (char *)": ");
			colour(CFG.InputColourF, CFG.InputColourB);
			fflush(stdout);
			alarm_on();
			GetPhone(temp, 16);

			if (strlen(temp) < 6) {
				Enter(1);
				language(12, 0, 47);
				Enter(1);
			} else {
				strcpy(usrconfig.sVoicePhone, temp);
				strcpy(Phone1, temp);
				break;
			}
		}
	} /* End of first if statement */

	if (CFG.iDataPhone) {
		while (TRUE) {
			Enter(1);
			/* Please enter you Data Number */
			language(10, 0, 48);
			Enter(1);

			pout(10, 0, (char *)": ");
			colour(CFG.InputColourF, CFG.InputColourB);
			alarm_on();
			GetPhone(temp, 16);

			/*
			 * If no dataphone, copy voicephone.
			 */
			if (strcmp(temp, "") == 0) {
				strcpy(usrconfig.sDataPhone, usrconfig.sVoicePhone);
				break;
			}

			if( strlen(temp) < 6) {
				Enter(1);
				/* Please enter a proper phone number */
				language(12, 0, 47);
				Enter(1);
			} else {
				strcpy(usrconfig.sDataPhone, temp);
				strcpy(Phone2, temp);
				break;
			}
		}
	} /* End of if Statement */

	if(!CFG.iDataPhone)
		printf("\n");

	if (ieLogin && (strlen(ieLocation) >= CFG.CityLen) && (strlen(ieLocation) < 24)) {
		strcpy(usrconfig.sLocation, ieLocation);
	} else {
		while (TRUE) {
			Enter(1);
			/* Enter your location */
			pout(14, 0, (char *) Language(49));
			colour(CFG.InputColourF, CFG.InputColourB);
			alarm_on();
			if (CFG.iCapLocation) { /* Cap Location is turn on, Capitalise first letter */
				fflush(stdout);
				GetnameNE(temp, 24);
			} else
				GetstrC(temp, 80);
	
			if( strlen(temp) < CFG.CityLen) {
				Enter(1);
				/* Please enter a longer location */
				language(12, 0, 50);
				Enter(1);
				printf("%s%d)", (char *) Language(74), CFG.CityLen);
				Enter(1);
			} else {
				strcpy(usrconfig.sLocation, temp);
				UserCity(mypid, FullName, temp);
				break;
			}
		}
	}

	if(CFG.iHandle) {
		Enter(1);
		/* Enter a handle (Enter to Quit): */
		pout(12, 0, (char *) Language(412));
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		alarm_on();
		Getname(temp, 34);

		if(strcmp(temp, "") == 0)
			strcpy(usrconfig.sHandle, "");
		else
			strcpy(usrconfig.sHandle, temp);
	}

	/*
	 * Note, the users database always contains the english sex
	 */
	if(CFG.iSex) {
		while (TRUE) {
			Enter(1);
			/* What is your sex? (M)ale or (F)emale: */
			language(9, 0, 51);
			colour(CFG.InputColourF, CFG.InputColourB);
			fflush(stdout);
			alarm_on();
		 	i = toupper(Getone());

			if (i == Keystroke(51, 0)) {
				/* Male */
				sprintf(usrconfig.sSex, "Male");
				pout(CFG.InputColourF, CFG.InputColourB, (char *) Language(52));
				Enter(1);
				break;
			} else
				if (i == Keystroke(51, 1)) {
					/* Female */
					sprintf(usrconfig.sSex, "Female");
					pout(CFG.InputColourF, CFG.InputColourB, (char *) Language(53));
					Enter(1);
					break;
				} else {
					Enter(2);
					/* Please answer M or F */
					language(12, 0, 54);
					Enter(1);
				}
		}
	} else /* End of if Statement */
		sprintf(usrconfig.sSex, "Unknown"); /* If set off, set to Unknown */

	while (TRUE) {
		Enter(1);
		/* Please enter your Date of Birth DD-MM-YYYY: */
		pout(3, 0, (char *) Language(56));
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		alarm_on();
		GetDate(temp, 10);

		sprintf(temp1, "%c%c%c%c", temp[6], temp[7], temp[8], temp[9]);
		sprintf(temp2, "%02d", l_date->tm_year);
		iLang = atoi(temp2) + 1900;
		sprintf(temp2, "%04d", iLang);

		Syslog('-', "DOB: test %s %s", temp1, temp2);

		if ((strcmp(temp1,temp2)) == 0) {
			Enter(1);
			/* Sorry you entered this year by mistake. */
			pout(12, 0, (char *) Language(57));
			Enter(1);
		} else
			if((strlen(temp)) != 10) {
				Enter(1);
				/* Please enter the correct date format */
				pout(12, 0, (char *) Language(58));
				Enter(1);
			} else {
				strcpy(usrconfig.sDateOfBirth,temp);
				break;
			}
	}

	usrconfig.tFirstLoginDate = ltime; /* Set first login date to current date */
	usrconfig.tLastLoginDate = (time_t)0; /* To force setting new limits */
	strcpy(usrconfig.sExpiryDate,"00-00-0000");
	usrconfig.ExpirySec = CFG.newuser_access;
	usrconfig.Security = CFG.newuser_access;
	usrconfig.Email = CFG.GiveEmail;

	if (ieLogin) 
		usrconfig.HotKeys = ieHOT;
	else {
		if (CFG.iHotkeys) {
			while (TRUE) {
				Enter(1);
				/* Would you like hot-keyed menus [Y/n]: */
				pout(12, 0, (char *) Language(62));
				colour(CFG.InputColourF, CFG.InputColourB);
				alarm_on();
			 	GetstrC(temp, 8);
	
				if ((toupper(temp[0]) == Keystroke(62, 0)) || (strcmp(temp,"") == 0)) {
					usrconfig.HotKeys = TRUE;
					break;
				}
				if (toupper(temp[0]) == Keystroke(62, 1)) {
					usrconfig.HotKeys = FALSE;
					break;
				} else {
					/* Please answer Y or N */
					pout(15, 0, (char *) Language(63));
				}
			}
		} /* End of if Statement */
		else
			usrconfig.HotKeys = TRUE; /* Default set it to Hotkeys */
	}

	usrconfig.iTimeLeft    = 20;  /* Set Timeleft in users file to 20 */

	Enter(1);
	if (ieLogin)
		usrconfig.iScreenLen = ieRows;
	else {
		/* Please enter your Screen Length [24]: */
		pout(13, 0, (char *) Language(64));
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		alarm_on();
		Getnum(temp, 3);

		if(strlen(temp) == 0)
			usrconfig.iScreenLen = 24;
		else
			usrconfig.iScreenLen = atoi(temp);
	}

	alarm_on();

	usrconfig.tLastPwdChange  = ltime; /* Days Since Last Password Change */
	usrconfig.iLastFileArea   = 1;

	sprintf(usrconfig.sProtocol, "%s", (char *) Language(65));
	usrconfig.DoNotDisturb = FALSE;
	usrconfig.MailScan     = TRUE;
	usrconfig.ieFILE       = TRUE;
	usrconfig.ieNEWS       = TRUE;
	usrconfig.Cls          = TRUE;
	usrconfig.More         = TRUE;
	usrconfig.ieASCII8     = TRUE;

	/*
	 * Search a free slot in the users datafile
	 */
	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((pUsrConfig = fopen(temp, "r+")) == NULL) {
		WriteError("Can't open file: %s", temp);
		ExitClient(1);
	}

	fread(&usrconfighdr, sizeof(usrconfighdr), 1, pUsrConfig);
	offset = ftell(pUsrConfig);
	Found = FALSE;

	while ((fread(&us, usrconfighdr.recsize, 1, pUsrConfig) == 1) && (!Found)) {
		if (us.sUserName[0] == '\0') {
			Found = TRUE;
		} else {
			offset = ftell(pUsrConfig);
			recno++;
		}
	}
	
	if (Found)
		fseek(pUsrConfig, offset, SEEK_SET);
	else
		fseek(pUsrConfig, 0, SEEK_END);

	fwrite(&usrconfig, sizeof(usrconfig), 1, pUsrConfig);
	fclose(pUsrConfig);

	Enter(2);
	/* Your user account has been created: */
	pout(14, 0, (char *) Language(67));
	Enter(2);

	/* Login Name : */
	pout(9, 0, (char *) Language(68));
	colour(11, 0);
	printf("%s (%s)\n", FullName, UnixName);
	/* Password   : */
	pout(9, 0, (char *) Language(69));
	pout(3, 0, (char *)"<");
	/* not displayed */
	pout(15, 0, (char *) Language(70));
	pout(3, 0, (char *)">\n\n");
	fflush(stdout);
	fflush(stdin);
 
	if(CFG.iVoicePhone) {
	  	if(TelephoneScan(Phone1, FullName))
	  		Syslog('!', "Duplicate phone numbers found");
  	}

	if(CFG.iDataPhone) {
		if(TelephoneScan(Phone2, FullName))
			Syslog('!', "Duplicate phone numbers found");
	}
 
	free(temp1);
	free(temp2);
	free(Phone1);
	free(Phone2);

	Syslog('+', "Completed new-user procedure");
	/* New user registration completed. */
	pout(10, 0, (char *) Language(71));
	Enter(2);
	alarm_on();
	Pause();
	alarm_off();
	printf("\n");
	return recno;
}


