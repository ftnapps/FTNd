#ifndef	ZMRECV_H
#define	ZMRECV_H

/* $Id$ */

int	procheader(char*);
int	zmrcvfiles(int, int);
int	putsec(char*,int);
int	closeit(int);

#endif
