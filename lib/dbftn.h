#ifndef	_DBFTN_H
#define	_DBFTN_H


struct	_fidonethdr	fidonethdr;	/* Header record		    */
struct	_fidonet	fidonet;	/* Fidonet datarecord		    */
int			fidonet_cnt;	/* Fidonet records in database	    */
char			fidonet_fil[81];/* Fidonet database filename	    */

int	InitFidonet(void);		/* Initialize fidonet database	    */
int	TestFidonet(unsigned short);	/* Test if zone is in memory	    */
int	SearchFidonet(unsigned short);	/* Search specified zone and load   */


#endif

