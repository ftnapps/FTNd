/*****************************************************************************
 *
 * File ..................: charconv_jp.c
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

#include "libs.h"
#include "structs.h"
#include "common.h"


/* ### Modified by P. Saratxaga on 26 Oct 95 ###
 * This is the kcon.c taken from JE version (code from T. Tanaka)
 * I've modified it to support 8bit -> 8bit transcoding in addition of
 * japanese (16 bits) ones.
 * Also the codings are not readen from a config file but taken from the
 * charset values in Header lines.
 */



void OPENINOUTFILES(FILE **in, FILE **out, char *src)
{
	*in=tmpfile();
	*out=tmpfile();
	fwrite(src, sizeof(char), strlen(src), *in);
	rewind(*in);
}



void CLOSEINOUTFILES(FILE **in, FILE **out,char **dest)
{
	int destlen, c;
	char *p;

	rewind(*out);
	for(destlen = 0; (c = fgetc(*out)) != EOF; destlen++);
	rewind(*out);
	if(*dest)
		free(*dest);
	*dest = (char *)malloc((destlen + 1) * sizeof(char));
	for(p = *dest; (c = fgetc(*out)) != EOF; p++)
		*p = (char)(c & 0xff);
	*p = '\0';
	fclose(*in);
	fclose(*out);
}



void sjis2jis(int *p1,int *p2)
{
	register unsigned char c1 = *p1;
	register unsigned char c2 = *p2;
	register int adjust = c2 < 159;
	register int rowOffset = c1 < 160 ? 112 : 176;
	register int cellOffset = adjust ? (31 + (c2 > 127)) : 126;

	*p1 = ((c1 - rowOffset) << 1) - adjust;
	*p2 -= cellOffset;
}



void jis2sjis(int *p1,int *p2)
{
	register unsigned char c1 = *p1;
	register unsigned char c2 = *p2;
	register int rowOffset = c1 < 95 ? 112 : 176;
	register int cellOffset = c1 % 2 ? 31 + (c2 > 95) : 126;

	*p1 = ((c1 + 1) >> 1) + rowOffset;
	*p2 = c2 + cellOffset;
}



void shift2seven(char *src,char **dest,int incode,char ki[],char ko[])
{
	int p1,p2,intwobyte = FALSE;
	FILE *in, *out;

	OPENINOUTFILES(&in,&out,src);

	while ((p1 = fgetc(in)) != EOF) {
		switch (p1) {
			case NUL :
			case FF :
					break;
			case CR :
			case NL :
					if (intwobyte) {
						intwobyte = FALSE;
						fprintf(out,"%c%s",ESC,ko);
					}
					fprintf(out,"%c",NL);
					break;
			default :
					if SJIS1(p1) {
						p2 = fgetc(in);
						if SJIS2(p2) {
							sjis2jis(&p1,&p2);
							if (!intwobyte) {
								intwobyte = TRUE;
								fprintf(out,"%c%s",ESC,ki);
							}
						}
						fprintf(out,"%c%c",p1,p2);
					} else if HANKATA(p1) {
						han2zen(in,&p1,&p2,incode);
						sjis2jis(&p1,&p2);
						if (!intwobyte) {
							intwobyte = TRUE;
							fprintf(out,"%c%s",ESC,ki);
						}
						fprintf(out,"%c%c",p1,p2);
					} else {
						if (intwobyte) {
							intwobyte = FALSE;
							fprintf(out,"%c%s",ESC,ko);
						}
						fprintf(out,"%c",p1);
					}
					break;
		}
	}
	if (intwobyte)
		fprintf(out,"%c%s",ESC,ko);
	CLOSEINOUTFILES(&in,&out,dest);
}



void shift2euc(char *src, char **dest, int incode, int tofullsize)
{
	int p1,p2;
	FILE *in, *out;

	OPENINOUTFILES(&in,&out,src);

	while ((p1 = fgetc(in)) != EOF) {
		switch (p1) {
			case CR :
			case NL :
					fprintf(out,"%c",NL);
					break;
			case NUL :
			case FF :
					break;
			default :
					if SJIS1(p1) {
						p2 = fgetc(in);
						if SJIS2(p2) {
							sjis2jis(&p1,&p2);
							p1 += 128;
							p2 += 128;
						}
						fprintf(out,"%c%c",p1,p2);
					} else if HANKATA(p1) {
						if (tofullsize) {
							han2zen(in,&p1,&p2,incode);
							sjis2jis(&p1,&p2);
							p1 += 128;
							p2 += 128;
						} else {
							p2 = p1;
							p1 = 142;
						}
						fprintf(out,"%c%c",p1,p2);
					} else
						fprintf(out,"%c",p1);
					break;
		}
	}
	CLOSEINOUTFILES(&in,&out,dest);
}



