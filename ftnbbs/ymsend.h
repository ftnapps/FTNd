#ifndef	_YMSEND_H
#define	_YMSEND_H

/* ymsend.h */

#define RETRYMAX 10

#define CPMEOF 032  /* CP/M EOF, Ctrl-Z				*/
#define WANTCRC 'C' /* send C not NAK to get crc not checksum	*/
#define WANTG 'G'   /* Send G not NAK to get nonstop batch xmsn */


int ymsndfiles(down_list *, int);

#endif
