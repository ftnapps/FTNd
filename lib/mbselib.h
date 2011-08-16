/*****************************************************************************
 *
 * $Id: mbselib.h,v 1.106 2008/12/28 12:20:14 mbse Exp $
 * Purpose ...............: MBSE BBS main library header
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
 *   
 * Michiel Broek                FIDO:           2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#ifndef _MBSELIB_H
#define	_MBSELIB_H

#include "../config.h"


/*
 * System libraries for all sources
 */
#ifndef _GNU_SOURCE
#define	_GNU_SOURCE 1
#endif
#define	_REGEX_RE_COMP

#define	TRUE 1
#define	FALSE 0


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>   
#include <utime.h>
#include <stdarg.h>
#include <pwd.h>
#include <netdb.h>
#ifdef	SHADOW_PASSWORD
#include <shadow.h>
#endif
#include <sys/ioctl.h>
#ifdef	HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <sys/file.h>
#include <syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/ftp.h>
#include <arpa/telnet.h>
#include <sys/un.h>
#include <sys/time.h>
#include <regex.h>
#include <setjmp.h>
#include <grp.h>
#include <sys/resource.h>
#ifdef	HAVE_ZLIB_H
#include <zlib.h>
#endif
#ifdef	HAVE_BZLIB_H
#include <bzlib.h>
#endif
#ifdef  HAVE_GEOIP_H
#include <GeoIP.h>
#include <GeoIPCity.h>
#endif
#if !defined(__ppc__)
#include <sys/poll.h>
#endif
#include <locale.h>
#include <langinfo.h>
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif


/* used to use #elif, but native braindead hpux 9.00 c compiler didn't 
 *  * understand it */
#ifdef HAVE_TERMIOS_H
/* get rid of warnings on SCO ODT 3.2 */
struct termios;
# include <termios.h>
# define USE_TERMIOS
#else
# if defined(HAVE_SYS_TERMIOS_H)
#  include <sys/termios.h>
#  define USE_TERMIOS
# else
#  if defined(HAVE_TERMIO_H)
#   include <termio.h>
#   define USE_TERMIO
#  else
#   if defined(HAVE_SYS_TERMIO_H)
#    include <sys/termio.h>
#    define USE_TERMIO
#   else
#    if defined(HAVE_SGTTY_H)
#     include <sgtty.h>
#     define USE_SGTTY
#     ifdef LLITOUT
       extern int Locmode;	/* Saved "local mode" for 4.x BSD "new driver" */
       extern int Locbit;	/* Bit SUPPOSED to disable output translations */
#     endif
#    else
#     error neither termio.h nor sgtty.h found. Cannot continue.
#    endif
#   endif
#  endif
# endif
#endif

#ifdef USE_SGTTY
#  ifdef TIOCSBRK
#    define CANBREAK
#  endif
#endif
#ifdef USE_TERMIO
#  define CANBREAK
#endif

#include <stddef.h>
#include <fcntl.h>
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__ppc__)
#include <netinet/in_systm.h>
#include <libgen.h>
#endif
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#if defined(__NetBSD__)
#include <re_comp.h>
#endif
#if defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/sysctl.h>
#endif


/*
 *  Some older systems don;t have this
 */
#ifndef ICMP_FILTER
#define ICMP_FILTER     1

struct icmp_filter {
            u_int32_t       data;
};

#endif

/* Some old libs don't have socklen_t */
#ifndef socklen_t
#define socklen_t unsigned int
#endif


/*
 * Pragma pack is to make all databases portable
 */
#pragma pack(1)


/*****************************************************************************
 *
 *  Macro's used
 */

#define MBSE_SS(x) (x)?(x):"(null)"
#define SOCKA_A4(a) ((void *)&((struct sockaddr_in *)(a))->sin_addr)


/*****************************************************************************
 *
 *  Defines
 */
#define PRODCODE	0x11ff		/* Official MBSE FTSC product code  */
#define Max_passlen     14		/* Define maximum passwd length     */
#define SS_BUFSIZE      1024            /* Socket buffersize		    */
#define MAXNLLINELEN    1024		/* Maximum nodelist line length	    */



/*
 *  Binkly Style Outbound (BSO) flo status
 */
#define LEAVE   0
#define KFS     1
#define TFS     2
#define DSF     3



#define MAXSUBJ 71


/*
 * Aka matching levels
 */
#define METRIC_EQUAL	0
#define METRIC_POINT	1
#define METRIC_NODE	2
#define METRIC_NET	3
#define METRIC_ZONE	4
#define METRIC_DOMAIN	5
#define METRIC_MAX METRIC_DOMAIN



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



/*
 * Fidonet message status bits
 */
#define M_PVT           0x0001
#define M_CRASH         0x0002
#define M_RCVD          0x0004
#define M_SENT          0x0008
#define M_FILE          0x0010
#define M_TRANSIT       0x0020
#define M_ORPHAN        0x0040
#define M_KILLSENT      0x0080
#define M_LOCAL         0x0100
#define M_HOLD          0x0200
#define M_REQ           0x0800
#define M_RRQ           0x1000
#define M_IRR           0x2000
#define M_AUDIT         0x4000
#define M_FILUPD        0x8000



/*
 *  Returned function keys
 */
#define KEY_BACKSPACE   8
#define KEY_LINEFEED    10
#define KEY_ENTER       13
#define KEY_ESCAPE      27
#define KEY_RUBOUT      127
#define KEY_UP          200
#define KEY_DOWN        201
#define KEY_LEFT        202
#define KEY_RIGHT       203
#define KEY_HOME        204
#define KEY_END         205
#define KEY_INS         206
#if GBK_DEL
#define KEY_DEL         GBK_DEL 
#else
#define KEY_DEL         207
#endif
#define KEY_PGUP        208
#define KEY_PGDN        209
#define	KEY_F10		210
#define	KEY_F1		211
#define	KEY_F2		212
#define KEY_F3          213
#define KEY_F4          214
#define KEY_F5          215
#define KEY_F6          216
#define KEY_F7          217
#define KEY_F8          218
#define KEY_F9          219


#ifndef LINES
#define LINES           24
#endif
#ifndef COLS
#define COLS            80
#endif


/*
 * ANSI colors
 */
#define BLACK           0
#define BLUE            1
#define GREEN           2
#define CYAN            3
#define RED             4
#define MAGENTA         5
#define BROWN           6
#define LIGHTGRAY       7
#define DARKGRAY        8
#define LIGHTBLUE       9
#define LIGHTGREEN      10
#define LIGHTCYAN       11
#define LIGHTRED        12
#define LIGHTMAGENTA    13
#define YELLOW          14
#define WHITE           15



/*
 * ANSI Escape sequences
 */
#define ANSI_RED	"\x1B[31;1m"
#define ANSI_YELLOW	"\x1B[33;1m"
#define ANSI_BLUE	"\x1B[34;1m"
#define ANSI_GREEN	"\x1B[32;1m"
#define ANSI_WHITE	"\x1B[37;1m"
#define ANSI_CYAN	"\x1B[36;1m"
#define ANSI_MAGENTA	"\x1B[35;1m"

#define	ANSI_HOME	"\x1B[H"
#define ANSI_UP		"\x1B[A"
#define ANSI_DOWN	"\x1B[B"
#define	ANSI_RIGHT	"\x1B[C"
#define ANSI_LEFT	"\x1B[D"

#define ANSI_BOLD	"\x1B[1m"
#define ANSI_NORMAL	"\x1B[0m"
#define ANSI_CLEAR	"\x1B[2J"
#define	ANSI_CLREOL	"\x1B[K"



/*
 * Exit status values
 */
#define	MBERR_OK		0	/* No errors			    */
#define	MBERR_COMMANDLINE	100	/* Commandline error		    */
#define	MBERR_CONFIG_ERROR	101	/* Configuration error		    */
#define	MBERR_INIT_ERROR	102	/* Initialisation error		    */
#define	MBERR_DISK_FULL		103	/* Some disk partition full	    */
#define	MBERR_UPS_ALARM		104	/* UPS alarm detected		    */
#define	MBERR_NO_RECIPIENTS	105	/* No valid recipients		    */
#define	MBERR_EXEC_FAILED	106	/* Execute external prog failed	    */
#define	MBERR_TTYIO_ERROR	107	/* Set tty failed		    */
#define	MBERR_FTRANSFER		108	/* File transfer error		    */
#define	MBERR_ATTACH_FAILED	109	/* File attach failed		    */
#define	MBERR_NO_PROGLOCK	110	/* Cannot lock program, retry later */
#define MBERR_NODE_NOT_IN_LIST	111	/* Node not in nodelist		    */
#define	MBERR_NODE_MAY_NOT_CALL	112	/* Node may not be called	    */
#define	MBERR_NO_CONNECTION	113	/* Cannot make connection	    */
#define	MBERR_PORTERROR		114	/* Cannot open tty port		    */
#define	MBERR_NODE_LOCKED	115	/* Node is locked		    */
#define	MBERR_NO_IP_ADDRESS	116	/* Node IP address not found	    */
#define	MBERR_UNKNOWN_SESSION	117	/* Unknown session		    */
#define	MBERR_NOT_ZMH		118	/* Not Zone Mail Hour		    */
#define	MBERR_MODEM_ERROR	119	/* Modem error			    */
#define	MBERR_NO_PORT_AVAILABLE	120	/* No modemport available	    */
#define	MBERR_SESSION_ERROR	121	/* Session error (password)	    */
#define	MBERR_EMSI		122	/* EMSI session error		    */
#define	MBERR_FTSC		123	/* FTSC session error		    */
#define	MBERR_WAZOO		124	/* WAZOO session error		    */
#define	MBERR_YOOHOO		125	/* YOOHOO session error		    */
#define	MBERR_OUTBOUND_SCAN	126	/* Outbound scan error		    */
#define	MBERR_CANNOT_MAKE_POLL	127	/* Cannot make poll		    */
#define	MBERR_REQUEST		128	/* File request error		    */
#define MBERR_DIFF_ERROR	129	/* Error processing nodediff	    */
#define	MBERR_VIRUS_FOUND	130	/* Virus found			    */
#define	MBERR_GENERAL		131	/* General error		    */
#define	MBERR_TIMEOUT		132	/* Timeout error		    */
#define	MBERR_TTYIO		200	/* Base for ttyio errors	    */
#define	MBERR_EXTERNAL		256	/* Status external prog + 256	    */



#define ADDR_NESTED 1   
#define ADDR_MULTIPLE 2
#define ADDR_UNMATCHED 4
#define ADDR_BADTOKEN 8
#define ADDR_BADSTRUCT 16
#define ADDR_ERRMAX 5



/*****************************************************************************
 *
 *  Supported character sets.
 */
