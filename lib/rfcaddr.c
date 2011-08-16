/*****************************************************************************
 *
 * $Id: rfcaddr.c,v 1.7 2004/02/21 14:24:04 mbroek Exp $
 * Purpose ...............: MBSE BBS Common Library - RFC address functions
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
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

#include "../config.h"
#include "mbselib.h"


extern int addrerror;

static char *errname[] = {
	(char *)"nested <>",
	(char *)"multiple <>",
	(char *)"unmatched <>""()",
	(char *)"badtoken",
	(char *)"badstructure",
};



char *addrerrstr(int err)
{
	int		i;
	static char	buf[128];

	buf[0] = '\0';
	for (i = 0; i < ADDR_ERRMAX; i++)
		if (err & (1 << i)) {
			if (buf[0])
				strcat(buf,",");
			strcat(buf, errname[i]);
		}
	if (buf[0] == '\0') 
		strcpy(buf,"none");
	return buf;
}



void tidyrfcaddr(parsedaddr addr)
{
	if (addr.target) 
		free(addr.target);
	if (addr.remainder) 
		free(addr.remainder);
	if (addr.comment) 
		free(addr.comment);
}



parsedaddr parserfcaddr(char *s)
{
	parsedaddr	result;
	char		*inbrackets = NULL, *outbrackets = NULL, *cleanbuf = NULL, *combuf = NULL;
	char		*t, *r, *c, *p, *q, **x;
	int		quotes, brackets, escaped, anglecomplete;
	char		*firstat, *lastat, *percent, *colon, *comma, *exclam;

	result.target    = NULL;
	result.remainder = NULL;
	result.comment   = NULL;
	addrerror = 0;

	if ((s == NULL) || (*s == '\0')) 
		return result;

	/*
	 *  First check if there is an "angled" portion 
	 */
	inbrackets  = calloc(strlen(s)+1, sizeof(char));
	outbrackets = calloc(strlen(s)+1, sizeof(char));
	brackets = quotes = escaped = anglecomplete = 0;
	for (p = s,q = inbrackets, r = outbrackets, x = &r; *p; p++) {
		if (escaped) 
			escaped = FALSE;
		else /* process all special chars */
		switch (*p) {
		case '\\':	escaped = TRUE; break;
		case '\"':	quotes = !quotes; break;
		case '<':	if (quotes) 
					break;
				if (brackets) 
					addrerror |= ADDR_NESTED;
				if (anglecomplete) 
					addrerror |= ADDR_MULTIPLE;
				brackets++;
				x = &q;
				break;
		case '>':	if (quotes) 
					break;
				if (brackets) 
					brackets--;
				else 
					addrerror |= ADDR_UNMATCHED;
				if (!brackets) 
					anglecomplete = 1;
				break;
		}
		*((*x)++) = *p;
		if (!brackets) 
			x = &r;
	}
	*q = '\0';
	*r = '\0';
	if (brackets || quotes) 
		addrerror |= ADDR_UNMATCHED;

	if (addrerror) 
		goto leave1;

	cleanbuf = calloc(strlen(s)+1, sizeof(char));
	combuf = calloc(strlen(s)+1, sizeof(char));
	if (*inbrackets) { /* there actually is an angled portion */
		strcpy(combuf, outbrackets);
		c = combuf + strlen(combuf);
		p = inbrackets + 1;
		*(p+strlen(p)-1) = '\0';
	} else {
		c = combuf;
		p = outbrackets;
	}


	/* OK, now we have result.comment filled with wat was outside
	   angle brackets, c pointing past the end of it,
	   p pointing to what is supposed to be address, with angle
	   brackets already removed */
	quotes = brackets = escaped = 0;
	for (r = cleanbuf, x = &r; *p; p++) {
		if (escaped) {
			escaped=0;
			*((*x)++)=*p;
		} else /* process all special chars */
		if (isspace(*p)) {
			if ((quotes) || (brackets)) 
				*((*x)++) = *p;
		} else
		switch (*p) {
		case '\\':	escaped=1;
				/* pass backslash itself only inside quotes
				   and comments, or for the special cases
				   \" and \\  otherwise eat it away */
				if ((quotes) || (brackets)) 
					*((*x)++) = *p;
				else if ((*(p+1)=='"') || (*(p+1)=='\\')) 
					*((*x)++) = *p;
				break;
		case '\"':	quotes = !quotes;
				*((*x)++) = *p;
				break;
		case '(':
				brackets++;
				x = &c;
				break;
		case ')':
				if (brackets) 
					brackets--;
				else 
					addrerror |= ADDR_UNMATCHED;
				if (!brackets) 
					x = &r;
				break;
		default:
				*((*x)++) = *p;
				break;
		}
	}
	*r = '\0';
	*c = '\0';
	if (brackets || quotes) 
		addrerror |= ADDR_UNMATCHED;

	if (addrerror) 
		goto leave2;

	/* OK, now we have inangles buffer filled with the 'clean' address,
	   all comments removed, and result.comment is ready filled */

	/* seach for special chars that are outside quotes */

	firstat = lastat = percent = colon = comma = exclam = NULL;
	quotes = 0; escaped = 0;
	for (p = cleanbuf; *p; p++)
		if (*p == '\\') 
			p++; 
		else if (*p == '\"') 
			quotes = !quotes;
		else if (!quotes)
			switch (*p) {
			case '@':
					if (!firstat) 
						firstat = p;
					lastat = p;
					break;
			case '%':
					percent = p;
					break;
			case ':':
					colon = p;
					break;
			case ',':
					comma = p;
					break;
			case '!':
					if (!exclam) 
						exclam = p;
					break;
			}
	if ((firstat == cleanbuf) && colon) {
		if (comma && (comma < colon)) {
			*comma = '\0';
			r = comma + 1;
		} else {
			*colon = '\0';
			r = colon + 1;
		}
		t = firstat + 1;
	} else if (lastat) {
		*lastat = '\0';
		r = cleanbuf;
		t = lastat + 1;
	} else if (exclam) {
		*exclam = '\0';
		r = exclam + 1;
		t = cleanbuf;
	} else if (percent) {
		*percent = '\0';
		r = cleanbuf;
		t = percent + 1;
	} else {
		/* unquote it if necessary */
		if ((*cleanbuf == '\"') && (*(p = (cleanbuf+strlen(cleanbuf)-1)) == '\"')) {
			*p = '\0';
			r = cleanbuf + 1;
		} else 
			r = cleanbuf;
		t = NULL;
	}
	if (t && (*t != '\0')) 
		result.target = xstrcpy(t);
	if (r && (*r != '\0')) 
		result.remainder = xstrcpy(r);
	if (*combuf != '\0') 
		result.comment = xstrcpy(combuf);

leave1: /* this is also normal exit */
	free(cleanbuf);
	free(combuf);
	free(inbrackets);
	free(outbrackets);
	return result;

leave2: /* if error found on second stage, free */
	free(cleanbuf);
	free(combuf);
	return result;
}

