/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Message line editor.
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
#include "../lib/memwatch.h"
#include "../lib/mbse.h"
#include "../lib/structs.h"
#include "../lib/users.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "mail.h"
#include "input.h"
#include "language.h"
#include "timeout.h"
#include "lineedit.h"


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
	if((Line - 1) == TEXTBUFSIZE) {
		Enter(1);
		/* Maximum message length exceeded */
		pout(3, 0, (char *) Language(166));
		Enter(1);
		return;
	}

	while (TRUE) {
		colour(10, 0);
		printf("%-2d : ", Line);
		colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
		fflush(stdout);
		alarm_on();
		GetstrP(Message[Line], 72, 0);

		if((strcmp(Message[Line], "")) == 0)
			return;

		Line++;
		if((Line - 1) == TEXTBUFSIZE) {
			Enter(1);
			/* Maximum message length exceeded */
			pout(12, 0, (char *) Language(166));
			Enter(1);
			return;
		}
	}
}



void Line_Edit_Delete()
{
	int	i, start, end = 0, total;
	int	Loop;
	char	temp[81];

	while (TRUE) {
		colour(10, 0);
		/* Delete starting at line */
		printf("\n\n%s#(1 - %d): ", (char *) Language(176), (Line - 1) );
		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(temp, 80);
		if((strcmp(temp, "")) == 0) {
			/* Aborted. */
			pout(15, 0, (char *) Language(177));
			Enter(1);
			return;
		}
		
		start = atoi(temp);
		colour(10, 0);
		if(start > (Line - 1) )
			/* Please enter a number in the range of */
			printf("\n%s(1 - %d)", (char *) Language(178), (Line - 1) );
		else
			break;
	}

	while (TRUE) {
		colour(10, 0);
		/* Delete ending   at line */
		printf("%s# (1 - %d): ", (char *) Language(179), (Line - 1) );
		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(temp, 80);
		if((strcmp(temp, "")) == 0) {
			/* Aborted. */
			pout(15, 0, (char *) Language(176));
			Enter(1);
			return;
		}

		end = atoi(temp);

		colour(10, 0);
		if(end > (Line - 1))
			/* Please enter a number in the range of */
			printf("\n%s(1 - %d)\n\n", (char *) Language(179), (Line - 1) );
		else
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
	int	j, edit;
	char	temp[81];

	while (TRUE) {
		while (TRUE) {
			colour(10, 0);
			/* Enter line # to edit */
			printf("\n%s(1 - %d): ", (char *) Language(181), (Line - 1) );
			colour(CFG.InputColourF, CFG.InputColourB);
			GetstrC(temp, 80);
			if((strcmp(temp, "")) == 0)
				return;

			edit = atoi(temp);

			colour(10, 0);
			if(edit > Line)
				/* Please enter a number in the range of */
				printf("\n%s(1 - %d) ", (char *) Language(178), (Line - 1) );
			else
				break;
		}

		colour(10, 0);
		printf("\n%d : ", edit);
		colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
		printf("%s", Message[edit]);
		fflush(stdout);
		j = strlen(Message[edit]);
		colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
		alarm_on();
		GetstrP(Message[edit], 81, j);
	}
}



void Line_Edit_Insert()
{
	int	i, j, start, end = 0, total;
	char	temp[81];

	if((Line - 1) == TEXTBUFSIZE) {
		Enter(1);
		/* Maximum message length exceeded */
		pout(3, 0, (char *) Language(166));
		Enter(1);
		return;
	}

	while (TRUE) {
		colour(10, 0);
		/* Enter line # to insert text before */
		printf("\n\n%s(1 - %d): ", (char *) Language(183), (Line - 1));
		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(temp, 80);
		if((strcmp(temp, "")) == 0) {
			/* Aborted. */
			pout(15, 0, (char *) Language(177));
			return;
		}

		start = atoi(temp);

		colour(10, 0);
		if(start > (Line - 1))
			/* Please enter a number in the range of */
			printf("\n%s(1 - %d)", (char *) Language(178), (Line - 1));
		else
			break;
 	}

	j = start;
	colour(10, 0);
	printf("\n%-2d : ", start);
	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	GetstrC(temp, 80);

	if((strcmp(temp, "")) == 0)
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
	int	edit;
	char	temp[81];

	while (TRUE) {
		while (TRUE) {
			colour(10, 0);
			/* Enter line # to replace */
			printf("\n\n%s(1 - %d): ", (char *) Language(185), (Line - 1) );
			colour(CFG.InputColourF, CFG.InputColourB);
			GetstrC(temp, 80);
			if((strcmp(temp, "")) == 0)
				return;

			edit = atoi(temp);

			colour(10, 0);
			if(edit > Line)
				/* Please enter a number in the range of */
				printf("\n%s(1 - %d) ", (char *) Language(178), (Line - 1));
			else
				break;
		}

		Enter(1);
		/* Line reads: */
		pout(15, 0, (char *) Language(186));
		Enter(1);

		colour(10, 0);
		printf("%d : ", edit);
		colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
		printf("%s\n\n", Message[edit]);

		colour(10, 0);
		printf("%d : ", edit);
		colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
		GetstrC(temp, 80);
		if((strcmp(temp, "")) == 0) {
			Enter(1);
			/* Unchanged. */
			pout(15, 0, (char *) Language(187));
			Enter(1);
		} else
			strcpy(Message[edit], temp);

		Enter(1);
		/* Line now reads: */
		pout(15, 0, (char *) Language(188));
		Enter(1);

		colour(10, 0);
		printf("%d : ", edit);

		colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
		printf("%s", Message[edit]);
	}
}



void Line_Edit_Text()
{
	int	edit;
	char	temp[81];
	char	temp1[81];

	while (TRUE) {
		while (TRUE) {
			colour(10, 0);
			/* Enter line # to edit */
			printf("\n\n%s(1 - %d): ", (char *) Language(194), (Line - 1));
			colour(CFG.InputColourF, CFG.InputColourB);
			GetstrC(temp, 80);
			if((strcmp(temp, "")) == 0)
				return;

			edit = atoi(temp);

			colour(10, 0);
			if(edit > Line)
				/* Please enter a number in the range of */
				printf("\n%s(1 - %d) ", (char *) Language(178), (Line - 1) );
			else
				break;
		}

		Enter(1);
		/* Line reads: */
		pout(15, 0, (char *) Language(186));
		Enter(1);
		colour(10, 0);
		printf("%d : ", edit);
		colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
		printf("%s\n\n", Message[edit]);

		/* Text to replace: */
		pout(10, 0, (char *) Language(195));
		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(temp, 80);
		/* Replacement text: */
		pout(10, 0, (char *) Language(196));
		colour(CFG.InputColourF, CFG.InputColourB);
		GetstrC(temp1, 80);

		strreplace(Message[edit], temp, temp1);

		Enter(1);
		/* Line now reads: */
		pout(15, 0, (char *) Language(197));
		Enter(1);
		colour(10, 0);
		printf("%d : ", edit);
		colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
		printf("%s", Message[edit]);
	}
}



void Line_Edit_Center()
{
	int	i, j, z, center;
	int	maxlen = 78;
	char	*CEnter;
	char	temp[81];

	colour(15, 0);
	/* Enter line # to center */
	printf("\n\n%s(1 - %d): ", (char *) Language(203), (Line - 1));
	fflush(stdout);
	GetstrC(temp, 80);
	if((strcmp(temp, "")) == 0)
		return;

	CEnter = calloc(81, sizeof(char));
	center = atoi(temp);
	j = strlen(Message[center]);
	if (j >= maxlen)
		/* Line is maximum length and cannot be centered */
		printf("\n%s\n", (char *) Language(204));
	else {
		z = 35 - (j / 2);

		for(i = 0; i < z; i++)
			strcat(CEnter," ");
		strcat(CEnter, Message[center]);
		strcpy(Message[center], CEnter);
	}

	colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
	printf("\n%s\n", Message[center]);
	free(CEnter);
}



int Line_Edit()
{
	int	i, j;

	clear();
	colour(12, 0);
	/* Begin your message now, Blank line to end */
	Center((char *) Language(164));
	/* Maximum of TEXTBUFSIZE lines, 73 chars per line */
	Center((char *) Language(165));
	colour(14, 0);
	printf("   (");
	for (i = 0; i < 74; i++)
		printf("-");
	printf(")\n");

	Line_Edit_Append();
	
	while (TRUE) {
		colour(14, 0);
		/* Functions available: (Current Message: */
		printf("\n%s%d ", (char *) Language(167), (Line - 1));
		/* Lines) */
		printf("%s\n\n", (char *) Language(168));
		colour(11, 0);
		/* L - List message      S - Save message      C - Continue message */
		printf("%s\n", (char *) Language(169));

		/* Q - Quit message      D - Delete line       I - Insert line  */
		printf("%s\n", (char *) Language(170));

		/* T - Text edit         E - Edit line         R - Replace line */
		printf("%s\n", (char *) Language(171));

		/* Z - Center line                                              */
		printf("%s\n", (char *) Language(172));

		colour(15, 0);
		printf("\n%s [", (char *) Language(173));
		for (i = 0; i < 10; i++)
			putchar(Keystroke(172, i));
		printf("]: ");
		fflush(stdout);

		alarm_on();
		j = toupper(Getone());

		if (j == Keystroke(172, 2)) {
			/* Continue */
			pout(15, 0, (char *) Language(174));
			Enter(1);
			Line_Edit_Append();
		} else

		if (j == Keystroke(172, 4)) {
			/* Delete */
			pout(15, 0, (char *) Language(175));
			Enter(1);
			Line_Edit_Delete();
		} else

		if (j == Keystroke(172, 7)) {
			/* Edit */
			pout(15, 0, (char *) Language(180));
			Enter(1);
			Line_Edit_Edit();
		} else

		if (j == Keystroke(172, 5)) {
			/* Insert */
  			pout(15, 0, (char *) Language(182));
  			Enter(1);
			Line_Edit_Insert();
		} else

		if (j == Keystroke(172, 0)) {
			pout(15, 0, (char *) Language(184));
			Enter(2);

			for(i = 1; i < Line; i++) {
				colour(10, 0);
				printf("%d: ", i);
				colour(CFG.MsgInputColourF, CFG.MsgInputColourB);
				printf("%s\n", Message[i]);
			}
		} else

		if (j == Keystroke(172, 8)) {
			/* Replace */
			pout(15, 0, (char *) Language(362));
			Enter(1);
			Line_Edit_Replace();
		} else

		if (j == Keystroke(172, 3)) {
			/* Quit */
			pout(15, 0, (char *) Language(189));
			Enter(2);

			/* Are you sure [y/N] */
			printf("%s", (char *) Language(190));
			fflush(stdout);
			alarm_on();

			if (toupper(Getone()) == Keystroke(190, 0)) {
				/* Yes */
				pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(356));
				Enter(1);
				/* Message aborted. */
				pout(15, 0, (char *) Language(191));
				Enter(2);

				fflush(stdout);				
				sleep(1);
				return FALSE;
			} 

			colour(CFG.HiliteF, CFG.HiliteB);
			/* No */
			printf("%s\n", (char *) Language(192));
		} else

		if (j == Keystroke(172, 6)) {
			/* Text Edit */
			pout(15, 0, (char *) Language(193));
			Line_Edit_Text();
		} else

		if (j == Keystroke(172, 1)) {
			/* Save */
			pout(15, 0, (char *) Language(198));
			Enter(1);
			fflush(stdout);

			if (Line > 1)
				return TRUE;
			
			return FALSE;
		} else

		if (j == Keystroke(172, 9)) {
			/* Center */
			pout(15, 0, (char *) Language(376));
			Enter(1);
			Line_Edit_Center();
		}
	}
}


