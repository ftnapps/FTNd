#ifndef	_MBMSG_H
#define	_MBMSG_H

void	ProgName(void);
void	Help(void);
void	die(int);
void	DoMsgBase(void);
void	PackArea(char *, long);
void	LinkArea(char *, long);
void	IndexArea(char *, long);
void	KillArea(char *, char *, int, int, long);

#endif