void euc2seven(char *src,char **dest,int incode,char ki[],char ko[])
{
  int p1,p2,intwobyte = FALSE;
  FILE *in, *out;

  OPENINOUTFILES(&in,&out,src);

  while ((p1 = fgetc(in)) != EOF) {
    switch (p1) {
      case NL :
        if (intwobyte) {
          intwobyte = FALSE;
          fprintf(out,"%c%s",ESC,ko);
        }
        fprintf(out,"%c",p1);
        break;
      case FF :
        break;
      default :
        if ISEUC(p1) {
          p2 = fgetc(in);
          if ISEUC(p2) {
            p1 -= 128;
            p2 -= 128;
            if (!intwobyte) {
              intwobyte = TRUE;
              fprintf(out,"%c%s",ESC,ki);
            }
          }
          fprintf(out,"%c%c",p1,p2);
        }
        else if (p1 == 142) {
          p2 = fgetc(in);
          if HANKATA(p2) {
            p1 = p2;
            han2zen(in,&p1,&p2,incode);
            sjis2jis(&p1,&p2);
            if (!intwobyte) {
              intwobyte = TRUE;
              fprintf(out,"%c%s",ESC,ki);
            }
          }
          fprintf(out,"%c%c",p1,p2);
        }
        else {
          if (intwobyte) {
            intwobyte = FALSE;
            fprintf(out,"%c%s",ESC,ko);
          }
          fprintf(out,"%c",p1);
        }
        break;
    }
  }
  if (intwobyte)
    fprintf(out,"%c%s",ESC,ko);
  CLOSEINOUTFILES(&in,&out,dest);
}
 


void euc2shift(char *src,char **dest,int incode,int tofullsize)
{
  int p1,p2;
  FILE *in, *out;

  OPENINOUTFILES(&in,&out,src);

  while ((p1 = fgetc(in)) != EOF) {
    switch (p1) {
      case FF :
        break;
      default :
        if ISEUC(p1) {
          p2 = fgetc(in);
          if ISEUC(p2) {
            p1 -= 128;
            p2 -= 128;
            jis2sjis(&p1,&p2);
          }
          fprintf(out,"%c%c",p1,p2);
        }
        else if (p1 == 142) {
          p2 = fgetc(in);
          if HANKATA(p2) {
            if (tofullsize) {
              p1 = p2;
              han2zen(in,&p1,&p2,incode);
              fprintf(out,"%c%c",p1,p2);
            }
            else {
              p1 = p2;
              fprintf(out,"%c",p1);
            }
          }
          else
            fprintf(out,"%c%c",p1,p2);
        }
        else
          fprintf(out,"%c",p1);
        break;
    }
  }
  CLOSEINOUTFILES(&in,&out,dest);
}



void euc2euc(char *src,char **dest,int incode,int tofullsize)
{
  int p1,p2;
  FILE *in, *out;

  OPENINOUTFILES(&in,&out,src);

  while ((p1 = fgetc(in)) != EOF) {
    switch (p1) {
      case FF :
        break;
      default :
        if ISEUC(p1) {
          p2 = fgetc(in);
          if ISEUC(p2)
            fprintf(out,"%c%c",p1,p2);
        }
        else if (p1 == 142) {
          p2 = fgetc(in);
          if (HANKATA(p2) && tofullsize) {
            p1 = p2;
            han2zen(in,&p1,&p2,incode);
            sjis2jis(&p1,&p2);
            p1 += 128;
            p2 += 128;
          }
          fprintf(out,"%c%c",p1,p2);
        }
        else
          fprintf(out,"%c",p1);
        break;
    }
  }
  CLOSEINOUTFILES(&in,&out,dest);
}

void shift2shift(char *src,char **dest,int incode,int tofullsize)
{
  int p1,p2;
  FILE *in, *out;

  OPENINOUTFILES(&in,&out,src);
  
  while ((p1 = fgetc(in)) != EOF) {
    switch (p1) {
      case CR :
      case NL :
        fprintf(out,"%c",NL);
        break;
      case NUL :
      case FF :
        break;
      default :
        if SJIS1(p1) {
          p2 = fgetc(in);
          if SJIS2(p2)
            fprintf(out,"%c%c",p1,p2);
        }
        else if (HANKATA(p1) && tofullsize) {
          han2zen(in,&p1,&p2,incode);
          fprintf(out,"%c%c",p1,p2);
        }
        else
          fprintf(out,"%c",p1);
        break;
    }
  }
  CLOSEINOUTFILES(&in,&out,dest);
}

