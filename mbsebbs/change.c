/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Change user settings
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
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "change.h"
#include "dispfile.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "misc.h"
#include "timeout.h"
#include "exitinfo.h"
#include "bye.h"


int Chg_Language(int NewMode)
{
	FILE	*pLang;
	int	iLang, iFoundLang = FALSE;
	char	*temp;

	temp = calloc(PATH_MAX, sizeof(char));

	if (!NewMode)
		ReadExitinfo();

	while(TRUE) {
		sprintf(temp, "%s/etc/language.data", getenv("MBSE_ROOT"));
		if(( pLang = fopen(temp, "r")) == NULL) {
			WriteError("$Can't open %s", temp);
			printf("\nFATAL: Can't open language file\n\n");
			Pause();
			free(temp);
			return 0;
		}
		fread(&langhdr, sizeof(langhdr), 1, pLang);

		colour(CFG.HiliteF, CFG.HiliteB);
		/* Select your preferred language */
		printf("\n%s\n\n", (char *) Language(378));

		iLang = 6;
		colour(9,0);
		while (fread(&lang, langhdr.recsize, 1, pLang) == 1)
			if (lang.Available) {
				colour(13, 0);
			   	printf("(%s)", lang.LangKey);
   				colour(8,0);
   				printf(" %c ", 46);
   				colour(3,0);
		   		printf("%-29s    ", lang.Name);

				iLang++;
				if ((iLang % 2) == 0)
					printf("\n");
			}
		Enter(1);

		colour(CFG.HiliteF, CFG.HiliteB);
		/* Select language: */
		printf("\n%s", (char *) Language(379));

		fflush(stdout);
		alarm_on();
		iLang = toupper(Getone());

		printf("%c", iLang);

		fseek(pLang, langhdr.hdrsize, 0);

		while (fread(&lang, langhdr.recsize, 1, pLang) == 1) {
			strcpy(lang.LangKey,tu(lang.LangKey));
			if ((lang.LangKey[0] == iLang) && (lang.Available)) {
				strcpy(CFG.current_language, lang.Filename);
				iFoundLang = TRUE;
				break;
			}
		}
	
		fclose(pLang);

		if(!iFoundLang) {
			Enter(2);
			/* Invalid selection, please try again! */
			pout(10, 0, (char *) Language(265));
			Enter(2);
		} else {
			exitinfo.iLanguage = iLang;
			strcpy(CFG.current_language, lang.Filename);
			Free_Language();
			InitLanguage();

			colour(10, 0);
			/* Language now set to" */
			printf("\n\n%s%s\n\n", (char *) Language(380), lang.Name);

			if (!NewMode) {
				Syslog('+', "Changed language to %s", lang.Name);
				WriteExitinfo();
				Pause();
			}
			break;
		}
	}

	free(temp);
	Enter(1);
	return iLang;
}



void Chg_Password()
{
	char	*temp1, *temp2;

  	temp1 = calloc(PATH_MAX, sizeof(char));
	temp2 = calloc(PATH_MAX, sizeof(char));

	ReadExitinfo();
	DisplayFile((char *)"password");

	Enter(1);
	/* Old password: */
	language(15, 0, 120);
	fflush(stdout);
	colour(CFG.InputColourF, CFG.InputColourB);
	Getpass(temp1);

	if (!strcmp(exitinfo.Password, temp1)) {
		while (TRUE) {
			Enter(1);
			/* New password: */
			language(9, 0, 121);
			fflush(stdout);
			colour(CFG.InputColourF, CFG.InputColourB);
			Getpass(temp1);
			if((strlen(temp1)) >= CFG.password_length) {
				Enter(1);
				/* Confirm new password: */
				language(9, 0, 122);
				colour(CFG.InputColourF, CFG.InputColourB);
				fflush(stdout);
				Getpass(temp2);
				if(( strcmp(temp1,temp2)) != 0) {
					/* Passwords do not match! */
					Enter(2);
					language(12, 0, 123);
					Enter(1);
				} else {
					fflush(stdout);
					fflush(stdin);
					break;
				}
			} else {
				colour(12, 0);
				/* Your password must contain at least %d characters! Try again.*/
				printf("\n%s%d %s\n\n", (char *) Language(42), CFG.password_length, (char *) Language(43));
			}
		}

		Syslog('+', "%s/bin/mbpasswd -n %s ******", getenv("MBSE_ROOT"), exitinfo.Name);
		sprintf(temp1, "%s/bin/mbpasswd -n %s %s", getenv("MBSE_ROOT"), exitinfo.Name, temp2);
		if (system(temp1) != 0) {
			WriteError("Failed to set new Unix password");
		} else {
			memset(&exitinfo.Password, 0, sizeof(exitinfo.Password));
			strcpy(exitinfo.Password, temp2);
			exitinfo.tLastPwdChange = time(NULL);
			Enter(1);
			/* Password Change Successful */
			language(10, 0, 124);
			Syslog('+', "User changed his password");
			WriteExitinfo();
		}
	} else {
		Enter(1);
		/* Old password incorrect! */
		language(12, 0, 125);
	}

	free(temp1);
	free(temp2);
	Enter(2);
	Pause();
}



