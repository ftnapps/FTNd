/* $Id$ */

#ifndef	_MGRUTIL_H
#define	_MGRUTIL_H


void MacroRead(FILE *, FILE *);
int  MsgResult(const char *, FILE * );
void GetRpSubject(const char *, char*);

void WriteMailGroups(FILE *, faddr *);
void WriteFileGroups(FILE *, faddr *);
char *GetBool(int);
void CleanBuf(char *);
void ShiftBuf(char *, int);
void MgrPasswd(faddr *, char *, FILE *, int, int);
void MgrNotify(faddr *, char *, FILE *, int);
int  UplinkRequest(faddr *, int, char *);

#endif

