//	$Id$
/*!	@file
	@brief BREGEXP Library Handler

	Perl5互換正規表現を扱うDLLであるBREGEXP.DLLを利用するためのインターフェース

	@author genta
	@date Jun. 10, 2001
	@date 2002/2/1 hor		ReleaseCompileBufferを適宜追加
	@date Jul. 25, 2002 genta 行頭条件を考慮した検索を行うように．(置換はまだ)
*/
/*
	Copyright (C) 2001-2002, genta
	Copyright (C) 2002, novice, hor
	Copyright (C) 2003, かろと

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose, 
	including commercial applications, and to alter it and redistribute it 
	freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented;
		   you must not claim that you wrote the original software.
		   If you use this software in a product, an acknowledgment
		   in the product documentation would be appreciated but is
		   not required.

		2. Altered source versions must be plainly marked as such, 
		   and must not be misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		   distribution.
*/

#include <stdio.h>
#include <string.h>
#include "CBregexp.h"
#include "etc_uty.h"

// Compile時、行頭置換(len=0)の時にダミー文字列(１つに統一) by かろと
char CBregexp::tmpBuf[2] = "\0";

// 未使用なので削除
//const char BREGEXP_OPT_KI[]	= "ki";
//const char BREGEXP_OPT_K[]	= "k";

CBregexp::CBregexp() : m_sRep( NULL ),
	m_ePatType( PAT_NORMAL )	//	Jul, 25, 2002 genta
{
}

CBregexp::~CBregexp()
{
	//<< 2002/03/27 Azumaiya
	// 一応、クラスの終了時にコンパイルバッファを解放。
	DeinitDll();
	//>> 2002/03/27 Azumaiya
}

//	Jul. 5, 2001 genta 引数追加。ただし、ここでは使わない。
char *
CBregexp::GetDllName( char* str )
{
	return "BREGEXP.DLL";
}
/*!
	DLLの初期化

	関数のアドレスを取得してメンバに保管する．

	@retval 0 成功
	@retval 1 アドレス取得に失敗
*/
int CBregexp::InitDll()
{
	//	Apr. 15, 2002 genta
	//	CPPA.cpp を参考に設定を配列化した
	
	const ImportTable table[] = {
		{ &BMatch,		"BMatch" },
		{ &BSubst,		"BSubst" },
		{ &BTrans,		"BTrans" },
		{ &BSplit,		"BSplit" },
		{ &BRegfree, 	"BRegfree" },
		{ &BRegexpVersion,	"BRegexpVersion" },
		{ NULL, 0 }
	};
	
	if( ! RegisterEntries( table )){
		return 1;
	}
	
#if 0
	//	アドレス取得
	if( (BMatch = (BREGEXP_BMatch)GetProcAddress( GetInstance(), "BMatch" )) == NULL )
		return 1;
	if( (BSubst = (BREGEXP_BSubst)GetProcAddress( GetInstance(), "BSubst" )) == NULL )
		return 1;
	if( (BTrans = (BREGEXP_BTrans)GetProcAddress( GetInstance(), "BTrans" )) == NULL )
		return 1;
	if( (BSplit = (BREGEXP_BSplit)GetProcAddress( GetInstance(), "BSplit" )) == NULL )
		return 1;
	if( (BRegfree = (BREGEXP_BRegfree)GetProcAddress( GetInstance(), "BRegfree" )) == NULL )
		return 1;
	if( (BRegexpVersion = (BREGEXP_BRegexpVersion)GetProcAddress( GetInstance(), "BRegexpVersion" )) == NULL )
		return 1;
#endif

	return 0;
}

/*!
	BREGEXP構造体の解放
*/
int CBregexp::DeinitDll( void )
{
	ReleaseCompileBuffer();
	return 0;
}

