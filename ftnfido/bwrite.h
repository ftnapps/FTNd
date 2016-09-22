#ifndef	_BWRITE_H
#define	_BWRITE_H

/* bwrite.h */

int iwrite(int, FILE *);
int lwrite(int, FILE *);
int awrite(char *, FILE *);
int cwrite(char *, FILE *, int);
int kwrite(char *, FILE *, int);

#endif
