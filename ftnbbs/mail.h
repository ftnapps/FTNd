/* $Id: mail.h,v 1.6 2007/02/25 20:28:10 mbse Exp $ */

#ifndef _MAIL_H
#define _MAIL_H

#define	TEXTBUFSIZE	700


int  LC(int);				/* More prompt for reading messages */
int  Edit_Msg(void);                    /* Edit a message                   */
int  Ext_Edit(void);			/* External Message editor	    */
int  CheckLine(int, int, int);		/* Check linecounter for read       */
void SysopComment(char *);		/* Comment to Sysop		    */
void Post_Msg(void);			/* Post a message		    */
void Read_Msgs(void);			/* Read Messages		    */
void QuickScan_Msgs(void);		/* List Message Headers		    */
void Delete_Msg(void);			/* Delete a specified message	    */
void MsgArea_List(char *);		/* Select message area		    */
void CheckMail(void);			/* Check for new mail		    */
void MailStatus(void);			/* Mail status in areas		    */
void SetMsgArea(unsigned int);		/* Set message area and variables   */


#endif

