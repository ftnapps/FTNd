#ifndef	_OPENPORT_H
#define	_OPENPORT_H

/* openport.h */

int	io_mode(int, int);
void	hangup(void);
int 	rawport(void);
int 	cookedport(void);
void	sendbrk(void);

#endif
