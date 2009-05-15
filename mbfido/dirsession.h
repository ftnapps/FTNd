#ifndef _DIRSESSION_H
#define _DIRSESSION_H

/* $Id: dirsession.h,v 1.4 2002/11/11 12:56:19 mbroek Exp $ */


int  islocked(char *, int, int, int);	/* Is directory locked	    */
int  setlock(char *, int, int);		/* Lock directory	    */
void remlock(char *, int, int);		/* Unlock directory	    */
int  dirinbound(void);			/* Process nodes	    */

#endif
