
#include "../config.h"
#include "mbselib.h"

#ifndef HAVE_STRCASESTR

char *strcasestr(char *a, char *b)
{
	char *p,*max; 
	int l;

        if (a && b) {

	l=strlen(b);
	max=a+strlen(a)-l;
	for (p=a;p<=max;p++)
		if (!strncasecmp(p,b,l)) return(p); 
	return((char *)0);
        }
        else {
           return ((char *) 0);
        }
}

#endif

