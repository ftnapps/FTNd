/*****************************************************************************
 *
 * File ..................: bbs/funcs.c
 * Purpose ...............: Misc functions
 * Last modification date : 17-Oct-2001
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
#include "../lib/common.h"
#include "../lib/msgtext.h"
#include "../lib/msg.h"
#include "../lib/clcomm.h"
#include "funcs.h"
#include "language.h"
#include "funcs4.h"
#include "oneline.h"
#include "misc.h"
#include "bye.h"
#include "timeout.h"
#include "timecheck.h"
#include "exitinfo.h"
#include "mail.h"
#include "email.h"


extern long	ActiveMsgs;
extern time_t	t_start;
extern int	e_pid;



/*
 * Security Access Check
 */
int Access(securityrec us, securityrec ref)
{
	Syslog('B', "User %5d %08lx %08lx", us.level, us.flags, ~us.flags);
	Syslog('B', "Ref. %5d %08lx %08lx", ref.level, ref.flags, ref.notflags);

	if (us.level < ref.level)
		return FALSE;

	if ((ref.notflags & ~us.flags) != ref.notflags)
		return FALSE;

	if ((ref.flags & us.flags) != ref.flags)
		return FALSE;

	return TRUE;
}



void UserList(char *OpData)
{                                                                        
	FILE	*pUsrConfig;
	int	LineCount = 2;
	int	iFoundName = FALSE;
	int	iNameCount = 0;
	char	*Name, *sTemp, *User;
	char	*temp;
	struct	userhdr	uhdr;
	struct	userrec	u;

	temp  = calloc(PATH_MAX, sizeof(char));
	Name  = calloc(37, sizeof(char));
	sTemp = calloc(81, sizeof(char));
	User  = calloc(81, sizeof(char));

	clear();
	/* User List */
	language(15, 0, 126);
	Enter(1);
	LineCount = 1;

	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((pUsrConfig = fopen(temp, "rb")) == NULL) {
		WriteError("UserList: Can't open file: %s", temp);
		return;
	}
	fread(&uhdr, sizeof(uhdr), 1, pUsrConfig);

	/* Enter Username search string or (Enter) for all users: */
	language(15, 0, 127);
	colour(CFG.InputColourF, CFG.InputColourB);
	alarm_on();
	GetstrC(Name,35);
	clear();

	/* Name         Location                   Last On    Calls */
	language(15, 0, 128);
	Enter(1);

	colour(2, 0);
	fLine(79);

	colour(3, 0);
	while (fread(&u, uhdr.recsize, 1, pUsrConfig) == 1) {
		if ((strcmp(Name,"")) != 0) {
			if((strcmp(OpData, "/H")) == 0)
				sprintf(User, "%s", u.sHandle);
			else
				sprintf(User, "%s", u.sUserName);

			if ((strstr(tl(User), tl(Name)) != NULL)) {
				if ((!u.Hidden) && (!u.Deleted)) {
					if((strcmp(OpData, "/H")) == 0) {
						if((strcmp(u.sHandle, "") != 0 && *(u.sHandle) != ' '))
							printf("%-25s", u.sHandle);
						else
							printf("%-25s", u.sUserName);
					} else 
						printf("%-25s", u.sUserName);

					printf("%-30s%-14s%-11d", u.sLocation, StrDateDMY(u.tLastLoginDate), u.iTotalCalls);
					iFoundName = TRUE;
					LineCount++;
					iNameCount++; 
				}
			}
		} else
			if ((!u.Hidden) && (!u.Deleted) && (strlen(u.sUserName) > 0)) {
				if((strcmp(OpData, "/H")) == 0) {
					if((strcmp(u.sHandle, "") != 0 && *(u.sHandle) != ' '))
						printf("%-25s", u.sHandle);
					else
						printf("%-25s", u.sUserName);
				} else
					printf("%-25s", u.sUserName);

	   	    		printf("%-30s%-14s%-11d", u.sLocation, StrDateDMY(u.tLastLoginDate), u.iTotalCalls);
				iFoundName = TRUE;
				LineCount++;
				iNameCount++;
				Enter(1);
			}

		if (LineCount >= exitinfo.iScreenLen - 2) {
			LineCount = 0;
			Pause();
			colour(3, 0);
		}
	}

	if(!iFoundName) {
		language(3, 0, 129);
		Enter(1);
	}

	fclose(pUsrConfig);

	colour(2, 0);
	fLine(79); 

	free(temp);
	free(Name);
	free(sTemp);
	free(User);

	Pause();
}



