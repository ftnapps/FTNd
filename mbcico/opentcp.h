/* $Id$ */

#ifndef	_OPENTCP_H
#define	_OPENTCP_H

int  opentcp(char *);
void closetcp(void);

#ifdef USE_TELNET
void telnet_init(void);
#endif

#endif
