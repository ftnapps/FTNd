/*****************************************************************************
 *
 * File ..................: common/charconv_hz.c
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
#include "clcomm.h"


int LF2CR = FALSE;      /* flag for converting ASCII <LF> to <CR> */
int CR2LF=FALSE;        /* flag for converting ASCII <CR> to <LF> */
int pass8 = FALSE;      /* flat for parsing all 8 bits of a character */
int termStyle = FALSE;  /* flag for ignoring line-continuation markers */
int MAXLEN = 77;        /* default maximum line length in the above style */
int MINLEN = 7;         /* minimum line length in the above style */
int errorCount = 0;     /* number of parsing errors detected */

/*
 * internal functions
 */
void EOFerror(void);
void ESCerror(int c);
void GBerror(int c1,int c2);
void GBerror1(int c);
void GBtoSGB(int hi, int lo, int *hi1, int *lo1);
void mac2gb(int hi, int lo, int *hi1, int *lo1);
void dos2gb(int hi, int lo, int *hi1, int *lo1);


void zw2gb(char *src,char **dest)
{
	char *buf;

	buf=(char*)malloc(strlen(*dest) * sizeof(char));

	zw2hz(src,&buf);
	hz2gb(buf,dest);
  
	free(buf);
}



void zw2hz(char *src,char **dest)
{
/*
  Copyright (C) 1989, 1992      Fung F. Lee

  zw2hz 2.0: do a straightforward conversion from a zW file into a HZ file

  This version was an update of version 1.1, because the specification of
  zW had been changed by the original authors.

  Since the set of all zW files is a proper subset of the set of all
  HZ (HGB) files, it is always possible to do perfect translation from
  zW to HZ (HGB); but not vice versa.
  * HGB - High-bit-set GB, as used in CCDOS, Macintosh System 6.0.x and later.

  As for error handling, I took the lazy approach. For example, if the
  original zW file contains invalid GB codes, they will also show up in
  the output HZ file, and can be detected by "hz2gb -v".

  This program is free for general distribution.
*/

/* As we do not want to impose any limit of line length (such as 80 characters
   per line), we parse the input stream on a character by character basis,
   because in the worst case, a line can be as long as a file.
   Although in practice the line length (with or without soft CR marker at
   its end) is likely to be about 80 characters or so, I am not sure what 
   the maximum line length is enforced by the zW standard, nor do I think
   it is a necessary assumption for proper decoding.
 */
	int c1, c2;
	int ASCIImode = TRUE;
	int lineStart = TRUE;
	FILE *fin, *fout;

	OPENINOUTFILES(&fin,&fout,src);

	while ((c1 = fgetc(fin)) != EOF) {
		if (ASCIImode) {
			if (c1 == '\n') {
				fputc('\n', fout);
				lineStart = TRUE;
			} else if (lineStart && c1 == 'z') {
				c2 = fgetc(fin);
				if (c2 == EOF) {
					fputc(c1, fout); 
					break;
				}
				if (c2 == 'W') {
					fprintf(fout, "~{");
					ASCIImode = FALSE;
				} else {
					fputc(c1, fout);
					fputc(c2, fout);
				}
				lineStart = FALSE;
			} else {
				fputc(c1, fout);
				lineStart = FALSE;
			}
		} else { /* GBmode */
			c2 = fgetc(fin);
			if (c1 == '\n') {
				ungetc(c2, fin);
				fprintf(fout, "~}~\n"); 	/* soft CR - with line continuation */
				lineStart = TRUE;
				ASCIImode = TRUE;
			} else if (c2 == EOF) {
				fputc(c1, fout);
				break;
			} else if (c1 == '#' && c2 == '\n') {
				fprintf(fout, "~}\n"); 		/* hard CR */
				lineStart = TRUE;
				ASCIImode = TRUE;
			} else if (c2 == '\n') { 		/* This may be an invalid zW sequence, ... */
								/* anyway, for robustness, I choose ... */
				/* eat c1 */			/* c1 may be ' ' or something else */
				fprintf(fout, "~}\n"); 		/* hard CR */
				lineStart = TRUE;
				ASCIImode = TRUE;
			} else if (c1 == '#' && c2 == ' ') {
				fprintf(fout, "~} ~{");  	/* temporary escape and back */
			} else if (c1 == ' ') {   		/* 0x20?? is now for ASCII characters */
				fprintf(fout, "~}%c~{", c2);  	/* temporary escape and back */
			} else  { 				/* ASSUME they are GB codes, and fix them in program hz2gb */
				fputc(c1, fout); fputc(c2, fout);
			}
		}
	}

	CLOSEINOUTFILES(&fin,&fout,dest);
}



