/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Message line editor.
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
#include "mail.h"
#include "input.h"
#include "language.h"
#include "timeout.h"
#include "lineedit.h"
#include "term.h"
#include "ttyio.h"


extern	int	Line;
extern	char	*Message[];


/*
 *  Internal prototypes
 */
void	Line_Edit_Append(void);		/* Append lines			     */
void	Line_Edit_Delete(void);		/* Delete lines			     */
void	Line_Edit_Edit(void);		/* Edit lines			     */
void	Line_Edit_Insert(void);		/* Insert lines			     */
void	Line_Edit_Replace(void);	/* Replace lines		     */
void	Line_Edit_Text(void);		/* Edit (replace) text in line	     */
void	Line_Edit_Center(void);		/* Center a line		     */




void Line_Edit_Append()
{
    char    msg[41];

    if ((Line - 1) == TEXTBUFSIZE) {
	Enter(1);
	/* Maximum message length exceeded */
	pout(CYAN, BLACK, (char *) Language(166));
	Enter(1);
	return;
    }

    while (TRUE) {
	snprintf(msg, 41, "%-2d : ", Line);
	pout(LIGHTGREEN, BLACK, msg);
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	alarm_on();
	GetstrP(Message[Line], 72, 0);

	if ((strcmp(Message[Line], "")) == 0)
	    return;

	Line++;
	if ((Line - 1) == TEXTBUFSIZE) {
	    Enter(1);
	    /* Maximum message length exceeded */
	    pout(LIGHTRED, BLACK, (char *) Language(166));
	    Enter(1);
	    return;
	}
    }
}



void Line_Edit_Delete()
{
    int	    i, start, end = 0, total, Loop;
    char    temp[81];

    while (TRUE) {
	Enter(2);
	/* Delete starting at line */
	snprintf(temp, 81, "%s#(1 - %d): ", (char *) Language(176), (Line - 1) );
	pout(LIGHTGREEN, BLACK, temp);
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp, 80);
	if ((strcmp(temp, "")) == 0) {
	    /* Aborted. */
	    pout(WHITE, BLACK, (char *) Language(177));
	    Enter(1);
	    return;
	}
		
	start = atoi(temp);
	if (start > (Line - 1)) {
	    Enter(1);
	    /* Please enter a number in the range of */
	    snprintf(temp, 81, "%s(1 - %d)", (char *) Language(178), (Line - 1) );
	    pout(LIGHTGREEN, BLACK, temp);
	} else
	    break;
    }

    while (TRUE) {
	/* Delete ending   at line */
	snprintf(temp, 81, "%s# (1 - %d): ", (char *) Language(179), (Line - 1) );
	pout(LIGHTGREEN, BLACK, temp);
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp, 80);
	if ((strcmp(temp, "")) == 0) {
	    /* Aborted. */
	    pout(WHITE, BLACK, (char *) Language(176));
	    Enter(1);
	    return;
	}

	end = atoi(temp);

	if(end > (Line - 1)) {
	    Enter(1);
	    /* Please enter a number in the range of */
	    snprintf(temp, 81, "%s(1 - %d)", (char *) Language(179), (Line - 1) );
	    pout(LIGHTGREEN, BLACK, temp);
	    Enter(2);
	} else
	    break;
    }

    /* Get total by minusing the end line from the start line  */
    /* and + 1 will give you total lines between start and end */
    total = (end - start) + 1;

    /* Define loop by minusing total lines from end which will */
    /* do a loop for only the amount of lines left after the   */
    /* end line */
    Loop = Line - end++;

    /* Minus the total amount of deleted lines from the current */
    /* amount of lines to keep track of how many lines you are  */
    /* working with                                             */
    Line -= total;

    /* Do loop to copy the current message over the deleted lines */

    for (i = 0; i < Loop; i++)
	strcpy(*(Message + start++), *(Message + end++));
}



