/* funcs.h */

#ifndef _FUNCS_H
#define _FUNCS_H

int  Access(securityrec, securityrec);	    /* Check security access		*/
void UserList(char *);			    /* Get complete users list		*/
void TimeStats(void);			    /* Get users Time Statistics	*/
void ExtDoor(char *, int, int, int, int);   /* Run external door		*/
int  exec_nosuid(char *);		    /* Execute as real user		*/
int  DisplayFile(char *);		    /* Display .ans/.asc textfile	*/
int  DisplayFileEnter(char *);		    /* Display .ans/.asc wait for Enter */
int  CheckFile(char *, int);		    /* Check for Dupe file in Database  */
void ControlCodeF(int);			    /* Check Control Codes in File	*/ 
void ControlCodeU(int);			    /* Check Control Codes in File	*/ 
void ControlCodeK(int);			    /* Check Control Codes in File	*/ 
void ViewTextFile(char *);		    /* View text file			*/
void LogEntry(char *);			    /* Create log entry in logfile	*/
void SwapDate(char *, char *);		    /* Swap two Date strings around	*/
int  TotalUsers(void);			    /* Returns total numbers of users   */


#endif

