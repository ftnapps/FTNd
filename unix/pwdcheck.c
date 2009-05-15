/*****************************************************************************
 *
 * $Id: pwdcheck.c,v 1.2 2003/08/15 20:05:36 mbroek Exp $
 * Purpose ...............: MBSE BBS Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyrioght ...: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
 *   
 * Michiel Broek                FIDO:           2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include <stdio.h>
#include "mblogin.h"
#include <pwd.h>
#include <syslog.h>
#include "pwauth.h"
#include "pwdcheck.h"


#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#ifdef USE_PAM
// #include "pam_defs.h"
#endif

#define WRONGPWD2	"incorrect password for `%s'"

void passwd_check(const char *user, const char *passwd, const char *progname)
{
#ifdef USE_PAM
	pam_handle_t *pamh = NULL;
	int retcode;
	struct pam_conv conv = { misc_conv, NULL };

	if (pam_start(progname, user, &conv, &pamh)) {
bailout:
		SYSLOG((LOG_WARN, WRONGPWD2, user));
		sleep(1);
		fprintf(stderr, "Incorrect password for %s.\n", user);
		exit(1);
	}
	if (pam_authenticate(pamh, 0))
		goto bailout;

	retcode = pam_acct_mgmt(pamh, 0);
	if (retcode == PAM_NEW_AUTHTOK_REQD) {
		retcode = pam_chauthtok(pamh, PAM_CHANGE_EXPIRED_AUTHTOK);
	} else if (retcode)
		goto bailout;

	if (pam_setcred(pamh, 0))
		goto bailout;

	/* no need to establish a session; this isn't a session-oriented
	 * activity... */

#else /* !USE_PAM */

#ifdef SHADOW_PASSWORD
	struct spwd *sp;

	if ((sp = getspnam(user)))
		passwd = sp->sp_pwdp;
	endspent();
#endif
	if (pw_auth(passwd, user, PW_LOGIN, (char *) 0) != 0) {
		syslog(LOG_WARNING, WRONGPWD2, user);
		sleep(1);
		fprintf(stderr, "Incorrect password for %s.\n", user);
		exit(1);
	}
#endif /* !USE_PAM */
}

