/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Message to next User door
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
#include "../lib/ansi.h"
#include "../lib/clcomm.h"
#include "../lib/common.h"
#include "nextuser.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "timeout.h"


int iLoop, iLine = 1;

char sFrom[81];
char To[81];
char Subject[81];

int  finish(void);
void addlines(void);

char *sLiNE[11];



void nextuser(void)
{
	int i;

	for (i = 0; i < 11; i++)
		*(sLiNE + i) = (char *) calloc(81, sizeof(char));

	addlines();
	while (TRUE) {
		if (finish() == TRUE)
			break;
	}

	for (i = 0; i < 11; i++)
		free(*(sLiNE + i));
}



void addlines(void)
{
	iLine = 1;
	clear();

	/* Message to Nextuser Door */
	pout(15, 0, (char *) Language(107));
	Enter(2);

	Syslog('+', "%s ran Nextuser Door", exitinfo.sUserName);

	/* The FROM, TO and SUBJECT fields are optional. */
	pout(14, 0, (char *) Language(108));
	Enter(1);

	Enter(1);
	/*    From: */
	pout(12, 0, (char *) Language(109));
	colour(9, 0);
	fflush(stdout);
 	Getname(sFrom, 50);

	/*      To: */
	pout(12, 0, (char *) Language(110));
	colour(9, 0);
	fflush(stdout);
	Getname(To, 50);

	/* Subject: */
	pout(12, 0, (char *) Language(111));
	colour(9, 0);
	fflush(stdout);
	GetstrC(Subject, 80);

	Enter(2);
	/*  Type up to 10 lines 74 Characters per line */
	pout(10 , 0, (char *) Language(112));
	Enter(1);

	colour(14, 0);
	printf("  ’");
	for(iLoop = 0; iLoop <= 75; iLoop++)
		printf("Õ");
	printf("∏");

	while (1) {
		colour(12, 0);
		printf("%d: ", iLine);
		colour(9, 0);
		fflush(stdout);
		GetstrC(*(sLiNE + iLine), 75);

		if ((strcmp(*(sLiNE + iLine), "")) == 0)
			return;

		iLine++;
		if(iLine >= 11)
			break;
	}

	pout(14, 0, (char *)"  ‘");
	for(iLoop = 0; iLoop <= 75; iLoop++)
		printf("Õ");
	printf("æ");
}



