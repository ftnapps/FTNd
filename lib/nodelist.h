/* $Id$ */

#ifndef _NODELIST_H
#define	_NODELIST_H

#include "../config.h"


#define MAXNAME 35
#define MAXUFLAGS 16



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
	int		t1;			/* T flag, first char	*/
	int		t2;			/* T flag, second char	*/
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


#endif

