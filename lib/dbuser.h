#ifndef	_DBUSER_H
#define	_DBUSER_H


struct	userhdr		usrhdr;		/* Header record		    */
struct	userrec		usr;		/* User datarecord		    */
int			usr_cnt;	/* User records in database	    */
char			usr_fil[81];	/* User database filename	    */

int	InitUser(void);			/* Initialize user database	    */
int	TestUser(char *);		/* Test if user is in memory	    */
int	SearchUser(char *);		/* Search specified user and load   */


#endif

