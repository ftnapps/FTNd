/* $Id$ */

#ifndef	_UTMP_HH
#define	_UTMP_HH


void checkutmp(int);

#ifndef HAVE_UPDWTMP
void updwtmp(const char *, const struct utmp *);
#endif  /* ! HAVE_UPDWTMP */

#ifdef HAVE_UTMPX_H
#ifndef HAVE_UPDWTMPX
static void updwtmpx(const char *, const struct utmpx *);
#endif  /* ! HAVE_UPDWTMPX */
#endif  /* ! HAVE_UTMPX_H */

void setutmp(const char *, const char *, const char *);

#endif
