/* $Id: taskutil.h,v 1.10 2006/03/06 19:55:10 mbse Exp $ */

#ifndef	_TASKUTIL_H
#define	_TASKUTIL_H


#define TRUE    1
#define FALSE   0
#define SS_BUFSIZE      1024            /* Socket buffersize    */
#define MBSE_SS(x)      (x)?(x):"(null)"


typedef struct  _srv_auth {
        struct  _srv_auth       *next;
        char                    *hostname;
        char                    *authcode;
} srv_auth;



/*
 *  Function prototypes
 */
void		WriteError(const char *, ...);
void		Syslogp(int, char *);
void		Syslog(int, const char *, ...);
int		ulog(char *, char *, char *, char *, char*);
char            *xstrcpy(char *);
char            *xstrcat(char *, char *);
void            CreateSema(char *);
void		TouchSema(char *);
void            RemoveSema(char *);
int		IsSema(char *);
int             file_exist(char *, int);
int		mkdirs(char *, mode_t);
int		file_size(char *);
time_t		file_time(char *);
char		*ascfnode(faddr *, int);
void		ascfnode_r(faddr *, int, char *);
void		fido2str_r(fidoaddr, int, char *);
void		Dos2Unix_r(char *, char *);
char		*dayname(void);
void		InitFidonet(void);
int		SearchFidonet(unsigned short);
char		*printable(char *, int);


#endif

