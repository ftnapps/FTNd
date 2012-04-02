#ifndef _NEW_H
#define _NEW_H


int  CountNewfiles(void);
int  OpenNewfiles(void);
void CloseNewfiles(int);
void EditNewfiles(void);
void InitNewfiles(void);
int  new_doc(FILE *, FILE *, int);

#endif

