/* $Id$ */

#ifndef _COMMON_H
#define	_COMMON_H

#include "../config.h"

#pragma pack(1)

#define	PRODCODE    0x00fe		    /* Unasigned 16 bits product code	*/
// #define	PRODCODE    0x11ff	    /* Official MBSE FTSC product code	*/

#define	LEAVE	0
#define	KFS	1
#define	TFS	2
#define	DSF	3


#define MAXNAME 35
#define MAXUFLAGS 16


#define METRIC_EQUAL 0
#define METRIC_POINT 1
#define METRIC_NODE 2
#define METRIC_NET 3
#define METRIC_ZONE 4
#define METRIC_DOMAIN 5
#define METRIC_MAX METRIC_DOMAIN



/*
 * Fidonet message status bits
 */
#define	M_PVT		0x0001
#define	M_CRASH		0x0002
#define	M_RCVD		0x0004
#define	M_SENT		0x0008
#define	M_FILE		0x0010
#define	M_TRANSIT	0x0020
#define	M_ORPHAN	0x0040
#define	M_KILLSENT	0x0080
#define	M_LOCAL		0x0100
#define	M_HOLD		0x0200
#define	M_REQ		0x0800
#define	M_RRQ		0x1000
#define	M_IRR		0x2000
#define	M_AUDIT		0x4000
#define	M_FILUPD	0x8000



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


#ifndef LINES
#define	LINES		24
#endif
#ifndef COLS
#define	COLS		80
#endif


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


#define MAXSUBJ 71
#define MSGTYPE 2


typedef struct _parsedaddr {
	char *target;
	char *remainder;
	char *comment;
} parsedaddr;


#define ADDR_NESTED 1
#define ADDR_MULTIPLE 2
#define ADDR_UNMATCHED 4
#define ADDR_BADTOKEN 8
#define ADDR_BADSTRUCT 16
#define ADDR_ERRMAX 5

/*
 * From rfcaddr.c
 */
char		*addrerrstr(int);
void		tidyrfcaddr(parsedaddr);
parsedaddr	parserfcaddr(char *);


typedef struct _faddr {
	char *name;
	unsigned int point;
	unsigned int node;
	unsigned int net;
	unsigned int zone;
	char *domain;
} faddr;



typedef struct _fa_list {
		struct _fa_list *next;
		faddr 		*addr;
		int		force;
} fa_list;



typedef struct  _ftnmsg {
        int             flags;
        int             ftnorigin;
        faddr           *to;
        faddr           *from;
        time_t          date;
        char            *subj;
        char            *msgid_s;
        char            *msgid_a;
        unsigned long   msgid_n;
        char            *reply_s;
        char            *reply_a;
        unsigned long   reply_n;
        char            *origin;
        char            *area;
} ftnmsg;



extern struct _ftscprod {
	unsigned short code;
	char *name;
} ftscprod[];



extern char SigName[32][16];


int	ttyfd;				/* Filedescriptor for raw mode	*/
struct	termios	tbufs, tbufsavs;	/* Structure for raw mode	*/



/*
 * From endian.c
 */
int le_int(int);



/*
 * From attach.c
 */
int		attach(faddr, char *, int, char);



/*
 * From dostran.c
 */
char		*Dos2Unix(char *);
char		*Unix2Dos(char *);



/*
 * From execute.c
 */
int		execute(char *, char *, char *, char *, char *, char *);
int		execsh(char *, char *, char *, char *);



/*
 * From expipe.c
 */
FILE	*expipe(char *, char *, char *);
int	exclose(FILE *);



/*
 * From faddr.c
 */
char		*aka2str(fidoaddr aka);
fidoaddr	str2aka(char *addr);



/*
 * From falists.c
 */
void		tidy_falist(fa_list **);
void		fill_list(fa_list **,char *,fa_list **);
void		fill_path(fa_list **,char *);
void		sort_list(fa_list **);
void		uniq_list(fa_list **);
int		in_list(faddr *,fa_list **, int);




/*
 * From ftn.c
 */
faddr		*parsefnode(char *);
faddr		*parsefaddr(char *);
char		*ascinode(faddr *,int);
char		*ascfnode(faddr *,int); 
void		tidy_faddr(faddr *);
int		metric(faddr *, faddr *);
faddr		*fido2faddr(fidoaddr);
fidoaddr	*faddr2fido(faddr *);
faddr		*bestaka_s(faddr *);
int		is_local(faddr *);
int		chkftnmsgid(char *);



/*
 * From getheader.c
 */
int		getheader(faddr *, faddr *, FILE *, char *);



/*
 * From gmtoffset.c
 */
long		gmt_offset(time_t);
char		*gmtoffset(time_t);
char		*str_time(time_t);
char		*t_elapsed(time_t, time_t);


/*
 * From mbfile.c
 */
