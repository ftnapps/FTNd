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
int strequal(const char *, const char *);
int strhasupper(const char *);
int strhaslower(const char *);
int strisnormal(char *);
int str_checksum(const char *);
char *safe_strcpy(char *, const char *, size_t);
static void init_chartest(void);
static int is_reserved_msdos(char *);
static int is_illegal_name(char *);
int is_mangled(char *);
int is_8_3( char *, int);
static char *map_filename(char *, char *, int);
static void do_fwd_mangled_map(char *, char *);
void mangle_name_83( char *);
int name_map_mangle(char *, int, int, int);


#define pstrcpy(d,s) safe_strcpy((d),(s),sizeof(pstring)-1)
#define pstrcat(d,s) safe_strcat((d),(s),sizeof(pstring)-1)
#define PTR_DIFF(p1,p2) ((int)(((const char *)(p1)) - (const char *)(p2)))

#define isdoschar(c) (dos_char_map[(c&0xff)] != 0)

typedef enum {CASE_UPPER, CASE_LOWER} CASES;

int	case_default = CASE_UPPER;	/* Are conforming 8.3 names all upper or lower?   */
int	case_mangle = TRUE;    		/* If true, all chars in 8.3 should be same case. */

#define PSTRING_LEN 1024
typedef char pstring[PSTRING_LEN];


/*******************************************************************
  compare 2 strings 
********************************************************************/
int strequal(const char *s1, const char *s2)
{
    if (s1 == s2) 
	return(TRUE);
    if (!s1 || !s2) 
	return(FALSE);
  
    return(strcasecmp(s1,s2)==0);
}


/****************************************************************************
does a string have any uppercase chars in it?
****************************************************************************/
int strhasupper(const char *s)
{
    while (*s) {
	if (isupper(*s))
	    return TRUE;
	s++;
    }
    return FALSE;
}


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
 * check if a string is in "normal" case
 * ********************************************************************/
int strisnormal(char *s)
{
    if (case_default == CASE_UPPER)
	return(!strhaslower(s));

    return(!strhasupper(s));
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
    int res = 0;
    int c;
    int i=0;
        
    while(*s) {
	c = *s;
	res ^= (c << (i % 15)) ^ (c >> (15-(i%15)));
	s++;
	i++;
    }
    return res;
} /* str_checksum */

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
 * isbasecahr()   - Given a character, check the chartest array to see
 *                  if that character is in the basechars set.  This is
 *                  faster than using strchr().
 *
 * isillegal()    - Given a character, check the chartest array to see
 *                  if that character is in the illegal characters set.
 *                  This is faster than using strchr().
 *
 * mangled_cache  - Cache header used for storing mangled -> original
 *                  reverse maps.
 *
 * mc_initialized - False until the mangled_cache structure has been
 *                  initialized via a call to reset_mangled_cache().
 *
 * MANGLED_CACHE_MAX_ENTRIES - Default maximum number of entries for the
 *                  cache.  A value of 0 indicates "infinite".
 *
 * MANGLED_CACHE_MAX_MEMORY  - Default maximum amount of memory for the
 *                  cache.  When the cache was kept as an array of 256
 *                  byte strings, the default cache size was 50 entries.
 *                  This required a fixed 12.5Kbytes of memory.  The
 *                  mangled stack parameter is no longer used (though
 *                  this might change).  We're now using a fixed 16Kbyte
 *                  maximum cache size.  This will probably be much more
 *                  than 50 entries.
 */

char magic_char = '~';

static char basechars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_-!@#$%";
#define MANGLE_BASE       (sizeof(basechars)/sizeof(char)-1)

static unsigned char chartest[256]  = { 0 };
static int	     ct_initialized = FALSE;

#define mangle(V) ((char)(basechars[(V) % MANGLE_BASE]))
#define BASECHAR_MASK 0xf0
#define ILLEGAL_MASK  0x0f
#define isbasechar(C) ( (chartest[ ((C) & 0xff) ]) & BASECHAR_MASK )
#define isillegal(C) ( (chartest[ ((C) & 0xff) ]) & ILLEGAL_MASK )

//static ubi_cacheRoot mangled_cache[1] =  { { { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0 } };
//static int           mc_initialized   = FALSE;
//#define MANGLED_CACHE_MAX_ENTRIES 0
//#define MANGLED_CACHE_MAX_MEMORY  16384

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
} /* is_illegal_name */


