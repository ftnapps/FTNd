/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Main user login procedure.  Checks for limits, 
 *                          new ratio's cats all the welcome screens, and 
 *                          does a lot of checking in general.
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
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/mberrors.h"
#include "timeout.h"
#include "user.h"
#include "dispfile.h"
#include "funcs.h"
#include "input.h"
#include "misc.h"
#include "bye.h"
#include "file.h"
#include "mail.h"
#include "change.h"
#include "menu.h"
#include "exitinfo.h"
#include "language.h"
#include "offline.h"
#include "email.h"



extern int	sock;
extern pid_t	mypid;
char		*StartTime;


/* 
 * Non global function prototypes
 */
void SwapDate(char *, char *);              /* Swap two Date strings around     */



void GetLastUser(void);
void GetLastUser(void)
{
        FILE    *pCallerLog;
        char    *sDataFile;

        sDataFile = calloc(PATH_MAX, sizeof(char));
        sprintf(sDataFile, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));
	/*
	 * Fix security in case it is wrong.
	 */
	chmod(sDataFile, 0660);
	
        if((pCallerLog = fopen(sDataFile, "r+")) == NULL)
                WriteError("GetLastUser: Can't open file: %s", sDataFile);
        else {
                fread(&SYSINFO, sizeof(SYSINFO), 1, pCallerLog);

                /* Get lastcaller in memory */
                strcpy(LastCaller, SYSINFO.LastCaller);
                LastCallerTime = SYSINFO.LastTime;

                /* Set next lastcaller (this user) */
                if (!usrconfig.Hidden) {
                        strcpy(SYSINFO.LastCaller,exitinfo.sUserName);
                        SYSINFO.LastTime = time(NULL);
                }

                SYSINFO.SystemCalls++;
                switch(ttyinfo.type) {
                        case POTS:
                                SYSINFO.Pots++;
                                break;

                        case ISDN:
                                SYSINFO.ISDN++;
                                break;

                        case NETWORK:
                                SYSINFO.Network++;
                                break;

                        case LOCAL:
                                SYSINFO.Local++;
                                break;
                }

                rewind(pCallerLog);
                fwrite(&SYSINFO, sizeof(SYSINFO), 1, pCallerLog);

                fclose(pCallerLog);
        }
        free(sDataFile);
}



/*
 * Function will take two date strings in the following format DD-MM-YYYY and 
 * swap them around in the following format YYYYMMDD
 * ie. 01-02-1995 will become 19950201 so that the leading Zeros are not in
 * the beginning as leading Zeros will fall away if you try compare the
 * two with a if statement (Millenium proof).
 */
void SwapDate(char *Date3, char *Date4)
{
        char *temp2, *temp3;

        temp2 = calloc(10, sizeof(char));
        temp3 = calloc(10, sizeof(char));
        Date1 = calloc(10, sizeof(char));
        Date2 = calloc(10, sizeof(char));

        temp2[0] = Date3[6];
        temp2[1] = Date3[7];
        temp2[2] = Date3[8];
        temp2[3] = Date3[9];
        temp2[4] = Date3[3];
        temp2[5] = Date3[4];
        temp2[6] = Date3[0];
        temp2[7] = Date3[1];
        temp2[8] = '\0';

        temp3[0] = Date4[6];
        temp3[1] = Date4[7];
        temp3[2] = Date4[8];
        temp3[3] = Date4[9];
        temp3[4] = Date4[3];
        temp3[5] = Date4[4];
        temp3[6] = Date4[0];
        temp3[7] = Date4[1];
        temp3[8] = '\0';

        strcpy(Date1, temp2);
        strcpy(Date2, temp3);

        free(temp2);
        free(temp3);
}



