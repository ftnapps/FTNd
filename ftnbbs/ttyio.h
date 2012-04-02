/* $Id: ttyio.h,v 1.5 2006/03/20 19:13:14 mbse Exp $ */

#ifndef TTYIO_H
#define TTYIO_H

#define TCHECK() tty_check()
#define FLUSHIN() tty_flushin()
#define FLUSHOUT() tty_flushout()
#define PUTCHAR(x) tty_putc(x)
#define PUT(x,y) tty_put(x,y)
#define PUTSTR(x) tty_put(x,strlen(x))
#define GETCHAR(x) tty_getc(x)
#define STATUS tty_status

#define STAT_SUCCESS 0
#define STAT_ERROR   1
#define STAT_TIMEOUT 2
#define STAT_EOFILE  3
#define STAT_HANGUP  4
#define STAT_EMPTY   5
#define	STAT_UNCOMP  6

#define SUCCESS (STATUS == 0)
#define TERROR (-STAT_ERROR)
#define TIMEOUT (-STAT_TIMEOUT)
#define EOFILE (-STAT_EOFILE)
#define HANGUP (-STAT_HANGUP)
#define EMPTY (-STAT_EMPTY)


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
#ifndef XON
#define XON 0x11
#endif
#define DC1 0x11
#define DC2 0x12
#ifndef	XOFF
#define XOFF 0x13
#endif
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


int	tty_status;
int	tty_check(void);
int	tty_getc(int);
int	tty_putc(int);
int	tty_put(char *, int);
int	tty_get(char *, int, int);
void	tty_flushin(void);
void	tty_flushout(void);

#endif
