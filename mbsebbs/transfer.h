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
	long			area;		/* File area or 0	*/
	unsigned long		size;		/* File size		*/
	unsigned		kfs	: 1;	/* Kill File Sent	*/
	unsigned		sent	: 1;	/* File is Sent		*/
	unsigned		failed	: 1;	/* Transfer failed	*/
} down_list;



/*
 *  List of uploaded files.
 */
typedef struct _up_list {
	struct _up_list		*next;
	char			*filename;	/* Filename		*/
	unsigned long		size;		/* Filesize		*/
} up_list;


int     ForceProtocol(void);
void	add_download(down_list **, char *, char *, long, unsigned long, int);
void	tidy_download(down_list **);
int	download(down_list *);
void	tidy_upload(up_list **);
int	upload(up_list **);
char	*transfertime(struct timeval, struct timeval, long bytes, int);

#endif