/*
 * Function to check if User Handle exists and returns a 0 or 1
 */
int CheckHandle(char *);
int CheckHandle(char *Name)
{
    FILE    *fp;
    int     Status = FALSE;
    struct  userhdr uhdr;
    struct  userrec u;
    char    *temp;

    temp = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((fp = fopen(temp,"rb")) != NULL) {
        fread(&uhdr, sizeof(uhdr), 1, fp);

        while (fread(&u, uhdr.recsize, 1, fp) == 1) {
            if ((strcasecmp(u.sHandle, Name)) == 0) {
                Status = TRUE;
                break;
            }
        }
        fclose(fp);
    }

    free(temp);
    return Status;
}



/*
 * Function will allow a user to change his handle
 */
void Chg_Handle()
{
    char    *Handle, *temp;

    Handle = calloc(81, sizeof(char));
    temp   = calloc(81, sizeof(char));

    ReadExitinfo();
    Syslog('+', "Old handle \"%s\"", exitinfo.sHandle);

    while (TRUE) {
	Enter(1);
	/* Enter a handle (Enter to Quit): */
	pout(9, 0, (char *) Language(412));
	colour(CFG.InputColourF, CFG.InputColourB);
	fflush(stdout);
	Getname(temp, 34);

	if ((strcmp(temp, "")) == 0) {
	    free(Handle);
	    free(temp);
	    return;
	}
	strcpy(Handle, tlcap(temp));

	if (CheckHandle(Handle) || CheckUnixNames(Handle)) {
	    pout(12, 0, (char *)"\nThat handle is already been used\n");
	} else if (CheckName(Handle)) {
	    pout(12, 0, (char *)"\nThat name is already been used\n");
	} else if((strcmp(Handle, "sysop")) == 0) {
	    pout(12, 0, (char *)"\nYou cannot use Sysop as a handle\n");
	} else if(strcmp(temp, "") != 0) {
	    Setup(exitinfo.sHandle, temp);
	    pout(10, 0, (char *)"\nHandle Changed!\n\n");
	    Syslog('+', "New handle \"%s\"", exitinfo.sHandle);
	    break;
	}
    }

    WriteExitinfo();
    free(temp);
    free(Handle);
}



/*
 * Toggle hotkeys
 */
void Chg_Hotkeys()
{
	ReadExitinfo();
	Enter(2);

	if (exitinfo.HotKeys) {
		exitinfo.HotKeys = FALSE;
		/* Hotkeys are now OFF */
		pout(10, 0, (char *) Language(146));
	} else {
		exitinfo.HotKeys = TRUE;
		/* Hotkeys are now ON */
		pout(10, 0, (char *) Language(145));
	}

	Enter(2);
	sleep(2);
	Syslog('+', "Hotkeys changed to %s", exitinfo.HotKeys?"True":"False");
	WriteExitinfo();
}



/*
 * Toggle Mail Check
 */
void Chg_MailCheck()
{
	ReadExitinfo();
	Enter(2);

	if (exitinfo.MailScan) {
		exitinfo.MailScan = FALSE;
		/* New Mail check is now OFF */
		pout(10, 0, (char *) Language(367));
	} else {
		exitinfo.MailScan = TRUE;
		/* New Mail check is now ON */
		pout(10, 0, (char *) Language(366));
	}

	Enter(2);
	sleep(2);
	Syslog('+', "New Mail Check changed to %s", exitinfo.MailScan ?"True":"False");
	WriteExitinfo();
}



/*
 * Toggle New Files Check
 */