/* ************************************************************************** **
 * Return True if the name *could be* a mangled name.
 *
 *  Input:  s - A path name - in UNIX pathname format.
 *
 *  Output: True if the name matches the pattern described below in the
 *          notes, else False.
 *
 *  Notes:  The input name is *not* tested for 8.3 compliance.  This must be
 *          done separately.  This function returns true if the name contains
 *          a magic character followed by excactly two characters from the
 *          basechars list (above), which in turn are followed either by the
 *          nul (end of string) byte or a dot (extension) or by a '/' (end of
 *          a directory name).
 *
 * ************************************************************************** **
 */
int is_mangled(char *s)
{
    char *magic;

    if (!ct_initialized)
	init_chartest();

    magic = strchr(s, magic_char);
    while (magic && magic[1] && magic[2]) {			/* 3 chars, 1st is magic.	  */
	if( ('.' == magic[3] || '/' == magic[3] || !(magic[3]))	/* Ends with '.' or nul or '/' ?  */
	    && isbasechar(toupper(magic[1]))			/* is 2nd char basechar?	  */
	    && isbasechar(toupper(magic[2])))			/* is 3rd char basechar?	  */
	    return TRUE;					/* If all above, then true,	  */
	magic = strchr(magic+1, magic_char);			/* else seek next magic.	  */
    }
    return FALSE;
} /* is_mangled */


/* ************************************************************************** **
 * Return True if the name is a valid DOS name in 8.3 DOS format.
 *
 *  Input:  fname       - File name to be checked.
 *          check_case  - If True, and if case_mangle is True, then the
 *                        name will be checked to see if all characters
 *                        are the correct case.  See case_mangle and
 *                        case_default above.
 *
 *  Output: True if the name is a valid DOS name, else FALSE.
 *
 * ************************************************************************** **
 */
int is_8_3( char *fname, int check_case )
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

    Syslog('f', "Checking %s for 8.3\n", fname);

    /* Can't be 0 chars or longer than 12 chars */
    if( (len == 0) || (len > 12) )
	return FALSE;

    /* Mustn't be an MS-DOS Special file such as lpt1 or even lpt1.txt */
    if (is_reserved_msdos(fname))
	return FALSE;

    /* Check that all characters are the correct case, if asked to do so. */
    if (check_case && case_mangle) {
	switch (case_default) {
	    case CASE_LOWER:
			if (strhasupper(fname))
			    return FALSE;
			break;
	    case CASE_UPPER:
			if (strhaslower(fname))
			    return FALSE;
			break;
	}
    }

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
} /* is_8_3 */


/* ************************************************************************** **
 * Used only in do_fwd_mangled_map(), below.
 * ************************************************************************** **
 */
static char *map_filename( char *s,         /* This is null terminated */
                           char *pattern,   /* This isn't. */
                           int len )        /* This is the length of pattern. */
{
    static pstring matching_bit;    /* The bit of the string which matches */
				    /* a * in pattern if indeed there is a * */
    char *sp;			    /* Pointer into s. */
    char *pp;			    /* Pointer into p. */
    char *match_start;		    /* Where the matching bit starts. */
    pstring pat;

    strncpy( pat, pattern, len );   /* Get pattern into a proper string! */
    pstrcpy( matching_bit, "" );    /* Match but no star gets this. */
    pp = pat;			    /* Initialize the pointers. */
    sp = s;

    if( strequal(s, ".") || strequal(s, "..")) {
	return NULL;                /* Do not map '.' and '..' */
    }

    if ((len == 1) && (*pattern == '*')) {
	return NULL;                /* Impossible, too ambiguous for */
    }				    /* words! */

    while ((*sp)		    /* Not the end of the string. */
	&& (*pp)		    /* Not the end of the pattern. */
	&& (*sp == *pp)		    /* The two match. */
	&& (*pp != '*')) {	    /* No wildcard. */
	    sp++;                   /* Keep looking. */
	    pp++;
    }

    if (!*sp && !*pp)		    /* End of pattern. */
	return (matching_bit);	    /* Simple match.  Return empty string. */

    if (*pp == '*') {
	pp++;                       /* Always interrested in the chacter */
				    /* after the '*' */
	if (!*pp) {                 /* It is at the end of the pattern. */
	    strncpy(matching_bit, s, sp-s);
	    return matching_bit;
	} else {
	    /* The next character in pattern must match a character further */
	    /* along s than sp so look for that character. */
	    match_start = sp;
	    while ((*sp)            /* Not the end of s. */
		&& (*sp != *pp))    /* Not the same  */
		    sp++;           /* Keep looking. */
	    if (!*sp) {             /* Got to the end without a match. */
		return( NULL );
	    } else {                /* Still hope for a match. */
		/* Now sp should point to a matching character. */
		strncpy(matching_bit, match_start, sp-match_start);
		/* Back to needing a stright match again. */
		while ((*sp)		    /* Not the end of the string. */
		    && (*pp)		    /* Not the end of the pattern. */
		    && (*sp == *pp)) {	    /* The two match. */
			sp++;               /* Keep looking. */
			pp++;
		}
		if (!*sp && !*pp)   /* Both at end so it matched */
		    return matching_bit;
		else
		    return NULL;
	    }
	}
    }
    return NULL;		    /* No match. */
} /* map_filename */


