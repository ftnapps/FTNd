#ifndef _CLCOMM_H
#define	_CLCOMM_H

/* $Id$ */


#pragma pack(1)

#define	SS_BUFSIZE	1024		/* Socket buffersize	*/
#define MBSE_SS(x) (x)?(x):"(null)"

/*
 * Logging flagbits, ' ' ? ! + -
 */
#define	DLOG_ALLWAYS	0x00000001
#define	DLOG_ERROR	0x00000002
#define	DLOG_ATTENT	0x00000004
#define	DLOG_NORMAL	0x00000008
#define	DLOG_VERBOSE	0x00000010



/*
 * Debug levels: A B C D E F H I L M N O P R S T X Z
 */
#define	DLOG_TCP	0x00000020
#define	DLOG_BBS	0x00000040
#define	DLOG_CHAT	0x00000080
#define	DLOG_DEVIO	0x00000100
#define	DLOG_EXEC	0x00000200
#define	DLOG_FILEFWD	0x00000400
#define	DLOG_HYDRA	0x00001000
#define	DLOG_IEMSI	0x00002000
#define	DLOG_LOCK	0x00010000
#define	DLOG_MAIL	0x00020000
#define	DLOG_NODELIST	0x00040000
#define	DLOG_OUTSCAN	0x00080000
#define	DLOG_PACK	0x00100000
#define	DLOG_ROUTE	0x00400000
#define	DLOG_SESSION	0x00800000
#define	DLOG_TTY	0x01000000
#define	DLOG_XMODEM	0x10000000
#define	DLOG_ZMODEM	0x40000000



typedef struct	_srv_auth {
	struct	_srv_auth	*next;
	char			*hostname;
	char			*authcode;
} srv_auth;


extern char SigName[32][16];


/*
 *  From clcomm.c
 */
char		*xmalloc(size_t);
char		*xstrcpy(char *);
char		*xstrcat(char *, char *);
void		InitClient(char *, char *, char *, char *, long, char *, char *, char *);
void		ExitClient(int);
void		SockS(const char *, ...);
char		*SockR(const char *, ...);
void		WriteError(const char *, ...);
void		Syslog(int, const char *, ...);
void		Syslogp(int, char *);
void		Mgrlog(const char *, ...);
void		RegTCP(void);
void		IsDoing(const char *, ...);
void		SetTTY(char *);
void		UserCity(pid_t, char *, char *);
void		DoNop(void);
void		Nopper(void);
void		Altime(int);
unsigned long	sequencer(void);
char		*printable(char *, int);
char		*printablec(char);



/*
 * From client.c
 */
int		socket_connect(char *, char *, char *);
int		socket_send(char *);
char		*socket_receive(void);
int		socket_shutdown(pid_t);



/*
 *  From crc.c
 */
unsigned long  crc32ccitt(char *, int);
unsigned short crc16ccitt(char *, int);
unsigned long  str_crc32(char *str);
unsigned long  StringCRC32(char *);
unsigned long  upd_crc32(char *buf, unsigned long crc, int len);
unsigned long  norm_crc32(unsigned long crc);
unsigned short crc16xmodem(char *, int);
unsigned char  checksum(char *, int);



/*
 *  from semafore.c
 */
void            CreateSema(char *);
void            RemoveSema(char *);
int             IsSema(char *);


#endif