void Chg_FileCheck()
{
	ReadExitinfo();
	Enter(2);

	if (exitinfo.ieFILE) {
		exitinfo.ieFILE = FALSE;
		/* New Files check is now OFF */
		pout(10, 0, (char *) Language(371));
	} else {
		exitinfo.ieFILE = TRUE;
		/* New Files check is now ON */
		pout(10, 0, (char *) Language(370));
	}

	Enter(2);
	sleep(2);
	Syslog('+', "Check New Files changed to %s", exitinfo.ieFILE ?"True":"False");
	WriteExitinfo();
}



/*
 * Choose Message Editor
 */
void Chg_FsMsged()
{
    int	    z;

    ReadExitinfo();
    Enter(2);

    /*                               Now using the */
    pout(LIGHTMAGENTA, BLACK, (char *)Language(372));
    /*                 Line/Fullscreen/External    */
    colour(LIGHTCYAN, BLACK);
    printf(" %s ", Language(387 + (exitinfo.MsgEditor & 3)));
    /*                                      Editor */
    pout(LIGHTMAGENTA, BLACK, (char *)Language(390));
    Enter(1);

    if (strlen(CFG.externaleditor))
	/* Select: 1) Line editor, 2) Fullscreen editor, 3) External editor */
	pout(WHITE, BLACK, (char *)Language(373));
    else
	/* Select: 1) Line editor, 2) Fullscreen editor */
	pout(WHITE, BLACK, (char *)Language(438));
    fflush(stdout);
    alarm_on();
    z = toupper(Getone());

    if (z == Keystroke(373, 0)) {
	exitinfo.MsgEditor = LINEEDIT;
	Syslog('+', "User selected line editor");
    } else if (z == Keystroke(373, 1)) {
	exitinfo.MsgEditor = FSEDIT;
	Syslog('+', "User selected fullscreen editor");
    } else if ((z == Keystroke(373, 2) && strlen(CFG.externaleditor))) {
	exitinfo.MsgEditor = EXTEDIT;
	Syslog('+', "User selected external editor");
    }

    Enter(2);

    /*                               Now using the */
    pout(LIGHTMAGENTA, BLACK, (char *)Language(372));
    /*                 Line/Fullscreen/External    */
    colour(LIGHTCYAN, BLACK);
    printf(" %s ", Language(387 + (exitinfo.MsgEditor & 3)));
    /*                                      Editor */
    pout(LIGHTMAGENTA, BLACK, (char *)Language(390));

    Enter(2);
    sleep(2);
    WriteExitinfo();
}



/*
 * Toggle Fullscreen Editor Shotcut keys
 */
void Chg_FsMsgedKeys()
{
    ReadExitinfo();
    Enter(2);

    if (exitinfo.FSemacs) {
	exitinfo.FSemacs = FALSE;
	/* Fullscreen Editor shortcut keys set to Wordstar */
	pout(10, 0, (char *) Language(473));
    } else {
	exitinfo.FSemacs = TRUE;
	/* Fullscreen Editor shortcut keys set to Emacs */
	pout(10, 0, (char *) Language(472));
    }
    Enter(2);
    sleep(2);
    Syslog('+', "FS editor shortcut keys changed to %s", exitinfo.FSemacs?"Emacs":"Wordstar");
    WriteExitinfo();
}



/*
 * Function to toggle DoNotDisturb Flag
 */
void Chg_Disturb()
{
	ReadExitinfo();
	colour(10, 0);

	if(exitinfo.DoNotDisturb) {
		exitinfo.DoNotDisturb = FALSE;
		/* Do not disturb turned OFF */
		printf("\n%s\n", (char *) Language(416));
	} else {
		exitinfo.DoNotDisturb = TRUE;
		/* Do not disturb turned ON */
		printf("\n%s\n", (char *) Language(417));
	}

	Syslog('+', "Do not disturb now %s", exitinfo.DoNotDisturb?"True":"False");
	UserSilent(exitinfo.DoNotDisturb);
	sleep(2);
	WriteExitinfo();
}



