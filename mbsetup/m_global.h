/* m_global.h */

#ifndef _GLOBAL_H
#define _GLOBAL_H

void config_check(char *path);
int  config_open(void);
void config_close(void);
int  config_read(void);
int  config_write(void);
void global_menu(void);
int  PickAka(char *, int);
int  global_doc(FILE *, FILE *, int);

#endif

