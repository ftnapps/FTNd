#ifndef _PING_H
#define _PING_H

/* $Id: ping.h,v 1.6 2006/02/13 19:26:30 mbse Exp $ */

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
