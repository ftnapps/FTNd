/*****************************************************************************
 *
 * $Id: oneline.c,v 1.15 2007/02/26 14:48:23 mbse Exp $
 * Purpose ...............: Oneliner functions.
 *
 *****************************************************************************
 * Copyright (C) 1997-2005
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
#include "oneline.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "term.h"
#include "ttyio.h"


char	sOneliner[81];
int	iColour;		/* Current color	*/


void Oneliner_Check()
{
    FILE    *pOneline;
    char    *sFileName;

    sFileName = calloc(PATH_MAX, sizeof(char));
    snprintf(sFileName, PATH_MAX, "%s/etc/oneline.data", getenv("MBSE_ROOT"));

    if ((pOneline = fopen(sFileName, "r")) == NULL) {
	if ((pOneline = fopen(sFileName, "w")) != NULL) {
	    olhdr.hdrsize = sizeof(olhdr);
	    olhdr.recsize = sizeof(ol);
	    fwrite(&olhdr, sizeof(olhdr), 1, pOneline);
	    fclose(pOneline);
	    Syslog('-', "Created oneliner database");
	}
    } else {
	fclose(pOneline);
    }

    chmod(sFileName, 0660);
    free(sFileName);
}



void Oneliner_Add()
{
    FILE    *pOneline;
    char    *sFileName;
    int	    x;
    char    temp[81];

    Oneliner_Check();

    sFileName = calloc(PATH_MAX, sizeof(char));
    snprintf(sFileName, PATH_MAX, "%s/etc/oneline.data", getenv("MBSE_ROOT"));

    if ((pOneline = fopen(sFileName, "a+")) == NULL) {
	WriteError("Can't open file: %s", sFileName); 
	return;
    }
    free(sFileName);

    memset(&ol, 0, sizeof(ol));
    clear();
    /* MBSE BBS Oneliners will randomly appear on the main menu. */
    poutCR(WHITE, BLACK, Language(341));
    Enter(1);

    /* Obscene or libellous oneliners will be deleted!! */
    poutCR(WHITE, BLUE, Language(342));
    Enter(1);

    /* Please enter your oneliner below. You have 75 characters.*/
    poutCR(LIGHTRED, BLACK, Language(343));
    pout(WHITE, BLACK, (char *)"> ");
    colour(CFG.InputColourF, CFG.InputColourB);
    GetstrC(temp, 75);
			
    if ((strcmp(temp, "")) == 0) {
	fclose(pOneline);
	return;
    } else {
	x = strlen(temp);
	if (x >= 78)
	    temp[78] = '\0';
				
	strcpy(ol.Oneline, temp);
    }
		
    Enter(1);
    /* Oneliner added */
    pout(CYAN, BLACK, Language(344));
    Enter(2);
    Pause();

    Syslog('!', "User added oneliner:");
    Syslog('!', ol.Oneline);
		
    snprintf(ol.UserName,36,"%s", exitinfo.sUserName);
    snprintf(ol.DateOfEntry,12,"%02d-%02d-%04d",l_date->tm_mday,l_date->tm_mon+1,l_date->tm_year+1900);
    ol.Available = TRUE;

    fwrite(&ol, sizeof(ol), 1, pOneline);
    fclose(pOneline);
}




/* 
 * Print global string sOneliner centered on the screen
 */
void Oneliner_Print()
{
    int	    i, x, z, Strlen, Maxlen = 80;
    char    sNewOneliner[81] = "";

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

    if (Strlen == Maxlen) {
	PUTSTR(sOneliner);
	Enter(1);
    } else {
	x = Maxlen - Strlen;
	z = x / 2;
	for(i = 0; i < z; i++)
	    strcat(sNewOneliner," ");
	strcat(sNewOneliner, sOneliner);
	colour(iColour, 0);
	PUTSTR(sNewOneliner);
	Enter(1);
    }
}



/*
 * Get a random oneliner
 */
