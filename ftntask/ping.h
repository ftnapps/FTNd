#ifndef _PING_H
#define _PING_H

/* ping.h,v */

/*
 *  Defines. 
 */
#define ICMP_BASEHDR_LEN        8
#define ICMP_MAX_ERRS           5
#define SET_SOCKA_LEN4(socka)

void init_pingsocket(void);
void ping_receive(char *, int);
void check_ping(void);
void deinit_ping(void);

#endif
