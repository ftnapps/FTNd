/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Misc functions
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
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "../lib/mberrors.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "misc.h"
#include "timeout.h"
#include "exitinfo.h"


extern char	*StartTime;	/* Time user logged in	    */

/*
 * Last caller action flags
 */
int		LC_Download = FALSE;
int		LC_Upload = FALSE;
int		LC_Read = FALSE;
int		LC_Wrote = FALSE;
int		LC_Chat = FALSE;
int		LC_Olr = FALSE;
int		LC_Door = FALSE;



int ChkFiles()
{
	FILE	*pCallerLog, *pUsersFile;
	char	*sDataFile;
	time_t	Now;
	char	*temp;

	sDataFile = calloc(PATH_MAX, sizeof(char));
	temp = calloc(PATH_MAX, sizeof(char));
	sprintf(sDataFile, "%s/etc/sysinfo.data", getenv("MBSE_ROOT"));
	chmod(sDataFile, 0660);

	/*
	 * Check if users.data exists, if not create a new one.
	 */
	sprintf(temp, "%s/etc/users.data", getenv("MBSE_ROOT"));
	if ((pUsersFile = fopen(temp,"rb")) == NULL) {
		if((pUsersFile = fopen(temp,"wb")) == NULL) {
			WriteError("$Can't create %s", temp);
			ExitClient(MBERR_INIT_ERROR); 
		} else {
			usrconfighdr.hdrsize = sizeof(usrconfighdr);
			usrconfighdr.recsize = sizeof(usrconfig);
			fwrite(&usrconfighdr, sizeof(usrconfighdr), 1, pUsersFile);
			fclose(pUsersFile);
			chmod(temp, 0660);
		}
	}

	/*
	 * Check if sysinfo.data exists, if not, create a new one.
	 */
	if ((pCallerLog = fopen(sDataFile, "rb")) == NULL) {
		if((pCallerLog = fopen(sDataFile, "wb")) == NULL)
			WriteError("$ChkFiles: Can't create %s", sDataFile);
		else {
			memset((char *)&SYSINFO, 0, sizeof(SYSINFO));
			Now = time(NULL);
			SYSINFO.StartDate = Now;

			rewind(pCallerLog);
			fwrite(&SYSINFO, sizeof(SYSINFO), 1, pCallerLog);
			fclose(pCallerLog);
			chmod(sDataFile, 0660);
		}
	}
	free(temp);
	free(sDataFile);
	return 1;
}



void DisplayLogo()
{
	FILE	*pLogo;
	char	*sString, *temp;

	temp = calloc(PATH_MAX, sizeof(char));
	sString = calloc(81, sizeof(char));

	sprintf(temp, "%s/%s", CFG.bbs_txtfiles, CFG.welcome_logo);
	if((pLogo = fopen(temp,"rb")) == NULL)
		WriteError("$DisplayLogo: Can't open %s", temp);
	else {
		while( fgets(sString,80,pLogo) != NULL)
			printf("%s", sString);
		fclose(pLogo);
	}
	free(sString);
	free(temp);
}



/*
 * Update a variable in the exitinfo file.
 */
void Setup(char *Option, char *variable)
{
	ReadExitinfo();
	strcpy(Option, variable);
	WriteExitinfo();
}



void SaveLastCallers()
{
	FILE	*pGLC;
	char	*sFileName;
	char	sFileDate[9];
	char	sDate[9];
	struct	stat statfile;

	/*
	 * First check if we passed midnight, in that case we
	 * create a fresh file.
	 */
	sFileName = calloc(PATH_MAX, sizeof(char));
	sprintf(sFileName,"%s/etc/lastcall.data", getenv("MBSE_ROOT"));
	stat(sFileName, &statfile);

	sprintf(sFileDate,"%s", StrDateDMY(statfile.st_mtime));
	sprintf(sDate,"%s", (char *) GetDateDMY());

	if ((strcmp(sDate,sFileDate)) != 0) {
		unlink(sFileName);
		Syslog('+', "Erased old lastcall.data");
	}

	/*
	 * Check if file exists, if not create the file and
	 * write the fileheader.
	 */
	if ((pGLC = fopen(sFileName, "r")) == NULL) {
		if ((pGLC = fopen(sFileName, "w")) != NULL) {
			LCALLhdr.hdrsize = sizeof(LCALLhdr);
			LCALLhdr.recsize = sizeof(LCALL);
			fwrite(&LCALLhdr, sizeof(LCALLhdr), 1, pGLC);
			Syslog('+', "Created new lastcall.data");
		}
		fclose(pGLC);
	}
	chmod(sFileName, 0660);

	if ((pGLC = fopen(sFileName,"a+")) == NULL) {
		WriteError("$Can't open %s", sFileName);
		return;
	} else {
		ReadExitinfo();
		memset(&LCALL, 0, sizeof(LCALL));
		sprintf(LCALL.UserName,"%s", exitinfo.sUserName);
		sprintf(LCALL.Handle,"%s", exitinfo.sHandle);
		sprintf(LCALL.Name, "%s", exitinfo.Name);
		sprintf(LCALL.TimeOn,"%s", StartTime);
		sprintf(LCALL.Device,"%s", pTTY);
		LCALL.SecLevel = exitinfo.Security.level;
		LCALL.Calls  = exitinfo.iTotalCalls;
		LCALL.CallTime = exitinfo.iConnectTime;
		LCALL.Download = LC_Download;
		LCALL.Upload = LC_Upload;
		LCALL.Read = LC_Read;
		LCALL.Wrote = LC_Wrote;
		LCALL.Chat = LC_Chat;
		LCALL.Olr = LC_Olr;
		LCALL.Door = LC_Door;
		sprintf(LCALL.Speed, "%s", ttyinfo.speed);

		/* If true then set hidden so it doesn't display in lastcallers function */
		LCALL.Hidden = exitinfo.Hidden;

		sprintf(LCALL.Location,"%s", exitinfo.sLocation);

		rewind(pGLC); /* ???????????? */
		fwrite(&LCALL, sizeof(LCALL), 1, pGLC);
		fclose(pGLC);
 	}
	free(sFileName);
}



/* Gets Date for GetLastCallers(), returns DD:Mmm */
char *GLCdate()
{
	static char	GLcdate[15];

	Time_Now = time(NULL);
	l_date = localtime(&Time_Now);
	sprintf(GLcdate,"%02d-", l_date->tm_mday);

	strcat(GLcdate,GetMonth(l_date->tm_mon+1));
	return(GLcdate);
}



