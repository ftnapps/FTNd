#ifndef	_UTIC_H
#define	_UTIC_H

/* $Id: utic.h,v 1.3 2005/12/03 14:52:35 mbse Exp $ */

char *MakeTicName(void);
int  Day_Of_Year(void);
int  Rearc(char *);
void Bad(char *, ...);
void ReCalcCrc(char *);
int  Get_File_Id(void);


#endif