void hz2gb(char *src,char **dest)
{
/*
  Copyright (C) 1989, 1992      Fung F. Lee

  hz2gb 2.0: convert a HZ file into a Macintosh* / CCDOS SGB file.
  *For Macintosh pre-6.0.x Simplified Chinese Operating System.
  Later versions use the same internal code (High-bit-set GB) as CCDOS does.

  The HZ specification does not dictate how to convert invalid HZ files,
  just as the definition of a programming language usually does not specify
  how a compiler should handle illegal programming constructs.
  The error recovery procedure of this HZ decoder was designed after
  examination of the conversion errors reported by hz2gb 1.1 of some of the
  "HZ" files posted on the news group alt.chinese.text.  I suspected that 
  most of the errors occured due to improper manual insertion of escape
  sequences, and/or using invalid GB codes, such as those for "space" ($2121).
  Such errors should not have occured if the files were first properly edited
  as GB codes, and then converted by an HZ encoder, such as gb2hz (preferably
  with the -t option.)

  To prevent some hanzi displayers from ill behaviour, the output stream
  should be or should be corrected to be valid mixed ASCII and GB sequences.

  The error recovery procedure is by no means unique, and may change in the
  future. Users should NOT regard the error recovery features as part of the
  HZ specification. 

  This program is free for general distribution.  
*/
	FILE	*fin, *fout;
	int	c1, c2, c3, c4;
	int	ASCIImode = TRUE;

	OPENINOUTFILES(&fin,&fout,src);
    
	while ((c1=fgetc(fin)) != EOF) {
		if (!pass8) 
			c1 = CLEAN7(c1);
		if (ASCIImode) {
			if (c1 == '~') {
				if ((c2 = fgetc(fin)) == EOF) {
					EOFerror(); 
					break;
				}
				if (!pass8) 
					c2 = CLEAN7(c2);
				switch (c2) {
					case '~' :	fputc('~', fout); 
							break;
					case '{' :	ASCIImode = FALSE; 
							break;
					case '\n':	/* line-continuation marker: eat it unless ... */
							if (termStyle) 
								fputc('\n', fout);
							break;
					default  :	ESCerror(c2);
							fputc('~', fout); 
							fputc(c2, fout); 
							break;
				}
			} else {
				if (LF2CR && c1=='\n') 
					c1 = '\r';
				fputc(c1, fout);
			}
		} else { /* GBmode */
			if (isprint(c1)) {
				if ((c2 = fgetc(fin)) == EOF) {
					EOFerror(); 
					break;
				}
				if (!pass8) 
					c2 = CLEAN7(c2);
				if (isGB1(c1) && isGB2(c2)) {
					GBtoSGB(c1, c2, &c3, &c4);
					fputc(c3, fout);
					fputc(c4, fout);
				} else if (c1 == '~' && c2 == '}') { 	/* 0x7E7D */
					ASCIImode = TRUE;
				} else if (isGB1U(c1) && isGB2(c2)) { 	/* 0x78?? - 0x7D?? */
					GBerror(c1, c2);		/* non-standard extended code? */
					fputc(HI(BOX), fout); 
					fputc(LO(BOX), fout);
				} else if (c1 == '~') {			/* 0x7E */
					GBerror(c1, c2);		/* undefined shift-out code? */
					ASCIImode = TRUE;		/* safer assumption? */
					fputc(c1, fout); 
					fputc(c2, fout);
				} else if (c1 == ' ') {			/* 0x20 */
					GBerror(c1, c2);		/* looks like artifacts of zwdos? */
					fputc(c2, fout);
				} else if (c2 == ' ') {			/* 0x20 */
					GBerror(c1, c2);		/* null image looks like "sp"? */
					fputc(HI(SPACE), fout); 
					fputc(LO(SPACE), fout);
				} else {				/* isprint(c1) && !isprint(c2)) */
					GBerror(c1, c2);		/* premature shift-out? */
					ASCIImode = TRUE;		/* safer assumption? */
					fputc(c1, fout); 
					fputc(c2, fout);
				}
			} else {   					/* !isprint(c1) */
				GBerror1(c1);				/* premature shift-out? */
				ASCIImode = TRUE;			/* safer assumption? */
				fputc(c1, fout);
			}
		}
	}

	CLOSEINOUTFILES(&fin,&fout,dest);
}



void GBtoSGB(int hi, int lo, int *hi1, int *lo1)
{
#ifdef DOS
	*hi1 = 0x80 | hi;
	*lo1 = 0x80 | lo;
#endif
#ifdef MAC
	*hi1 = 0x81 + (hi - 0x21)/2;
	if (hi%2 != 0) {
		*lo1 = 0x40 + (lo - 0x21);
		if (*lo1 >= 0x7F) 
			*lo1 += 1;
	} else
		*lo1 = 0x9F + (lo - 0x21);
#endif
}



