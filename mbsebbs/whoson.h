/* $Id: whoson.h,v 1.2 2003/10/11 21:22:16 mbroek Exp $ */

#ifndef _WHOSON_H
#define _WHOSON_H

void WhosOn(char *);			/* What users are currently online */
void WhosDoingWhat(int, char *);	/* Update what user is doing	   */
void SendOnlineMsg(char *);		/* Send On-Line Message to User	   */

#endif
