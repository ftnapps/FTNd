/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Time/Bytes Bank
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
 *   
 * Michiel Broek		FIDO:		2:2801/16
 * Beekmansbos 10		Internet:	mbroek@ux123.pttnwb.nl
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
#include "bank.h"
#include "input.h"
#include "language.h"
#include "dispfile.h"
#include "timeout.h"
#include "timecheck.h"
#include "whoson.h"
#include "exitinfo.h"


/*
 * internal function prototypes
 */
int  BankMenu(void);
void AddAccount(void);
void Deposit(void);
void Withdraw(void);
void DepositTime(void); 
void WithdrawTime(void);

void DepositBytes(void);
void WithdrawBytes(void);

FILE *pBank;
int  usernum = 0;
long bankset;



/*
 * Timebank, called from the menu system.
 */
void Bank()
{
	int	FoundName = FALSE;
	int	Loop = TRUE;
	char	*temp, temp1[81];

	DisplayFile((char *)"bank");
	temp = calloc(PATH_MAX, sizeof(char));

	while(Loop) {
		FoundName = FALSE;
		usernum = 0;
		WhosDoingWhat(TIMEBANK);

		sprintf(temp, "%s/etc/bank.data", getenv("MBSE_ROOT"));
		if ((pBank = fopen(temp, "r+")) == NULL) {
			/* Create a new database */
			pBank = fopen(temp, "a+");
			bankhdr.hdrsize = sizeof(bankhdr);
			bankhdr.recsize = sizeof(bank);
			fwrite(&bankhdr, sizeof(bankhdr), 1, pBank);
			fclose(pBank);
			chmod(temp, 0660);
			Syslog('-', "Created %s", temp);
			AddAccount();
			if ((pBank = fopen(temp, "r+")) == NULL) {
				WriteError("Unable to open %s", temp);
				free(temp);
				return;
			}
		}

		fread(&bankhdr, sizeof(bankhdr), 1, pBank);
		fseek(pBank, bankhdr.hdrsize, 0);
		while (fread(&bank, bankhdr.recsize, 1, pBank) == 1) {
			if((strcmp(bank.Name, exitinfo.sUserName)) == 0) {
				FoundName = TRUE;
				break;
			} else 
				usernum++;
		} /* End of while */

		fclose(pBank);

		if(!FoundName)
			AddAccount();

		if ((pBank = fopen(temp, "r+")) == NULL)
			AddAccount();

		bankset = bankhdr.hdrsize + (usernum * bankhdr.recsize);
		if (fseek(pBank, bankset, 0) != 0)
			WriteError("Can't move pointer there.");

		fread(&bank, bankhdr.recsize, 1, pBank);

		sprintf(temp1, "%s", (char *) GetDateDMY());
		if ((strcmp(temp1, bank.Date)) != 0) {
			bank.TimeWithdraw  = 0;
			bank.KByteWithdraw = 0;
			bank.TimeDeposit   = 0;
			bank.KByteDeposit  = 0;
			sprintf(bank.Date, "%s", (char *) GetDateDMY());
		}

 		Loop = BankMenu();
	}
	free(temp);
}



int BankMenu()
{
	int i;

	clear();
	/*                          MBSE BBS System Bank         */
	language(4, 7, 11);
	colour(15, 0);
	sLine();
	/* Bank Account: */
	language(3, 0, 12);
	poutCR(15, 0, bank.Name);
	colour(15, 0);
	sLine();

	/* Time in account                    */
	language(10, 0, 13);
	colour(15, 0);
	printf("%d\n", bank.TimeBalance);

	/* Bytes in account                   */
	language(10, 0, 14);
	colour(15, 0);
	printf("%d\n", bank.KByteBalance);

	/* Time deposited today               */
	language(10, 0, 15);
	colour(15, 0);
	printf("%d\n", bank.TimeDeposit);

	/* Bytes deposited today              */
	language(10, 0, 16);
	colour(15, 0);
	printf("%d\n", bank.KByteDeposit);

	/* Time withdrawn today               */
	language(10, 0, 17);
	colour(15, 0);
	printf("%d\n", bank.TimeWithdraw);

	/* Bytes withdrawn today              */
	language(10, 0, 18);
	colour(15, 0); 
	printf("%d\n", bank.KByteWithdraw);

	colour(15, 0);
	sLine();
	TimeCheck();
	/* (D)eposit, (W)ithdraw, (Q)uit */
	language(12, 0, 19);
	Enter(2);

	/* Bank > 	*/
	language(15, 0, 20);
	fflush(stdout);

	alarm_on();
	i = toupper(Getone());

	if (i == Keystroke(19, 0))
		Deposit();
	else
		if (i == Keystroke(19, 1))
			Withdraw();
		else
			if (i == Keystroke(19, 2))
				return FALSE;

	return TRUE;
}



