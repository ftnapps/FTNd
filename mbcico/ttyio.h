/* $Id$ */

#ifndef TTYIO_H
#define TTYIO_H

/*
 * Timer numbers for Hydra
 */
#define TIMERNO_BRAIN	0
#define TIMERNO_RX	1
#define	TIMERNO_TX	2

#define RESETTIMER(x) tty_resettimer(x)
#define RESETTIMERS() tty_resettimers()
#define SETTIMER(x,y) tty_settimer(x,y)
#define EXPIRED(x) tty_expired(x)
#define RUNNING(x) tty_running(x)

#define TCHECK() tty_check()
#define PUTCHECK(x) tty_putcheck(x)
#define WAITPUTGET(x) tty_waitputget(x)
#define FLUSHOUT() tty_flushout()
#define FLUSHIN() tty_flushin()
#define PUTCHAR(x) tty_putc(x)
#define PUT(x,y) tty_put(x,y)
#define PUTSTR(x) tty_put(x,strlen(x))
#define GETCHAR(x) tty_getc(x)
#define UNGETCHAR(x) tty_ungetc(x)
#define GET(x,y,z) tty_get(x,y,z)
#define PUTGET(a,b,x,y) tty_putget(a,b,x,y)
#define STATUS tty_status

#define STAT_SUCCESS 0
#define STAT_ERROR   1
#define STAT_TIMEOUT 2
#define STAT_EOFILE  3
#define STAT_HANGUP  4
#define STAT_EMPTY   5

#define SUCCESS (STATUS == 0)
#define TERROR (-STAT_ERROR)
#define TIMEOUT (-STAT_TIMEOUT)
#define EOFILE (-STAT_EOFILE)
#define HANGUP (-STAT_HANGUP)
#define EMPTY (-STAT_EMPTY)

#define GET_COMPLETE(x) (x & 1)
#define PUT_COMPLETE(x) (x & 2)

#ifndef NUL
#define NUL 0x00
#endif
#define SOH 0x01
#define STX 0x02
#define ETX 0x03
#define EOT 0x04
#define ENQ 0x05
#define ACK 0x06
#define BEL 0x07
#define BS  0x08
#define HT  0x09
#define LF  0x0a
#define VT  0x0b
#ifndef	FF
#define FF  0x0c
#endif
#ifndef	CR
#define CR  0x0d
#endif
#define SO  0x0e
#define SI  0x0f
#define DLE 0x10
#define XON 0x11
#define DC1 0x11
#define DC2 0x12
#define XOFF 0x13
#define DC3 0x13
#define DC4 0x14
#define NAK 0x15
#define SYN 0x16
#define ETB 0x17
#define CAN 0x18
#define EM  0x19
#define SUB 0x1a
#ifndef	ESC
#define ESC 0x1b
#endif
#define RS  0x1e
#define US  0x1f
#define TSYNC 0xae
#define YOOHOO 0xf1

/* ### Modifned by T.Tanaka on 4 Dec 1995 */
#define	ClearArray(x)		memset((char *)x, 0, sizeof x)

#define	NETADD(c)	{ PUTCHAR(c); }
#define	NET2ADD(c1,c2)	{ NETADD(c1); NETADD(c2); }

#define	MY_STATE_WILL		0x01
#define	MY_WANT_STATE_WILL	0x02
#define	MY_STATE_DO		0x04
#define	MY_WANT_STATE_DO	0x08
#define	my_state_is_do(opt)		(telnet_options[opt]&MY_STATE_DO)
#define	my_state_is_will(opt)		(telnet_options[opt]&MY_STATE_WILL)
#define my_want_state_is_do(opt)	(telnet_options[opt]&MY_WANT_STATE_DO)
#define my_want_state_is_will(opt)	(telnet_options[opt]&MY_WANT_STATE_WILL)
#define	my_state_is_dont(opt)		(!my_state_is_do(opt))
#define	my_state_is_wont(opt)		(!my_state_is_will(opt))
#define my_want_state_is_dont(opt)	(!my_want_state_is_do(opt))
#define my_want_state_is_wont(opt)	(!my_want_state_is_will(opt))
#define	set_my_want_state_do(opt)	{telnet_options[opt] |= MY_WANT_STATE_DO;}
#define	set_my_want_state_will(opt)	{telnet_options[opt] |= MY_WANT_STATE_WILL;}
#define	set_my_want_state_dont(opt)	{telnet_options[opt] &= ~MY_WANT_STATE_DO;}
#define	set_my_want_state_wont(opt)	{telnet_options[opt] &= ~MY_WANT_STATE_WILL;}

