/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Misc functions
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
#include "funcs4.h"
#include "language.h"
#include "input.h"
#include "oneline.h"
#include "misc.h"
#include "bye.h"
#include "timeout.h"
#include "timecheck.h"
#include "exitinfo.h"
//#include "whoson.h"
#include "mail.h"
#include "email.h"


extern long	ActiveMsgs;
//extern time_t	t_start;
//extern int	e_pid;
//extern char	**environ;


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




