#ifndef	_OPENPORT_H
#define	_OPENPORT_H

/* $Id$ */

void	linedrop(int);
void	sigpipe(int);
int 	rawport(void);
int 	cookedport(void);

int	tty_raw(int);
int	tty_cooked(void);

#endif
