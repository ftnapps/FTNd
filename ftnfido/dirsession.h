#ifndef _DIRSESSION_H
#define _DIRSESSION_H

/* dirsession.h */


int  islocked(char *, int, int, int);	/* Is directory locked	    */
int  setlock(char *, int, int);		/* Lock directory	    */
void remlock(char *, int, int);		/* Unlock directory	    */
int  dirinbound(void);			/* Process nodes	    */

#endif
