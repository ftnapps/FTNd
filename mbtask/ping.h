#ifndef _PING_H
#define _PING_H

/* $Id$ */

/*
 *  Defines. 
 */
#define ICMP_BASEHDR_LEN        8
#define ICMP_MAX_ERRS           5
#define SET_SOCKA_LEN4(socka)

void	init_pingsocket(void);
void	*ping_thread(void*);

#endif
