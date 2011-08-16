/*****************************************************************************
 *
 * $Id: mangle.c,v 1.19 2007/03/03 14:28:40 mbse Exp $
 * Purpose ...............: Mangle a unix name to DOS 8.3 filename
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
 *   
 * Michiel Broek		FIDO:	2:280/2802
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
 *****************************************************************************
 * Ideas taken from Samba, Copyright (C) Andrew Tridgell 1992-1998
 *****************************************************************************/

#include "../config.h"
#include "mbselib.h"


/*
 * Prototype functions
 */
int		strhaslower(const char *);
char		*safe_strcpy(char *, const char *, size_t);
static void	init_chartest(void);
static int	is_reserved_msdos(char *);
int		is_8_3(char *);


#define PTR_DIFF(p1,p2) ((int)(((const char *)(p1)) - (const char *)(p2)))

/* -------------------------------------------------------------------------- **
 * Other stuff...
 *
 * magic_char     - This is the magic char used for mangling.  It's global.
 *
 * MANGLE_BASE    - This is the number of characters we use for name mangling.
 *
 * basechars      - The set characters used for name mangling.  This
 *                  is static (scope is this file only).
 *
 * mangle()       - Macro used to select a character from basechars (i.e.,
 *                  mangle(n) will return the nth digit, modulo MANGLE_BASE).
 *
 * chartest       - array 0..255.  The index range is the set of all possible
 *                  values of a byte.  For each byte value, the content is a
 *                  two nibble pair.  See BASECHAR_MASK and ILLEGAL_MASK,
 *                  below.
 *
 * ct_initialized - False until the chartest array has been initialized via
 *                  a call to init_chartest().
 *
 * BASECHAR_MASK  - Masks the upper nibble of a one-byte value.
 *
 * ILLEGAL_MASK   - Masks the lower nibble of a one-byte value.
 *
 * isillegal()    - Given a character, check the chartest array to see
 *                  if that character is in the illegal characters set.
 *                  This is faster than using strchr().
 *
 */

char magic_char = '~';

static char basechars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_-!@#$%";
#define MANGLE_BASE       (sizeof(basechars)/sizeof(char)-1)

static unsigned char chartest[256]  = { 0 };
static int           ct_initialized = FALSE;

#define mangle(V) ((char)(basechars[(V) % MANGLE_BASE]))
#define BASECHAR_MASK 0xf0
#define ILLEGAL_MASK  0x0f
#define isillegal(C) ( (chartest[ ((C) & 0xff) ]) & ILLEGAL_MASK )


/****************************************************************************
does a string have any lowercase chars in it?
****************************************************************************/
int strhaslower(const char *s)
{
    while (*s) {
	if (islower(*s))
	    return TRUE;
	s++;
    }
    return FALSE;
}




/*******************************************************************
safe string copy into a known length string. maxlength does not
include the terminating zero.
********************************************************************/

char *safe_strcpy(char *dest,const char *src, size_t maxlength)
{
    size_t len;

    if (!dest) {
        Syslog('+', "ERROR: NULL dest in safe_strcpy");
        return NULL;
    }

    if (!src) {
        *dest = 0;
        return dest;
    }  

    len = strlen(src);

    if (len > maxlength) {
            WriteError("ERROR: string overflow by %d in safe_strcpy [%.50s]", (int)(len-maxlength), src);
            len = maxlength;
    }

    memcpy(dest, src, len);
    dest[len] = 0;
    return dest;
}



/* ************************************************************************** **
 * Initialize the static character test array.
 *
 *  Input:  none
 *
 *  Output: none
 *
 *  Notes:  This function changes (loads) the contents of the <chartest>
 *          array.  The scope of <chartest> is this file.
 *
 * ************************************************************************** **
 */
static void init_chartest( void )
{
    char          *illegalchars = (char *)"*\\/?<>|\":";
    unsigned char *s;
  
    memset( (char *)chartest, '\0', 256 );

    for( s = (unsigned char *)illegalchars; *s; s++ )
	chartest[*s] = ILLEGAL_MASK;

    for( s = (unsigned char *)basechars; *s; s++ )
	chartest[*s] |= BASECHAR_MASK;

    ct_initialized = TRUE;
} /* init_chartest */


