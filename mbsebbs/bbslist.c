/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Handle BBS lists
 * ToDo ..................: Add use of new fields
 *			    Verify check at logon
 *			    Intro New BBS at logon
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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
#include "bbslist.h"
#include "funcs.h"
#include "input.h"
#include "language.h"



void BBS_Add(void)
{
	FILE 	*pBBSList;
	char	*sFileName;
	char	*temp;
	
	sFileName = calloc(PATH_MAX, sizeof(char));
	temp      = calloc(PATH_MAX, sizeof(char)); 
	
	sprintf(sFileName,"%s/etc/bbslist.data", getenv("MBSE_ROOT"));

	if((pBBSList = fopen(sFileName, "a+")) == NULL) {
		WriteError("Can't open file: %s", sFileName);
		free(temp);
		free(sFileName);
		return;
	}

	if (ftell(pBBSList) == 0) {
		/*
		 * The file looks new created, add header
		 */
		bbshdr.hdrsize = sizeof(bbshdr);
		bbshdr.recsize = sizeof(bbs);
		fwrite(&bbshdr, sizeof(bbshdr), 1, pBBSList);
	}

	if(exitinfo.GraphMode) {
		colour(9, 0);
		printf("[2J\n\t\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177);
		printf("\t\t\t\t%c%c", 177, 177);
		colour(15, 0);
		/* Adding BBS */
		printf(" %s", (char *) Language(300));
		colour(9, 0);
		printf("%c%c %c\n", 177, 177, 219);
		printf("\t\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c %c\n", 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 219);
		printf("\t\t\t\t  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n\n", 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 219);
	} else {
		printf("\n\t\t\t\t+--------------+\n");
		/* Adding BBS */
		printf("\t\t\t\t|  %s |\n", (char *) Language(300));
		printf("\t\t\t\t+--------------+\n\n");
	}

	memset(&bbs, 0, sizeof(bbs));

	while (TRUE) {
		/* BBS Name: */
		pout(15, 0, (char *) Language(301));
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		GetstrC(bbs.BBSName, 40);

		if((strlen(bbs.BBSName)) > 3)
			break;
		else {
			Enter(1);
			pout(12, 0, (char *) Language(302));
			Enter(2);
			Pause();
			free(temp);
			free(sFileName);
			return;
		}
	}

	while (TRUE) {
		/* Phone Number: */
		pout(15, 0, (char *) Language(303));
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		GetstrC(bbs.Phone[0], 19);

		if((strlen(bbs.Phone[0])) > 3)
			break;
	}

	while (TRUE) {
		/* Sysop Name: */
		pout(15, 0, (char *) Language(304));
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		Getname(bbs.Sysop, 35);

		if((strlen(bbs.Sysop)) > 3)
			break;
	}

	while (TRUE) {
		/* BBS Software: */
		pout(15, 0, (char *) Language(305));
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		GetstrC(bbs.Software, 19);

		if((strlen(bbs.Software)) >= 2)
			break;
	}

	while (TRUE) {
		/* Storage (Gigabyte): */
		pout(15, 0, (char *) Language(306));
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		Getnum(temp, 8);

		if ((strlen(temp)) > 0) {
			bbs.Storage = atoi(temp);
			break;
		}
	}

	while (TRUE) {
		/* Speeds: */
		pout(15, 0, (char *) Language(307));
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		GetstrC(bbs.Speeds[0], 39);

		if((strlen(bbs.Speeds[0])) > 2)
			break;
	}

	Enter(1);
	/* Would you like to add a extended discription? [Y/n]: */
	pout(15, 0, (char *) Language(308));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp, 80);
	if ((toupper(temp[0]) == Keystroke(308, 0)) || (strcmp(temp, "") == 0)) {
		colour(14, 0);
		/* Please a enter discription for */
		printf("\n%s%s (2 Lines)\n", (char *) Language(309), bbs.BBSName);
		pout(15, 0, (char *)": ");
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		GetstrC(bbs.Desc[0], 65);
		pout(15, 0, (char *)": ");
		colour(CFG.InputColourF, CFG.InputColourB);
		fflush(stdout);
		GetstrC(bbs.Desc[1], 65);
	} 

	printf("\n");
	Syslog('+', "User added BBS to list");

	sprintf(bbs.UserName,"%s", exitinfo.sUserName);
	sprintf(bbs.DateOfEntry,"%02d-%02d-%04d",l_date->tm_mday,l_date->tm_mon+1,l_date->tm_year+1900);
	sprintf(bbs.Verified,"%02d-%02d-%04d",l_date->tm_mday,l_date->tm_mon+1,l_date->tm_year+1900);
	bbs.Available = TRUE;

	fwrite(&bbs, sizeof(bbs), 1, pBBSList);
	fclose(pBBSList);
	chmod(sFileName, 0660);
	free(temp);
	free(sFileName);
}



