/* $Id: jammsg.h,v 1.3 2005/10/11 20:49:42 mbse Exp $ */

#ifndef	_JAMMSG_H
#define	_JAMMSG_H


int		JAM_AddMsg(void);
void		JAM_Close(void);
int		JAM_Delete(unsigned int);
void		JAM_DeleteJAM(char *);
int		JAM_GetLastRead(lastread *);
unsigned int	JAM_Highest(void);
int		JAM_Lock(unsigned int);
unsigned int	JAM_Lowest(void);
void		JAM_New(void);
int		JAM_NewLastRead(lastread);
int		JAM_Next(unsigned int *);
unsigned int	JAM_Number(void);
int		JAM_Open(char *);
void		JAM_Pack(void);
int		JAM_Previous(unsigned int *);
int		JAM_ReadHeader(unsigned int);
int		JAM_Read(unsigned int, int);
int		JAM_SetLastRead(lastread);
void		JAM_UnLock(void);
int		JAM_WriteHeader(unsigned int);


#endif

