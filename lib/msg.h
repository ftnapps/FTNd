#ifndef	_MSG_H
#define	_MSG_H

#define MAX_LINE_LENGTH	512


/*
 * Global message buffer
 */
typedef	struct _msgbuf {
	unsigned long	Id;
	unsigned long	Current;
	char		From[101];		/* From name		    */
	char		To[101];		/* To name		    */
	char		Subject[101];		/* Message subject	    */
	unsigned	Local		: 1;	/* Message is local	    */
	unsigned	Intransit	: 1;	/* Message is in transit    */
	unsigned	Private		: 1;	/* Message is private	    */
	unsigned	Received	: 1;	/* Message is received	    */
	unsigned	Sent		: 1;	/* Message is sent	    */
	unsigned	KillSent	: 1;	/* Kill after sent	    */
	unsigned	ArchiveSent	: 1;	/* Archive after sent	    */
	unsigned	Hold		: 1;	/* Hold message		    */
	unsigned	Crash		: 1;	/* Crash flag		    */
	unsigned	Immediate	: 1;	/* Immediate mail	    */
	unsigned	Direct		: 1;	/* Direct flag		    */
	unsigned	Gate		: 1;	/* Send via gateway	    */
	unsigned	FileRequest	: 1;	/* File request		    */
	unsigned	FileAttach	: 1;	/* File attached	    */
	unsigned	TruncFile	: 1;	/* Trunc file after sent    */
	unsigned	KillFile	: 1;	/* Kill file after sent	    */
	unsigned	ReceiptRequest	: 1;	/* Return receipt request   */
	unsigned	ConfirmRequest	: 1;	/* Confirm receipt request  */
	unsigned	Orphan		: 1;	/* Orphaned message	    */
	unsigned	Encrypt		: 1;	/* Encrypted message	    */
	unsigned	Compressed	: 1;	/* Compressed message	    */
	unsigned	Escaped		: 1;	/* Msg is 7bit ASCII	    */
	unsigned	ForcePU		: 1;	/* Force PickUp		    */				
	unsigned	Localmail	: 1;	/* Local use only	    */
	unsigned	Echomail	: 1;	/* Echomail flag	    */
	unsigned	Netmail		: 1;	/* Netmail flag		    */
	unsigned	News		: 1;	/* News article		    */
	unsigned	Email		: 1;	/* e-mail message	    */
	unsigned	Nntp		: 1;	/* Offer to NNTP server	    */
	unsigned	Nodisplay	: 1;	/* No display to user	    */
	unsigned	Locked		: 1;	/* Locked, no edit allowed  */
	unsigned	Deleted		: 1;	/* Msg is deleted	    */
	time_t		Written;		/* Date message is written  */
	time_t		Arrived;		/* Date message arrived	    */
	time_t		Read;			/* Date message is received */
	char		FromAddress[101];	/* From address		    */
	char		ToAddress[101];		/* To address		    */
	unsigned long	Reply;			/* Message is reply to	    */
	unsigned long	Original;		/* Original message	    */
	unsigned long	MsgIdCRC;		/* Message Id CRC	    */
	unsigned long	ReplyCRC;		/* Reply CRC		    */
	char		Msgid[81];		/* Msgid string		    */
	char		Replyid[81];		/* Replyid string	    */
	char		ReplyAddr[81];		/* Gated Reply Address	    */
	char		ReplyTo[81];		/* Gated Reply To	    */
	long		Size;			/* Message size during write*/
} msgbuf;



/*
 * Globale message area buffer
 */
typedef	struct _msgbase {
	char		Path[PATH_MAX];		/* Path to message base	    */
	unsigned	Open		: 1;	/* If base is open	    */
	unsigned	Locked		: 1;	/* If base is locked	    */
	unsigned long	LastNum;		/* Lastread message number  */
	unsigned long	Lowest;			/* Lowest message number    */
	unsigned long	Highest;		/* Highest message number   */
	unsigned long	Total;			/* Total number of msgs	    */
} msgbase;



/*
 * LastRead structure
 */
typedef struct _lastread {
	unsigned long	UserCRC;		/* CRC32 lowercase username */
	unsigned long	UserID;			/* Unique user-id	    */
	unsigned long	LastReadMsg;		/* Last Read message number */
	unsigned long	HighReadMsg;		/* Highes read message	    */
} lastread;



/*
 * Global variables
 */
msgbuf		Msg;				/* Message buffer	    */
msgbase		MsgBase;			/* Message Base buffer	    */
msgbase		EmailBase;			/* Email Base buffer	    */
lastread	LastRead;			/* LastRead pointer record  */
char		szWrp[MAX_LINE_LENGTH + 1];



/*
 * Common function prototypes.
 */
char		*strlwr(char *);
char		*strupr(char *);
long		filelength(int);
long		tell(int);



/*
 * Message Base Prototypes
 */
int		Msg_AddMsg(void);
void		Msg_Close(void);
int		Msg_Delete(unsigned long);
int		Msg_GetLastRead(lastread *);
unsigned long	Msg_Highest(void);
int		Msg_Lock(unsigned long);
unsigned long	Msg_Lowest(void);
void		Msg_New(void);
int		Msg_NewLastRead(lastread);
int		Msg_Next(unsigned long *);
unsigned long	Msg_Number(void);
int		Msg_Open(char *);
void		Msg_Pack(void);
int		Msg_Previous(unsigned long *);
int		Msg_ReadHeader(unsigned long);
int		Msg_Read(unsigned long, int);
int		Msg_SetLastRead(lastread);
void		Msg_UnLock(void);
int		Msg_WriteHeader(unsigned long);
void		Msg_Write(FILE *);


#endif

