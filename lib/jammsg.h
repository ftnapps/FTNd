#ifndef	_JAMMSG_H
#define	_JAMMSG_H


int		JAM_AddMsg(void);
void		JAM_Close(void);
int		JAM_Delete(unsigned long);
int		JAM_GetLastRead(lastread *);
unsigned long	JAM_Highest(void);
int		JAM_Lock(unsigned long);
unsigned long	JAM_Lowest(void);
void		JAM_New(void);
int		JAM_NewLastRead(lastread);
int		JAM_Next(unsigned long *);
unsigned long	JAM_Number(void);
int		JAM_Open(char *);
void		JAM_Pack(void);
int		JAM_Previous(unsigned long *);
int		JAM_ReadHeader(unsigned long);
int		JAM_Read(unsigned long, int);
int		JAM_SetLastRead(lastread);
void		JAM_UnLock(void);
int		JAM_WriteHeader(unsigned long);


#endif

