#ifndef	_SENDMAIL_H
#define	_SENDMAIL_H


FILE *SendMgrMail(faddr *, int, int, char *, char *, char *);
void CloseMail(FILE *, faddr*);


#endif

