#ifndef _DISPLAYFILE_H
#define _DISPLAYFILE_H

/* $Id: dispfile.h,v 1.3 2007/02/25 20:28:09 mbse Exp $ */


void DisplayRules(void);		    /* Display area rules		*/
int  DisplayTextFile(char *);		    /* Display a flat textfile		*/
int  DisplayFile(char *);		    /* Display .ans/.asc textfile	*/
int  DisplayFileEnter(char *);		    /* Display .ans/.asc wait for Enter */
char *ControlCodeF(int);		    /* Check Control Codes in File	*/ 
char *ControlCodeU(int);		    /* Check Control Codes in File	*/ 
char *ControlCodeK(int);		    /* Check Control Codes in File	*/ 


#endif