void user()
{
    FILE	*pUsrConfig, *pLimits;
    int		i, x, FoundName = FALSE, iFoundLimit = FALSE, IsNew = FALSE;
    long	l1, l2;
    char	*token, temp[PATH_MAX], temp1[84], UserName[37];
    time_t	LastLogin;
    struct stat st;

    grecno = 0;
    Syslog('+', "Unixmode login: %s", sUnixName);

    sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((pUsrConfig = fopen(temp,"r+")) == NULL) {
	/*
	 * This should not happen.
	 */
	WriteError("$Can't open %s", temp);
	printf("Can't open userfile, run \"newuser\" first");
	ExitClient(MBERR_OK);
    }

    fread(&usrconfighdr, sizeof(usrconfighdr), 1, pUsrConfig);
    while (fread(&usrconfig, usrconfighdr.recsize, 1, pUsrConfig) == 1) {
	if (strcmp(usrconfig.Name, sUnixName) == 0) {
	    FoundName = TRUE;
	    break;
	} else
	    grecno++;
    }
							
    if (!FoundName) {
	fclose(pUsrConfig);
	printf("Unknown username: %s\n", sUnixName);
	/* FATAL ERROR: You are not in the BBS users file.*/
	printf("%s\n", (char *) Language(389));
	/* Please run 'newuser' to create an account */
	printf("%s\n", (char *) Language(390));
	Syslog('?', "FATAL: Could not find user in BBS users file.");
	Syslog('?', "       and system is using unix accounts\n");
	ExitClient(MBERR_OK);
    }

    /*
     * Copy username, split first and lastname.
     */
    strcpy(UserName, usrconfig.sUserName);
    if ((strchr(UserName,' ') == NULL && !CFG.iOneName)) {
	token = strtok(UserName, " ");
  	strcpy(FirstName, token);
  	token = strtok(NULL, "\0");
	i = strlen(token);
	for(x = 2; x < i; x++) {
	    if (token[x] == ' ')
		token[x] = '\0';
	}
	strcpy(LastName, token);
    } else
	strcpy(FirstName, UserName);
    strcpy(UserName, usrconfig.sUserName);
    Syslog('+', "%s On-Line at %s", UserName, ttyinfo.comment);
    IsDoing("Just Logged In");

    /*
     * Check some essential files, create them if they don't exist.
     */
    ChkFiles();

    /*
     * Setup users favourite language.
     */
    Set_Language(usrconfig.iLanguage);
    Free_Language();
    InitLanguage();

    /*
     * User logged in, tell it to the server. Check if a location is
     * set, if Ask User location for new users is off, this field is
     * empty but we have to send something to the server.
     */
    if (strlen(usrconfig.sLocation))
	UserCity(mypid, usrconfig.Name, usrconfig.sLocation);
    else
	UserCity(mypid, usrconfig.Name, (char *)"N/A");

    /*
     * Set last file and message area so these numbers are saved when
     * the user hangs up or is logged off before het gets to the main
     * menu. Later in this function the areas are set permanent.
     */
    iAreaNumber = usrconfig.iLastFileArea;
    iMsgAreaNumber = usrconfig.iLastMsgArea;

    /*
     * See if this user is the Sysop.
     */
    strcpy(temp, UserName);
    strcpy(temp1, CFG.sysop_name);
    if ((strcasecmp(CFG.sysop_name, UserName)) == 0) {
	/*
	 * If login name is sysop, set SYSOP true 
	 */
	SYSOP = TRUE;
	Syslog('+', "Sysop is online");
    }

    /*
     * Is this a new user?
     */
    if (usrconfig.iTotalCalls == 0)
	IsNew = TRUE;

    TermInit(usrconfig.GraphMode);

    /*
     * Pause after logo screen.
     */
    alarm_on();
    Pause();

    if (usrconfig.Archiver[0] == '\0') {
	usrconfig.Archiver[0] = 'Z';
	usrconfig.Archiver[1] = 'I';
	usrconfig.Archiver[2] = 'P';
	Syslog('+', "Setup default archiver ZIP");
    }

    /*
     * Check users date format. We do it strict as we
     * need this to be good for several other purposes.
     * If it is correct, the users age is set in UserAge
     */
    if (!Test_DOB(usrconfig.sDateOfBirth)) {
	Syslog('!', "Error in Date of Birth");
	Chg_DOB();
	strcpy(usrconfig.sDateOfBirth, exitinfo.sDateOfBirth);
    }

    /*
     * Check to see if user must expire
     */
    sprintf(temp,"%s", (char *) GetDateDMY());
    SwapDate(temp, usrconfig.sExpiryDate);

    /* Convert Date1 & Date2 to longs for compare */
    l1 = atol(Date1);
    l2 = atol(Date2);

    if (l1 >= l2 && l2 != 0) {
	/* 
	 * If Expiry Date is the same as today expire to 
	 * Expire Sec level
	 */
	usrconfig.Security = usrconfig.ExpirySec;
	Syslog('!', "User is expired, resetting level");
	/*
	 * Show texfile to user telling him about this.
	 */
	DisplayFile((char *)"expired");
    }

    free(Date1); 
    free(Date2);

    /* 
     * Copy limits.data into memory
     */
    sprintf(temp, "%s/etc/limits.data", getenv("MBSE_ROOT"));

    if ((pLimits = fopen(temp,"rb")) == NULL) {
	WriteError("$Can't open %s", temp);
    } else {
	fread(&LIMIThdr, sizeof(LIMIThdr), 1, pLimits);

	while (fread(&LIMIT, sizeof(LIMIT), 1, pLimits) == 1) {
 	    if (LIMIT.Security == usrconfig.Security.level) {
		iFoundLimit = TRUE;
		break;
	    }
	}
	fclose(pLimits);
    }

    if (!iFoundLimit) {
	WriteError("Unknown Security Level in limits.data");
	usrconfig.iTimeLeft = 0; /* Could not find limit, so set to Zero */
	usrconfig.iTimeUsed = 0; /* Set to Zero as well  */
    } else {
	/*
	 * Give user new time limit everyday, also new users get a new limit.
	 */
	sprintf(temp,"%s", (char *) GetDateDMY());
	if (((strcmp(StrDateDMY(usrconfig.tLastLoginDate), temp)) != 0) || IsNew) {
	    /*
	     *  If no timelimit set give user 24 hours.
	     */
	    if (LIMIT.Time)
		usrconfig.iTimeLeft = LIMIT.Time;
	    else
		usrconfig.iTimeLeft = 86400;
	    usrconfig.iTimeUsed    = 0;          /* Set time used today to Zero       */
	    usrconfig.iConnectTime = 0;	     /* Set connect time to Zero          */

	    /*
	     * Give user new bytes and files every day if needed.
	     */
	    if (LIMIT.DownK && LIMIT.DownF) {
		usrconfig.DownloadKToday = LIMIT.DownK;
		usrconfig.DownloadsToday = LIMIT.DownF;
	    }
	}
    } /* End of else  */

    usrconfig.iConnectTime = 0;

    /* Copy Users Protocol into Memory */
    Set_Protocol(usrconfig.sProtocol);
    tlf(usrconfig.sProtocol);

    /* 
     * Set last login Date and Time, copy previous session
     * values in memory.
     */
    sprintf(LastLoginDate, "%s", StrDateDMY(usrconfig.tLastLoginDate));
    sprintf(LastLoginTime, "%s", StrTimeHMS(usrconfig.tLastLoginDate));
    LastLogin = usrconfig.tLastLoginDate;
    usrconfig.tLastLoginDate = ltime; /* Set current login to current date */
    usrconfig.iTotalCalls++;

    /*
     * Update user record.
     */
    if (fseek(pUsrConfig, usrconfighdr.hdrsize + (grecno * usrconfighdr.recsize), 0) != 0) {
	WriteError("Can't seek in %s/etc/users.data", getenv("MBSE_ROOT"));
    } else {
	fwrite(&usrconfig, sizeof(usrconfig), 1, pUsrConfig);
    }
    fclose(pUsrConfig);

    /*
     * Write users structure to tmp file in ~/home/unixname/exitinfo
     * A copy of the userrecord is also in the variable exitinfo.
     */
    if (! InitExitinfo())
	Good_Bye(MBERR_INIT_ERROR);

    GetLastUser();
    StartTime = xstrcpy(GetLocalHM());
    ChangeHomeDir(exitinfo.Name, exitinfo.Email);

    Syslog('+', "User successfully logged into BBS");
    Syslog('+', "Level %d (%s), %d mins. left, port %s", exitinfo.Security.level, LIMIT.Description, exitinfo.iTimeLeft, pTTY);
    Time2Go = time(NULL);
    Time2Go += exitinfo.iTimeLeft * 60;
    iUserTimeLeft = exitinfo.iTimeLeft;

    IsDoing("Welcome screens");
    DisplayFile((char *)"mainlogo");
    DisplayFile((char *)"welcome");

    /*
     * The following files are only displayed if the user has
     * turned the Bulletins on.
     */
    if (exitinfo.ieNEWS) {
	DisplayFile((char *)"welcome1");
	DisplayFile((char *)"welcome2");
	DisplayFile((char *)"welcome3");
	DisplayFile((char *)"welcome4");
	DisplayFile((char *)"welcome5");
	DisplayFile((char *)"welcome6");
	DisplayFile((char *)"welcome7");
	DisplayFile((char *)"welcome8");
	DisplayFile((char *)"welcome9");

	sprintf(temp, "%s", (char *) GetDateDMY() );
	if ((strcmp(exitinfo.sDateOfBirth, temp)) == 0)
	    DisplayFile((char *)"birthday");

	/*
	 * Displays file if it exists DD-MM.A??
	 */
	sprintf(temp, "%s", (char *) GetDateDMY());
	strcpy(temp1, "");
	strncat(temp1, temp, 5);
	sprintf(temp, "%s", temp1);
	DisplayFile(temp);
	
	/*
	 * Displays users security file if it exists
	 */
	sprintf(temp, "sec%d", exitinfo.Security.level);
	DisplayFile(temp);

	/*
	 * Display News file
	 */
	DisplayFile((char *)"news");
    }

    /*
     * Display Onceonly file, first get the date of that
     * file, search order is the same as in DisplayFile()
     */
    st.st_mtime = 0;
    if (exitinfo.GraphMode) {
	sprintf(temp, "%s/onceonly.ans", lang.TextPath);
	stat(temp, &st);
	if (st.st_mtime == 0) {
	    sprintf(temp, "%s/onceonly.ans", CFG.bbs_txtfiles);
	    stat(temp, &st);
	}
    }
    if (st.st_mtime == 0) {
	sprintf(temp, "%s/onceonly.asc", lang.TextPath);
	stat(temp, &st);
	if (st.st_mtime == 0) {
	    sprintf(temp, "%s/onceonly.asc", CFG.bbs_txtfiles);
	    stat(temp, &st);
	}
    }

    if ((st.st_mtime != 0) && (LastLogin < st.st_mtime))
	DisplayFile((char *)"onceonly");
	
    OLR_SyncTags();

    if (exitinfo.MailScan) {
	IsDoing("New mail check");
	CheckMail();
    }

    /*
     * We don't show new files to new users.
     */
    if (exitinfo.ieFILE && (!IsNew)) {
	IsDoing("New files check");
	NewfileScan(FALSE);
    }
    
    /* 
     * Copy last file Area in to current Area 
     */
    SetFileArea(exitinfo.iLastFileArea);

    /*
     * Copy Last Message Area in to Current Msg Area
     */
    SetMsgArea(usrconfig.iLastMsgArea);
    SetEmailArea((char *)"mailbox");

    /*
     * Set or Reset the DoNotDisturb flag, now is the time
     * we may be interrupted.
     */
    UserSilent(usrconfig.DoNotDisturb);

    /*
     * Start the menu.
     */
    menu();
}


