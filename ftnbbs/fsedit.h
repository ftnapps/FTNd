#ifndef	_FSEDIT_H
#define	_FSEDIT_H

/* $Id: fsedit.h,v 1.9 2007/02/25 20:28:09 mbse Exp $ */


int	Fs_Edit(void);	/* The fullscreen message editor	*/

extern	int	Line;		/* Number of lines + 1			*/
extern	char	*Message[];	/* TEXTBUFSIZE lines of MAX_LINE_LENGTH chars	*/

int		Row;		/* Current row on screen		*/
int		Col;		/* Current column in text and on screen	*/
int		TopVisible;	/* First visible line of text		*/
int		InsMode;	/* Insert mode				*/
int		CurRow;		/* Current row in buffer		*/

void Show_Ins(void);
void Top_Help(void);
void Top_Menu(void);
void Ls(int);
void Rs(void);
void Ws(int);
void Hl(int, char *);
void Full_Help(void);
void Setcursor(void);
void Beep(void);
void Refresh(void);
void GetstrLC(char *, int);
void ScrollUp(void);
void ScrollDown(void);
void FsMove(unsigned char);
int FsWordWrap(void);

#endif /* _FSEDIT_H */

