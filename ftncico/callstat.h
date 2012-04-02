/* $Id: callstat.h,v 1.4 2005/10/16 12:13:20 mbse Exp $ */

#ifndef CALLSTAT_H
#define CALLSTAT_H


typedef struct _callstat {
	int trytime;
	int tryno;
	int trystat;
} callstat;

callstat *getstatus(faddr*);
void putstatus(faddr*,int,int);

#endif
