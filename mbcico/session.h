/* $Id: session.h,v 1.10 2007/04/30 19:04:18 mbse Exp $ */

#ifndef _SESSION_H
#define _SESSION_H

#define TCPMODE_NONE 0
#define	TCPMODE_IFC 1		/* ifcico native EMSI on raw TCP */
#define	TCPMODE_ITN 2		/* EMSI encapsulation through telnet */
#define	TCPMODE_IBN 3		/* Binkp protocol */

#define SESSION_UNKNOWN 0
#define SESSION_FTSC 1
#define SESSION_YOOHOO 2
#define SESSION_EMSI 3
#define SESSION_BINKP 4

#define SESSION_SLAVE 0
#define SESSION_MASTER 1

#define	STATE_SECURE 0
#define	STATE_UNSECURE 1
#define	STATE_BAD 2

extern node *nlent;
extern fa_list *remote;

typedef struct _file_list {
	struct _file_list *next;
	char *local;
	char *remote;
	int disposition;
	FILE *flofp;
	off_t floff;
} file_list;

#define HOLD_MAIL "h"
#define NONHOLD_MAIL "dco"
#define ALL_MAIL "dcoh"

extern int session_flags;
extern int remote_flags;
#define FTSC_XMODEM_CRC  1 /* xmodem-crc */
#define FTSC_XMODEM_RES  2 /* sealink-resync */
#define FTSC_XMODEM_SLO  4 /* sealink-overdrive */
#define FTSC_XMODEM_XOF  8 /* xoff flow control, aka macflow */
#define WAZOO_ZMODEM_ZAP 1 /* ZedZap allowed */

#define SESSION_WAZOO 0x8000 /* WaZOO type file requests */
#define SESSION_BARK  0x4000 /* bark type file requests */
#define SESSION_IFNA  0x2000 /* DietIFNA transfer from Yoohoo session */
#define SESSION_FNC   0x1000 /* Filename conversion sending files */

#define SESSION_TCP   0x0800 /* Established over TCP/IP link */
#define SESSION_HYDRA 0x0400 /* Hydra special file requests  */

extern int localoptions;
#define NOCALL   0x0001
// #define NOHOLD   0x0002
// #define NOPUA    0x0004
#define NOWAZOO  0x0008
#define NOEMSI   0x0010
#define NOFREQS  0x0020
#define NOZMODEM 0x0040
#define NOZEDZAP 0x0080
#define	NOJANUS  0x0100
#define NOHYDRA  0x0200
#define	NOPLZ    0x0400
#define	NOGZBZ2  0x0800
#define	NONR	 0x1000

struct	_history	history;	/* History record for sessions	*/

int session(faddr*,node*,int,int,char*);

#endif

