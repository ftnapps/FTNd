/* door.h */

#ifndef _DOOR_H
#define _DOOR_H

void ExtDoor(char *, int, int, int, int, int, int, char *);   /* Run external door */
int  exec_nosuid(char *);		    /* Execute as real user		*/

#endif