#define	FTNC_ERROR		-1	/* Error entry			    */
#define FTNC_NONE		0	/* Undefined			    */
#define	FTNC_CP437		1	/* IBM CP 437 (Western Europe)	    */
#define	FTNC_CP850		2	/* IBM CP 850 (Latin-1)		    */
#define	FTNC_CP865		3	/* IBM CP 865 (Nordic)		    */
#define	FTNC_CP866		4	/* IBM CP 866 (Russian)		    */
#define	FTNC_LATIN_1		5	/* ISO 8859-1 (Western Europe)	    */
#define	FTNC_LATIN_2		6	/* ISO 8859-2 (Eastern Europe)	    */
#define	FTNC_LATIN_5		7	/* ISO 8859-5 (Turkish)		    */
#define	FTNC_MAC		8	/* MacIntosh character set	    */
#define FTNC_CP852		9	/* IBM CP 852 (Czech, Latin-2)	    */
#define	FTNC_CP895		10	/* IBM CP 895 (Czech, Kamenicky)    */
#define	FTNC_KOI8_R		11	/* Unix koi8-r			    */
#define	FTNC_CP936		12	/* IBM CP 936 (Chinese, GBK)	    */
#define	FTNC_LATIN_9		13	/* ISO 8859-15 (West Europe EURO    */
#define	FTNC_UTF8		14	/* UTF-8			    */
#define FTNC_MAXCHARS		14	/* Highest charset number	    */


extern struct _charalias {
    char 	*alias;
    char 	*ftnkludge;
} charalias[];


extern struct _charmap {
    int		ftncidx;
    char 	*ftnkludge;
    char 	*rfcname;
    char 	*ic_ftn;
    char 	*lang;
    char 	*desc;
} charmap[];


/*****************************************************************************
 *
 *  Global typedefs.
 *
 */
typedef enum {YES, NO, ASK} ASKTYPE;
typedef enum {LOCALMAIL, NETMAIL, ECHOMAIL, NEWS, LIST} MSGTYPE;
typedef enum {BOTH, PRIVATE, PUBLIC, RONLY, FTNMOD, USEMOD} MSGKINDSTYPE;
typedef enum {IGNORE, CREATE, KILL} ORPHANTYPE;
typedef enum {SEND, RECV, BOTHDIR} NODETYPE;
typedef enum {POTS, ISDN, NETWORK, LOCAL} LINETYPE;
typedef enum {BROWSING, DOWNLOAD, UPLOAD, READ_POST, DOOR, SYSOPCHAT,
	      FILELIST, WHOSON, OLR} DOESTYPE;
typedef enum {I_AVT0, I_ANSI, I_VT52, I_VT100, I_TTY} ITERM;
typedef enum {I_DZA, I_ZAP, I_ZMO, I_SLK, I_KER} IPROT;
typedef enum {E_NOISP, E_TMPISP, E_PRMISP} EMODE;
typedef enum {AREAMGR, FILEMGR, EMAIL} SERVICE;
typedef enum {FEEDINN, FEEDRNEWS, FEEDUUCP} NEWSFEED;
typedef enum {S_DIRECT, S_DIR, S_FTP} SESSIONTYPE;
typedef enum {SCAN_EXTERN, CLAM_STREAM, FP_STREAM} SCANTYPE;



/***********************************************************************
 *
 *  Routing definitions
 */
#define R_NOROUTE   0           /* No route descision made          */
#define R_ROUTE     1           /* Route to destination             */
#define	R_DIRECT    2		/* Direct route			    */
#define	R_REDIRECT  3		/* Redirect to new address	    */
#define	R_BOUNCE    4		/* Bounce back to sender	    */
#define	R_CC	    5		/* Make a CC			    */
#define	R_LOCAL	    6		/* Local destination		    */
#define	R_UNLISTED  7		/* Unlisted destination		    */


/***********************************************************************
 *
 *  Nodelist definitions.
 *
 */

#define MAXNAME 35
#define MAXUFLAGS 16


/*
 *  Nodelist index file to nodelists. (node.files)
 */
typedef struct	_nlfil {
	char		filename[13];		/* Nodelist filename	*/
	char		domain[13];		/* Domain name		*/
	unsigned short	number;			/* File number		*/
} nlfil;



/*
 *  Nodelist index file for node lookup. (node.index)
 */
typedef struct	_nlidx {
	unsigned short	zone;			/* Zone number		*/
	unsigned short	net;			/* Net number		*/
	unsigned short	node;			/* Node number		*/
	unsigned short	point;			/* Point number		*/
	unsigned short	region;			/* Region of node	*/
	unsigned short	upnet;			/* Uplink net		*/
	unsigned short	upnode;			/* Uplink node		*/
	unsigned char	type;			/* Node type		*/
	unsigned char	pflag;			/* Node status		*/
	unsigned short	fileno;			/* Nodelist number	*/
	int		offset;			/* Offset in nodelist	*/
} nlidx;



/*
 *  Nodelist usernames index file. (node.users)
 */
typedef struct	_nlusr {
	char		user[36];		/* User name		*/
	unsigned short	zone;			/* Zone number		*/
	unsigned short	net;			/* Net number		*/
	unsigned short	node;			/* Node number		*/
	unsigned short	point;			/* Point number		*/
} nlusr;



/*
 *  type values
 */
#define NL_NONE		0
#define NL_ZONE		1
#define	NL_REGION	2
#define	NL_HOST		3
#define	NL_HUB		4
#define	NL_NODE		5
#define	NL_POINT	6



/*
 *  pflag values, all bits zero, node may be dialed analogue FTS-0001.
 *  the rest are special cases.
 */
#define NL_DOWN		0x01			/* Node is Down		*/
#define NL_HOLD		0x02			/* Node is Hold		*/
#define	NL_PVT		0x04			/* Private node		*/
#define NL_DUMMY	0x08			/* Dummy entry		*/


/************************************************************************
 *
 *  Other BBS structures
 *
 */


#ifndef _SECURITYSTRUCT
#define _SECURITYSTRUCT

/*
 * Security structure
 */
typedef struct _security {
	unsigned int	level;			/* Security level	   */
	unsigned int	flags;			/* Access flags		   */
	unsigned int	notflags;		/* No Access flags	   */
} securityrec;

#endif


/* 
 * Fidonet 5d address structure 
 */
typedef struct _fidoaddr {
	unsigned short	zone;			/* Zone number		   */
	unsigned short	net;			/* Net number		   */
	unsigned short	node;			/* Node number		   */
	unsigned short	point;			/* Point number		   */
	char		domain[13];		/* Domain name (no dots)   */
} fidoaddr;



/*
 * Connected system structure
 */
typedef	struct _sysconnect {
	fidoaddr	aka;			/* Address of system	   */
	unsigned short	sendto;			/* If we send to system	   */
	unsigned short	receivefrom;		/* If we receive from      */
	unsigned 	pause		: 1;	/* If system is paused	   */
	unsigned 	cutoff		: 1;	/* Cutoff by moderator	   */
	unsigned	spare3		: 1;
	unsigned	spare4		: 1;
	unsigned	spare5		: 1;
	unsigned	spare6		: 1;
	unsigned	spare7		: 1;
	unsigned	spare8		: 1;
	unsigned	spare9		: 1;	/* Forces enough space	   */
} sysconnect;


	int		Diw;			/* Day in week index	   */
	int		Miy;			/* Month in year index	   */


/*
 * Statistic counters structure
 */
typedef struct _statcnt {
	unsigned int	tdow[7];		/* Days of current week	   */
	unsigned int	ldow[7];		/* Days of previous week   */
	unsigned int	tweek;			/* Week total counters	   */
	unsigned int	lweek;			/* Last week counters	   */
	unsigned int	month[12];		/* Monthly total counters  */
	unsigned int	total;			/* The ever growing total  */
} statcnt;



/*
 * Find replace match structure (phone translation etc).
 */
typedef struct _dual {
	char		match[21];		/* String to match	   */
	char		repl[21];		/* To replace with	   */
} dual;



/****************************************************************************
 *
 *  Datafile records structure in $MBSE_ROOT/etc
 *
 */


/*
 * Task Manager configuration (task.data)
 */
struct	taskrec {
	float		maxload;		/* Maximum system load	    */

	char		xisp_connect[81];	/* ISP connect command	    */
	char		xisp_hangup[81];	/* ISP hangup command	    */
	char		isp_ping1[41];		/* ISP ping host 1	    */
	char		isp_ping2[41];		/* ISP ping host 2	    */

	char		zmh_start[6];		/* Zone Mail Hour start	    */
	char		zmh_end[6];		/* Zone Mail Hour end	    */

	char		cmd_mailout[81];	/* mailout command	    */
	char		cmd_mailin[81];		/* mailin command	    */
	char		cmd_newnews[81];	/* newnews command	    */
	char		cmd_mbindex1[81];	/* mbindex command 1	    */
	char		cmd_mbindex2[81];	/* mbindex command 2	    */
	char		cmd_mbindex3[81];	/* mbindex command 3	    */
	char		cmd_msglink[81];	/* msglink command	    */
	char		cmd_reqindex[81];	/* reqindex command	    */

	int		xmax_pots;
	int		xmax_isdn;
	int		max_tcp;		/* maximum TCP/IP calls	    */

	unsigned	xipblocks	: 1;
	unsigned	xdebug		: 1;	/* debugging on/off	    */
};



/*
 * Special mail services (service.data)
 */
struct	servicehdr {
	int		hdrsize;		/* Size of header	    */
	int		recsize;		/* Size of records	    */
	int32_t		lastupd;		/* Last updated at	    */
};

struct	servicerec {
	char		Service[16];		/* Service name		    */
	int		Action;			/* Service action	    */
	unsigned	Active		: 1;	/* Service is active	    */
	unsigned	Deleted		: 1;	/* Service is deleted	    */
};



/*
 * Domain translation (domain.data)
 */
struct	domhdr {
	int		hdrsize;		/* Size of header	   */
	int		recsize;		/* Size of records	   */
	int32_t		lastupd;		/* Last updated at	   */
};

struct	domrec {
        char            ftndom[61];             /* Fidonet domain          */
        char            intdom[61];             /* Internet domain         */
	unsigned	Active		: 1;	/* Domain is active	   */
	unsigned	Deleted		: 1;	/* Domain is deleted	   */
};



/*
 * System Control Structures (sysinfo.data)
 */
struct	sysrec {
	unsigned int	SystemCalls;		/* Total # of system calls */
	unsigned int	Pots;			/* POTS calls		   */
	unsigned int	ISDN;			/* ISDN calls		   */
	unsigned int	Network;		/* Network (internet) calls*/
	unsigned int	Local;			/* Local calls		   */
	unsigned int	xADSL;			/*			   */
	int32_t		StartDate;		/* Start Date of BBS	   */
	char		LastCaller[37];		/* Last Caller to BBS	   */
	int32_t		LastTime;		/* Time of last caller	   */
};



/*
 * Protocol Control Structure (protocol.data)
 */
struct	prothdr {
	int		hdrsize;		/* Size of header	    */
	int		recsize;		/* Size of records	    */
};

struct	prot {
	char		ProtKey[2];		/* Protocol Key             */
	char		ProtName[21];		/* Protocol Name            */
	char		ProtUp[51];		/* Upload Path & Binary     */
	char		ProtDn[51];		/* Download Path & Bianry   */
	unsigned	Available	: 1;	/* Available/Not Available  */
	unsigned	xBatch		: 1;
	unsigned	xBidir		: 1;
	unsigned	Deleted		: 1;	/* Protocol is deleted	    */
	unsigned	Internal	: 1;	/* Internal protocol	    */
	char		Advice[31];		/* Small advice to user	    */
	int		Efficiency;		/* Protocol efficiency in % */
	securityrec	Level;			/* Sec. level to select	    */
};



