/*****************************************************************************
 *
 * File ..................: bbs/safe.c
 * Purpose ...............: Safe Door
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
#include "exitinfo.h"
#include "funcs.h"
#include "funcs4.h"
#include "misc.h"
#include "safe.h"
#include "timeout.h"
#include "language.h"



FILE *pSafe;

int iLoop, iFirst, iSecond, iThird;
char sFirst[4], sSecond[4], sThird[4];


int  getdigits(void);
int  SafeCheckUser(void);



void Safe(void)
{
	long	isize;
	int	i;

	isize = sizeof(int);
	srand(Time_Now);

	WhosDoingWhat(SAFE);

	Syslog('+', "User starts Safe Cracker Door");

	Enter(1);
	/* Safe Cracker Door */
	pout(15, 0, (char *) Language(86));
	Enter(1);

	clear();

	DisplayFile(CFG.sSafeWelcome);

	if (SafeCheckUser() == TRUE)
		return;

	/* In the safe lies */
	pout(15, 0, (char *) Language(87));

	fflush(stdout);
	alarm_on();
	Getone();

	clear();

	Enter(2);
	pout(10, 0, (char *) Language(88));
	Enter(2);

	colour(13, 0);
	printf("%s", CFG.sSafePrize);

	Enter(2);
	/* Do you want to open the safe ? [Y/n]: */
	pout(15, 0, (char *) Language(102));

	fflush(stdout);
	alarm_on();
	i = toupper(Getone());

	if (i == Keystroke(102, 1)) {
		Syslog('+', "User exited Safe Cracker Door");
		return;
	}

	/*
	 * Loop until the safe is opened, maximum trys
	 * exceeded or the user is tired of this door.
	 */
	while (TRUE) {
		/* Get digits, TRUE if safe cracked. */
		if (getdigits() == TRUE)
			break;

		Enter(1);
		/* Do you want to try again ? [Y/n]: */
		pout(12, 0, (char *) Language(101));
		fflush(stdout);

		alarm_on();
		i = toupper(Getone());
		if (i == Keystroke(101, 1)) 
			break;

		if (SafeCheckUser() == TRUE)
			break;
	}
	Syslog('+', "User exited Safe Cracker Door");
}



/*
 * Ask use for digits, returns TRUE if the safe is cracked.
 */
