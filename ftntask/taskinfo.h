#ifndef _TASKINFO_H
#define _TASKINFO_H

/* $Id: taskinfo.h,v 1.3 2006/01/30 22:27:03 mbse Exp $ */


void	get_sysinfo_r(char *);		    /* Get System Info		*/
void	get_lastcallercount_r(char *);	    /* Get Lastcallers count	*/
void	get_lastcallerrec_r(int, char *);   /* Get Lastcaller record	*/


#endif

