/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Oneliner functions.
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
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/libs.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "oneline.h"
#include "funcs.h"
#include "input.h"
#include "language.h"


char	sOneliner[81];
int	iColour;		/* Current color	*/


void Oneliner_Check()
{
	FILE	*pOneline;
	char	*sFileName;

	sFileName = calloc(128, sizeof(char));
	sprintf(sFileName, "%s/etc/oneline.data", getenv("MBSE_ROOT"));

	if ((pOneline = fopen(sFileName, "r")) == NULL) {
		if ((pOneline = fopen(sFileName, "w")) != NULL) {
			olhdr.hdrsize = sizeof(olhdr);
			olhdr.recsize = sizeof(ol);
			fwrite(&olhdr, sizeof(olhdr), 1, pOneline);
			fclose(pOneline);
			chmod(sFileName, 0660);
			Syslog('-', "Created oneliner database");
		}
	}
	free(sFileName);
}



void Oneliner_Add()
{
	FILE	*pOneline;
	char	*sFileName;
	int	x;
	char	temp[81];

	Oneliner_Check();

	sFileName = calloc(128, sizeof(char));
	sprintf(sFileName,"%s/etc/oneline.data", getenv("MBSE_ROOT"));

	if((pOneline = fopen(sFileName, "a+")) == NULL) {
		WriteError("Can't open file: %s", sFileName); 
		return;
	}
	free(sFileName);

	memset(&ol, 0, sizeof(ol));
	clear();
	/* MBSE BBS Oneliners will randomly appear on the main menu. */
	poutCR(15, 0, Language(341));
	Enter(1);

	/* Obscene or libellous oneliners will be deleted!! */
	poutCR(15, 1, Language(342));
	Enter(1);

	/* Please enter your oneliner below. You have 75 characters.*/
	pout(12, 0, Language(343));
	Enter(1);
	pout(15, 0, (char *)"> ");
	colour(CFG.InputColourF, CFG.InputColourB);
	fflush(stdout);
	GetstrC(temp, 75);
			
	if((strcmp(temp, "")) == 0) {
		fclose(pOneline);
		return;
	} else {
		x = strlen(temp);
		if(x >= 78)
			temp[78] = '\0';
				
		strcpy(ol.Oneline, temp);
	}
		
	Enter(1);
	/* Oneliner added */
	pout(3, 0, Language(344));
	Enter(2);
	Pause();

	Syslog('!', "User added oneliner:");
	Syslog('!', ol.Oneline);
		
	sprintf(ol.UserName,"%s", exitinfo.sUserName);
	sprintf(ol.DateOfEntry,"%02d-%02d-%04d",l_date->tm_mday,l_date->tm_mon+1,l_date->tm_year+1900);
	ol.Available = TRUE;

	fwrite(&ol, sizeof(ol), 1, pOneline);
	fclose(pOneline);
}




/* 
 * Print global string sOneliner centered on the screen
 */
void Oneliner_Print()
{
	int	i, x, z;
	int	Strlen;
	int	Maxlen = 80;
	char	sNewOneliner[81] = "";

	/*
	 * Select a new colour
	 */
	if (iColour < 8)
		iColour = 8;
	else
		if (iColour == 15)
			iColour = 8;
		else
			iColour++;

	/*
	 * Get a random oneliner
	 */
	strcpy(sOneliner, Oneliner_Get());

	/*
	 * Now display it on screen
	 */
	Strlen = strlen(sOneliner);

	if(Strlen == Maxlen)
		printf("%s\n", sOneliner);
	else {
		x = Maxlen - Strlen;
		z = x / 2;
		for(i = 0; i < z; i++)
			strcat(sNewOneliner," ");
		strcat(sNewOneliner, sOneliner);
		colour(iColour, 0);
		printf("%s\n", sNewOneliner);
	}
}



/*
 * Get a random oneliner
 */
char *Oneliner_Get()
{
	FILE	*pOneline;
	int	i, j, in, id;
	int	recno = 0;
	long	offset;
	int	nrecno;
	char	*sFileName;
	static	char temp[81];

	/*
	 * Get a random oneliner
	 */
	sFileName = calloc(128, sizeof(char));
	sprintf(sFileName,"%s/etc/oneline.data", getenv("MBSE_ROOT"));

	if((pOneline = fopen(sFileName, "r+")) == NULL) {
		WriteError("Can't open file: %s", sFileName);
		return '\0';
	}
	fread(&olhdr, sizeof(olhdr), 1, pOneline);

	while (fread(&ol, olhdr.recsize, 1, pOneline) == 1) {
		recno++;
	}
	nrecno = recno;
	fseek(pOneline, olhdr.hdrsize, 0);

	/*
	 * Generate random record number
	 */
	while (TRUE) {
		in = nrecno;
		id = getpid();

		i = rand();
		j = i % id;
		if ((j <= in))
			break;
	}

	offset = olhdr.hdrsize + (j * olhdr.recsize);
	if (fseek(pOneline, offset, 0) != 0) {
		WriteError("Can't move pointer in %s", sFileName); 
		return '\0';
	}

	fread(&ol, olhdr.recsize, 1, pOneline);
	memset(&temp, 0, sizeof(temp));
	strcpy(temp, ol.Oneline);
	fclose(pOneline);
	free(sFileName);
	return temp;
}



/* 
 * List Oneliners
 */
