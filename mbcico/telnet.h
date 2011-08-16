#ifndef	_TELNET_H
#define	_TELNET_H

/* $Id: telnet.h,v 1.2 2004/02/01 21:17:40 mbroek Exp $ */


#define TOPT_BIN                0
#define TOPT_ECHO               1
#define TOPT_SUPP               3

void telnet_init(int);
void telnet_answer(int, int, int);
void telout_filter(int, int);
void telin_filter(int, int);


#endif
