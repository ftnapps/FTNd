#ifndef _EXITINFO_H
#define _EXITINFO_H

void InitExitinfo(void);		/* Create exitinfo		   */
void ReadExitinfo(void);		/* Read Users Config in Memory     */
void WriteExitinfo(void);		/* Write Users config from memory  */
void WhosOn(char *);			/* What users are currently online */
void WhosDoingWhat(int);		/* Update what user is doing	   */
void SendOnlineMsg(char *);		/* Send On-Line Message to User	   */


#endif

