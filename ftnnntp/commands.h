#ifndef	_COMMANDS_H
#define	_COMMANDS_H

/* $Id: commands.h,v 1.5 2005/02/14 19:57:19 mbse Exp $ */

#ifndef	USE_NEWSGATE

char *getrfcchrs(int);
char *make_msgid(char *);
void command_abhs(char *);	    /* ARTICLE/BODY/HEADER/STAT	*/
void command_group(char *);	    /* GROUP			*/
void command_list(char *);	    /* LIST			*/
void command_post(char *);	    /* POST			*/
void command_xover(char *);	    /* XOVER			*/

#endif

#endif
