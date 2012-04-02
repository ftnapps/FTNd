#ifndef _GLOBAL_H
#define _GLOBAL_H

/* $Id: m_global.h,v 1.2 2004/03/24 22:23:21 mbroek Exp $ */

void config_check(char *path);
int  config_open(void);
void config_close(void);
int  config_read(void);
int  config_write(void);
void global_menu(void);
int  PickAka(char *, int);
void web_secflags(FILE *, char *, securityrec);
int  global_doc(FILE *, FILE *, int);

#endif
