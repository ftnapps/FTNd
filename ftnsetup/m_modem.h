#ifndef _MODEM_H
#define _MODEM_H


int  CountModem(void);
void EditModem(void);
void InitModem(void);
char *PickModem(char *);
int  modem_doc(FILE *, FILE *, int);

#endif

