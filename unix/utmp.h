/* $Id: utmp.h,v 1.4 2004/12/28 15:30:53 mbse Exp $ */

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

#if defined(__linux__) /* XXX */
void setutmp(const char *, const char *, const char *);
#elif HAVE_UTMPX_H
void setutmp(const char *, const char *, const char *);
#elif __FreeBSD__ || __NetBSD__ || __OpenBSD__
void setutmp(const char *, const char *, const char *);
#else /* !SVR4 */
void setutmp(const char *, const char *);
#endif

#endif
