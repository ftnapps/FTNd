/* $Id$ */

#ifndef _MBFUTIL_H_
#define _MBFUTIL_H

void	ProgName(void);			/* Program name header		*/
void	die(int onsig);			/* Shutdown and cleanup		*/
void	Help(void);			/* Show help screen		*/
void	Marker(void);			/* Eyecatcher			*/
void	DeleteVirusWork(void);		/* Delete unarc directory	*/
int	UnpackFile(char *File);		/* Unpack archive		*/
int	AddFile(struct FILERecord, int, char *, char *, char *);
int	CheckFDB(int, char *);		/* Check FDB of area		*/
int	LoadAreaRec(int);		/* Load Area record		*/

#endif
