#ifndef _COMMON_H
#define	_COMMON_H


#pragma pack(1)

#define	MBSE_SS(x) (x)?(x):"(null)"
#define	SS_BUFSIZE	2048


/*
 *  Returned function keys
 */
#define	KEY_BACKSPACE	8
#define	KEY_LINEFEED	10
#define KEY_ENTER	13
#define	KEY_ESCAPE	27
#define KEY_RUBOUT	127
#define	KEY_UP		200
#define	KEY_DOWN	201
#define	KEY_LEFT	202
#define	KEY_RIGHT	203
#define	KEY_HOME	204
#define	KEY_END		205
#define	KEY_INS		206
#define	KEY_DEL		207
#define	KEY_PGUP	208
#define	KEY_PGDN	209


#define	LINES		24
#define	COLS		80


/*
 * ANSI colors
 */
#define BLACK		0
#define	BLUE		1
#define	GREEN		2
#define	CYAN		3
#define	RED		4
#define	MAGENTA		5
#define	BROWN		6
#define	LIGHTGRAY	7
#define	DARKGRAY	8
#define	LIGHTBLUE	9
#define	LIGHTGREEN	10
#define	LIGHTCYAN	11
#define	LIGHTRED	12
#define	LIGHTMAGENTA	13
#define	YELLOW		14
#define	WHITE		15

#define	ANSI_CLEAR	"\x1B[2J"
#define	ANSI_HOME	"\x1B[H"

extern char SigName[32][16];



int	ttyfd;				/* Filedescriptor for raw mode	    */
struct	termio	tbuf, tbufsav;		/* Structure for raw mode	    */


void		InitClient(char *);
void		ExitClient(int);
void		SockS(const char *, ...);
char		*SockR(const char *, ...);
void		Syslog(int, const char *, ...);
void		IsDoing(const char *, ...);
void		Nopper(void);
int		socket_connect(char *);
int		socket_send(char *);
char		*socket_receive(void);
int		socket_shutdown(pid_t);
unsigned long	str_crc32(char *str);
unsigned long	StringCRC32(char *);
long		gmt_offset(time_t);
char		*gmtoffset(time_t);
char		*str_time(time_t);
char		*t_elapsed(time_t, time_t);
void		Setraw(void);			/* Set raw mode		    */
void		Unsetraw(void);			/* Unset raw mode	    */
int		Waitchar(unsigned char *, int);	/* Wait n * 10mSec for char */
int		Escapechar(unsigned char *);	/* Escape sequence test	    */
char		*xstrcpy(char *);
char		*padleft(char *str, int size, char pad);
void		Striplf(char *String);
void		colour(int, int);
void		clear(void);
void		locate(int, int);
void		mvprintw(int, int, const char *, ...);


#endif