void Line_Edit_Edit()
{
    int	    j, edit;
    char    temp[81];

    while (TRUE) {
	while (TRUE) {
	    Enter(1);
	    /* Enter line # to edit */
	    snprintf(temp, 81, "%s(1 - %d): ", (char *) Language(181), (Line - 1) );
	    pout(LIGHTGREEN, BLACK, temp);
	    colour(CFG.InputColourF, CFG.InputColourB);
	    GetstrC(temp, 80);
	    if ((strcmp(temp, "")) == 0)
		return;

	    edit = atoi(temp);

	    if (edit > Line) {
		Enter(1);
		/* Please enter a number in the range of */
		snprintf(temp, 81, "%s(1 - %d) ", (char *) Language(178), (Line - 1) );
		pout(LIGHTGREEN, BLACK, temp);
	    } else
		break;
	}

	Enter(1);
	snprintf(temp, 81, "%d : ", edit);
	pout(LIGHTGREEN, BLACK, temp);
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	PUTSTR(Message[edit]);
	j = strlen(Message[edit]);
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	alarm_on();
	GetstrP(Message[edit], 80, j);
    }
}



void Line_Edit_Insert()
{
    int	    i, j, start, end = 0, total;
    char    temp[81];

    if ((Line - 1) == TEXTBUFSIZE) {
	Enter(1);
	/* Maximum message length exceeded */
	pout(CYAN, BLACK, (char *) Language(166));
	Enter(1);
	return;
    }

    while (TRUE) {
	Enter(2);
	/* Enter line # to insert text before */
	snprintf(temp, 81, "%s(1 - %d): ", (char *) Language(183), (Line - 1));
	pout(LIGHTGREEN, BLACK, temp);
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp, 80);
	if ((strcmp(temp, "")) == 0) {
	    /* Aborted. */
	    pout(WHITE, BLACK, (char *) Language(177));
	    return;
	}

	start = atoi(temp);

	if (start > (Line - 1)) {
	    Enter(1);
	    /* Please enter a number in the range of */
	    snprintf(temp, 81, "%s(1 - %d)", (char *) Language(178), (Line - 1));
	    pout(LIGHTGREEN, BLACK, temp);
	} else
	    break;
    }

    Enter(1);
    j = start;
    snprintf(temp, 81, "%-2d : ", start);
    pout(LIGHTGREEN, BLACK, temp);
    colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
    GetstrC(temp, 80);

    if ((strcmp(temp, "")) == 0)
	return;

    total = Line - start;
    end = Line;
    Line++;
    start = Line;

    for (i = 0; i < total + 1; i++) {
	strcpy(Message[start], Message[end]);
	start--;
	end--;
    }

    strcpy(Message[j], temp);
}



void Line_Edit_Replace()
{
    int	    edit;
    char    temp[81];

    while (TRUE) {
	while (TRUE) {
	    Enter(2);
	    /* Enter line # to replace */
	    snprintf(temp, 81, "%s(1 - %d): ", (char *) Language(185), (Line - 1) );
	    pout(LIGHTGREEN, BLACK, temp);
	    colour(CFG.InputColourF, CFG.InputColourB);
	    GetstrC(temp, 80);
	    if ((strcmp(temp, "")) == 0)
		return;

	    edit = atoi(temp);

	    if (edit > Line) {
		Enter(1);
		/* Please enter a number in the range of */
		snprintf(temp, 81, "%s(1 - %d) ", (char *) Language(178), (Line - 1));
		pout(LIGHTGREEN, BLACK, temp);
	    } else
		break;
	}

	Enter(1);
	/* Line reads: */
	pout(WHITE, BLACK, (char *) Language(186));
	Enter(1);

	snprintf(temp, 81, "%d : ", edit);
	pout(LIGHTGREEN, BLACK, temp);
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	PUTSTR(Message[edit]);
	Enter(2);

	snprintf(temp, 81, "%d : ", edit);
	pout(LIGHTGREEN, BLACK, temp);
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	GetstrC(temp, 80);
	if ((strcmp(temp, "")) == 0) {
	    Enter(1);
	    /* Unchanged. */
	    pout(WHITE, BLACK, (char *) Language(187));
	    Enter(1);
	} else
	    strcpy(Message[edit], temp);

	Enter(1);
	/* Line now reads: */
	pout(WHITE, BLACK, (char *) Language(188));
	Enter(1);

	snprintf(temp, 81, "%d : ", edit);
	pout(LIGHTGREEN, BLACK, temp);

	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	PUTSTR(Message[edit]);
    }
}



