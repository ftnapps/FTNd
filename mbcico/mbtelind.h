#ifndef	_MBTELIND_H
#define	_MBTELIND_H

/* $Id$ */

#define MBT_WILL                251
#define MBT_WONT                252
#define MBT_DO                  253
#define MBT_DONT                254
#define MBT_IAC			255

#define TOPT_BIN		0
#define TOPT_ECHO		1
#define TOPT_SUPP		3


void die(int);
void com_gw(int);
void telnet_answer(int, int);
int  telnet_init(void);
int  telnet_read(char *, int);
int  telnet_write(char *, int);
int  telnet_buffer(char *, int);

#endif
