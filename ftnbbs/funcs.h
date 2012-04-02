/* $Id: funcs.h,v 1.6 2002/10/01 16:54:16 mbroek Exp $ */

#ifndef _FUNCS_H
#define _FUNCS_H


void UserSilent(int);			/* Update users silent flag info	    */
int  CheckStatus(void);			/* Check BBS open status		    */
int  CheckUnixNames(char *);		/* Check Unix and other forbidden names	    */
int  CheckName(char *);			/* Check if user name exists                */
char *ChangeHomeDir(char *, int);	/* Change and Create Users Home Directories */
void CheckDir(char *);			/* Check and create directory		    */
void FindMBSE(void);			/* Load Configuration file in memory        */
char *GLCdateyy(void);			/* Returns current date  DD-Mmm-YYYY        */
char *GetMonth(int);			/* Returns Mmm				    */

#endif
