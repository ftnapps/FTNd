#ifndef _DIRSESSION_H
#define _DIRSESSION_H

/* $Id$ */


int  islocked(char *, int, int);	/* Is directory locked	    */
int  setlock(char *, int);		/* Lock directory	    */
void remlock(char *, int);		/* Unlock directory	    */
void dirinbound(void);			/* Process nodes	    */

#endif