void TimeStats()
{
	clear();
	ReadExitinfo();

	colour(15, 0);
	/* TIME STATISTICS for */
	printf("\n%s%s ", (char *) Language(134), exitinfo.sUserName);
	/* on */
	printf("%s %s\n", (char *) Language(135), (char *) logdate());

	colour(12, 0);
	fLine(79);

	printf("\n");
	
	colour(10, 0);

	/* Current Time */
	printf("%s %s\n", (char *) Language(136), (char *) GetLocalHMS());

	/* Current Date */
	printf("%s %s\n\n", (char *) Language(137), (char *) GLCdateyy());

	/* Connect time */
	printf("%s %d %s\n", (char *) Language(138), exitinfo.iConnectTime, (char *) Language(471));

	/* Time used today */
	printf("%s %d %s\n", (char *) Language(139), exitinfo.iTimeUsed, (char *) Language(471));

	/* Time remaining today */
	printf("%s %d %s\n", (char *) Language(140), exitinfo.iTimeLeft, (char *) Language(471));

	/* Daily time limit */
	printf("%s %d %s\n", (char *) Language(141), exitinfo.iTimeUsed + exitinfo.iTimeLeft, (char *) Language(471));

	printf("\n");
	Pause();
}



char *Gdate(time_t, int);
char *Gdate(time_t tt, int Y2K)
{
	static char	GLC[15];
	struct tm	*tm;

	tm = localtime(&tt);
	if (Y2K)
		sprintf(GLC, "%02d-%02d-%04d", tm->tm_mon +1, tm->tm_mday, tm->tm_year + 1900);
	else
		sprintf(GLC, "%02d-%02d-%02d", tm->tm_mon +1, tm->tm_mday, tm->tm_year % 100);

	return (GLC);
}



char *Rdate(char *, int);
char *Rdate(char *ind, int Y2K)
{
	static char	GLC[15];

	memset(&GLC, 0, sizeof(GLC));
	GLC[0] = ind[3];
	GLC[1] = ind[4];
	GLC[2] = '-';
	GLC[3] = ind[0];
	GLC[4] = ind[1];
	GLC[5] = '-';
	if (Y2K) {
		GLC[6] = ind[6];
		GLC[7] = ind[7];
		GLC[8] = ind[8];
		GLC[9] = ind[9];
	} else {
		GLC[6] = ind[8];
		GLC[7] = ind[9];
	}

	return GLC;
}



/*
 * Function will run a external program or door
 */
