#ifndef CALLSTAT_H
#define CALLSTAT_H

/* $Id$ */


typedef struct _callstat {
	int trytime;
	int tryno;
	int trystat;
} callstat;


callstat *getstatus(faddr*);

#endif