/*
 * Oneliners Control Structure (oneline.data)
 */
struct	onelinehdr {
	int		hdrsize;		/* Size of header	    */
	int		recsize;		/* Size of record	    */
};

struct	oneline	{
	char		Oneline[81];		/* Oneliner text            */
	char		UserName[36];		/* User who wrote oneliner  */
	char		DateOfEntry[12];	/* Date of oneliner entry   */
	unsigned	Available	: 1;	/* Available Status         */
};



/*
 * File Areas Control Structure (fareas.data)
 */
struct	fileareashdr {
	int		hdrsize;		/* Size of header	    */
	int		recsize;		/* Size of records	    */
};

struct	fileareas {
	char		Name[45];		/* Filearea Name            */
	char		Path[81];		/* Filearea Path            */
	securityrec	DLSec;			/* Download Security        */
	securityrec	UPSec;			/* Upload Security          */
	securityrec	LTSec;			/* List Security            */
	int		Age;			/* Age to access area	    */
	unsigned	New		: 1;	/* New Files Check          */
	unsigned	Dupes		: 1;	/* Check for Duplicates     */
	unsigned	Free		: 1;	/* All files are Free       */
	unsigned	DirectDL	: 1;	/* Direct Download          */
	unsigned	PwdUP		: 1;	/* Password Uploads         */
	unsigned	FileFind	: 1;	/* FileFind Scan	    */
	unsigned	AddAlpha	: 1;	/* Add New files sorted	    */
	unsigned	Available	: 1;	/* Area is available	    */
	unsigned	xCDrom		: 1;
	unsigned	FileReq		: 1;	/* Allow File Requests	    */
	char		BbsGroup[13];		/* BBS Group 		    */
	char		Password[21];		/* Area Password            */
	unsigned	DLdays;			/* Move not DL for days     */
	unsigned	FDdays;			/* Move if FD older than    */
	unsigned	MoveArea;		/* Move to Area             */
	int		xCost;
	char		xFilesBbs[65];
	char		NewGroup[13];		/* Newfiles scan group	    */
	char		Archiver[6];		/* Archiver for area	    */
	unsigned	Upload;			/* Upload area		    */
};



/*
 * Index file for fast search of file requests (request.index)
 */
struct	FILEIndex {
	char		Name[13];		/* Short DOS name	   */
	char		LName[81];		/* Long filename	   */
	int		AreaNum;		/* File area number	   */
	int		Record;			/* Record in database	   */
};



/*
 * Files database (file#.data)
 */
struct FILE_recordhdr {
	int		hdrsize;		/* Size of header	    */
	int		recsize;		/* Record size		    */
};


struct  FILE_record {
	char            Name[13];               /* DOS style filename       */
	char            LName[81];              /* Long filename            */
	char            TicArea[21];            /* Tic area file came in    */
	unsigned int	Size;                   /* File Size                */
	unsigned int	Crc32;                  /* File CRC-32              */
	char            Uploader[36];           /* Uploader name            */
	int32_t		UploadDate;             /* Date/Time uploaded       */
	int32_t         FileDate;               /* Real file date           */
	int32_t         LastDL;                 /* Last Download date       */
	unsigned int	TimesDL;                /* Times file was dl'ed     */
	char            Password[16];           /* File password            */
	char            Desc[25][49];           /* file description         */
	char		Magic[21];		/* Magic request name	    */
	unsigned        Deleted      : 1;       /* Deleted                  */
	unsigned        NoKill       : 1;       /* Cannot be deleted        */
	unsigned        Announced    : 1;       /* File is announced        */
	unsigned        Double       : 1;       /* Double record            */
};



/*
 * Old File Record Control Structure (fdb#.data)
 */
struct	OldFILERecord {
	char		Name[13];		/* DOS style filename	    */
	char		LName[81];		/* Long filename	    */
	char		xTicArea[9];		/* Tic area file came in    */
	unsigned int	TicAreaCRC;		/* CRC of TIC area name	    */
	unsigned int	Size;			/* File Size                */
	unsigned int	Crc32;			/* File CRC-32		    */
	char		Uploader[36];		/* Uploader name            */
	int32_t		UploadDate;		/* Date/Time uploaded	    */
	int32_t		FileDate;		/* Real file date	    */
	int32_t		LastDL;			/* Last Download date	    */
	unsigned int	TimesDL;		/* Times file was dl'ed     */
	unsigned int	TimesFTP;		/* Times file was FTP'ed    */
	unsigned int	TimesReq;		/* Times file was frequed   */
	char		Password[16];		/* File password            */
	char		Desc[25][49];		/* file description         */
	int		Cost;			/* File cost		    */
	unsigned	Free         : 1;	/* Free File		    */
	unsigned	Deleted      : 1;	/* Deleted		    */
	unsigned	Missing      : 1;	/* Missing		    */
	unsigned	NoKill	     : 1;	/* Cannot be deleted        */
	unsigned	Announced    : 1;	/* File is announced	    */
	unsigned	Double	     : 1;	/* Double record	    */
};



/*
 * BBS List Control Structure (bbslist.data)
 */
struct	bbslisthdr {
	int		hdrsize;		/* Size of header	    */
	int		recsize;		/* Size of records	    */
};

struct	bbslist {
	char		UserName[36];		/* User Name                */
	char		DateOfEntry[12];	/* Entry date               */
	char		Verified[12];		/* Last Verify date	    */
	unsigned	Available	: 1;	/* Available Status	    */
	char		BBSName[41];		/* BBS Name                 */
	int		Lines;			/* Nr of phone lines	    */
	char		Phone[5][21];		/* BBS phone number         */
	char		Speeds[5][41];		/* Speeds for each line	    */
	fidoaddr	FidoAka[5];		/* Fidonet Aka's	    */
	char		Software[20];		/* BBS Software             */
	char		Sysop[36];		/* Name of Sysop            */
	int		Storage;		/* Storage amount in megs   */
	char		Desc[2][81];	 	/* Description              */
	char		IPaddress[51];		/* IP or domain name	    */
	char		Open[21];		/* Online time		    */
};



/* 
 * Last Callers Control Structure (lastcall.data)
 */
struct	lastcallershdr {
	int		hdrsize;		/* Size of header	    */
	int		recsize;		/* Size of records	    */
};

struct  lastcallers {  
	char		UserName[36];		/* User Name                */
	char		Handle[36];		/* User Handle              */
	char		Name[9];		/* Unix Name		    */
	char		TimeOn[6];		/* Time user called bbs     */
	int		CallTime;		/* Time this call	    */
	char		Device[10];		/* Device user used         */
	int		Calls;			/* Total calls to bbs       */
	unsigned int	SecLevel;		/* Users security level	    */
	char		Speed[21];		/* Caller speed		    */
	unsigned	Hidden     : 1;		/* Hidden or Not at time    */
	unsigned	Download   : 1;		/* If downloaded	    */
	unsigned	Upload	   : 1;		/* If uploaded		    */
	unsigned	Read	   : 1;		/* If read messages	    */
	unsigned	Wrote	   : 1;		/* If wrote a message	    */
	unsigned	Chat	   : 1;		/* If did chat		    */
	unsigned	Olr	   : 1;		/* If used Offline Reader   */
	unsigned	Door	   : 1;		/* If used a Door	    */
	char		Location[28];		/* User Location            */
};



/* 
 * System Control Structure (config.data)
 */
struct	sysconfig {
						/* Registration Info	    */
	char		sysop_name[36];		/* Sysop Name               */
	char		bbs_name[36];		/* BBS Name                 */
	char		sysop[9];		/* Unix Sysop name	    */
	char		location[36];		/* System location	    */
	char		bbsid[9];		/* QWK/Bluewave BBS ID	    */
	char		xbbsid2[3];		/* Omen filename	    */
	char		sysdomain[36];		/* System Domain name	    */
	char		comment[56];		/* Do what you like here    */
	char		origin[51];		/* Default origin line	    */

						/* FileNames		    */
	char		error_log[15];		/* Name of Error Log	    */
	char		default_menu[15];	/* Default Menu		    */
	char		xcurrent_language[15];	/* Default Language	    */
	char		chat_log[15];		/* Chat Logfile		    */
	char		welcome_logo[15];	/* Welcome Logofile	    */

						/* Paths		    */
	char		rnewspath[65];		/* Path to rnews	    */
	char		xbbs_menus[65];
	char		xbbs_txtfiles[65];
	char		nntpnode[65];		/* NNTP server		    */
	char		msgs_path[65];		/* Path to *.msg area	    */
	char		alists_path[65];	/* Area lists storage	    */
	char		req_magic[65];		/* Request magic directory  */
	char		bbs_usersdir[65];	/* Users Home Dir Base	    */
	char		nodelists[65];		/* Nodelists		    */
	char		inbound[65];		/* Inbound directory	    */
	char		pinbound[65];		/* Protected inbound	    */
	char		outbound[65];		/* Outbound		    */
	char		externaleditor[65];	/* External mail editor	    */
	char		dospath[65];		/* DOS path		    */
	char		uxpath[65];		/* Unix path		    */

						/* Allfiles/Newfiles	    */
	char		ftp_base[65];		/* FTP root		    */
	int		newdays;		/* New files since	    */
	securityrec	security;		/* Max level list	    */

	unsigned	addr4d		  : 1;	/* Use 4d addressing	    */
	unsigned	leavecase	  : 1;	/* Leave outbound case	    */

						/* BBS Globals		    */
	int		max_logins;		/* Max simult. logins	    */
	unsigned	NewAreas	  : 1;	/* Notify if new msg areas  */
	unsigned	xelite_mode       : 1;
	unsigned	slow_util	  : 1;	/* Run utils slowly	    */
	unsigned	exclude_sysop     : 1;	/* Exclude Sysop from lists */
	unsigned	UsePopDomain	  : 1;	/* Add domain pop3 login    */
	unsigned	xChkMail          : 1;
	unsigned	iConnectString	  : 1;  /* Display Connect String   */
	unsigned	iAskFileProtocols : 1;	/* Ask user FileProtocols   */
                                                /* before every d/l or u/l  */
	unsigned	sysop_access;		/* Sysop Access Security    */
	int		password_length;        /* Minimum Password Length  */
	int		bbs_loglevel;		/* Logging level for BBS    */
	int		iPasswd_Char;		/* Password Character       */
  	int		iQuota;			/* User homedir quota in MB */
	int		idleout;                /* Idleout Value            */
	int		CityLen;		/* Minimum city length	    */
	short		OLR_NewFileLimit;	/* Limit Newfilesscan days  */
	unsigned	iCRLoginCount;          /* Count login Enters       */