void EOFerror()
{
	errorCount++;
	Syslog('m', "hz2gb: Unexpected EOF");
}


void ESCerror(int c)
{
	errorCount++;
	Syslog('m', "hz2gb: Invalid ASCII escape sequence:\"~%c\"", c);
}


void GBerror(int c1, int c2)
{
	errorCount++;
	Syslog('m', "hz2gb: Invalid GB code:\"%c%c\"(0x%4x)", c1,c2, DB(c1,c2));
}



void GBerror1(int c)
{
	errorCount++;
	Syslog('m', "hz2gb: Invalid GB code first byte:'%c'(0x%2x)", c, c);
}



void gb2hz(char *src,char **dest)
{
/*
  Copyright (C) 1989      Fung F. Lee

  sgb2hz: convert a Macintosh/CCDOS SGB file into a HZ file.

  This program is free for general distribution.  

*/
	FILE *fin, *fout;
	int c1, c2, c3, c4;
#ifdef MAC
	int hi;
#endif
	int GBmode = FALSE;
	int len = 0;

	OPENINOUTFILES(&fin,&fout,src);
  
	while ((c1=fgetc(fin)) != EOF) {
		if (notAscii(c1)) 
#ifdef MAC
		{
			hi = c1 & 0xF0;
			switch (hi) {
				case 0x80:
				case 0x90:
				case 0xA0:
						if (termStyle) {
							if (GBmode && len>MAXLEN-5) {
								fprintf(fout, "~}~\n");
								GBmode = FALSE; 
								len = 0;
							} else if (!GBmode && len>MAXLEN-7) {
								fprintf(fout, "~\n");
								GBmode = FALSE; len = 0;
							}
						}
						if (!GBmode) { /* switch to GB mode */
							fprintf(fout, "~{");
							len += 2;
						}
						GBmode = TRUE;
						c2 = fgetc(fin);
						mac2gb(c1, c2, &c3, &c4);
						fputc(c3, fout);
						fputc(c4, fout);
						len += 2;
						break;
				case 0xB0:
				case 0xC0:
				case 0xD0:
				case 0xE0:
						WriteError("gb2hz: ignored non-Ascii character: %2x\n", c1);
						break;
				case 0xF0:
						switch (c1) {
							case 0xFD:
							case 0xFE:
							case 0xFF:
									WriteError("gb2hz: ignored non-Ascii character: %2x\n", c1);
									break;
							default:
									c2 = fgetc(fin);
									WriteError("gb2hz: ignored user defined SGB code: %2x%2x\n", c1, c2);
									break;
						}
			}
		}
#endif
#ifdef DOS
		{
			if (termStyle) {
				if (GBmode && len>MAXLEN-5) {
					fprintf(fout, "~}~\n");
					GBmode = FALSE; 
					len = 0;
				} else if (!GBmode && len>MAXLEN-7) {
					fprintf(fout, "~\n");
					GBmode = FALSE; len = 0;
				}
			}
			if (!GBmode) { /* switch to GB mode */
				fprintf(fout, "~{");
				len += 2;
			}
			GBmode = TRUE;
			c2 = fgetc(fin);
			dos2gb(c1, c2, &c3, &c4);
			fputc(c3, fout);
			fputc(c4, fout);
			len += 2;
		}
#endif
		/* c1 is ASCII */
		else {
			if (GBmode) {
				fprintf(fout, "~}"); 
				len += 2;
			}
			/* assert(len<=MAXLEN-1) */
			if (termStyle && (len>MAXLEN-2 || (len>MAXLEN-3 && c1=='~'))) {
				fprintf(fout, "~\n");
				len = 0;
			}
			GBmode = FALSE;
			if (CR2LF && c1=='\r') 
				c1 = '\n';
			fputc(c1, fout);
			len++;
			if (c1=='\n') 
				len=0;
			else if (c1== '~') {
				fputc('~', fout); 
				len++;
			}
		}
	}
	if (GBmode) 
		fprintf(fout, "~}");

	CLOSEINOUTFILES(&fin,&fout,dest);
}



#ifdef MAC
void mac2gb(int hi, int lo, int *hi1, int *lo1)
{
	if (lo >= 0x9F) {
		*hi1 = 0x21 + (hi - 0x81) * 2 + 1;
		*lo1 = 0x21 + (lo - 0x9F);
	} else {
		*hi1 = 0x21 + (hi - 0x81) * 2;
		if (lo > 0x7F) 
			lo--;
		*lo1 = 0x21 + (lo - 0x40);
	}
}
#endif



#ifdef DOS
void dos2gb(int hi, int lo, int *hi1, int *lo1)
{
	*hi1 = hi - 0x80;
	*lo1 = lo - 0x80;
}
#endif

