/* $Id$ */

#ifndef	_MGRUTIL_H
#define	_MGRUTIL_H


/*
 * Linked list for atea areas create
 */
typedef struct _AreaList {
	struct _AreaList    *next;
	char		    Name[51];
	int		    IsPresent;
	int		    DoDelete;
} AreaList;


void MacroRead(FILE *, FILE *);
int  MsgResult(const char *, FILE * );
void GetRpSubject(const char *, char*);

void WriteMailGroups(FILE *, faddr *);
void WriteFileGroups(FILE *, faddr *);
void CleanBuf(char *);
void ShiftBuf(char *, int);
void MgrPasswd(faddr *, char *, FILE *, int, int);
void MgrNotify(faddr *, char *, FILE *, int);
int  UplinkRequest(faddr *, faddr *, int, char *);
int  Areas(void);


#endif

