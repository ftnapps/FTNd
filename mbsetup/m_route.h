#ifndef _ROUTE_H
#define _ROUTE_H

/* $Id: m_route.h,v 1.1 2002/07/17 17:24:49 mbroek Exp $ */

int  CountRoute(void);
int  OpenRoute(void);
void CloseRoute(int);
void EditRoute(void);
void InitRoute(void);
int  route_doc(FILE *, FILE *, int);

#endif
