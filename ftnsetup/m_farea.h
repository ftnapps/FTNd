#ifndef _FAREA_H
#define _FAREA_H

/* $Id: m_farea.h,v 1.3 2005/10/11 20:49:48 mbse Exp $ */

int  CountFilearea(void);
void EditFilearea(void);
int  PickFilearea(char *);
void InitFilearea(void);
int  bbs_file_doc(FILE *, FILE *, int);

#endif

