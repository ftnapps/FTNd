/* $Id$ */

#ifndef _COMMON_H
#define	_COMMON_H

#include "../config.h"

#pragma pack(1)

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
 * Analogue Modem flag values, order is important, first the
 * compresion capabilities, then the linespeeds. This is late
 * tested by portsel to find the fastest common connection
 * speed for a given line if you have multiple dialout modems.
 */
#define NL_MNP          0x00000001L
#define NL_V42          0x00000002L
#define NL_V42B         0x00000004L
#define NL_V22		0x00000008L
#define NL_V29		0x00000010L
#define	NL_V32		0x00000020L
#define NL_H96          0x00000040L
#define NL_HST          0x00000080L
#define NL_MAX          0x00000100L
#define NL_PEP          0x00000200L
#define NL_CSP          0x00000400L
#define NL_V32B		0x00000800L
#define NL_H14          0x00001000L
#define NL_V32T         0x00002000L
#define NL_H16          0x00004000L
#define NL_ZYX          0x00008000L
#define NL_Z19          0x00010000L
#define NL_VFC		0x00020000L
#define NL_V34          0x00040000L
#define	NL_X2C		0x00080000L
#define	NL_X2S		0x00100000L
#define	NL_V90C		0x00200000L
#define	NL_V90S		0x00400000L



/*
 * ISDN Flags
 */
#define ND_V110L	0x00000001L
#define	ND_V110H	0x00000002L
#define	ND_V120L	0x00000004L
#define	ND_V120H	0x00000008L
#define	ND_X75		0x00000010L



/*
 * TCP/IP flags
 */
#define	IP_IBN		0x00000001L
#define	IP_IFC		0x00000002L
#define	IP_ITN		0x00000004L
#define	IP_IVM		0x00000008L
#define	IP_IP		0x00000010L
#define IP_IFT		0x00000020L



/*
 * Online special flags
 */
#define OL_CM		0x00000001L
#define	OL_MO		0x00000002L
#define	OL_LO		0x00000004L
#define	OL_MN		0x00000008L



/*
 * Request flags
 */
#define RQ_RQMODE	0x0000000fL 
#define RQ_RQ_BR	0x00000001L
#define RQ_RQ_BU	0x00000002L
#define RQ_RQ_WR	0x00000004L
#define RQ_RQ_WU	0x00000008L
#define RQ_XA	(RQ_RQ_BR | RQ_RQ_BU | RQ_RQ_WR | RQ_RQ_WU)
#define RQ_XB	(RQ_RQ_BR | RQ_RQ_BU | RQ_RQ_WR           )
#define RQ_XC	(RQ_RQ_BR |            RQ_RQ_WR | RQ_RQ_WU)
#define RQ_XP	(RQ_RQ_BR | RQ_RQ_BU                      )
#define RQ_XR	(RQ_RQ_BR |            RQ_RQ_WR           )
#define RQ_XW	(                      RQ_RQ_WR           )
#define RQ_XX	(                      RQ_RQ_WR | RQ_RQ_WU)



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


#define MAXSUBJ 71
#define MSGTYPE 2


/*
#define FLG_PVT 0x0001
#define FLG_CRS 0x0002
#define FLG_RCV 0x0004
#define FLG_SNT 0x0008
#define FLG_ATT 0x0010
#define FLG_TRN 0x0020
#define FLG_ORP 0x0040
#define FLG_K_S 0x0080
#define FLG_LOC 0x0100
#define FLG_HLD 0x0200
#define FLG_RSV 0x0400
#define FLG_FRQ 0x0800
#define FLG_RRQ 0x1000
#define FLG_RRC 0x2000
#define FLG_ARQ 0x4000
#define FLG_FUP 0x8000
*/


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



/*
 *  Nodelist entry
 */
typedef struct	_node {
	faddr		addr;			/* Node address		*/
	unsigned short	upnet;			/* Uplink netnumber	*/
	unsigned short  upnode;			/* Uplink nodenumber	*/
	unsigned short	region;			/* Region belongin to	*/
	unsigned char	type;
	unsigned char	pflag;
	char		*name;			/* System name		*/
	char		*location;		/* System location	*/
	char		*sysop;			/* Sysop name		*/
	char		*phone;			/* Phone number		*/
	unsigned	speed;			/* Baudrate		*/
	unsigned long	mflags;			/* Modem flags		*/
	unsigned long	dflags;			/* ISDN flags		*/
	unsigned long	iflags;			/* TCP-IP flags		*/
	unsigned long	oflags;			/* Online flags		*/
	unsigned long	xflags;			/* Request flags	*/
	char		*uflags[MAXUFLAGS];	/* User flags		*/
} node;



