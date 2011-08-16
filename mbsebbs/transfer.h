#ifndef	_TRANSFER_H
#define	_TRANSFER_H

/* $Id: transfer.h,v 1.8 2005/10/11 20:49:48 mbse Exp $ */



/*
 *  List of files to download.
 */
typedef struct _down_list {
	struct _down_list	*next;
	char			*local;		/* Local filename	*/
	char			*remote;	/* Remove filename	*/
	int			cps;		/* CPS after sent	*/
	int			area;		/* File area or 0	*/
	unsigned int		size;		/* File size		*/
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
	unsigned int		size;		/* Filesize		*/
} up_list;


int     ForceProtocol(void);
void	add_download(down_list **, char *, char *, int, unsigned int, int);
void	tidy_download(down_list **);
int	download(down_list *);
void	tidy_upload(up_list **);
int	upload(up_list **);
char	*transfertime(struct timeval, struct timeval, int bytes, int);

#endif
