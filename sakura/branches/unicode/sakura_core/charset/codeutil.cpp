#include "stdafx.h"
#include "codeutil.h"
#include "charcode.h"
#include <mbstring.h>

/*!
	@brief 拡張版 SJIS→JIS変換

	SJISコードをJISに変換する．その際，JISに対応領域のないIBM拡張文字を
	NEC選定IBM拡張文字に変換する．

	Shift_JIS fa40〜fc4b の範囲の文字は 8754〜879a または ed40〜eefc に
	散在する文字に変換された後に，JISに変換されます．
	
	@param pszSrc [in] 変換する文字列へのポインタ (Shift JIS)
	
	@author すい
	@date 2002.10.03 1文字のみ扱い，変換まで行うように変更 genta
*/
unsigned short _mbcjmstojis_ex( unsigned char* pszSrc )
{
	unsigned int	tmpw;	/* ← int が 16 bit 以上である事を期待しています。 */
	
	if(	_IS_SJIS_1(* pszSrc    ) &&	/* Shift_JIS 全角文字の 1バイト目 */
		_IS_SJIS_2(*(pszSrc+1) )	/* Shift_JIS 全角文字の 2バイト目 */
	){	/* Shift_JIS全角文字である */
		tmpw = ( ((unsigned int)*pszSrc) << 8 ) | ( (unsigned int)*(pszSrc + 1) );
		if(
			( *pszSrc == 0x0fa ) ||
			( *pszSrc == 0x0fb ) ||
			( ( *pszSrc == 0x0fc ) && ( *(pszSrc+1) <= 0x04b ) )
		) {		/* fa40〜fc4b の文字である。 */
			/* 文字コード変換処理 */
			if		  ( tmpw <= 0xfa49 ) {	tmpw -= 0x0b51;	}	/* fa40〜fa49 → eeef〜eef8 (��〜��) */
			else	if( tmpw <= 0xfa53 ) {	tmpw -= 0x72f6;	}	/* fa4a〜fa53 → 8754〜875d (�T〜�]) */
			else	if( tmpw <= 0xfa57 ) {	tmpw -= 0x0b5b;	}	/* fa54〜fa57 → eef9〜eefc (¬〜��) */
			else	if( tmpw == 0xfa58 ) {	tmpw  = 0x878a;	}	/* �� */
			else	if( tmpw == 0xfa59 ) {	tmpw  = 0x8782;	}	/* �� */
			else	if( tmpw == 0xfa5a ) {	tmpw  = 0x8784;	}	/* �� */
			else	if( tmpw == 0xfa5b ) {	tmpw  = 0x879a;	}	/* ∵ */
			else	if( tmpw <= 0xfa7e ) {	tmpw -= 0x0d1c;	}	/* fa5c〜fa7e → ed40〜ed62 (�@〜�b) */
			else	if( tmpw <= 0xfa9b ) {	tmpw -= 0x0d1d;	}	/* fa80〜fa9b → ed63〜ed7e (�c〜�~) */
			else	if( tmpw <= 0xfafc ) {	tmpw -= 0x0d1c;	}	/* fa9c〜fafc → ed80〜ede0 (��〜��) */
			else	if( tmpw <= 0xfb5b ) {	tmpw -= 0x0d5f;	}	/* fb40〜fb5b → ede1〜edfc (�瘁`��) */
			else	if( tmpw <= 0xfb7e ) {	tmpw -= 0x0d1c;	}	/* fb5c〜fb7e → ee40〜ee62 (�@〜�b) */
			else	if( tmpw <= 0xfb9b ) {	tmpw -= 0x0d1d;	}	/* fb80〜fb9b → ee63〜ee7e (�c〜�~) */
			else	if( tmpw <= 0xfbfc ) {	tmpw -= 0x0d1c;	}	/* fb9c〜fbfc → ee80〜eee0 (��〜��) */
			else{							tmpw -= 0x0d5f;	}	/* fc40〜fc4b → eee1〜eeec (�瘁`��) */
		}
		return (unsigned short) _mbcjmstojis( tmpw );
	}
	return 0;
}

