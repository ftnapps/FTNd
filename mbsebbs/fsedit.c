/*****************************************************************************
 *
 * File ..................: bbs/fsedit.c
 * Purpose ...............: FullScreen Message editor.
 * Last modification date : 20-Oct-2001
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
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "mail.h"
#include "funcs4.h"
#include "language.h"
#include "timeout.h"
#include "pinfo.h"
#include "fsedit.h"


void Show_Ins(void)
{
	locate(1, 70);
	colour(YELLOW, BLUE);
	printf("%s", InsMode ? "INS": "OVR");
	fflush(stdout);
}


void Top_Help() 
{
	locate(1,1);
	colour(YELLOW, BLUE);
	printf("%s", padleft((char *)"Press ESC for menu, other keys is edit text", 80, ' '));
	Show_Ins();
}


void Top_Menu(void)
{
	locate(1,1);
	colour(WHITE, RED);
	printf("%s", padleft((char *)"(A)bort (H)elp (S)ave - Any other key is continue edit", 80, ' '));
	fflush(stdout);
}


void Ls(int a, int y)
{
	locate(y, 10);
	printf("%c ", a ? 179 : '|');
}


void Rs(int a)
{
	colour(LIGHTGREEN, BLUE);
	printf("%c", a ? 179 : '|');
}


void Ws(int a, int y)
{
	int	i;

	Ls(a, y);
	for (i = 0; i < 57; i++)
		printf(" ");
	Rs(a);
}


void Hl(int a, int y, char *txt)
{
	Ls(a, y);
	colour(WHITE, BLUE);
	printf("%s", padleft(txt, 57, ' '));
	Rs(a);
}


void Full_Help(void)
{
	int	a, i;

	a = exitinfo.GraphMode;

	colour(LIGHTGREEN, BLUE);

	/* Top row */
	locate(1, 10);
	printf("%c", a ? 213 : '+');
	for (i = 0; i < 58; i++)
		printf("%c", a ? 205 : '=');
	printf("%c", a ? 184 : '+');

	Ws(a, 2);

	Ls(a, 3);
	colour(YELLOW, BLUE);
	printf("%s", padleft((char *)"                  Editor Help", 57, ' '));
	Rs(a);

	Ws(a,  4);
	Hl(a,  5, (char *)"Ctrl-S or LeftArrow     - Cursor left");
	Hl(a,  6, (char *)"Ctrl-D or RightArrow    - Cursor right");
	Hl(a,  7, (char *)"Ctrl-E or UpArrow       - Cursor up");
	Hl(a,  8, (char *)"Ctrl-X or DownArrow     - Cursor down");
	Hl(a,  9, (char *)"Ctrl-V or Insert        - Insert or Overwrite");
	Hl(a, 10, (char *)"Ctrl-N                  - Insert line");
	Hl(a, 11, (char *)"Ctrl-Y                  - Delete line");
	Ws(a, 12);
	Hl(a, 13, (char *)"Ctrl-L                  - Refresh screen");
	Hl(a, 14, (char *)"Ctrl-R                  - Read from file");
	Ws(a, 15);

	locate(16,10);
	printf("%c", a ? 212 : '+');
	for (i = 0; i < 58; i++)
		printf("%c", a ? 205 : '=');
	printf("%c", a ? 190 : '+');
	fflush(stdout);
}


void Setcursor(void)
{
	CurRow = Row + TopVisible - 1;
	locate(Row + 1, Col);
	fflush(stdout);
}


void Beep(void)
{
	printf("\007");
	fflush(stdout);
}


/*
 *  Refresh and rebuild screen in editing mode.
 */
void Refresh(void) 
{
	int	i, j = 2;

	clear();
	Top_Help();
	locate(j,1);
	colour(CFG.TextColourF, CFG.TextColourB);

	for (i = 1; i <= Line; i++) {
		if ((i >= TopVisible) && (i < (TopVisible + exitinfo.iScreenLen -1))) {
			locate(j, 1);
			j++;
			printf("%s", Message[i]);
		}
	}
	Setcursor();
}


