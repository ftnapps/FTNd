#ifndef	_MBMSG_H
#define	_MBMSG_H

/* $Id: mbmsg.h,v 1.4 2005/10/11 20:49:47 mbse Exp $ */

void	ProgName(void);
void	Help(void);
void	die(int);
void	DoMsgBase(void);
void	PackArea(char *, int);
void	LinkArea(char *, int);
void	KillArea(char *, char *, int, int, int);

#endif

