#ifndef	_YMRECV_H
#define	_YMRECV_H

/* $Id$ */

#define WANTCRC 0103    /* send C not NAK to get crc not checksum */
#define WCEOT (-10)

#define RETRYMAX 10

int wcrxpn(char *);
int wcrx(void);

#endif
