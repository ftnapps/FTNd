/* $Id$ */

#ifndef _FUNCS_H
#define _FUNCS_H

int  Access(securityrec, securityrec);	    /* Check security access		*/
void UserList(char *);			    /* Get complete users list		*/
void TimeStats(void);			    /* Get users Time Statistics	*/
int  CheckFile(char *, int);		    /* Check for Dupe file in Database  */
void ViewTextFile(char *);		    /* View text file			*/
void LogEntry(char *);			    /* Create log entry in logfile	*/


#endif
