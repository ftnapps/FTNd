/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Common utilities - character set conversion
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
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
#include "records.h"
#include "common.h"
#include "clcomm.h"


#ifndef BUFSIZ
#define BUFSIZ 512
#endif


char *oldfilemap=NULL;
char maptab[] = {
"\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017"
"\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037"
"\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057"
"\060\061\062\063\064\065\066\067\070\071\072\073\074\075\076\077"
"\100\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117"
"\120\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137"
"\140\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157"
"\160\161\162\163\164\165\166\167\170\171\172\173\174\175\176\177"
"\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217"
"\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237"
"\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257"
"\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277"
"\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317"
"\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337"
"\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357"
"\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377"
};



static int ctoi(char *);
static int ctoi(char *s)
{
	int i;

	if (!strncmp(s,"0x",2)) 
		sscanf(s+2,"%x",&i);
	else if (*s == '0') 
		sscanf(s,"%o",&i);
	else if (strspn(s,"0123456789") == strlen(s)) 
		sscanf(s,"%d",&i);
	else 
		i=0;
	return i;
}



static int getmaptab(char *);
static int getmaptab(char *maptab_name)
{
	FILE	*fp;
	char	buf[BUFSIZ], *p, *q;
	int	in, on;

	if ((fp = fopen(maptab_name, "r")) == NULL) {
		WriteError("$can't open mapchan file \"%s\" ", maptab_name);
		return 0;
	}

	while (fgets(buf, sizeof(buf)-1, fp)) {
		p = strtok(buf," \t\n#");
		q = strtok(NULL," \t\n#");
		if (p && q) {
			in = ctoi(p);
			on = ctoi(q);
			if (in && on) 
				maptab[in] = on;
		}
	}
	fclose(fp);

	return 0;
}



char *strnkconv(const char *src, int incode, int outcode, int n)
{
	char		ki[10], ko[10];
	int		kolen;
	static char	*dest;
	int		destlen;
	int		i;

	outcode = getkcode(outcode, ki, ko);
	kolen = strlen(ko);

	dest = strkconv(src, incode, outcode);
	destlen = strlen(dest);

	if(destlen >= kolen && destlen > strlen(src)) {
		for(i = 0; i < kolen; i++)
			*(dest + n - 1 + i) = ko[i];
		*(dest + n) = '\0';
	}
  
	return dest;
}



char *strkconv(const char *src, int incode, int outcode)
{
	static char *dest;

	if ((incode==outcode) && (incode!=CHRS_NOTSET) && (incode!=CHRS_AUTODETECT))
		return (char *)src;

	if (!src)
		return NULL;

	if((incode == CHRS_AUTODETECT) || (incode == CHRS_NOTSET)) {
		if (LANG_BITS == 16) {
			incode = iso2022_detectcode((char *)src,incode);
		}
	}

	if(dest)
		free(dest);
	/* FIXME: should be 
	 * dest = (char *)malloc((strlen(src) + 1) + (6 * "number of \n + 1"));
	 */
	dest = (char *)malloc(((strlen(src) + 1) * 2) + 6 );

	kconv((char *)src, &dest, incode, outcode);
	return dest;
}



