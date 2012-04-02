#ifndef	_MBAUTH_H
#define	_MBAUTH_H

/* $Id: auth.h,v 1.2 2004/06/20 14:38:11 mbse Exp $ */

#ifndef	USE_NEWSGATE

int  check_auth(char *);	/* Check user is authorized	*/
void auth_user(char *);		/* Auth username		*/
void auth_pass(char *);		/* Auth password		*/

#endif

#endif
