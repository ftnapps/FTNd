/*****************************************************************************
 *
 * File ..................: charconv_utf.c
 * Purpose ...............: Common utilities
 * Last modification date : 29-Aug-2000
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "libs.h"
#include "memwatch.h"
#include "structs.h"
#include "common.h"


char Base_64Code[] =  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


/* returns numeric value from a Base64Code[] digit */
static int index_hex2[128] = {
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,0x3e,  -1,  -1,  -1,0x3f,
    0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,
    0x3c,0x3d,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,0x00,0x01,0x02,0x03,0x04,0x05,0x06,
    0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,
    0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
    0x17,0x18,0x19,  -1,  -1,  -1,  -1,  -1,
      -1,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
    0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,
    0x31,0x32,0x33,  -1,  -1,  -1,  -1,  -1
};



void utf7_to_eight(char *in,char **out,int *code)
{
    int isb64,l_code=CHRS_AUTODETECT;
    char *p, *q, *buf;

    buf=malloc(strlen(in)*sizeof(char));

    isb64=0;
    for (p = in, q = buf; *p != '\0';) {

        if (isb64) { /* we are in B64 encoding, that is in utf-7 */
            int bit_buffer=0;
            int nbits=0;
            int i,l,result,offset=0;

            /* find the lenght of the B64 string */ 
            l=strspn(p,Base_64Code);
            for (i=0;i<l;i++) {
                bit_buffer <<= 6;
                bit_buffer |= index_hex2[(unsigned int)*p++];
                nbits += 6;
                if (nbits >= 8) {
                    nbits -= 8;
                    result = ((bit_buffer >> nbits)&0xff);
                    /* if the charset code is unknown try to find it.
                     * it only works for latin1 (iso-8859-1), cyrillic, greek,
                     * arabic and hebrew (iso-8859-[5678]), as for other latin
                     * encodings it is harder, iso-8859-2 is assumed as it is
                     * the most common
                     */
                    if ((l_code==CHRS_AUTODETECT) || (l_code==CHRS_ISO_8859_1)) {
                        if (result == 0x00) l_code=CHRS_ISO_8859_1;
                        else if (result == 0x01) l_code=CHRS_ISO_8859_2;
                        else if (result == 0x03) l_code=CHRS_ISO_8859_7;
                        else if (result == 0x04) l_code=CHRS_ISO_8859_5;
                        else if (result == 0x05) l_code=CHRS_ISO_8859_8;
                        else if (result == 0x06) l_code=CHRS_ISO_8859_6;
                    }
                    /* what to add to next byte to convert to iso-8859-*
                     * note that it doesn't work for iso-8859-{2,3,4,9,10}
                     * as the offset changes for almost each char 
                     */ 
                    if (result == 0x00) offset=0x00;
                    else if (result == 0x03) offset=0x30;
                    else if (result == 0x04) offset=0xa0;
                    else if (result == 0x05) offset=0x10;
                    else if (result == 0x06) offset=0xa0;

                    /* convert to the right 8bit char by adding offset */
                    if (result < 0x06) *q++ = (char)((bit_buffer & 0xff) + offset);
                    else *q++ = (char)(bit_buffer & 0xff);
                }
            }
	    /* end of B64 encoding */	
            if (*p == '-') p++;
            isb64=0;
        } else if (*p == '+') { /* '+' is the beginning of a new B64 section */
            isb64=1;
            p++;
        } else { /* ascii encoding */
            *q++=*p++;
        }
    }
    *q = '\0';

    /* now we know the 8bit charset that was encoded whith utf-7,
     * so ask again to see if a conversion to FTN charset is needed
     */
    if (*code==CHRS_AUTODETECT || *code==CHRS_NOTSET)
        *code=getoutcode(l_code);
    switch (l_code) {
      case CHRS_ISO_8859_1 :
      case CHRS_ISO_8859_15: 
        switch (*code) {
          case CHRS_CP437 :     eight2eight(buf,out,(char *)ISO_8859_1__CP437); break;
          case CHRS_CP850 :     eight2eight(buf,out,(char *)ISO_8859_1__CP850); break;
          case CHRS_MACINTOSH : eight2eight(buf,out,(char *)ISO_8859_1__MACINTOSH); break;
          default :             noconv(buf,out); break;
        }
        break;
      case CHRS_ISO_8859_5 :
        switch (*code) {
          case CHRS_CP866 :   eight2eight(buf,out,(char *)ISO_8859_5__CP866); break;
          case CHRS_KOI8_R :
          case CHRS_KOI8_U :  eight2eight(buf,out,(char *)ISO_8859_5__KOI8); break;
          case CHRS_MIK_CYR : eight2eight(buf,out,(char *)ISO_8859_5__MIK_CYR); break;
          default :           noconv(buf,out); break;
        }
        break;
      case CHRS_ISO_8859_8 :
        switch (*code) {
          case CHRS_CP424 : eight2eight(buf,out,(char *)ISO_8859_8__CP424); break;
          case CHRS_CP862 : eight2eight(buf,out,(char *)ISO_8859_8__CP862); break;
          default :         noconv(buf,out); break;
        }
        break;
      default :                 noconv(in,out); break;
    }
}