extern struct _fkey {
	char		*key;
	unsigned long	flag;
} fkey[];



extern struct _dkey {
	char		*key;
	unsigned long	flag;
} dkey[];



extern struct _ikey {
	char		*key;
	unsigned long	flag;
} ikey[];



extern struct _okey {
	char		*key;
	unsigned long	flag;
} okey[];



extern struct _xkey {
	char		*key;
	unsigned long	flag;
} xkey[];



extern struct _nodelist {
	char		*domain;
	FILE		*fp;
} *nodevector;



struct _ixentry {
	unsigned short	zone;
	unsigned short	net;
	unsigned short	node;
	unsigned short	point;
};



extern struct _pkey {
	char		*key;
	unsigned char	type;
	unsigned char	pflag;
} pkey[];


extern char SigName[32][16];


int	ttyfd;				/* Filedescriptor for raw mode	*/
struct	termios	tbufs, tbufsavs;	/* Structure for raw mode	*/



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
int             mkdirs(char *name);
int		diskfree(int);


/*
 * From nodelist.c
 */
int		initnl(void);
node		*getnlent(faddr *);
void		olflags(unsigned long);
void		rqflags(unsigned long);
void		moflags(unsigned long);
void		diflags(unsigned long);
void		ipflags(unsigned long);



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
 * Frome mime.c
 */
char		*qp_decode(char *);
/* int=0 for text (normal mode), int=1 for headers and gatebau MSGID */
char		*qp_encode(char *,int);
char		*b64_decode(char *);
char		*b64_encode(char *);



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
 *  recognized charsets
 */
#define CHRS_AUTODETECT		-1
#define CHRS_NOTSET		 0
#define CHRS_ASCII		 1 /* us-ascii */
#define CHRS_BIG5		 2 /* Chinese Big5 charset */
#define CHRS_CP424		 3 /* hebrew EBCDIC */
#define CHRS_CP437		 4 /* Latin-1 MS codage (cp437) */
#define CHRS_CP850		 5 /* Latin-1 MS codage (cp850) */
#define CHRS_CP852		 6 /* Polish MS-DOS codage */
#define CHRS_CP862		 7 /* Hebrew PC */
#define CHRS_CP866		 8 /* Cyrillic Alt-PC (cp866) */
#define CHRS_CP895		 9 /* Kamenicky (DOS charset in CZ & SK) */
#define CHRS_EUC_JP		10 /* Japanese EUC */
#define CHRS_EUC_KR		11 /* Korean EUC */
#define CHRS_FIDOMAZOVIA	12 /* Polish "FIDOMAZOVIA" charset */
#define CHRS_GB			13 /* Chinese GB 2312 8 bits */
#define CHRS_HZ			14 /* Chinese HZ coding */
#define CHRS_ISO_2022_CN	15 /* Chinese GB 2312 7 bits */
#define CHRS_ISO_2022_JP	16 /* Japanese iso-2022-jp */
#define CHRS_ISO_2022_KR        17 /* Korean iso-2022-kr */
#define CHRS_ISO_2022_TW	18 /* Taiwanese iso-2022-tw */
#define CHRS_ISO_8859_1		19 /* Latin-1, Western Europe, America */ 
#define CHRS_ISO_8859_1_QP	20
#define CHRS_ISO_8859_2		21 /* Latin-2, Eastern Europe */
#define CHRS_ISO_8859_3		22 /* Latin-3, Balkanics languages */
#define CHRS_ISO_8859_4		23 /* Latin-4, Scandinavian, Baltic */
#define CHRS_ISO_8859_5		24 /* Cyrillic (iso-8859-5) */
#define CHRS_ISO_8859_6		25 /* Arabic (iso-8859-6) */
#define CHRS_ISO_8859_7		26 /* Greek (iso-8859-7) */
#define CHRS_ISO_8859_8		27 /* Hebrew (iso-8859-8) */
#define CHRS_ISO_8859_9		28 /* Latin-5, Turkish */
#define CHRS_ISO_8859_10	29 /* Latin-6, Lappish/Nordic/Eskimo */
#define CHRS_ISO_8859_11	30 /* Thai (iso-8859-11, aka TIS620) */
#define CHRS_ISO_8859_15	31 /* Latin-0 (Latin-1 + a few letters) */
#define CHRS_KOI8_R		32 /* Cyrillic Koi8 (Russian) */
#define CHRS_KOI8_U		33 /* Cyrillic Koi8 (Ukranian) */
#define CHRS_MACINTOSH		34 /* Macintosh */
#define CHRS_MIK_CYR		35 /* Bulgarian "Mik" cyrillic charset */
#define CHRS_NEC		36 /* Japanese NEC-JIS charset */
#define CHRS_SJIS		37 /* Japanese Shift-JIS (MS codage) */
#define CHRS_UTF_7		38 /* Unicode in UTF-7 encoding */
#define CHRS_UTF_8		39 /* Unicode in UTF-8 encoding */
#define CHRS_VISCII_10		40 /* VISCII 1.0 */
#define CHRS_VISCII_11		41 /* VISCII 1.1 */
#define CHRS_ZW			42 /* Chinese Zw encoding */

