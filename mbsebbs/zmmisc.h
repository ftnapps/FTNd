#ifndef	_ZMMISC_H
#define	_ZMMISC_H

/* $Id$ */

void	get_frame_buffer(void);
void	free_frame_buffer(void);
void	zsbhdr(int, int, register char *);
void	zshhdr(int, int, register char *);
void	zsdata(register char *, int, int);
int	zrdata(register char *, int);
int	zrdat32(register char *, int);
int	zgethdr(char *);
int	zrbhdr(register char *);
void	zsendline(int);
int	zdlread(void);
void	stohdr(long);
long	rclhdr(register char *);


#endif
