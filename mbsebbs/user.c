/*****************************************************************************
 *
 * $Id: user.c,v 1.43 2007/03/03 14:38:31 mbse Exp $
 * Purpose ...............: Main user login procedure.  Checks for limits, 
 *                          new ratio's cats all the welcome screens, and 
 *                          does a lot of checking in general.
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
#include "../lib/mbse.h"
#include "../lib/users.h"
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
#include "term.h"
#include "ttyio.h"



extern int	sock;
extern int	cols;
extern int	rows;
extern pid_t	mypid;
char		*StartTime = NULL;


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
        snprintf(sDataFile, PATH_MAX, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));
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



void user(void)
{
    FILE	*pUsrConfig, *pLimits;
    int		i, x, FoundName = FALSE, iFoundLimit = FALSE, IsNew = FALSE, logins = 0, Start;
    int		l1, l2;
    char	*token, temp[PATH_MAX], temp1[84], UserName[37], buf[128], *fullname;
    time_t	LastLogin;
    struct stat st;

    grecno = 0;
    Syslog('+', "Unixmode login: %s", sUnixName);

    snprintf(temp, PATH_MAX, "%s/etc/users.data", getenv("MBSE_ROOT"));
    if ((pUsrConfig = fopen(temp,"r+")) == NULL) {
	/*
	 * This should not happen.
	 */
	WriteError("$Can't open %s", temp);
	PUTSTR((char *)"Can't open userfile, run \"newuser\" first");
	Enter(1);
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
	snprintf(temp, PATH_MAX, "Unknown username: %s\r\n", sUnixName);
	PUTSTR(temp);
	/* FATAL ERROR: You are not in the BBS users file.*/
	snprintf(temp, PATH_MAX, "%s\r\n", (char *) Language(389));
	PUTSTR(temp);
	/* Please run 'newuser' to create an account */
	snprintf(temp, PATH_MAX, "%s\r\n", (char *) Language(390));
	PUTSTR(temp);
	Syslog('?', "FATAL: Could not find user in BBS users file.");
	Syslog('?', "       and system is using unix accounts\n");
	Free_Language();
	ExitClient(MBERR_OK);
    }

    /*
     * Copy username, split first and lastname.
     */
    strncpy(UserName, usrconfig.sUserName, sizeof(UserName)-1);
    if ((strchr(UserName,' ') == NULL) && !CFG.iOneName) {
	token = strtok(UserName, " ");
  	strncpy(FirstName, token, sizeof(FirstName)-1);
  	token = strtok(NULL, "\0");
	i = strlen(token);
	for (x = 2; x < i; x++) {
	    if (token[x] == ' ')
		token[x] = '\0';
	}
	strncpy(LastName, token, sizeof(LastName)-1);
    } else
	strncpy(FirstName, UserName, sizeof(FirstName)-1);
    strncpy(UserName, usrconfig.sUserName, sizeof(UserName)-1);
    Syslog('+', "%s On-Line from \"%s\", node %d", UserName, ttyinfo.comment, iNode);
    IsDoing("Just Logged In");

    /*
     * Check some essential files, create them if they don't exist.
     */
    ChkFiles();

    /*
     * Setup users favourite language.
     */
    utf8 = (usrconfig.Charset == FTNC_UTF8);
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
     * Count simultaneous logins
     */
    Start = TRUE;
    while (TRUE) {
	if (Start)
	    snprintf(buf, 128, "GMON:1,1;");
	else
	    snprintf(buf, 128, "GMON:1,0;");
	Start = FALSE;
	if (socket_send(buf) == 0) {
	    strcpy(buf, socket_receive());
	    if (strncmp(buf, "100:0;", 6) == 0)
		break;  /* No more data */
	    if (strstr(buf, "mbsebbs")) {
		/*
		 * Only mbsebbs is wanted
		 */
		strtok(buf, ",");				    /* response */
		strtok(NULL, ",");				    /* pid	*/
		strtok(NULL, ",");				    /* tty	*/
		fullname = xstrcpy(cldecode(strtok(NULL, ",")));    /* username */
		if (strcmp(fullname, usrconfig.Name) == 0) {
		    logins++;
		}
		free(fullname);
	    }
	}
    }
    if (CFG.max_logins && (logins > CFG.max_logins)) {
	Syslog('+', "User logins %d, allowed %d, disconnecting", logins, CFG.max_logins);
	colour(LIGHTRED, BLACK);
	snprintf(temp, PATH_MAX, "%s %d %s\r\n", (char *) Language(18), CFG.max_logins, (char *) Language(19));
	PUTSTR(temp);
	Quick_Bye(MBERR_INIT_ERROR);
    }
    
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
    snprintf(temp,PATH_MAX, "%s", (char *) GetDateDMY());
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
    snprintf(temp, PATH_MAX, "%s/etc/limits.data", getenv("MBSE_ROOT"));

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
	snprintf(temp,PATH_MAX, "%s", (char *) GetDateDMY());
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
	    if (LIMIT.DownK) {
		usrconfig.DownloadKToday = LIMIT.DownK;
	    }
            if (LIMIT.DownF) {
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
    snprintf(LastLoginDate, 12, "%s", StrDateDMY(usrconfig.tLastLoginDate));
    snprintf(LastLoginTime, 9, "%s", StrTimeHMS(usrconfig.tLastLoginDate));
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

    /*
     * If user has not set a preferred character set, force this
     */
    if (exitinfo.Charset == FTNC_NONE) {
	Chg_Charset();
    }

    setlocale(LC_CTYPE, getlocale(exitinfo.Charset)); 	 
    Syslog('b', "setlocale(LC_CTYPE, NULL) returns \"%s\"", printable(setlocale(LC_CTYPE, NULL), 0));

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

	snprintf(temp, PATH_MAX, "%s", (char *) GetDateDMY() );
	if ((strcmp(exitinfo.sDateOfBirth, temp)) == 0)
	    DisplayFile((char *)"birthday");

	/*
	 * Displays file if it exists DD-MM.A??
	 */
	snprintf(temp, PATH_MAX, "%s", (char *) GetDateDMY());
	strcpy(temp1, "");
	strncat(temp1, temp, 5);
	snprintf(temp, PATH_MAX, "%s", temp1);
	DisplayFile(temp);
	
	/*
	 * Displays users security file if it exists
	 */
	snprintf(temp, PATH_MAX, "sec%d", exitinfo.Security.level);
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
    snprintf(temp, PATH_MAX, "%s/share/int/txtfiles/%s/onceonly.ans", getenv("MBSE_ROOT"), lang.lc);
    stat(temp, &st);
    if (st.st_mtime == 0) {
	snprintf(temp, PATH_MAX, "%s/share/int/txtfiles/%s/onceonly.ans", getenv("MBSE_ROOT"), CFG.deflang);
	stat(temp, &st);
    }
    if (st.st_mtime == 0) {
	snprintf(temp, PATH_MAX, "%s/share/int/txtfiles/%s/onceonly.asc", getenv("MBSE_ROOT"), lang.lc);
	stat(temp, &st);
	if (st.st_mtime == 0) {
	    snprintf(temp, PATH_MAX, "%s/share/int/txtfiles/%s/onceonly.asc", getenv("MBSE_ROOT"), CFG.deflang);
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


