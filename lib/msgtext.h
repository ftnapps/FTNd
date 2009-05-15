#ifndef _MSGTEXT_H
#define _MSGTEXT_H

typedef struct _lData {
   struct _lData	*Previous;
   struct _lData	*Next;
   void *		Value;
   char			Data[1];
} LDATA;



unsigned short	Elements;

unsigned short	MsgText_Add1(void * lpData);
unsigned short	MsgText_Add2(char * lpData);
unsigned short	MsgText_Add3(void * lpData, unsigned short usSize);
void  		MsgText_Clear(void);
void *  	MsgText_First(void);
unsigned short	MsgText_Insert1(void * lpData);
unsigned short	MsgText_Insert2(char * lpData);
unsigned short	MsgText_Insert3(void * lpData, unsigned short usSize);
void *  	MsgText_Last(void);
void *  	MsgText_Next(void);
void *  	MsgText_Previous(void);
void   		MsgText_Remove(void);
unsigned short	MsgText_Replace1(void * lpData);
unsigned short	MsgText_Replace2(char * lpData);
unsigned short	MsgText_Replace3(void * lpData, unsigned short usSize);
void *		MsgText_Value(void);


#endif

