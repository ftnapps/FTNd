/* $Id$ */

#ifndef	_OPENTCP_H
#define	_OPENTCP_H

#define MBT_TIMEOUT 500
#define MBT_BUFLEN  8192

#define TOPT_BIN                0
#define TOPT_ECHO               1
#define TOPT_SUPP               3

int  opentcp(char *);
void closetcp(void);

#endif
