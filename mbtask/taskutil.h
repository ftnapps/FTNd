/* $Id$ */

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
long		file_size(char *);
time_t		file_time(char *);
char		*ascfnode(faddr *, int);
char		*fido2str(fidoaddr, int);
char		*Dos2Unix(char *);
char		*dayname(void);
void		InitFidonet(void);
int		SearchFidonet(unsigned short);
char		*printable(char *, int);


#endif

