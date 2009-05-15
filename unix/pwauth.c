/*****************************************************************************
 *
 * $Id: pwauth.c,v 1.4 2003/08/15 20:05:36 mbroek Exp $
 * Purpose ...............: MBSE BBS Shadow Password Suite
 * Original Source .......: Shadow Password Suite
 * Original Copyright ....: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2002
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
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include "mblogin.h"
#include "pwauth.h"
#include "getdef.h"
#include "encrypt.h"


/* standard password prompt by default */
static const char *PROMPT = gettext_noop("Password: ");
int wipe_clear_pass = 1;
char *clear_pass = NULL;



/*
 * pw_auth gets the user's cleartext password and encrypts it
 * using the salt in the encrypted password.  The results are
 * compared.
 */
int pw_auth(const char *cipher, const char *user, int reason, const char *input)
{
	char	prompt[1024];
	char	*clear = NULL;
	const char *cp;
	int	retval;

	/*
	 * There are programs for adding and deleting authentication data.
	 */
	if (reason == PW_ADD || reason == PW_DELETE)
		return 0;

	/*
	 * There are even programs for changing the user name ...
	 */
	if (reason == PW_CHANGE && input != (char *) 0)
		return 0;

	/*
	 * WARNING:
	 *
	 * When we change a password and we are root, we don't prompt.
	 * This is so root can change any password without having to
	 * know it.  This is a policy decision that might have to be
	 * revisited.
	 */
	if (reason == PW_CHANGE && getuid () == 0)
		return 0;

	/*
	 * WARNING:
	 *
	 * When we are logging in a user with no ciphertext password,
	 * we don't prompt for the password or anything.  In reality
	 * the user could just hit <ENTER>, so it doesn't really
	 * matter.
	 */
	if (cipher == (char *) 0 || *cipher == '\0')
		return 0;

	/*
	 * Prompt for the password as required.  FTPD and REXECD both
	 * get the cleartext password for us.
	 */
	if (reason != PW_FTP && reason != PW_REXEC && !input) {
		cp = PROMPT;
		snprintf(prompt, sizeof prompt, cp, user);
		clear = getpass(prompt);
		if (!clear) {
			static char c[1];
			c[0] = '\0';
			clear = c;
		}
		input = clear;
	}

	/*
	 * Convert the cleartext password into a ciphertext string.
	 * If the two match, the return value will be zero, which is
	 * SUCCESS.  Otherwise we see if SKEY is being used and check
	 * the results there as well.
	 */
	retval = strcmp(pw_encrypt(input, cipher), cipher);

	/*
	 * Things like RADIUS authentication may need the password -
	 * if the external variable wipe_clear_pass is zero, we will
	 * not wipe it (the caller should wipe clear_pass when it is
	 * no longer needed).  --marekm
	 */

	clear_pass = clear;
	if (wipe_clear_pass && clear && *clear)
		strzero(clear);
	return retval;
}


