#ifndef _MISC_H
#define _MISC_H

void Setup(char *, char *); /* This function replaces a string in the users file */
int  GetLastUser(void);
void LastCallers(char *);
void SaveLastCallers(void);
char *GLCdate(void);		/* Returns current date  DD-Mmm      */
void DisplayLogo(void);
int  ChkFiles(void);
int  MoreFile(char *);
void Check_PM(void);		/* Check for personal message		*/

#endif