void Debug(void)
{
	Syslog('b', "FSEDIT: Col=%d Row=%d TopVisible=%d Lines=%d CurRow=%d Len=%d", 
		Col, Row, TopVisible, Line, Row+TopVisible-1, strlen(Message[Row+TopVisible-1]));
}


void GetstrLC(char *sStr, int iMaxlen)
{
	unsigned char	ch = 0;
	int		iPos = 0;

	strcpy(sStr, "");
	alarm_on();

	while (ch != 13) {

		fflush(stdout);
		ch = Readkey();

		if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
			if (iPos > 0) {
				printf("\b \b");
				sStr[--iPos] = '\0';
			} else
				putchar('\007');
		}

		if (ch > 31 && ch < 127) {
			if (iPos <= iMaxlen) {
				iPos++;
				sprintf(sStr, "%s%c", sStr, ch);
				printf("%c", ch);
			} else
				putchar('\007');
		}
	}

	printf("\n");
}


void ScrollUp()
{
	Syslog('b', "FSEDIT: Scroll up");
	if (TopVisible > 12) {
		TopVisible -= 12;
		Row += 12;
	} else {
		Row += TopVisible - 1;
		TopVisible = 1;
	}
	Refresh();
	/* Debug(); */
}


void ScrollDown()
{
	Syslog('b', "FSEDIT: Scroll down");
	if ((TopVisible + 12) <= Line) {
		Row -= 12;
		TopVisible += 12;
	}
	Refresh();
	/* Debug(); */
}


