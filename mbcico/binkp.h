#ifndef	_BINKP_H
#define	_BINKP_H


/* $Id: binkp.h,v 1.11 2005/09/12 13:47:09 mbse Exp $ */

/*

   binkp's frames:

    +---------------------- 0=data block, 1=message(command)
    |                +----- data block size / msg's argument size
    |                |
    7 6543210 76543210
   +-+-------+--------+--- ... ---+
   | |   HI      LO   |           | -- data block / msg's argument
   +-+-------+--------+--- ... ---+

   binkp's frames in PLZ mode:

    +---------------------- 0=data block, 1=message(command)
    | +-------------------- 0=standard block, 1=zlib compressed block
    | |               +---- data block size / msg's argument size
    | |               |
    7 6 543210 76543210
   +-+-+------+--------+--- ... ---+
   | | |  HI      LO   |           | -- data block / msg's argument
   +-+-+------+--------+--- ... ---+

 */


/* protocol version */
#define	PRTCLNAME "binkp"
#define PRTCLVER "1.1"
#define PRTCLOLD "1.0"

#define MAX_BLKSIZE 0x7fff	/* Don't change!				*/
#define	BLK_HDR_SIZE 2		/* 2 bytes header				*/
#define	BINKP_TIMEOUT 300	/* Global timeout value				*/
#define	SND_BLKSIZE 4096	/* Blocksize transmitter			*/

#define MM_NUL	0		/* Ignored by binkp (data optionally logged)	*/
#define MM_ADR	1		/* System aka's					*/
#define MM_PWD	2		/* Password					*/
#define MM_FILE	3
#define MM_OK	4		/* The password is ok				*/
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
#define	BINKP_PLZ_BLOCK		0x4000
#define BINKP_ZIPBUFLEN		(((BINKP_PLZ_BLOCK * 11) / 10) + 12)

typedef struct _binkp_frame {
	unsigned short	header;
	unsigned char	id;
	unsigned char	*data;
} binkp_frame;



/*
 *  Linked list of files to send and responses from the receiver.
 */
typedef struct _binkp_list {
	struct _binkp_list	*next;
	char			*remote;	/* Remote filename		*/
	char			*local;		/* Local filename		*/
	int			state;		/* File state			*/
	int			get;		/* Boolean GET flag		*/
	off_t			size;		/* File size			*/
	time_t			date;		/* File date & time		*/
	off_t			offset;		/* Start offset			*/
	int			compress;	/* Compression state		*/
} binkp_list;



/*
 * Linked FIFO list of received commands to be processed by the transmitter.
 */
typedef struct _the_queue {
    struct  _the_queue		*next;
    int				cmd;		/* M_xxx command id		*/
    char			*data;		/* Frame data in the queue	*/
} the_queue;



int  binkp(int);
void binkp_abort(void);

#endif
