/* $Id: input.h,v 1.6 2005/10/11 20:49:48 mbse Exp $ */

#ifndef _INPUT_H
#define _INPUT_H


void		CheckScreen(void);	/* Detect screensize changes	    */
int		Speed(void);			/* Get (locked) tty speed   */
int		Waitchar(unsigned char *, int);	/* Wait n* 10mSec for char  */
int		Escapechar(unsigned char *);	/* Escape sequence test	    */
unsigned char	Readkey(void);			/* Read a translated key    */


void BackErase(void);		/* Send backspace with erase		    */
void GetstrU(char *, int);	/* Get string, forbid spaces		    */
void GetstrP(char *, int, int); /* Get string with cursor position	    */
void GetstrC(char *, int);      /* Get string, length, clear string         */
void Getnum(char *, int);       /* Get only numbers from user               */
void Getname(char *, int);      /* Get name & convert every 1st char to U/C */
void GetnameNE(char *, int);    /* Get name & convert every 1st char to U/C */
void GetDate(char *, int);      /* Get users birth date and check           */
void GetPhone(char *, int);     /* Get telephone number                     */
void Getpass(char *);		/* Get a password from the user		    */
void Pause(void);		/* Puts Pause on Screen and halts screen    */

int traduce(char *);

#endif
