#ifndef	_RAD64_H
#define	_RAD64_H


#ifndef HAVE_C64I
int	c64i(char);
int	i64c(int);
#endif
#ifndef HAVE_A64L
char	*l64a(long);
long	a64l(const char *);
#endif

#endif

