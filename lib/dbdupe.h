#ifndef	_DBDUPE_H
#define	_DBDUPE_H

typedef enum {D_ECHOMAIL, D_FILEECHO, D_NEWS} DUPETYPE;

void	InitDupes(void);
int	CheckDupe(unsigned long, int, int);
void	CloseDupes(void);

#endif


