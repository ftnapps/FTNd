/* m_archive.h */

#ifndef _ARCHIVE_H
#define _ARCHIVE_H


int  CountArchive(void);
void EditArchive(void);
char *PickArchive(char *, int);
void InitArchive(void);
int  archive_doc(FILE *, FILE *, int);

#endif