#define CHRS_ISO_11             91
#define CHRS_ISO_4              92
#define CHRS_ISO_60             93



/*
 * languages (used for LANG_DEFAULT definition)
 */
#define LANG_WEST		1 /* West-European languages */	
#define LANG_EAST		2 /* East-Eurpean languages */
#define LANG_JAPAN		3 /* japanese */
#define LANG_KOREA		4 /* korean */
#define LANG_CHINA		5 /* chinese */
#define LANG_CYRILLIC		6 /* Cyrillic based languages */



/*
 * Define these according to the values used in your country
 */
#define CHRS_DEFAULT_FTN	CHRS_CP437
#define CHRS_DEFAULT_RFC	CHRS_ISO_8859_1
#define LANG_DEFAULT		LANG_WEST

#if (LANG_DEFAULT==LANG_JAPAN || LANG_DEFAULT==LANG_KOREA || LANG_DEFAULT==LANG_CHINA)
#define LANG_BITS	16
#else
#define LANG_BITS	8
#endif



/*
 * used to recognize pgpsigned messages
 */
#define PGP_SIGNED_BEGIN	"-----BEGIN PGP SIGNED MESSAGE-----"
#define PGP_SIG_BEGIN		"-----BEGIN PGP SIGNATURE-----"
#define PGP_SIG_END		"-----END PGP SIGNATURE-----"



/*
 * charset reading functions
 */
int	getoutcode(int);
int	getincode(int);
char	*getcharset(int);
char	*getchrs(int);
int	getcode(char *);
int	readchrs(char *);
int	readcharset(char *);
void	writechrs(int,FILE *,int);



/*
 * some special chars values
 */
#define NUL         0
#define NL          10
#define FF          12
#define CR          13
#define ESC         27


/* ************ general functions ************* */
char	*hdrconv(char *, int, int);
char	*hdrnconv(char *, int, int, int);
char	*strnkconv(const char *, int, int, int);
char	*strkconv(const char *, int, int);
void	kconv(char *, char **, int, int);


/* ************ 8 bit charsets **************** */
void	noconv(char *, char **);
void	eight2eight(char *, char **, char *);



/*
 * maptabs names
 */
