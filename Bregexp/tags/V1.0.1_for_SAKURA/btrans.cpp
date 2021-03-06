//
// btrans.cc
//
//   translate front-end
////////////////////////////////////////////////////////////////////////////////
//  1999.11.24   update by Tatsuo Baba
//
//  You may distribute under the terms of either the GNU General Public
//  License or the Artistic License, as specified in the perl_license.txt file.
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define KANJI
#include "global.h"
#include "sv.h"
#include "intreg.h"

int trans(regexp* rx,char* target,char* targetendp,char *msg);

regexp *trcomp(char* res,char* resend,char* rp,char* rpend,
			   int* flag,char *msg);
static SV *cvchar(char* str,char* strend);
static char specchar(char* p,int *next);


// compile translate string 
regexp *trcomp(char* str,char* strend,char* rp,char* rpend,
			   int* flag,char *msg)
{
	int slen = strend - str;
	int rlen = rpend - rp;
	if (slen < 1)
		return NULL;
    register char *p = str;
    register char *pend = strend;

	regexp *rx = (regexp*) new char[sizeof(regexp)];
	if (rx == NULL) {
		strcpy(msg,"out of space trcomp");
		return NULL;
	}

	memset(rx,0,sizeof(regexp));
	rx->pmflags = *flag | PMf_TRANSLATE;

	SV *tstr = cvchar(str,strend);
	SV *rstr = cvchar(rp,rpend);



    int tlen = SvCUR(tstr);
    rlen = SvCUR(rstr);
    register U8 *t = (U8*)SvPVX(tstr);
    register U8 *r = (U8*)SvPVX(rstr);

    register int i; /* indexes t */
    register int j; /* indexes j */
    register int k; /* indexes tbl */
    int lastrch = -1;
    int tbl_size = 256;
      
    int del_char;
    int complement;
    int kanji;

    /* the even index holds the t-char(in 2byte), and the odd index
       holds the r-char(in 2 byte) if t-char is to be removed, then
       r-char is -2. */
    register U16 *tbl;

	tbl = new U16[tbl_size];
  
    
    complement	= rx->pmflags & PMf_TRANS_COMPLEMENT;
    del_char	= rx->pmflags & PMf_TRANS_DELETE;
    kanji	= rx->pmflags & PMf_KANJI;

    for (i = 0, j = 0, k = 0; i < tlen; ) {
	U32 tch, rch;
	if (kanji && iskanji(t[i]) && i < tlen-1) {
		tch = ((U8)t[i] <<8) | (U8)t[i+1];
	    i+=2;
	} else {
	    tch = (U8)t[i]; 
	    i++;
	}
	if (j >= rlen) {
	    if (del_char) rch = (unsigned)-2;
	    else rch = lastrch;
	} else {
	    if (kanji && iskanji(r[j]) && j < rlen-1) {
		rch = ((U8)r[j] <<8) | (U8)r[j+1];
		j += 2;
	    } else {
		rch = (U8)r[j];
		j++;
	    }
	    lastrch = rch;
	}
	if (k >= tbl_size) {
		U16 *tp = new U16[tbl_size+256];
		memcpy(tp,tbl,tbl_size * sizeof(U16));
		delete [] tbl;
		tbl = tp;
	    tbl_size += 256;
	}
	tbl[k++] = tch;
	tbl[k++] = rch;
    }
    if (k >= tbl_size) {
		U16 *tp = new U16[tbl_size+4];
		memcpy(tp,tbl,tbl_size * sizeof(U16));
		delete [] tbl;
		tbl = tp;
	    tbl_size += 4;
    }
    /* mark the end */
    tbl[k++] = (U16)-1;
    tbl[k++] = (U16)-1;
    rx->transtblp = (char*)tbl;
    sv_free(tstr);
    sv_free(rstr);

	return rx;

}

static SV *cvchar(char* str,char* strend)
{
	int next;
	char ender;
	U16 lastch = 0;
	int len = strend - str;
	char *p = str;
	char *pend = strend;
	SV *dst = newSVpv("",0);
	while (p < pend) { 
		if (*p != '\\' && *p != '-') {	// no magic char ?
			lastch = *p;
			if (iskanji(*p)) {		// kanji ?
				lastch = ((U8)*p <<8) | (U8)p[1];
				sv_catpvn(dst,p,2);
				p++;
			} else
				sv_catpvn(dst,p,1);
			p++;
			continue;
		}
		p++;
		if (p >= pend) {
			sv_catpvn(dst,p-1,1);
			break;
		}
		if (p[-1] == '-') {		// - ?
			char* tp = p -1;
			U16 toch;
			if (iskanji(*p)) {		// kanji ?
				toch = ((U8)*p <<8) | (U8)p[1];
				p += 2;
			} else {
				toch = *p++;
				if (p[-1] == '\\') {
					if (p +1 >= pend) {
						sv_catpvn(dst,p-1,2);
						break;
					}
					toch = specchar(p,&next);
					p += next;
				}
			}
					
			if (lastch >= toch || toch - lastch > 255) {
				sv_catpvn(dst,tp,p - tp);
				continue;
			}
			int clen = toch > 255 ? 2 :1;
			for (lastch++;toch >= lastch;lastch++) {
				if (clen ==1)
					sv_catpvn(dst,(char*)&lastch,clen);
				else {
					char ch[2];
					ch[0] = lastch >> 8;
					ch[1] = (char)lastch;
					sv_catpvn(dst,ch,clen);
				}
			}
			lastch--;
			continue;
		}

		if (*p == '\\') {
			sv_catpvn(dst,p,1);
			p++;
			continue;
		}
		ender = specchar(p,&next);
		sv_catpvn(dst,&ender,1);
		lastch = ender;
		p += next;
	}


	return dst;
}


