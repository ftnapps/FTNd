#ifndef	_TELNET_H
#define	_TELNET_H

/* telnet.h */


#define TOPT_BIN                0
#define TOPT_ECHO               1
#define TOPT_SUPP               3

void telnet_init(int);
void telnet_answer(int, int, int);
void telout_filter(int, int);
void telin_filter(int, int);


#endif