/*
 * UNICODE                   UTF-8
 * -------------   ------------------------------------
 * 0000 -> 007F  =  7 bits = 0xxxxxxx
 * 0080 -> 07FF  = 11 bits = 110xxxxx 10xxxxxx
 * 0800 -> FFFF  = 16 bits = 1110xxxx 10xxxxxx 10xxxxxx
 */
void utf8_to_eight(char *in,char **out,int *code)
{
    int is8bit,l_code=CHRS_AUTODETECT;
    char *p, *q, *buf;

    buf=malloc(strlen(in)*sizeof(char));

    is8bit=0;
    for (p = in, q = buf; *p != '\0';) {
        int bit_buffer=0;
        int nbits=0;
        int result,offset=0;

        if ((*p & 0xff) >= 0xe0) { /* 16 bits = 1110xxxx 10xxxxxx 10xxxxxx */
            bit_buffer=((*p++ & 0xff) & 0x0f);
            bit_buffer=(bit_buffer << 4);
            bit_buffer+=((*p++ & 0xff) & 0xbf);
            bit_buffer=(bit_buffer << 6);
            bit_buffer+=((*p++ & 0xff) & 0x3f);
            nbits=16;
        } else if ((*p & 0xff) >= 0xc0) { /* 11 bits = 110xxxxx 10xxxxxx */
            bit_buffer=((*p++ & 0xff) & 0x2f);
            bit_buffer=(bit_buffer << 6);
            bit_buffer+=((*p++ & 0xff) & 0x3f);
            nbits=11;
        } else { /* 7 bits = 0xxxxxxx */
            bit_buffer=(*p++ & 0xff);
            nbits=7;
        }

        if (nbits >= 8) {
            result = ((bit_buffer >> 8)&0xff);
            /* if the charset code is unknown try to find it.
             * it only works for latin1 (iso-8859-1), cyrillic, greek,
             * arabic and hebrew (iso-8859-[5678]), as for other latin
             * encodings it is harder, iso-8859-2 is assumed as it is
             * the most common
             */
            if ((l_code==CHRS_AUTODETECT) || (l_code==CHRS_ISO_8859_1)) {
                if (result == 0x00) l_code=CHRS_ISO_8859_1;
                else if (result == 0x01) l_code=CHRS_ISO_8859_2;
                else if (result == 0x03) l_code=CHRS_ISO_8859_7;
                else if (result == 0x04) l_code=CHRS_ISO_8859_5;
                else if (result == 0x05) l_code=CHRS_ISO_8859_8;
                else if (result == 0x06) l_code=CHRS_ISO_8859_6;
            }
            /* what to add to next byte to convert to iso-8859-*
             * note that it doesn't work for iso-8859-{2,3,4,9,10}
             * as the offset changes for almost each char
             */
            if (result == 0x00) offset=0x00;
            else if (result == 0x03) offset=0x30;
            else if (result == 0x04) offset=0xa0;
            else if (result == 0x05) offset=0x10;
            else if (result == 0x06) offset=0xa0;
            /* convert to the right 8bit char by adding offset */
            if (result < 0x06) *q++ = (char)((bit_buffer & 0xff) + offset);
            else *q++ = (char)(bit_buffer & 0xff);
        } else { /* ascii encoding */
            *q++ = (char)(bit_buffer & 0xff);
        }
    }
    *q = '\0';
    /* now we know the 8bit charset that was encoded whith utf-7,
     * so ask again to see if a conversion to FTN charset is needed
     */
    if (*code==CHRS_AUTODETECT || *code==CHRS_NOTSET)
        *code=getoutcode(l_code);
    switch (l_code) {
      case CHRS_ISO_8859_1 :
      case CHRS_ISO_8859_15:
        switch (*code) {
          case CHRS_CP437 :     eight2eight(buf,out,(char *)ISO_8859_1__CP437); break;
          case CHRS_CP850 :     eight2eight(buf,out,(char *)ISO_8859_1__CP850); break;
          case CHRS_MACINTOSH : eight2eight(buf,out,(char *)ISO_8859_1__MACINTOSH); break;
          default :             noconv(buf,out); break;
        }
        break;
      case CHRS_ISO_8859_5 :
        switch (*code) {
          case CHRS_CP866 :   eight2eight(buf,out,(char *)ISO_8859_5__CP866); break;
          case CHRS_KOI8_R :
          case CHRS_KOI8_U :  eight2eight(buf,out,(char *)ISO_8859_5__KOI8); break;
          case CHRS_MIK_CYR : eight2eight(buf,out,(char *)ISO_8859_5__MIK_CYR); break;
          default :           noconv(buf,out); break;
        }
        break;
      case CHRS_ISO_8859_8 :
        switch (*code) {
          case CHRS_CP424 : eight2eight(buf,out,(char *)ISO_8859_8__CP424); break;
          case CHRS_CP862 : eight2eight(buf,out,(char *)ISO_8859_8__CP862); break;
          default :         noconv(buf,out); break;
        }
        break;
      default :                 noconv(in,out); break;
    }
}


