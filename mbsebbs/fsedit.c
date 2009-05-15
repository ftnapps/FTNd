/*****************************************************************************
 *
 * $Id: fsedit.c,v 1.28 2007/08/25 18:32:08 mbse Exp $
 * Purpose ...............: FullScreen Message editor.
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
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
#include "pinfo.h"
#include "fsedit.h"
#include "term.h"
#include "ttyio.h"


extern int  cols;
extern int  rows;

char	hstr[81];


void Show_Ins(void)
{
    locate(1, 70);
    colour(YELLOW, BLUE);
    if (InsMode)
	PUTSTR((char *)"INS");
    else
	PUTSTR((char *)"OVR");
}


void Top_Help() 
{
    char    temp[81];

    locate(1,1);
    colour(YELLOW, BLUE);
    snprintf(temp, 81, "%s", padleft((char *)"Press ESC for menu, other keys is edit text", 79, ' '));
    PUTSTR(temp);
    Show_Ins();
}


void Top_Menu(void)
{
    char    temp[81];

    locate(1,1);
    colour(WHITE, RED);
    snprintf(temp, 81, "%s", padleft((char *)"(A)bort (H)elp (S)ave - Any other key is continue edit", 79, ' '));
    PUTSTR(temp);
}


void Ls(int y)
{
    strcpy(hstr, locate_str(y, 10));
    strncat(hstr, (char *)"\xB3", 80);
}


void Rs()
{
    strncat(hstr,  colour_str(LIGHTGREEN, BLUE), 80);
    strncat(hstr, (char *)"\xB3", 80);
}


void Ws(int y)
{
    int	i;

    Ls(y);
    for (i = 0; i < 58; i++)
	strncat(hstr, (char *)" ", 80);
    Rs();
}


void Hl(int y, char *txt)
{
    Ls(y);
    strncat(hstr, colour_str(WHITE, BLUE), 80);
    strncat(hstr, padleft(txt, 58, ' '), 80);
    Rs();
    PUTSTR(chartran(hstr));
}


void Full_Help(void)
{
    strcpy(hstr, colour_str(LIGHTGREEN, BLUE));

    /* Top row */
    strncat(hstr, locate_str(1, 10), 80);
    strncat(hstr, (char *)"\xDA", 80);
    strncat(hstr, hLine_str(58), 80);
    strncat(hstr, (char *)"\xBF", 80);
    PUTSTR(chartran(hstr));

    Ws(2);
    PUTSTR(chartran(hstr));

    Ls(3);
    strncat(hstr, colour_str(YELLOW, BLUE), 80);
    strncat(hstr, padleft((char *)"                  Editor Help", 58, ' '), 80);
    Rs();
    PUTSTR(chartran(hstr));

    Ws(4);
    PUTSTR(chartran(hstr));

    Hl( 5, (char *)"Ctrl-S or LeftArrow     - Cursor left");
    Hl( 6, (char *)"Ctrl-D or RightArrow    - Cursor right");
    Hl( 7, (char *)"Ctrl-E or UpArrow       - Cursor up");
    Hl( 8, (char *)"Ctrl-X or DownArrow     - Cursor down");
    Hl( 9, (char *)"Ctrl-V or Insert        - Insert or Overwrite");
    Hl(10, (char *)"Ctrl-N                  - Insert line");
    Hl(11, (char *)"Ctrl-Y                  - Delete line");
    Ws(12);
    PUTSTR(chartran(hstr));
    Hl(13, (char *)"Ctrl-L                  - Refresh screen");
    Hl(14, (char *)"Ctrl-R                  - Read from file");
    Ws(15);
    PUTSTR(chartran(hstr));

    strcpy(hstr, locate_str(16,10));
    strncat(hstr, (char *)"\xC0", 80);
    strncat(hstr, hLine_str(58), 80);
    strncat(hstr, (char *)"\xD9", 80);
    PUTSTR(chartran(hstr));
}



void Setcursor(void)
{
    CurRow = Row + TopVisible - 1;
    locate(Row + 1, Col);
}


void Beep(void)
{
    PUTCHAR('\007');
}


/*
 *  Refresh and rebuild screen in editing mode.
 */
