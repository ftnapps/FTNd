#ifndef	_OPENPORT_H
#define	_OPENPORT_H

/* openport.h */

void	linedrop(int);
void	interrupt(int);
void	sigpipe(int);
#ifdef TIOCWONLINE
void	alarmsig(int);
#endif
int 	openport(char *, int);
void	localport(void);
void	nolocalport(void);
int 	rawport(void);
int 	cookedport(void);
void	closeport(void);
void	sendbrk(void);

int	tty_raw(int);
int	tty_local(void);
int	tty_nolocal(void);
int	tty_cooked(void);


#endif

