#ifndef	_BINKP_H
#define	_BINKP_H

/*

   binkp's frames:

    +---------------------- 0=data block, 1=message(command)
    |                +---- data block size / msg's argument size
    |                |
    7  6543210 76543210
   +-+-------+--------+--- ... ---+
   | |   HI      LO   |           | -- data block / msg's argument
   +-+-------+--------+--- ... ---+

 */


/* protocol version */
#define BINKP_VERSION "1.1"

#define MAX_BLKSIZE 0x7fff	/* Don't change!				*/
#define	BLK_HDR_SIZE 2		/* 2 bytes header				*/
#define	BINKP_TIMEOUT 180	/* Global timeout value				*/
#define	SND_BLKSIZE 4096	/* Blocksize transmitter			*/

#define MM_NUL	0		/* Ignored by binkp (data optionally logged)	*/
#define MM_ADR	1		/* System aka's					*/
#define MM_PWD	2		/* Password					*/
#define MM_FILE	3
#define MM_OK	4		/* The password is ok (data ignored)		*/
#define MM_EOB	5		/* End-of-batch (data ignored)			*/
#define MM_GOT	6		/* File received				*/
#define MM_ERR	7		/* Misc errors					*/
#define MM_BSY	8		/* All AKAs are busy				*/
#define MM_GET	9		/* Get a file from offset			*/
#define MM_SKIP	10		/* Skip a file					*/
#define MM_MAX  10

#define MM_DATA  0xff

#define BINKP_DATA_BLOCK	0x0000
#define BINKP_CONTROL_BLOCK	0x8000


typedef struct _binkp_frame {
	unsigned short	header;
	unsigned char	id;
	unsigned char	*data;
} binkp_frame;



/*
 *  Linked list of files to send and responses from the receiver.
 */
typedef enum {NoState, Sending, IsSent, Got, Skipped, Get} FileState;

typedef struct _binkp_list {
	struct _binkp_list	*next;
	char			*remote;	/* Remote filename		*/
	char			*local;		/* Local filename		*/
	int			state;		/* File state			*/
	int			get;		/* Boolean GET flag		*/
	off_t			size;		/* File size			*/
	time_t			date;		/* File date & time		*/
	off_t			offset;		/* Start offset			*/
} binkp_list;



/*
 * state.NR_flag: state of binkp when in NR mode
 */
#define NO_NR   0
#define WANT_NR 1
#define WE_NR   2
#define THEY_NR 3


int binkp(int);

#endif

