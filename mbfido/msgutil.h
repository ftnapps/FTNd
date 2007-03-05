/* $Id$ */

#ifndef	_MSGUTIL_H
#define	_MSGUTIL_H


void	Msg_Id(fidoaddr);
void	Msg_Pid(int);
void	Msg_Macro(FILE *);
int	Msg_Top(char *, int, fidoaddr);
void	Msg_Bot(fidoaddr, char *, char *);
void	CountPosted(char *);

#endif