void Chg_Location()
{
	char	temp[81];

	ReadExitinfo();
	Syslog('+', "Old location \"%s\"", exitinfo.sLocation);

	while (TRUE) {
		/* Old Location: */
		Enter(1);
		/* Old location: */
		pout(15, 0, (char *) Language(73));
		colour(9, 0);
		printf("%s\n", exitinfo.sLocation);
		Enter(1);
		/* Please enter your location: */
		pout(14, 0, (char *) Language(49));

		if(CFG.iCapLocation) {
			colour(CFG.InputColourF, CFG.InputColourB);
			fflush(stdout);
			GetnameNE(temp, 24);
		} else {
			colour(CFG.InputColourF, CFG.InputColourB);      			
			GetstrC(temp, 80);
		}

		if((strcmp(temp, "")) == 0)
			break;

		if(( strlen(temp)) < CFG.CityLen) {
			Enter(1);
			/* Please enter a longer location (min */
			colour(12, 0);
			printf("%s%d)", (char *) Language(74), CFG.CityLen);
			Enter(1);
		} else {
			Setup(exitinfo.sLocation,temp);
			break;
		}
	}

	Syslog('+', "New location \"%s\"", exitinfo.sLocation);
	WriteExitinfo();
}



void Chg_Address()
{
    int	    i;
    char    temp[41];
    
    ReadExitinfo();
    Syslog('+', "Old address \"%s\"", exitinfo.address[0]);
    Syslog('+', "            \"%s\"", exitinfo.address[1]);
    Syslog('+', "            \"%s\"", exitinfo.address[2]);

    while (TRUE) {
	Enter(1);
	/* Old address: */
	pout(WHITE, BLACK, (char *) Language(476));
	Enter(1);
	colour(LIGHTBLUE, BLACK);
	printf("%s\n", exitinfo.address[0]);
	printf("%s\n", exitinfo.address[1]);
	printf("%s\n", exitinfo.address[2]);
	Enter(1);
	/* Your address, maximum 3 lines (only visible for the sysop): */
	pout(YELLOW, BLACK, (char *) Language(474));
	Enter(1);

	for (i = 0; i < 3; i++ ) {
	    colour(YELLOW, BLACK);
	    printf("%d: ", i+1);
	    colour(CFG.InputColourF, CFG.InputColourB);
	    fflush(stdout);
	    alarm_on();
	    GetstrC(temp, 40);
	    if (strcmp(temp, ""))
		Setup(exitinfo.address[i], temp);
	}

	if (strlen(exitinfo.address[0]) || strlen(exitinfo.address[1]) || strlen(exitinfo.address[2]))
	    break;

	Enter(1);
	/* You need to enter your address here */
	pout(LIGHTRED, BLACK, (char *)Language(475));
	Enter(1);
    }

    Syslog('+', "New address \"%s\"", exitinfo.address[0]);
    Syslog('+', "            \"%s\"", exitinfo.address[1]);
    Syslog('+', "            \"%s\"", exitinfo.address[2]);
    WriteExitinfo();
}



/*
 * Toggle Graphics
 */
void Chg_Graphics()
{
	ReadExitinfo();
	Enter(2);

	if (exitinfo.GraphMode) {
		exitinfo.GraphMode = FALSE;
		/* Ansi Mode turned OFF */
		pout(15, 0, (char *) Language(76));
	} else {
		exitinfo.GraphMode = TRUE;
		/* Ansi Mode turned ON */
		pout(15, 0, (char *) Language(75));
	}

	Syslog('+', "Graphics mode now %s", exitinfo.GraphMode?"On":"Off");
	Enter(2);
	TermInit(exitinfo.GraphMode);
	WriteExitinfo();
	sleep(2);
}



void Chg_VoicePhone()
{
	char	temp[81];

	ReadExitinfo();
	Syslog('+', "Old voice phone \"%s\"", exitinfo.sVoicePhone);

	while (TRUE) {
		Enter(1);
		/* Please enter you Voice Number */
		pout(10, 0, (char *) Language(45));
		Enter(1);
		pout(10, 0, (char *)": ");
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		GetPhone(temp, 16);

		if (strlen(temp) < 6) {
			Enter(1);
			/* Please enter a proper phone number */
			pout(12, 0, (char *) Language(47));
			Enter(1);
		} else {
			strcpy(exitinfo.sVoicePhone, temp);
			break;
		}
	}

	Syslog('+', "New voice phone \"%s\"", exitinfo.sVoicePhone);
	WriteExitinfo();
}



