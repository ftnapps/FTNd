/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Mangle a unix name to DOS 8.3 filename
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************
 * Ideas taken from Samba, Copyright (C) Andrew Tridgell 1992-1998
 *****************************************************************************/

#include "libs.h"
#include "structs.h"
#include "clcomm.h"
#include "common.h"


/*
 * Prototype functions
 */
int		strhaslower(const char *);
int		str_checksum(const char *);
char		*safe_strcpy(char *, const char *, size_t);
static void	init_chartest(void);
static int	is_reserved_msdos(char *);
static int	is_illegal_name(char *);
int		is_8_3(char *);


#define PTR_DIFF(p1,p2) ((int)(((const char *)(p1)) - (const char *)(p2)))

/* -------------------------------------------------------------------------- **
 * Other stuff...
 *
 * magic_char     - This is the magic char used for mangling.  It's
 *                  global.  There is a call to lp_magicchar() in server.c
 *                  that is used to override the initial value.
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




/*****************************************************************************
 * Provide a checksum on a string
 *
 *  Input:  s - the null-terminated character string for which the checksum
 *              will be calculated.
 *
 *  Output: The checksum value calculated for s.
 *
 * ****************************************************************************
 */
int str_checksum(const char *s)
{
    int	res = 0;
    int	c;
    int	i=0;
        
    while(*s) {
	c = *s;
	res ^= (c << (i % 15)) ^ (c >> (15-(i%15)));
	s++;
	i++;
    }
    return res;
}



/*******************************************************************
safe string copy into a known length string. maxlength does not
include the terminating zero.
********************************************************************/

char *safe_strcpy(char *dest,const char *src, size_t maxlength)
{
    size_t len;

    if (!dest) {
        Syslog('f', "ERROR: NULL dest in safe_strcpy");
        return NULL;
    }

    if (!src) {
        *dest = 0;
        return dest;
    }  

    len = strlen(src);

    if (len > maxlength) {
            Syslog('f', "ERROR: string overflow by %d in safe_strcpy [%.50s]", (int)(len-maxlength), src);
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
 * Determine whether or not a given name contains illegal characters, even
 * long names.
 *
 *  Input:  name  - The name to be tested.
 *
 *  Output: True if an illegal character was found in <name>, else False.
 *
 *  Notes:  This is used to test a name on the host system, long or short,
 *          for characters that would be illegal on most client systems,
 *          particularly DOS and Windows systems.  Unix and AmigaOS, for
 *          example, allow a filenames which contain such oddities as
 *          quotes (").  If a name is found which does contain an illegal
 *          character, it is mangled even if it conforms to the 8.3
 *          format.
 *
 * ************************************************************************** **
 */
static int is_illegal_name(char *name)
{
    unsigned char *s;

    if (!name)
	return TRUE;

    if (!ct_initialized)
	init_chartest();

    s = (unsigned char *)name;
    while (*s) {
	if (isillegal(*s))
	    return TRUE;
	else
	    s++;
    }

    return FALSE;
}



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
    int   len;
    int   l;
    char *p;
    char *dot_pos;
    char *slash_pos = strrchr( fname, '/' );

    /* If there is a directory path, skip it. */
    if (slash_pos)
	fname = slash_pos + 1;
    len = strlen(fname);

//    Syslog('f', "Checking %s for 8.3", fname);

    /* Can't be 0 chars or longer than 12 chars */
    if( (len == 0) || (len > 12) )
	return FALSE;

    /* Mustn't be an MS-DOS Special file such as lpt1 or even lpt1.txt */
    if (is_reserved_msdos(fname))
	return FALSE;

    /* Check that all characters are the correct case, if asked to do so. */
//    if (strhaslower(fname))
//	return FALSE;

    /* Can't contain invalid dos chars */
    /* Windows use the ANSI charset.
       But filenames are translated in the PC charset.
       This Translation may be more or less relaxed depending
       the Windows application. */

    /* %%% A nice improvment to name mangling would be to translate
       filename to ANSI charset on the smb server host */

    p       = fname;
    dot_pos = NULL;
    while (*p) {
	if (*p == '.' && !dot_pos)
	    dot_pos = (char *)p;
//	else
//	    if (!isdoschar(*p))
//		return FALSE;
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

    /* see smb.conf(5) for a description of the 'strip dot' parameter. */
    /* strip_dot defaults to no */
    if (/* lp_strip_dot() && */ len - l == 1 && !strchr( dot_pos + 1, '.' )) {
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
    int		csum;
    char	*p;
    char	extension[4];
    char	base[9];
    int		baselen = 0;
    int		extlen = 0;

    extension[0] = 0;
    base[0] = 0;

    p = strrchr(s,'.');  
    if (p && (strlen(p+1) < (size_t)4)) {
	int	all_normal = (!strhaslower(p+1)); /* XXXXXXXXX */

	if (all_normal && p[1] != 0) {
	    *p = 0;
	    csum = str_checksum(s);
	    *p = '.';
	} else
	    csum = str_checksum(s);
    } else
	csum = str_checksum(s);

    tu(s);
//    Syslog('f', "Mangling name %s to ",s);

    if (p) {
	if( p == s )
	    safe_strcpy( extension, "___", 3 );
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

    while (*p && baselen < 5) {
	if (*p != '.' )
	    base[baselen++] = p[0];
	p++;
    }
    base[baselen] = 0;

    csum = csum % (MANGLE_BASE*MANGLE_BASE);
    sprintf(s, "%s%c%c%c", base, magic_char, mangle(csum/MANGLE_BASE), mangle(csum));

    if( *extension ) {
	(void)strcat(s, ".");
	(void)strcat(s, extension);
    }

//    Syslog('f', "%s", s);
}



/*****************************************************************************
 * Convert a filename to DOS format.  Return True if successful.
 *
 *  Input:  OutName - Source *and* destination buffer. 
 *
 *                    NOTE that OutName must point to a memory space that
 *                    is at least 13 bytes in size!
 *
 *          need83  - If False, name mangling will be skipped unless the
 *                    name contains illegal characters.  Mapping will still
 *                    be done, if appropriate.  This is probably used to
 *                    signal that a client does not require name mangling,
 *                    thus skipping the name mangling even on shares which
 *                    have name-mangling turned on.
 *
 *  Output: Returns False only if the name wanted mangling but the share does
 *          not have name mangling turned on.
 *
 * ****************************************************************************
 */
int name_mangle(char *OutName, int need83)
{
        Syslog('f', "name_mangle(%s, need83 = %s)", OutName, need83 ? "TRUE" : "FALSE");

	/*
	 * Check for characters legal in Unix and illegal in DOS/Win
	 */
        if (!need83 && is_illegal_name(OutName))
            need83 = TRUE;

        /*
	 * check if it's already in 8.3 format
	 */
        if (need83 && !is_8_3(OutName)) {
                mangle_name_83(OutName);
        } else {
	    /*
	     * No mangling needed, convert to uppercase
	     */
	    tu(OutName);
	}

        Syslog('f',"name_mangle() ==> [%s]", OutName);
        return TRUE;
}