void ExtDoor(char *Program, int NoDoorsys, int Y2Kdoorsys, int Comport, int NoSuid)
{
	char	*String, *String1;
	int	i, rc;
	char	*temp1;
	FILE	*fp;

	temp1 = calloc(PATH_MAX, sizeof(char));
	String = calloc(81, sizeof(char));

	WhosDoingWhat(DOOR);

	if((strstr(Program, "/A")) != NULL) {
		colour(3, 0);
		if((String = strstr(Program, "/T=")) != NULL) {
			String1 = String + 3;
			printf("\n%s", String1);
		} else
			printf("\nPlease enter filename: ");

		fflush(stdout);
		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(temp1, 80);

		strreplace(Program, (char *)"/A", temp1);

		for(i = 0; i < strlen(Program); i++) {
			if(*(Program + i) == '\0')
				break;
			if(*(Program + i) == '/')
				*(Program + i) = '\0';
		}
	}

	free(String);
	Syslog('+', "Door: %s", Program);
	ReadExitinfo();
	alarm_set((exitinfo.iTimeLeft * 60) - 10);
	Altime((exitinfo.iTimeLeft * 60));

	/*
	 * Always remove the old door.sys first.
	 */
	sprintf(temp1, "%s/%s/door.sys", CFG.bbs_usersdir, exitinfo.Name);
	unlink(temp1);

	/*
	 * Write door.sys in users homedirectory
	 */
	if (!NoDoorsys) {
		if ((fp = fopen(temp1, "w+")) == NULL) {
			WriteError("$Can't create %s", temp1);
		} else {
			if (Comport) {
				fprintf(fp, "COM1:\r\n"); /* COM port             */
				fprintf(fp, "115200\r\n");/* Effective baudrate   */

			} else {
				fprintf(fp, "COM0:\r\n");/* COM port		*/
				fprintf(fp, "0\r\n");	/* Effective baudrate	*/
			}
			fprintf(fp, "8\r\n");		/* Databits		*/
			fprintf(fp, "1\r\n");		/* Node number		*/
			if (Comport)
				fprintf(fp, "115200\r\n");/* Locked baudrate	*/
			else
				fprintf(fp, "%ld\r\n", ttyinfo.portspeed); /* Locked baudrate */
			fprintf(fp, "Y\r\n");		/* Screen display	*/
			fprintf(fp, "N\r\n");		/* Printer on		*/
			fprintf(fp, "Y\r\n");		/* Page bell		*/
			fprintf(fp, "Y\r\n");		/* Caller alarm		*/
			fprintf(fp, "%s\r\n", exitinfo.sUserName);
			fprintf(fp, "%s\r\n", exitinfo.sLocation);
			fprintf(fp, "%s\r\n", exitinfo.sVoicePhone);
			fprintf(fp, "%s\r\n", exitinfo.sDataPhone);
			fprintf(fp, "%s\r\n", exitinfo.Password);
			fprintf(fp, "%d\r\n", exitinfo.Security.level);
			fprintf(fp, "%d\r\n", exitinfo.iTotalCalls);
			fprintf(fp, "%s\r\n", Gdate(exitinfo.tLastLoginDate, Y2Kdoorsys));
			fprintf(fp, "%d\r\n", exitinfo.iTimeLeft * 60);
			fprintf(fp, "%d\r\n", exitinfo.iTimeLeft);
			fprintf(fp, "GR\r\n");		/* ANSI graphics	*/
			fprintf(fp, "%d\r\n", exitinfo.iScreenLen);
			fprintf(fp, "N\r\n");		/* User mode, always N	*/
			fprintf(fp, "\r\n");		/* Always blank		*/
			fprintf(fp, "\r\n");		/* Always blank		*/
			fprintf(fp, "%s\r\n", Rdate(exitinfo.sExpiryDate, Y2Kdoorsys));
			fprintf(fp, "%d\r\n", grecno);	/* Users recordnumber	*/
			fprintf(fp, "%s\r\n", exitinfo.sProtocol);
			fprintf(fp, "%ld\r\n", exitinfo.Uploads);
			fprintf(fp, "%ld\r\n", exitinfo.Downloads);
			fprintf(fp, "%ld\r\n", LIMIT.DownK);
			fprintf(fp, "%ld\r\n", LIMIT.DownK);
			fprintf(fp, "%s\r\n", Rdate(exitinfo.sDateOfBirth, Y2Kdoorsys));
			fprintf(fp, "\r\n");		/* Path to userbase	*/
			fprintf(fp, "\r\n");		/* Path to messagebase	*/
			fprintf(fp, "%s\r\n", CFG.sysop_name);
			fprintf(fp, "%s\r\n", exitinfo.sHandle);
			fprintf(fp, "none\r\n");	/* Next event time	*/
			fprintf(fp, "Y\r\n");		/* Error free connect.	*/
			fprintf(fp, "N\r\n");		/* Always N		*/
			fprintf(fp, "Y\r\n");		/* Always Y		*/
			fprintf(fp, "7\r\n");		/* Default textcolor	*/
			fprintf(fp, "0\r\n");		/* Always 0		*/
			fprintf(fp, "%s\r\n", Gdate(exitinfo.tLastLoginDate, Y2Kdoorsys));
			fprintf(fp, "%s\r\n", StrTimeHM(t_start));
			fprintf(fp, "%s\r\n", LastLoginTime);
			fprintf(fp, "32768\r\n");	/* Always 32768		*/
			fprintf(fp, "%d\r\n", exitinfo.DownloadsToday);
			fprintf(fp, "%ld\r\n", exitinfo.UploadK);
			fprintf(fp, "%ld\r\n", exitinfo.DownloadK);
			fprintf(fp, "%s\r\n", exitinfo.sComment);
			fprintf(fp, "0\r\n");		/* Always 0		*/
			fprintf(fp, "%d\r\n\032", exitinfo.iPosted);
			fclose(fp);
		}
	}

	clear();
	printf("Loading ...\n\n");
	if (NoSuid) 
	    rc = exec_nosuid(Program);
	else
	    rc = execute((char *)"/bin/sh", (char *)"-c", Program, NULL, NULL, NULL);

	Altime(0);
	alarm_off();
	alarm_on();
	Syslog('+', "Door end, rc=%d", rc);

	free(temp1);
	printf("\n\n");
	Pause();
}