char *Oneliner_Get()
{
    FILE	*pOneline;
    int		i, j, in, id, recno = 0;
    int		offset;
    int		nrecno;
    char	*sFileName;
    static char	temp[81];

    /*
     * Get a random oneliner
     */
    sFileName = calloc(128, sizeof(char));
    snprintf(sFileName, PATH_MAX, "%s/etc/oneline.data", getenv("MBSE_ROOT"));

    if ((pOneline = fopen(sFileName, "r+")) == NULL) {
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
    FILE    *pOneline;
    int	    recno = 0, Colour = 1;
    char    *sFileName, msg[81];
	                                                                                  
    clear();
    sFileName = calloc(PATH_MAX, sizeof(char));
    snprintf(sFileName, PATH_MAX, "%s/etc/oneline.data", getenv("MBSE_ROOT"));

    if ((pOneline = fopen(sFileName, "r+")) == NULL) {
	WriteError("Can't open file: %s", sFileName);
	return;
    }
    fread(&olhdr, sizeof(olhdr), 1, pOneline);

    if ((SYSOP == TRUE) || (exitinfo.Security.level >= CFG.sysop_access)) {
	/* #  A   Date       User          Description */
	pout(LIGHTGREEN, BLACK, Language(345));
    } else {
	/* #  Description */
	pout(LIGHTGREEN, BLACK, Language(346));
    }
    Enter(1);
    colour(GREEN, BLACK);
    if (utf8)
	chartran_init((char *)"CP437", (char *)"UTF-8", 'B');
    PUTSTR(chartran(sLine_str()));
    chartran_close();

    while (fread(&ol, olhdr.recsize, 1, pOneline) == 1) {
	if ((SYSOP == TRUE) || (exitinfo.Security.level >= CFG.sysop_access)) {
	    snprintf(msg, 81, "%2d", recno);
	    pout(WHITE, BLACK, msg);

	    snprintf(msg, 81, "%2d ", ol.Available);
	    pout(LIGHTBLUE, BLACK, msg);

	    pout(LIGHTCYAN, BLACK, ol.DateOfEntry);

	    snprintf(msg, 81, "%-15s ", ol.UserName);
	    pout(CYAN, BLACK, msg);

	    snprintf(msg, 81, "%-.48s", ol.Oneline);
	    poutCR(Colour, BLACK, msg);
	} else {
	    snprintf(msg, 81, "%2d ", recno);
	    pout(WHITE, BLACK, msg);
	    snprintf(msg, 81, "%-.76s", ol.Oneline);
	    poutCR(Colour, BLACK, msg);
	}

	recno++;
	Colour++;
	if (Colour >= 16)
	    Colour = 1;
    }
    fclose(pOneline);
    Enter(1);
    Pause();
    free(sFileName);
}



void Oneliner_Show()
{
    FILE    *pOneline;
    int	    recno = 0;
    int	    offset;
    char    *sFileName, msg[81];

    sFileName = calloc(PATH_MAX, sizeof(char));
    snprintf(sFileName, PATH_MAX, "%s/etc/oneline.data", getenv("MBSE_ROOT"));

    if ((pOneline = fopen(sFileName, "r+")) == NULL) {
	WriteError("Can't open file: %s", sFileName);
	return;
    }
    fread(&olhdr, sizeof(olhdr), 1, pOneline);
    fseek(pOneline, 0, SEEK_END);
    recno = (ftell(pOneline) - olhdr.hdrsize) / olhdr.recsize;

    Enter(1);
    /* Please enter number to list: */
    snprintf(msg, 81, "%s (1..%d) ", Language(347), recno -1);
    pout(WHITE, BLACK, msg);
    colour(CFG.InputColourF, CFG.InputColourB);
    msg[0] = '\0';
    Getnum(msg, 10);
    recno = atoi(msg);

    offset = olhdr.hdrsize + (recno * olhdr.recsize);
    if (fseek(pOneline, offset, SEEK_SET) != 0)
	WriteError("Can't move pointer in %s",sFileName); 

    fread(&ol, olhdr.recsize, 1, pOneline);

    Enter(1);
    snprintf(msg, 11, "%d ", recno);
    pout(WHITE, BLACK, msg);
    pout(LIGHTRED, BLACK, ol.Oneline);
    Enter(2);

    Pause();
    fclose(pOneline);
    free(sFileName);
}



void Oneliner_Delete()
{
    FILE    *pOneline;
    int	    recno = 0, nrecno = 0;
    int	    offset;
    char    srecno[7], *sFileName, stemp[50], sUser[36], msg[81];

    sFileName = calloc(PATH_MAX, sizeof(char));
    snprintf(sFileName, PATH_MAX, "%s/etc/oneline.data", getenv("MBSE_ROOT"));

    if ((pOneline = fopen(sFileName, "r+")) == NULL) {
	WriteError("Can't open file: %s", sFileName);
	return;
    }
    fread(&olhdr, sizeof(olhdr), 1, pOneline);

    Enter(1);
    /* Please enter number to delete: */
    pout(WHITE, BLACK, Language(331));
    colour(CFG.InputColourF, CFG.InputColourB);
    GetstrC(srecno, 6);

    if ((strcmp(srecno,"")) == 0) {
	fclose(pOneline);
	return;
    }

    recno = atoi(srecno);

    nrecno = recno;
    recno = 0;
	
    while (fread(&ol, olhdr.recsize, 1, pOneline) == 1) {
	recno++;
    }

    if (nrecno >= recno) {
	Enter(1);
	/* Record does not exist */
	pout(LIGHTRED, BLACK, Language(319));
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
	snprintf(stemp,50,"%d", nrecno);

	/* Print UserName to String, so we can compare for deletion */
	snprintf(sUser,36,"%s", exitinfo.sUserName);

	if ((strcmp(sUser, ol.UserName)) != 0) {
	    if ((!SYSOP) && (exitinfo.Security.level < CFG.sysop_access)) {
		Enter(1);
		/* Record *//* does not belong to you.*/
		snprintf(msg, 81, "%s%s %s", (char *) Language(332), stemp, (char *) Language(333));
		pout(LIGHTRED, BLACK, msg);
		Enter(2);
		Syslog('!', "User tried to delete somebody else's record: %s", stemp);
		Pause();
		fclose(pOneline);
		return;
	    }
	}

	Enter(1);
	if ((ol.Available ) == FALSE) {
	    /* Record: %d already marked for deletion			*/
	    snprintf(msg, 81, "%s%d %s", (char *) Language(332), nrecno, (char *) Language(334));
	    pout(LIGHTRED, BLACK, msg);
	    Syslog('!', "User tried to mark an already marked record: %s", stemp);
	} else {
	    ol.Available = FALSE;
	    /* Record *//* marked for deletion */
	    snprintf(msg, 81, "%s%d %s", (char *) Language(332), nrecno, (char *) Language(334));
	    pout(LIGHTGREEN, BLACK, msg);
	    Syslog('+', "User marked oneliner record for deletion: %s", stemp);
	}
	Enter(2);
	Pause();

	if (fseek(pOneline, offset, 0) != 0)
	    WriteError("Can't move pointer in %s",sFileName); 
	fwrite(&ol, olhdr.recsize, 1, pOneline);
    }
    fclose(pOneline);
    free(sFileName);
}