void Chg_DataPhone()
{
	char	temp[81];

	ReadExitinfo();
	Syslog('+', "Old data phone \"%s\"", exitinfo.sDataPhone);

	while (1) {
		Enter(1);
		/* Please enter you Data Number */
		pout(10, 0, (char *) Language(48));
		Enter(1);
		pout(10, 0, (char *)": ");
		colour(CFG.InputColourF, CFG.InputColourB);
		GetPhone(temp, 16);

		if( strlen(temp) < 6) {
			Enter(1);
			/* Please enter a proper phone number */
			pout(12, 0, (char *) Language(47));
			Enter(1);
		} else {
			strcpy(exitinfo.sDataPhone, temp);
			break;
		}
	}

	Syslog('+', "New data phone \"%s\"", exitinfo.sDataPhone);
	WriteExitinfo();
}



void Chg_News()
{
	ReadExitinfo();

	if (exitinfo.ieNEWS) {
		exitinfo.ieNEWS = FALSE;
		/* News bulletins turned OFF */
		printf("\n\n%s\n\n", (char *) Language(79));
	} else {
		exitinfo.ieNEWS = TRUE;
		/* News bulletins turned ON */
		printf("\n\n%s\n\n", (char *) Language(78));
	}

	Syslog('+', "News bullentins now %s", exitinfo.ieNEWS?"True":"False");
	sleep(2);
	WriteExitinfo();
}



void Chg_ScreenLen()
{
	char	*temp;

	ReadExitinfo();
	temp = calloc(81, sizeof(char));
	Syslog('+', "Old screenlen %d", exitinfo.iScreenLen);
	fflush(stdin);

	Enter(1);
	/* Please enter your Screen Length? [24]: */
	pout(13, 0, (char *) Language(64));
	colour(CFG.InputColourF, CFG.InputColourB);
	fflush(stdout);
	Getnum(temp, 2);

	if((strcmp(temp, "")) == 0) {
		exitinfo.iScreenLen = 24;
		printf("\n%s\n\n", (char *) Language(80));
	} else {
		exitinfo.iScreenLen = atoi(temp);
		printf("\n%s%d\n\n", (char *) Language(81), exitinfo.iScreenLen);
	}

	Syslog('+', "New screenlen %d", exitinfo.iScreenLen);
	WriteExitinfo();
	Pause();
	free(temp);
}



/*
 * Check users Date of Birth, if it is ok, we calculate his age.
 */
int Test_DOB(char *DOB)
{
	int	tyear, year, month, day;
	char	temp[40], temp1[40];

	/*
	 * If Ask Date of Birth is off, assume users age is
	 * zero, and this check is ok.
	 */
	if (!CFG.iDOB) {
	    UserAge = 0;
	    return TRUE;
	}

	/*
	 *  First check length of string 
	 */
	if (strlen(DOB) != 10) {
		Syslog('!', "Date format length %d characters", strlen(DOB));
		/* Please enter the correct date format */
		language(14, 0, 83);
		return FALSE;
	}
	/*
	 * Split the date into pieces
	 */
	strcpy(temp1, DOB);
	strcpy(temp, strtok(temp1, "-"));
	day = atoi(temp);
	strcpy(temp, strtok(NULL, "-"));
	month = atoi(temp);
	strcpy(temp, strtok(NULL, ""));
	year = atoi(temp);
	tyear = l_date->tm_year + 1900;

	if (((tyear - year) < 10) || ((tyear - year) > 95)) {
		Syslog('!', "DOB: Year error: %d", tyear - year);
		return FALSE;
	}
	if ((month < 1) || (month > 12)) {
		Syslog('!', "DOB: Month error: %d", month);
		return FALSE;
	}
	if ((day < 1) || (day > 31)) {
		Syslog('!', "DOB: Day error: %d", day);
		return FALSE;
	}

	UserAge = tyear - year;
	if ((l_date->tm_mon + 1) < month)
		UserAge--;
	if (((l_date->tm_mon + 1) == month) && (l_date->tm_mday < day))
		UserAge--; 
	Syslog('B', "DOB: Users age %d year", UserAge);
	return TRUE;
}



void Chg_DOB()
{
	char	*temp;

	if (!CFG.iDOB)
	    return;

	temp  = calloc(81, sizeof(char));
	ReadExitinfo();
	Syslog('+', "Old DOB %s", exitinfo.sDateOfBirth);

	while (TRUE) {
		Enter(1);
		/* Please enter your Date of Birth DD-MM-YYYY: */
		pout(3, 0, (char *) Language(56));
		colour(CFG.InputColourF, CFG.InputColourB);
		GetDate(temp, 10);
		if (Test_DOB(temp)) {
			Setup(exitinfo.sDateOfBirth, temp);
			break;
		}
	}

	Syslog('+', "New DOB %s", exitinfo.sDateOfBirth);
	WriteExitinfo();
	free(temp);
}