void Refresh(void) 
{
    int	    i, j = 2;

    clear();
    Top_Help();
    locate(j,1);
    colour(CFG.TextColourF, CFG.TextColourB);

    for (i = 1; i <= Line; i++) {
	if ((i >= TopVisible) && (i < (TopVisible + rows -1))) {
	    locate(j, 1);
	    j++;
	    PUTSTR(Message[i]);
	}
    }
    Setcursor();
}



void GetstrLC(char *sStr, int iMaxlen)
{
    unsigned char   ch = 0;
    int		    iPos = 0;

    strcpy(sStr, "");
    alarm_on();

    while (ch != 13) {

	ch = Readkey();

	if ((ch == 8) || (ch == KEY_DEL) || (ch == 127)) {
	    if (iPos > 0) {
		BackErase();
		sStr[--iPos] = '\0';
	    } else {
		Beep();
	    }
	}

	if ((ch > 31 && ch < 127) || traduce((char *)&ch)) {
	    if (iPos <= iMaxlen) {
		iPos++;
		snprintf(sStr + strlen(sStr), 5, "%c", ch);
		PUTCHAR(ch);
	    } else {
		Beep();
	    }
	}
    }

    Enter(1);
}


void ScrollUp()
{
    if (TopVisible > 12) {
	TopVisible -= 12;
	Row += 12;
    } else {
	Row += TopVisible - 1;
	TopVisible = 1;
    }
    Refresh();
}


void ScrollDown()
{
    if ((TopVisible + 12) <= Line) {
	Row -= 12;
	TopVisible += 12;
    }
    Refresh();
}


void FsMove(unsigned char Direction)
{
    switch (Direction) {
	case KEY_RIGHT:
			/* 
			 * FIXME: FsMove KEY_RIGHT
			 * Handle long lines better.
			 * For now, we will just refuse to go past col 80
			 */
			if ((Col <= strlen(Message[CurRow])) && (Col < cols)){
			    Col++;
			    Setcursor();
			} else if (Row < (Line - TopVisible +1)) {
			    Row++;
			    Col = 1;
			    if (Row > (rows -1)) ScrollDown();
				Refresh();
			} else
			    Beep();
			break;

	case KEY_LEFT:
			if (Col > 1) {
			    Col--;
			    Setcursor();
			} else if (CurRow > 1) {
			    Col = strlen(Message[CurRow-1]) +1;
			    /* 
			     * FIXME: FsMove KEY_LEFT
			     * Handle long lines better.
			     * For now, we will just refuse to go past col 80
			     */
			    if (Col > cols) 
				Col = cols;
			    if (Row == 1) 
				ScrollUp();
			    Row--;
			    Setcursor();
			} else
			    Beep();
			break;

	case KEY_UP:
			if (CurRow > 1) {
			    Row--;
			    if (Col > strlen(Message[CurRow-1]) + 1)
				Col = strlen(Message[CurRow-1]) +1;
			    if ((Row < 1) && (CurRow != Row))
				ScrollUp();
			    else
				Setcursor();
			} else
			    Beep();
			break;

	case KEY_DOWN:
			if (Row < (Line - TopVisible + 1)) {
			    Row++;
			    if (Col > strlen(Message[CurRow+1]) + 1)
				Col = strlen(Message[CurRow+1]) + 1;
			    if (Row <= (rows -1))
				Setcursor();
			    else
				ScrollDown();
			} else
			    Beep();
			break;
    }
}



