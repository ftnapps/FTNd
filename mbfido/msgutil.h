/* $Id$ */

#ifndef	_MSGUTIL_H
#define	_MSGUTIL_H


void	Msg_Id(fidoaddr);
void	Msg_Pid(void);
void	Msg_Macro(FILE *);
int	Msg_Top(char *, int, fidoaddr);
void	Msg_Bot(fidoaddr, char *, char *);
void	CountPosted(char *);
char	*To_Low(char *, int);

#endif