void kconv(char *in, char **out, int incode, int outcode)
{
	char ki[10], ko[10];

	outcode = getkcode(outcode, ki, ko);
	if (incode == outcode) 
		noconv(in,out);
	else {
		switch (incode) {
			case CHRS_NOTSET :	noconv(in,out); 
						break;
			case CHRS_ASCII :	noconv(in,out); 
						break;
			case CHRS_BIG5 :
						switch (outcode) {
							default :	noconv(in,out); break;
						}
						break;
			case CHRS_CP424 :
				switch (outcode) {
					case CHRS_CP862 :	eight2eight(in,out,(char *)CP424__CP862); break;
					case CHRS_ISO_8859_8 :	eight2eight(in,out,(char *)CP424__ISO_8859_8); break;
					default :		noconv(in,out); break;
				}
			        break;
			case CHRS_CP437 :
				switch (outcode) {
					case CHRS_ISO_8859_1 :
					case CHRS_ISO_8859_15:	eight2eight(in,out,(char *)CP437__ISO_8859_1); break;
					case CHRS_MACINTOSH :	eight2eight(in,out,(char *)CP437__MACINTOSH); break;
					default : 		noconv(in,out); break;
				}
				break;
			case CHRS_CP850 :
				switch (outcode) {
					case CHRS_ISO_8859_1 :
					case CHRS_ISO_8859_15:	eight2eight(in,out,(char *)CP850__ISO_8859_1); break;
					case CHRS_MACINTOSH :   eight2eight(in,out,(char *)CP850__MACINTOSH); break;
					default :               noconv(in,out); break;
				}
				break;
			case CHRS_CP852 :
				switch (outcode) {
					case CHRS_FIDOMAZOVIA : eight2eight(in,out,(char *)CP852__FIDOMAZOVIA); break;
					case CHRS_ISO_8859_2 :  eight2eight(in,out,(char *)CP852__ISO_8859_2); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_CP862 :
				switch (outcode) {
					case CHRS_CP424 :	eight2eight(in,out,(char *)CP862__CP424); break;
					case CHRS_ISO_8859_8 :	eight2eight(in,out,(char *)CP862__ISO_8859_8); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_CP866 :
				switch (outcode) {
					case CHRS_ISO_8859_5 :	eight2eight(in,out,(char *)CP866__ISO_8859_5); break;
					case CHRS_KOI8_R :
					case CHRS_KOI8_U :	eight2eight(in,out,(char *)CP866__KOI8); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_CP895 :
				switch (outcode) {
					case CHRS_ISO_8859_2 :	eight2eight(in,out,(char *)CP895__ISO_8859_2); break;
					case CHRS_CP437 :	eight2eight(in,out,(char *)CP895__CP437); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_EUC_JP :
				switch (outcode) {
					case CHRS_EUC_JP :	euc2euc(in,out,incode,0); break;
					case CHRS_ISO_2022_JP :	euc2seven(in,out,incode,ki,ko); break;
					case CHRS_NEC :	  	euc2seven(in,out,incode,ki,ko); break;
					case CHRS_SJIS :	euc2shift(in,out,incode,0); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_EUC_KR :
				switch (outcode) {
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_FIDOMAZOVIA :
				switch (outcode) {
					case CHRS_CP852 : 	eight2eight(in,out,(char *)FIDOMAZOVIA__CP852); break;
					case CHRS_ISO_8859_2 :	eight2eight(in,out,(char *)FIDOMAZOVIA__ISO_8859_2); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_GB :
				switch (outcode) {
					case CHRS_HZ :		gb2hz(in,out); break;
					default :		noconv(in,out); break;
				}
			case CHRS_HZ :
				switch (outcode) {
					case CHRS_GB :		hz2gb(in,out); break;
					default :		noconv(in,out); break;
				}
			case CHRS_ISO_11 :
				switch (outcode) {
					case CHRS_ISO_8859_1 :
					case CHRS_ISO_8859_15:	eight2eight(in,out,(char *)ISO_11__ISO_8859_1); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_4 :
				switch (outcode) {
					case CHRS_ISO_8859_1 :
					case CHRS_ISO_8859_15:	eight2eight(in,out,(char *)ISO_4__ISO_8859_1); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_60 :
				switch (outcode) {
					case CHRS_ISO_8859_1 :
					case CHRS_ISO_8859_15:	eight2eight(in,out,(char *)ISO_60__ISO_8859_1); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_2022_CN :
				switch (outcode) {
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_2022_JP :
				switch (outcode) {
					case CHRS_EUC_JP :	seven2euc(in,out); break;
					case CHRS_ISO_2022_JP : seven2seven(in,out,ki,ko); break;
					case CHRS_NEC :	  	seven2seven(in,out,ki,ko); break;
					case CHRS_SJIS :	seven2shift(in,out); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_2022_KR :
				switch (outcode) {
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_2022_TW :
				switch (outcode) {
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_8859_1 :
			case CHRS_ISO_8859_15:
				switch (outcode) {
					case CHRS_CP437 :	eight2eight(in,out,(char *)ISO_8859_1__CP437); break;
					case CHRS_CP850 :	eight2eight(in,out,(char *)ISO_8859_1__CP850); break;
					case CHRS_MACINTOSH :  	eight2eight(in,out,(char *)ISO_8859_1__MACINTOSH); break;
					case CHRS_ISO_8859_1 :
					case CHRS_ISO_8859_15:	noconv(in,out); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_8859_2 :
				switch (outcode) {
					case CHRS_CP852 :	eight2eight(in,out,(char *)ISO_8859_2__CP852); break;
					case CHRS_CP895 :	eight2eight(in,out,(char *)ISO_8859_2__CP895); break;
					case CHRS_FIDOMAZOVIA : eight2eight(in,out,(char *)ISO_8859_2__FIDOMAZOVIA); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_8859_3 :
				switch (outcode) {
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_8859_4 :
				switch (outcode) {
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_8859_5 :
				switch (outcode) {
					case CHRS_CP866 :   	eight2eight(in,out,(char *)ISO_8859_5__CP866); break;
					case CHRS_KOI8_R :
					case CHRS_KOI8_U :  	eight2eight(in,out,(char *)ISO_8859_5__KOI8); break;
					case CHRS_MIK_CYR : 	eight2eight(in,out,(char *)ISO_8859_5__MIK_CYR); break;
					default :	      	noconv(in,out); break;
				}
				break;
			case CHRS_ISO_8859_6 :
				switch (outcode) {
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_8859_7 :
				switch (outcode) {
					default : 		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_8859_8 :
				switch (outcode) {
					case CHRS_CP424 :	eight2eight(in,out,(char *)ISO_8859_8__CP424); break;
					case CHRS_CP862 :	eight2eight(in,out,(char *)ISO_8859_8__CP862); break;
					default :	    	noconv(in,out); break;
				}
				break;
			case CHRS_ISO_8859_9 :
				switch (outcode) {
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_ISO_8859_10 :
				switch (outcode) {
					default :             	noconv(in,out); break;
				}
				break;
			case CHRS_ISO_8859_11 :
				switch (outcode) {
					default :             	noconv(in,out); break;
				}
				break;
			case CHRS_KOI8_R :
			case CHRS_KOI8_U :
				switch (outcode) {
					case CHRS_CP866 :	eight2eight(in,out,(char *)KOI8__CP866); break;
					case CHRS_ISO_8859_5 :	eight2eight(in,out,(char *)KOI8__ISO_8859_5); break;
					case CHRS_MIK_CYR :	eight2eight(in,out,(char *)KOI8__MIK_CYR); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_MACINTOSH :
				switch (outcode) {
					case CHRS_CP437 :	eight2eight(in,out,(char *)MACINTOSH__CP437); break;
					case CHRS_CP850 :	eight2eight(in,out,(char *)MACINTOSH__CP850); break;
					case CHRS_ISO_8859_1 :
					case CHRS_ISO_8859_15:	eight2eight(in,out,(char *)MACINTOSH__ISO_8859_1); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_MIK_CYR :
				switch (outcode) {
					case CHRS_ISO_8859_5 :	eight2eight(in,out,(char *)MIK_CYR__ISO_8859_5); break;
					case CHRS_KOI8_R :
					case CHRS_KOI8_U :	eight2eight(in,out,(char *)MIK_CYR__KOI8); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_NEC :
				switch (outcode) {
					case CHRS_EUC_JP :	seven2euc(in,out); break;
					case CHRS_ISO_2022_JP : seven2seven(in,out,ki,ko); break;
					case CHRS_NEC :	  	seven2seven(in,out,ki,ko); break;
					case CHRS_SJIS :	seven2shift(in,out); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_SJIS :
				switch (outcode) {
					case CHRS_EUC_JP :	shift2euc(in,out,incode,0); break;
					case CHRS_ISO_2022_JP : shift2seven(in,out,incode,ki,ko); break;
					case CHRS_NEC :	  	shift2seven(in,out,incode,ki,ko); break;
					case CHRS_SJIS :	shift2shift(in,out,incode,0); break;
					default :		noconv(in,out); break;
				}
				break;
			case CHRS_UTF_7 :
				utf7_to_eight(in,out,&outcode);
				break;
			case CHRS_UTF_8 :
				utf8_to_eight(in,out,&outcode);
				break;

			case CHRS_ZW :
				switch (outcode) {
					case CHRS_HZ :		zw2hz(in,out); break;
					case CHRS_GB :		zw2gb(in,out); break;
					default :             	noconv(in,out); break;
				}
				break;
			default :				noconv(in,out); break;
		}
	}
}



int getkcode(int code,char ki[],char ko[])
{
	if (code == CHRS_ISO_2022_CN) {
		strcpy(ki,"$A");
		strcpy(ko,"(T");
	} else if (code == CHRS_ISO_2022_JP) {
		strcpy(ki,"$B");
		strcpy(ko,"(B");
	} else if (code == CHRS_ISO_2022_KR) {
		strcpy(ki,"$(C");
		strcpy(ko,"(B");
	} else if (code == CHRS_ISO_2022_TW) {
		strcpy(ki,"$(G");
		strcpy(ko,"(B");
	}
	return code;	
}



int SkipESCSeq(FILE *in,int temp,int *intwobyte)
{
	int tempdata;

	tempdata = *intwobyte;
	if (temp == '$' || temp == '(')
		fgetc(in);
	if (temp == 'K' || temp == '$')
		*intwobyte = TRUE;
	else
		*intwobyte = FALSE;
	if (tempdata == *intwobyte)
		return FALSE;
	else
		return TRUE;
}



void noconv(char *in, char **out)
{
	char *p;

	p=*out;
	while (*in) 
		*p++=*in++;
	*p='\0';
}



void eight2eight(char *in,char **out, char *filemap)
{
	char	*p;
	int	i;

	if (oldfilemap != filemap) {
		oldfilemap = filemap;
		filemap = xstrcpy(getenv("MBSE_ROOT"));
		filemap = xstrcat(filemap, (char *)"/etc/maptabs/");
		filemap = xstrcat(filemap, oldfilemap);

		for (i = 0; i < 256; i++)
			maptab[i] = (unsigned char)i;

		getmaptab(filemap);
	}	
	p=*out;
	while (*in) {
		*p=maptab[*in & 0xff];
		in++;
		p++;
	}
	*p='\0';
}



int iso2022_detectcode(char *in,int whatcode)
{
	int c=0;

	while (((whatcode == CHRS_NOTSET) || (whatcode==CHRS_AUTODETECT)) && (*in)) {
		if ((c = (unsigned int)(*in++))) {
			if (c == ESC) {
				c = (unsigned int)(*in++);
				if (c == '$') {
					c = (unsigned int)(*in++);
					switch (c) {
						case 'A' :	whatcode = CHRS_ISO_2022_CN; break;
						case 'B' :
						case '@' :	whatcode = CHRS_ISO_2022_JP; break;
						case '(' :
						case ')' :	c = (unsigned int)(*in++);
								switch (c) {
									case 'A' :	whatcode = CHRS_ISO_2022_CN; break;
									case 'C' :	whatcode = CHRS_ISO_2022_KR; break;
									case 'D' :	whatcode = CHRS_ISO_2022_JP; break;
									case 'E' :	whatcode = CHRS_ISO_2022_CN; break;
									case 'G' :
									case 'H' :
									case 'I' :
									case 'J' :
									case 'K' :
									case 'L' :
									case 'M' :	whatcode = CHRS_ISO_2022_TW; break;
									case 'X' :	whatcode = CHRS_ISO_2022_CN; break;
									default:	break;
								} 
								break;
						case '*' : 	c = (unsigned int)(*in++);
								switch (c) {
									case 'H' :
									case 'X' :	whatcode = CHRS_ISO_2022_CN; break;
									default:	break;
								} 
								break;
						case '+' :	 c = (unsigned int)(*in++);
								switch (c) {
									case 'H' :
									case 'I' :
									case 'J' :
									case 'K' :
									case 'L' :
									case 'M' :
									case 'X' :      whatcode = CHRS_ISO_2022_CN; break;
									default:        break;
								}
								break;
						default:	break;
					}
				}
			} else if (whatcode == CHRS_NOTSET)
				return whatcode;
#if (LANG_DEFAULT == LANG_JAPAN)
			else if ((c >= 129 && c <= 141) || (c >= 143 && c <= 159))
				whatcode = CHRS_SJIS;
			else if (c == 142) {
				c = (unsigned int)(*in++);
				if ((c >= 64 && c <= 126) || (c >= 128 && c <= 160) || (c >= 224 && c <= 252))
					whatcode = CHRS_SJIS;
				else if (c >= 161 && c <= 223)
					whatcode = CHRS_AUTODETECT;
			} else if (c >= 161 && c <= 223) {
				c = (unsigned int)(*in++);
				if (c >= 240 && c <= 254)
					whatcode = CHRS_EUC_JP;
				else if (c >= 161 && c <= 223)
					whatcode = CHRS_AUTODETECT;
				else if (c >= 224 && c <= 239) {
					whatcode = CHRS_AUTODETECT;
					while (c >= 64 && c != EOF && whatcode == CHRS_AUTODETECT) {
						if (c >= 129) {
							if (c <= 141 || (c >= 143 && c <= 159))
								whatcode = CHRS_SJIS;
							else if (c >= 253 && c <= 254)
								whatcode = CHRS_EUC_JP;
						}
						c = (unsigned int)(*in++);
					}
				} else if (c <= 159)
					whatcode = CHRS_SJIS;
			} else if (c >= 240 && c <= 254)
				whatcode = CHRS_EUC_JP;
			else if (c >= 224 && c <= 239) {
				c = (unsigned int)(*in++);
				if ((c >= 64 && c <= 126) || (c >= 128 && c <= 160))
					whatcode = CHRS_SJIS;
				else if (c >= 253 && c <= 254)
					whatcode = CHRS_EUC_JP;
				else if (c >= 161 && c <= 252)
					whatcode = CHRS_AUTODETECT;
			}
#endif /* (LANG_DEFAULT == LANG_JAPAN) */
		}
	}
	return whatcode;
}



char *hdrnconv(char *s, int incode, int outcode, int n)
{
	char ki[10],ko[10];
	int kolen;
	static char *dest;
	int destlen;
	int i;

	getkcode(outcode, ki, ko);
	kolen = strlen(ko);
	dest = hdrconv(s, incode, outcode);
	destlen = strlen(dest);

	if(destlen >= kolen && destlen > n) {
		for(i = 0; i < kolen; i++)
			*(dest + n - 1 - kolen + i) = ko[i];
		*(dest + n) = '\0';
	}

	return dest;
}



char *hdrconv(char *s, int incode, int outcode)
{
#define BCODAGE		1
#define QCODAGE		2

	char	ttbuf[1024];
	char	*iptr, *tptr;
	char	*xbuf=NULL, *buf=NULL, *q;
	int	codage;

	iptr = s;
	while (*iptr) {
		if (!strncmp(iptr,"=?",2)) {
			q=strchr(iptr+2,'?');
			if (q) {
				incode=getcode(iptr+2);
				if (incode==CHRS_NOTSET) 
					return s;
				iptr=q;
			} else {
				return s;
			}
			if (!strncasecmp(iptr,"?Q?",3)) {
				codage = QCODAGE;
				iptr+=3;
			} else if (!strncasecmp(iptr,"?B?",3)) {
				codage = BCODAGE;
				iptr+=3;
			} else {
				iptr=xstrcpy(iptr);
				*(iptr+3)='\0';
				Syslog('+', "mimehdr_decode: unknown codage %s",iptr);
				return s;
			}
			tptr = ttbuf;
			while ((*iptr) && (strncmp(iptr,"?=",2))) 
				*tptr++ = *iptr++;
			*tptr = '\0';
			if (!strncmp(iptr,"?=",2)) { 
				iptr++; 
				iptr++; 
			}
			if (codage==QCODAGE) {
				while ((q = strchr(ttbuf, '_'))) 
					*q=' ';
				xbuf=xstrcat(xbuf,qp_decode(ttbuf));
			} else if (codage==BCODAGE) {
				xbuf=xstrcat(xbuf,b64_decode(ttbuf));
			}
                } else { /* not coded */
			*ttbuf=*iptr;
			*(ttbuf+1)='\0';
			xbuf=xstrcat(xbuf,ttbuf);
			iptr++;
		}
	}
	buf=strkconv(xbuf, incode, outcode);
	return buf;
}

