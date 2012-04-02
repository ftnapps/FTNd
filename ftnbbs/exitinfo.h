/* $Id: exitinfo.h,v 1.3 2001/11/11 12:07:39 mbroek Exp $ */

#ifndef _EXITINFO_H
#define _EXITINFO_H

int  InitExitinfo(void);		/* Create exitinfo		   */
void ReadExitinfo(void);		/* Read Users Config in Memory     */
void WriteExitinfo(void);		/* Write Users config from memory  */

#endif