#define CP424__CP862		"cp424__cp862"
#define CP424__ISO_8859_8	"cp424__iso-8859-8"
#define CP437__ISO_8859_1	"cp437__iso-8859-1"
#define CP437__MACINTOSH	"cp437__mac"
#define CP850__ISO_8859_1	"cp437__iso-8859-1"
#define CP850__MACINTOSH	"cp437__mac"
#define CP852__FIDOMAZOVIA	"cp852__fidomazovia"
#define CP852__ISO_8859_2	"cp852__iso-8859-2"
#define CP862__CP424		"cp862__cp424"
#define CP862__ISO_8859_8	"cp862__iso-8859-8"
#define CP866__ISO_8859_5	"mik__iso-8859-5"
#define CP866__KOI8		"cp866__koi8"
#define CP895__CP437            "cp895__cp437"
#define CP895__ISO_8859_2       "cp895__iso-8859-2"
#define FIDOMAZOVIA__CP852	"fidomazovia__cp852"
#define FIDOMAZOVIA__ISO_8859_2	"fidomazovia__iso-8859-2"
#define ISO_11__ISO_8859_1	"iso-11__iso-8859-1"
#define ISO_4__ISO_8859_1	"iso-4__iso-8859-1"
#define ISO_60__ISO_8859_1	"iso-60__iso-8859-1"
#define ISO_8859_1__CP437	"iso-8859-1__cp437"
#define ISO_8859_1__MACINTOSH	"iso-8859-1__mac"
#define ISO_8859_1__CP850	"iso-8859-1__cp437"
#define ISO_8859_2__CP852	"iso-8859-2__cp852"
#define ISO_8859_2__CP895       "iso-8859-2__cp895"
#define ISO_8859_2__FIDOMAZOVIA	"iso-8859-2__fidomazovia"
#define ISO_8859_5__CP866	"iso-8859-5__mik"
#define ISO_8859_5__KOI8	"iso-8859-5__koi8"
#define ISO_8859_5__MIK_CYR	"iso-8859-5__mik"
#define ISO_8859_8__CP424	"iso-8859-8__cp424"
#define ISO_8859_8__CP862	"iso-8859-8__cp862"
#define KOI8__CP866		"koi8__cp866"
#define KOI8__ISO_8859_5	"koi8__iso-8859-5"
#define KOI8__MIK_CYR		"koi8__mik"
#define MACINTOSH__CP437	"mac__cp437"
#define MACINTOSH__CP850	"mac__cp437"
#define MACINTOSH__ISO_8859_1	"mac__iso-8859-1"
#define MIK_CYR__ISO_8859_5	"mik__iso-8859-5"
#define MIK_CYR__KOI8		"mik__koi8"


/* ??? */
int	SkipESCSeq(FILE *, int, int *);
int	getkcode(int, char [],char []);
int	iso2022_detectcode(char *, int);


#define DOS
#define SPACE           0xA1A1          /* GB "space" symbol */
#define BOX             0xA1F5          /* GB "blank box" symbol */
#define isGB1(c)        ((c)>=0x21 && (c)<=0x77)        /* GB 1st byte */
#define isGB1U(c)       ((c)>=0x78 && (c)<=0x7D)        /* GB 1st byte unused*/
#define isGB2(c)        ((c)>=0x21 && (c)<=0x7E)        /* GB 2nd byte */
#define HI(code)        (((code) & 0xFF00)>>8)
#define LO(code)        ((code) & 0x00FF)
#define DB(hi,lo)       ((((hi)&0xFF) << 8) | ((lo)&0xFF))
#define CLEAN7(c)       ((c) & 0x7F)                    /* strip MSB */
#define notAscii(c)     ((c)&0x80)


/* Chinese charsets */
void gb2hz(char *in, char **out);
void hz2gb(char *in, char **out);
void zw2hz(char *in, char **out);
void zw2gb(char *in, char **out);



#define SJIS1(A)    ((A >= 129 && A <= 159) || (A >= 224 && A <= 239))
#define SJIS2(A)    (A >= 64 && A <= 252)
#define HANKATA(A)  (A >= 161 && A <= 223)
#define ISEUC(A)    (A >= 161 && A <= 254)
#define ISMARU(A)   (A >= 202 && A <= 206)
#define ISNIGORI(A) ((A >= 182 && A <= 196) || (A >= 202 && A <= 206))

void	OPENINOUTFILES(FILE **, FILE **, char *);
void	CLOSEINOUTFILES(FILE **, FILE **, char **);
void	han2zen(FILE *, int *, int *, int);
void	sjis2jis(int *, int *);
void	jis2sjis(int *, int *);

/* ************ 16 bits charsets ************* */
/* japanese charsets */
void	shift2seven(char *, char **, int, char [], char []);
void	shift2euc(char *, char **, int, int);
void	euc2seven(char *, char **, int, char [], char []);
void	euc2euc(char *, char **, int, int);
void	shift2shift(char *, char **, int, int);
void	euc2shift(char *, char **, int, int);
void	seven2shift(char *, char **);
void	seven2euc(char *, char **);
void	seven2seven(char *, char **, char [], char []);



void	utf7_to_eight(char *, char **, int *);
void	utf8_to_eight(char *, char **, int *);



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
 * msgflags.c
 */
int	flag_on(char *,char *);
int	flagset(char *);
char	*compose_flags(int,char *);
char	*strip_flags(char *);
int	flag_on(char *,char *);



/*
 * strcasestr.c
 */
#ifndef	HAVE_STRCASESTR
char *strcasestr(char *, char *);
#endif

#endif

