#ifndef CALLSTAT_H
#define CALLSTAT_H


#define	ST_PORTERR	1
#define	ST_NOCONN	2
#define	ST_MDMERR	3
#define	ST_LOCKED	4
#define	ST_LOOKUP	6
#define	ST_NOCALL7	7
#define	ST_NOCALL8	8
#define	ST_NOPORT	9
#define	ST_NOTZMH	10
#define	ST_SESSION	30


typedef struct _callstat {
	time_t trytime;
	int tryno;
	int trystat;
} callstat;

callstat *getstatus(faddr*);
void putstatus(faddr*,int,int);

#endif