/* Save Abort File */
int finish(void)
{
	FILE	*pTextFileANS, *pTextFileASC;
	int	iStrLen, i, x, NLChk = FALSE;
	char	*temp, *temp1;

	temp  = calloc(PATH_MAX, sizeof(char));
	temp1 = calloc(PATH_MAX, sizeof(char));

	while (TRUE) {
		Enter(1);
		poutCR(15, 0, (char *) Language(113));
		Enter(1);
		/* (L)ist, (R)eplace text, (E)dit line, (A)bort, (S)ave */
		pout(15, 1, (char *) Language(114));
		Enter(2);

		/* Select: */
  		pout(15, 0, (char *) Language(115));

		fflush(stdout);

		alarm_on();
		i = toupper(Getone());

		if (i == Keystroke(114, 3)) {
			/* Aborting... */
			pout(15, 0, (char *) Language(116));
			Enter(1);
			Enter(1);
			pout(15, 0, (char *) Language(117));
			poutCR(15, 0, CFG.bbs_name);
			sleep(2);
			Syslog('+', "User aborted message and exited door");
			free(temp);
			free(temp1);
			return TRUE;
		} else

		if (i == Keystroke(114, 2)) {
			/* Edit which line: */
			printf("\n %s", (char *) Language(118));
			GetstrC(temp, 80);

			if((strcmp(temp, "")) == 0)
				break;

			i = atoi(temp);
			if( i > iLine - 1) {
				/* Line does not exist. */
				printf("%s\n", (char *) Language(119));
				break;
			}
			x = strlen(sLiNE[i]);
			printf("%d : %s", i, *(sLiNE + i));           
			fflush(stdout);
			GetstrP(sLiNE[i],74, x);				                                            
		} else 

		if (i == Keystroke(114, 0)) {
			colour(14, 0);
			printf("\n\n  ’");
			for (iLoop = 0; iLoop <= 75; iLoop++)
				printf("Õ");
			printf("∏");

			for(i = 1; i < iLine; i++) {
				colour(12, 0);
				printf("%d: ", i);
				colour(9, 0);
				printf("%s\n", *(sLiNE + i));
			}

			colour(14, 0);
			printf("  ‘");
			for (iLoop = 0; iLoop <= 75; iLoop++)
				printf("Õ");
			printf("æ\n");
		} else

		if (i == Keystroke(114, 4)) {
			/* Open TextFile for Writing NextUser Info */
			sprintf(temp, "%s/%s.ans", CFG.bbs_txtfiles, CFG.sNuScreen);
			sprintf(temp1, "%s/%s.ans.bak", CFG.bbs_txtfiles, CFG.sNuScreen);
			rename(temp, temp1);
	
			if((pTextFileANS = fopen(temp, "w")) == NULL) {
				perror("");
				WriteError("NextUser: Can't open file: %s", temp);
				return TRUE;
			}

			sprintf(temp, "%s/%s.asc", CFG.bbs_txtfiles, CFG.sNuScreen);
			if(( pTextFileASC = fopen(temp, "w")) == NULL) {
				perror("");
				WriteError("NextUser: Can't open file: %s", temp);
				return TRUE;
			}

			fprintf(pTextFileANS,"%s%s%s%s",ANSI_CLEAR,ANSI_NORMAL,ANSI_WHITE,ANSI_BOLD);

			if((iStrLen = strlen(sFrom)) > 1) {
				fprintf(pTextFileANS,"%s\x1B[1;4HFrom:%s %s\n",ANSI_RED,ANSI_BLUE,sFrom);
				fprintf(pTextFileASC,"\n   From: %s\n", sFrom);
				Syslog('+', "   From: %s", sFrom);
       				NLChk = TRUE;
			}

			if((iStrLen = strlen(To)) > 1) {
				fprintf(pTextFileANS,"%s\x1B[2;6HTo:%s %s\n",ANSI_RED,ANSI_BLUE,To);
				fprintf(pTextFileASC,"     To: %s\n", To);
				Syslog('+', "     To: %s", To);
       				NLChk = TRUE;
			}

			if((iStrLen = strlen(Subject)) > 1) {
				fprintf(pTextFileANS,"%sSubject:%s %s\n\n",ANSI_RED,ANSI_BLUE,Subject);
				fprintf(pTextFileASC,"Subject: %s\n\n", Subject);
				Syslog('+', "Subject: %s", Subject);
       				NLChk = TRUE;
			}

			if(!NLChk) {
				fprintf(pTextFileANS, "\n");
				fprintf(pTextFileASC, "\n");
			}

			fprintf(pTextFileANS,"%s’",ANSI_YELLOW);
			for(iLoop = 0; iLoop <= 75; iLoop++) {
				fprintf(pTextFileANS,"Õ");
				fprintf(pTextFileASC,"=");
			}
			fprintf(pTextFileANS,"∏\n");
			fprintf(pTextFileASC,"\n");

			for(i = 0; i < iLine; i++) {
				if((iStrLen = strlen( *(sLiNE + i) )) > 0) {
				   	fprintf(pTextFileANS," %s%s\n",ANSI_BLUE, *(sLiNE + i));
				   	fprintf(pTextFileASC," %s\n", *(sLiNE + i));
				}
			}

			Enter(2);
			pout(12, 0, (char *) Language(340));
			fprintf(pTextFileANS,"%s‘",ANSI_YELLOW);
			for(iLoop = 0; iLoop <= 75; iLoop++) {
				fprintf(pTextFileANS,"Õ");
				fprintf(pTextFileASC,"=");
			}
			fprintf(pTextFileANS,"æ\n");
			fprintf(pTextFileASC,"\n");

			fprintf(pTextFileANS,"%s%s",ANSI_RED,CFG.sNuQuote);
			fprintf(pTextFileASC,"%s", CFG.sNuQuote);

			fclose(pTextFileANS);
			fclose(pTextFileASC);   
			colour(9, 0);
			/* Returning to */
			printf("\n%s%s\n", (char *) Language(117), CFG.bbs_name);
		
			Syslog('+', "User Saved Nextuser message and exited door");
			free(temp);
			free(temp1);
			return TRUE;
		} else

		if (i == Keystroke(114, 1)) {
			/* Edit which line: */
			colour(15, 0);
			printf("\n%s", (char *) Language(118));
			GetstrC(temp, 80);

			if((strcmp(temp, "")) == 0)
				break;

			i = atoi(temp);

			if( i > iLine - 1) {
				/* Line does not exist. */
				printf("\n%s", (char *) Language(119));
				break;
			}

			Enter(1);
			/* Line reads: */
			colour(15, 0);
			poutCR(15, 0, (char *) Language(186));
			printf("%2d: %s\n", i, *(sLiNE + i));

			Enter(1);
			/* Text to replace: */
			pout(15, 0, (char *) Language(195));
			GetstrC(temp, 80);

			if((strcmp(temp, "")) == 0)
				break;

			/* Replacement text: */
			pout(15, 0, (char *) Language(196));
			GetstrC(temp1, 80);

			if((strcmp(temp1, "")) == 0)
				break;

			strreplace(*(sLiNE + i), temp, temp1);
		} else
			printf("\n");
	}
	free(temp);
	free(temp1);
	return FALSE;
}