/*
 * Execute a door as real user, not suid.
 */
int exec_nosuid(char *mandato)
{
    int	    rc, status;
    pid_t   pid;

    if (mandato == NULL)
	return 1;   /* Prevent running a shell  */
    Syslog('+', "Execve: /bin/sh -c %s", mandato);
    pid = fork();
    if (pid == -1)
	return 1;
    if (pid == 0) {
	char *argv[4];
	argv[0] = (char *)"sh";
	argv[1] = (char *)"-c";
	argv[2] = mandato;
	argv[3] = 0;
	execve("/bin/sh", argv, environ);
	exit(127);
    }
    e_pid = pid;

    do {
	rc = waitpid(pid, &status, 0);
	e_pid = 0;
    } while (((rc > 0) && (rc != pid)) || ((rc == -1) && (errno == EINTR)));

    switch(rc) {
	case -1:
		WriteError("$Waitpid returned %d, status %d,%d", rc,status>>8,status&0xff);
		return -1;
	case 0:
		return 0;
	default:
		if (WIFEXITED(status)) {
		    rc = WEXITSTATUS(status);
		    if (rc) {
			WriteError("Exec_nosuid: returned error %d", rc);
			return rc;
		    }
		}
		if (WIFSIGNALED(status)) {
		    rc = WTERMSIG(status);
		    WriteError("Wait stopped on signal %d", rc);
		    return rc;
		}
		if (rc)
		    WriteError("Wait stopped unknown, rc=%d", rc);
		return rc;      
	}
	return 0;
}



/*
 * Function will display textfile in either ansi or ascii and 
 * display control codes if they exist.
 * Returns Success if it can display the requested file
 */
