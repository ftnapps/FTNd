#ifndef	_FSEDIT_H
#define	_FSEDIT_H

/* Includes needed for fsedit.c */

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


int	Fs_Edit(void);	/* The fullscreen message editor	*/

extern	int	Line;		/* Number of lines + 1			*/
extern	char	*Message[];	/* TEXTBUFSIZE lines of 80 chars	*/

int		Row;		/* Current row on screen		*/
int		Col;		/* Current column in text and on screen	*/
int		TopVisible;	/* First visible line of text		*/
int		InsMode;	/* Insert mode				*/
int		CurRow;		/* Current row in buffer		*/

void Show_Ins(void);
void Top_Help(void);
void Top_Menu(void);
void Ls(int, int);
void Rs(int);
void Ws(int, int);
void Hl(int, int, char *);
void Full_Help(void);
void Setcursor(void);
void Beep(void);
void Refresh(void);
void Debug(void);
void GetstrLC(char *, int);
void ScrollUp(void);
void ScrollDown(void);
void FsMove(unsigned char);
int FsWordWrap(void);

#endif

