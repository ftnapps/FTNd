#ifndef	_ZMMISC_H
#define	_ZMMISC_H

/* $Id$ */

void	get_frame_buffer(void);
void	free_frame_buffer(void);
void	zsbhdr(int, int, register char *);
void	zsbh32(int, register char *, int, int);
void	zshhdr(int, int, register char *);
void	zsdata(register char *, int, int);
void	zsda32(register char *, int, int);
int	zrdata(register char *, int);
int	zrdat32(register char *, int);
void	garbitch(void);
int	zgethdr(char *);
int	zrbhdr(register char *);
int	zrbhd32(register char *);
int	zrhhdr(char *);
void	zputhex(register int);
void	zsendline(int);
int	zgethex(void);
int	zgeth1(void);
int	zdlread(void);
int	noxrd7(void);
void	stohdr(long);
long	rclhdr(register char *);


#endif
