#ifndef _TICAREA_H
#define _TICAREA_H


int  OpenTicarea(void);
void CloseTicarea(int);
int  NodeInTic(fidoaddr);
int  CountTicarea(void);
void EditTicarea(void);
void InitTicarea(void);
char *PickTicarea(char *);
int  GroupInTic(char *);
int  tic_areas_doc(FILE *, FILE *, int);


#endif

