/* $Id$ */

#ifndef _PING_H
#define	_PING_H


typedef enum {P_BOOT, P_PAUSE, P_SENT, P_WAIT, P_ERROR} PINGSTATE;



/*
 *  Defines. 
 */
#define ICMP_BASEHDR_LEN        8
#define ICMP_MAX_ERRS           5
#define SET_SOCKA_LEN4(socka)

void		check_ping(void);
void		init_pingsocket(void);

#endif
