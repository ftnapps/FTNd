#ifndef	_FSEDIT_H
#define	_FSEDIT_H

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

#endif