						/* New Users		    */
	securityrec	newuser_access;		/* New Users Access level   */
	int		OLR_MaxMsgs;		/* OLR Max nr Msgs download */
	unsigned	iCapUserName    : 1;	/* Capitalize Username      */
	unsigned	xiAnsi	        : 1;
	unsigned	iSex            : 1;	/* Ask Sex                  */
	unsigned	iDataPhone      : 1;	/* Ask Data Phone           */
	unsigned	iVoicePhone     : 1;	/* Ask Voice Phone          */
	unsigned	iHandle         : 1;	/* Ask Alias/Handle         */
	unsigned	iDOB            : 1;	/* Ask Date of Birth        */
	unsigned	iTelephoneScan  : 1;	/* Telephone Scan           */
	unsigned	iLocation       : 1;	/* Ask Location             */
	unsigned	iCapLocation    : 1;	/* Capitalize Location      */
	unsigned	iHotkeys        : 1;	/* Ask Hot-Keys             */
	unsigned	GiveEmail	: 1;	/* Give user email	    */
	unsigned	AskAddress      : 1;	/* Ask Home Address	    */
	unsigned	iOneName        : 1;	/* Allow one user name      */
	unsigned	xAskScreenlen	: 1;
	unsigned	iCrashLevel;		/* User level for crash mail*/
	unsigned	iAttachLevel;		/* User level for fileattach*/

						/* Colors		    */
	int		TextColourF;            /* Text Colour Foreground   */
	int		TextColourB;            /* Text Colour Background   */
	int		UnderlineColourF;       /* Underline Text Colour    */
	int		UnderlineColourB;       /* Underline Colour         */
	int		InputColourF;           /* Input Text Colour        */
	int		InputColourB;           /* Input Text Colour        */
	int		CRColourF;              /* CR Text Colour           */
	int		CRColourB;              /* CR Text Colour           */
	int		MoreF;                  /* More Prompt Text Colour  */
	int		MoreB;                  /* More Prompt Text Colour  */
	int		HiliteF;                /* Hilite Text Colour       */
	int		HiliteB;                /* Hilite Text Colour 	    */
	int		FilenameF;              /* Filename Colour          */
	int		FilenameB;              /* Filename Colour          */
	int		FilesizeF;              /* Filesize Colour          */
	int		FilesizeB;              /* Filesize Colour          */
	int		FiledateF;              /* Filedate Colour          */
	int		FiledateB;              /* Filedate Colour          */
	int		FiledescF;              /* Filedesc Colour          */
	int		FiledescB;              /* Filedesc Colour          */
	int		MsgInputColourF;        /* MsgInput Filename Colour */
	int		MsgInputColourB;        /* MsgInput Filename Colour */

	char		xNuScreen[50];          /* Obsolete Next User Door  */
	char		xNuQuote[81];

	int		AskNewmail;		/* Ask newmail check	    */
	int		AskNewfiles;		/* Ask newfiles check	    */
	int		xAskDummy;

	int		xSafeMaxTrys;
	int		xSafeMaxNumber;
	unsigned	xSafeNumGen  : 1;
	char		xSafePrize[81];
	char		xSafeWelcome[81];
	char		xSafeOpened[81];

						/* Sysop Paging		    */
	int		iPageLength;		/* Page Length in Seconds   */
	int		iMaxPageTimes;		/* Max Pages per call       */
	unsigned	iAskReason     : 1;	/* Ask Reason               */
	int		iSysopArea;		/* Msg Area if Sysop not in */
	unsigned	xxExternalChat  : 1;
	char		xExternalChat[50];
	unsigned	iAutoLog       : 1;	/* Log Chats                */
	char		xChatDevice[20];
	unsigned	iChatPromptChk;		/* Check for chat at prompt */
	unsigned	iStopChatTime;		/* Stop time during chat    */
	char		xStartTime[7][6];
	char		xStopTime[7][6];
	char		xCallScript[51];

						/* Mail Options		    */
	char		deflang[11];		/* Default language	    */

	int		xMaxTimeBalance;	/* Obsolete Time Bank Door  */
	int		xMaxTimeWithdraw;
	int		xMaxTimeDeposit;
	int		xMaxByteBalance;
	int		xMaxByteWithdraw;
	int		xMaxByteDeposit;
	unsigned	xNewBytes	: 1;
	char		xTimeRatio[7];
	char		xByteRatio[7];

	int		new_groups;		/* Maximum newfiles groups  */
	int		new_split;		/* Split reports at KB.	    */
	int		new_force;		/* Force split at KB.	    */
	char		startname[9];		/* BBS startup name	    */
	char		extra4[239];

						/* TIC Processing	    */
	unsigned	ct_KeepDate	: 1;	/* Keep Filedate	    */
	unsigned	ct_KeepMgr	: 1;	/* Keep Mgr netmails	    */
	unsigned	xct_ResFuture	: 1;	/* Reset Future filedates   */
	unsigned	xct_LocalRep	: 1;	/* Respond to local requests*/
	unsigned	xct_ReplExt	: 1;	/* Replace Extension	    */
	unsigned	ct_PlusAll	: 1;	/* Filemgr: allow +%*	    */
	unsigned	ct_Notify	: 1;	/* Filemgr: Notify on/off   */
	unsigned	ct_Passwd	: 1;	/* Filemgr: Passwd change   */
	unsigned	ct_Message	: 1;	/* Filemgr: Msg file on/off */
	unsigned	ct_TIC		: 1;	/* Filemgr: TIC files on/off*/
	unsigned	ct_Pause	: 1;	/* Filemgr: Allow Pause	    */
	char		logfile[15];		/* System Logfile	    */
	int		OLR_MaxReq;		/* Max nr of Freq's	    */
	int		tic_days;		/* Keep on hold for n days  */
	char		hatchpasswd[21];	/* Internal Hatch Passwd    */
	unsigned int	xdrspace;
	char		xmgrname[5][21];	/* Areamgr names	    */
	int		tic_systems;		/* Systems in database	    */
	int		tic_groups;		/* Groups in database	    */
	int		tic_dupes;		/* TIC dupes dabase size    */
	char		badtic[65];		/* Bad TIC's path	    */
	char		ticout[65];		/* TIC queue		    */

						/* Mail Tosser		    */
	char		pktdate[65];		/* pktdate by Tobias Ernst  */
	int		maxpktsize;		/* Maximum packet size	    */
	int		maxarcsize;		/* Maximum archive size	    */
	int		toss_old;		/* Reject older then days   */
	char		xtoss_log[11];
	int		util_loglevel;		/* Logging level for utils  */
	char		badboard[65];		/* Bad Mail board	    */
	char		dupboard[65];		/* Dupe Mail board	    */
	char		popnode[65];		/* Node with pop3 boxes     */
	char		smtpnode[65];		/* SMTP node		    */
	int		toss_days;		/* Keep on hold		    */
	int		toss_dupes;		/* Dupes in database	    */
	int		defmsgs;		/* Default purge messages   */
	int		defdays;		/* Default purge days	    */
	int		freespace;		/* Free diskspace in MBytes */
	int		toss_systems;		/* Systems in database	    */
	int		toss_groups;		/* Groups in database       */
	char		xareamgr[5][21];	/* Areamgr names	    */

						/* Flags		    */
	char		fname[32][17];		/* Name of the access flags */
	fidoaddr	aka[40];		/* Fidonet AKA's	    */
	unsigned short	akavalid[40];		/* Fidonet AKA valid/not    */

	int		cico_loglevel;		/* Mailer loglevel	    */
	int		timeoutreset;		/* Reset timeout	    */
	int		timeoutconnect;		/* Connect timeout	    */
	int		dialdelay;		/* Delay between calls	    */
	unsigned	NoFreqs		: 1;	/* Don't allow requests	    */
	unsigned	NoCall		: 1;	/* Don't call		    */
	unsigned	NoMD5		: 1;	/* Don't do MD5		    */
	unsigned	xNoCRC32	: 1;
	unsigned	NoEMSI		: 1;	/* Don't do EMSI	    */
	unsigned	NoWazoo		: 1;	/* Don't do Yooho/2U2	    */
	unsigned	NoZmodem	: 1;	/* Don't do Zmodem	    */
	unsigned	NoZedzap	: 1;	/* Don't do Zedzap	    */

	unsigned	xNoJanus	: 1;
	unsigned	NoHydra		: 1;	/* Don't do Hydra	    */
	unsigned	ZeroLocks	: 1;	/* Allow 0 bytes locking    */
	unsigned	xNoITN		: 1;
	unsigned	xNoIFC		: 1;

	char		IP_Phone[21];		/* TCP/IP phonenumber	    */
	unsigned int	IP_Speed;		/* TCP/IP linespeed	    */
	char		IP_Flags[31];		/* TCP/IP EMSI flags	    */
	int		Req_Files;		/* Maximum files request    */
	int		Req_MBytes;		/* Maximum MBytes request   */
	char		extra5[96];
	dual		phonetrans[40];		/* Phone translation table  */

                                                /* Obsolete FTP Daemon      */
	int             xftp_limit;
	int             xftp_loginfails;
	unsigned        xftp_compress   : 1;
	unsigned        xftp_tar        : 1;
	unsigned        xftp_upl_mkdir  : 1;
	unsigned        xftp_log_cmds	: 1;
	unsigned        xftp_anonymousok: 1;
	unsigned        xftp_mbseok	: 1;
	unsigned        xftp_x7         : 1;
	unsigned        xftp_x8         : 1;
	unsigned        xftp_x9         : 1;
	char            xftp_readme_login[21];
	char		xftp_readme_cwd[21];
	char		xftp_msg_login[21];
	char		xftp_msg_cwd[21];
	char		xftp_msg_shutmsg[41];

						/* Download counts	    */
	char		www_logfile[81];	/* Apache logfile	    */
	char		ftp_logfile[81];	/* FTP server logfile	    */

	char		xftp_email[41];
	char		xftp_pth_filter[41];
	char		xftp_pth_message[81];

						/* HTML creation	    */
	char		www_root[81];		/* HTML doc root	    */
	char		www_link2ftp[21];	/* Link name to ftp_base    */
	char		www_url[41];		/* Webserver URL	    */
	char		www_charset[21];	/* Default characher set    */
	char		xwww_tbgcolor[21];
	char		xwww_hbgcolor[21];
	char		www_author[41];		/* Author name in pages	    */
	char		www_convert[81];	/* Graphic Convert command  */
	char		xwww_icon_home[21];
	char		xwww_name_home[21];
	char		xwww_icon_back[21];
	char		xwww_name_back[21];
	char		xwww_icon_prev[21];
	char		xwww_name_prev[21];
	char		xwww_icon_next[21];
	char		xwww_name_next[21];
	int		www_files_page;		/* Files per webpage	    */

	fidoaddr	EmailFidoAka;		/* Email aka in fidomode    */
	fidoaddr	UUCPgate;		/* UUCP gateway in fidomode */
	int		EmailMode;		/* Email mode to use	    */
	unsigned	modereader	: 1;	/* NNTP Mode Reader	    */
	unsigned	allowcontrol	: 1;	/* Allow control messages   */
	unsigned	dontregate	: 1;	/* Don't regate gated msgs  */
	char		xnntpuser[16];		/* NNTP username	    */
	char		xnntppass[16];		/* NNTP password	    */
	int		nntpdupes;		/* NNTP dupes database size */
	int		newsfeed;		/* Newsfeed mode	    */
	int		maxarticles;		/* Default max articles	    */
	char		xbbs_macros[65];
	char		out_queue[65];		/* Outbound queue path	    */

	char		mgrlog[15];		/* Area/File-mgr logfile    */
	char            aname[32][17];          /* Name of areas flags	    */

