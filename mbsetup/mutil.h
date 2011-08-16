#ifndef	_MUTIL_H
#define	_MUTIL_H

/* $Id: mutil.h,v 1.6 2004/03/29 20:41:26 mbroek Exp $ */

unsigned char	readkey(int, int, int, int);
unsigned char	testkey(int, int);
int		newpage(FILE *, int);
void		addtoc(FILE *, FILE *, int, int, int, char *);
void		disk_reset(void);
FILE		*open_webdoc(char *, char *, char *);
void		close_webdoc(FILE *);
void		add_webtable(FILE *, char *, char *);
void		add_webdigit(FILE *, char *, int);
void		add_colors(FILE *, char *, int, int);
void		add_statcnt(FILE *, char *, statcnt);
void		dotter(void);

#endif