int FsWordWrap()
{
    int		    WCol, i = 0;
    unsigned char   tmpLine[81];
    tmpLine[0]	    = '\0';

    /*
     * FIXME: FsWordWrap
     * The word wrap still fails the BIG WORD test
     * (BIG WORD = continuous string of characters that spans multiple
     *             lines without any spaces)
     */
	
    Syslog('b', "FSEDIT: Word Wrap");
    WCol = 79;
    while (Message[CurRow][WCol] != ' ' && WCol > 0)
	WCol--;
    if ((WCol > 0) && (WCol < cols)) 
	WCol++; 
    else 
	WCol=cols;
    if (WCol <= strlen(Message[CurRow])) {
	/*
	 * If WCol = 80 (no spaces in line) be sure to grab
	 * character 79.  Otherwise, drop it, because it's a space.
	 */
	if ((WCol == 80) || (WCol-1 == Col))
	    snprintf((char *)tmpLine + strlen((char *)tmpLine), 5, "%c", Message[CurRow][79]);
	/*
	 * Grab all characters from WCol to end of line.
	 */
	for (i = WCol; i < strlen(Message[CurRow]); i++) {
	    snprintf((char *)tmpLine + strlen((char *)tmpLine), 5, "%c", Message[CurRow][i]);
	}
	/*
	 * Truncate current row.
	 */
	Message[CurRow][WCol-1] = '\0';
	/* 
	 * If this is the last line, then be sure to create a
	 * new line.x
	 */
	if (CurRow >= Line) {
	    Line = CurRow+1;
	    Message[CurRow+1][0] = '\0';
	}
	/*
	 * If the wrapped section and the next row will not fit on
	 * one line, shift all lines down one and use the wrapped
	 * section to create a new line.
	 *
	 * Otherwise, slap the wrapped section on the front of the
	 * next row with a space if needed.
	 */
	if ((strlen((char *)tmpLine) + strlen(Message[CurRow+1])) > 79) {
	    for (i = Line; i > CurRow; i--)
		snprintf(Message[i+1], TEXTBUFSIZE +1, "%s", Message[i]);
	    snprintf(Message[CurRow+1], TEXTBUFSIZE +1, "%s", tmpLine);
	    Line++;
	    WCol = strlen((char *)tmpLine) + 1;
	} else {
	    if ((WCol == 80) && (Col >= WCol))
		WCol = strlen((char *)tmpLine)+1; 
	    else {
		if (tmpLine[strlen((char *)tmpLine)] != ' ')
		    snprintf((char *)tmpLine + strlen((char *)tmpLine), 1, " ");
		WCol = strlen((char *)tmpLine);
	    }
	    snprintf(Message[CurRow+1], TEXTBUFSIZE +1, "%s", strcat((char *)tmpLine, Message[CurRow+1]));
	}
    }

    return WCol;
}



