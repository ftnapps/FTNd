#ifndef _TASKDISK_H
#define _TASKDISK_H

/* $Id: taskdisk.h,v 1.6 2006/02/13 19:26:31 mbse Exp $ */

char	*disk_reset(void);		/* Reset disk tables	    */
void	disk_check_r(char *, char *);	/* Check space in Megabytes */
void	disk_getfs_r(char *);		/* Get disk status	    */
void	diskwatch(void);		/* Diskwatch		    */
void	deinit_diskwatch(void);		/* Release memory	    */

#endif