int DisplayFile(char *filename)
{
	FILE	*pFileName;
	long	iSec = 0;
	char	*sFileName, *tmp, *tmp1;
	char	newfile[PATH_MAX];
	int	i, x;

	sFileName = calloc(16385, sizeof(char));
	tmp       = calloc(PATH_MAX, sizeof(char));
	tmp1      = calloc(PATH_MAX, sizeof(char));

	/*
	 * Open the file in the following search order:
	 *  1 - if GraphMode -> users language .ans
	 *  2 - if GraphMode -> default language .ans
	 *  3 - users language .asc
	 *  4 - default language .asc
	 *  5 - Abort, there is no file to show.
	 */
	pFileName = NULL;
	if (exitinfo.GraphMode) {
		sprintf(newfile, "%s/%s.ans", lang.TextPath, filename);
		if ((pFileName = fopen(newfile, "rb")) == NULL) {
			sprintf(newfile, "%s/%s.ans", CFG.bbs_txtfiles, filename);
			pFileName = fopen(newfile, "rb");
		}
	}
	if (pFileName == NULL) {
		sprintf(newfile, "%s/%s.asc", lang.TextPath, filename);
		if ((pFileName = fopen(newfile, "rb")) == NULL) {
			sprintf(newfile, "%s/%s.asc", CFG.bbs_txtfiles, filename);
			if ((pFileName = fopen(newfile, "rb")) == NULL) {
				free(sFileName);
				free(tmp);
				free(tmp1);
				return FALSE;
			}
		}
	}

	Syslog('B', "Displayfile %s", newfile);

	while (!feof(pFileName)) {
		i = fread(sFileName, sizeof(char), 16384, pFileName);

		for(x = 0; x < i; x++) {
			switch(*(sFileName + x)) {
				case '':
					ControlCodeU(sFileName[++x]);
					break;

				case '':
					ControlCodeF(sFileName[++x]);
					break;

				case '':
					ControlCodeK(sFileName[++x]);
					break;

				case '':
					fflush(stdout);
					fflush(stdin);
					alarm_on();
					Getone();
					break;

				case '':
					/*
					 * This code will allow you to specify a security level
					 * in front of the text, ie ^B32000^Bthis is a test^B
					 * will print this is a test only if you have security
					 * above 32000. Only one set of control chars per line.
					 * You cannot have multiple securitys etc
					 */
					x++;
					strcpy(tmp1, "");
					while (*(sFileName + x) != '') {
						sprintf(tmp, "%c", *(sFileName + x));
						strcat(tmp1, tmp);
						x++;
					}
					x++;
					iSec = atoi(tmp1);
					while ((x <= i) && (*(sFileName + x) != '')) {
						if (exitinfo.Security.level >= iSec)
							printf("%c", *(sFileName + x));
						x++;
					} 
					break;

				case '':
					fflush(stdout);
					sleep(1);
					break;

				default:
					printf("%c", *(sFileName + x));

			} /* switch */
		} /* for */
	} /* while !eof */

	fclose(pFileName);
	free(sFileName);
	free(tmp);
	free(tmp1);
	return TRUE;
}



int DisplayFileEnter(char *File)
{
	int	rc;

	rc = DisplayFile(File);
	Enter(1);
	/* Press ENTER to continue */
	language(13, 0, 436);
	fflush(stdout);
	fflush(stdin);
	alarm_on();
	Getone();
	return rc;
}



int CheckFile(char *File, int iArea)
{
	FILE	*pFileB;
	int	iFile = FALSE;
	char	*sFileArea;

	sFileArea = calloc(PATH_MAX, sizeof(char));
	sprintf(sFileArea,"%s/fdb/fdb%d.dta", getenv("MBSE_ROOT"), iArea); 

	if(( pFileB = fopen(sFileArea,"r+")) == NULL) {
		mkdir(sFileArea, 755);
		return FALSE;
	}

	while ( fread(&file, sizeof(file), 1, pFileB) == 1) {
		if((strcmp(tl(file.Name), tl(File))) == 0) {
			iFile = TRUE;
			fclose(pFileB);
			return TRUE;
		}

	}

	fclose(pFileB);
	free(sFileArea);

	if(!iFile)
		return FALSE;
	return 1;
}



void ControlCodeF(int ch)
{
	/* Update user info */
	ReadExitinfo();

	switch (toupper(ch)) {
	case '!':
		printf(exitinfo.sProtocol);
		break;
	case 'A':
		printf("%ld", exitinfo.Uploads);
		break;

	case 'B':
		printf("%ld", exitinfo.Downloads);
		break;

	case 'C':
		printf("%lu", exitinfo.DownloadK);
		break;

	case 'D':
		printf("%lu", exitinfo.UploadK);
		break;

	case 'E':
		printf("%lu", exitinfo.DownloadK + exitinfo.UploadK);
		break;

	case 'F':
		printf("%lu", LIMIT.DownK); 
		break;

	case 'G':
		printf("%d", exitinfo.iTransferTime);
		break;

	case 'H':
		printf("%d", iAreaNumber);
		break;

	case 'I':
		printf(sAreaDesc);
		break;

	case 'J':
		printf("%u", LIMIT.DownF);
		break;

	case 'K':
		printf("%s", LIMIT.Description);
		break;

	default:
		printf(" ");
	}
}