int Fs_Edit()
{
    unsigned char   ch;
    int		    i, Changed = FALSE;
    char	    *filname, *tmpname;
    FILE	    *fd;

    Syslog('b', "FSEDIT: Entering FullScreen editor");
    if (utf8)
	chartran_init((char *)"CP437", (char *)"UTF-8", 'B');

    clear();
    InsMode = TRUE;
    TopVisible = 1;
    Col = 1;
    Row = 1;
    Refresh();

    while (TRUE) {
	Nopper();
	alarm_on();
	ch = Readkey();
	CurRow = Row + TopVisible - 1;

	switch (ch) {
	    case KEY_ENTER:
			    if (Col == 1) {
				/* Enter at beginning of line */
				for (i = Line; i >= CurRow; i--) {
				    snprintf(Message[i+1], TEXTBUFSIZE +1, "%s", Message[i]);
				}
				Message[i+1][0] = '\0';
			    } else {
				for (i = Line; i > CurRow; i--) {
				    snprintf(Message[i+1], TEXTBUFSIZE +1, "%s", Message[i]);
				}
				Message[CurRow+1][0] = '\0';
				if (Col <= strlen(Message[CurRow])) {
				    /* Enter in middle of line */
				    for (i = Col-1; i <= strlen(Message[CurRow]); i++) {
					snprintf(Message[CurRow+1] + strlen(Message[CurRow+1]), 5, "%c", Message[CurRow][i]);
				    }
				    Message[CurRow][Col-1] = '\0';
				}
				/* else Enter at end of line */
			    }
			    Line++;
			    Row++;
			    Col = 1;
			    if (Row >= (rows -1)) 
				ScrollDown();
			    Refresh();
			    Changed = TRUE;
			    break;

	    case ('N' - 64):  /* Insert line, scroll down */
			    for (i = Line; i >= CurRow; i--)
				snprintf(Message[i+1], TEXTBUFSIZE +1, "%s", Message[i]);
			    Message[CurRow][0] = '\0';
			    Line++;
			    Col = 1;
			    Refresh();
			    Changed = TRUE;
			    break;

	    case ('Y' - 64):  /* Erase line, scroll up */
			    if (Line == CurRow) {
				/* Erasing last line */
				if ((Line > 1) || (strlen(Message[CurRow]) > 0)) {
				    Message[CurRow][0] = '\0';
				    if (Line > 1) {
					Line--;
					if (Row == 1) 
					    ScrollUp();
					Row--;
				    }
				    if (Col > strlen(Message[CurRow]))
					Col = strlen(Message[CurRow]);
				    Refresh();
				    Changed = TRUE;
				} else
				    Beep();
			    } else {
				/* Erasing line in the middle */
				for (i = CurRow; i < Line; i++) {
				    snprintf(Message[i], TEXTBUFSIZE +1, "%s", Message[i+1]);
				}
				Message[i+1][0] = '\0';
				Line--;
				if (Col > strlen(Message[CurRow]) + 1)
				    Col = strlen(Message[CurRow]) + 1;
				Refresh();
				Changed = TRUE;
			    }
			    break;

	    case KEY_UP:
	    case ('E' - 64):
			    FsMove(KEY_UP);
			    break;

	    case KEY_DOWN:
	    case ('X' - 64):
			    FsMove(KEY_DOWN);
			    break;

	    case KEY_LEFT:
	    case ('S' - 64):
			    FsMove(KEY_LEFT);
			    break;

	    case KEY_RIGHT:
	    case ('D' - 64):
			    FsMove(KEY_RIGHT);
			    break;

	    case KEY_DEL:
			    if (Col <= strlen(Message[CurRow])) {
				/*
				 * If before the end of the line...
				 */
				Setcursor();
				for (i = Col; i <= strlen(Message[CurRow]); i++) {
				    Message[CurRow][i-1] = Message[CurRow][i];
				    PUTCHAR(Message[CurRow][i]);
				}
				PUTCHAR(' ');
				PUTCHAR('\b');
				Message[CurRow][i-1] = '\0';
				Setcursor();
			    } else if (((strlen(Message[CurRow]) + strlen(Message[CurRow+1]) < 75) 
				    || (strlen(Message[CurRow]) == 0)) && (CurRow < Line)) {
				for (i = 0; i < strlen(Message[CurRow+1]); i++)
				    snprintf(Message[CurRow] + strlen(Message[CurRow]), 5, "%c", Message[CurRow+1][i]);
				for (i = CurRow+1; i < Line; i++)
				    snprintf(Message[i], TEXTBUFSIZE +1, "%s", Message[i+1]);
				Message[Line][0] = '\0';
				Line--;
				Refresh();
			    } else
                                Beep();

			    /* 
			     * Trap the extra code so it isn't
			     * inserted in the text
			     */
			    if (ch == KEY_DEL) 
				Readkey();
			    break;

	    case KEY_BACKSPACE:
	    case KEY_RUBOUT:
			    if (Col == 1 && CurRow == 1) {
				/* BS on first character in message */
				Beep();
			    } else if (Col == 1) {
				/* BS at beginning of line */
				if ((strlen(Message[CurRow-1]) + strlen(Message[CurRow]) < 75) || strlen(Message[CurRow]) == 0) {
				    Col = strlen(Message[CurRow-1]) + 1;
				    strcat(Message[CurRow-1], Message[CurRow]);
				    for ( i = CurRow; i < Line; i++)
					snprintf(Message[i], TEXTBUFSIZE +1, "%s", Message[i+1]);
				    Message[i+1][0] = '\0';
				    Line--;
				    if (Row == 1) 
					ScrollUp();
				    Row--;
				    Refresh();
				    Changed = TRUE;
				} else {
				    i = strlen(Message[CurRow-1]) + strlen(Message[CurRow]);
				    Beep();
				}
			    } else {
				if (Col == strlen(Message[CurRow]) + 1) {
				    /* BS at end of line */
				    BackErase();
				    Col--;
				    Message[CurRow][Col-1] = '\0';
				    Changed = TRUE;
				} else {
				    /* BS in middle of line */
				    Col--;
				    Setcursor();
				    for (i = Col; i <= strlen(Message[CurRow]); i++) {
					Message[CurRow][i-1] = Message[CurRow][i];
					PUTCHAR(Message[CurRow][i]);
				    }
				    PUTCHAR(' ');
				    PUTCHAR('\b');
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
			    /* 
			     * Trap the extra code so it isn't
			     * inserted in the text
			     */
			    if (ch == KEY_INS) 
				Readkey();
			    break;

	    case ('L' - 64):  /* Refresh screen */
			    Refresh();
			    break;

	    case ('R' - 64):  /* Read from file */
			    Syslog('b', "FSEDIT: Read from file");

			    tmpname = calloc(PATH_MAX, sizeof(char));
			    filname = calloc(PATH_MAX, sizeof(char));

			    Enter(1);
			    /* Please enter filename: */
			    pout(YELLOW, BLACK, (char *) Language(245));
			    colour(CFG.InputColourF, CFG.InputColourB);
			    GetstrLC(filname, 80);

			    if ((strcmp(filname, "") == 0)) {
				Enter(2);
				/* No filename entered, aborting */
				pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(246));
				Enter(1);
				Pause();
				free(filname);
				free(tmpname);
				Refresh();
				break;
			    }

			    if (*(filname) == '/' || *(filname) == ' ') {
				Enter(2);
				/* Illegal filename */
				pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(247));
				Enter(1);
				Pause();
				free(tmpname);
				free(filname);
				Refresh();
				break;
			    }

			    snprintf(tmpname, PATH_MAX, "%s/%s/wrk/%s", CFG.bbs_usersdir, exitinfo.Name, filname);
			    if ((fd = fopen(tmpname, "r")) == NULL) {
				WriteError("$Can't open %s", tmpname);
				Enter(2);
				/* File does not exist, please try again */
				pout(CFG.HiliteF, CFG.HiliteB, (char *) Language(296));
				Enter(1);
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
				    snprintf(Message[Line], TEXTBUFSIZE +1, "%s", filname);
				    Line++;
				    if ((Line - 1) == TEXTBUFSIZE)
					break;
				}
				fclose(fd);
				Changed = TRUE;
				Syslog('+', "FSEDIT: Inserted file %s", tmpname);
			    }

			    free(tmpname);
			    free(filname);
			    Col = 1;
			    Refresh();
			    break;

	    case KEY_ESCAPE:  /* Editor menu */
			    Top_Menu();

			    ch = toupper(Readkey());
			    if (ch == 'A' || ch == 'S') {
				Syslog('b', "FSEDIT: %s message (%c)", (ch == 'S' && Changed) ? "Saving" : "Aborting", ch);
				clear();
				chartran_close();
				if (ch == 'S' && Changed) {
				    Syslog('+', "FSEDIT: Message will be saved");
				    return TRUE;
				} else {
				    Syslog('+', "FSEDIT: Message aborted");
				    return FALSE;
				}
			    }

			    if (ch == 'H') {
				Full_Help();
				ch = Readkey();
				Refresh();
			    } else
				Top_Help();

			    colour(CFG.TextColourF, CFG.TextColourB);
			    Setcursor();
			    break;
			
	    default:
			    if ((ch > 31) || traduce((char *)&ch) ) {
				/*
				 *  Normal printable characters or hi-ascii
				 */
				if (Col == strlen(Message[CurRow]) + 1) {
				    /*
				     *  Append to line
				     */
				    snprintf(Message[CurRow] + strlen(Message[CurRow]), 5, "%c", ch);
				    if (strlen(Message[CurRow]) > 79){
					Col = FsWordWrap();
					Row++;
					Refresh();
				    } else {
					Col++;
					PUTCHAR(ch);
				    }
				    Changed = TRUE;
				} else {
				    /*
				     *  Insert or overwrite
				     */
				    if (InsMode) {
					for (i = strlen(Message[CurRow]); i >= (Col-1); i--) {
					    /*
					     * Shift characters right
					     */
					    Message[CurRow][i+1] = Message[CurRow][i];
					}
					Message[CurRow][Col-1] = ch;
					Col++;
					if (strlen(Message[CurRow]) > 80) {
					    i = FsWordWrap();
					    if (Col > strlen(Message[CurRow])+1) {
						Col = Col - strlen(Message[CurRow]);
						if (Col > 1) 
						    Col--;
						Row++;
					    }
					    if (Row > (rows -1))
						ScrollDown();
					    else
						Refresh();
					} else {
					    locate(Row + 1, 1);
					    PUTSTR(Message[CurRow]);
					    Setcursor();
					}
					Changed = TRUE;
				    } else {
					Message[CurRow][Col-1] = ch;
					PUTCHAR(ch);
					Col++;
					Changed = TRUE;
				    }
				}
			    } 
	}
    }

    chartran_close();
    WriteError("FsEdit(): Impossible to be here");
    return FALSE;
}


