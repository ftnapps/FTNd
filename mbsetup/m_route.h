#ifndef _ROUTE_H
#define _ROUTE_H

/* $Id$ */

int  CountRoute(void);
int  OpenRoute(void);
void CloseRoute(int);
void EditRoute(void);
void InitRoute(void);
int  route_doc(FILE *, FILE *, int);

#endif
