/* $Id: createm.h,v 1.4 2005/08/10 18:57:22 mbse Exp $ */

#ifndef	_CREATEM_H
#define	_CREATEM_H


int	create_msgarea(char *, faddr *);
int	CheckEchoGroup(char *, int, faddr *);
void	msged_areas(FILE *);
void	gold_areas(FILE *);
void	gold_akamatch(FILE *);

#endif

