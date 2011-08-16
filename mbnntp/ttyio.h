#ifndef TTYIO_H
#define TTYIO_H

/* $Id: ttyio.h,v 1.2 2004/04/12 11:50:34 mbroek Exp $ */

#define FLUSHOUT() tty_flushout()
#define FLUSHIN() tty_flushin()
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

extern int tty_status;

extern int tty_getc(int);
extern int tty_put(char*,int);
extern void tty_flushout(void);
extern void tty_flushin(void);

#endif
