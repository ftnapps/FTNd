/* $Id$ */

#ifndef _FUNCS4_H
#define _FUNCS4_H


void UserSilent(int);		/* Update users silent flag info	    */
int  CheckStatus(void);		/* Check BBS open status		    */
int  TelephoneScan(char *, char *);/* Scans for Duplicate User Phone Numbers   */
int  CheckName(char *);         /* Check if user name exists                */
char *logdate(void);		/* Returns DD-Mon HH:MM:SS                  */
char *NameGen(char *);		/* Get and test for unix login              */
char *NameCreate(char *, char *, char *);/* Create users login in passwd file       */
char *ChangeHomeDir(char *, int);    /* Change and Create Users Home Directories */
void CheckDir(char *);		/* Check and create directory		    */
int  Check4UnixLogin(char *);   /* Check Passwd File for Users Login        */
int  CheckHandle(char *);       /* Check if user handle exists              */
int  BadNames(char *);          /* Check for Unwanted user names            */
void FindMBSE(void);		/* Load Configuration file in memory        */
char *GLCdateyy(void);		/* Returns current date  DD-Mmm-YYYY        */
char *GetMonth(int);		/* Returns Mmm				    */


#endif
