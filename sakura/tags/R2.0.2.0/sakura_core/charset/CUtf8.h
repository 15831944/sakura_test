#pragma once

#include "CCodeBase.h"
#include "CShiftJis.h"
#include "charset/codeutil.h"


class CUtf8 : public CCodeBase{
public:

	//CCodeBaseインターフェース
	EConvertResult CodeToUnicode(const CMemory& cSrc, CNativeW* pDst){	//!< 特定コード → UNICODE    変換
		*pDst->_GetMemory()=cSrc; return UTF8ToUnicode(pDst->_GetMemory());
	}
	EConvertResult UnicodeToCode(const CNativeW& cSrc, CMemory* pDst){	//!< UNICODE    → 特定コード 変換
		*pDst=*cSrc._GetMemory(); return UnicodeToUTF8(pDst);
	}
	void GetBom(CMemory* pcmemBom);																			//!< BOMデータ取得
	void GetEol(CMemory* pcmemEol, EEolType eEolType){ CShiftJis::S_GetEol(pcmemEol,eEolType); }	//!< 改行データ取得
	EConvertResult _UnicodeToHex(const wchar_t* cSrc, const int iSLen, TCHAR* pDst, const bool CESU8Mode);			//!< UNICODE → Hex 変換
	EConvertResult UnicodeToHex(const wchar_t* ps, const int nsl, TCHAR* pd){ return _UnicodeToHex(ps, nsl, pd, false); }

public:
	// UTF-8 / CESU-8 <-> Unicodeコード変換
	//2007.08.13 kobake 追加
	//2009.01.08        CESU-8 に対応
	static EConvertResult _UTF8ToUnicode( CMemory* pMem, bool bCESU8Mode );
	static EConvertResult _UnicodeToUTF8( CMemory* pMem, bool bCESU8Mode );
	inline static EConvertResult UTF8ToUnicode( CMemory* pmem ){ return _UTF8ToUnicode(pmem,false); }	// UTF-8 -> Unicodeコード変換
	inline static EConvertResult CESU8ToUnicode( CMemory* pmem ){ return _UTF8ToUnicode(pmem,true); }	// CESU-8 -> Unicodeコード変換
	inline static EConvertResult UnicodeToUTF8( CMemory* pmem ){ return  _UnicodeToUTF8(pmem,false); }	// Unicode → UTF-8コード変換
	inline static EConvertResult UnicodeToCESU8( CMemory* pmem ){ return _UnicodeToUTF8(pmem,true); }	// Unicode → CESU-8コード変換

protected:

	//変換の実装
	// 2008.11.10 変換ロジックを書き直す
	inline static int _Utf8ToUni_char( const unsigned char*, const int, unsigned short*, bool bCESU8Mode );
	static int Utf8ToUni( const char*, const int, wchar_t*, bool bCESU8Mode );
	inline static int _UniToUtf8_char( const unsigned short*, const int, unsigned char*, const bool bCSU8Mode );
	static int UniToUtf8( const wchar_t*, const int, char*, bool* pbError, bool bCSU8Mode );
};



/*!
	UTF-8 の一文字変換

	UTF-8 と CESU-8 とを場合分けして、それぞれ変換する

	高速化のため、インライン化

*/
inline int CUtf8::_Utf8ToUni_char( const unsigned char* pSrc, const int nSrcLen, unsigned short* pDst, bool bCESUMode )
{
	int nret;

	if( nSrcLen < 1 ){
		return 0;
	}

	if( bCESUMode != true ){
		// UTF-8 の処理
		if( nSrcLen < 4 ){
			pDst[0] = static_cast<unsigned short>(DecodeUtf8(pSrc, nSrcLen) & 0x0000ffff);
			nret = 1;
		}else if( nSrcLen == 4 ){
			// UTF-8 サロゲート領域の処理
			wchar32_t wc32 = DecodeUtf8( pSrc, 4 );
			EncodeUtf16Surrog( wc32, pDst );
			nret = 2;
		}else{
			// 保護コード
			pDst[0] = L'?';
			nret = 1;
		}
	}else{
		// CESU-8 の処理
		if( nSrcLen < 4 ){
			pDst[0] = static_cast<unsigned short>(DecodeUtf8(pSrc, nSrcLen) & 0x0000ffff);
			nret = 1;
		}else if( nSrcLen == 6 ){
			// CESU-8 サロゲート領域の処理
			pDst[0] = static_cast<unsigned short>( DecodeUtf8(&pSrc[0], 3) & 0x0000ffff );
			pDst[1] = static_cast<unsigned short>( DecodeUtf8(&pSrc[3], 3) & 0x0000ffff );
			nret = 2;
		}else{
			// 保護コード
			pDst[0] = L'?';
			nret = 1;
		}
	}

	return nret;
}



/*!
	Unicode -> UTF-8 の一文字変換

	nSrcLen は 1 または 2

	高速化のため、インライン化
*/
inline int CUtf8::_UniToUtf8_char( const unsigned short* pSrc, const int nSrcLen, unsigned char* pDst, bool bCESU8Mode )
{
	int nret;

	if( nSrcLen < 1 ){
		return 0;
	}

	if( bCESU8Mode != true ){
		// UTF-8 の処理
		wchar32_t wc32;
		if( nSrcLen == 2 ){
			wc32 = DecodeUtf16Surrog( pSrc[0], pSrc[1] );
		}else if( nSrcLen == 1 ){	// nSrcLen == 1
			wc32 = pSrc[0];
		}else{
			wc32 = L'?';
		}
		nret = EncodeUtf8( wc32, &pDst[0] );
	}else{
		// CESU-8 の処理
		int nclen = 0;
		nclen += EncodeUtf8( pSrc[0], &pDst[0] );
		if( nSrcLen == 2 ){
			nclen += EncodeUtf8( pSrc[1], &pDst[nclen] );
		}else{
			;
		}
		nret = nclen;
	}

	return nret;
}



