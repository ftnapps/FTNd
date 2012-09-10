#ifndef _ROUTE_H
#define _ROUTE_H

/* m_route.h $ */

int  CountRoute(void);
int  OpenRoute(void);
void CloseRoute(int);
void EditRoute(void);
void InitRoute(void);
int  route_doc(FILE *, FILE *, int);

#endif
