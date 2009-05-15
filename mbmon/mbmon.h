#ifndef _MBMON_H
#define	_MBMON_H

/* $Id: mbmon.h,v 1.3 2003/03/24 19:44:39 mbroek Exp $ */

static	void die(int);
void	ShowSysinfo(void);
void	ShowLastcaller(void);
void	system_moni(void);
void	system_stat(void);
void	disk_stat(void);
void	soft_info(void);
void	Chat(void);

#endif
