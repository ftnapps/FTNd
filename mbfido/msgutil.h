#ifndef	_MSGUTIL_H
#define	_MSGUTIL_H


void Msg_Id(fidoaddr);
void Msg_Pid(void);
void Msg_Top(void);
void Msg_Bot(fidoaddr, char *);
void CountPosted(char *);
char *To_Low(char *, int);

#endif

