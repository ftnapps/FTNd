#ifndef _LIMITSS_H
#define _LIMITSS_H

/* $Id$ */

int  CountLimits(void);
void EditLimits(void);
void InitLimits(void);
char *PickLimits(int);
char *get_limit_name(int);
int  bbs_limits_doc(FILE *, FILE *, int);

#endif