#define	IAC	255		/* interpret as command: */
#define	DONT	254		/* you are not to use option */
#define	DO	253		/* please, you use option */
#define	WONT	252		/* I won't use option */
#define	WILL	251		/* I will use option */

/* telnet options */
#define TELOPT_BINARY	0	/* 8-bit data path */
#define TELOPT_ECHO	1	/* echo */
#define	TELOPT_RCP	2	/* prepare to reconnect */
#define	TELOPT_SGA	3	/* suppress go ahead */
#define	TELOPT_NAMS	4	/* approximate message size */
#define	TELOPT_STATUS	5	/* give status */
#define	TELOPT_TM	6	/* timing mark */
#define	TELOPT_RCTE	7	/* remote controlled transmission and echo */
#define TELOPT_NAOL 	8	/* negotiate about output line width */
#define TELOPT_NAOP 	9	/* negotiate about output page size */
#define TELOPT_NAOCRD	10	/* negotiate about CR disposition */
#define TELOPT_NAOHTS	11	/* negotiate about horizontal tabstops */
#define TELOPT_NAOHTD	12	/* negotiate about horizontal tab disposition */
#define TELOPT_NAOFFD	13	/* negotiate about formfeed disposition */
#define TELOPT_NAOVTS	14	/* negotiate about vertical tab stops */
#define TELOPT_NAOVTD	15	/* negotiate about vertical tab disposition */
#define TELOPT_NAOLFD	16	/* negotiate about output LF disposition */
#define TELOPT_XASCII	17	/* extended ascic character set */
#define	TELOPT_LOGOUT	18	/* force logout */
#define	TELOPT_BM	19	/* byte macro */
#define	TELOPT_DET	20	/* data entry terminal */
#define	TELOPT_SUPDUP	21	/* supdup protocol */
#define	TELOPT_SUPDUPOUTPUT 22	/* supdup output */
#define	TELOPT_SNDLOC	23	/* send location */
#define	TELOPT_TTYPE	24	/* terminal type */
#define	TELOPT_EOR	25	/* end or record */
#define	TELOPT_TUID	26	/* TACACS user identification */
#define	TELOPT_OUTMRK	27	/* output marking */
#define	TELOPT_TTYLOC	28	/* terminal location number */
#define	TELOPT_3270REGIME 29	/* 3270 regime */
#define	TELOPT_X3PAD	30	/* X.3 PAD */
#define	TELOPT_NAWS	31	/* window size */
#define	TELOPT_TSPEED	32	/* terminal speed */
#define	TELOPT_LFLOW	33	/* remote flow control */
#define TELOPT_LINEMODE	34	/* Linemode option */
#define TELOPT_XDISPLOC	35	/* X Display Location */
#define TELOPT_ENVIRON	36	/* Environment variables */
#define	TELOPT_AUTHENTICATION 37/* Authenticate */
#define	TELOPT_ENCRYPT	38	/* Encryption option */
#define	TELOPT_EXOPL	255	/* extended-options-list */
/* ### */

extern int tty_status;

extern int tty_check(void);
extern int tty_waitputget(int);
extern int tty_ungetc(int);
extern int tty_getc(int);
extern int tty_get(char*,int,int);
extern int tty_putcheck(int);
extern int tty_putc(int);
extern int tty_put(char*,int);
extern int tty_putget(char**,int*,char**,int*);
extern void tty_flushout(void);
extern void tty_flushin(void);
extern void sendbrk(void);
extern int tty_resettimer(int tno);
extern void tty_resettimers(void);
extern int tty_settimer(int,int);
extern int tty_expired(int);
extern int tty_running(int);

#endif
