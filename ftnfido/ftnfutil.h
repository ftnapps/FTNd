/* ftnfutil.h */

#ifndef _FTNFUTIL_H_
#define _FTNFUTIL_H

void	ProgName(void);			/* Program name header		*/
void	die(int onsig);			/* Shutdown and cleanup		*/
void	Help(void);			/* Show help screen		*/
void	Marker(void);			/* Eyecatcher			*/
int	UnpackFile(char *File);		/* Unpack archive		*/
int	AddFile(struct FILE_record, int, char *, char *, char *);
int	CheckFDB(int, char *);		/* Check FDB of area		*/
int	LoadAreaRec(int);		/* Load Area record		*/
int	is_real_8_3(char *);		/* Check for 8.3 uppercase	*/

#endif
