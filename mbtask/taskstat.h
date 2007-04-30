#ifndef _TASKSTAT_H
#define _TASKSTAT_H

/* $Id$ */


#define PAUSETIME               3
#define TOSSWAIT_TIME           30


void		status_init(void);		/* Initialize status module	*/
void		stat_inc_clients(void);		/* Increase connected clients	*/
void		stat_dec_clients(void);		/* Decrease connected clients	*/
void		stat_set_open(int);		/* Set BBS open status		*/
void		stat_inc_serr(void);		/* Increase syntax error	*/
void		stat_inc_cerr(void);		/* Increase comms error		*/
void		stat_status_r(char *);		/* Return status record		*/
int		stat_bbs_stat(void);		/* Get BBS open status		*/
void		getseq_r(char *);	    	/* Get next sequence number	*/
unsigned int	gettoken(void);			/* Get next sequence number	*/
int		get_zmh(void);			/* Check Zone Mail Hour		*/
int		sem_set(char *, int);		/* Set/Reset semafore		*/
void		sem_status_r(char *, char *);	/* Get semafore status		*/
void		sem_create_r(char *, char *);	/* Create semafore		*/
void		sem_remove_r(char *, char *);	/* Remove semafore		*/
void		mib_set_mailer(char *);		/* MIB set mailer data		*/
void		mib_set_netmail(char *);	/* MIB set netmail data		*/
void		mib_set_email(char *);		/* MIB set email data		*/
void		mib_set_news(char *);		/* MIB set news data		*/
void		mib_set_echo(char *);		/* MIB set echomail data	*/
void		mib_set_files(char *);		/* MIB set files data		*/

#endif

