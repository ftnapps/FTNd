

#include "../config.h"

#ifndef	HAVE_GETUTENT

void setutent(void);
void endutent(void);
struct utmp *getutent(void);
struct utmp *getutline(const struct utmp *utent);

#endif