/* ************************************************************************** **
 * MangledMap is a series of name pairs in () separated by spaces.
 * If s matches the first of the pair then the name given is the
 * second of the pair.  A * means any number of any character and if
 * present in the second of the pair as well as the first the
 * matching part of the first string takes the place of the * in the
 * second.
 *
 * I wanted this so that we could have RCS files which can be used
 * by UNIX and DOS programs.  My mapping string is (RCS rcs) which
 * converts the UNIX RCS file subdirectory to lowercase thus
 * preventing mangling.
 *
 * (I think Andrew wrote the above, but I'm not sure. -- CRH)
 *
 * See 'mangled map' in smb.conf(5).
 *
 * ************************************************************************** **
 */

static void do_fwd_mangled_map(char *s, char *MangledMap)
  {
  char *start=MangledMap;       /* Use this to search for mappings. */
  char *end;                    /* Used to find the end of strings. */
  char *match_string;
  pstring new_string;           /* Make up the result here. */
  char *np;                     /* Points into new_string. */

  Syslog('f', "Mangled Mapping '%s' map '%s'\n", s, MangledMap);
  while( *start )
    {
    while( (*start) && (*start != '(') )
      start++;
    if( !*start )
      continue;                 /* Always check for the end. */
    start++;                    /* Skip the ( */
    end = start;                /* Search for the ' ' or a ')' */
    Syslog('f', "Start of first in pair '%s'\n", start);
    while( (*end) && !((*end == ' ') || (*end == ')')) )
      end++;
    if( !*end )
      {
      start = end;
      continue;                 /* Always check for the end. */
      }
    Syslog('f', "End of first in pair '%s'\n", end);
    if( (match_string = map_filename( s, start, end-start )) )
      {
      Syslog('f', "Found a match\n");
      /* Found a match. */
      start = end + 1;          /* Point to start of what it is to become. */
      Syslog('f', "Start of second in pair '%s'\n", start);
      end = start;
      np = new_string;
      while( (*end)             /* Not the end of string. */
          && (*end != ')')      /* Not the end of the pattern. */
          && (*end != '*') )    /* Not a wildcard. */
        *np++ = *end++;
      if( !*end )
        {
        start = end;
        continue;               /* Always check for the end. */
        }
      if( *end == '*' )
        {
        pstrcpy( np, match_string );
        np += strlen( match_string );
        end++;                  /* Skip the '*' */
        while( (*end)             /* Not the end of string. */
            && (*end != ')')      /* Not the end of the pattern. */
            && (*end != '*') )    /* Not a wildcard. */
          *np++ = *end++;
        }
      if( !*end )
        {
        start = end;
        continue;               /* Always check for the end. */
        }
      *np++ = '\0';             /* NULL terminate it. */
      Syslog('f', "End of second in pair '%s'\n", end);
      pstrcpy( s, new_string );  /* Substitute with the new name. */
      Syslog('f', "s is now '%s'\n", s);
      }
    start = end;              /* Skip a bit which cannot be wanted anymore. */
    start++;
    }
  } /* do_fwd_mangled_map */

