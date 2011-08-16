#ifndef CALLSTAT_H
#define CALLSTAT_H

/* $Id: callstat.h,v 1.5 2006/01/30 22:27:03 mbse Exp $ */


typedef struct _callstat {
	int trytime;
	int tryno;
	int trystat;
} callstat;


void getstatus_r(faddr *, callstat *);

#endif
