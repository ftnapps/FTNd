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
#include "ubi_sLinkList.h"
#include "ubi_dLinkList.h"
#include "ubi_Cache.h"

typedef enum {CASE_UPPER, CASE_LOWER} CASES;

int	case_default = CASE_UPPER;	/* Are conforming 8.3 names all upper or lower?   */
int	case_mangle = TRUE;    		/* If true, all chars in 8.3 should be same case. */

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

// static ubi_cacheRoot mangled_cache[1] =  { { { 0, 0, 0, 0 }, 0, 0, 0, 0, 0, 0 } };
static int           mc_initialized   = FALSE;
#define MANGLED_CACHE_MAX_ENTRIES 0
#define MANGLED_CACHE_MAX_MEMORY  16384

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
  if( p )
    *p = '\0';

  tu( upperFname );
  p = upperFname + 1;
  switch( upperFname[0] )
    {
    case 'A':
      if( 0 == strcmp( p, "UX" ) )
        return( TRUE );
      break;
    case 'C':
      if( (0 == strcmp( p, "LOCK$" ))
       || (0 == strcmp( p, "ON" ))
       || (0 == strcmp( p, "OM1" ))
       || (0 == strcmp( p, "OM2" ))
       || (0 == strcmp( p, "OM3" ))
       || (0 == strcmp( p, "OM4" ))
        )
        return( TRUE );
      break;
    case 'L':
      if( (0 == strcmp( p, "PT1" ))
       || (0 == strcmp( p, "PT2" ))
       || (0 == strcmp( p, "PT3" ))
        )
        return( TRUE );
      break;
    case 'N':
      if( 0 == strcmp( p, "UL" ) )
        return( TRUE );
      break;
    case 'P':
      if( 0 == strcmp( p, "RN" ) )
        return( TRUE );
      break;
    }

  return( FALSE );
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
static int is_illegal_name( char *name )
  {
  unsigned char *s;
  int            skip;

  if( !name )
    return( TRUE );

  if( !ct_initialized )
    init_chartest();

  s = (unsigned char *)name;
  while( *s )
    {
    skip = get_character_len( *s );
    if( skip != 0 )
      {
      s += skip;
      }
    else
      {
      if( isillegal( *s ) )
        return( TRUE );
      else
        s++;
      }
    }

  return( FALSE );
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
int is_mangled( char *s )
  {
  char *magic;

  if( !ct_initialized )
    init_chartest();

  magic = strchr( s, magic_char );
  while( magic && magic[1] && magic[2] )          /* 3 chars, 1st is magic. */
    {
    if( ('.' == magic[3] || '/' == magic[3] || !(magic[3]))          /* Ends with '.' or nul or '/' ?  */
     && isbasechar( toupper(magic[1]) )           /* is 2nd char basechar?  */
     && isbasechar( toupper(magic[2]) ) )         /* is 3rd char basechar?  */
      return( TRUE );                           /* If all above, then true, */
    magic = strchr( magic+1, magic_char );      /*    else seek next magic. */
    }
  return( FALSE );
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
  int   skip;
  char *p;
  char *dot_pos;
  char *slash_pos = strrchr( fname, '/' );

  /* If there is a directory path, skip it. */
  if( slash_pos )
    fname = slash_pos + 1;
  len = strlen( fname );

  Syslog('f', "Checking %s for 8.3\n", fname);

  /* Can't be 0 chars or longer than 12 chars */
  if( (len == 0) || (len > 12) )
    return( FALSE );

  /* Mustn't be an MS-DOS Special file such as lpt1 or even lpt1.txt */
  if( is_reserved_msdos( fname ) )
    return( FALSE );

  /* Check that all characters are the correct case, if asked to do so. */
  if( check_case && case_mangle )
    {
    switch( case_default )
      {
      case CASE_LOWER:
        if( strhasupper( fname ) )
          return(FALSE);
        break;
      case CASE_UPPER:
        if( strhaslower( fname ) )
          return(FALSE);
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
  while( *p )
    {
    if( (skip = get_character_len( *p )) != 0 )
      p += skip;
    else 
      {
      if( *p == '.' && !dot_pos )
        dot_pos = (char *)p;
      else
        if( !isdoschar( *p ) )
          return( FALSE );
      p++;
      }
    }

  /* no dot and less than 9 means OK */
  if( !dot_pos )
    return( len <= 8 );
        
  l = PTR_DIFF( dot_pos, fname );

  /* base must be at least 1 char except special cases . and .. */
  if( l == 0 )
    return( 0 == strcmp( fname, "." ) || 0 == strcmp( fname, ".." ) );

  /* base can't be greater than 8 */
  if( l > 8 )
    return( FALSE );

  /* see smb.conf(5) for a description of the 'strip dot' parameter. */
  if( lp_strip_dot()
   && len - l == 1
   && !strchr( dot_pos + 1, '.' ) )
    {
    *dot_pos = 0;
    return( TRUE );
    }

  /* extension must be between 1 and 3 */
  if( (len - l < 2 ) || (len - l > 4) )
    return( FALSE );

  /* extensions may not have a dot */
  if( strchr( dot_pos+1, '.' ) )
    return( FALSE );

  /* must be in 8.3 format */
  return( TRUE );
  } /* is_8_3 */


/* ************************************************************************** **
 * Compare two cache keys and return a value indicating their ordinal
 * relationship.
 *
 *  Input:  ItemPtr - Pointer to a comparison key.  In this case, this will
 *                    be a mangled name string.
 *          NodePtr - Pointer to a node in the cache.  The node structure
 *                    will be followed in memory by a mangled name string.
 *
 *  Output: A signed integer, as follows:
 *            (x < 0)  <==> Key1 less than Key2
 *            (x == 0) <==> Key1 equals Key2
 *            (x > 0)  <==> Key1 greater than Key2
 *
 *  Notes:  This is a ubiqx-style comparison routine.  See ubi_BinTree for
 *          more info.
 *
 * ************************************************************************** **
 */
static signed int cache_compare( ubi_btItemPtr ItemPtr, ubi_btNodePtr NodePtr )
  {
  char *Key1 = (char *)ItemPtr;
  char *Key2 = (char *)(((ubi_cacheEntryPtr)NodePtr) + 1);

  return( StrCaseCmp( Key1, Key2 ) );
  } /* cache_compare */

/* ************************************************************************** **
 * Free a cache entry.
 *
 *  Input:  WarrenZevon - Pointer to the entry that is to be returned to
 *                        Nirvana.
 *  Output: none.
 *
 *  Notes:  This function gets around the possibility that the standard
 *          free() function may be implemented as a macro, or other evil
 *          subversions (oh, so much fun).
 *
 * ************************************************************************** **
 */
static void cache_free_entry( ubi_trNodePtr WarrenZevon )
  {
          ZERO_STRUCTP(WarrenZevon);
          free( WarrenZevon );
  } /* cache_free_entry */

