#ifndef	_TERM_H
#define	_TERM_H

/* $Id$ */

void            Enter(int);
char		*pout_str(int, int, char *);
void            pout(int, int, char *);
char		*poutCR_str(int, int, char *);
void            poutCR(int, int, char *);
char		*poutCenter_str(int,int,char *);
void            poutCenter(int,int,char *);
char		*colour_str(int, int);
void            colour(int, int);
char		*Center_str(char *);
void            Center(char *);
char		*clear_str(void);
void            clear(void);
char		*locate_str(int, int);
void            locate(int, int);
char		*hLine_str(int);
char		*fLine_str(int);
void            fLine(int);
char		*sLine_str(void);
void            sLine(void);
void            mvprintw(int, int, const char *, ...);


#endif