static char specchar(char* p,int *next)
{
	char ender;		
	int numlen = 0;		
	switch (*p++) {
    case '/':
		ender = '/';
		break;
    case 'n':
		ender = '\n';
		break;
    case 'r':
		ender = '\r';
		break;
    case 't':
		ender = '\t';
		break;
    case 'f':
		ender = '\f';
		break;
    case 'e':
		ender = '\033';
		break;
    case 'a':
		ender = '\007';
		break;
    case 'x':
		ender = (char)scan_hex(p, 2, &numlen);
		break;
    case 'c':
		ender = *p++;
		if (isLOWER(ender))
		    ender = toUPPER(ender);
		ender ^= 64;
		break;
    case '0':
		--p;
	    ender = (char)scan_oct(p, 3, &numlen);
		break;
    case '\0':
		/* FALL THROUGH */
    default:
		ender = p[-1];
    }
	*next = numlen + 1;
	return ender;
}

void sv_catkanji(SV *sv,U32 tch);


int trans(regexp* rx,char* target,char* targetendp,char *msg)
{
    register short *tbl;
    register U8 *s;
    register U8 *send;
    register int matches = 0;
    register int squash = rx->pmflags & PMf_TRANS_SQUASH;
    int len;
    U32 last_rch;
    // This variable need, doesn't it?
    // replase sv ;
    SV *dest_sv = newSVpv("",0);

    int del_char = rx->pmflags & PMf_TRANS_DELETE;
    int complement = rx->pmflags & PMf_TRANS_COMPLEMENT;
    int kanji = rx->pmflags & PMf_KANJI;


    tbl = (short*)rx->transtblp;
    s = (U8*)target;
	len = targetendp - target;
    if (!len)
		return 0;
    send = s + len;
    while (s < send) {
		U32 tch, rch;
		U8 *next_s;
		U16 *tp;
		int matched;
		if (kanji && iskanji(*s) && s < send-1) {
		    tch = ((U8) *s)<<8 | ((U8) *(s+1));
		    next_s = s+2;
		} else {
		    tch = *(U8*)s;
		    next_s = s+1;
		}
		/* look for ch in tbl */
		if (!complement) {
			for (tp = (U16*)tbl; *tp != (U16)(-1); tp += 2) {
				if (*tp == tch) break;
			}
			matched = (*tp != (U16)(-1));
			rch = tp[1];
		} else {
			for (tp = (U16*)tbl; *tp != (U16)(-1); tp += 2) {
				if (*tp == tch) break;
			}
			matched = (*tp == (U16)(-1));
			rch = (U16)(del_char ? -2 : -1);
		}

	
		if (!matched) {
			sv_catkanji(dest_sv, tch);
		} else {
			matches++;
			if (rch == (U16)(-2)) {
			/* delete this character */
			} else if (squash) {
				if (last_rch == (rch==(U16)(-1)?tch:rch)) {
					;	// delete this char
				} else {
					sv_catkanji(dest_sv, rch == (U16)(-1) ? tch : rch);
//					matches++;
			}
			} else {
				sv_catkanji(dest_sv, rch==(U16)(-1) ? tch : rch);
//				matches++;
			}
		}
		last_rch = ((rch==(U16)(-1)||rch==(U16)(-2)) ? tch : rch);
		s = next_s;
    }
//    matches += (s-(U8*)target) - dlen;	/* account for disappeared chars */
	

    rx->outp = SvPVX(dest_sv);
    rx->outendp = rx->outp + SvCUR(dest_sv);
	dest_sv->xpv_pv = NULL;
	sv_free(dest_sv);


	return matches;
}

void sv_catkanji(SV *sv,U32 tch)
{
 
	if (tch < 256) {
		sv_catpvn(sv,(char*)&tch,1);
		return ;
	}
	char ch[2];
	ch[0] = tch >> 8;
	ch[1] = (char)tch;
	sv_catpvn(sv,ch,2);
	return ;
}     