	unsigned	ca_PlusAll	: 1;	/* Areamgr: allow +%*       */
	unsigned	ca_Notify	: 1;	/* Areamgr: Notify on/off   */
	unsigned	ca_Passwd	: 1;	/* Areamgr: Passwd change   */
	unsigned	ca_Pause	: 1;	/* Areamgr: Allow Pause     */
	unsigned	ca_Check	: 1;	/* Flag for upgrade check   */

	char		rulesdir[65];		/* Area rules directory	    */
	char		debuglog[15];		/* Debug logfile	    */
	char		tmailshort[65];		/* T-Mail short filebox base*/
	char		tmaillong[65];		/* T-Mail long filebox base */

	int		priority;		/* Child process priority   */
	unsigned	do_sync		: 1;	/* Sync() during execute    */
	unsigned	is_upgraded	: 1;	/* For internal upgrade use */
	unsigned	nntpforceauth	: 1;	/* Force NNTP authenticate  */

	char		myfqdn[64];		/* My real FQDN		    */
	int		www_mailerlines;	/* Limit mailhistory lines  */

	char		nntpuser[32];		/* NNTP username            */
	char		nntppass[32];		/* NNTP password            */
	unsigned int	nntpport;		/* NNTP port if not 119	    */
};



/*
 * Limits Control Structure (limits.data)
 */
struct	limitshdr {
	int		hdrsize;		/* Size of header	   */
	int		recsize;		/* Size of records	   */
};

struct	limits {
	unsigned int	Security;		/* Security Level          */
	int		Time;			/* Amount of time per call */
	unsigned int	DownK;			/* Download KB per call    */
	unsigned int	DownF;			/* Download files per call */
	char		Description[41];	/* Description for level   */
	unsigned	Available	: 1;	/* Is this limit available */
	unsigned	Deleted		: 1;	/* Is this limit deleted   */
};                             



/*
 * Menu File Control Structure (*.mnu)
 */
struct	menufile {
	char		MenuKey[2];		/* Menu Key                 */
	int		MenuType;		/* Menu Type                */
	char		OptionalData[81];	/* Optional Date            */
	char		Display[81];		/* Menu display line	    */
	securityrec	MenuSecurity;		/* Menu Security Level      */
	int		Age;			/* Minimum Age to use menu  */
	unsigned int	xMaxSecurity;
	char		DoorName[15];		/* Door name		    */
	char		TypeDesc[30];		/* Menu Type Description    */
#ifdef WORDS_BIGENDIAN
						/* All bits swapped         */
	unsigned        HideDoor        : 1;    /* Hide door from lists     */
	unsigned        SingleUser      : 1;    /* Single user door         */
	unsigned        NoPrompt        : 1;    /* No prompt after door     */
	unsigned        NoSuid          : 1;    /* Execute noduid           */
	unsigned        Comport         : 1;    /* Vmodem comport mode      */
	unsigned        Y2Kdoorsys      : 1;    /* Write Y2K style door.sys */
	unsigned        NoDoorsys       : 1;    /* Suppress door.sys        */
	unsigned        AutoExec        : 1;    /* Auto Exec Menu Type      */
#else
	unsigned	AutoExec	: 1;	/* Auto Exec Menu Type      */
	unsigned	NoDoorsys	: 1;	/* Suppress door.sys	    */
	unsigned	Y2Kdoorsys	: 1;	/* Write Y2K style door.sys */
	unsigned	Comport		: 1;	/* Vmodem comport mode	    */
	unsigned	NoSuid		: 1;	/* Execute door nosuid	    */
	unsigned	NoPrompt	: 1;	/* No prompt after door	    */
	unsigned	SingleUser	: 1;	/* Single user door	    */
	unsigned	HideDoor	: 1;	/* Hide door from lists	    */
#endif
	int		xUnused;
	int		HiForeGnd;		/* High ForeGround color    */
	int		HiBackGnd;		/* High ForeGround color    */
	int		ForeGnd;		/* Normal ForeGround color  */
	int		BackGnd;		/* Normal BackGround color  */
};



/*
 * News dupes database. Stores newsgroupname and CRC32 of article msgid.
 */
struct	newsdupes {
	char		NewsGroup[65];		/* Name of the group	    */
	unsigned int	Crc;			/* CRC32 of msgid	    */
};



/* 
 * Message Areas Structure (mareas.data)
 * This is also used for echomail, netmail and news
 */
struct	msgareashdr {
	int		hdrsize;		/* Size of header	    */
	int		recsize;		/* Size of records	    */
	int		syssize;		/* Size for systems	    */
	int32_t		lastupd;		/* Last date stats updated  */
};

struct msgareas {
	char		Name[41];		/* Message area Name        */
	char		Tag[51];		/* Area tag		    */
	char		Base[65];		/* JAM base		    */
	char		QWKname[21];		/* QWK area name	    */
	int		Type;			/* Msg Area Types           */
						/* Local, Net, Echo, News,  */
						/* Listserv		    */
	int		MsgKinds;		/* Type of Messages         */
						/* Public,Private,ReadOnly  */
	int		DaysOld;		/* Days to keep messages    */
	int		MaxMsgs;		/* Maximum number of msgs   */
	int		UsrDelete;		/* Allow users to delete    */
	securityrec	RDSec;        		/* Read Security            */
	securityrec	WRSec;			/* Write Security           */
	securityrec	SYSec;			/* Sysop Security           */
	int		Age;			/* Age to access this area  */
	char		Password[20];		/* Area Password            */
	char		Group[13];		/* Group Area               */
	fidoaddr	Aka;			/* Fidonet address	    */
	char		Origin[65];		/* Origin Line              */
	unsigned	Aliases		: 1;	/* Allow aliases	    */
	unsigned	NetReply;		/* Area for Netmail reply   */
	unsigned	Active		: 1;	/* Area is active	    */
	unsigned	OLR_Forced	: 1;	/* OLR Area always on	    */
	unsigned	xFileAtt	: 1;	/* Allow file attach	    */
	unsigned	xModerated	: 1;	/* Moderated newsgroup	    */
	unsigned	Quotes		: 1;	/* Add random quotes	    */
	unsigned	Mandatory	: 1;	/* Mandatory for nodes	    */
	unsigned	UnSecure	: 1;	/* UnSecure tossing	    */
	unsigned	xUseFidoDomain	: 1;
	unsigned	OLR_Default	: 1;	/* OLR Deafault turned on   */
	unsigned	xPrivate	: 1;	/* Pvt bits allowed	    */
	unsigned	xCheckSB	: 1;
	unsigned	xPassThru	: 1;
	unsigned	xNotiFied	: 1;
	unsigned	xUplDisc	: 1;
	statcnt		Received;		/* Received messages	    */
	statcnt		Posted;			/* Posted messages	    */
	int32_t		LastRcvd;		/* Last time msg received   */
	int32_t		LastPosted;		/* Last time msg posted	    */
	char		Newsgroup[81];		/* Newsgroup/Mailinglist    */
	char		xDistribution[17];	/* Ng distribution	    */
	char		xModerator[65];
	int		xRfccode;
	int		Charset;		/* FTN characterset	    */
	int		MaxArticles;		/* Max. newsarticles to get */
	securityrec	LinkSec;		/* Link security flags	    */
	int32_t		Created;		/* Area creation date	    */
};



/*
 * Structure for Language file (language.data)
 */
struct	languagehdr {
	int		hdrsize;		/* Size of header	   */
	int		recsize;		/* Size of records	   */
};

struct language {
	char		Name[30];		/* Name of Language        */
	char		LangKey[2];		/* Language Key            */
	char		xMenuPath[81];		/* Path of menu directory  */
	char		xTextPath[81];		/* Path of text files      */
	unsigned	Available	: 1;	/* Availability of Language*/
	unsigned	Deleted		: 1;	/* Language is deleted	   */
	char		xFilename[81];		/* Path of language file   */
	securityrec	Security;		/* Security level	   */
	char		xMacroPath[81];		/* Path to the macro files */
	char		lc[10];			/* ISO language code	   */
};



/* 
 * Structure for Language Data File (english.lang)
 */
struct langdata {
	char		sString[85];		/* Language text	   */
	char		sKey[30];		/* Keystroke characters	   */
};
 


/*
 * Fidonet Networks (fidonet.data)
 */
struct _fidonethdr {
	int		hdrsize;		/* Size of header record   */
	int		recsize;		/* Size of records	   */
};

typedef	struct	_seclist {
	char		nodelist[9];		/* Secondary nodelist name */
	unsigned short	zone;			/* Adress for this list	   */
	unsigned short	net;
	unsigned short	node;
} seclistrec;

struct _fidonet {
	char		domain[13];		/* Network domain name	   */
	char		nodelist[9];		/* Nodelist name	   */
	seclistrec	seclist[6];		/* 6 secondary nodelists   */
	unsigned short	zone[6];		/* Maximum 6 zones	   */
	char		comment[41];		/* Record comment	   */
	unsigned	available : 1;		/* Network available	   */
	unsigned	deleted	  : 1;		/* Network is deleted	   */
};



/*
 * Archiver programs (archiver.data)
 */
struct _archiverhdr {
	int		hdrsize;		/* Size of header record   */
	int		recsize;		/* Size of records	   */
};

struct _archiver {
	char		comment[41];		/* Archiver comment	   */
	char		name[6];		/* Archiver name	   */
	unsigned	available	: 1;	/* Archiver available	   */
	unsigned	deleted  	: 1;	/* Archiver is deleted     */
	char		farc[65];		/* Archiver for files	   */
	char		marc[65];		/* Archiver for mail	   */
	char		barc[65];		/* Archiver for banners	   */
	char		tarc[65];		/* Archiver test	   */
	char		funarc[65];		/* Unarc files		   */
	char		munarc[65];		/* Unarc mail		   */
	char		iunarc[65];		/* Unarc FILE_ID.DIZ	   */
	char		varc[65];		/* View archive		   */
};



/*
 * Virus scanners (virscan.data)
 */
struct	_virscanhdr {
	int		hdrsize;		/* Size of header record   */
	int		recsize;		/* Size of records	   */
};

struct	_virscan {
	char		comment[41];		/* Comment		   */
	char		scanner[65];		/* Scanner command	   */
	unsigned	available	: 1;	/* Scanner available	   */
	unsigned	deleted  	: 1;	/* Scanner is deleted	   */
	char		options[65];		/* Scanner options	   */
	int		error;			/* Error level for OK	   */
	int		scantype;		/* Virus scanner type	   */
	char		host[65];		/* Stream scanner host	   */
	unsigned int	port;			/* Stream scanner port	   */
};



/*
 * TTY information
 */
struct	_ttyinfohdr {
	int		hdrsize;		/* Size of header record   */
	int		recsize;		/* Size of records	   */
};

struct	_ttyinfo {
	char		comment[41];		/* Comment for tty	   */
	char		tty[7];			/* TTY device name	   */
	char		phone[26];		/* Phone or dns name	   */
	char		speed[21];		/* Max speed for this tty  */
	char		flags[31];		/* Fidonet capabilty flags */
	int		type;			/* Pots/ISDN/Netw/Local	   */
	unsigned	available	: 1;	/* Available flag	   */
	unsigned	xauthlog	: 1;
	unsigned	honor_zmh	: 1;	/* Honor ZMH on this line  */
	unsigned	deleted		: 1;	/* Is deleted		   */
	unsigned	callout		: 1;	/* Callout allowed	   */
	char		modem[31];		/* Modem type		   */
	char		name[36];		/* EMSI line name	   */
	int		portspeed;		/* Locked portspeed	   */
};



