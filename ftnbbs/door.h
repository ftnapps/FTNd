/* $Id: door.h,v 1.3 2003/10/11 21:22:16 mbroek Exp $ */

#ifndef _DOOR_H
#define _DOOR_H

void ExtDoor(char *, int, int, int, int, int, int, char *);   /* Run external door */
int  exec_nosuid(char *);		    /* Execute as real user		*/

#endif
