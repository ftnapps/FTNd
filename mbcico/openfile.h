/* $Id: openfile.h,v 1.2 2003/08/23 14:57:52 mbroek Exp $ */

#ifndef	_OPENFILE_H
#define	_OPENFILE_H

FILE *openfile(char *, time_t, off_t, off_t *, int(*)(off_t));
int  closefile(void);

#endif