/*
 * Modem definitions.
 */
struct	_modemhdr {
	int		hdrsize;		/* Size of header record   */
	int		recsize;		/* Size of records	   */
};

struct	_modem {
	char		modem[31];		/* Modem type		   */
	char		init[3][61];		/* Init strings		   */
	char		ok[11];			/* OK string		   */
	char		hangup[41];		/* Hangup command	   */
	char		info[41];		/* After hangup get info   */
	char		dial[41];		/* Dial command		   */
	char		connect[20][31];	/* Connect strings	   */
	char		error[10][21];		/* Error strings	   */
	char		reset[61];		/* Reset string		   */
	int		costoffset;		/* Offset add to connect   */
	char		speed[16];		/* EMSI speed string	   */
	unsigned	available	: 1;	/* Is modem available	   */
	unsigned	deleted		: 1;	/* Is modem deleted	   */
	unsigned	stripdash	: 1;	/* Strip dashes from dial  */
};



/*
 * Structure for TIC areas (tic.data)
 */
struct	_tichdr {
	int		hdrsize;		/* Size of header 	   */
	int		recsize;		/* Size of records	   */
	int		syssize;		/* Size for systems	   */
	int		lastupd;		/* Last statistic update   */
};

struct	_tic {
	char		Name[21];		/* Area name		   */
	char		Comment[56];		/* Area comment		   */
	int		FileArea;		/* The BBS filearea	   */
	char		Message[15];		/* Message file		   */
	char		Group[13];		/* FDN group		   */
	int		KeepLatest;		/* Keep latest n files	   */
	int		xOld[6];
	int32_t		AreaStart;		/* Startdate		   */
	fidoaddr	Aka;			/* Fidonet address	   */
	char		Convert[6];		/* Archiver to convert	   */
	int32_t		LastAction;		/* Last Action in this area*/
	char		Banner[15];		/* Banner file		   */
	int		xUnitCost;
	int		xUnitSize;
	int		xAddPerc;
	unsigned	Replace		: 1;	/* Allow Replace	   */
	unsigned	DupCheck	: 1;	/* Dupe Check		   */
	unsigned	Secure		: 1;	/* Check for secure system */
	unsigned	Touch		: 1;	/* Touch filedate	   */
	unsigned	VirScan 	: 1;	/* Run Virus scanners	   */
	unsigned	Announce	: 1;	/* Announce files	   */
	unsigned	UpdMagic	: 1;	/* Update Magic database   */
	unsigned	FileId		: 1;	/* Check FILE_ID.DIZ	   */
	unsigned	ConvertAll	: 1;	/* Convert allways	   */
	unsigned	SendOrg		: 1;	/* Send Original to downl's*/
	unsigned	Mandat		: 1;	/* Mandatory area	   */
	unsigned	Notified	: 1;	/* Notified if disconn.	   */
	unsigned	UplDiscon	: 1;	/* Uplink disconnected	   */
	unsigned	Active		: 1;	/* If this area is active  */
	unsigned	Deleted		: 1;	/* If this area is deleted */
	unsigned	NewSR		: 1;	/* Connect new links SR	   */
	statcnt		Files;			/* Total processed files   */
	statcnt		KBytes;			/* Total processed KBytes  */
	securityrec	LinkSec;		/* Link security flags	   */
};



/*
 * Nodes, up- and downlinks. (nodes.data)
 */
struct	_nodeshdr {
	int		hdrsize;		/* Size of header	   */
	int		recsize;		/* Size of records	   */
	int		filegrp;		/* Size for file groups	   */
	int		mailgrp;		/* Size for mail groups	   */
	int		lastupd;		/* Last statistic update   */
};

struct	_nodes {
	char		Sysop[36];		/* Sysop name		    */
	fidoaddr	Aka[20];		/* Aka's for this system    */
	char		Fpasswd[16];		/* Files password	    */
	char		Epasswd[16];		/* Mail password	    */
	char		Apasswd[16];		/* Areamgr password	    */
	char		UplFmgrPgm[9];		/* Uplink FileMgr program   */
	char		UplFmgrPass[16];	/* Uplink FileMgr password  */
	char		UplAmgrPgm[9];		/* Uplink AreaMgr program   */
	char		UplAmgrPass[16];	/* Uplink AreaMgr password  */

	unsigned	Direct		: 1;	/* Netmail Direct	    */
	unsigned	Message		: 1;	/* Send Message w. files    */
	unsigned	Tic		: 1;	/* Send TIC files	    */
	unsigned	Notify		: 1;	/* Send Notify messages	    */
	unsigned	FileFwd		: 1;	/* Accept File Forward	    */
	unsigned	MailFwd		: 1;	/* Accept Mail Forward	    */
	unsigned	AdvTic		: 1;	/* Advanced Tic files	    */
	unsigned	UplFmgrBbbs	: 1;	/* Uplink FileMgr BBBS	    */

	unsigned	UplAmgrBbbs	: 1;	/* Uplink AreaMgr BBBS	    */
	unsigned	Crash		: 1;	/* Netmail crash	    */
	unsigned	Hold		: 1;	/* Netmail hold		    */
	unsigned	NoGZ		: 1;	/* Disable GZ/BZ2	    */
	unsigned	MailPwdCheck	: 1;	/* Mail password check	    */
	unsigned	Deleted		: 1;	/* Node is deleted	    */
	unsigned	NoEMSI		: 1;	/* No EMSI handshake	    */
	unsigned	NoWaZOO		: 1;	/* No YooHoo/2U2 handshake  */

	unsigned	NoFreqs		: 1;	/* Don't allow requests	    */
	unsigned	NoCall		: 1;	/* Don't call this node	    */
	unsigned	TIC_AdvSB	: 1;	/* Advanced tic SB lines    */
	unsigned	TIC_To		: 1;	/* Add To line to ticfile   */
	unsigned	NoZmodem	: 1;	/* Don't use Zmodem	    */
	unsigned	NoZedzap	: 1;	/* Don't use Zedzap	    */
	unsigned	NoPLZ		: 1;	/* Disable PLZ compress	    */
	unsigned	NoHydra		: 1;	/* Don't use Hydra	    */

	unsigned	DoNR		: 1;	/* NR mode for this node    */
	unsigned	PackNetmail	: 1;	/* Pack netmail		    */
	unsigned	ARCmailCompat	: 1;	/* ARCmail Compatibility    */
	unsigned	ARCmailAlpha	: 1;	/* Allow a..z ARCmail name  */
	unsigned	FNC		: 1;	/* Node needs 8.3 filenames */
	unsigned	WrongEscape	: 1;	/* Binkp wrong escape	    */
	unsigned	NoBinkp11	: 1;	/* No binkp/1.1 mode	    */
	unsigned	IgnHold		: 1;	/* Ignore Hold/Down status  */

	char		xExtra[94];
	int32_t		StartDate;		/* Node start date	    */
	int32_t		LastDate;		/* Last action date	    */
	int		xCredit;		/* Node's credit	    */
	int		xDebet;			/* Node's debet		    */
	int		xAddPerc;		/* Add Percentage	    */
	int		xWarnLevel;		/* Warning level	    */
	int		xStopLevel;		/* Stop level		    */
	fidoaddr	RouteVia;		/* Routing address	    */
	int		Language;		/* Language for netmail	    */
	statcnt		FilesSent;		/* Files sent to node	    */
	statcnt		FilesRcvd;		/* Files received from node */
	statcnt		F_KbSent;		/* File KB. sent	    */
	statcnt		F_KbRcvd;		/* File KB. received	    */
	statcnt		MailSent;		/* Messages sent to node    */
	statcnt		MailRcvd;		/* Messages received	    */
	char		dial[41];		/* Dial command override    */
	char		phone[2][21];		/* Phone numbers override   */

	char		Spasswd[16];		/* Session password	    */
	int		Session_out;		/* Outbound session type    */
	int		Session_in;		/* Inbound session type	    */

						/* Directory in/outbound    */
	char		Dir_out_path[65];	/* Outbound files	    */
	char		Dir_out_clock[65];	/* Outbound filelock check  */
	char		Dir_out_mlock[65];	/* Outbound filelock create */
	char		Dir_in_path[65];	/* Inbound files	    */
	char		Dir_in_clock[65];	/* Inbound filelock check   */
	char		Dir_in_mlock[65];	/* Inbound filelock create  */
	unsigned	Dir_out_chklck	: 1;	/* Outbound check lock	    */
	unsigned	Dir_out_waitclr	: 1;	/* Outbound wait for clear  */
	unsigned	Dir_out_mklck	: 1;	/* Outbound create lock	    */
	unsigned	Dir_in_chklck	: 1;	/* Inbound check lock	    */
	unsigned	Dir_in_waitclr	: 1;	/* Inbound wait for clear   */
	unsigned	Dir_in_mklck	: 1;	/* Inbound create lock	    */
	unsigned	Tic4d		: 1;	/* 4d addresses in ticfile  */

						/* FTP transfers	    */
	char		FTP_site[65];		/* Site name or IP address  */
	char		FTP_user[17];		/* Username		    */
	char		FTP_pass[17];		/* Password		    */
	char		FTP_starthour[6];	/* Start hour		    */
	char		FTP_endhour[6];		/* End hour		    */
	char		FTP_out_path[65];	/* Outbound files path	    */
	char		FTP_out_clock[65];	/* Outbound filelock check  */
	char		FTP_out_mlock[65];	/* Outbound filelock create */
	char		FTP_in_path[65];	/* Inbound files path	    */
	char		FTP_in_clock[65];	/* Inbound filelock check   */
	char		FTP_in_mlock[65];	/* Inbound filelock create  */
	unsigned	FTP_lock1byte	: 1;	/* Locksize 1 or 0 bytes    */
	unsigned	FTP_unique	: 1;	/* Unique storage	    */
	unsigned	FTP_uppercase	: 1;	/* Force uppercase	    */
	unsigned	FTP_lowercase	: 1;	/* Force lowercase	    */
	unsigned	FTP_passive	: 1;	/* Passive mode		    */
	unsigned	FTP_out_chklck	: 1;	/* Outbound check lockfile  */
	unsigned	FTP_out_waitclr	: 1;	/* Outbound wait for clear  */
	unsigned	FTP_out_mklck	: 1;	/* Outbound create lock	    */
	unsigned	FTP_in_chklck	: 1;	/* Inbound check lockfile   */
	unsigned	FTP_in_waitclr	: 1;	/* Inbound wait for clear   */
	unsigned	FTP_in_mklck	: 1;	/* Inbound create lock	    */

	char		OutBox[65];		/* Node's personal outbound */
	char		Nl_flags[65];		/* Override nodelist flags  */
	char		Nl_hostname[41];	/* Override hostname	    */

						/* Contact information	    */
	char		Ct_phone[17];		/* Node's private phone	    */
	char		Ct_fax[17];		/* Node's fax		    */
	char		Ct_cellphone[21];	/* Node's cellphone	    */
	char		Ct_email[31];		/* Node's email		    */
	char		Ct_remark[65];		/* Remark		    */