void AddAccount()
{
	char *temp;
	                                  
	temp = calloc(PATH_MAX, sizeof(char));

	sprintf(temp, "%s/etc/bank.data", getenv("MBSE_ROOT"));

	if ((pBank = fopen(temp, "a+")) == NULL)
		WriteError("Can't open %s for updating", temp);
	else {
		memset(&bank, 0, sizeof(bank));
		strcpy(bank.Name, exitinfo.sUserName); 
		sprintf(bank.Date, "%s", (char *) GetDateDMY());
		fwrite(&bank, sizeof(bank), 1, pBank);
		fclose(pBank);
	}
	free(temp);
}



void Deposit()
{
	int i;

	/* (T)ime, (B)ytes, (Q)uit : */
	language(3, 0, 21);
	fflush(stdout);

	alarm_on();
	i = toupper(Getone());

	if (i == Keystroke(21, 0))
		DepositTime();
	else
		if (i == Keystroke(21, 1))
			DepositBytes();
}



void Withdraw()
{
	int i;

	/* (T)ime, (B)ytes, (Q)uit : */
	language(3, 0, 21);
	fflush(stdout);

	alarm_on();
	i = toupper(Getone());

	if (i == Keystroke(21, 0))
		WithdrawTime();
	else
		if (i == Keystroke(21, 1))
			WithdrawBytes();
}



void DepositTime()
{
	int x;
	char *temp;

	temp = calloc(PATH_MAX, sizeof(char));

	ReadExitinfo();
	fflush(stdin);

	if(exitinfo.iTimeLeft <= 5) {
		Enter(2);
		/* You must have at least 5 minutes remaining to deposit */
		language(12, 0, 22);
		Enter(2);
		Pause();
		free(temp);
		return;
	}

	Enter(1);
	/* How much time. Minutes available to you is */
	language(15, 0, 23);
	printf("%d: ", exitinfo.iTimeLeft - 5);
	colour(CFG.InputColourF, CFG.InputColourB);
 	GetstrC(temp, 80);

	if((strcmp(temp, "")) == 0) {
		free(temp);
		return;
	}

	x = atoi(temp);
	bank.TimeDeposit += x;
	bank.TimeBalance += x;

	if(bank.TimeDeposit > CFG.iMaxTimeDeposit) {
		colour(10, 0);
		Enter(1);
		/* You have tried to deposit more than the maximum limit today. */
		language(10, 0, 24);
		Enter(1);

		/* Maximum allowed minutes to deposit per day: */
		language(12, 0, 25);
		printf("%d.\n\n", CFG.iMaxTimeDeposit);
		Pause();
		fclose(pBank);
		free(temp);
		return;
	}

	if(bank.TimeBalance > CFG.iMaxTimeBalance) {
		Enter(1);
		/* You have exeeded your account balance. */
		language(10, 0, 26);
		/* Maximum allowable minutes in bank account is: */
		language(12, 0, 27);
		printf("%d\n", CFG.iMaxTimeBalance);
		/* You are allowed to deposit: */
		language(14, 0, 28);
		printf("%d\n\n", CFG.iMaxTimeBalance - (bank.TimeBalance - x) );
		Pause();
		fclose(pBank);
		free(temp);
		return;
	}

	Time2Go -= (x * 60);
	TimeCheck();

	bankset = bankhdr.hdrsize + (usernum * bankhdr.recsize);
	if(fseek(pBank, bankset, 0) != 0)
		WriteError("Can't move pointer there.");

	fwrite(&bank, sizeof(bank), 1, pBank); 
	fclose(pBank);

	free(temp);
}



void WithdrawTime()
{
	char *temp;
	int x;

	temp = calloc(PATH_MAX, sizeof(char));

	ReadExitinfo();

	/* How much time. Minutes available to you is */
	Enter(1);
	language(15, 0, 23);
	printf("%d: ", bank.TimeBalance);
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp, 80);

	if((strcmp(temp, "")) == 0) {
		free(temp);
		return;
	}

	x = atoi(temp);

	bank.TimeWithdraw += x;
	bank.TimeBalance -= x;

	if(bank.TimeWithdraw > CFG.iMaxTimeWithdraw) {
		Enter(1);
		/* You have tried to withdraw more than the maximum limit today. */
		language(10, 0, 29);
		/* Maximum allowed to withdraw per day: */
		language(12, 0, 30);
		printf("%d.\n\n", CFG.iMaxTimeWithdraw);
		Pause();
		fclose(pBank);
		free(temp);
		return;
	}

	if(x > bank.TimeBalance + x) {
		Enter(1);
		/* You have tried to withdraw more time than is in your bank account. */
		language(10, 0, 31);
		/* Current bank balance: 			*/
		language(10, 0, 32);
		printf("%d.\n\n", bank.TimeBalance + x);
		Pause();
		fclose(pBank);
		free(temp);
		return;
	}

	Time2Go += (x * 60);
	TimeCheck();

	bankset = bankhdr.hdrsize + (usernum * bankhdr.recsize);

	if(fseek(pBank, bankset, 0) != 0)
		WriteError("Can't move pointer there.");

	fwrite(&bank, sizeof(bank), 1, pBank);
	fclose(pBank);
	free(temp);
}