void ControlCodeU(int ch)
{
	/*
	 * Update user info
	 */
	TimeCheck();
	ReadExitinfo();

	switch (toupper(ch)) {
	case 'A':
		printf("%s", exitinfo.sUserName);
		break;

	case 'B':
		printf(exitinfo.sLocation);
		break;

	case 'C':
		printf(exitinfo.sVoicePhone);
		break;

	case 'D':
		printf(exitinfo.sDataPhone);
		break;

	case 'E':
		printf(LastLoginDate);
		break;

	case 'F':
		printf("%s %s", StrDateDMY(exitinfo.tFirstLoginDate), StrTimeHMS(exitinfo.tFirstLoginDate));
		break;
		
	case 'G':
		printf(LastLoginTime);
		break;

	case 'H':
		printf("%d", exitinfo.Security.level);
		break;
		
	case 'I':
		printf("%d",  exitinfo.iTotalCalls);
		break;

	case 'J':
		printf("%d", exitinfo.iTimeUsed);
		break;

	case 'K':
		printf("%d", exitinfo.iConnectTime);
		break;

	case 'L':
		printf("%d", exitinfo.iTimeLeft);
		break;

	case 'M':
		printf("%d", exitinfo.iScreenLen);
		break;

	case 'N':
		printf(FirstName);
		break;

	case 'O':
		printf(LastName);
		break;

	case 'Q':
	 	printf("%s", exitinfo.ieNEWS ? (char *) Language(147) : (char *) Language(148));
		break;

	case 'P':
		printf("%s", exitinfo.GraphMode ? (char *) Language(147) : (char *) Language(148));
		break;

	case 'R':
		printf("%s", exitinfo.HotKeys ? (char *) Language(147) : (char *) Language(148));
		break;

	case 'S':
		printf("%d", exitinfo.iTimeUsed + exitinfo.iTimeLeft);
		break;

	case 'T':
		printf(exitinfo.sDateOfBirth);
		break;

	case 'U':
		printf("%d", exitinfo.iPosted);
		break;

	case 'X':
		printf(lang.Name);
		break;

	case 'Y':
		printf(exitinfo.sHandle);
		break;

	case 'Z':
	 	printf("%s", exitinfo.DoNotDisturb ? (char *) Language(147) : (char *) Language(148));
		break;

	case '1':
		printf("%s", exitinfo.MailScan ? (char *) Language(147) : (char *) Language(148));
		break;

	case '2':
		printf("%s", exitinfo.ieFILE ? (char *) Language(147) : (char *) Language(148));
		break;

	case '3':
		printf("%s", exitinfo.FsMsged ? (char *) Language(147) : (char *) Language(148));
		break;

	default:
		printf(" ");
	}
}