/*
 * Change default protocol.
 */
void Chg_Protocol()
{
	FILE	*pProtConfig;
	int	iProt, iFoundProt = FALSE;
	int	precno = 0;
	char	*temp;
	char	Prot[2];

	temp = calloc(PATH_MAX, sizeof(char));
	ReadExitinfo();
	Syslog('+', "Old protocol %s", sProtName);

	while(TRUE) {
		sprintf(temp, "%s/etc/protocol.data", getenv("MBSE_ROOT"));
	
		if ((pProtConfig = fopen(temp, "r")) == NULL) {
			WriteError("$Can't open %s", temp);
			/* Protocol: Can't open protocol file. */
			printf("\n%s\n\n", (char *) Language(262));
			Pause();
			free(temp);
			fclose(pProtConfig);
			return;
		}
		fread(&PROThdr, sizeof(PROThdr), 1, pProtConfig);

		colour(CFG.HiliteF, CFG.HiliteB);
		/* Select your preferred protocol */
		printf("\n%s\n\n", (char *) Language(263));

		colour(9,0);
		while (fread(&PROT, PROThdr.recsize, 1, pProtConfig) == 1)
			if (PROT.Available && Access(exitinfo.Security, PROT.Level))
			   	printf("(%s)  %-20s Efficiency %3d %%\n", PROT.ProtKey, PROT.ProtName, PROT.Efficiency);

		colour(CFG.HiliteF, CFG.HiliteB);
		printf("\n%s", (char *) Language(264));

		fflush(stdout);
		alarm_on();
		iProt = toupper(Getone());

		printf("%c", iProt);
		sprintf(Prot, "%c", iProt);

		fseek(pProtConfig, PROThdr.hdrsize, 0);
		while (fread(&PROT, PROThdr.recsize, 1, pProtConfig) == 1) {
			if ((strncmp(PROT.ProtKey, Prot, 1) == 0) &&
			    PROT.Available && Access(exitinfo.Security, PROT.Level)) {
				strcpy(sProtName, PROT.ProtName);
				strcpy(sProtUp, PROT.ProtUp);
				strcpy(sProtDn, PROT.ProtDn);
				strcpy(sProtAdvice, PROT.Advice);
				uProtBatch = PROT.Batch;
				uProtBidir = PROT.Bidir;
				iProtEfficiency = PROT.Efficiency;
				iFoundProt = TRUE;
			} else
				precno++;
		}

		fclose(pProtConfig);

		if (iProt == 13) {
			free(temp);
			return;
		} else 
			if (!iFoundProt) {
				Enter(2);
				pout(10, 0, (char *) Language(265));
				Enter(2);
				/* Loop for new attempt */
			} else {
				Setup(exitinfo.sProtocol, sProtName);
				colour(10,0);
				/* Protocol now set to: */
				printf("\n\n%s%s\n\n", (char *) Language(266), sProtName);
				Pause();
				break;
			}
	}

	Syslog('+', "New protocol %s", sProtName);
	WriteExitinfo();
	free(temp);
}



void Set_Protocol(char *Protocol)
{
	FILE	*pProtConfig;
	int	precno = 0;
	char	*temp;

	temp = calloc(PATH_MAX, sizeof(char));

	sprintf(temp, "%s/etc/protocol.data", getenv("MBSE_ROOT"));

	if(( pProtConfig = fopen(temp, "rb")) == NULL) {
		WriteError("$Can't open %s", temp);
		/* Protocol: Can't open protocol file. */
		printf("\n%s\n\n", (char *) Language(262));
		Pause();
		free(temp);
		return;
	}

	fread(&PROThdr, sizeof(PROThdr), 1, pProtConfig);

	while (fread(&PROT, PROThdr.recsize, 1, pProtConfig) == 1) {
		if ((strcmp(PROT.ProtName, Protocol)) == 0) {
			tlf(sProtName);
			strcpy(sProtName, PROT.ProtName);
			strcpy(sProtUp, PROT.ProtUp);
			strcpy(sProtDn, PROT.ProtDn);
			strcpy(sProtAdvice, PROT.Advice);
			uProtBatch = PROT.Batch;
			uProtBidir = PROT.Bidir;
			iProtEfficiency = PROT.Efficiency;
		} else
			precno++;
	}

	free(temp);
	fclose(pProtConfig);
}



