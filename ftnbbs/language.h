#ifndef _LANGUAGE_H
#define _LANGUAGE_H


void language(int, int, int);	/* Print language text                      */
int  Keystroke(int, int);	/* Return keystroke			    */
void InitLanguage(void);	/* Initialize Language                      */
char *Language(int);		/* Return line for output                   */
void Set_Language(int);		/* Set Language Variables Up                */
void Free_Language(void);	/* Free language memory			    */

#endif

