#ifndef _TASKSTAT_H
#define _TASKSTAT_H

void		status_init(void);	/* Initialize status module	*/
void		stat_inc_clients(void);	/* Increase connected clients	*/
void		stat_dec_clients(void);	/* Decrease connected clients	*/
void		stat_set_open(int);	/* Set BBS open status		*/
void		stat_inc_serr(void);	/* Increase syntax error	*/
void		stat_inc_cerr(void);	/* Increase comms error		*/
char		*stat_status(void);	/* Return status record		*/
int		stat_bbs_stat(void);	/* Get BBS open status		*/
char		*getseq(void);		/* Get next sequence number	*/
int		get_zmh(void);		/* Check Zone Mail Hour		*/
int		sem_set(char *, int);	/* Set/Reset semafore		*/
char		*sem_status(char *);	/* Get semafore status		*/
char		*sem_create(char *);	/* Create semafore		*/
char		*sem_remove(char *);	/* Remove semafore		*/

#endif