/* ************************************************************************** **
 * Return True if a name is a special msdos reserved name.
 *
 *  Input:  fname - String containing the name to be tested.
 *
 *  Output: True, if the name matches one of the list of reserved names.
 *
 *  Notes:  This is a static function called by is_8_3(), below.
 *
 * ************************************************************************** **
 */
static int is_reserved_msdos( char *fname )
{
    char upperFname[13];
    char *p;

    strncpy (upperFname, fname, 12);

    /* lpt1.txt and con.txt etc are also illegal */
    p = strchr(upperFname,'.');
    if (p)
	*p = '\0';

    tu(upperFname);
    p = upperFname + 1;
    switch (upperFname[0]) {
	case 'A':
		if( 0 == strcmp( p, "UX" ) )
		    return TRUE;
		break;
	case 'C':
		if ((0 == strcmp( p, "LOCK$" )) || (0 == strcmp( p, "ON" )) || (0 == strcmp( p, "OM1" ))
		    || (0 == strcmp( p, "OM2" )) || (0 == strcmp( p, "OM3" )) || (0 == strcmp( p, "OM4" )))
		    return TRUE;
		break;
	case 'L':
		if( (0 == strcmp( p, "PT1" )) || (0 == strcmp( p, "PT2" )) || (0 == strcmp( p, "PT3" )))
		    return TRUE;
		break;
	case 'N':
		if( 0 == strcmp( p, "UL" ) )
		    return TRUE;
		break;
	case 'P':
		if( 0 == strcmp( p, "RN" ) )
		    return TRUE;
		break;
    }

    return FALSE;
} /* is_reserved_msdos */




/* ************************************************************************** **
 * Return True if the name is a valid DOS name in 8.3 DOS format.
 *
 *  Input:  fname       - File name to be checked.
 *          check_case  - If True, then the
 *                        name will be checked to see if all characters
 *                        are the correct case.
 *
 *  Output: True if the name is a valid DOS name, else FALSE.
 *
 * ************************************************************************** **
 */
int is_8_3( char *fname)
{
    int		len;
    int		l, i;
    char	*p;
    char	*dot_pos;
    char	*slash_pos = strrchr( fname, '/' );

    /* If there is a directory path, skip it. */
    if (slash_pos)
	fname = slash_pos + 1;
    len = strlen(fname);

    /* Can't be 0 chars or longer than 12 chars */
    if ((len == 0) || (len > 12))
	return FALSE;

    /* Mustn't be an MS-DOS Special file such as lpt1 or even lpt1.txt */
    if (is_reserved_msdos(fname))
	return FALSE;

    init_chartest();
    for (i = 0; i < strlen(fname); i++) {
	if (isillegal(fname[i])) {
	    Syslog('+', "Illegal character in filename");
	    return FALSE;
	}
    }

    /* Can't contain invalid dos chars */
    p       = fname;
    dot_pos = NULL;
    while (*p) {
	if (*p == '.' && !dot_pos)
	    dot_pos = (char *)p;
	p++;
    }

    /* no dot and less than 9 means OK */
    if (!dot_pos)
	return (len <= 8);
        
    l = PTR_DIFF(dot_pos, fname);

    /* base must be at least 1 char except special cases . and .. */
    if (l == 0)
	return(0 == strcmp( fname, "." ) || 0 == strcmp( fname, ".." ));

    /* base can't be greater than 8 */
    if (l > 8)
	return FALSE;

    if (len - l == 1 && !strchr( dot_pos + 1, '.' )) {
	*dot_pos = 0;
	return TRUE;
    }

    /* extension must be between 1 and 3 */
    if ((len - l < 2 ) || (len - l > 4))
	return FALSE;

    /* extensions may not have a dot */
    if (strchr( dot_pos+1, '.' ))
	return FALSE;

    /* must be in 8.3 format */
    return TRUE;
}



/*****************************************************************************
 * do the actual mangling to 8.3 format
 * the buffer must be able to hold 13 characters (including the null)
 *****************************************************************************
 */