void Line_Edit_Text()
{
    int	    edit;
    char    temp[81], temp1[81];

    while (TRUE) {
	while (TRUE) {
	    Enter(2);
	    /* Enter line # to edit */
	    snprintf(temp, 81, "%s(1 - %d): ", (char *) Language(194), (Line - 1));
	    pout(LIGHTGREEN, BLACK, temp);
	    colour(CFG.InputColourF, CFG.InputColourB);
	    GetstrC(temp, 80);
	    if ((strcmp(temp, "")) == 0)
		return;

	    edit = atoi(temp);

	    if (edit > Line) {
		Enter(1);
		/* Please enter a number in the range of */
		snprintf(temp, 81, "%s(1 - %d) ", (char *) Language(178), (Line - 1) );
		pout(LIGHTGREEN, BLACK, temp);
	    } else
		break;
	}

	Enter(1);
	/* Line reads: */
	pout(WHITE, BLACK, (char *) Language(186));
	Enter(1);
	snprintf(temp, 81, "%d : ", edit);
	pout(LIGHTGREEN, BLACK, temp);
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	PUTSTR(Message[edit]);
	Enter(2);

	/* Text to replace: */
	pout(LIGHTGREEN, BLACK, (char *) Language(195));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp, 80);
	/* Replacement text: */
	pout(LIGHTGREEN, BLACK, (char *) Language(196));
	colour(CFG.InputColourF, CFG.InputColourB);
	GetstrC(temp1, 80);

	strreplace(Message[edit], temp, temp1);

	Enter(1);
	/* Line now reads: */
	pout(WHITE, BLACK, (char *) Language(197));
	Enter(1);
	snprintf(temp, 81, "%d : ", edit);
	pout(LIGHTGREEN, BLACK, temp);
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	PUTSTR(Message[edit]);
    }
}



void Line_Edit_Center()
{
    int	    i, j, z, center, maxlen = 78;
    char    *CEnter, temp[81];

    Enter(2);
    /* Enter line # to center */
    snprintf(temp, 81, "%s(1 - %d): ", (char *) Language(203), (Line - 1));
    pout(WHITE, BLACK, temp);
    GetstrC(temp, 80);
    if ((strcmp(temp, "")) == 0)
	return;

    CEnter = calloc(81, sizeof(char));
    center = atoi(temp);
    j = strlen(Message[center]);
    if (j >= maxlen) {
	Enter(1);
	/* Line is maximum length and cannot be centered */
	pout(LIGHTGREEN, BLACK, (char *) Language(204));
	Enter(1);
    } else {
	z = 35 - (j / 2);

	for (i = 0; i < z; i++)
	    strcat(CEnter," ");
	strcat(CEnter, Message[center]);
	strcpy(Message[center], CEnter);
    }

    Enter(1);
    colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
    PUTSTR(Message[center]);
    Enter(1);
    free(CEnter);
}



