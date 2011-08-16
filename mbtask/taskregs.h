#ifndef _TASKREGS_H
#define _TASKREGS_H

/* $Id: taskregs.h,v 1.10 2006/05/22 12:09:15 mbse Exp $ */


#define MAXCLIENT       100


/*
 *  Connected clients information
 */
#define RB      5

typedef struct _reg_info {
        pid_t           pid;                    /* Pid or zero if free  */
        char            tty[7];                 /* Connected tty        */
        char            uname[36];              /* User name            */
        char            prg[15];                /* Program name         */
        char            city[36];               /* Users city           */
        char            doing[36];              /* What is going on     */
        int		started;                /* Startime connection  */
        int		lastcon;                /* Last connection      */
        int             altime;                 /* Alarm time           */
        unsigned        silent          : 1;    /* Do not disturb       */
        unsigned        ismsg           : 1;    /* Message waiting      */
	unsigned	istcp		: 1;	/* Is a TCP/IP session	*/
	unsigned	paging		: 1;	/* Is paging sysop	*/
	unsigned	haspaged	: 1;	/* Has paged sysop	*/
        int             ptr_in;                 /* Input buffer pointer */
        int             ptr_out;                /* Output buffer ptr    */
        char            fname[RB][36];          /* Message from user    */
        char            msg[RB][81];            /* The message itself   */
	char		reason[81];		/* Chat reason		*/
} reg_info;



int	reg_newcon(char *);
int	reg_closecon(char *);
void	reg_check(void);
int	reg_doing(char *);
int	reg_ip(char *);
int	reg_nop(char *);
int	reg_timer(int, char *);
int	reg_tty(char *);
int	reg_user(char *);
int	reg_silent(char *);		    /* Set/Reset do not disturb	    */
void	reg_ipm_r(char *, char *);	    /* Check for personal message   */
int	reg_spm(char *);		    /* Send personal message	    */
void	reg_fre_r(char *);		    /* Check if system is free	    */
void	get_reginfo_r(int, char *);	    /* Get registration info	    */
int	reg_sysop(char *);		    /* Registrate sysop presence    */
int	reg_page(char *);		    /* Page sysop for chat	    */
int	reg_cancel(char *);		    /* Cancel sysop page	    */
void	reg_checkpage_r(char *, char *);    /* Check paging status	    */
int	reg_ispaging(pid_t);		    /* Check if user with pid paged */
void	reg_sysoptalk(pid_t);		    /* Is now talking to the sysop  */

#endif