int		file_cp(char *from, char *to);
int		file_rm(char *path);
int		file_mv(char *oldpath, char *newpath);
int		file_exist(char *path, int mode);
long		file_size(char *path);
long		file_crc(char *path, int);
time_t		file_time(char *path);
int             mkdirs(char *name, mode_t);
int		diskfree(int);
int		getfilecase(char *, char *);



/*
 * From nodelock.c
 */
int		nodelock(faddr *);
int		nodeulock(faddr *);


/*
 * From noderecord.c
 */
int		noderecord(faddr *);



/*
 * From pktname.c
 */
char		*prepbuf(faddr *);	
char		*pktname(faddr *, char);
char		*reqname(faddr *);
char		*floname(faddr *, char);
char		*splname(faddr *);
char		*bsyname(faddr *);
char		*stsname(faddr *);
char		*polname(faddr *);
char		*dayname(void);
char		*arcname(faddr *, unsigned short, int);



/*
 * From rawio.c
 */
void		Setraw(void);			/* Set raw mode		    */
void		Unsetraw(void);			/* Unset raw mode	    */
unsigned char	Getone(void);			/* Get one raw character    */
long		Speed(void);			/* Get (locked) tty speed   */
int		Waitchar(unsigned char *, int);	/* Wait n * 10mSec for char */
int		Escapechar(unsigned char *);	/* Escape sequence test	    */
unsigned char	Readkey(void);			/* Read a translated key    */



/*
 * From strutil.c
 */
char		*padleft(char *str, int size, char pad);
char		*tl(char *str);
void		Striplf(char *String);
void		tlf(char *str);
char		*tu(char *str);
char		*tlcap(char *);
char		*Hilite(char *, char *);
void		Addunderscore(char *);
void		strreplace(char *, char *, char*);
char		*GetLocalHM(void); 
char		*StrTimeHM(time_t);
char		*StrTimeHMS(time_t);
char		*GetLocalHMS(void);
char		*StrDateMDY(time_t *);
char		*StrDateDMY(time_t);
char		*GetDateDMY(void);
char		*OsName(void);
char		*OsCPU(void);
char		*TearLine(void);



/*
 * From term.c
 */
void		TermInit(int);
void		Enter(int);
void		pout(int, int, char *);
void		poutCR(int, int, char *);
void		poutCenter(int,int,char *);
void		colour(int, int);
void		Center(char *);
void		clear(void);
void		locate(int, int);
void		fLine(int);
void		sLine(void);
void		mvprintw(int, int, const char *, ...);



/*
 * From unpacker.c
 */
char		*unpacker(char *);
int 		getarchiver(char *);



/*
 * From packet.c
 */
FILE		*openpkt(FILE *, faddr *, char);
void		closepkt(void);



/*
 * From ftnmsg.c
 */
char		*ftndate(time_t);
FILE		*ftnmsghdr(ftnmsg *,FILE *,faddr *,char, char *);
void		tidy_ftnmsg(ftnmsg *);



/*
 * From rfcdate.c
 */
time_t		parsefdate(char *, void *);
char		*rfcdate(time_t);



/*
 * From rfcmsg.c
 */

typedef struct _rfcmsg {
        struct  _rfcmsg *next;
        char    *key;
        char    *val;
} rfcmsg;

rfcmsg *parsrfc(FILE *);
void tidyrfc(rfcmsg *);
void dumpmsg(rfcmsg *,FILE *);


/*
 * From hdr.c
 */
char *hdr(char *, rfcmsg *);



/*
 * From batchrd.c
 */
char *bgets(char *, int, FILE *);



/*
 * some special chars values
 */
// #define NUL         0
// #define NL          10
// #define FF          12
// #define CR          13
// #define ESC         27



/*
 * parsedate.c
 */
typedef struct _TIMEINFO {
    time_t  time;
    long usec;
    long tzone;
} TIMEINFO;

/*
**  Meridian:  am, pm, or 24-hour style.
*/
typedef enum _MERIDIAN {
    MERam, MERpm, MER24
} MERIDIAN;


typedef union {
    time_t		Number;
    enum _MERIDIAN	Meridian;
} CYYSTYPE;

#define	tDAY	257
#define	tDAYZONE	258
#define	tMERIDIAN	259
#define	tMONTH	260
#define	tMONTH_UNIT	261
#define	tSEC_UNIT	262
#define	tSNUMBER	263
#define	tUNUMBER	264
#define	tZONE	265


extern CYYSTYPE cyylval;


time_t parsedate(char *, TIMEINFO *);



/*
 * strcasestr.c
 */
#ifndef	HAVE_STRCASESTR
char *strcasestr(char *, char *);
#endif



/*
 * mangle.c
 */
void	mangle_name_83( char *);    /* Mangle name to 8.3 format	*/
void	name_mangle(char *);	    /* Mangle name or make uppercase	*/


/*
 * sectest.c
 */
int  Access(securityrec, securityrec);  /* Check security access	*/
int  Le_Access(securityrec, securityrec);  /* Endian independant	*/

#endif