void BBS_List(void)
{
	FILE	*pBBSList;
	int	recno = 0;
	char	*sFileName;

	sFileName = calloc(PATH_MAX, sizeof(char));
	sprintf(sFileName,"%s/etc/bbslist.data", getenv("MBSE_ROOT"));

	if((pBBSList = fopen(sFileName, "r+")) == NULL) {
		WriteError("BBSList: Can't open file: %s", sFileName);
		free(sFileName);
		return;
	}

	fread(&bbshdr, sizeof(bbshdr), 1, pBBSList);

	if(exitinfo.GraphMode) {
		colour(9, 0);
		printf("[2J\n\t\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177);
		printf("\t\t\t\t%c%c", 177, 177);
		colour(15, 0);
		printf(" %s", (char *) Language(310));
		colour(9, 0);
		printf("%c%c %c\n", 177, 177, 219);
		printf("\t\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c %c\n", 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 219);
		printf("\t\t\t\t  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n\n", 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 219);
	} else {
		printf("\n\t\t\t\t+---------------+\n");
		/* BBS Listing */
		printf("\t\t\t\t|  %s |\n", (char *) Language(310));
		printf("\t\t\t\t+---------------+\n\n");
	}

	/* #    BBS Name               Number         Software        Gigabyte   Speed*/
	colour(15, 0);
	printf("%s\n", Language(311));
	colour(12, 0);
	sLine();

	while (fread(&bbs, bbshdr.recsize, 1, pBBSList) == 1) {
		if ((bbs.Available)) {
			colour(15, 0);
			printf("%-5d", recno);

			colour(10, 0);
			bbs.BBSName[22] = '\0';
			printf("%-23s", bbs.BBSName);

			colour(11, 0);
			bbs.Phone[0][14] = '\0';
			printf("%-15s", bbs.Phone[0]);

			colour(14, 0);
			bbs.Software[15] = '\0';
			printf("%-16s", bbs.Software);

			colour(13, 0);
			printf("%-11d", bbs.Storage);

			colour(8, 0);
			bbs.Speeds[0][9] = '\0';
			printf("%s\n", bbs.Speeds[0]);
		}
		recno++;
	}
	colour(12, 0);
	sLine();
	fclose(pBBSList);
	free(sFileName);
	Pause();
}



void BBS_Search(void)
{
	FILE	*pBBSList;
	int	recno = 0;
	int	iFoundBBS = FALSE;
	char	*sFileName;
	char	*Name;
	char	*sTemp;
	long	offset;

	sFileName = calloc(PATH_MAX, sizeof(char));
	sprintf(sFileName,"%s/etc/bbslist.data", getenv("MBSE_ROOT"));

	if((pBBSList = fopen(sFileName, "r+")) == NULL) {
		WriteError("BBSList: Can't open file: %s", sFileName);
		free(sFileName);
		return;
	}

	fread(&bbshdr, sizeof(bbshdr), 1, pBBSList);
	Name  = calloc(30, sizeof(char));
	sTemp = calloc(81, sizeof(char));

	if(exitinfo.GraphMode) {
		colour(9, 0);
		printf("[2J\n\t\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177);
		printf("\t\t\t\t%c%c ", 177, 177);
		colour(15, 0);
		printf("%s", (char *) Language(312));
		colour(9, 0);
		printf("%c%c %c\n", 177, 177, 219);
		printf("\t\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c %c\n", 177, 177, 177, 177, 177, 177, 177, 177, 177, 177,177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177,219);
		printf("\t\t\t\t  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n\n", 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220,219);
	} else {
		printf("\n\t\t\t\t+--------------------+\n");
		/* Search for a BBS */
		printf("\t\t\t\t  |  %s |\n", (char *) Language(312));
		printf("\t\t\t\t  +--------------------+\n\n");
	}

	while (TRUE) {
		/* Please enter 3 letters of BBS to search for: */
		pout(15, 0, (char *) Language(313));
		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(Name, 29);

		if((strcmp(Name,"")) == 0) {
			fflush(stdin);
			fclose(pBBSList);
			free(sFileName);
			free(sTemp);
			free(Name);
			return;
		}

		if((strlen(Name)) > 2)
			break;
		else {
			Enter(1);
			/* I need at least 3 letters ...*/
			pout(12, 0, (char *) Language(314));
			Enter(2);
		}
	}

	while (fread(&bbs, bbshdr.recsize, 1, pBBSList) == 1) {
		if((strstr(tl(bbs.BBSName), tl(Name)) != NULL)) {
			tlf(bbs.BBSName);
			colour(14, 0);
			/* BBS Name: */
			printf("\n%s%s\n\n", (char *) Language(301), bbs.BBSName);
			/* View this BBS? [Y/n]: */
			pout(15, 0, (char *) Language(315));
			colour(CFG.InputColourF, CFG.InputColourB);
			GetstrC(sTemp, 80);
			if ((toupper(sTemp[0]) == Keystroke(315, 0)) || (strcmp(sTemp,"") == 0)) {
				iFoundBBS = TRUE;
				break;
			} else
				recno++;
		} else {
			recno++;
		}
	}

	if(!iFoundBBS) {
		Enter(1);
		/* Could not find the BBS Listed ... */
		pout(12, 0, (char *) Language(316));
		Enter(2);
		fclose(pBBSList);
		Pause();
		free(sFileName);
		free(Name);
		free(sTemp);
		return;
	}

	offset = bbshdr.hdrsize + (recno * bbshdr.recsize);
	if(fseek(pBBSList, offset, 0) != 0) 
		WriteError("Can't move pointer there. %s",sFileName); 

	fread(&bbs, bbshdr.recsize, 1, pBBSList);
	if(exitinfo.GraphMode) {
		colour(9, 0);
		printf("[2J\n\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177);
		printf("\t\t\t%c%c ", 177, 177);
		colour(15, 0);
		/* Search for a BBS */
		printf("%s", (char *) Language(312));
		colour(9, 0);
		printf("%c%c %c\n", 177, 177, 219);
		printf("\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c %c\n", 177, 177, 177, 177, 177, 177, 177, 177, 177, 177,177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177,219);
		printf("\t\t\t  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n\n", 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220,219);
	} else {
		printf("\n\t\t\t+--------------------+\n");
		/* Search for a BBS */
		printf("\t\t\t|  %s |\n", (char *) Language(312));
		printf("\t\t\t+--------------------+\n\n");
	}

	/* #    BBS Name               Number         Software        Storage    Speed */
	colour(15, 0);
	printf("%s\n", Language(311));

	colour(12, 0);
	sLine();

	colour(15, 0);
	printf("%-5d", recno);

	colour(10, 0);
	bbs.BBSName[22] = '\0';
	printf("%-23s", bbs.BBSName);

	colour(11, 0);
	bbs.Phone[0][14] = '\0';
	printf("%-15s", bbs.Phone[0]);

	colour(14, 0);
	bbs.Software[15] = '\0';
	printf("%-16s", bbs.Software);

	colour(13, 0);
	printf("%-11d", bbs.Storage);

	colour(8, 0);
	bbs.Speeds[0][9] = '\0';
	printf("%s\n", bbs.Speeds[0]);

	colour(12, 0);
	sLine();
	fclose(pBBSList);
	Pause();
	free(sFileName);
	free(sTemp);
	free(Name);
}



void BBS_Show(void)
{
	FILE		*pBBSList;
	int		recno  = 0;
	int		nrecno = 0;
	long int	offset;
	char		*sFileName;
	char		srecno[10];

	sFileName = calloc(PATH_MAX, sizeof(char));
	sprintf(sFileName,"%s/etc/bbslist.data", getenv("MBSE_ROOT"));

	if((pBBSList = fopen(sFileName, "r+")) == NULL) {
		WriteError("Can't open file: %s", sFileName);
		free(sFileName);
		return;
	}
	free(sFileName);
	fread(&bbshdr, sizeof(bbshdr), 1, pBBSList);

	if(exitinfo.GraphMode) {
		colour(9, 0);
		printf("[2J\n\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177);
		printf("\t\t\t%c%c ", 177, 177);
		colour(15, 0);
		/* Show a BBS */
		printf("%s", (char *) Language(317));
		colour(9, 0);
		printf("%c%c %c\n", 177, 177, 219); 
		printf("\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c %c\n", 177, 177, 177, 177, 177,177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 219);
		printf("\t\t\t  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n\n", 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220,219);
	} else {
		printf("\n\t\t\t+--------------+\n");
		/* Show a BBS */
		printf("\t\t\t|  %s |\n", (char *) Language(317));
		printf("\t\t\t+--------------+\n\n");
	}

	Enter(1);
	/* Please enter number to list: */
	pout(15, 0, (char *) Language(318));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(srecno, 9);

	if((strcmp(srecno,"")) == 0)
		return;

	recno = atoi(srecno);
	nrecno = recno;
	recno = 0;

	while (fread(&bbs, bbshdr.recsize, 1, pBBSList) == 1) 
		recno++;

	if(nrecno >= recno) {
		Enter(1);
		/* Record does not exist */
		pout(12, 0, (char *) Language(319));
		Enter(2);
		fclose(pBBSList);
		Pause();
		return;
	} else {
		offset = bbshdr.hdrsize + (nrecno * bbshdr.recsize);
		if(fseek(pBBSList, offset, 0) != 0)
			WriteError("Can't move pointer there. %s",sFileName);
                                                             
		fread(&bbs, bbshdr.recsize, 1, pBBSList);

		colour(12, 0);
		sLine();

		/*  Record        : */
		pout(15, 0, (char *) Language(320));
		colour(10, 0);
		printf("%d\n", nrecno);

		/*  BBS Name      : */
		pout(15, 0, (char *) Language(321));
		colour(10, 0);	
		printf("%s\n", bbs.BBSName);

		/*  Number        : */
		pout(15, 0, (char *) Language(322));
		colour(10, 0);
		printf("%s\n", bbs.Phone[0]);

		/* Software      : */
		pout(15, 0, (char *) Language(323));
		colour(10, 0);
		printf("%s\n", bbs.Software);

		/* Storage       : */
		pout(15, 0, (char *) Language(324));
		colour(10, 0);
		printf("%d\n", bbs.Storage);

		/*  Speeds        : */
		pout(15, 0, (char *) Language(325));
		colour(10, 0);
		printf("%s\n", bbs.Speeds[0]);

		/*  Sysop Name    : */
		pout(15, 0, (char *) Language(326));
		colour(10, 0);
		printf("%s\n", bbs.Sysop);

		if((strcmp(bbs.Desc[0],"")) != 0) {
			pout(15, 0, (char *)" Description   : ");
			colour(13, 0);
			bbs.Desc[0][62] = '\0';
			printf("%s\n", bbs.Desc[0]);
		}
		if((strcmp(bbs.Desc[1],"")) != 0) {
			pout(15, 0, (char *)"               : ");
			colour(13, 0);
			bbs.Desc[1][62] = '\0';
			printf("%s\n", bbs.Desc[1]);
		}

		colour(12, 0);
		sLine();

		if((SYSOP == TRUE) || (exitinfo.Security.level >= CFG.sysop_access)) {
			pout(15, 0, (char *)"Sysop extra information\n");
			colour(12, 0);
			sLine();

			/*  Available     : */
			pout(15, 0, (char *) Language(327));
			colour(10, 0);
			printf("%d\n", bbs.Available);

			/*  Date of Entry : */
			pout(15, 0, (char *) Language(328));
			colour(10, 0);
			printf("%s\n", bbs.DateOfEntry);

			/*  Entry Name    : */
			pout(15, 0, (char *) Language(329));
			colour(10, 0);
			printf("%s\n", bbs.UserName);

			colour(12, 0);
			sLine();
		}
		Pause();
	}
	fclose(pBBSList);
}



void BBS_Delete(void)
{
    FILE    *pBBSLine;
    int	    recno = 0, nrecno = 0;
    long    offset;
    char    srecno[7], *sFileName, stemp[50], sUser[35];

    sFileName = calloc(PATH_MAX, sizeof(char));
    sprintf(sFileName,"%s/etc/bbslist.data", getenv("MBSE_ROOT"));

    if ((pBBSLine = fopen(sFileName, "r+")) == NULL) {
	WriteError("Can't open file: %s", sFileName);
	free(sFileName);
	return;
    }
    fread(&bbshdr, sizeof(bbshdr),1 , pBBSLine);

    if (exitinfo.GraphMode) {
	colour(9, 0);
	printf("[2J\n\t\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177);
	printf("\t\t\t\t%c%c", 177, 177);
	colour(15, 0);
	/* Delete BBS */
	printf(" %s", (char *) Language(330));
	colour(9, 0);
	printf("%c%c %c\n", 177, 177, 219);
	printf("\t\t\t\t%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c %c\n",  177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 219);
	printf("\t\t\t\t  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n\n", 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 220, 219);
    } else {
	printf("\n\t\t\t\t+--------------+\n");
	/* Delete BBS */
	printf("\t\t\t\t|  %s |\n", (char *) Language(330));
	printf("\t\t\t\t+--------------+\n\n");
    }

    Enter(1);
    /* Please enter number to delete: */
    pout(15, 0, (char *) Language(331));
    colour(CFG.InputColourF, CFG.InputColourB);
    GetstrC(srecno, 9);

    if ((strcmp(srecno,"")) == 0)
	return;

    recno = atoi(srecno);
    nrecno = recno;
    recno = 0;
		                                                                                           
    while (fread(&bbs, bbshdr.recsize, 1, pBBSLine) == 1)
	recno++;

    if (nrecno >= recno) {
	Enter(1);
	/* Record does not exist */
	pout(12, 0, (char *) Language(319));
	Enter(2);
	fclose(pBBSLine);
	free(sFileName);
	Pause();
	return;
    } else {
	offset = bbshdr.hdrsize + (nrecno * bbshdr.recsize);
	if (fseek(pBBSLine, offset, 0) != 0)
	    WriteError("Can't move pointer there. %s",sFileName); 

	fread(&bbs, sizeof(bbs), 1, pBBSLine);
	                                                                                            
	/* Convert Record Int to string, so we can print to logfiles */
	sprintf(stemp,"%d", nrecno);

	/* Print UserName to String, so we can compare for deletion */
	sprintf(sUser,"%s", exitinfo.sUserName);

	if ((strcmp(sUser, bbs.UserName)) != 0) {
	    if ((!SYSOP) && (exitinfo.Security.level < CFG.sysop_access)) {
		/* Record */ /* does not belong to you.*/
		printf("\n%s%s %s\n\n", (char *) Language(332), stemp, (char *) Language(333));
		Syslog('!', "User tried to delete somebody else's bbslist record: %s", stemp);
		free(sFileName);
		fclose(pBBSLine);
		return;
	    }
	}

	if ((bbs.Available == FALSE)) {
	    colour(12, 0);
	    /* Record */
	    printf("\n%s%d %s\n\n", (char *) Language(332), nrecno, (char *) Language(334));
	    Syslog('!', "User tried to mark an already marked bbslist record: %s", stemp);
	} else {
	    bbs.Available = FALSE;
	    colour(10, 0);
	    /* Record: */
	    printf("\n%s%d %s\n\n", (char *) Language(332), nrecno, (char *) Language(335));
	    Syslog('+', "User marked bbslist record for deletion: %s", stemp);
	    colour(15, 2);
	    /* The Sysop will purge the list once he has *//* seen you have marked a record for deletion. */
	    printf("%s\n%s\n\n", (char *) Language(336), (char *) Language(337));
	    Pause();
	}

	offset = bbshdr.hdrsize + (nrecno * bbshdr.recsize);
	if (fseek(pBBSLine, offset, 0) != 0)
	    WriteError("Can't move pointer there. %s",sFileName); 
	fwrite(&bbs, sizeof(bbs), 1, pBBSLine);
    }

    fclose(pBBSLine);
    chmod(sFileName, 0660);
    free(sFileName);
}


