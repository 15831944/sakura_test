#pragma once

#include "CCodeBase.h"
#include "CShiftJis.h"

class CUtf7 : public CCodeBase{
public:
	//CCodeBaseインターフェース
	EConvertResult CodeToUnicode(const CMemory& cSrc, CNativeW* pDst){ *pDst->_GetMemory()=cSrc; return UTF7ToUnicode(pDst->_GetMemory()); }	//!< 特定コード → UNICODE    変換
	EConvertResult UnicodeToCode(const CNativeW& cSrc, CMemory* pDst){ *pDst=*cSrc._GetMemory(); return UnicodeToUTF7(pDst); }	//!< UNICODE    → 特定コード 変換
	void GetEol(CMemory* pcmemEol, EEolType eEolType){ CShiftJis::S_GetEol(pcmemEol,eEolType); }	//!< 改行データ取得

public:
	//実装
	static EConvertResult UTF7ToUnicode(CMemory* pMem);		// UTF-7     → Unicodeコード変換 //2007.08.13 kobake 追加
	static EConvertResult UnicodeToUTF7(CMemory* pMem);		// Unicode   → UTF-7コード変換
	static int IsUTF7Direct( wchar_t ); /* Unicode文字がUTF7で直接エンコードできるか */ // 2002.10.25 Moca
	static int MemBASE64_Encode( const char*, int, char**, int, int );/* Base64エンコード */


public:
	//各種判定定数
	static const bool UTF7SetD[];	// UTF7SetD を処理する際に使うブール値
public:
	//各種判定関数
	static bool IsUtf7SetDChar( const uchar_t ); // UTF-7 Set D の文字を判別
};

inline bool CUtf7::IsUtf7SetDChar( const uchar_t c )
{
	return ( !(c & 0x80) && UTF7SetD[c] );
}
