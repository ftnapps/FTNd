#ifndef	_DBMSGS_H
#define	_DBMSGS_H


struct	msgareashdr	msgshdr;	/* Header record		    */
struct	msgareas	msgs;		/* Msgss datarecord		    */
struct	_mgrouphdr	mgrouphdr;	/* Group header record		    */
struct	_mgroup		mgroup;		/* Group record			    */
int			msgs_cnt;	/* Msgs records in database	    */

int	InitMsgs(void);			/* Initialize msgs database	    */
int	SearchMsgs(char *);		/* Search specified msg area	    */
int	SearchMsgsNews(char *);		/* Search specified msg area	    */
int	MsgSystemConnected(sysconnect); /* Is system connected		    */
int	MsgSystemConnect(sysconnect *, int); /* Connect/change/delete system*/
int	GetMsgSystem(sysconnect *, int);/* Get connected system		    */
int	SearchNetBoard(unsigned short, unsigned short);	/* Search netmail   */
void	UpdateMsgs(void);		/* Update current messages record   */

#endif

