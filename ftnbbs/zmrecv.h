#ifndef	ZMRECV_H
#define	ZMRECV_H

/* $Id: zmrecv.h,v 1.5 2004/11/27 22:04:12 mbse Exp $ */

int	procheader(char*);
int	zmrcvfiles(int, int);
int	putsec(char*,int);
int	closeit(int);

#endif