void ControlCodeK(int ch)
{
	FILE		*pCallerLog;
	char		sDataFile[PATH_MAX];
	lastread	LR;

	switch (toupper(ch)) {
	case 'A':
		printf("%s", (char *) GetDateDMY());
		break;

	case 'B':
		printf("%s", (char *) GetLocalHMS());
		break;

	case 'C':
		printf("%s", (char *) GLCdate());
		break;

	case 'D':
		printf("%s", (char *) GLCdateyy());
		break;

	case 'E':
		printf("%d", Speed() );
		break;

	case 'F':
	  	printf("%s", LastCaller);
		break;

	case 'G':
		printf("%d", TotalUsers());
		break;

	case 'H':
		sprintf(sDataFile, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));
		if((pCallerLog = fopen(sDataFile, "rb")) != NULL) {
			fread(&SYSINFO, sizeof(SYSINFO), 1, pCallerLog);
			printf("%ld", SYSINFO.SystemCalls);
			fclose(pCallerLog);
		}
		break;

	case 'I':
		printf("%d", iMsgAreaNumber + 1);
		break;

	case 'J':
		printf(sMsgAreaDesc);
		break;

	case 'K':
		printf("%s", Oneliner_Get());
		break;

	case 'L':
		SetMsgArea(iMsgAreaNumber);
		printf("%ld", MsgBase.Total);
		break;

	case 'M':
		LR.UserID = grecno;
		if (Msg_Open(sMsgAreaBase)) {
			if (Msg_GetLastRead(&LR) == TRUE) {
				if (LR.HighReadMsg > MsgBase.Highest)
					LR.HighReadMsg = MsgBase.Highest;
				printf("%ld", LR.HighReadMsg);
			} else
				printf("?");
			Msg_Close();
		}
		break;

	case 'N':
		printf("%s", sMailbox);
		break;

	case 'O':
		SetEmailArea(sMailbox);
		printf("%ld", EmailBase.Total);
		break;

	case 'P':
		sprintf(sDataFile, "%s/%s/%s", CFG.bbs_usersdir, exitinfo.Name, sMailbox);
		LR.UserID = grecno;
		if (Msg_Open(sDataFile)) {
			if (Msg_GetLastRead(&LR) == TRUE) {
				if (LR.HighReadMsg > EmailBase.Highest)
					LR.HighReadMsg = EmailBase.Highest;
				printf("%ld", LR.HighReadMsg);
			} else
				printf("?");
			Msg_Close();
		}
		break;

	default:
		printf(" ");

	}
}



/*
 * View a textfile.
 */
void ViewTextFile(char *Textfile)
{
	FILE	*fp;
	int	iLine = 0;
	char	*temp, *temp1;
	char	sPrompt[] = "\n(More (Y/n/=): ";
	int	i, x, z;

	x = strlen(sPrompt);

	temp1 = calloc(PATH_MAX, sizeof(char));
	temp  = calloc(81, sizeof(char));

	sprintf(temp1, "%s", Textfile);

	if(( fp = fopen (temp1, "r")) != NULL) {
		while (fgets(temp, 80, fp) != NULL) {
			printf("%s", temp);
			++iLine;
			if(iLine >= exitinfo.iScreenLen && iLine < 1000) {
				iLine = 0;
				pout(CFG.MoreF, CFG.MoreB, sPrompt);

				fflush(stdout);
				z = Getone();
				switch(z) {

				case 'n':
				case 'N':
					printf("\n");
					break;

				case '=':
					iLine = 1000;
				}
				for(i = 0; i < x; i++)
					printf("\b");
				for(i = 0; i < x; i++)
					printf(" ");
				printf("\r");
			}
		}
		fclose(fp);
	}

	Pause();
	free(temp1);
	free(temp);
}



/*
 * Function will make log entry in users logfile
 * Understands @ for Fileareas and ^ for Message Areas
 */
void LogEntry(char *Log)
{
	char *Entry, *temp;
	int i;

	Entry = calloc(256, sizeof(char));
	temp  = calloc(1, sizeof(char));

	for(i = 0; i < strlen(Log); i++) {
		if(*(Log + i) == '@')
			strcat(Entry, sAreaDesc);
		else 
			if(*(Log + i) == '^')
				strcat(Entry, sMsgAreaDesc);
			else {
				sprintf(temp, "%c", *(Log + i));
				strcat(Entry, temp);
			}
	}

	Syslog('+', Entry);
	free(Entry);
	free(temp);
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



/*
 * Function returns total number of bbs users
 */
int TotalUsers()
{
	FILE	*pUsrConfig;
	int	ch = 0;
	char	*temp;
	struct	userhdr	uhdr;
	struct	userrec	u;

	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if(( pUsrConfig = fopen(temp,"rb")) == NULL)
		WriteError("ControlCodeK: Can't open users file %s for reading", temp);
	else {
		fread(&uhdr, sizeof(uhdr), 1, pUsrConfig);
			
		while (fread(&u, uhdr.recsize, 1, pUsrConfig) == 1)
			if ((!u.Deleted) && (strlen(u.sUserName) > 0))
				ch++;

		fclose(pUsrConfig);
	}
	free(temp);

	return ch;
} 



