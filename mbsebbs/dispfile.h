#ifndef _DISPLAYFILE_H
#define _DISPLAYFILE_H

/* $Id$ */


void DisplayRules(void);		    /* Display area rules		*/
int  DisplayTextFile(char *);		    /* Display a flat textfile		*/
int  DisplayFile(char *);		    /* Display .ans/.asc textfile	*/
int  DisplayFileEnter(char *);		    /* Display .ans/.asc wait for Enter */
void ControlCodeF(int);			    /* Check Control Codes in File	*/ 
void ControlCodeU(int);			    /* Check Control Codes in File	*/ 
void ControlCodeK(int);			    /* Check Control Codes in File	*/ 


#endif