void seven2shift(char *src,char **dest)
{
  int temp,p1,p2,intwobyte = FALSE;
  FILE *in, *out;

  OPENINOUTFILES(&in,&out,src);

  while ((p1 = fgetc(in)) != EOF) {
    switch (p1) {
      case ESC :
        temp = fgetc(in);
        SkipESCSeq(in,temp,&intwobyte);
        break;
      case NL :
        if (intwobyte)
          intwobyte = FALSE;
        fprintf(out,"%c",p1);
        break;
      case FF :
        break;
      default :
        if (intwobyte) {
          p2 = fgetc(in);
          jis2sjis(&p1,&p2);
          fprintf(out,"%c%c",p1,p2);
        }
        else
          fprintf(out,"%c",p1);
        break;
    }
  }
  CLOSEINOUTFILES(&in,&out,dest);
}
  
void seven2euc(char *src, char **dest)
{
  int temp,p1,p2,intwobyte = FALSE;
  FILE *in, *out;

  OPENINOUTFILES(&in,&out,src);

  while ((p1 = fgetc(in)) != EOF) {
    switch (p1) {
      case ESC :
        temp = fgetc(in);
        SkipESCSeq(in,temp,&intwobyte);
        break;
      case NL :
        if (intwobyte)
          intwobyte = FALSE;
        fprintf(out,"%c",p1);
        break;
      case FF :
        break;
      default :
        if (intwobyte) {
          p2 = fgetc(in);
          p1 += 128;
          p2 += 128;
          fprintf(out,"%c%c",p1,p2);
        }
        else
          fprintf(out,"%c",p1);
        break;
    }
  }
  CLOSEINOUTFILES(&in,&out,dest);
}

void seven2seven(char *src,char **dest,char ki[],char ko[])
{
  int temp,p1,p2,change,intwobyte = FALSE;
  FILE *in, *out;

  OPENINOUTFILES(&in,&out,src);

  while ((p1 = fgetc(in)) != EOF) {
    switch (p1) {
      case ESC :
        temp = fgetc(in);
        change = SkipESCSeq(in,temp,&intwobyte);
        if ((intwobyte) && (change))
          fprintf(out,"%c%s",ESC,ki);
        else if (change)
          fprintf(out,"%c%s",ESC,ko);
        break;
      case NL :
        if (intwobyte) {
          intwobyte = FALSE;
          fprintf(out,"%c%s",ESC,ko);
        }
        fprintf(out,"%c",p1);
        break;
      case FF :
        break;
      default :
        if (intwobyte) {
          p2 = fgetc(in);
          fprintf(out,"%c%c",p1,p2);
        }
        else
          fprintf(out,"%c",p1);
        break;
    }
  }
  if (intwobyte)
    fprintf(out,"%c%s",ESC,ko);
  CLOSEINOUTFILES(&in,&out,dest);
}

