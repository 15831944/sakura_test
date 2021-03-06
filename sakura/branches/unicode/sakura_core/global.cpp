/*!	@file
	@brief 文字列共通定義

	@author Norio Nakatani
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001, MIK, Stonee, jepro
	Copyright (C) 2002, KK
	Copyright (C) 2003, MIK
	Copyright (C) 2005, Moca

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "stdafx.h"
#include "global.h"

//2007.10.02 kobake CEditWndのインスタンスへのポインタをここに保存しておく
CEditWnd* g_pcEditWnd = NULL;

//2007.10.17 kobake 変数名変更: gm_pszCodeNameArr_1→gm_pszCodeNameArr_Normal
const TCHAR* gm_pszCodeNameArr_Normal[] = {
	_T("SJIS"),			/* SJIS */
	_T("JIS"),			/* JIS */
	_T("EUC"),			/* EUC */
	_T("Unicode"),		/* Unicode */
	_T("UTF-8"),		/* UTF-8 */
	_T("UTF-7"),		/* UTF-7 */
	_T("UniBE")			/* Unicode BigEndian */
};

//2007.10.17 kobake 変数名変更: gm_pszCodeNameArr_2→gm_pszCodeNameArr_Short
const TCHAR* gm_pszCodeNameArr_Short[] = {
	_T("SJIS"),			/* SJIS */
	_T("JIS"),			/* JIS */
	_T("EUC"),			/* EUC */
	_T("Uni"),			/* Unicode */
	_T("UTF-8"),		/* UTF-8 */
	_T("UTF-7"),		/* UTF-7 */
	_T("UniBE")			/* Unicode BigEndian */
};

//2007.10.17 kobake 変数名変更: gm_pszCodeNameArr_3→gm_pszCodeNameArr_Bracket
const TCHAR* gm_pszCodeNameArr_Bracket[] = {
	_T("  [SJIS]"),		/* SJIS */
	_T("  [JIS]"),		/* JIS */
	_T("  [EUC]"),		/* EUC */
	_T("  [Unicode]"),	/* Unicode */
	_T("  [UTF-8]"),	/* UTF-8 */
	_T("  [UTF-7]"),	/* UTF-7 */
	_T("  [UniBE]")		/* Unicode BigEndian */
};

const ECodeType gm_nCodeComboValueArr[] = {
	CODE_AUTODETECT,	/* 文字コード自動判別 */
	CODE_SJIS,
	CODE_JIS,
	CODE_EUC,
	CODE_UNICODE,
	CODE_UNICODEBE,
	CODE_UTF8,
	CODE_UTF7
};
const TCHAR* const	gm_pszCodeComboNameArr[] = {
	_T("自動選択"),
	_T("SJIS"),
	_T("JIS"),
	_T("EUC"),
	_T("Unicode"),
	_T("UnicodeBE"),
	_T("UTF-8"),
	_T("UTF-7")
};

const int gm_nCodeComboNameArrNum = _countof( gm_nCodeComboValueArr );


/*! 選択領域描画用パラメータ */
const COLORREF	SELECTEDAREA_RGB = RGB( 255, 255, 255 );
const int		SELECTEDAREA_ROP2 = R2_XORPEN;

/*! 行終端子の配列 */
const enumEOLType gm_pnEolTypeArr[EOL_TYPE_NUM] = {
	EOL_NONE			,	// == 0
	EOL_CRLF			,	// == 2
	EOL_LFCR			,	// == 2
	EOL_LF				,	// == 1
	EOL_CR					// == 1
};

/*!
  iniの色設定を番号でなく文字列で書き出す。(added by Stonee, 2001/01/12, 2001/01/15)
  配列の順番は共有メモリ中のデータの順番と一致している。

  @note 数値による内部的対応はglobal.hで行っているので参照のこと。(Mar. 7, 2001 jepro)
  CShareDataからglobalに移動
*/
const ColorAttributeData g_ColorAttributeArr[] =
{
	{_T("TXT"), COLOR_ATTRIB_FORCE_DISP | COLOR_ATTRIB_NO_EFFECTS},
	{_T("RUL"), COLOR_ATTRIB_NO_EFFECTS},
	{_T("CAR"), COLOR_ATTRIB_FORCE_DISP | COLOR_ATTRIB_NO_BACK | COLOR_ATTRIB_NO_EFFECTS},	// キャレット		// 2006.12.07 ryoji
	{_T("IME"), COLOR_ATTRIB_NO_BACK | COLOR_ATTRIB_NO_EFFECTS},	// IMEキャレット	// 2006.12.07 ryoji
	{_T("UND"), COLOR_ATTRIB_NO_BACK | COLOR_ATTRIB_NO_EFFECTS},
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
	{_T("KW"), 0},
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


/*[EOF]*/
