#ifndef	_BWRITE_H
#define	_BWRITE_H

int iwrite(int,FILE *);
int lwrite(long,FILE *);
int awrite(char *,FILE *);
int cwrite(char *,FILE *, int);
int kwrite(char *,FILE *, int);

#endif