void han2zen(FILE *in,int *p1,int *p2,int incode)
{
  int junk,maru,nigori;

  maru = nigori = FALSE;
  if (incode == CHRS_SJIS) {
    *p2 = fgetc(in);
    if (*p2 == 222) {
      if (ISNIGORI(*p1) || *p1 == 179)
        nigori = TRUE;
      else
        ungetc(*p2,in);
    }
    else if (*p2 == 223) {
      if ISMARU(*p1)
        maru = TRUE;
      else
        ungetc(*p2,in);
    }
    else
      ungetc(*p2,in);
  }
  else if (incode == CHRS_EUC_JP) {
    junk = fgetc(in);
    if (junk == 142) {
      *p2 = fgetc(in);
      if (*p2 == 222) {
        if (ISNIGORI(*p1) || *p1 == 179)
          nigori = TRUE;
        else {
          ungetc(*p2,in);
          ungetc(junk,in);
        }
      }
      else if (*p2 == 223) {
        if ISMARU(*p1)
          maru = TRUE;
        else {
          ungetc(*p2,in);
          ungetc(junk,in);
        }
      }
      else {
        ungetc(*p2,in);
        ungetc(junk,in);
      }
    }
    else
      ungetc(junk,in);
  }
  switch (*p1) {
    case 161 :
      *p1 = 129;
      *p2 = 66;
      break;
    case 162 :
      *p1 = 129;
      *p2 = 117;
      break;
    case 163 :
      *p1 = 129;
      *p2 = 118;
      break;
    case 164 :
      *p1 = 129;
      *p2 = 65;
      break;
    case 165 :
      *p1 = 129;
      *p2 = 69;
      break;
    case 166 :
      *p1 = 131;
      *p2 = 146;
      break;
    case 167 :
      *p1 = 131;
      *p2 = 64;
      break;
    case 168 :
      *p1 = 131;
      *p2 = 66;
      break;
    case 169 :
      *p1 = 131;
      *p2 = 68;
      break;
    case 170 :
      *p1 = 131;
      *p2 = 70;
      break;
    case 171 :
      *p1 = 131;
      *p2 = 72;
      break;
    case 172 :
      *p1 = 131;
      *p2 = 131;
      break;
    case 173 :
      *p1 = 131;
      *p2 = 133;
      break;
    case 174 :
      *p1 = 131;
      *p2 = 135;
      break;
    case 175 :
      *p1 = 131;
      *p2 = 98;
      break;
    case 176 :
      *p1 = 129;
      *p2 = 91;
      break;
    case 177 :
      *p1 = 131;
      *p2 = 65;
      break;
    case 178 :
      *p1 = 131;
      *p2 = 67;
      break;
    case 179 :
      *p1 = 131;
      *p2 = 69;
      break;
    case 180 :
      *p1 = 131;
      *p2 = 71;
      break;
    case 181 :
      *p1 = 131;
      *p2 = 73;
      break;
    case 182 :
      *p1 = 131;
      *p2 = 74;
      break;
    case 183 :
      *p1 = 131;
      *p2 = 76;
      break;
    case 184 :
      *p1 = 131;
      *p2 = 78;
      break;
    case 185 :
      *p1 = 131;
      *p2 = 80;
      break;
    case 186 :
      *p1 = 131;
      *p2 = 82;
      break;
    case 187 :
      *p1 = 131;
      *p2 = 84;
      break;
    case 188 :
      *p1 = 131;
      *p2 = 86;
      break;
    case 189 :
      *p1 = 131;
      *p2 = 88;
      break;
    case 190 :
      *p1 = 131;
      *p2 = 90;
      break;
    case 191 :
      *p1 = 131;
      *p2 = 92;
      break;
    case 192 :
      *p1 = 131;
      *p2 = 94;
      break;
    case 193 :
      *p1 = 131;
      *p2 = 96;
      break;
    case 194 :
      *p1 = 131;
      *p2 = 99;
      break;
    case 195 :
      *p1 = 131;
      *p2 = 101;
      break;
    case 196 :
      *p1 = 131;
      *p2 = 103;
      break;
    case 197 :
      *p1 = 131;
      *p2 = 105;
      break;
    case 198 :
      *p1 = 131;
      *p2 = 106;
      break;
    case 199 :
      *p1 = 131;
      *p2 = 107;
      break;
    case 200 :
      *p1 = 131;
      *p2 = 108;
      break;
    case 201 :
      *p1 = 131;
      *p2 = 109;
      break;
    case 202 :
      *p1 = 131;
      *p2 = 110;
      break;
    case 203 :
      *p1 = 131;
      *p2 = 113;
      break;
    case 204 :
      *p1 = 131;
      *p2 = 116;
      break;
    case 205 :
      *p1 = 131;
      *p2 = 119;
      break;
    case 206 :
      *p1 = 131;
      *p2 = 122;
      break;
    case 207 :
      *p1 = 131;
      *p2 = 125;
      break;
    case 208 :
      *p1 = 131;
      *p2 = 126;
      break;
    case 209 :
      *p1 = 131;
      *p2 = 128;
      break;
    case 210 :
      *p1 = 131;
      *p2 = 129;
      break;
    case 211 :
      *p1 = 131;
      *p2 = 130;
      break;
    case 212 :
      *p1 = 131;
      *p2 = 132;
      break;
    case 213 :
      *p1 = 131;
      *p2 = 134;
      break;
    case 214 :
      *p1 = 131;
      *p2 = 136;
      break;
    case 215 :
      *p1 = 131;
      *p2 = 137;
      break;
    case 216 :
      *p1 = 131;
      *p2 = 138;
      break;
    case 217 :
      *p1 = 131;
      *p2 = 139;
      break;
    case 218 :
      *p1 = 131;
      *p2 = 140;
      break;
    case 219 :
      *p1 = 131;
      *p2 = 141;
      break;
    case 220 :
      *p1 = 131;
      *p2 = 143;
      break;
    case 221 :
      *p1 = 131;
      *p2 = 147;
      break;
    case 222 :
      *p1 = 129;
      *p2 = 74;
      break;
    case 223 :
      *p1 = 129;
      *p2 = 75;
      break;
  }
  if (nigori) {
    if ((*p2 >= 74 && *p2 <= 103) || (*p2 >= 110 && *p2 <= 122))
      (*p2)++;
    else if (*p1 == 131 && *p2 == 69)
      *p2 = 148;
  }
  else if (maru && *p2 >= 110 && *p2 <= 122)
    *p2 += 2;
}
