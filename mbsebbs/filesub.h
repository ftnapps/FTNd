#ifndef _FILESUB_H
#define	_FILESUB_H

/* $Id$ */

FILE		*OpenFareas(int);
int		ForceProtocol(void);
int		CheckBytesAvailable(long);
int		iLC(int);
void		Header(void);
void		Sheader(void);
int		ShowOneFile(void);
int		Addfile(char *, int, int);
void		InitTag(void);
void		SetTag(_Tag);
void		Blanker(int);
void		GetstrD(char *, int);
void		Mark(void);
int		UploadB_Home(char *);
char		*GetFileType(char *);
void		Home(void);
int		ScanDirect(char *);
int		ScanArchive(char *, char *);
int		ImportFile(char *, int, int, time_t, off_t);
unsigned long	Quota(void);
void		ImportHome(char *);

#endif
