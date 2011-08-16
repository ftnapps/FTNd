#ifndef _TASKIBC_H
#define	_TASKIBC_H

/* $Id: taskibc.h,v 1.28 2006/05/25 19:17:49 mbse Exp $ */

#define MAXIBC_NCS      50                  /* Maximum Neighbour ChatServers    */
#define MAXIBC_SRV      200                 /* Maximum Servers in chatnetwork   */
#define MAXIBC_USR      500                 /* Maximum Users in chatnetwork     */
#define MAXIBC_CHN      200                 /* Maximum Channels in chatnetwork  */



/*
 * Linked list of neighbour chatservers
 */
typedef struct _ncs_rec {
    char		server[64];	    /* Server address		*/
    char		resolved[64];	    /* Resolved server address	*/
    char		myname[64];	    /* My server address	*/
    char		passwd[16];	    /* Server password		*/
    int			state;		    /* Connection state		*/
    time_t		action;		    /* Time for next action	*/
    time_t		last;		    /* Last received message	*/
    int			version;	    /* Protocol version of peer	*/
    unsigned		remove	    : 1;    /* If entry must be removed	*/
    unsigned		compress    : 1;    /* User link compression	*/
    unsigned		gotpass	    : 1;    /* Received valid password	*/
    unsigned		gotserver   : 1;    /* Received valid server	*/
    unsigned		dyndns	    : 1;    /* Is a dynamic dns remote	*/
    struct sockaddr_in	servaddr_in;	    /* Peer socketaddress	*/
    int			socket;		    /* Peer socket		*/
    unsigned int	token;		    /* Server token		*/
    int			halfdead;	    /* Halfdead connect count	*/
    unsigned int	crc;		    /* CRC value of record	*/
} _ncs_list;

_ncs_list		ncs_list[MAXIBC_NCS];



/*
 * Database with servers
 */
typedef struct _srv_rec {
    char		server[64];	    /* FQDN of the server	*/
    char		router[64];	    /* Route to this server	*/
    int			hops;		    /* Howmany hops away	*/
    time_t		connected;	    /* Connection time		*/
    char		prod[21];	    /* Product name		*/
    char		vers[21];	    /* Version string		*/
    char		fullname[37];	    /* Full BBS name		*/
    int			users;		    /* Users in chat		*/
} _srv_list;

_srv_list		srv_list[MAXIBC_SRV];



/*
 * Database with users
 */
typedef struct _usr_rec {
    char		server[64];	    /* FQDN of users server	*/
    char		name[10];	    /* Users unix name		*/
    char		nick[10];	    /* Users nick name		*/
    char		realname[37];	    /* Users real name		*/
    char		channel[21];	    /* Users channel		*/
    time_t		connected;	    /* Users connect time	*/
    unsigned		sysop	    : 1;    /* User is a sysop		*/
    pid_t		pid;		    /* Users pid if local	*/
    int			pointer;	    /* Users message pointer	*/
} _usr_list;

_usr_list		usr_list[MAXIBC_USR];



/*
 * Database with channels
 */
typedef struct _chn_rec {
    char		server[64];	    /* Originating server	*/
    char		name[21];	    /* Channel name		*/
    char		topic[55];	    /* Channel topic		*/
    char		owner[10];	    /* Channel owner		*/
    time_t		created;	    /* Channel created		*/
    time_t		lastmsg;	    /* Last message in channel	*/
    int			users;		    /* Users in channel		*/
} _chn_list;

_chn_list		chn_list[MAXIBC_CHN];



/*
 * Database with banned users (not yet in use)
 */
//typedef struct _ban_rec {
//    char		server[64];	    /* Users server             */
//    char		name[10];	    /* Users name		*/
//    char		channel[21];	    /* Users banned channel	*/
//    time_t		kicked;		    /* Users banned time	*/
//} _ban_list;



/*
 * Database with nicknames (not yet in use)
 */
//typedef struct _nick_rec {
//    char		server[64];	    /* Originating server       */
//    char		nick[10];	    /* Nickname			*/
//    time_t		lastused;   	    /* Last used time		*/
//} _nick_list;




int  add_user(char *, char *, char *);
void del_user(char *, char *);
int  add_channel(char *, char *, char *);
void del_channel(char *);
int  do_command(char *, char *, char *);

void send_all(char *);
void send_at(char *, char *, char *);
void send_nick(char *, char *, char *);
void check_servers(void);
void ibc_receiver(char *);
void ibc_init(void);
void ibc_shutdown(void);


#endif
