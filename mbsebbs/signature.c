/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Edit message signature.
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
#include "signature.h"
#include "funcs.h"
#include "input.h"
#include "language.h"
#include "timeout.h"
#include "term.h"


#define	MAXSIGLINES	4
#define	LENSIGLINES	74

char	sLiNE[MAXSIGLINES+1][LENSIGLINES+1];	/* Edit buffer		*/


/*
 * Internal prototypes
 */
int editsignature(void);
int loadsignature(void);


/*
 * Edit signature file, called by menu 319
 */
void signature(void)
{
    memset(&sLiNE, 0, sizeof(sLiNE));

    if (loadsignature()) {
	while (TRUE) {
	    if (editsignature() == TRUE)
		break;
	}
    }

    Enter(2);
    /* Returning to */
    pout(CFG.MoreF, CFG.MoreB, (char *) Language(117));
    poutCR(CFG.MoreF, CFG.MoreB, CFG.bbs_name);
    sleep(2);
}



void toprow(void);
void toprow(void)
{
    int	    i;

    pout(YELLOW, BLACK, (char *)" Õ");
    for (i = 0; i < LENSIGLINES; i++)
	printf("Í");
    printf("¸\n");
}



void botrow(void);
void botrow(void)
{
    int	    i;

    pout(YELLOW, BLACK, (char *)" Ô");
    for (i = 0; i < LENSIGLINES; i++)
	printf("Í");
    printf("¾\n");
}



/*
 * Load signature file in memory for editing.
 */
int loadsignature(void)
{
    char    *temp;
    FILE    *fp;
    int	    i;

    temp  = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/%s/.signature", CFG.bbs_usersdir, exitinfo.Name);

    if ((fp = fopen(temp, "r")) == NULL) {
	WriteError("$Can't load %s", temp);
	free(temp);
	return FALSE;
    }
    
    clear();

    /* Edit message signature */
    pout(WHITE, BLACK, (char *) Language(107));
    Enter(2);

    Syslog('+', "%s will edit his .signature", exitinfo.sUserName);

    i = 0;
    while (fgets(temp, LENSIGLINES+1, fp)) {
	Striplf(temp);
	strcpy(sLiNE[i], temp);
	i++;
    }
    fclose(fp);
    free(temp);

    toprow();
    for (i = 0; i < MAXSIGLINES; i++) {
	colour(LIGHTRED, BLACK);
	printf("%d:", i+1);
	colour(CFG.MoreF, CFG.MoreB);
	printf("%s\n", sLiNE[i]);
	fflush(stdout);
    }
    botrow();

    return TRUE;
}



/*
 * Edit signature file with a simple line editor.
 */
int editsignature(void)
{
    FILE    *fp;
    int	    i, x;
    char    *temp, *temp1;

    temp  = calloc(PATH_MAX, sizeof(char));
    temp1 = calloc(PATH_MAX, sizeof(char));
    sprintf(temp, "%s/%s/.signature", CFG.bbs_usersdir, exitinfo.Name);

    while (TRUE) {
	Enter(1);
	/* Functions available: */
	poutCR(CFG.HiliteF, CFG.HiliteB, (char *) Language(113));
	Enter(1);
	/* (L)ist, (R)eplace text, (E)dit line, (A)bort, (S)ave */
	pout(YELLOW, RED, (char *) Language(114));
	Enter(2);

	/* Select: */
  	pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(115));
	fflush(stdout);

	alarm_on();
	i = toupper(Getone());
	Enter(1);

	if (i == Keystroke(114, 3)) {
	    /* Aborting... */
	    pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(116));
	    Syslog('+', "User aborted .signature editor");
	    free(temp);
	    free(temp1);
	    return TRUE;

	} else if (i == Keystroke(114, 2)) {
	    /* Edit which line: */
	    colour(CFG.HiliteF, CFG.HiliteB);
	    printf("\n %s", (char *) Language(118));
	    colour(CFG.InputColourF, CFG.InputColourB);
	    GetstrC(temp, 3);

	    if ((strcmp(temp, "")) == 0)
		break;

	    i = atoi(temp);
	    if ((i < 1) || (i > MAXSIGLINES)) {
		/* Line does not exist. */
		printf("%s\n", (char *) Language(119));
		break;
	    }

	    x = strlen(sLiNE[i-1]);
	    colour(LIGHTRED, BLACK);
	    printf("%d:", i);
	    colour(CFG.InputColourF, CFG.InputColourB);
	    printf("%s", sLiNE[i-1]);
	    fflush(stdout);
	    GetstrP(sLiNE[i-1], LENSIGLINES-1, x);

	} else if (i == Keystroke(114, 0)) {
	    /* List lines */
	    toprow();
	    for (i = 0; i < MAXSIGLINES; i++) {
		colour(LIGHTRED, BLACK);
		printf("%d:", i+1);
		colour(CFG.MoreF, CFG.MoreB);
		printf("%s\n", sLiNE[i]);
	    }
	    botrow();

	} else if (i == Keystroke(114, 4)) {
	    Enter(1);
	    /* Saving... */
	    pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(340));

	    /* Open TextFile for Writing NextUser Info */
	    sprintf(temp, "%s/%s/.signature", CFG.bbs_usersdir, exitinfo.Name);
	    if ((fp = fopen(temp, "w")) == NULL) {
		WriteError("$Can't open %s", temp);
		free(temp);
		free(temp1);
		return TRUE;
	    }
	    for (i = 0; i < MAXSIGLINES; i++) {
		if (strlen(sLiNE[i]))
		    fprintf(fp, "%s\n", sLiNE[i]);
	    }
	    fclose(fp);
		
	    Syslog('+', "User Saved .signature");
	    free(temp);
	    free(temp1);
	    return TRUE;

	} else if (i == Keystroke(114, 1)) {
	    /* Edit which line: */
	    colour(CFG.HiliteF, CFG.HiliteB);
	    printf("\n%s", (char *) Language(118));
	    colour(CFG.InputColourF, CFG.InputColourB);
	    GetstrC(temp, 3);

	    if ((strcmp(temp, "")) == 0)
		break;

	    i = atoi(temp);

	    if ((i < 1) || (i > MAXSIGLINES)) {
		/* Line does not exist. */
		printf("\n%s", (char *) Language(119));
		break;
	    }

	    Enter(1);
	    /* Line reads: */
	    poutCR(CFG.MoreF, CFG.MoreB, (char *) Language(186));
	    printf("%d:%s\n", i, sLiNE[i-1]);

	    Enter(1);
	    /* Text to replace: */
	    pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(195));
	    colour(CFG.InputColourF, CFG.InputColourB);
	    GetstrC(temp, LENSIGLINES-1);

	    if ((strcmp(temp, "")) == 0)
		break;

	    /* Replacement text: */
	    pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(196));
	    colour(CFG.InputColourF, CFG.InputColourB);
	    GetstrC(temp1, LENSIGLINES-1);

	    if ((strcmp(temp1, "")) == 0)
		break;
	    strreplace(sLiNE[i-1], temp, temp1);

	} else
	    printf("\n");
    }

    free(temp);
    free(temp1);
    return FALSE;
}

