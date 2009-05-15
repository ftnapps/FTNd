#ifndef _AREAMGR_H
#define _AREAMGR_H


void A_Status(faddr *, char *);
void A_List(faddr *, char *, int);
void A_Flow(faddr *, char *, int);
int  AreaMgr(faddr *, faddr *, char *, char *, time_t, int, FILE *);


#endif