/*! @brief ライブラリに渡すための検索・置換パターンを作成する
**
** @note szPattern2: == NULL:検索 != NULL:置換
** 
** @param szPattern [in] 検索パターン
** @param szPattern2 [in] 置換パターン(NULLなら検索)
** @param bOption [in] 検索オプション
**
** @retval ライブラリに渡す検索パターンへのポインタを返す
** @note 返すポインタは、呼び出し側で delete すること
** 
** @date 2003.05.03 かろと 関数に切り出し
*/
char* CBregexp::MakePattern( const char* szPattern, const char* szPattern2, int bOption ) {
	static const char DELIMITER = '\xFF';		//<! デリミタ
 
	// パターン種別の設定
	if (szPattern[0] == '^') {
		m_ePatType = PAT_TOP;
	} else if (szPattern[strlen(szPattern)-1] == '$') {
		m_ePatType = PAT_BOTTOM;
	} else {
		m_ePatType = PAT_NORMAL;
	} 

	// 検索パターン作成
	char *szNPattern;		//!< ライブラリ渡し用の検索パターン文字列
	char *pPat;				//!< パターン文字列操作用のポインタ
	if (szPattern2 == NULL) {
		// 検索(BMatch)時
		szNPattern = new char[ strlen(szPattern) + 15 ];	//	15：「s///option」が余裕ではいるように。
		pPat = szNPattern;
		*pPat++ = 'm';
	} else {
		// 置換(BSubst)時
		szNPattern = new char[ strlen(szPattern) + strlen(szPattern2) + 15 ];
		pPat = szNPattern;
		*pPat++ = 's';
	}
	*pPat++ = DELIMITER;
	while (*szPattern != '\0') { *pPat++ = *szPattern++; }
	*pPat++ = DELIMITER;
	if (szPattern2 != NULL) {
		while (*szPattern2 != '\0') { *pPat++ = *szPattern2++; }
		*pPat++ = DELIMITER;
	}
	*pPat++ = 'k';			// 漢字対応
	*pPat++ = 'm';			// 複数行対応(但し、呼び出し側が複数行対応でない)
	if( !(bOption & 0x01) ) {		// 2002/2/1 hor IgnoreCase オプション追加 マージ：aroka
		*pPat++ = 'i';		// 同上
	}
	*pPat = '\0';
	return szNPattern;
}


/*!
	JRE32のエミュレーション関数．空の文字列に対して検索を行うことにより
	BREGEXP構造体の生成のみを行う．

	検索文字列はperl形式 i.e. /pattern/option または m/pattern/option

	@param szPattern [in] 検索パターン
	@param bOption [in]
		0x01：大文字小文字の区別をする。

	@retval true 成功
	@retval false 失敗
*/
bool CBregexp::Compile( const char* szPattern, int bOption )
{

	//	DLLが利用可能でないときはエラー終了
	if( !IsAvailable() )
		return false;

	//	BREGEXP構造体の解放
	ReleaseCompileBuffer();

	// ライブラリに渡す検索パターンを作成
	// 別関数で共通処理に変更 2003.05.03 by かろと
	char *szNPattern = MakePattern( szPattern, NULL, bOption );
	BMatch( szNPattern, tmpBuf, tmpBuf+1, &m_sRep, m_szMsg );
	delete [] szNPattern;

	//	メッセージが空文字列でなければ何らかのエラー発生。
	//	サンプルソース参照
	if( m_szMsg[0] ){
		ReleaseCompileBuffer();
		return false;
	}
	
	// 行頭条件チェックは、MakePatternに取り込み 2003.05.03 by かろと

	return true;
}

/*!
	JRE32のエミュレーション関数．既にあるコンパイル構造体を利用して検索（1行）を
	行う．

	@param target [in] 検索対象領域先頭アドレス
	@param len [in] 検索対象領域サイズ
	@param nStart [in] 検索開始位置．(先頭は0)
	@param rep [out] コンパイル構造体へのアドレスを返す。
	検索には内部で保持している構造体が用いられるため、ここに別の構造体を設定しても
	それは検索には用いられない。

	@retval true Match
	@retval false No Match または エラー。エラーはrep == NULLにより判定可能。

	@note 戻り値==-1

	@par repに返される情報


*/
bool CBregexp::GetMatchInfo( const char* target, int len, int nStart, BREGEXP**rep )
{
	//	DLLが利用可能でないとき、または構造体が未設定の時はエラー終了
	if( (!IsAvailable()) || m_sRep == NULL ){
		*rep = NULL;
		return false;
	}

	// 行の先頭("^")の検索時の特別処理 by かろと
	/*
	** 行頭(^)とマッチするのは、nStart=0の時だけなので、それ以外は false
	*/
	if( m_ePatType == PAT_TOP && nStart != 0 ) {
		// nStart!=0でも、BMatch()にとっては行頭になるので、ここでfalseにする必要がある
		return false;
	}
			

	*rep = m_sRep;
	//	検索文字列＝NULLを指定すると前回と同一の文字列と見なされる
	int matched = BMatch( NULL, target + nStart, target + len, &m_sRep, m_szMsg );
	if ( matched < 0 ) {
		// BMatchエラー
		// エラー処理をしていなかったので、nStart>=lenのような場合に、マッチ扱いになり
		// 無限置換等の不具合になっていた 2003.05.03 by かろと
		*rep = NULL;
		return false;
	} else if ( matched == 0 ) {
		return false;
	} else {
		return true;
	}

	return false;
}

