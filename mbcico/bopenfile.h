/* $Id: bopenfile.h,v 1.1 2005/09/12 13:47:09 mbse Exp $ */

#ifndef	_BOPENFILE_H
#define	_BOPENFILE_H

FILE *bopenfile(char *, time_t, off_t, off_t *);
int  bclosefile(int);

#endif
