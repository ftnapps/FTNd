#ifndef _CHANGE_H
#define _CHANGE_H

/* $Id: change.h,v 1.7 2007/02/25 20:28:08 mbse Exp $ */

int  Chg_Language(int);		/* Change language			*/
void Chg_Password(void);	/* Change BBS Password			*/
void Chg_Handle(void);		/* Change Handle			*/
void Chg_Hotkeys(void);		/* Toggle Hotkeys			*/
void Chg_Disturb(void);		/* Toggle "Do not disturb"		*/
void Chg_MailCheck(void);	/* Toggle New Mail Check		*/
void Chg_FileCheck(void);	/* Toggle New Files Check		*/
void Chg_FsMsged(void);		/* Toggle Fullscreen Editor		*/
void Chg_FsMsgedKeys(void);	/* Toggle FS editor shortcut keys	*/
void Chg_Location(void);	/* Change location			*/
void Chg_Address(void);		/* Change address			*/
void Chg_VoicePhone(void);	/* Change voicephone			*/
void Chg_DataPhone(void);	/* Change dataphone			*/
void Chg_News(void);		/* Toggle News Bulletins		*/
int  Test_DOB(char *);		/* Test of Date of Birth is valid	*/
void Chg_DOB(void);		/* Change Date of Birth			*/
void Chg_Archiver(void);	/* Change default archiver		*/
void Chg_Protocol(void);	/* Change default transfer protocol.	*/
void Set_Protocol(char *);	/* Set default protocol			*/
void Chg_OLR_ExtInfo(void);	/* Set OLR Extended Info		*/
void Chg_Charset(void);		/* Change character set			*/


#endif
