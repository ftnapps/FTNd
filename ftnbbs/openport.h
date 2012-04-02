#ifndef	_OPENPORT_H
#define	_OPENPORT_H

/* $Id: openport.h,v 1.2 2004/11/20 13:30:13 mbse Exp $ */

int	io_mode(int, int);
void	hangup(void);
int 	rawport(void);
int 	cookedport(void);
void	sendbrk(void);

#endif