int Fs_Edit()
{
	unsigned char	ch;
	int		i, Changed = FALSE;
	char		*filname, *tmpname;
	FILE		*fd;

	Syslog('b', "FSEDIT: Entering FullScreen editor");
	clear();
	fflush(stdout);
	if ((ttyfd = open("/dev/tty", O_RDWR|O_NONBLOCK)) < 0) {
		WriteError("$Can't open tty");
		return FALSE;
	}
	Setraw();
	InsMode = TRUE;
	TopVisible = 1;
	Col = 1;
	Row = 1;
	Refresh();
	Debug();

	while (TRUE) {
		Nopper();
		alarm_on();
		ch = Readkey();
		CurRow = Row + TopVisible - 1;

		switch (ch) {
		case KEY_ENTER:
			Syslog('b', "FSEDIT: Enter pressed");
			Debug();
			if (Col == 1) {
				/* Enter at beginning of line */
				for (i = Line; i >= CurRow; i--) {
					sprintf(Message[i+1], "%s", Message[i]);
				}
				Message[i+1][0] = '\0';
			} else {
				for (i = Line; i > CurRow; i--) {
					sprintf(Message[i+1], "%s", Message[i]);
				}
				Message[CurRow+1][0] = '\0';
				if (Col <= strlen(Message[CurRow])) {
					/* Enter in middle of line */
					for (i = Col-1; i <= strlen(Message[CurRow]); i++) {
						sprintf(Message[CurRow+1], "%s%c", Message[CurRow+1], Message[CurRow][i]);
					}
					Message[CurRow][Col-1] = '\0';
				}
				/* else Enter at end of line */
			}
			Line++;
			Row++;
			Col = 1;
			if (Row >= (exitinfo.iScreenLen -1)) ScrollDown();
			Refresh();
			/* Debug(); */
			Changed = TRUE;
			break;

		case ('N' - 64):  /* Insert line, scroll down */
			Syslog('b', "FSEDIT: Insert line");
			Debug();
			for (i = Line; i >= CurRow; i--)
				sprintf(Message[i+1], "%s", Message[i]);
			Message[CurRow][0] = '\0';
			Line++;
			Col = 1;
			Refresh();
			/* Debug(); */
			Changed = TRUE;
			break;

		case ('Y' - 64):  /* Erase line, scroll up */
				Syslog('b', "FSEDIT: Erase line");
				Debug();
				if (Line == CurRow) {
					/* Erasing last line */
					if ((Line > 1) || (strlen(Message[CurRow]) > 0)) {
						Message[CurRow][0] = '\0';
						if (Line > 1) {
							Line--;
							if (Row == 1) ScrollUp();
							Row--;
						}
						if (Col > strlen(Message[CurRow]))
						    Col = strlen(Message[CurRow]);
						Refresh();
						/* Debug(); */
						Changed = TRUE;
					} else
						Beep();
				} else {
					/* Erasing line in the middle */
					for (i = CurRow; i < Line; i++) {
						sprintf(Message[i], "%s", Message[i+1]);
					}
					Message[i+1][0] = '\0';
					Line--;
					if (Col > strlen(Message[CurRow]) + 1)
						Col = strlen(Message[CurRow]) + 1;
					Refresh();
					/* Debug(); */
					Changed = TRUE;
				}
				break;

		case KEY_UP:
		case ('E' - 64):
			Syslog('b', "FSEDIT: Cursor up");
			Debug();
			if (Row > 1) {
				Row--;
				if (Col > strlen(Message[CurRow-1]) + 1)
					Col = strlen(Message[CurRow-1]) + 1;
				Setcursor();
				/* Debug(); */
			} else {
				ScrollUp();
			}
			break;

		case KEY_DOWN:
		case ('X' - 64):
			Syslog('b', "FSEDIT: Cursor down");
			Debug();
			if (Row < (Line - TopVisible + 1)) {
				if (Row < (exitinfo.iScreenLen -1)) {
					Row++;
					if (Col > strlen(Message[CurRow+1]) + 1)
						Col = strlen(Message[CurRow+1]) + 1;
					Setcursor();
					/* Debug(); */
				} else {
					ScrollDown();
				}
			} else
				Beep();
			break;

		case KEY_LEFT:
		case ('S' - 64):
			Syslog('b', "FSEDIT: Cursor left");
			Debug();
			if (Col > 1) {
				Col--;
				Setcursor();
				/* Debug(); */
			} else if (CurRow > 1) {
				Col = strlen(Message[CurRow-1]) +1;
				if (Row == 1) ScrollUp();
				Row--;
				Setcursor();
				/* Debug(); */
			} else if (TopVisible > 12) {
				Refresh();
				/* Debug(); */
			} else
				Beep();
			
			break;

		case KEY_RIGHT:
		case ('D' - 64):
			Syslog('b', "FSEDIT: Cursor Right");
			Debug();
			if (Col <= strlen(Message[CurRow])) {
				Col++;
				Setcursor();
				/* Debug(); */
			} else if (Row < (Line - TopVisible +1)) {
				Row++;
				Col = 1;
				if (Row > (exitinfo.iScreenLen -1)) ScrollDown();
				Refresh();
				/* Debug(); */
			} else
				Beep();

			break;

		case KEY_DEL:
			Syslog('b', "FSEDIT: DEL key");
			Debug();
			if (Col <= strlen(Message[CurRow])) {
				/*
				 * If before the end of the line...
				 */
				Setcursor();
				for (i = Col; i <= strlen(Message[CurRow]); i++) {
					Message[CurRow][i-1] = Message[CurRow][i];
					printf("%c", Message[CurRow][i]);
				}
				printf(" \b");
				Message[CurRow][i-1] = '\0';
				Setcursor();
			} else if ((strlen(Message[CurRow]) + strlen(Message[CurRow+1]) < 75) && (CurRow < Line)) {
				for (i = 0; i < strlen(Message[CurRow+1]); i++)
					sprintf(Message[CurRow], "%s%c", Message[CurRow], Message[CurRow+1][i]);
				for (i = CurRow+1; i < Line; i++)
					sprintf(Message[i], "%s", Message[i+1]);
				Message[Line][0] = '\0';
				Line--;
				Refresh();
			} else
                                Beep();

			/* 
			 * Trap the extra code so it isn't
			 * inserted in the text
			 */
			ch = Readkey();
			break;

		case KEY_BACKSPACE:
		case KEY_RUBOUT:
			Syslog('b', "FSEDIT: BS (Backspace)");
			Debug();
			if (Col == 1 && CurRow == 1) {
				/* BS on first character in message */
				Beep();
			} else if (Col == 1) {
				/* BS at beginning of line */
				if ((strlen(Message[CurRow-1]) + strlen(Message[CurRow]) < 75)
					|| strlen(Message[CurRow]) == 0) {
					Col = strlen(Message[CurRow-1]) + 1;
					strcat(Message[CurRow-1], Message[CurRow]);
					for ( i = CurRow; i < Line; i++)
						sprintf(Message[i], "%s", Message[i+1]);
					Message[i+1][0] = '\0';
					Line--;
					if (Row == 1) ScrollUp();
					Row--;
					Refresh();
					/* Debug(); */
					Changed = TRUE;
				} else {
					i = strlen(Message[CurRow-1]) + strlen(Message[CurRow]);
					Syslog('b', "FSEDIT: BS lines are too big! %d + %d = %d",
					       strlen(Message[CurRow]),
					       strlen(Message[CurRow-1]),
					       i);
					Beep();
				}
			} else {
				if (Col == strlen(Message[CurRow]) + 1) {
					/* BS at end of line */
					printf("\b \b");
					fflush(stdout);
					Col--;
					Message[CurRow][Col-1] = '\0';
					Changed = TRUE;
				} else {
					/* BS in middle of line */
					Col--;
					Setcursor();
					for (i = Col; i <= strlen(Message[CurRow]); i++) {
						Message[CurRow][i-1] = Message[CurRow][i];
						printf("%c", Message[CurRow][i]);
					}
					printf(" \b");
					Message[CurRow][strlen(Message[CurRow])] = '\0';
					Setcursor();
					Changed = TRUE;
				}
			}
			break;

		case KEY_INS:  /* Toggle Insert Mode */
		case ('V' - 64):
			InsMode = !InsMode;
			Show_Ins();
			colour(CFG.TextColourF, CFG.TextColourB);
			Setcursor();
			Syslog('b', "FSEDIT: InsertMode now %s", InsMode ? "True" : "False");
			/* 
			 * Trap the extra code so it isn't
			 * inserted in the text
			 */
			ch = Readkey();
			break;

		case ('L' - 64):  /* Refresh screen */
			Syslog('b', "FSEDIT: Refresh()");
			Refresh();
			Debug();
			break;

		case ('R' - 64):  /* Read from file */
			Syslog('b', "FSEDIT: Read from file");

			tmpname = calloc(PATH_MAX, sizeof(char));
			filname = calloc(PATH_MAX, sizeof(char));

			colour(14, 0);
			/* Please enter filename: */
			printf("\n%s", (char *) Language(245));
			colour(CFG.InputColourF, CFG.InputColourB);
			GetstrLC(filname, 80);

			if ((strcmp(filname, "") == 0)) {
				colour(CFG.HiliteF, CFG.HiliteB);
				/* No filename entered, aborting */
				printf("\n\n%s\n", (char *) Language(246));
				Pause();
				free(filname);
				free(tmpname);
				Refresh();
				/* Debug(); */
				break;
			}

			if (*(filname) == '/' || *(filname) == ' ') {
				colour(CFG.HiliteF, CFG.HiliteB);
				/* Illegal filename */
				printf("\n\n%s\n", (char *) Language(247));
				Pause();
				free(tmpname);
				free(filname);
				Refresh();
				/* Debug(); */
				break;
			}

			sprintf(tmpname, "%s/%s/wrk/%s", CFG.bbs_usersdir, exitinfo.Name, filname);
			if ((fd = fopen(tmpname, "r")) == NULL) {
				WriteError("$Can't open %s", tmpname);
				colour(CFG.HiliteF, CFG.HiliteB);
				/* File does not exist, please try again */
				printf("\n\n%s\n", (char *) Language(296));
				Pause();
			} else {
				while ((fgets(filname, 80, fd)) != NULL) {
					for (i = 0; i < strlen(filname); i++) {
						if (*(filname + i) == '\0')
							break;
						if (*(filname + i) == '\n')
							*(filname + i) = '\0';
						if (*(filname + i) == '\r')
							*(filname + i) = '\0';
					}
					/*
					 * Make sure that any tear or origin lines are
					 * made invalid.
					 */
					if (strncmp(filname, (char *)"--- ", 4) == 0)
						filname[1] = 'v';
					if (strncmp(filname, (char *)" * Origin:", 10) == 0)
						filname[1] = '+';
					sprintf(Message[Line], "%s", filname);
					Line++;
				}
				fclose(fd);
				Changed = TRUE;
				Syslog('+', "FSEDIT: Inserted file %s", tmpname);
			}

			free(tmpname);
			free(filname);
			Col = 1;
			Refresh();
			/* Debug(); */
			break;

		case KEY_ESCAPE:  /* Editor menu */
			Syslog('b', "FSEDIT: Escape pressed");
			Top_Menu();

			ch = toupper(Readkey());
			if (ch == 'A' || ch == 'S') {
				Syslog('b', "FSEDIT: %s message (%c)", (ch == 'S' && Changed) ? "Saving" : "Aborting", ch);
				Unsetraw();
				close(ttyfd);
				Debug();
				clear();
				fflush(stdout);
				for (i = 1; i <= Line; i++)
					Syslog('b', "%3d \"%s\"", i, Message[i]);
				if (ch == 'S' && Changed) {
					Syslog('+', "FSEDIT: Message saved");
					return TRUE;
				} else {
					Syslog('+', "FSEDIT: Message aborted");
					return FALSE;
				}
			}

			if (ch == 'H') {
				Syslog('b', "FSEDIT: User wants help");
				Full_Help();
				ch = Readkey();
				Refresh();
			} else
				Top_Help();

			colour(CFG.TextColourF, CFG.TextColourB);
			Setcursor();
			break;
			
		default:
			if (ch > 31 && ch < 127) {
				/*
				 *  Normal printable characters
				 */
				/* Debug(); */
				if (Col == strlen(Message[CurRow]) + 1) {
					/*
					 *  Append to line
					 */
					if (Col < 79) {
						Col++;
						sprintf(Message[CurRow], "%s%c", Message[CurRow], ch);
						printf("%c", ch);
						fflush(stdout);
						Changed = TRUE;
					} else {
						/*
						 *  Do simple word wrap
						 */
						for (i = Line; i > CurRow; i--) {
							/*
							 * Shift lines down by one
							 */
							sprintf(Message[i+1], "%s", Message[i]);
						}
						/*
						 * Clear row for wrapped words
						 */
						Message[CurRow+1][0] = '\0';
						Col = 74;
						while (Message[CurRow][Col] != ' ' && i != 0)
							Col--;
						Col++;
						if (Col <= strlen(Message[CurRow])) {
							/* 
							 * Move end of line to new row
							 */
							for (i = Col; i <= strlen(Message[CurRow]); i++) {
								sprintf(Message[CurRow+1], "%s%c", Message[CurRow+1], Message[CurRow][i]);
							}
							Message[CurRow][Col-1] = '\0';
						}
						sprintf(Message[CurRow+1], "%s%c", Message[CurRow+1], ch);
						Line++;
						Row++;
						Col = strlen(Message[CurRow+1])+1;
						Refresh();
						/* Debug(); */
						Changed = TRUE;
					}
				} else {
					/*
					 *  Insert or overwrite
					 */
					Syslog('b', "FSEDIT: %s in line", InsMode ? "Insert" : "Overwrite");
					if (InsMode) {
						if (strlen(Message[CurRow]) < 80) {
							for (i = strlen(Message[CurRow]); i >= (Col-1); i--) {
								/*
								 * Shift characters right
								 */
								Message[CurRow][i+1] = Message[CurRow][i];
							}
							Message[CurRow][Col-1] = ch;
							Col++;
							locate(Row + 1, 1);
							printf(Message[CurRow]);
							Setcursor();
							Changed = TRUE;
						} else {
							Beep();
						}
					} else {
						Message[CurRow][Col-1] = ch;
						printf("%c", ch);
						fflush(stdout);
						Col++;
						Changed = TRUE;
					}
				}
			} else
				Syslog('b', "FSEDIT: Pressed %d (unsupported)", ch);
		}
	}

	WriteError("FsEdit(): Impossible to be here");
	Unsetraw();
	close(ttyfd);
	return FALSE;
}


