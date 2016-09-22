#ifndef	_EMAIL_H
#define	_EMAIL_H

/* email.h */

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