int Line_Edit()
{
    int	    i, j;
    char    msg[81];

    clear();
    colour(LIGHTRED, BLACK);
    /* Begin your message now, Blank line to end */
    Center((char *) Language(164));
    /* Maximum of TEXTBUFSIZE lines, 73 chars per line */
    Center((char *) Language(165));
    colour(YELLOW, BLACK);
    PUTSTR((char *)"   (");
    for (i = 0; i < 74; i++)
	PUTSTR((char *)"-");
    PUTSTR((char *)")");
    Enter(1);

    Line_Edit_Append();
	
    while (TRUE) {
	Enter(1);
	/* Functions available: (Current Message: */                    /* Lines) */
	snprintf(msg, 81, "%s%d %s", (char *) Language(167), (Line - 1), (char *) Language(168));
	pout(YELLOW, BLACK, msg);
	Enter(2);

	/* L - List message      S - Save message      C - Continue message */
	pout(LIGHTCYAN, BLACK, (char *) Language(169));
	Enter(1);

	/* Q - Quit message      D - Delete line       I - Insert line  */
	pout(LIGHTCYAN, BLACK, (char *) Language(170));
	Enter(1);

	/* T - Text edit         E - Edit line         R - Replace line */
	pout(LIGHTCYAN, BLACK, (char *) Language(171));
	Enter(1);

	/* Z - Center line                                              */
	pout(LIGHTCYAN, BLACK, (char *) Language(172));
	Enter(2);

	snprintf(msg, 81, "%s [", (char *) Language(173));
	pout(WHITE, BLACK, msg);
	for (i = 0; i < 10; i++)
	    PUTCHAR(Keystroke(172, i));
	PUTSTR((char *)"]: ");

	alarm_on();
	j = toupper(Readkey());

	if (j == Keystroke(172, 2)) {
	    /* Continue */
	    pout(WHITE, BLACK, (char *) Language(174));
	    Enter(1);
	    Line_Edit_Append();
	} else if (j == Keystroke(172, 4)) {
	    /* Delete */
	    pout(WHITE, BLACK, (char *) Language(175));
	    Enter(1);
	    Line_Edit_Delete();
	} else if (j == Keystroke(172, 7)) {
	    /* Edit */
	    pout(WHITE, BLACK, (char *) Language(180));
	    Enter(1);
	    Line_Edit_Edit();
	} else if (j == Keystroke(172, 5)) {
	    /* Insert */
  	    pout(WHITE, BLACK, (char *) Language(182));
  	    Enter(1);
	    Line_Edit_Insert();
	} else if (j == Keystroke(172, 0)) {
	    pout(WHITE, BLACK, (char *) Language(184));
	    Enter(2);

	    for (i = 1; i < Line; i++) {
		snprintf(msg, 81, "%d: ", i);
		pout(LIGHTGREEN, BLACK, msg);
		colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
		PUTSTR(Message[i]);
		Enter(1);
	    }
	} else if (j == Keystroke(172, 8)) {
	    /* Replace */
	    pout(WHITE, BLACK, (char *) Language(362));
	    Enter(1);
	    Line_Edit_Replace();
	} else if (j == Keystroke(172, 3)) {
	    /* Quit */
	    pout(WHITE, BLACK, (char *) Language(189));
	    Enter(2);

	    /* Are you sure [y/N] */
	    PUTSTR((char *) Language(190));
	    alarm_on();

	    if (toupper(Readkey()) == Keystroke(190, 0)) {
		/* Yes */
		pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(356));
		Enter(1);
		/* Message aborted. */
		pout(WHITE, BLACK, (char *) Language(191));
		Enter(2);

		sleep(1);
		return FALSE;
	    } 

	    /* No */
	    pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(192));
	    Enter(1);
	} else if (j == Keystroke(172, 6)) {
	    /* Text Edit */
	    pout(WHITE, BLACK, (char *) Language(193));
	    Line_Edit_Text();
	} else if (j == Keystroke(172, 1)) {
	    /* Save */
	    pout(WHITE, BLACK, (char *) Language(198));
	    Enter(1);

	    if (Line > 1)
		return TRUE;
			
	    return FALSE;
	} else if (j == Keystroke(172, 9)) {
	    /* Center */
	    pout(WHITE, BLACK, (char *) Language(376));
	    Enter(1);
	    Line_Edit_Center();
	}
    }
}


