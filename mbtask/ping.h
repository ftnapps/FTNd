/* $Id$ */

#ifndef _PING_H
#define	_PING_H


typedef enum {P_INIT, P_SENT, P_FAIL, P_OK, P_ERROR, P_NONE} PINGSTATE;



/*
 *  Defines. 
 */
#define ICMP_BASEHDR_LEN        8
#define ICMP_MAX_ERRS           5
#define SET_SOCKA_LEN4(socka)

void		check_ping(void);
void		state_ping(void);
void		init_pingsocket(void);

#endif