// 2002/01/19 novice
/*!
	正規表現による文字列置換

	@param szPattern0 [in] マッチパターン
	@param szPattern1 [in] 置換文字列
	@param target [in] 置換対象データ
	@param len [in] 置換対象データ長
	@param out [out] 置換後文字列		// 2002.01.26 hor
	@param bOption [in]
		0x01：大文字小文字の区別をする。

	@retval true 成功
	@retval false 失敗
*/
bool CBregexp::Replace( const char* szPattern0, const char* szPattern1, const char *target, int len, char **out, int bOption)
{
	int result;

	if( !IsAvailable() ){
		return false;
	}

	ReleaseCompileBuffer();

	// ライブラリに渡す検索パターンを作成
	// 別関数で共通処理に変更 2003.05.03 by かろと
	char *szNPattern = MakePattern( szPattern0, szPattern1, bOption );

	// nLenが０だと、BSubst()が置換に失敗してしまうので、代用データ(tmpBuf)を使う 2003.05.03 かろと
	if( len == 0 ) {
		target = tmpBuf;
		len = 1;
	}
	// 置換実行
	result = BSubst( szNPattern, target, target + len, &m_sRep, m_szMsg );
	delete [] szNPattern;

	//	メッセージが空文字列でなければ何らかのエラー発生。
	//	サンプルソース参照
	if( m_szMsg[0] ){
		ReleaseCompileBuffer();
		return false;
	}

	if( result ){
		if( m_sRep->outp != NULL && m_sRep->outp != '\0' ){
//			strcpy( target, m_sRep->outp );
			int i=lstrlen(m_sRep->outp);
			*out = new char[i+1];
			strcpy( *out, m_sRep->outp );
			(*out)[i] = '\0';
		}else{
//			strcpy( target, "" );
			*out = new char[1];
			(*out)[0] = '\0';
		}

		ReleaseCompileBuffer();
		return true;
	}

	ReleaseCompileBuffer();
	return false;
}

//<< 2002/03/27 Azumaiya
/*!
	JRE32のエミュレーション関数．空の文字列に対して置換を行うことにより
	BREGEXP構造体の生成のみを行う．

	@param szPattern0 [in] 置換パターン
	@param szPattern1 [in] 置換後文字列パターン
	@param bOption [in]
		0x01：大文字小文字の区別をする。

	@retval true 成功
	@retval false 失敗
*/
bool CBregexp::CompileReplace( const char* szPattern0, const char* szPattern1, int bOption )
{

	if( !IsAvailable() ){
		return false;
	}

	ReleaseCompileBuffer();

	// ライブラリに渡す検索パターンを作成
	// 別関数で共通処理に変更 2003.05.03
	char *szNPattern = MakePattern(szPattern0, szPattern1, bOption);
	// 置換実行
	BSubst( szNPattern, tmpBuf, tmpBuf + 1, &m_sRep, m_szMsg );
	delete [] szNPattern;

	//	メッセージが空文字列でなければ何らかのエラー発生。
	//	サンプルソース参照
	if( m_szMsg[0] ){
		ReleaseCompileBuffer();
		return false;
	}

	// 行頭条件チェックは、MakePatternに取り込み 2003.05.03
	return true;
}

/*!
	正規表現による文字列置換
	既にあるコンパイル構造体を利用して置換（1行）を
	行う．

	@param szTarget [in] 置換対象データ
	@param nLen [in] 置換対象データ長
	@param pszOut [out] 置換後文字列
	@param pnOutLen [out] 置換後文字列の長さ

	@retval true 成功
	@retval false 失敗
*/
bool CBregexp::GetReplaceInfo(char *szTarget, int nLen, char **pszOut, int *pnOutLen)
{
	if( !IsAvailable() || m_sRep == NULL )
	{
		return false;
	}

	//	From Here 2003.05.03 かろと
	// nLenが０だと、BSubst()が置換に失敗してしまうので、代用データ(tmpBuf)を使う
	if( nLen == 0 ) {
		szTarget = tmpBuf;
		nLen = 1;
	}
	//	To Here 2003.05.03 かろと

	int result = BSubst( NULL, szTarget, szTarget + nLen, &m_sRep, m_szMsg );

	//	メッセージが空文字列でなければ何らかのエラー発生。
	//	サンプルソース参照
	if( m_szMsg[0] )
	{
		// 逆方向への置換等再呼び出しを考慮して、コンパイルバッファはクリアしない by かろと
//		ReleaseCompileBuffer();
		return false;
	}

	if( !result ) {
		// 置換するものがなかった
		return false;
	}

	if( m_sRep->outp != NULL && m_sRep->outp[0] != '\0' )
	{
		int i = strlen(m_sRep->outp);
		*pszOut = new char[i+1];
		memcpy( *pszOut, m_sRep->outp, i );
		(*pszOut)[i] = '\0';
		*pnOutLen = i;
	}
	else
	{
		*pszOut = new char[1];
		(*pszOut)[0] = '\0';
		*pnOutLen = 0;
	}

	return true;
}
//>> 2002/03/27 Azumaiya

/*[EOF]*/
