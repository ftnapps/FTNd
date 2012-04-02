#ifndef	_EMAIL_H
#define	_EMAIL_H

/* $Id: email.h,v 1.3 2005/10/11 20:49:48 mbse Exp $ */

void ShowEmailHdr(void);
int  Read_a_Email(unsigned int);
void Read_Email(void);
void Write_Email(void);
void Reply_Email(int);
void QuickScan_Email(void);
void Trash_Email(void);
void Choose_Mailbox(char *);
void SetEmailArea(char *);


#endif

