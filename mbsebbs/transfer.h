#ifndef	_TRANSFER_H
#define	_TRANSFER_H

/* $Id$ */



/*
 *  List of files to download.
 */
typedef struct _down_list {
	struct _down_list	*next;
	char			*local;		/* Local filename	*/
	char			*remote;	/* Remove filename	*/
	long			cps;		/* CPS after sent	*/
	unsigned		kfs	: 1;	/* Kill File Sent	*/
	unsigned		sent	: 1;	/* File is Sent		*/
} down_list;


/*
 *  List of uploaded files.
 */
typedef struct _up_list {
	struct _up_list		*next;
	char			*remote;	/* Remote filename	*/
	char			*local;		/* Local filename	*/
	long			cps;		/* CPS after received	*/
	unsigned		success	: 1;	/* If received Ok.	*/
} up_list;


int download(down_list **);
int upload(up_list **);


#endif
