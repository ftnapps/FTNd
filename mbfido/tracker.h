#ifndef	_TRACKER_H
#define	_TRACKER_H

#define	R_NOROUTE	0
#define	R_LOCAL		1
#define	R_DIRECT	2
#define	R_ROUTE		3
#define R_UNLISTED	4


int	TrackMail(fidoaddr, fidoaddr *);
int	GetRoute(char *, fidoaddr *);
void	TestTracker(void);

#endif

