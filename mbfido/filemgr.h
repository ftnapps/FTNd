#ifndef _FILEMGR_H
#define _FILEMGR_H


void F_Status(faddr *);
void F_List(faddr *, int);
int  FileMgr(faddr *, faddr *, time_t, int, FILE *);


#endif

