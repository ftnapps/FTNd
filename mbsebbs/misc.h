/* $Id: misc.h,v 1.3 2001/11/12 21:42:17 mbroek Exp $ */

#ifndef _MISC_H
#define _MISC_H

void Setup(char *, char *); /* This function replaces a string in the users file */
void SaveLastCallers(void);
char *GLCdate(void);		/* Returns current date  DD-Mmm      */
void DisplayLogo(void);
int  ChkFiles(void);

#endif
