/* $Id: pwauth.h,v 1.1 2002/01/05 13:57:10 mbroek Exp $ */

#ifndef _PWAUTH_H
#define	_PWAUTH_H

int pw_auth(const char *program,const char *user,int flag,const char *input);


/*
 * Local access
 */
#define	PW_SU		1
#define	PW_LOGIN	2

/*
 * Administrative functions
 */
#define	PW_ADD		101
#define	PW_CHANGE	102
#define	PW_DELETE	103

/*
 * Network access
 */
#define	PW_TELNET	201
#define	PW_RLOGIN	202
#define	PW_FTP		203
#define	PW_REXEC	204

#endif