/*****************************************************************************
 * do the actual mangling to 8.3 format
 * the buffer must be able to hold 13 characters (including the null)
 *****************************************************************************
 */
void mangle_name_83(char *s)
{
    int csum;
    char *p;
    char extension[4];
    char base[9];
    int baselen = 0;
    int extlen = 0;
    int skip;

    extension[0] = 0;
    base[0] = 0;

    p = strrchr(s,'.');  
    if (p && (strlen(p+1) < (size_t)4)) {
	int	all_normal = (strisnormal(p+1)); /* XXXXXXXXX */

	if( all_normal && p[1] != 0 ) {
	    *p = 0;
	    csum = str_checksum( s );
	    *p = '.';
	} else
	    csum = str_checksum(s);
    } else
	csum = str_checksum(s);

    tu(s);
    Syslog('f', "Mangling name %s to ",s);

  if (p) {
    if( p == s )
      safe_strcpy( extension, "___", 3 );
    else {
      *p++ = 0;
      while (*p && extlen < 3) {
        skip = get_character_len(*p);
        switch(skip) {
          case 2: 
            if( extlen < 2 ) {
              extension[extlen++] = p[0];
              extension[extlen++] = p[1];
              } else {
              extension[extlen++] = mangle( (unsigned char)*p );
              }
            p += 2;
            break;
          case 1:
            extension[extlen++] = p[0];
            p++;
            break;
          default:
            if(/* isdoschar (*p) && */ *p != '.' )
              extension[extlen++] = p[0];
            p++;
            break;
          }
        }
      extension[extlen] = 0;
      }
    }

  p = s;

  while( *p && baselen < 5 ) {
    skip = get_character_len(*p);
    switch( skip ) {
      case 2:
        if( baselen < 4 ) {
          base[baselen++] = p[0];
          base[baselen++] = p[1];
          } else {
          base[baselen++] = mangle( (unsigned char)*p );
          }
        p += 2;
        break;
      case 1:
        base[baselen++] = p[0];
        p++;
        break;
        if(/* isdoschar( *p ) && */ *p != '.' )
          base[baselen++] = p[0];
        p++;
        break;
      }
    }
  base[baselen] = 0;

  csum = csum % (MANGLE_BASE*MANGLE_BASE);

  (void)slprintf(s, 12, "%s%c%c%c",
                 base, magic_char, mangle( csum/MANGLE_BASE ), mangle( csum ) );

  if( *extension ) {
    (void)pstrcat( s, "." );
    (void)pstrcat( s, extension );
    }

  Syslog('f', "%s\n", s);

} /* mangle_name_83 */

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
 *          cache83 - If False, the mangled name cache will not be updated.
 *                    This is usually used to prevent that we overwrite
 *                    a conflicting cache entry prematurely, i.e. before
 *                    we know whether the client is really interested in the
 *                    current name.  (See PR#13758).  UKD.
 *          snum    - Share number.  This identifies the share in which the
 *                    name exists.
 *
 *  Output: Returns False only if the name wanted mangling but the share does
 *          not have name mangling turned on.
 *
 * ****************************************************************************
 */
int name_map_mangle(char *OutName, int need83, int cache83, int snum)
{
        char *map;
        Syslog('f',"name_map_mangle( %s, need83 = %s, cache83 = %s, %d )\n", OutName,
                need83 ? "TRUE" : "FALSE", cache83 ? "TRUE" : "FALSE", snum);

#ifdef MANGLE_LONG_FILENAMES
        if( !need83 && is_illegal_name(OutName) )
                need83 = TRUE;
#endif

        /* apply any name mappings */
        map = lp_mangled_map(snum);

        if (map && *map) {
                do_fwd_mangled_map( OutName, map );
        }

        /* check if it's already in 8.3 format */
        if (need83 && !is_8_3(OutName, TRUE)) {
                char *tmp = NULL; 

                if (!lp_manglednames(snum)) {
                        return(FALSE);
                }

                /* mangle it into 8.3 */
                if (cache83)
                        tmp = strdup(OutName);

                mangle_name_83(OutName);

                if(tmp != NULL) {
                        cache_mangled_name(OutName, tmp);
                        free(tmp);
                }
        }

        Syslog('f',"name_map_mangle() ==> [%s]\n", OutName);
        return(TRUE);
} /* name_map_mangle */