int getdigits(void)
{
	long	result;
	int	i;
	char	temp[81];

	colour(15, 0);
	/* Please enter three numbers consisting from 1 to */
	printf("\n\n%s%d\n", (char *) Language(89), CFG.iSafeMaxNumber);
	/* Please enter three combinations. */
	printf("%s", (char *) Language(90));

	while (TRUE) {
		Enter(2);
		/* 1st Digit */
		pout(12, 0, (char *) Language(91));
		colour(9, 0);
		fflush(stdout);
		Getnum(sFirst, 2);
		sprintf(temp, "1st: %s", sFirst);
		if((strcmp(sFirst, "")) != 0) {
			Syslog('-', temp);
			iFirst=atoi(sFirst);
		}

		if((iFirst > CFG.iSafeMaxNumber) || (iFirst <= 0) || (strcmp(sFirst, "") == 0)) {
			colour(15,1);
			/* Please try again! You must input a number greater than Zero and less than */
			printf("\n%s%d.", (char *) Language(92), CFG.iSafeMaxNumber);
       		        Syslog('-', "Value out of range!");
		} else 
			break;
	}

	while (TRUE) {
		Enter(1);
		/* 2nd digit: */
		pout(12, 0, (char *) Language(93));
		colour(9, 0);
		fflush(stdout);
		Getnum(sSecond, 2);
		sprintf(temp, "2nd: %s", sSecond);
		if((strcmp(sSecond, "")) != 0) {
			Syslog('-', temp);
			iSecond=atoi(sSecond);
		}

		if((iSecond > CFG.iSafeMaxNumber) || (iSecond <= 0) || (strcmp(sSecond, "") == 0)) {
			colour(15,1);
			/* Please try again! You must input a number greater than Zero and less than */
			printf("\n%s%d.\n", (char *) Language(92), CFG.iSafeMaxNumber);
			Syslog('-', "Value out of range!");
		} else
			break;
	}

	while (TRUE) {
		Enter(1);
		pout(12, 0, (char *) Language(94));
		colour(9, 0);
		fflush(stdout);
		Getnum(sThird, 2);
		if((strcmp(sThird, "")) != 0) {
			if((strcmp(sThird, "737") != 0) && (strcmp(sThird, "747") != 0)) {
				sprintf(temp, "3rd: %s", sThird);
				Syslog('!', temp);
			} else {
				sprintf(temp, "3rd: %d", CFG.iSafeMaxNumber - 1);
				Syslog('-', temp);
			}
		}
 		iThird=atoi(sThird);

		if((iThird == 737) || (iThird == 747))
			break;

		if((iThird > CFG.iSafeMaxNumber) || (iThird <= 0)) {
			colour(15,1);
			/* Please try again! You must input a number greater than Zero and less than */
			printf("\n%s%d.\n", (char *) Language(92), CFG.iSafeMaxNumber);
       		        Syslog('-', "Value out of range!");
		} else
			break;
	}

	/* Left: */
	Enter(1);
	pout(12, 0, (char *) Language(95));
	poutCR(9, 0, sFirst);

	/* Right: */
	pout(12, 0, (char *) Language(96));
	poutCR(9, 0, sSecond);

	/* Left: */
	pout(12, 0, (char *) Language(95));
	poutCR(9, 0, sThird);

	Enter(1);
	/* Attempt to open safe with this combination [Y/n]: */
	pout(12, 0, (char *) Language(97));
	fflush(stdout);
	alarm_on();
	i = toupper(Getone());
	sprintf(temp, "%c", i);

	if ((i == Keystroke(97, 0)) || (i == 13)) {
		printf("\n\n");

		/* Left: */
		pout(12, 0, (char *) Language(95));
		for (iLoop = 0; iLoop < iFirst; iLoop++)
			pout(14, 0, (char *)".");
		poutCR(9, 0, sFirst);

		/* Right: */
		pout(12, 0, (char *) Language(96));
		for (iLoop = 0; iLoop < iSecond; iLoop++)
			pout(14, 0, (char *)".");
		poutCR(9, 0, sSecond);

		/* Left: */
		pout(12, 0, (char *) Language(95));
		for (iLoop = 0; iLoop < iThird; iLoop++)
			pout(14, 0, (char *)"."); 
		poutCR(9, 0, sThird);

		if(CFG.iSafeNumGen) {
			CFG.iSafeFirstDigit  = (rand() % CFG.iSafeMaxNumber) + 1;
			CFG.iSafeSecondDigit = (rand() % CFG.iSafeMaxNumber) + 1;
			CFG.iSafeThirdDigit  = (rand() % CFG.iSafeMaxNumber) + 1;
		}

		if(CFG.iSafeFirstDigit == iFirst)
			if(CFG.iSafeSecondDigit == iSecond)
				if(CFG.iSafeThirdDigit == iThird) {

					DisplayFile(CFG.sSafeOpened);

					Enter(1);
					/* You have won the following... */
					pout(12, 0, (char *) Language(98));
					Enter(2);
					poutCR(13, 0, CFG.sSafePrize);
					Enter(1);

			 		sprintf(temp, "%s/etc/safe.data", getenv("MBSE_ROOT"));
					if(( pSafe = fopen(temp, "r+")) == NULL)
						WriteError("Can't open %s for updating", temp);
					else {
						fseek(pSafe, 0L, SEEK_END);
						result = ftell(pSafe);
						result /= sizeof(safe);

						fread(&safe, sizeof(safe), 1, pSafe);

						safe.Opened = TRUE;

						fseek(pSafe, 0L, SEEK_END);
						result = ftell(pSafe);
						result /= sizeof(safe);
						fwrite(&safe, sizeof(safe), 1, pSafe);
						fclose(pSafe);
					}

					Syslog('!', "User opened Safe Cracker Door");

					Pause();
					return TRUE;
		}

		Enter(1);
		pout(10, 0, (char *) Language(99));
		Enter(1);

		if(CFG.iSafeNumGen) {
			Enter(1);
			/* The safe code was: */
			pout(12, 0, (char *) Language(100));
			Enter(2);
			colour(12, 0);

			/* Left: */
			printf("%s%d\n", (char *) Language(95), CFG.iSafeFirstDigit);

			/* Right */
			printf("%s%d\n", (char *) Language(96), CFG.iSafeSecondDigit);

			/* Left */
			printf("%s%d\n", (char *) Language(95), CFG.iSafeThirdDigit);
		}

		if(iThird == 737)
			CFG.iSafeNumGen = FALSE;

		if(iThird == 747) {
			colour(9, 0);
			printf("Code: %d %d %d\n", CFG.iSafeFirstDigit, CFG.iSafeSecondDigit, CFG.iSafeThirdDigit);
		}

		Enter(1);
		/* Please press key to continue */
		pout(10, 0, (char *) Language(87));
		alarm_on();
		getchar();
	}
	return FALSE;
}



