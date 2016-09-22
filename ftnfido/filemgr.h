#ifndef _FILEMGR_H
#define _FILEMGR_H


void F_Status(faddr *, char *);
void F_List(faddr *, char *, int);
int  FileMgr(faddr *, faddr *, char *, char *, time_t, int, FILE *);


#endif