	securityrec	Security;		/* Security flags	    */
	char            Archiver[6];            /* Archiver to use          */
};



/*
 * Groups for file areas. (fgroups.data)
 */
struct	_fgrouphdr {
	int		hdrsize;		/* Size of header	   */
	int		recsize;		/* Size of records	   */
	int		lastupd;		/* Last statistics update  */
};

struct	_fgroup {
	char		Name[13];		/* Group Name		   */
	char		Comment[56];		/* Group Comment	   */
	unsigned	Active		: 1;	/* Group Active		   */
	unsigned	Deleted		: 1;	/* Is group deleted	   */
	unsigned	xDivideCost	: 1;	/* Divide cost over links  */
	fidoaddr	UseAka;			/* Aka to use		   */
	fidoaddr	UpLink;			/* Uplink address	   */
	int		xUnitCost;		/* Cost per unit	   */
	int		xUnitSize;		/* Size per unit	   */
	int		xAddProm;		/* Promillage to add	   */
	int		StartDate;		/* Start Date		   */
	int		LastDate;		/* Last active date	   */
	char		AreaFile[13];		/* Areas filename	   */
	statcnt		Files;			/* Files processed	   */
	statcnt		KBytes;			/* KBytes msgs or files	   */
						/* Auto add area options   */
	int		StartArea;		/* Lowest filearea nr.	   */
	char		Banner[15];		/* Banner to add	   */
	char		Convert[6];		/* Archiver to convert	   */
	unsigned	FileGate	: 1;	/* List is in filegate fmt */
	unsigned	AutoChange	: 1;	/* Auto add/del areas      */
	unsigned	UserChange	: 1;	/* User add/del areas      */
	unsigned	Replace		: 1;	/* Allow replace	   */
	unsigned	DupCheck	: 1;	/* Dupe Check		   */
	unsigned	Secure		: 1;	/* Check for secure system */
	unsigned	Touch		: 1;	/* Touch filedates	   */
	unsigned	VirScan		: 1;	/* Run Virus scanners	   */
	unsigned	Announce	: 1;	/* Announce files	   */
	unsigned	UpdMagic	: 1;	/* Update Magic database   */
	unsigned	FileId		: 1;	/* Check FILE_ID.DIZ	   */
	unsigned	ConvertAll	: 1;	/* Convert always	   */
	unsigned	SendOrg		: 1;	/* Send original file	   */
	unsigned	xRes6		: 1;
	unsigned	xRes7		: 1;
	unsigned	xRes8		: 1;
	char		BasePath[65];		/* File area base path     */
	securityrec	DLSec;			/* Download Security	   */
	securityrec	UPSec;			/* Upload Security	   */
	securityrec	LTSec;			/* List Security	   */
	char		BbsGroup[13];		/* BBS Group		   */
	char		AnnGroup[13];		/* BBS Announce Group	   */
	unsigned	Upload;			/* Upload area		   */
	securityrec	LinkSec;		/* Default link security   */
};



/*
 * Groups for message areas. (mgroups.data)
 */
struct	_mgrouphdr {
	int		hdrsize;		/* Size of header	   */
	int		recsize;		/* Size of records	   */
	int		lastupd;		/* Last statistics update  */
};

struct	_mgroup {
	char		Name[13];		/* Group Name		   */
	char		Comment[56];		/* Group Comment	   */
	unsigned	Active		: 1;	/* Group Active		   */
	unsigned	Deleted		: 1;	/* Group is deleted	   */
	fidoaddr	UseAka;			/* Aka to use		   */
	fidoaddr	UpLink;			/* Uplink address	   */
	int		xOld[6];
	int		StartDate;		/* Start Date		   */
	int		LastDate;		/* Last active date	   */
	char		AreaFile[13];		/* Areas filename	   */
	statcnt		MsgsRcvd;		/* Received messages	   */
	statcnt		MsgsSent;		/* Sent messages	   */
						/* Auto create options	   */
	char		BasePath[65];		/* Base path to JAM areas  */
	securityrec	RDSec;			/* Read security	   */
	securityrec	WRSec;			/* Write security	   */
	securityrec	SYSec;			/* Sysop secirity	   */
	unsigned	NetReply;		/* Netmail reply area	   */
	unsigned	UsrDelete	: 1;	/* Allow users to delete   */
	unsigned	Aliases		: 1;	/* Allow aliases	   */
	unsigned	Quotes		: 1;	/* Add random quotes	   */
	unsigned	AutoChange	: 1;	/* Auto add/del from list  */
	unsigned	UserChange	: 1;	/* User add/del from list  */
	unsigned	xRes6		: 1;
	unsigned	xRes7		: 1;
	unsigned	xRes8		: 1;
	unsigned	StartArea;		/* Start at area number    */
	securityrec	LinkSec;		/* Default link security   */
	int		Charset;		/* Default charaacter set  */
	int		GoldEDgroup;		/* GoldED group number	   */
};



/*
 *  Groups for newfiles announce. (ngroups.data)
 */
struct	_ngrouphdr {
	int		hdrsize;		/* Size of header	   */
	int		recsize;		/* Size of records	   */
};

struct	_ngroup {
	char		Name[13];		/* Group Name		   */
	char		Comment[56];		/* Group Comment	   */
	unsigned	Active		: 1;	/* Group Active		   */
	unsigned	Deleted		: 1;	/* Group is deleted	   */
};



/*
 * Hatch manager (hatch.data)
 */
struct	_hatchhdr {
	int		hdrsize;		/* Size of header	   */
	int		recsize;		/* Size of records	   */
	int		lastupd;		/* Last stats update	   */
};

struct	_hatch {
	char		Spec[79];		/* File spec to hatch	   */
	char		Name[21];		/* File Echo name	   */
	char		Replace[15];		/* File to replace	   */
	char		Magic[15];		/* Magic to update	   */
	char		Desc[256];		/* Description for file	   */
	unsigned	DupeCheck	: 1;	/* Check for dupes	   */
	unsigned	Active		: 1;	/* Record active	   */
	unsigned	Deleted		: 1;	/* Record is deleted	   */
	unsigned short	Days[7];		/* Days in the week	   */
	unsigned short	Month[32];		/* Days in the month	   */
	statcnt		Hatched;		/* Hatched statistics	   */
};



/*
 * Magic manager (magic.data)
 */
typedef enum {
	MG_EXEC,				/* Execute command	   */
	MG_COPY,				/* Copy file		   */
	MG_UNPACK,				/* Unpack file	 	   */
	MG_KEEPNUM,				/* Keep nr of files	   */
	MG_MOVE,				/* Move to other area	   */
	MG_UPDALIAS,				/* Update alias		   */
	MG_ADOPT,				/* Adopt file		   */
	MG_DELETE				/* Delete file		   */
} MAGICTYPE;

struct	_magichdr {
	int		hdrsize;		/* Size of header	   */
	int		recsize;		/* Size of records	   */
};

struct	_magic {
	char		Mask[15];		/* Filemask for magic	   */
	unsigned int	Attrib;			/* Record type		   */
	unsigned	Active		: 1;	/* Record active	   */
	unsigned	Compile		: 1;	/* Compile Flag 	   */
	unsigned	Deleted		: 1;	/* Deleted record	   */
	char		From[21];		/* From area		   */
	char		Path[65];		/* Destination path	   */
	char		Cmd[65];		/* Command to execute	   */
	int		KeepNum;		/* Keep number of files	   */
	char		ToArea[21];		/* Destination area	   */
};



/*
 * Newfile reports (newfiles.data)
 */
struct	_newfileshdr {
	int		hdrsize;		/* Size of header	   */
	int		recsize;		/* Size of records	   */
	int		grpsize;		/* Size of groups	   */
};

struct	_newfiles {
	char		Comment[56];		/* Comment		   */
	char		Area[51];		/* Message area		   */
	char		Origin[51];		/* Origin line, or random  */
	char		From[36];		/* From field		   */
	char		Too[36];		/* To field		   */
	char		Subject[61];		/* Subject field	   */
	int		Language;		/* Language		   */
	char		Template[15];		/* Template filename	   */
	fidoaddr	UseAka;			/* Aka to use		   */
	unsigned	Active		: 1;	/* Active		   */
	unsigned	xHiAscii	: 1;
	unsigned	Deleted		: 1;	/* Report is deleted	   */
	int		charset;		/* CHRS kludge to use	   */
};



/*
 * Scanmanager (scanmgr.data)
 */
struct	_scanmgrhdr {
	int		hdrsize;		/* Size of header	   */
	int		recsize;		/* Size of records	   */
};

struct	_scanmgr {
	char		Comment[56];		/* Comment		   */
	char		Origin[51];		/* Origin line		   */
	fidoaddr	Aka;			/* Fido address		   */
	char		ScanBoard[51];		/* Board to scan	   */
	char		ReplBoard[51];		/* Reply board		   */
	int		Language;		/* Language to use	   */
	char		template[15];		/* Template filename	   */
	unsigned	Active		: 1;	/* Record active	   */
	unsigned	NetReply	: 1;	/* Netmail reply	   */
	unsigned	Deleted		: 1;	/* Area is deleted	   */
	unsigned	xHiAscii	: 1;
	int		keywordlen;		/* Minimum keyword length  */
	int		charset;		/* Characterset to use	   */
};



/*
 * Record structure for file handling
 */
struct	_filerecord {
	char		Echo[21];		/* File echo		   */
	char		Comment[56];		/* Comment		   */
	char		Group[13];		/* Group		   */
	char		Name[13];		/* File Name		   */
	char		LName[81];		/* Long FileName	   */
	unsigned int	Size;			/* File Size		   */
	unsigned int	SizeKb;			/* File Size in Kb	   */
	int32_t		Fdate;			/* File Date		   */
	char		Origin[24];		/* Origin system	   */
	char		From[24];		/* From system		   */
	char		Crc[9];			/* CRC 32		   */
	char		Replace[81];		/* Replace file		   */
	char		Magic[21];		/* Magic name		   */
	char		Desc[256];		/* Short description	   */
	char		LDesc[25][49];		/* Long description	   */
	int		TotLdesc;		/* Total long desc lines   */
	unsigned	Announce	: 1;	/* Announce this file	   */
};



/*
 *  Mailer history file (mailhist.data)
 *  The first record conatains only the date (online) from which date this
 *  file is valid. The offline date is the date this file is created or
 *  packed. From the second record and on the records are valid data records.
 */
struct _history {
	fidoaddr	aka;			/* Node number		   */
	char		system_name[36];	/* System name		   */
	char		sysop[36];		/* Sysop name		   */
	char		location[36];		/* System location	   */
	char		tty[7];			/* Tty of connection	   */
	int		online;			/* Starttime of session	   */
	int		offline;		/* Endtime of session	   */
	unsigned int	sent_bytes;		/* Bytes sent		   */
	unsigned int	rcvd_bytes;		/* Bytes received	   */
	int		cost;			/* Session cost		   */
	unsigned	inbound		: 1;	/* Inbound session	   */
};



/*
 * Routing file, will override standard routing.
 * The mask is some kind of free formatted string like:
 *   1:All
 *   2:2801/16
 *   2:2801/All
 * If sname is used, the message To name is also tested for, this way
 * extra things can be done for netmail to a specific person.
 */
