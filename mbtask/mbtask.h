#ifndef _MBTASK_H
#define	_MBTASK_H

/* $Id: mbtask.h,v 1.12 2006/02/13 19:26:30 mbse Exp $ */


/*
 *  Defines. 
 *  SLOWRUN is number of seconds for scheduling mailer calls. Leave at 20!
 */
#define MAXTASKS                10
#define SLOWRUN                 20



/*
 *  Running tasks information
 */
typedef struct _onetask {
	char		name[16];		/* Name of the task	*/
	char		cmd[PATH_MAX];		/* Command to binary	*/
	char		opts[128];		/* Commandline opts	*/
	int		tasktype;		/* Type of task		*/
	pid_t		pid;			/* Pid of task		*/
	int		running;		/* Running or not	*/
	int		status;			/* Waitpid status	*/
	int		rc;			/* Exit code		*/
} onetask;



/*
 * Logging flagbits, ' ' ? ! + -
 */
#define DLOG_ALLWAYS    0x00000001
#define DLOG_ERROR      0x00000002
#define DLOG_ATTENT     0x00000004
#define DLOG_NORMAL     0x00000008
#define DLOG_VERBOSE    0x00000010



time_t		file_time(char *);
void		load_maincfg(void);
void		load_taskcfg(void);
pid_t		launch(char *, char *, char *, int);
int		runtasktype(int);
int		checktasks(int);
void		die(int);
void		start_scheduler(int);
void		scheduler(void);
int		locktask(char *);
void		ulocktask(void);

#endif
