#ifndef	_DBTIC_H
#define	_DBTIC_H


struct	_tichdr		tichdr;		/* Header record		    */
struct	_tic		tic;		/* Tics datarecord		    */
struct	_fgrouphdr	fgrouphdr;	/* Group header record		    */
struct	_fgroup		fgroup;		/* Group record			    */
int			tic_cnt;	/* Tic records in database	    */

int	InitTic(void);			/* Initialize tic database	    */
int	SearchTic(char *);		/* Search specified msg are	    */
int	TicSystemConnected(sysconnect);	/* Is system connected		    */
int	TicSystemConnect(sysconnect *, int); /* Connect/change/delete system*/
int	GetTicSystem(sysconnect *, int);/* Get connected system		    */
void	UpdateTic(void);		/* Update current messages record   */

#endif