struct _routehdr {
	int            hdrsize;                /* Size of header	    */
	int            recsize;                /* Size of records	    */
};


struct _route {
	char		mask[25];		/* Mask to check	    */
	char		sname[37];		/* Opt. name to test	    */
	int		routetype;		/* What to do with it	    */
	fidoaddr	dest;			/* Destination address	    */
	char		dname[37];		/* Destination name	    */
	unsigned	Active	    : 1;	/* Is record active	    */
	unsigned	Deleted	    : 1;	/* Is record deleted	    */
};



/*
 * IBC servers to connect to.
 */
struct _ibcsrvhdr {
	int		hdrsize;		/* Size of header	    */
	int		recsize;		/* Size of record	    */
};


struct _ibcsrv {
	char		comment[41];		/* Comment		    */
	char		server[64];		/* Peer FQDN server name    */
	char		myname[64];		/* My FQDN server name	    */
	char		passwd[16];		/* Password		    */
	unsigned	Active	    : 1;	/* Is server active	    */
	unsigned	Deleted	    : 1;	/* Must server be deleted   */
	unsigned	Compress    : 1;	/* Use compresssion	    */
	unsigned	Dyndns	    : 1;	/* Remote uses dynamic dns  */
};



/*
 *  From clcomm.c
 */
char		*xmalloc(size_t);
char		*xstrcpy(char *);
char		*xstrcat(char *, char *);
void		InitClient(char *, char *, char *, char *, int, char *, char *, char *);
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
int		enoughspace(unsigned int);
unsigned int	sequencer(void);
char		*clencode(char *);
char		*cldecode(char *);
char		*printable(char *, int);
char		*printablec(char);



/*
 * From client.c
 */
int		iNode;	    /* Current node number  */
int		socket_connect(char *, char *, char *);
int		socket_send(char *);
char		*socket_receive(void);
int		socket_shutdown(pid_t);



/*
 *  From crc.c
 */
unsigned int	crc32ccitt(char *, int);
unsigned short	crc16ccitt(char *, int);
unsigned int	str_crc32(char *str);
unsigned int	StringCRC32(char *);
unsigned int	upd_crc32(char *buf, unsigned int crc, int len);
unsigned int	norm_crc32(unsigned int crc);
unsigned short	crc16xmodem(char *, int);
unsigned char	checksum(char *, int);



/*
 *  from semafore.c
 */
void            CreateSema(char *);
void            RemoveSema(char *);
int             IsSema(char *);



typedef struct _parsedaddr {
	char *target;
	char *remainder;
	char *comment;
} parsedaddr;



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
        int		date;
        char            *subj;
        char            *msgid_s;
        char            *msgid_a;
        unsigned int	msgid_n;
        char            *reply_s;
        char            *reply_a;
        unsigned int	reply_n;
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
int	le_int(int);
unsigned short	le_us(unsigned short);



/*
 * From attach.c
 */
int	attach(faddr, char *, int, char);
void	un_attach(faddr *, char *);



/*
 * From dostran.c
 */
char	*Dos2Unix(char *);
char	*Unix2Dos(char *);



/*
 * From execute.c
 */
int	execute(char **, char *, char *, char *);
int	execute_str(char *, char *, char *, char *, char *, char *);
int	execute_pth(char *, char *, char *, char *, char *);
int	execsh(char *, char *, char *, char *);



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
int		getheader(faddr *, faddr *, FILE *, char *, int);



/*
 * From gmtoffset.c
 */
int		gmt_offset(time_t);
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
int		file_size(char *path);
int		file_crc(char *path, int);
time_t		file_time(char *path);
int             mkdirs(char *name, mode_t);
int		getfilecase(char *, char *);



/*
 * From nodelock.c
 */
int		nodelock(faddr *, pid_t);
int		nodeulock(faddr *, pid_t);


/*
 * From noderecord.c
 */
int		noderecord(faddr *);



/*
 * From pktname.c
 */
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
void		mbse_Setraw(void);			/* Set raw mode		    */
void		mbse_Unsetraw(void);			/* Unset raw mode	    */
unsigned char	mbse_Getone(void);			/* Get one raw character    */
int		mbse_Speed(void);			/* Get (locked) tty speed   */
int		mbse_Waitchar(unsigned char *, int);	/* Wait n * 10mSec for char */
int		mbse_Escapechar(unsigned char *);	/* Escape sequence test	    */
unsigned char	mbse_Readkey(void);			/* Read a translated key    */



/*
 * From strutil.c
 */
char		*padleft(char *str, int size, char pad);
char		*tl(char *str);
void		Striplf(char *);
void		mbse_CleanSubject(char *);
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
void		mbse_TermInit(int, int, int);
void		mbse_colour(int, int);
void		mbse_clear(void);
void		mbse_locate(int, int);
void		mbse_mvprintw(int, int, const char *, ...);



/*
 * From unpacker.c
 */
char		*unpacker(char *);
int 		getarchiver(char *);



/*
 * From packet.c
 */
FILE		*openpkt(FILE *, faddr *, char, int);
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



/*
 * From hdr.c
 */
char *hdr(char *, rfcmsg *);



/*
 * From batchrd.c
 */
char *bgets(char *, int, FILE *);



/*
 * parsedate.c
 */
typedef struct _TIMEINFO {
    time_t  time;
    int	    usec;
    int	    tzone;
} TIMEINFO;

/*
**  Meridian:  am, pm, or 24-hour style.
*/
typedef enum _MERIDIAN {
    MERam, MERpm, MER24
} MERIDIAN;


typedef union {
    int32_t		Number;
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
int	is_8_3( char *);	    /* Return TRUE if name is 8.3	*/
void	mangle_name_83( char *);    /* Mangle name to 8.3 format	*/
void	name_mangle(char *);	    /* Mangle name or make uppercase	*/



/*
 * sectest.c
 */
int  Access(securityrec, securityrec);  /* Check security access	*/
int  Le_Access(securityrec, securityrec);  /* Endian independant	*/


/*
 * proglock.c
 */
int lockprogram(char *);	    /* Lock a program			*/
void ulockprogram(char *);	    /* Unlock a program			*/



/*
 * timers.c
 */
int gpt_resettimer(int);    	    /* Reset timer no			*/
void gpt_resettimers(void);	    /* Reset all timers			*/
int gpt_settimer(int, int);	    /* Set timer no to time		*/
int gpt_expired(int);		    /* Is timer expired			*/
int gpt_running(int);		    /* Is timer running			*/
int msleep(int);		    /* Milliseconds timer		*/


/*
 * remask.c
 */
char *re_mask(char *, int);	    /* Bluid file mask			*/



/*
 * rearc.c
 */ 
int rearc(char *, char *, int);     /* Rearc command                    */



/*
 * magic.c
 */
void magic_update(char *, char *);  /* Update magic alias		*/
int  magic_check(char *, char *);   /* Check if magic alias exists	*/



/*
 * pidinfo.c
 */
int pid2prog(pid_t, char *, size_t);	/* Find progrname for a pid	*/


/*
 * tmpwork.c
 */
void clean_tmpwork(void);	    /* Remove tmp workdir		*/
int  create_tmpwork(void);	    /* Create tmp workdir		*/


/*
 * virscan.c
 */
int VirScanFile(char *);		/* VirScan a file		*/


/*************************************************************************
 *
 *  Charset mapping
 */
int	find_ftn_charset(char *);	/* Return FTN charset index	    */
int	find_rfc_charset(char *);	/* Return RFC charset index	    */
char    *getftnchrs(int);               /* Return FTN characterset name     */
char	*getrfcchrs(int);		/* Return RFC characterset name	    */
char	*getlocale(int);		/* Return locale name		    */
char    *getchrsdesc(int);              /* Return characterset description  */
char	*get_ic_ftn(int);		/* Return iconv name FTN side	    */
char	*get_ic_rfc(int);		/* Return iconv name RFC side	    */
int	chartran_init(char *, char *, int);	/* Initialize chartran		    */
void	chartran_close(void);		/* Deinit chartran		    */
char	*chartran(char *);		/* Translate string		    */


/****************************************************************************
 *
 *  Records data
 */

struct	servicehdr	servhdr;		/* Services database	   */
struct	servicerec	servrec;

struct	sysrec		SYSINFO;		/* System info statistics  */

struct	prothdr		PROThdr;		/* Transfer protocols	   */
struct	prot		PROT;

struct	onelinehdr	olhdr;			/* Oneliner database	   */
struct	oneline		ol;

struct	fileareashdr	areahdr;		/* File areas		   */
struct	fileareas	area;
struct	OldFILERecord	oldfile;		/* Pre 0.51.2 structure	   */
struct	FILE_recordhdr	fdbhdr;    		/* Files database          */
struct	FILE_record	fdb;
struct	_fgrouphdr	fgrouphdr;		/* File groups		   */
struct	_fgroup		fgroup;

struct	_ngrouphdr	ngrouphdr;		/* Newfiles groups	   */
struct	_ngroup		ngroup;	

struct	bbslisthdr	bbshdr;			/* BBS list		   */
struct	bbslist		bbs;

struct	lastcallershdr	LCALLhdr;		/* Lastcallers info	   */
struct	lastcallers	LCALL;

struct	sysconfig	CFG;			/* System configuration	   */

struct	limitshdr	LIMIThdr;		/* User limits		   */
struct	limits		LIMIT;

struct	menufile	menus;

struct	msgareashdr	msgshdr;		/* Messages configuration  */
struct	msgareas	msgs;
struct	_mgrouphdr	mgrouphdr;		/* Message groups	   */
struct	_mgroup		mgroup;

struct	languagehdr	langhdr;		/* Language data	   */
struct	language	lang;			  			  
struct	langdata	ldata;

struct	_fidonethdr	fidonethdr;		/* Fidonet structure	   */
struct	_fidonet	fidonet;
struct  domhdr		domainhdr;
struct  domrec		domtrans;

struct	_archiverhdr	archiverhdr;		/* Archivers		   */
struct	_archiver	archiver;

struct	_virscanhdr	virscanhdr;		/* Virus scanners	   */
struct	_virscan	virscan;

struct	_ttyinfohdr	ttyinfohdr;		/* TTY lines		   */
struct	_ttyinfo	ttyinfo;
struct	_modemhdr	modemhdr;		/* Modem models		   */
struct	_modem		modem;

struct	_tichdr		tichdr;			/* TIC areas		   */
struct	_tic		tic;
struct	_hatchhdr	hatchhdr;		/* Hatch areas		   */
struct	_hatch		hatch;
struct	_magichdr	magichdr;		/* Magic areas		   */
struct	_magic		magic;

struct	_nodeshdr	nodeshdr;		/* Fidonet nodes	   */
struct	_nodes		nodes;

struct	_newfileshdr	newfileshdr;		/* New file reports	   */
struct	_newfiles	newfiles;

struct	_scanmgrhdr	scanmgrhdr;		/* Filefind areas	   */
struct	_scanmgr	scanmgr;

struct	_routehdr	routehdr;		/* Routing file		    */
struct	_route		route;

struct	_ibcsrvhdr	ibcsrvhdr;		/* IBC servers		    */
struct	_ibcsrv		ibcsrv;


#endif
