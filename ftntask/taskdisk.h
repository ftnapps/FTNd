#ifndef _TASKDISK_H
#define _TASKDISK_H

/* taskdisk.h  */

char	*disk_reset(void);		/* Reset disk tables	    */
void	disk_check_r(char *, char *);	/* Check space in Megabytes */
void	disk_getfs_r(char *);		/* Get disk status	    */
void	diskwatch(void);		/* Diskwatch		    */
void	deinit_diskwatch(void);		/* Release memory	    */

#endif
