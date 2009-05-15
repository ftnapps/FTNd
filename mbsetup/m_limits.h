#ifndef _LIMITSS_H
#define _LIMITSS_H

/* $Id: m_limits.h,v 1.3 2004/03/24 22:23:21 mbroek Exp $ */

int  CountLimits(void);
void EditLimits(void);
void InitLimits(void);
char *PickLimits(int);
char *get_limit_name(int);
int  bbs_limits_doc(FILE *, FILE *, int);

#endif
