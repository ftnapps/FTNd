#ifndef _AREAMGR_H
#define _AREAMGR_H


void A_Status(faddr *);
void A_List(faddr *, int);
void A_Flow(faddr *, int);
int  AreaMgr(faddr *, faddr *, time_t, int, FILE *);


#endif

