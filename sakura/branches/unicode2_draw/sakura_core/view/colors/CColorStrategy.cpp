#include "stdafx.h"
#include "view/colors/CColorStrategy.h"
#include "CColor_Comment.h"
#include "CColor_Quote.h"
#include "CColor_RegexKeyword.h"
#include "CColor_Space.h"
#include "CColor_Found.h"
#include "CColor_Url.h"
#include "CColor_Numeric.h"
#include "CColor_KeywordSet.h"
#include "CColor_CtrlCode.h"
#include "CColor_Eol.h"
#include "CColor_Found.h"
#include "CColor_Eol.h"
#include "doc/CLayout.h"

CLogicInt SColorStrategyInfo::GetPosInLayout() const
{
	return nPosInLogic - pDispPos->GetLayoutRef()->GetLogicOffset();
}





CColorStrategyPool::CColorStrategyPool()
{
	m_vStrategies.push_back(new CColor_Found);			// マッチ文字列
	m_vStrategies.push_back(new CColor_Eol);			// 行末
	m_vStrategies.push_back(new CColor_RegexKeyword);	// 正規表現キーワード
	m_vStrategies.push_back(new CColor_LineComment);	// 行コメント
	m_vStrategies.push_back(new CColor_BlockComment);	// ブロックコメント
	m_vStrategies.push_back(new CColor_BlockComment2);	// ブロックコメント2
	m_vStrategies.push_back(new CColor_SingleQuote);	// シングルクォーテーション文字列
	m_vStrategies.push_back(new CColor_DoubleQuote);	// ダブルクォーテーション文字列
	m_vStrategies.push_back(new CColor_Url);			// URL
	m_vStrategies.push_back(new CColor_Numeric);		// 半角数字
	m_vStrategies.push_back(new CColor_KeywordSet);		// キーワードセット
	m_vStrategies.push_back(new CColor_Tab);			// タブ
	m_vStrategies.push_back(new CColor_ZenSpace);		// 全角スペース
	m_vStrategies.push_back(new CColor_HanSpace);		// 半角空白（半角スペース）
	m_vStrategies.push_back(new CColor_CtrlCode);		// コントロールコード
}

CColorStrategyPool::~CColorStrategyPool()
{
	for(int i=0;i<(int)m_vStrategies.size();i++){
		delete m_vStrategies[i];
	}
	m_vStrategies.clear();
}




/*!
  iniの色設定を番号でなく文字列で書き出す。(added by Stonee, 2001/01/12, 2001/01/15)
  配列の順番は共有メモリ中のデータの順番と一致している。

  @note 数値による内部的対応はglobal.hで行っているので参照のこと。(Mar. 7, 2001 jepro)
  CShareDataからglobalに移動
*/
const SColorAttributeData g_ColorAttributeArr[] =
{
	{_T("TXT"), COLOR_ATTRIB_FORCE_DISP | COLOR_ATTRIB_NO_EFFECTS},
	{_T("RUL"), COLOR_ATTRIB_NO_EFFECTS},
	{_T("CAR"), COLOR_ATTRIB_FORCE_DISP | COLOR_ATTRIB_NO_BACK | COLOR_ATTRIB_NO_EFFECTS},	// キャレット		// 2006.12.07 ryoji
	{_T("IME"), COLOR_ATTRIB_NO_BACK | COLOR_ATTRIB_NO_EFFECTS},	// IMEキャレット	// 2006.12.07 ryoji
	{_T("UND"), COLOR_ATTRIB_NO_BACK | COLOR_ATTRIB_NO_EFFECTS},
	{_T("CVL"), COLOR_ATTRIB_NO_BACK | ( COLOR_ATTRIB_NO_EFFECTS & ~COLOR_ATTRIB_NO_BOLD )}, // 2007.09.09 Moca カーソル位置縦線
	{_T("LNO"), 0},
	{_T("MOD"), 0},
	{_T("TAB"), 0},
	{_T("SPC"), 0},	//2002.04.28 Add By KK
	{_T("ZEN"), 0},
	{_T("CTL"), 0},
	{_T("EOL"), 0},
	{_T("RAP"), 0},
	{_T("VER"), 0},  // 2005.11.08 Moca 指定桁縦線
	{_T("EOF"), 0},
	{_T("NUM"), 0},	//@@@ 2001.02.17 by MIK 半角数値の強調
	{_T("FND"), 0},
	{_T("KW1"), 0},
	{_T("KW2"), 0},
	{_T("KW3"), 0},	//@@@ 2003.01.13 by MIK 強調キーワード3-10
	{_T("KW4"), 0},
	{_T("KW5"), 0},
	{_T("KW6"), 0},
	{_T("KW7"), 0},
	{_T("KW8"), 0},
	{_T("KW9"), 0},
	{_T("KWA"), 0},
	{_T("CMT"), 0},
	{_T("SQT"), 0},
	{_T("WQT"), 0},
	{_T("URL"), 0},
	{_T("RK1"), 0},	//@@@ 2001.11.17 add MIK
	{_T("RK2"), 0},	//@@@ 2001.11.17 add MIK
	{_T("RK3"), 0},	//@@@ 2001.11.17 add MIK
	{_T("RK4"), 0},	//@@@ 2001.11.17 add MIK
	{_T("RK5"), 0},	//@@@ 2001.11.17 add MIK
	{_T("RK6"), 0},	//@@@ 2001.11.17 add MIK
	{_T("RK7"), 0},	//@@@ 2001.11.17 add MIK
	{_T("RK8"), 0},	//@@@ 2001.11.17 add MIK
	{_T("RK9"), 0},	//@@@ 2001.11.17 add MIK
	{_T("RKA"), 0},	//@@@ 2001.11.17 add MIK
	{_T("DFA"), 0},	//DIFF追加	//@@@ 2002.06.01 MIK
	{_T("DFC"), 0},	//DIFF変更	//@@@ 2002.06.01 MIK
	{_T("DFD"), 0},	//DIFF削除	//@@@ 2002.06.01 MIK
	{_T("BRC"), 0},	//対括弧	// 02/09/18 ai Add
	{_T("MRK"), 0},	//ブックマーク	// 02/10/16 ai Add
	{_T("LAST"), 0}	// Not Used
};



/*
 * カラー名からインデックス番号に変換する
 */
SAKURA_CORE_API int GetColorIndexByName( const TCHAR *name )
{
	int	i;
	for( i = 0; i < COLORIDX_LAST; i++ )
	{
		if( _tcscmp( name, g_ColorAttributeArr[i].szName ) == 0 ) return i;
	}
	return -1;
}

/*
 * インデックス番号からカラー名に変換する
 */
SAKURA_CORE_API const TCHAR* GetColorNameByIndex( int index )
{
	return g_ColorAttributeArr[index].szName;
}
