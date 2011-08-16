#ifndef	_BWRITE_H
#define	_BWRITE_H

/* $Id: bwrite.h,v 1.2 2005/10/11 20:49:47 mbse Exp $ */

int iwrite(int, FILE *);
int lwrite(int, FILE *);
int awrite(char *, FILE *);
int cwrite(char *, FILE *, int);
int kwrite(char *, FILE *, int);

#endif