void mangle_name_83(char *s)
{
    int		crc16, i;
    char	*p, *q;
    char	extension[4];
    char	base[9];
    int		baselen = 0;
    int		extlen = 0;

    extension[0] = 0;
    base[0] = 0;

    /*
     * First, convert some common Unix extensions to extensions of 3
     * characters. If none fits, don't change anything now.
     */
    if (strcmp(q = s + strlen(s) - strlen(".tar.gz"), ".tar.gz") == 0) {
	*q = '\0';
	q = (char *)"tgz";
    } else if (strcmp(q = s + strlen(s) - strlen(".tar.z"), ".tar.z") == 0) {
	*q = '\0';
	q = (char *)"tgz";
    } else if (strcmp(q = s + strlen(s) - strlen(".tar.Z"), ".tar.Z") == 0) {
	*q = '\0';
	q = (char *)"taz";
    } else if (strcmp(q = s + strlen(s) - strlen(".html"), ".html") == 0) {
	*q = '\0';
	q = (char *)"htm";
    } else if (strcmp(q = s + strlen(s) - strlen(".shtml"), ".shtml") == 0) {
	*q = '\0';
	q = (char *)"stm";
    } else if (strcmp(q = s + strlen(s) - strlen(".conf"), ".conf") == 0) {
	*q = '\0';
	q = (char *)"cnf";
    } else if (strcmp(q = s + strlen(s) - strlen(".mpeg"), ".mpeg") == 0) {
	*q = '\0';
	q = (char *)"mpg";
    } else if (strcmp(q = s + strlen(s) - strlen(".smil"), ".smil") == 0) {
	*q = '\0';
	q = (char *)"smi";
    } else if (strcmp(q = s + strlen(s) - strlen(".perl"), ".perl") == 0) {
	*q = '\0';
	q = (char *)"pl";
    } else if (strcmp(q = s + strlen(s) - strlen(".jpeg"), ".jpeg") == 0) {
	*q = '\0';
	q = (char *)"jpg";
    } else if (strcmp(q = s + strlen(s) - strlen(".tiff"), ".tiff") == 0) {
	*q = '\0';
	q = (char *)"tif";
    } else {
	q = NULL;
    }
    
    if (q) {
	/*
	 * Extension is modified, apply changes
	 */
	p = s + strlen(s);
	*p++ = '.';
	for (i = 0; i < strlen(q); i++)
	    *p++ = q[i];
	*p++ = '\0';
    }

    /*
     * Now start name mangling
     */
    p = strrchr(s,'.');  
    if (p && (strlen(p+1) < (size_t)4)) {
	int	all_normal = (!strhaslower(p+1)); /* XXXXXXXXX */

	if (all_normal && p[1] != 0) {
	    *p = 0;
	    crc16 = crc16xmodem(s, strlen(s));
	    *p = '.';
	} else {
	    crc16 = crc16xmodem(s, strlen(s));
	}
    } else {
	crc16 = crc16xmodem(s, strlen(s));
    }

    tu(s);

    if (p) {
	if (p == s)
	    safe_strcpy(extension, "___", 3);
	else {
	    *p++ = 0;
	    while (*p && extlen < 3) {
		if (*p != '.' )
		    extension[extlen++] = p[0];
		p++;
	    }
	    extension[extlen] = 0;
	}
    }

    p = s;

    /*
     * Changed to baselen 4, original this is 5.
     * 24-11-2002 MB.
     */
    while (*p && baselen < 4) {
	if (*p != '.' )
	    base[baselen++] = p[0];
	p++;
    }
    base[baselen] = 0;

    if (crc16 > (MANGLE_BASE * MANGLE_BASE * MANGLE_BASE))
	Syslog('!', "WARNING: mangle_name_83() crc16 overflow");
    crc16 = crc16 % (MANGLE_BASE * MANGLE_BASE * MANGLE_BASE);
    snprintf(s, 9, "%s%c%c%c%c", base, magic_char, 
	    mangle(crc16 / (MANGLE_BASE * MANGLE_BASE)), mangle(crc16 / MANGLE_BASE), mangle(crc16));
    if ( *extension ) {
	(void)strcat(s, ".");
	(void)strcat(s, extension);
    }
}



/*****************************************************************************
 * Convert a filename to DOS format.
 *
 *  Input:  OutName - Source *and* destination buffer. 
 *
 *                    NOTE that OutName must point to a memory space that
 *                    is at least 13 bytes in size! That should always be
 *                    the case of course.
 *
 * ****************************************************************************
 */
void name_mangle(char *OutName)
{
    char    *p;

    p = xstrcpy(OutName);
    /*
     * check if it's already in 8.3 format
     */
    if (!is_8_3(OutName)) {
	mangle_name_83(OutName);
    } else {
	/*
	 * No mangling needed, convert to uppercase
	 */
	tu(OutName);
    }

    free(p);
}


