#ifndef	_OPENFILE_H
#define	_OPENFILE_H

FILE *openfile(char *, time_t, off_t, off_t *, int(*)(off_t));
int  closefile(int);


#endif

