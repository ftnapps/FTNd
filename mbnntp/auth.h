#ifndef	_MBAUTH_H
#define	_MBAUTH_H

/* $Id$ */

#ifndef	USE_NEWSGATE

int  check_auth(char *);	/* Check user is authorized	*/
void auth_user(char *);		/* Auth username		*/
void auth_pass(char *);		/* Auth password		*/

#endif

#endif