void DepositBytes()
{
	char *temp;
	int x;

	temp = calloc(PATH_MAX, sizeof(char));

	ReadExitinfo();

	if(exitinfo.DownloadKToday <= 5) {
		Enter(2);
		/* You must have at least 5 bytes remaining to deposit */
		language(12, 0, 22);
		Enter(2);
		Pause();
		free(temp);
		return;
	}

	Enter(1);
	/* Bytes available: */
	language(15, 0, 36);
	printf("%lu: ", exitinfo.DownloadKToday);
	colour(CFG.InputColourF, CFG.InputColourB);
 	GetstrC(temp, 80);

	if((strcmp(temp, "")) == 0) {
		free(temp);
		return;
	}

	x = atoi(temp);
	bank.KByteDeposit += x;
	bank.KByteBalance += x;

	if(bank.KByteDeposit > CFG.iMaxByteDeposit * 1000) {
		Enter(1);
		/* You have tried to deposit more than the maximum limit today. */
		language(10, 0, 25);
		Enter(1);
		colour(12, 0);
		/* Maximum allowed kilobytes to deposit per day: */
		language(12, 0, 33);
		printf("%d.\n\n", CFG.iMaxByteDeposit);
		Pause();
		fclose(pBank);
		free(temp);
		return;
	}

	if(bank.KByteBalance > CFG.iMaxByteBalance * 1000) {
		Enter(1);
		/* You have exeeded your account balance. */
		language(10, 0, 34);
		Enter(1);
		/* Maximum allowable kilobytes in bank account is: */
		language(12, 0, 35);
		printf("%d\n", CFG.iMaxByteBalance);
		colour(14, 0);
		/* You are allowed to deposit: */
		language(14, 0, 28);
		printf("%d\n\n", CFG.iMaxByteBalance - (bank.KByteBalance - x) );
		Pause();
		fclose(pBank);
		free(temp);
		return;
	}

	exitinfo.DownloadKToday -= x;
	WriteExitinfo();

	bankset = bankhdr.hdrsize + (usernum * bankhdr.recsize);

	if(fseek(pBank, bankset, 0) != 0)
		WriteError("Can't move pointer there.");

	fwrite(&bank, sizeof(bank), 1, pBank);
	fclose(pBank);
	free(temp);
}



void WithdrawBytes()
{
	char *temp;
	int x;

	temp = calloc(PATH_MAX, sizeof(char));

	ReadExitinfo();

	Enter(1);
	/* How many bytes. Bytes available to you is */
	language(15, 0, 36);
	printf("%d: ", bank.KByteBalance);
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp, 80);

	if((strcmp(temp, "")) == 0) {
		free(temp);
		return;
	}

	x = atoi(temp);
	bank.KByteWithdraw += x;
	bank.KByteBalance -= x;

	if(bank.KByteWithdraw > CFG.iMaxByteWithdraw * 1000) {
		Enter(1);
		/* You have tried to withdraw more than the maximum limit today. */
		language(10, 0, 29);
		Enter(1);
		colour(12, 0);
		/* Maximum allowed to withdraw per day: */
		language(12, 0, 30);
		printf("%d bytes.\n\n", CFG.iMaxByteWithdraw);
		Pause();
		fclose(pBank);
		free(temp);
		return;
	}

	if(x > bank.KByteBalance + x) {
		Enter(1);
		/* You have tried to withdraw more time than is in your bank account. */
		language(10, 0, 31);
		Enter(1);
		colour(12, 0);
		/* Current bank balance: */
		language(12, 0, 32);
		printf("%d.\n\n", bank.KByteBalance + x);
		Pause();
		fclose(pBank);
		free(temp);
		return;
	}

	exitinfo.DownloadKToday += x;
	WriteExitinfo();

	bankset = bankhdr.hdrsize + (usernum * bankhdr.recsize);

	if(fseek(pBank, bankset, 0) != 0)
		WriteError("Can't move pointer there.");

	fwrite(&bank, sizeof(bank), 1, pBank);
	fclose(pBank);
	free(temp);
}


