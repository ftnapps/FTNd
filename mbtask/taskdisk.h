#ifndef _TASKDISK_H
#define _TASKDISK_H

/* $Id$ */

char		*disk_reset(void);	/* Reset disk tables		*/
char		*disk_free(void);	/* Enough free space		*/
char		*disk_getfs(void);	/* Get disk status		*/
void		*disk_thread(void);	/* Disk watch thread		*/

#endif
