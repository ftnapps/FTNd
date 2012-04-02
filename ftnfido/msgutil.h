/* $Id: msgutil.h,v 1.5 2007/03/05 12:25:20 mbse Exp $ */

#ifndef	_MSGUTIL_H
#define	_MSGUTIL_H


void	Msg_Id(fidoaddr);
void	Msg_Pid(int);
void	Msg_Macro(FILE *);
int	Msg_Top(char *, int, fidoaddr);
void	Msg_Bot(fidoaddr, char *, char *);
void	CountPosted(char *);

#endif

