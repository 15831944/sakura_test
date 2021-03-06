//	$Id$
/*!	@file
	CRegexKeyword Library

	正規表現キーワードを扱う。
	BREGEXP.DLLを利用する。

	@author MIK
	@date Nov. 17, 2001
	$Revision$
*/
/*
	Copyright (C) 2001, MIK

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//@@@ 2001.11.17 add start MIK

//class CRegexKeyword;

#ifndef	_REGEX_KEYWORD_H_
#define	_REGEX_KEYWORD_H_

#include <windows.h>
#include "CShareData.h"
//#include "CMemory.h"
#include "global.h"
#include "CBregexp.h"


#define USE_PARENT	//親を使ってキーワード格納領域を削減する。
//#undef USE_PARENT

/*
 * パラメータ宣言
 */
#define RK_EMPTY          0      //初期状態
#define RK_CLOSE          1      //BREGEXPクローズ
#define RK_OPEN           2      //BREGEXPオープン
#define RK_ACTIVE         3      //コンパイル済み
#define RK_ERROR          9      //コンパイルエラー

#define RK_MATCH          4      //マッチする
#define RK_NOMATCH        5      //この行ではマッチしない

#define RK_SIZE           100    //最大登録可能数

//#define RK_HEAD_CHAR      '^'    //行先頭の正規表現
#define RK_HEAD_STR1      "/^"   //BREGEXP
#define RK_HEAD_STR1_LEN  2
#define RK_HEAD_STR2      "m#^"  //BREGEXP
#define RK_HEAD_STR2_LEN  3
#define RK_HEAD_STR3      "m/^"  //BREGEXP
#define RK_HEAD_STR3_LEN  3
//#define RK_HEAD_STR4      "#^"   //BREGEXP
//#define RK_HEAD_STR4_LEN  2

#define RK_KAKOMI_1_START "/"
#define RK_KAKOMI_1_END   "/k"
#define RK_KAKOMI_2_START "m#"
#define RK_KAKOMI_2_END   "#k"
#define RK_KAKOMI_3_START "m/"
#define RK_KAKOMI_3_END   "/k"
//#define RK_KAKOMI_4_START "#"
//#define RK_KAKOMI_4_END   "#k"



//!	正規表現キーワード検索情報構造体
typedef struct RegexInfo_t {
	BREGEXP	*pBregexp;	//BREGEXP構造体
#ifdef USE_PARENT
#else
	struct RegexKeywordInfo	sRegexKey;	//コンパイルパターンを保持
#endif
	char   nStatus;		//状態(EMPTY,CLOSE,OPEN,ACTIVE,ERROR)
	char   nMatch;		//このキーワードのマッチ状態(EMPTY,MATCH,NOMATCH)
	int    nOffset;		//マッチした位置
	int    nLength;		//マッチした長さ
	int    nHead;		//先頭のみチェックするか？
	int    nFlag;           //色指定のチェックが入っているか？ YES=RK_EMPTY, NO=RK_NOMATCH
} REGEX_INFO;



//!	正規表現キーワードクラス
/*!
	正規表現キーワードを扱う。
*/
class SAKURA_CORE_API CRegexKeyword : public CBregexp {
public:
	CRegexKeyword();
	~CRegexKeyword();

	//! 行検索開始
	BOOL RegexKeyLineStart( void );
	//! 行検索
	BOOL RegexIsKeyword( const char *pLine, int nPos, int nLineLen, int *nMatchLen, int *nMatchColor );
	//! タイプ設定
	BOOL RegexKeySetTypes( Types *pTypesPtr );
	//! 書式(囲み)チェック
	BOOL RegexKeyCheckSyntax( const char *s );

//	CShareData	m_cShareData;
//	DLLSHAREDATA*	m_pShareData;
	int		m_nTypeIndex;		//現在のタイプ設定番号
	BOOL		m_bUseRegexKeyword;	//正規表現キーワードを使用する・しない


protected:
	//! コンパイル
	BOOL RegexKeyCompile(void);
	//! 変数初期化
	BOOL RegexKeyInit( void );


private:
	Types		*m_pTypes;		//タイプ設定へのポインタ(呼び出し側が持っているもの)
	int		m_nCompiledMagicNumber;	//コンパイル済みか？
	int		m_nRegexKeyCount;	//現在のキーワード数
	REGEX_INFO	m_sInfo[MAX_REGEX_KEYWORD];	//キーワード一覧(BREGEXPコンパイル対象)
	char		m_szMsg[256];		//!< BREGEXPからのメッセージを保持する
};

#endif	//_REGEX_KEYWORD_H_

//@@@ 2001.11.17 add end MIK

/*[EOF]*/
