#ifndef	_TERM_H
#define	_TERM_H

/* $Id$ */

void            TermInit(int, int, int);
void            Enter(int);
void            pout(int, int, char *);
void            poutCR(int, int, char *);
void            poutCenter(int,int,char *);
void            colour(int, int);
void            Center(char *);
void            clear(void);
void            locate(int, int);
void            fLine(int);
void            sLine(void);
void            mvprintw(int, int, const char *, ...);


#endif