/*
 * Returns true when safe already cracked or maximum trys exceeded
 */
int SafeCheckUser(void)
{
	int  Counter = 0;
	char *File, *Name, *Date;

	File = calloc(PATH_MAX, sizeof(char));
	Name = calloc(50, sizeof(char));
	Date = calloc(50, sizeof(char));

	sprintf(Name, "%s", exitinfo.sUserName);
	sprintf(Date, "%s", (char *) GetDateDMY());
	sprintf(File, "%s/etc/safe.data", getenv("MBSE_ROOT"));

	if(( pSafe = fopen(File, "r")) == NULL) {
		if((pSafe = fopen(File, "w")) != NULL) {
			sprintf(safe.Date, "%s", (char *) GetDateDMY());
			sprintf(safe.Name, "%s", Name);
			safe.Trys   = 0;
			safe.Opened = 0;
			fwrite(&safe, sizeof(safe), 1, pSafe);
			fclose(pSafe);
		}
	} else {
		while ( fread(&safe, sizeof(safe), 1, pSafe) == 1) {
			if(safe.Opened) {
				fclose(pSafe);
				Syslog('+', "Safe is currently LOCKED - exiting door.");

				/* THE SAFE IS CURRENTLY LOCKED */
				poutCR(15, 4, (char *) Language(103));
				Enter(1);
				colour(12, 0);

				/* has cracked the safe. */
				printf("%s, %s\n", safe.Name, (char *) Language(104));

				/* The safe will remain locked until the sysop rewards the user. */
				pout(10, 0, (char *) Language(105));
				Enter(2);
				Pause();
				fclose(pSafe);
				free(File);
				free(Name);
				free(Date);
				return TRUE;
			}
		}
		rewind(pSafe);

		fread(&safe, sizeof(safe), 1, pSafe);
		if((strcmp(Date, safe.Date)) != 0) {
			fclose(pSafe);
			if((pSafe = fopen(File, "w")) != NULL) {
				sprintf(safe.Date, "%s", (char *) GetDateDMY());
				sprintf(safe.Name, "%s", Name);
				safe.Trys   = 0;
				safe.Opened = 0;
				fwrite(&safe, sizeof(safe), 1, pSafe);
				fclose(pSafe);
			}
		} else {
			while ( fread(&safe, sizeof(safe), 1, pSafe) == 1) {
				if((strcmp(Name, safe.Name)) == 0)
					Counter++;
			}

			rewind(pSafe);

			if(Counter >= CFG.iSafeMaxTrys - 1) {
				Enter(2);
				/* Maximum trys per day exceeded */
				pout(15, 0, (char *) Language(106));
				Enter(1);
				sleep(3);
				fclose(pSafe);
				free(File);
				free(Name);
				free(Date);
				return TRUE;
			}

			fclose(pSafe);
		
			if(( pSafe = fopen(File, "a+")) == NULL)
				WriteError("Unable to append to safe.data", File);
			else {
				sprintf(safe.Date, "%s", (char *) GetDateDMY());
				sprintf(safe.Name, "%s", Name);
				safe.Trys   = 0;
				safe.Opened = 0;                              
				fwrite(&safe, sizeof(safe), 1, pSafe);
				fclose(pSafe);
			}
		}
	}
	free(File);
	free(Name);
	free(Date);
	return FALSE;
}