void Oneliner_List()
{
	FILE	*pOneline;
	int	recno = 0;
	int	Colour = 1;
	char	*sFileName;
	                                                                                  
	clear();
	sFileName = calloc(128, sizeof(char));
	sprintf(sFileName,"%s/etc/oneline.data", getenv("MBSE_ROOT"));

	if((pOneline = fopen(sFileName, "r+")) == NULL) {
		WriteError("Can't open file: %s", sFileName);
		return;
	}
	fread(&olhdr, sizeof(olhdr), 1, pOneline);

	if((SYSOP == TRUE) || (exitinfo.Security.level >= CFG.sysop_access)) {
		/* #  A   Date       User          Description */
		pout(10, 0, Language(345));
		Enter(1);
	} else {
		/* #  Description */
		pout(10, 0, Language(346));
		Enter(1);
	}
	colour(2, 0);
	sLine();

	while (fread(&ol, olhdr.recsize, 1, pOneline) == 1) {
		if((SYSOP == TRUE) || (exitinfo.Security.level >= CFG.sysop_access)) {
			colour(15, 0);
			printf("%2d", recno);

			colour(9, 0);
			printf("%2d ", ol.Available);

			colour(11, 0);
			printf("%s ", ol.DateOfEntry);

			colour(3, 0);
			printf("%-15s ", ol.UserName);

			colour(Colour, 0);
			printf("%-.48s\n", ol.Oneline);
		} else {
			colour(15, 0);
			printf("%2d ", recno);
			colour(Colour, 0);
			printf("%-.76s\n", ol.Oneline);
		}

		recno++;
		Colour++;
		if(Colour >= 16)
			Colour = 1;
	}
	fclose(pOneline);
	printf("\n");
	Pause();
	free(sFileName);
}



void Oneliner_Show()
{
	FILE	*pOneline;
	int	recno = 0;
	long 	offset;
	char	*sFileName;

	sFileName = calloc(128, sizeof(char));
	sprintf(sFileName,"%s/etc/oneline.data", getenv("MBSE_ROOT"));

	if((pOneline = fopen(sFileName, "r+")) == NULL) {
		WriteError("Can't open file: %s", sFileName);
		return;
	}
	fread(&olhdr, sizeof(olhdr), 1, pOneline);

	Enter(1);
	/* Please enter number to list: */
	pout(15, 0, Language(347));
	colour(CFG.InputColourF, CFG.InputColourB);
	scanf("%d", &recno);

	offset = olhdr.hdrsize + (recno * olhdr.recsize);
	if (fseek(pOneline, offset, 0) != 0)
		WriteError("Can't move pointer in %s",sFileName); 

	fread(&ol, olhdr.recsize, 1, pOneline);

	colour(15, 0);
	printf("\n%d ", recno);
	colour(12, 0);
	printf("%s\n\n", ol.Oneline);

	Pause();
	fclose(pOneline);
	free(sFileName);
}



void Oneliner_Delete()
{
	FILE	*pOneline;
	int	recno = 0;
	long	offset;
	int	nrecno = 0;
	char	srecno[7];
	char	*sFileName;
 	char	stemp[50];
	char	sUser[35];

	sFileName = calloc(128, sizeof(char));
	sprintf(sFileName,"%s/etc/oneline.data", getenv("MBSE_ROOT"));

	if((pOneline = fopen(sFileName, "r+")) == NULL) {
		WriteError("Can't open file: %s", sFileName);
		return;
	}
	fread(&olhdr, sizeof(olhdr), 1, pOneline);

	Enter(1);
	/* Please enter number to delete: */
	pout(15, 0, Language(331));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(srecno, 6);

	if((strcmp(srecno,"")) == 0) {
		fclose(pOneline);
		return;
	}

	recno = atoi(srecno);

	nrecno = recno;
	recno = 0;
	
	while (fread(&ol, olhdr.recsize, 1, pOneline) == 1) {
		recno++;
	}

	if(nrecno >= recno) {
		Enter(1);
		/* Record does not exist */
		pout(12, 0, Language(319));
		Enter(2);
		fclose(pOneline);
		Pause();
	} else {
		offset = olhdr.hdrsize + (nrecno * olhdr.recsize);
		if (fseek(pOneline, offset, 0) != 0) {
			WriteError("Can't move pointer in %s",sFileName); 
		}

		fread(&ol, olhdr.recsize, 1, pOneline);

		/* Convert Record Int to string, so we can print to logfiles */
		sprintf(stemp,"%d", nrecno);

		/* Print UserName to String, so we can compare for deletion */
		sprintf(sUser,"%s", exitinfo.sUserName);

		if((strcmp(sUser, ol.UserName)) != 0) {
			if((!SYSOP) && (exitinfo.Security.level < CFG.sysop_access)) {
				colour(12, 0);
				/* Record *//* does not belong to you.*/
				printf("\n%s%s %s\n\n", (char *) Language(332), stemp, (char *) Language(333));
				Syslog('!', "User tried to delete somebody else's record: %s", stemp);
				Pause();
				fclose(pOneline);
				return;
			}
		}

		if ((ol.Available ) == FALSE) {
			colour(12, 0);
			/* Record: %d already marked for deletion			*/
			printf("\n%s%d %s\n\n", (char *) Language(332), nrecno, (char *) Language(334));
			Syslog('!', "User tried to mark an already marked record: %s", stemp);
			Pause();
		} else {
			ol.Available = FALSE;
			colour(10, 0);
			/* Record *//* marked for deletion */
			printf("\n%s%d %s\n\n", (char *) Language(332), nrecno, (char *) Language(334));
			Syslog('+', "User marked oneliner record for deletion: %s", stemp);
			Pause();
		}

		if (fseek(pOneline, offset, 0) != 0)
			WriteError("Can't move pointer in %s",sFileName); 
		fwrite(&ol, olhdr.recsize, 1, pOneline);
	}
	fclose(pOneline);
	free(sFileName);
}


