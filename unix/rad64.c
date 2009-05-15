/*****************************************************************************
 *
 * File ..................: mbuseradd/rad64c.c
 * Purpose ...............: MBSE BBS Shadow Password Suite
 * Last modification date : 25-Sep-2000
 * Original Source .......: Shadow Password Suite
 * Original Copyrioght ...: Julianne Frances Haugh and others.
 *
 *****************************************************************************
 * Copyright (C) 1997-2000
 *   
 * Michiel Broek		FIDO:		2:280/2802
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

/*
 * Copyright 1989 - 1992, Julianne Frances Haugh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "../config.h"
#include "rad64.h"


#ifndef HAVE_C64I
/*
 * c64i - convert a radix 64 character to an integer
 */
int c64i(char c)
{
	if (c == '.')
		return (0);

	if (c == '/')
		return (1);

	if (c >= '0' && c <= '9')
		return (c - '0' + 2);

	if (c >= 'A' && c <= 'Z')
		return (c - 'A' + 12);

	if (c >= 'a' && c <= 'z')
		return (c - 'a' + 38);
	else
		return (-1);
}



/*
 * i64c - convert an integer to a radix 64 character
 */
int i64c(int i)
{
	if (i <= 0)
		return ('.');

	if (i == 1)
		return ('/');

	if (i >= 2 && i < 12)
		return ('0' - 2 + i);

	if (i >= 12 && i < 38)
		return ('A' - 12 + i);

	if (i >= 38 && i < 63)
		return ('a' - 38 + i);

	return ('z');
}
#endif


#ifndef HAVE_A64L
/*
 * l64a - convert a long to a string of radix 64 characters
 */
char *l64a(long l)
{
	static	char	buf[8];
	int	i = 0;

	if (l < 0L)
		return ((char *) 0);

	do {
		buf[i++] = i64c ((int) (l % 64));
		buf[i] = '\0';
	} while (l /= 64L, l > 0 && i < 6);

	return (buf);
}



/*
 * a64l - convert a radix 64 string to a long integer
 */
long a64l(const char *s)
{
	int	i;
	long	value;
	long	shift = 0;

	for (i = 0, value = 0L;i < 6 && *s;s++) {
		value += (c64i (*s) << shift);
		shift += 6;
	}
	return (value);
}

#endif /* !HAVE_A64L */

