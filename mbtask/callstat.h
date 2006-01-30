#ifndef CALLSTAT_H
#define CALLSTAT_H

/* $Id$ */


typedef struct _callstat {
	int trytime;
	int tryno;
	int trystat;
} callstat;


void getstatus_r(faddr *, callstat *);

#endif
