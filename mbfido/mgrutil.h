/* $Id$ */

#ifndef	_MGRUTIL_H
#define	_MGRUTIL_H


void WriteMailGroups(FILE *, faddr *);
void WriteFileGroups(FILE *, faddr *);
char *GetBool(int);
void CleanBuf(char *);
void ShiftBuf(char *, int);
void MgrPasswd(faddr *, char *, FILE *, int);
void MgrNotify(faddr *, char *, FILE *);
int  UplinkRequest(faddr *, int, char *);

#endif

