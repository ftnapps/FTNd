/* $Id$ */

#ifndef CALLSTAT_H
#define CALLSTAT_H


typedef struct _callstat {
	time_t trytime;
	int tryno;
	int trystat;
} callstat;

callstat *getstatus(faddr*);
void putstatus(faddr*,int,int);

#endif
