#ifndef	_MSGUTIL_H
#define	_MSGUTIL_H

/* msgutil.h */

char *rfcdate(time_t);			/* Create RFC style date	     */
int  Open_Msgbase(char *, int);		/* Open msgbase for read/write	     */
void Close_Msgbase(char *);		/* Close msgbase		     */
void Add_Headkludges(faddr *, int);	/* Header part of kludges	     */
void Add_Footkludges(int, char *, int);	/* Footer part of kludges	     */
void Sema_Mailout(void);		/* Set mailout semafore		     */


#endif

