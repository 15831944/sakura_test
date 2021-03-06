/*!	@file
	@brief タイプ別設定ダイアログボックス

	@author Norio Nakatani
	@date 1998/12/24  新規作成
*/
/*
	Copyright (C) 1998-2002, Norio Nakatani
	Copyright (C) 2000, jepro, genta
	Copyright (C) 2001, jepro, genta, MIK, hor, Stonee, asa-o
	Copyright (C) 2002, YAZAKI, aroka, MIK, genta, こおり, Moca
	Copyright (C) 2003, MIK, zenryaku, Moca, naoh, KEITA, genta
	Copyright (C) 2005, MIK, genta, Moca, ryoji
	Copyright (C) 2006, ryoji, fon
	Copyright (C) 2007, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "stdafx.h"
#include "sakura_rc.h"
#include "CPropTypes.h"
#include "debug.h"
#include <windows.h>
#include <commctrl.h>
#include "CDlgOpenFile.h"
#include "CDlgKeywordSelect.h"
#include "global.h"
#include "CDataProfile.h"
#include "CShareData.h"
#include "funccode.h"	//Stonee, 2001/05/18
#include "CDlgSameColor.h"	// 2006.04.26 ryoji
#include "charcode.h"
#include "CEditApp.h"
#include "io/CTextStream.h"
#include "util/shell.h"
#include "util/file.h"
#include "util/module.h"
using namespace std;

//2007.11.29 kobake 変数の意味を明確にするため、nMethos を テンプレート化。
template <class TYPE>
struct TYPE_NAME {
	TYPE			nMethod;
	wchar_t*		pszName;
};

TYPE_NAME<EOutlineType> OlmArr[] = {
//	{ OUTLINE_C,		L"C" },
	{ OUTLINE_CPP,		L"C/C++" },
	{ OUTLINE_PLSQL,	L"PL/SQL" },
	{ OUTLINE_JAVA,		L"Java" },
	{ OUTLINE_COBOL,	L"COBOL" },
	{ OUTLINE_PERL,		L"Perl" },			//Sep. 8, 2000 genta
	{ OUTLINE_ASM,		L"アセンブラ" },
	{ OUTLINE_VB,		L"Visual Basic" },	// 2001/06/23 N.Nakatani
	{ OUTLINE_PYTHON,	L"Python" },		//	2007.02.08 genta
	{ OUTLINE_WZTXT,	L"WZ階層付テキスト" },	// 2003.05.20 zenryaku, 2003.06.23 Moca 名称変更
	{ OUTLINE_HTML,		L"HTML" },			// 2003.05.20 zenryaku
	{ OUTLINE_TEX,		L"TeX" },			// 2003.07.20 naoh
	{ OUTLINE_TEXT,		L"テキスト" }		//Jul. 08, 2001 JEPRO 常に最後尾におく
};


TYPE_NAME<ESmartIndentType> SmartIndentArr[] = {
	{ SMARTINDENT_NONE,	L"なし" },
	{ SMARTINDENT_CPP,	L"C/C++" }
};

//	Nov. 20, 2000 genta
TYPE_NAME<int> ImeStateArr[] = {
	{ 0, L"標準設定" },
	{ 1, L"全角" },
	{ 2, L"全角ひらがな" },
	{ 3, L"全角カタカナ" },
	{ 4, L"無変換" }
};

TYPE_NAME<int> ImeSwitchArr[] = {
	{ 0, L"そのまま" },
	{ 1, L"常にON" },
	{ 2, L"常にOFF" },
};

/*!	2行目以降のインデント方法

	@sa CLayoutMgr::SetLayoutInfo()
	@date Oct. 1, 2002 genta 
*/
TYPE_NAME<int> IndentTypeArr[] = {
	{ 0, L"なし" },
	{ 1, L"tx2x" },
	{ 2, L"論理行先頭" },
};



//	行コメントに関する情報
struct {
	int nEditID;
	int nCheckBoxID;
	int nTextID;
} const cLineComment[COMMENT_DELIMITER_NUM] = {
	{ IDC_EDIT_LINECOMMENT	, IDC_CHECK_LCPOS , IDC_EDIT_LINECOMMENTPOS },
	{ IDC_EDIT_LINECOMMENT2	, IDC_CHECK_LCPOS2, IDC_EDIT_LINECOMMENTPOS2},
	{ IDC_EDIT_LINECOMMENT3	, IDC_CHECK_LCPOS3, IDC_EDIT_LINECOMMENTPOS3}
};

WNDPROC	m_wpColorListProc;


wchar_t* MakeRGBStr( DWORD dwRGB, wchar_t* pszText )
{
	auto_sprintf( pszText, L"RGB( %d, %d, %d )",
		GetRValue( dwRGB ),
		GetGValue( dwRGB ),
		GetBValue( dwRGB )
	);
	return pszText;
}


//@@@ 2001.02.04 Start by MIK: Popup Help
#include "sakura.hh"
static const DWORD p_helpids1[] = {	//11300
	IDC_CHECK_WORDWRAP,				HIDC_CHECK_WORDWRAP,		//英文ワードラップ
	IDC_EDIT_TABSPACE,				HIDC_EDIT_TABSPACE,			//TAB幅 // Sep. 19, 2002 genta
	IDC_COMBO_IMESWITCH,			HIDC_COMBO_IMESWITCH,		//IMEのON/OFF状態
	IDC_COMBO_IMESTATE,				HIDC_COMBO_IMESTATE,		//IMEの入力モード
	IDC_COMBO_SMARTINDENT,			HIDC_COMBO_SMARTINDENT,		//スマートインデント
	IDC_COMBO_OUTLINES,				HIDC_COMBO_OUTLINES,		//アウトライン解析方法
	IDC_EDIT_TYPENAME,				HIDC_EDIT_TYPENAME,			//設定の名前
	IDC_EDIT_TYPEEXTS,				HIDC_EDIT_TYPEEXTS,			//ファイル拡張子
	IDC_EDIT_MAXLINELEN,			HIDC_EDIT_MAXLINELEN,		//折り返し桁数
	IDC_EDIT_CHARSPACE,				HIDC_EDIT_CHARSPACE,		//文字の間隔
	IDC_EDIT_LINESPACE,				HIDC_EDIT_LINESPACE,		//行の間隔
	IDC_EDIT_INDENTCHARS,			HIDC_EDIT_INDENTCHARS,		//その他のインデント対象文字
	IDC_EDIT_TABVIEWSTRING,			HIDC_EDIT_TABVIEWSTRING,	//TAB表示文字列
	IDC_CHECK_INS_SPACE,			HIDC_CHECK_INS_SPACE,		//スペースの挿入
	IDC_SPIN_MAXLINELEN,			HIDC_EDIT_MAXLINELEN,
	IDC_SPIN_CHARSPACE,				HIDC_EDIT_CHARSPACE,
	IDC_SPIN_LINESPACE,				HIDC_EDIT_LINESPACE,
	IDC_CHECK_KINSOKUHEAD,			HIDC_CHECK_KINSOKUHEAD,		//行頭禁則	//@@@ 2002.04.08 MIK
	IDC_CHECK_KINSOKUTAIL,			HIDC_CHECK_KINSOKUTAIL,		//行末禁則	//@@@ 2002.04.08 MIK
	IDC_CHECK_KINSOKURET,			HIDC_CHECK_KINSOKURET,		//改行文字をぶら下げる	//@@@ 2002.04.14 MIK
	IDC_CHECK_KINSOKUKUTO,			HIDC_CHECK_KINSOKUKUTO,		//句読点をぶら下げる	//@@@ 2002.04.17 MIK
	IDC_EDIT_KINSOKUHEAD,			HIDC_EDIT_KINSOKUHEAD,		//行頭禁則	//@@@ 2002.04.08 MIK
	IDC_EDIT_KINSOKUTAIL,			HIDC_EDIT_KINSOKUTAIL,		//行末禁則	//@@@ 2002.04.08 MIK
	IDC_CHECK_TAB_ARROW,			HIDC_CHECK_TAB_ARROW,		//矢印表示	// 2006.08.06 ryoji
	IDC_CHECK_INDENT,				HIDC_CHECK_INDENT,			//自動インデント	// 2006.08.19 ryoji
	IDC_CHECK_INDENT_WSPACE,		HIDC_CHECK_INDENT_WSPACE,	//全角空白もインデント	// 2006.08.19 ryoji
	IDC_COMBO_INDENTLAYOUT,			HIDC_COMBO_INDENTLAYOUT,	//折り返し行インデント	// 2006.08.06 ryoji
	IDC_CHECK_RTRIM_PREVLINE,		HIDC_CHECK_RTRIM_PREVLINE,	//改行時に末尾の空白を削除	// 2006.08.06 ryoji
	IDC_RADIO_OUTLINEDEFAULT,		HIDC_RADIO_OUTLINEDEFAULT,	//標準ルール	// 2006.08.06 ryoji
	IDC_RADIO_OUTLINERULEFILE,		HIDC_RADIO_OUTLINERULEFILE,	//ルールファイル	// 2006.08.06 ryoji
	IDC_EDIT_OUTLINERULEFILE,		HIDC_EDIT_OUTLINERULEFILE,	//ルールファイル名	// 2006.08.06 ryoji
	IDC_BUTTON_RULEFILE_REF,		HIDC_BUTTON_RULEFILE_REF,	//ルールファイル参照	// 2006/09/09 novice
	IDC_CHECK_DOCICON,				HIDC_CHECK_DOCICON,			//文書アイコンを使う	// 2006.08.06 ryoji
//	IDC_STATIC,						-1,
	0, 0
};
static const DWORD p_helpids2[] = {	//11400
	IDC_BUTTON_TEXTCOLOR,			HIDC_BUTTON_TEXTCOLOR,			//文字色
	IDC_BUTTON_BACKCOLOR,			HIDC_BUTTON_BACKCOLOR,			//背景色
	IDC_BUTTON_SAMETEXTCOLOR,		HIDC_BUTTON_SAMETEXTCOLOR,		//文字色統一
	IDC_BUTTON_SAMEBKCOLOR,			HIDC_BUTTON_SAMEBKCOLOR,		//背景色統一
	IDC_BUTTON_IMPORT,				HIDC_BUTTON_IMPORT_COLOR,		//インポート
	IDC_BUTTON_EXPORT,				HIDC_BUTTON_EXPORT_COLOR,		//エクスポート
	IDC_CHECK_DISP,					HIDC_CHECK_DISP,				//色分け表示
	IDC_CHECK_FAT,					HIDC_CHECK_FAT,					//太字
	IDC_CHECK_UNDERLINE,			HIDC_CHECK_UNDERLINE,			//下線
	IDC_CHECK_LCPOS,				HIDC_CHECK_LCPOS,				//桁指定１
	IDC_CHECK_LCPOS2,				HIDC_CHECK_LCPOS2,				//桁指定２
	IDC_COMBO_SET,					HIDC_COMBO_SET_COLOR,			//強調キーワード１セット名
	IDC_COMBO_SET2,					HIDC_COMBO_SET2_COLOR,			//強調キーワード２セット名
	IDC_EDIT_BLOCKCOMMENT_FROM,		HIDC_EDIT_BLOCKCOMMENT_FROM,	//ブロックコメント１開始
	IDC_EDIT_BLOCKCOMMENT_TO,		HIDC_EDIT_BLOCKCOMMENT_TO,		//ブロックコメント１終了
	IDC_EDIT_LINECOMMENT,			HIDC_EDIT_LINECOMMENT,			//行コメント１
	IDC_EDIT_LINECOMMENT2,			HIDC_EDIT_LINECOMMENT2,			//行コメント２
	IDC_EDIT_LINECOMMENTPOS,		HIDC_EDIT_LINECOMMENTPOS,		//桁数１
	IDC_EDIT_LINECOMMENTPOS2,		HIDC_EDIT_LINECOMMENTPOS2,		//桁数２
	IDC_EDIT_LINETERMCHAR,			HIDC_EDIT_LINETERMCHAR,			//行番号区切り
	IDC_EDIT_BLOCKCOMMENT_FROM2,	HIDC_EDIT_BLOCKCOMMENT_FROM2,	//ブロックコメント２開始
	IDC_EDIT_BLOCKCOMMENT_TO2,		HIDC_EDIT_BLOCKCOMMENT_TO2,		//ブロックコメント２終了
	IDC_EDIT_LINECOMMENT3,			HIDC_EDIT_LINECOMMENT3,			//行コメント３
	IDC_LIST_COLORS,				HIDC_LIST_COLORS,				//色指定
	IDC_CHECK_LCPOS3,				HIDC_CHECK_LCPOS3,				//桁指定３
	IDC_EDIT_LINECOMMENTPOS3,		HIDC_EDIT_LINECOMMENTPOS3,		//桁数３
	IDC_RADIO_ESCAPETYPE_1,			HIDC_RADIO_ESCAPETYPE_1,		//文字列エスケープ（C言語風）
	IDC_RADIO_ESCAPETYPE_2,			HIDC_RADIO_ESCAPETYPE_2,		//文字列エスケープ（PL/SQL風）
	IDC_RADIO_LINENUM_LAYOUT,		HIDC_RADIO_LINENUM_LAYOUT,		//行番号の表示（折り返し単位）
	IDC_RADIO_LINENUM_CRLF,			HIDC_RADIO_LINENUM_CRLF,		//行番号の表示（改行単位）
	IDC_RADIO_LINETERMTYPE0,		HIDC_RADIO_LINETERMTYPE0,		//行番号区切り（なし）
	IDC_RADIO_LINETERMTYPE1,		HIDC_RADIO_LINETERMTYPE1,		//行番号区切り（縦線）
	IDC_RADIO_LINETERMTYPE2,		HIDC_RADIO_LINETERMTYPE2,		//行番号区切り（任意）
	IDC_BUTTON_KEYWORD_SELECT,		HIDC_BUTTON_KEYWORD_SELECT,		//強調キーワード2〜10	// 2006.08.06 ryoji
	IDC_EDIT_VERTLINE,				HIDC_EDIT_VERTLINE,				//縦線の桁指定	// 2006.08.06 ryoji
//	IDC_STATIC,						-1,
	0, 0
};
static const DWORD p_helpids3[] = {	//11500
	IDC_BUTTON_HOKANFILE_REF,		HIDC_BUTTON_HOKANFILE_REF,			//入力補完 単語ファイル参照
	IDC_CHECK_HOKANLOHICASE,		HIDC_CHECK_HOKANLOHICASE,			//入力補完の英大文字小文字
	IDC_CHECK_HOKANBYFILE,			HIDC_CHECK_HOKANBYFILE,				//現在のファイルから入力補完
	IDC_EDIT_HOKANFILE,				HIDC_EDIT_HOKANFILE,				//単語ファイル名
	IDC_EDIT_TYPEEXTHELP,			HIDC_EDIT_TYPEEXTHELP,				//外部ヘルプファイル名	// 2006.08.06 ryoji
	IDC_BUTTON_TYPEOPENHELP,		HIDC_BUTTON_TYPEOPENHELP,			//外部ヘルプファイル参照	// 2006.08.06 ryoji
	IDC_EDIT_TYPEEXTHTMLHELP,		HIDC_EDIT_TYPEEXTHTMLHELP,			//外部HTMLヘルプファイル名	// 2006.08.06 ryoji
	IDC_BUTTON_TYPEOPENEXTHTMLHELP,	HIDC_BUTTON_TYPEOPENEXTHTMLHELP,	//外部HTMLヘルプファイル参照	// 2006.08.06 ryoji
	IDC_CHECK_TYPEHTMLHELPISSINGLE,	HIDC_CHECK_TYPEHTMLHELPISSINGLE,	//ビューアを複数起動しない	// 2006.08.06 ryoji
//	IDC_STATIC,						-1,
	0, 0
};
//To Here Jul. 05, 2001



/* p1 ダイアログプロシージャ */
INT_PTR CALLBACK PropTypesP1Proc(
	HWND	hwndDlg,	// handle to dialog box
	UINT	uMsg,		// message
	WPARAM	wParam,		// first message parameter
	LPARAM	lParam 		// second message parameter
)
{
	PROPSHEETPAGE*	pPsp;
	CPropTypes* pCPropTypes;
	switch( uMsg ){
	case WM_INITDIALOG:
		pPsp = (PROPSHEETPAGE*)lParam;
		pCPropTypes = ( CPropTypes* )(pPsp->lParam);
		if( NULL != pCPropTypes ){
			return pCPropTypes->DispatchEvent_p1( hwndDlg, uMsg, wParam, pPsp->lParam );
		}else{
			return FALSE;
		}
	default:
		// Modified by KEITA for WIN64 2003.9.6
		pCPropTypes = ( CPropTypes* )::GetWindowLongPtr( hwndDlg, DWLP_USER );
		if( NULL != pCPropTypes ){
			return pCPropTypes->DispatchEvent_p1( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	}
}

/* p2 ダイアログプロシージャ */
INT_PTR CALLBACK PropTypesP2Proc(
	HWND	hwndDlg,	// handle to dialog box
	UINT	uMsg,		// message
	WPARAM	wParam,		// first message parameter
	LPARAM	lParam 		// second message parameter
)
{
	PROPSHEETPAGE*	pPsp;
	CPropTypes* pCPropTypes;
	switch( uMsg ){
	case WM_INITDIALOG:
		pPsp = (PROPSHEETPAGE*)lParam;
		pCPropTypes = ( CPropTypes* )(pPsp->lParam);
		if( NULL != pCPropTypes ){
			return pCPropTypes->DispatchEvent_p2( hwndDlg, uMsg, wParam, pPsp->lParam );
		}else{
			return FALSE;
		}
	default:
		// Modified by KEITA for WIN64 2003.9.6
		pCPropTypes = ( CPropTypes* )::GetWindowLongPtr( hwndDlg, DWLP_USER );
		if( NULL != pCPropTypes ){
			return pCPropTypes->DispatchEvent_p2( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	}
}







/* p3 ダイアログプロシージャ */
INT_PTR CALLBACK PropTypesP3_newProc(
	HWND	hwndDlg,	// handle to dialog box
	UINT	uMsg,		// message
	WPARAM	wParam,		// first message parameter
	LPARAM	lParam 		// second message parameter
)
{
	PROPSHEETPAGE*	pPsp;
	CPropTypes*		pCPropTypes;
	switch( uMsg ){
	case WM_INITDIALOG:
		pPsp = (PROPSHEETPAGE*)lParam;
		pCPropTypes = ( CPropTypes* )(pPsp->lParam);
		if( NULL != pCPropTypes ){
			return pCPropTypes->DispatchEvent_p3_new( hwndDlg, uMsg, wParam, pPsp->lParam );
		}else{
			return FALSE;
		}
	default:
		// Modified by KEITA for WIN64 2003.9.6
		pCPropTypes = ( CPropTypes* )::GetWindowLongPtr( hwndDlg, DWLP_USER );
		if( NULL != pCPropTypes ){
			return pCPropTypes->DispatchEvent_p3_new( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	}
}




CPropTypes::CPropTypes()
{
	/* 共有データ構造体のアドレスを返す */
	m_pShareData = CShareData::getInstance()->GetShareData();
	// Mar. 31, 2003 genta メモリ削減のためポインタに変更
	m_pCKeyWordSetMgr = &(m_pShareData->m_CKeyWordSetMgr);

	m_hInstance = NULL;		/* アプリケーションインスタンスのハンドル */
	m_hwndParent = NULL;	/* オーナーウィンドウのハンドル */
	m_hwndThis  = NULL;		/* このダイアログのハンドル */
	m_nPageNum = 0;

	// 2005.11.30 Moca カスタム色を設定・保持
	int i;
	for( i = 0; i < _countof(m_dwCustColors); i++ ){
		m_dwCustColors[i] = RGB( 255, 255, 255 );
	}
	
	/* ヘルプファイルのフルパスを返す */
	m_szHelpFile = CEditApp::Instance()->GetHelpFilePath();

	return;
}





CPropTypes::~CPropTypes()
{
//	if( NULL != m_hbmpToolButtons ){
//		/* ビットマップ破棄 */
//		::DeleteObject( m_hbmpToolButtons );
//	}
	return;
}





/* 初期化 */
void CPropTypes::Create( HINSTANCE hInstApp, HWND hwndParent )
{
	m_hInstance = hInstApp;		/* アプリケーションインスタンスのハンドル */
	m_hwndParent = hwndParent;	/* オーナーウィンドウのハンドル */
	return;
}





/* 色選択ダイアログ */
BOOL CPropTypes::SelectColor( HWND hwndParent, COLORREF* pColor, DWORD* pCustColors )
{
	CHOOSECOLOR		cc;
	cc.lStructSize = sizeof_raw( cc );
	cc.hwndOwner = hwndParent;
	cc.hInstance = NULL;
	cc.rgbResult = *pColor;
	cc.lpCustColors = pCustColors;
	cc.Flags = /*CC_PREVENTFULLOPEN |*/ CC_RGBINIT;
	cc.lCustData = NULL;
	cc.lpfnHook = NULL;
	cc.lpTemplateName = NULL;
	if( FALSE == ::ChooseColor( &cc ) ){
		return FALSE;
	}
	*pColor = cc.rgbResult;
	return TRUE;
}





/* 色ボタンの描画 */
void CPropTypes::DrawColorButton( DRAWITEMSTRUCT* pDis, COLORREF cColor )
{
//	MYTRACE_A( "pDis->itemAction = " );

	COLORREF	cBtnHiLight		= (COLORREF)::GetSysColor(COLOR_3DHILIGHT);
	COLORREF	cBtnShadow		= (COLORREF)::GetSysColor(COLOR_3DSHADOW);
	COLORREF	cBtnDkShadow	= (COLORREF)::GetSysColor(COLOR_3DDKSHADOW);
	COLORREF	cBtnFace		= (COLORREF)::GetSysColor(COLOR_3DFACE);
	COLORREF	cRim;
	HBRUSH		hBrush;
	HBRUSH		hBrushOld;
	HPEN		hPen;
	HPEN		hPenOld;
	RECT		rc;
	RECT		rcFocus;

	/* ボタンの表面の色で塗りつぶす */
	hBrush = ::CreateSolidBrush( cBtnFace );
	::FillRect( pDis->hDC, &(pDis->rcItem), hBrush );
	::DeleteObject( hBrush );

	/* 枠の描画 */
	rcFocus = rc = pDis->rcItem;
	rc.top += 4;
	rc.left += 4;
	rc.right -= 4;
	rc.bottom -= 4;
	rcFocus = rc;
//	rc.right -= 11;

	if( pDis->itemState & ODS_SELECTED ){
		hPen = ::CreatePen( PS_SOLID, 0, cBtnDkShadow );
		hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
		::MoveToEx( pDis->hDC, 0, pDis->rcItem.bottom - 2, NULL );
		::LineTo( pDis->hDC, 0, 0 );
		::LineTo( pDis->hDC, pDis->rcItem.right - 1, 0 );
		::SelectObject( pDis->hDC, hPenOld );
		::DeleteObject( hPen );

		hPen = ::CreatePen( PS_SOLID, 0, cBtnShadow );
		hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
		::MoveToEx( pDis->hDC, 1, pDis->rcItem.bottom - 3, NULL );
		::LineTo( pDis->hDC, 1, 1 );
		::LineTo( pDis->hDC, pDis->rcItem.right - 2, 1 );
		::SelectObject( pDis->hDC, hPenOld );
		::DeleteObject( hPen );

		hPen = ::CreatePen( PS_SOLID, 0, cBtnHiLight );
		hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
		::MoveToEx( pDis->hDC, 0, pDis->rcItem.bottom - 1, NULL );
		::LineTo( pDis->hDC, pDis->rcItem.right - 1, pDis->rcItem.bottom - 1 );
		::LineTo( pDis->hDC, pDis->rcItem.right - 1, -1 );
		::SelectObject( pDis->hDC, hPenOld );
		::DeleteObject( hPen );

		rc.top += 1;
		rc.left += 1;
		rc.right += 1;
		rc.bottom += 1;

		rcFocus.top += 1;
		rcFocus.left += 1;
		rcFocus.right += 1;
		rcFocus.bottom += 1;

	}else{
		hPen = ::CreatePen( PS_SOLID, 0, cBtnHiLight );
		hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
		::MoveToEx( pDis->hDC, 0, pDis->rcItem.bottom - 2, NULL );
		::LineTo( pDis->hDC, 0, 0 );
		::LineTo( pDis->hDC, pDis->rcItem.right - 1, 0 );
		::SelectObject( pDis->hDC, hPenOld );
		::DeleteObject( hPen );

		hPen = ::CreatePen( PS_SOLID, 0, cBtnShadow );
		hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
		::MoveToEx( pDis->hDC, 1, pDis->rcItem.bottom - 2, NULL );
		::LineTo( pDis->hDC, pDis->rcItem.right - 2, pDis->rcItem.bottom - 2 );
		::LineTo( pDis->hDC, pDis->rcItem.right - 2, 0 );
		::SelectObject( pDis->hDC, hPenOld );
		::DeleteObject( hPen );

		hPen = ::CreatePen( PS_SOLID, 0, cBtnDkShadow );
		hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
		::MoveToEx( pDis->hDC, 0, pDis->rcItem.bottom - 1, NULL );
		::LineTo( pDis->hDC, pDis->rcItem.right - 1, pDis->rcItem.bottom - 1 );
		::LineTo( pDis->hDC, pDis->rcItem.right - 1, -1 );
		::SelectObject( pDis->hDC, hPenOld );
		::DeleteObject( hPen );
	}
	if( pDis->itemState & ODS_DISABLED ){
	}else{
		/* 指定色で塗りつぶす */
		hBrush = ::CreateSolidBrush( cColor );
		hBrushOld = (HBRUSH)::SelectObject( pDis->hDC, hBrush );
		cRim = cBtnShadow;
		hPen = ::CreatePen( PS_SOLID, 0, cRim );
		hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
		::RoundRect( pDis->hDC, rc.left, rc.top, rc.right, rc.bottom , 5, 5 );
		::SelectObject( pDis->hDC, hPenOld );
		::SelectObject( pDis->hDC, hBrushOld );
		::DeleteObject( hPen );
		::DeleteObject( hBrush );
	}


//	/* 区切りたて棒 */
//	hPen = ::CreatePen( PS_SOLID, 0, cBtnShadow );
//	hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
//	::MoveToEx( pDis->hDC, rc.right + 3, rc.top, NULL );
//	::LineTo( pDis->hDC, rc.right + 3, rc.bottom );
//	::SelectObject( pDis->hDC, hPenOld );
//	::DeleteObject( hPen );
//
//	hPen = ::CreatePen( PS_SOLID, 0, cBtnHiLight );
//	hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
//	::MoveToEx( pDis->hDC, rc.right + 4, rc.top, NULL );
//	::LineTo( pDis->hDC, rc.right + 4, rc.bottom );
//	::SelectObject( pDis->hDC, hPenOld );
//	::DeleteObject( hPen );
//
//	/* ▼記号 */
//	hPen = ::CreatePen( PS_SOLID, 0, cBtnDkShadow );
//	hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
//	::MoveToEx( pDis->hDC, rc.right + 6		, rc.top + 6, NULL );
//	::LineTo(	pDis->hDC, rc.right + 6 + 5	, rc.top + 6 );
//	::MoveToEx( pDis->hDC, rc.right + 7		, rc.top + 7, NULL );
//	::LineTo(	pDis->hDC, rc.right + 7 + 3	, rc.top + 7 );
//	::MoveToEx( pDis->hDC, rc.right + 8		, rc.top + 8, NULL );
//	::LineTo(	pDis->hDC, rc.right + 8 + 1	, rc.top + 8 );
//	::SelectObject( pDis->hDC, hPenOld );
//	::DeleteObject( hPen );

	/* フォーカスの長方形 */
	if( pDis->itemState & ODS_FOCUS ){
		rcFocus.top -= 3;
		rcFocus.left -= 3;
		rcFocus.right += 2;
		rcFocus.bottom += 2;
		::DrawFocusRect( pDis->hDC, &rcFocus );
	}
	return;
}










//	キーワード：タイプ別設定タブ順序(プロパティシート)
/* プロパティシートの作成 */
int CPropTypes::DoPropertySheet( int nPageNum )
{
	int					nRet;
	PROPSHEETPAGE		psp[16];
	int					nIdx;

	m_nMaxLineKetas_org = m_Types.m_nMaxLineKetas;

	// 2005.11.30 Moca カスタム色の先頭にテキスト色を設定しておく
	m_dwCustColors[0] = m_Types.m_ColorInfoArr[COLORIDX_TEXT].m_colTEXT;
	m_dwCustColors[1] = m_Types.m_ColorInfoArr[COLORIDX_TEXT].m_colBACK;

	nIdx = 0;
	memset_raw( &psp[nIdx], 0, sizeof_raw( psp[nIdx] ) );
	psp[nIdx].dwSize      = sizeof_raw( psp[nIdx] );
	psp[nIdx].dwFlags     = /*PSP_USEICONID |*/ PSP_USETITLE | PSP_HASHELP;
	psp[nIdx].hInstance   = m_hInstance;
	psp[nIdx].pszTemplate = MAKEINTRESOURCE( IDD_PROPTYPESP1 );
	psp[nIdx].pszIcon     = NULL;
	psp[nIdx].pfnDlgProc  = (DLGPROC)PropTypesP1Proc;
	psp[nIdx].pszTitle    = _T("スクリーン");
	psp[nIdx].lParam      = (LPARAM)this;
	psp[nIdx].pfnCallback = NULL;
	nIdx++;

	memset_raw( &psp[nIdx], 0, sizeof_raw( psp[nIdx] ) );
	psp[nIdx].dwSize      = sizeof_raw( psp[nIdx] );
	psp[nIdx].dwFlags     = /*PSP_USEICONID |*/ PSP_USETITLE | PSP_HASHELP;
	psp[nIdx].hInstance   = m_hInstance;
	psp[nIdx].pszTemplate = MAKEINTRESOURCE( IDD_PROP_COLOR );
	psp[nIdx].pszIcon     = NULL /*MAKEINTRESOURCE( IDI_BORDER) */;
	psp[nIdx].pfnDlgProc  = (DLGPROC)PropTypesP3_newProc;
	psp[nIdx].pszTitle    = _T("カラー");
	psp[nIdx].lParam      = (LPARAM)this;
	psp[nIdx].pfnCallback = NULL;
	nIdx++;

	// 2001/06/14 Start by asa-o: タイプ別設定に支援タブ追加
	memset_raw( &psp[nIdx], 0, sizeof_raw( psp[nIdx] ) );
	psp[nIdx].dwSize      = sizeof_raw( psp[nIdx] );
	psp[nIdx].dwFlags     = PSP_USETITLE | PSP_HASHELP;
	psp[nIdx].hInstance   = m_hInstance;
	psp[nIdx].pszTemplate = MAKEINTRESOURCE( IDD_PROPTYPESP2 );
	psp[nIdx].pszIcon     = NULL;
	psp[nIdx].pfnDlgProc  = (DLGPROC)PropTypesP2Proc;
	psp[nIdx].pszTitle    = _T("支援");
	psp[nIdx].lParam      = (LPARAM)this;
	psp[nIdx].pfnCallback = NULL;
	nIdx++;
	// 2001/06/14 End

	// 2001.11.17 add start MIK タイプ別設定に正規表現キーワードタブ追加
	memset_raw( &psp[nIdx], 0, sizeof_raw( psp[nIdx] ) );
	psp[nIdx].dwSize      = sizeof_raw( psp[nIdx] );
	psp[nIdx].dwFlags     = PSP_USETITLE | PSP_HASHELP;
	psp[nIdx].hInstance   = m_hInstance;
	psp[nIdx].pszTemplate = MAKEINTRESOURCE( IDD_PROP_REGEX );
	psp[nIdx].pszIcon     = NULL;
	psp[nIdx].pfnDlgProc  = (DLGPROC)PropTypesRegex;
	psp[nIdx].pszTitle    = _T("正規表現キーワード");
	psp[nIdx].lParam      = (LPARAM)this;
	psp[nIdx].pfnCallback = NULL;
	nIdx++;
	// 2001.11.17 add end MIK

	// 2006.04.10 fon ADD-start タイプ別設定に「キーワードヘルプ」タブを追加
	memset_raw( &psp[nIdx], 0, sizeof_raw( psp[nIdx] ) );
	psp[nIdx].dwSize      = sizeof_raw( psp[nIdx] );
	psp[nIdx].dwFlags     = PSP_USETITLE | PSP_HASHELP;
	psp[nIdx].hInstance   = m_hInstance;
	psp[nIdx].pszTemplate = MAKEINTRESOURCE( IDD_PROP_KEYHELP );
	psp[nIdx].pszIcon     = NULL;
	psp[nIdx].pfnDlgProc  = (DLGPROC)PropTypesKeyHelp;
	psp[nIdx].pszTitle    = _T("キーワードヘルプ");
	psp[nIdx].lParam      = (LPARAM)this;
	psp[nIdx].pfnCallback = NULL;
	nIdx++;
	// 2006.04.10 fon ADD-end


	PROPSHEETHEADER		psh;
	memset_raw( &psh, 0, sizeof_raw( psh ) );
	
	//	Jun. 29, 2002 こおり
	//	Windows 95対策．Property SheetのサイズをWindows95が認識できる物に固定する．
	psh.dwSize = sizeof_old_PROPSHEETHEADER;

	// JEPROtest Sept. 30, 2000 タイプ別設定の隠れ[適用]ボタンの正体はここ。行頭のコメントアウトを入れ替えてみればわかる
	psh.dwFlags    = /*PSH_USEICONID |*/ PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE/* | PSH_HASHELP*/;
	psh.hwndParent = m_hwndParent;
	psh.hInstance  = m_hInstance;
	psh.pszIcon    = NULL;
	psh.pszCaption = _T("タイプ別設定");	// Sept. 8, 2000 jepro 単なる「設定」から変更
	psh.nPages     = nIdx;

	//- 20020106 aroka # psh.nStartPage は unsigned なので負にならない
	if( -1 == nPageNum ){
		psh.nStartPage = m_nPageNum;
	}else
	if( 0 > nPageNum ){			//- 20020106 aroka
		psh.nStartPage = 0;
	}else{
		psh.nStartPage = nPageNum;
	}
	if( psh.nPages - 1 < psh.nStartPage ){
		psh.nStartPage = psh.nPages - 1;
	}
	psh.ppsp = psp;
	psh.pfnCallback = NULL;

//<<<<<<< .mine
//	nRet = ::PropertySheet( &psh );
//=======
//	m_hbmpToolButtons = ::LoadBitmap( m_hInstance, MAKEINTRESOURCE( IDB_MYTOOL ) );

	nRet = MyPropertySheet( &psh );	// 2007.05.24 ryoji 独自拡張プロパティシート
//>>>>>>> .r1121
	if( -1 == nRet ){
		TCHAR*	pszMsgBuf;
		::FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			::GetLastError(),
			MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // デフォルト言語
			(LPTSTR)&pszMsgBuf,
			0,
			NULL
		);
		::MYMESSAGEBOX( NULL, MB_OK | MB_ICONINFORMATION | MB_TOPMOST, _T("作者に教えて欲しいエラー"),
			_T("CPropTypes::DoPropertySheet()内でエラーが出ました。\n")
			_T("psh.nStartPage=[%d]\n")
			_T("::PropertySheet()失敗。\n")
			_T("\n")
			_T("%ls\n"),
			psh.nStartPage,
			pszMsgBuf
		);
		::LocalFree( pszMsgBuf );
	}

//	::DeleteObject( m_hbmpToolButtons );
//	m_hbmpToolButtons = NULL;
	return nRet;
}





/* p1 メッセージ処理 */
INT_PTR CPropTypes::DispatchEvent_p1(
	HWND		hwndDlg,	// handle to dialog box
	UINT		uMsg,		// message
	WPARAM		wParam,		// first message parameter
	LPARAM		lParam 		// second message parameter
)
{
	WORD		wNotifyCode;
	WORD		wID;
//	HWND		hwndCtl;
	NMHDR*		pNMHDR;
	NM_UPDOWN*	pMNUD;
	int			idCtrl;
	int			nVal;
//	LPDRAWITEMSTRUCT pDis;

	switch( uMsg ){

	case WM_INITDIALOG:
		m_hwndThis = hwndDlg;
		/* ダイアログデータの設定 p1 */
		SetData_p1( hwndDlg );
		// Modified by KEITA for WIN64 2003.9.6
		::SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

		/* ユーザーがエディット コントロールに入力できるテキストの長さを制限する */
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_TYPENAME ), EM_LIMITTEXT, (WPARAM)( _countof( m_Types.m_szTypeName ) - 1 ), 0 );
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_TYPEEXTS ), EM_LIMITTEXT, (WPARAM)( _countof( m_Types.m_szTypeExts ) - 1 ), 0 );
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_INDENTCHARS ), EM_LIMITTEXT, (WPARAM)( _countof( m_Types.m_szIndentChars ) - 1 ), 0 );
//#ifdef COMPILE_TAB_VIEW  //@@@ 2001.03.16 by MIK
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_TABVIEWSTRING ), EM_LIMITTEXT, (WPARAM)( _countof( m_Types.m_szTabViewString ) - 1 ), 0 );
//#endif
		//	Oct. 5, 2002 genta 画面上でも入力制限
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_OUTLINERULEFILE ),  EM_LIMITTEXT, (WPARAM)( _countof2( m_Types.m_szOutlineRuleFilename ) - 1 ), 0 );

		if( 0 == m_Types.m_nIdx ){
			::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_TYPENAME ), FALSE );
			::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_TYPEEXTS ), FALSE );
		}

		return TRUE;
	case WM_COMMAND:
		wNotifyCode	= HIWORD(wParam);	/* 通知コード */
		wID			= LOWORD(wParam);	/* 項目ID､ コントロールID､ またはアクセラレータID */
//		hwndCtl		= (HWND) lParam;	/* コントロールのハンドル */
		switch( wNotifyCode ){
		/* ボタン／チェックボックスがクリックされた */
		case BN_CLICKED:
			switch( wID ){
			/*	2002.04.01 YAZAKI オートインデントを削除（もともと不要）
				アウトライン解析にルールファイル関連を追加
			*/
			case IDC_RADIO_OUTLINEDEFAULT:	/* アウトライン解析→標準ルール */
				::EnableWindow( ::GetDlgItem( hwndDlg, IDC_COMBO_OUTLINES ), TRUE );
				::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_OUTLINERULEFILE ), FALSE );
				::EnableWindow( ::GetDlgItem( hwndDlg, IDC_BUTTON_RULEFILE_REF ), FALSE );

				::SendMessageAny( ::GetDlgItem( hwndDlg, IDC_COMBO_OUTLINES ), CB_SETCURSEL, 0, 0 );

				return TRUE;
			case IDC_RADIO_OUTLINERULEFILE:	/* アウトライン解析→ルールファイル */
				::EnableWindow( ::GetDlgItem( hwndDlg, IDC_COMBO_OUTLINES ), FALSE );
				::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_OUTLINERULEFILE ), TRUE );
				::EnableWindow( ::GetDlgItem( hwndDlg, IDC_BUTTON_RULEFILE_REF ), TRUE );
				return TRUE;

			case IDC_BUTTON_RULEFILE_REF:	/* アウトライン解析→ルールファイルの「参照...」ボタン */
				{
					CDlgOpenFile	cDlgOpenFile;
					TCHAR			szPath[_MAX_PATH + 1];
					// 2003.06.23 Moca 相対パスは実行ファイルからのパスとして開く
					// 2007.05.19 ryoji 相対パスは設定ファイルからのパスを優先
					if( _IS_REL_PATH( m_Types.m_szOutlineRuleFilename ) ){
						GetInidirOrExedir( szPath, m_Types.m_szOutlineRuleFilename );
					}else{
						_tcscpy( szPath, m_Types.m_szOutlineRuleFilename );
					}
					/* ファイルオープンダイアログの初期化 */
					cDlgOpenFile.Create(
						m_hInstance,
						hwndDlg,
						_T("*.*"),
						szPath
					);
					if( cDlgOpenFile.DoModal_GetOpenFileName( szPath ) ){
						_tcscpy( m_Types.m_szOutlineRuleFilename, szPath );
						::DlgItem_SetText( hwndDlg, IDC_EDIT_OUTLINERULEFILE, m_Types.m_szOutlineRuleFilename );
					}
				}
				return TRUE;

			case IDC_CHECK_TAB_ARROW:
				// Mar. 31, 2003 genta 矢印表示のON/OFFをTAB文字列設定に連動させる
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_TAB_ARROW ) ){
					::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_TABVIEWSTRING ), FALSE );
				}
				else {
					::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_TABVIEWSTRING ), TRUE );
				}
				return TRUE;

			}
			break;	/* BN_CLICKED */
		}
		break;	/* WM_COMMAND */
	case WM_NOTIFY:
		idCtrl = (int)wParam;
		pNMHDR = (NMHDR*)lParam;
		pMNUD  = (NM_UPDOWN*)lParam;
		switch( idCtrl ){
		case IDC_SPIN_MAXLINELEN:
			/* 折り返し桁数 */
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_MAXLINELEN, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < MINLINEKETAS ){
				nVal = MINLINEKETAS;
			}
			if( nVal > MAXLINEKETAS ){
				nVal = MAXLINEKETAS;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_MAXLINELEN, nVal, FALSE );
			return TRUE;
		case IDC_SPIN_CHARSPACE:
			/* 文字の隙間 */
//			MYTRACE_A( "IDC_SPIN_CHARSPACE\n" );
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_CHARSPACE, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 0 ){
				nVal = 0;
			}
			if( nVal > COLUMNSPACE_MAX ){ // Feb. 18, 2003 genta 最大値の定数化
				nVal = COLUMNSPACE_MAX;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_CHARSPACE, nVal, FALSE );
			return TRUE;
		case IDC_SPIN_LINESPACE:
			/* 行の隙間 */
//			MYTRACE_A( "IDC_SPIN_LINESPACE\n" );
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_LINESPACE, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
//	From Here Oct. 8, 2000 JEPRO 行間も最小0まで設定できるように変更(昔に戻っただけ?)
//			if( nVal < 1 ){
//				nVal = 1;
//			}
			if( nVal < 0 ){
				nVal = 0;
			}
//	To Here  Oct. 8, 2000
			if( nVal > LINESPACE_MAX ){ // Feb. 18, 2003 genta 最大値の定数化
				nVal = LINESPACE_MAX;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_LINESPACE, nVal, FALSE );
			return TRUE;
		case IDC_SPIN_REPEATEDSCROLLLINENUM:
			/* キーリピート時のスクロール行数 */
//			MYTRACE_A( "IDC_SPIN_REPEATEDSCROLLLINENUM\n" );
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_REPEATEDSCROLLLINENUM, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 1 ){
				nVal = 1;
			}
			if( nVal > 10 ){
				nVal = 10;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_REPEATEDSCROLLLINENUM, nVal, FALSE );
			return TRUE;
		case IDC_SPIN_TABSPACE:
			//	Sep. 22, 2002 genta
			/* TAB幅 */
//			MYTRACE_A( "IDC_SPIN_CHARSPACE\n" );
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_TABSPACE, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 1 ){
				nVal = 1;
			}
			if( nVal > 64 ){
				nVal = 64;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_TABSPACE, nVal, FALSE );
			return TRUE;

		default:
			switch( pNMHDR->code ){
			case PSN_HELP:
				OnHelp( hwndDlg, IDD_PROPTYPESP1 );
				return TRUE;
			case PSN_KILLACTIVE:
				/* ダイアログデータの取得 p1 */
				GetData_p1( hwndDlg );

				return TRUE;
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
			case PSN_SETACTIVE:
				m_nPageNum = 0;
				return TRUE;
			}
			break;
		}

//		MYTRACE_A( "pNMHDR->hwndFrom	=%xh\n",	pNMHDR->hwndFrom );
//		MYTRACE_A( "pNMHDR->idFrom	=%xh\n",	pNMHDR->idFrom );
//		MYTRACE_A( "pNMHDR->code		=%xh\n",	pNMHDR->code );
//		MYTRACE_A( "pMNUD->iPos		=%d\n",		pMNUD->iPos );
//		MYTRACE_A( "pMNUD->iDelta		=%d\n",		pMNUD->iDelta );
		break;

//@@@ 2001.02.04 Start by MIK: Popup Help
	case WM_HELP:
		{
			HELPINFO *p = (HELPINFO *)lParam;
			MyWinHelp( (HWND)p->hItemHandle, m_szHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)p_helpids1 );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		}
		return TRUE;
		/*NOTREACHED*/
//		break;
//@@@ 2001.02.04 End

//@@@ 2001.11.17 add start MIK
	//Context Menu
	case WM_CONTEXTMENU:
		MyWinHelp( hwndDlg, m_szHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)p_helpids1 );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		return TRUE;
//@@@ 2001.11.17 add end MIK

	}
	return FALSE;
}





/* ダイアログデータの設定 p1 */
void CPropTypes::SetData_p1( HWND hwndDlg )
{
	BOOL	bRet;
	static	int	nTabArr[] = { 2, 3, 4, 6, 8 };
	static	int	nTabArrNum = _countof( nTabArr );
	int			i;

	/* タイプ属性：名称 */
	::DlgItem_SetText( hwndDlg, IDC_EDIT_TYPENAME, m_Types.m_szTypeName );

	/* タイプ属性：拡張子リスト */
	::DlgItem_SetText( hwndDlg, IDC_EDIT_TYPEEXTS, m_Types.m_szTypeExts );

	/* 折り返し文字数 */
	bRet = ::SetDlgItemInt( hwndDlg, IDC_EDIT_MAXLINELEN, (Int)m_Types.m_nMaxLineKetas, FALSE );

	/* 文字の隙間 */
	bRet = ::SetDlgItemInt( hwndDlg, IDC_EDIT_CHARSPACE, m_Types.m_nColmSpace, FALSE );

	/* 行の隙間 */
	bRet = ::SetDlgItemInt( hwndDlg, IDC_EDIT_LINESPACE, m_Types.m_nLineSpace, FALSE );

	/* TAB幅 */
	//	Sep. 22, 2002 genta
	::SetDlgItemInt( hwndDlg, IDC_EDIT_TABSPACE, (Int)m_Types.m_nTabSpace, FALSE );

	/* TAB表示文字列 */
	::DlgItem_SetText( hwndDlg, IDC_EDIT_TABVIEWSTRING, m_Types.m_szTabViewString );

	//タブ矢印表示	//@@@ 2003.03.26 MIK
	::CheckDlgButton( hwndDlg, IDC_CHECK_TAB_ARROW, m_Types.m_bTabArrow );
	// Mar. 31, 2003 genta 矢印表示のON/OFFをTAB文字列設定に連動させる
	if( m_Types.m_bTabArrow ){
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_TABVIEWSTRING ), FALSE );
	}
	else {
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_TABVIEWSTRING ), TRUE );
	}
// From Here 2001.12.03 hor
	/* スペースの挿入 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_INS_SPACE, m_Types.m_bInsSpace );
// To Here 2001.12.03 hor

	/* その他のインデント対象文字 */
	::DlgItem_SetText( hwndDlg, IDC_EDIT_INDENTCHARS, m_Types.m_szIndentChars );


	/* アウトライン解析方法
	
		2002.04.01 YAZAKI ルールファイル関連追加
	*/
	HWND	hwndCombo;
	int		nSelPos;
	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_OUTLINES );
	::SendMessageAny( hwndCombo, CB_RESETCONTENT, 0, 0 );
	nSelPos = 0;
	for( i = 0; i < _countof( OlmArr ); ++i ){
		::SendMessage( hwndCombo, CB_INSERTSTRING, i, (LPARAM)OlmArr[i].pszName );
		if( OlmArr[i].nMethod == m_Types.m_nDefaultOutline ){	/* アウトライン解析方法 */
			nSelPos = i;
		}
	}

	// 2003.06.23 Moca ルールファイル名は使わなくてもセットしておく
	::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_OUTLINERULEFILE ), TRUE );
	::DlgItem_SetText( hwndDlg, IDC_EDIT_OUTLINERULEFILE, m_Types.m_szOutlineRuleFilename );

	if( m_Types.m_nDefaultOutline == OUTLINE_FILE ){
		::CheckDlgButton( hwndDlg, IDC_RADIO_OUTLINEDEFAULT, FALSE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_OUTLINERULEFILE, TRUE );

		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_COMBO_OUTLINES ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_BUTTON_RULEFILE_REF ), TRUE );

	}else{
		::CheckDlgButton( hwndDlg, IDC_RADIO_OUTLINEDEFAULT, TRUE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_OUTLINERULEFILE, FALSE );

		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_COMBO_OUTLINES ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_OUTLINERULEFILE ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_BUTTON_RULEFILE_REF ), FALSE );

		::SendMessageAny( hwndCombo, CB_SETCURSEL, nSelPos, 0 );
	}

	/* インデント */
	::CheckDlgButton( hwndDlg, IDC_CHECK_INDENT, m_Types.m_bAutoIndent );

	/* 日本語空白もインデント */
	::CheckDlgButton( hwndDlg, IDC_CHECK_INDENT_WSPACE, m_Types.m_bAutoIndent_ZENSPACE );

	/* 2005.10.11 ryoji 改行時に末尾の空白を削除 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_RTRIM_PREVLINE, m_Types.m_bRTrimPrevLine );

	/* スマートインデント種別 */
//	HWND	hwndCombo;
//	int		nSelPos;
	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_SMARTINDENT );
	::SendMessageAny( hwndCombo, CB_RESETCONTENT, 0, 0 );
	nSelPos = 0;
	for( i = 0; i < _countof( SmartIndentArr ); ++i ){
		::SendMessage( hwndCombo, CB_INSERTSTRING, i, (LPARAM)SmartIndentArr[i].pszName );
		if( SmartIndentArr[i].nMethod == m_Types.m_nSmartIndent ){	/* スマートインデント種別 */
			nSelPos = i;
		}
	}
	::SendMessageAny( hwndCombo, CB_SETCURSEL, nSelPos, 0 );

	/* 折り返しは2行目以降を字下げ表示 */
	//	Oct. 1, 2002 genta コンボボックスに変更
	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_INDENTLAYOUT );
	::SendMessageAny( hwndCombo, CB_RESETCONTENT, 0, 0 );
	nSelPos = 0;
	for( i = 0; i < _countof( IndentTypeArr ); ++i ){
		::SendMessage( hwndCombo, CB_INSERTSTRING, i, (LPARAM)IndentTypeArr[i].pszName );
		if( IndentTypeArr[i].nMethod == m_Types.m_nIndentLayout ){	/* 折り返しインデント種別 */
			nSelPos = i;
		}
	}
	::SendMessageAny( hwndCombo, CB_SETCURSEL, nSelPos, 0 );

	//	From Here Nov. 20, 2000 genta
	//	IME入力モード
	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_IMESTATE );
	::SendMessageAny( hwndCombo, CB_RESETCONTENT, 0, 0 );
	int ime = m_Types.m_nImeState >> 2;
	nSelPos = 0;
	for( i = 0; i < _countof( ImeStateArr ); ++i ){
		::SendMessage( hwndCombo, CB_INSERTSTRING, i, (LPARAM)ImeStateArr[i].pszName );
		if( ImeStateArr[i].nMethod == ime ){	/* IME状態 */
			nSelPos = i;
		}
	}
	::SendMessageAny( hwndCombo, CB_SETCURSEL, nSelPos, 0 );
	//	IME ON/OFF制御
	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_IMESWITCH );
	::SendMessageAny( hwndCombo, CB_RESETCONTENT, 0, 0 );
	ime = m_Types.m_nImeState & 3;
	nSelPos = 0;
	for( i = 0; i < _countof( ImeSwitchArr ); ++i ){
		::SendMessage( hwndCombo, CB_INSERTSTRING, i, (LPARAM)ImeSwitchArr[i].pszName );
		if( ImeSwitchArr[i].nMethod == ime ){	/* IME状態 */
			nSelPos = i;
		}
	}
	::SendMessageAny( hwndCombo, CB_SETCURSEL, nSelPos, 0 );
	//	To Here Nov. 20, 2000 genta

	/* 英文ワードラップをする */
	::CheckDlgButton( hwndDlg, IDC_CHECK_WORDWRAP, m_Types.m_bWordWrap );

	/* 禁則処理 */
	{	//@@@ 2002.04.08 MIK start
		::CheckDlgButton( hwndDlg, IDC_CHECK_KINSOKUHEAD, m_Types.m_bKinsokuHead ? TRUE : FALSE );
		::CheckDlgButton( hwndDlg, IDC_CHECK_KINSOKUTAIL, m_Types.m_bKinsokuTail ? TRUE : FALSE );
		::CheckDlgButton( hwndDlg, IDC_CHECK_KINSOKURET,  m_Types.m_bKinsokuRet  ? TRUE : FALSE );	/* 改行文字をぶら下げる */	//@@@ 2002.04.13 MIK
		::CheckDlgButton( hwndDlg, IDC_CHECK_KINSOKUKUTO, m_Types.m_bKinsokuKuto ? TRUE : FALSE );	/* 句読点をぶら下げる */	//@@@ 2002.04.17 MIK
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_KINSOKUHEAD ), EM_LIMITTEXT, _countof(m_Types.m_szKinsokuHead) - 1, 0 );
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_KINSOKUTAIL ), EM_LIMITTEXT, _countof(m_Types.m_szKinsokuTail) - 1, 0 );
		::DlgItem_SetText( hwndDlg, IDC_EDIT_KINSOKUHEAD, m_Types.m_szKinsokuHead );
		::DlgItem_SetText( hwndDlg, IDC_EDIT_KINSOKUTAIL, m_Types.m_szKinsokuTail );
	}	//@@@ 2002.04.08 MIK end
	
	//	Sep. 10, 2002 genta
	::CheckDlgButton( hwndDlg, IDC_CHECK_DOCICON, m_Types.m_bUseDocumentIcon  ? TRUE : FALSE );

	return;
}





/* ダイアログデータの取得 p1 */
int CPropTypes::GetData_p1( HWND hwndDlg )
{
//#ifdef COMPILE_TAB_VIEW  //@@@ 2001.03.16 by MIK
	int  i;
//#endif

//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
//	m_nPageNum = 0;
	/* タイプ属性：名称 */
	::DlgItem_GetText( hwndDlg, IDC_EDIT_TYPENAME, m_Types.m_szTypeName, _countof( m_Types.m_szTypeName ) );
	/* タイプ属性：拡張子リスト */
	::DlgItem_GetText( hwndDlg, IDC_EDIT_TYPEEXTS, m_Types.m_szTypeExts, _countof( m_Types.m_szTypeExts ) );


	/* 折り返し桁数 */
	m_Types.m_nMaxLineKetas = CLayoutInt(::GetDlgItemInt( hwndDlg, IDC_EDIT_MAXLINELEN, NULL, FALSE ));
	if( m_Types.m_nMaxLineKetas < CLayoutInt(MINLINEKETAS) ){
		m_Types.m_nMaxLineKetas = CLayoutInt(MINLINEKETAS);
	}
	if( m_Types.m_nMaxLineKetas > CLayoutInt(MAXLINEKETAS) ){
		m_Types.m_nMaxLineKetas = CLayoutInt(MAXLINEKETAS);
	}

	/* 文字の隙間 */
	m_Types.m_nColmSpace = ::GetDlgItemInt( hwndDlg, IDC_EDIT_CHARSPACE, NULL, FALSE );
	if( m_Types.m_nColmSpace < 0 ){
		m_Types.m_nColmSpace = 0;
	}
	if( m_Types.m_nColmSpace > COLUMNSPACE_MAX ){ // Feb. 18, 2003 genta 最大値の定数化
		m_Types.m_nColmSpace = COLUMNSPACE_MAX;
	}

	/* 行の隙間 */
	m_Types.m_nLineSpace = ::GetDlgItemInt( hwndDlg, IDC_EDIT_LINESPACE, NULL, FALSE );
//	From Here Oct. 8, 2000 JEPRO 行間も最小0まで設定できるように変更(昔に戻っただけ?)
//	if( m_Types.m_nLineSpace < 1 ){
//		m_Types.m_nLineSpace = 1;
//	}
	if( m_Types.m_nLineSpace < 0 ){
		m_Types.m_nLineSpace = 0;
	}
//	To Here  Oct. 8, 2000
	if( m_Types.m_nLineSpace > LINESPACE_MAX ){	// Feb. 18, 2003 genta 最大値の定数化
		m_Types.m_nLineSpace = LINESPACE_MAX;
	}

	/* その他のインデント対象文字 */
	::DlgItem_GetText( hwndDlg, IDC_EDIT_INDENTCHARS, m_Types.m_szIndentChars, _countof( m_Types.m_szIndentChars ) - 1 );

	/* TAB幅 */
	m_Types.m_nTabSpace = CLayoutInt(::GetDlgItemInt( hwndDlg, IDC_EDIT_TABSPACE, NULL, FALSE ));
	if( m_Types.m_nTabSpace < CLayoutInt(1) ){
		m_Types.m_nTabSpace = CLayoutInt(1);
	}
	if( m_Types.m_nTabSpace > CLayoutInt(64) ){
		m_Types.m_nTabSpace = CLayoutInt(64);
	}

//#ifdef COMPILE_TAB_VIEW  //@@@ 2001.03.16 by MIK
	/* TAB表示文字列 */
	WIN_CHAR szTab[8+1+1]; /* +1. happy */
	::DlgItem_GetText( hwndDlg, IDC_EDIT_TABVIEWSTRING, szTab, _countof( szTab ) - 1 );
	wcscpy( m_Types.m_szTabViewString, L"^       " );
	for( i = 0; i < 8; i++ ){
		if( !TCODE::isTabAvailableCode(szTab[i]) )break;
		m_Types.m_szTabViewString[i] = szTab[i];
	}
//#endif

	//タブ矢印表示	//@@@ 2003.03.26 MIK
	m_Types.m_bTabArrow = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_TAB_ARROW );
// 2001.12.03 hor
	/* スペースの挿入 */
	m_Types.m_bInsSpace = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_INS_SPACE );
// From Here 2001.12.03 hor

	/* アウトライン解析方法
	
		2002.04.01 YAZAKI ルールファイル関連追加
	*/
	HWND	hwndCombo;
	int		nSelPos;
	
	// 2003.06.23 Moca ルールを使っていなくてもファイル名を保持
	::DlgItem_GetText( hwndDlg, IDC_EDIT_OUTLINERULEFILE, m_Types.m_szOutlineRuleFilename, _countof2( m_Types.m_szOutlineRuleFilename ));

	if ( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_OUTLINERULEFILE) ){
		m_Types.m_nDefaultOutline = OUTLINE_FILE;
	}
	else {
		hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_OUTLINES );
		nSelPos = ::SendMessageAny( hwndCombo, CB_GETCURSEL, 0, 0 );
		m_Types.m_nDefaultOutline = OlmArr[nSelPos].nMethod;	/* アウトライン解析方法 */
	}

	/* インデント */
	m_Types.m_bAutoIndent = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_INDENT );

	/* 日本語空白もインデント */
	m_Types.m_bAutoIndent_ZENSPACE = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_INDENT_WSPACE );

	/* 2005.10.11 ryoji 改行時に末尾の空白を削除 */
	m_Types.m_bRTrimPrevLine = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_RTRIM_PREVLINE );

	/* スマートインデント種別 */
//	HWND	hwndCombo;
//	int		nSelPos;
	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_SMARTINDENT );
	nSelPos = ::SendMessageAny( hwndCombo, CB_GETCURSEL, 0, 0 );
	m_Types.m_nSmartIndent = SmartIndentArr[nSelPos].nMethod;	/* スマートインデント種別 */

	/* 折り返しは2行目以降を字下げ表示 */
	//	Oct. 1, 2002 genta コンボボックスに変更
	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_INDENTLAYOUT );
	nSelPos = ::SendMessageAny( hwndCombo, CB_GETCURSEL, 0, 0 );
	m_Types.m_nIndentLayout = IndentTypeArr[nSelPos].nMethod;	/* 折り返し部インデント種別 */

	//	From Here Nov. 20, 2000 genta	IME状態
	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_IMESTATE );
	nSelPos = ::SendMessageAny( hwndCombo, CB_GETCURSEL, 0, 0 );
	m_Types.m_nImeState = ImeStateArr[nSelPos].nMethod << 2;	//	IME入力モード

	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_IMESWITCH );
	nSelPos = ::SendMessageAny( hwndCombo, CB_GETCURSEL, 0, 0 );
	m_Types.m_nImeState |= ImeSwitchArr[nSelPos].nMethod;	//	IME ON/OFF
	//	To Here Nov. 20, 2000 genta

	/* 英文ワードラップをする */
	m_Types.m_bWordWrap = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_WORDWRAP );

	/* 禁則処理 */
	{	//@@@ 2002.04.08 MIK start
		m_Types.m_bKinsokuHead = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_KINSOKUHEAD ) ? TRUE : FALSE;
		m_Types.m_bKinsokuTail = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_KINSOKUTAIL ) ? TRUE : FALSE;
		m_Types.m_bKinsokuRet  = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_KINSOKURET  ) ? TRUE : FALSE;	/* 改行文字をぶら下げる */	//@@@ 2002.04.13 MIK
		m_Types.m_bKinsokuKuto = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_KINSOKUKUTO ) ? TRUE : FALSE;	/* 句読点をぶら下げる */	//@@@ 2002.04.17 MIK
		::DlgItem_GetText( hwndDlg, IDC_EDIT_KINSOKUHEAD, m_Types.m_szKinsokuHead, _countof( m_Types.m_szKinsokuHead ) );
		::DlgItem_GetText( hwndDlg, IDC_EDIT_KINSOKUTAIL, m_Types.m_szKinsokuTail, _countof( m_Types.m_szKinsokuTail ) );
	}	//@@@ 2002.04.08 MIK end

	//	Sep. 10, 2002 genta
	m_Types.m_bUseDocumentIcon = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_DOCICON ) ? TRUE : FALSE;

	return TRUE;
}



// 2001/06/13 Start By asa-o: タイプ別設定の支援タブに関する処理

/* p2 メッセージ処理 */
INT_PTR CPropTypes::DispatchEvent_p2(
	HWND		hwndDlg,	// handle to dialog box
	UINT		uMsg,		// message
	WPARAM		wParam,		// first message parameter
	LPARAM		lParam 		// second message parameter
)
{
	WORD		wNotifyCode;
	WORD		wID;
	NMHDR*		pNMHDR;

	switch( uMsg ){
	case WM_INITDIALOG:
		/* ダイアログデータの設定 p2 */
		SetData_p2( hwndDlg );
		// Modified by KEITA for WIN64 2003.9.6
		::SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

		/* ユーザーがエディット コントロールに入力できるテキストの長さを制限する */
		/* 入力補完 単語ファイル */
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_HOKANFILE ), EM_LIMITTEXT, (WPARAM)(_MAX_PATH - 1 ), 0 );
		/* キーワードヘルプ 辞書ファイル */
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_KEYWORDHELPFILE ), EM_LIMITTEXT, (WPARAM)(_MAX_PATH - 1 ), 0 );

		return TRUE;
	case WM_COMMAND:
		wNotifyCode = HIWORD(wParam);	/* 通知コード */
		wID			= LOWORD(wParam);	/* 項目ID､ コントロールID､ またはアクセラレータID */
//		hwndCtl		= (HWND) lParam;	/* コントロールのハンドル */
		switch( wNotifyCode ){
		/* ボタン／チェックボックスがクリックされた */
		case BN_CLICKED:
			/* ダイアログデータの取得 p2 */
			GetData_p2( hwndDlg );
			switch( wID ){
			case IDC_BUTTON_HOKANFILE_REF:	/* 入力補完 単語ファイルの「参照...」ボタン */
				{
					CDlgOpenFile	cDlgOpenFile;
					TCHAR			szPath[_MAX_PATH + 1];
					// 2003.06.23 Moca 相対パスは実行ファイルからのパスとして開く
					// 2007.05.19 ryoji 相対パスは設定ファイルからのパスを優先
					if( _IS_REL_PATH( m_Types.m_szHokanFile ) ){
						GetInidirOrExedir( szPath, m_Types.m_szHokanFile );
					}else{
						_tcscpy( szPath, m_Types.m_szHokanFile );
					}
					/* ファイルオープンダイアログの初期化 */
					cDlgOpenFile.Create(
						m_hInstance,
						hwndDlg,
						_T("*.*"),
						szPath
					);
					if( cDlgOpenFile.DoModal_GetOpenFileName( szPath ) ){
						_tcscpy( m_Types.m_szHokanFile, szPath );
						::DlgItem_SetText( hwndDlg, IDC_EDIT_HOKANFILE, m_Types.m_szHokanFile );
					}
				}
				return TRUE;
			case IDC_BUTTON_TYPEOPENHELP:	/* 外部ヘルプ１の「参照...」ボタン */
				{
					CDlgOpenFile	cDlgOpenFile;
					TCHAR			szPath[_MAX_PATH + 1];
					// 2003.06.23 Moca 相対パスは実行ファイルからのパスとして開く
					// 2007.05.21 ryoji 相対パスは設定ファイルからのパスを優先
					if( _IS_REL_PATH( m_Types.m_szExtHelp ) ){
						GetInidirOrExedir( szPath, m_Types.m_szExtHelp, TRUE );
					}else{
						_tcscpy( szPath, m_Types.m_szExtHelp );
					}
					/* ファイルオープンダイアログの初期化 */
					cDlgOpenFile.Create(
						m_hInstance,
						hwndDlg,
						_T("*.hlp"),
						szPath
					);
					if( cDlgOpenFile.DoModal_GetOpenFileName( szPath ) ){
						_tcscpy( m_Types.m_szExtHelp, szPath );
						::DlgItem_SetText( hwndDlg, IDC_EDIT_TYPEEXTHELP, m_Types.m_szExtHelp );
					}
				}
				return TRUE;
			case IDC_BUTTON_TYPEOPENEXTHTMLHELP:	/* 外部HTMLヘルプの「参照...」ボタン */
				{
					CDlgOpenFile	cDlgOpenFile;
					TCHAR			szPath[_MAX_PATH + 1];
					// 2003.06.23 Moca 相対パスは実行ファイルからのパスとして開く
					// 2007.05.21 ryoji 相対パスは設定ファイルからのパスを優先
					if( _IS_REL_PATH( m_Types.m_szExtHtmlHelp ) ){
						GetInidirOrExedir( szPath, m_Types.m_szExtHtmlHelp, TRUE );
					}else{
						_tcscpy( szPath, m_Types.m_szExtHtmlHelp );
					}
					/* ファイルオープンダイアログの初期化 */
					cDlgOpenFile.Create(
						m_hInstance,
						hwndDlg,
						_T("*.chm;*.col"),
						szPath
					);
					if( cDlgOpenFile.DoModal_GetOpenFileName( szPath ) ){
						_tcscpy( m_Types.m_szExtHtmlHelp, szPath );
						::DlgItem_SetText( hwndDlg, IDC_EDIT_TYPEEXTHTMLHELP, m_Types.m_szExtHtmlHelp );
					}
				}
				return TRUE;
			}
			break;	/* BN_CLICKED */
		}
		break;	/* WM_COMMAND */
	case WM_NOTIFY:
//		idCtrl = (int)wParam;
		pNMHDR = (NMHDR*)lParam;
//		pMNUD  = (NM_UPDOWN*)lParam;
		switch( pNMHDR->code ){
		case PSN_HELP:	//Jul. 03, 2001 JEPRO 支援タブのヘルプを有効化
			OnHelp( hwndDlg, IDD_PROPTYPESP2 );
			return TRUE;
		case PSN_KILLACTIVE:
			/* ダイアログデータの取得 p2 */
			GetData_p2( hwndDlg );
			return TRUE;
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
		case PSN_SETACTIVE:
			m_nPageNum = 2;
			return TRUE;
		}
		break;

//From Here Jul. 05, 2001 JEPRO: Popup Help
	case WM_HELP:
		{
			HELPINFO *p = (HELPINFO *)lParam;
			MyWinHelp( (HWND)p->hItemHandle, m_szHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)p_helpids3 );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		}
		return TRUE;
		/*NOTREACHED*/
//		break;
//To Here  Jul. 05, 2001

//@@@ 2001.11.17 add start MIK
	//Context Menu
	case WM_CONTEXTMENU:
		MyWinHelp( hwndDlg, m_szHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)p_helpids3 );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		return TRUE;
//@@@ 2001.11.17 add end MIK

	}
	return FALSE;
}

/* ダイアログデータの設定 p2 */
void CPropTypes::SetData_p2( HWND hwndDlg )
{
	/* 入力補完 単語ファイル */
	::DlgItem_SetText( hwndDlg, IDC_EDIT_HOKANFILE, m_Types.m_szHokanFile );

//	2001/06/19 asa-o
	/* 入力補完機能：英大文字小文字を同一視する */
	::CheckDlgButton( hwndDlg, IDC_CHECK_HOKANLOHICASE, m_Types.m_bHokanLoHiCase );

	// 2003.06.25 Moca ファイルからの補完機能
	::CheckDlgButton( hwndDlg, IDC_CHECK_HOKANBYFILE, m_Types.m_bUseHokanByFile );

	//@@@ 2002.2.2 YAZAKI
	::DlgItem_SetText( hwndDlg, IDC_EDIT_TYPEEXTHELP, m_Types.m_szExtHelp );
	::DlgItem_SetText( hwndDlg, IDC_EDIT_TYPEEXTHTMLHELP, m_Types.m_szExtHtmlHelp );
	::CheckDlgButton( hwndDlg, IDC_CHECK_TYPEHTMLHELPISSINGLE, m_Types.m_bHtmlHelpIsSingle );
	return;
}

/* ダイアログデータの取得 p2 */
int CPropTypes::GetData_p2( HWND hwndDlg )
{
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
//	m_nPageNum = 2;

//	2001/06/19	asa-o
	/* 入力補完機能：英大文字小文字を同一視する */
	m_Types.m_bHokanLoHiCase = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_HOKANLOHICASE );

	m_Types.m_bUseHokanByFile = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_HOKANBYFILE );

	/* 入力補完 単語ファイル */
	::DlgItem_GetText( hwndDlg, IDC_EDIT_HOKANFILE, m_Types.m_szHokanFile, _countof2( m_Types.m_szHokanFile ));

	//@@@ 2002.2.2 YAZAKI
	::DlgItem_GetText( hwndDlg, IDC_EDIT_TYPEEXTHELP, m_Types.m_szExtHelp, _countof2( m_Types.m_szExtHelp ));
	::DlgItem_GetText( hwndDlg, IDC_EDIT_TYPEEXTHTMLHELP, m_Types.m_szExtHtmlHelp, _countof2( m_Types.m_szExtHtmlHelp ));
	m_Types.m_bHtmlHelpIsSingle = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_TYPEHTMLHELPISSINGLE );
	return TRUE;
}

// 2001/06/13 End




/* 色の設定をインポート */
void CPropTypes::p3_Import_Colors( HWND hwndDlg )
{
	/* ファイルオープンダイアログの初期化 */
	TCHAR*			pszMRU        = NULL;
	TCHAR*			pszOPENFOLDER = NULL;
	CDlgOpenFile	cDlgOpenFile;
	cDlgOpenFile.Create2(
		m_hInstance,
		hwndDlg,
		_T("*.col"),
		m_pShareData->m_szIMPORTFOLDER, // インポート用フォルダ
		&pszMRU,
		&pszOPENFOLDER
	);
	TCHAR	szPath[_MAX_PATH + 1] = _T("");
	if( !cDlgOpenFile.DoModal_GetOpenFileName( szPath ) ){
		return;
	}

	/* ファイルのフルパスを、フォルダとファイル名に分割 */
	/* [c:\work\test\aaa.txt] → [c:\work\test] + [aaa.txt] */
	::SplitPath_FolderAndFile( szPath, m_pShareData->m_szIMPORTFOLDER, NULL );
	_tcscat( m_pShareData->m_szIMPORTFOLDER, _T("\\") );


	/* 色設定Ver1か */
	CTextInputStream in(szPath);
	if(!in){
		ErrorMessage(hwndDlg, _T("ファイルを開けませんでした。\n\n%ts"), szPath );
		return;
	}

	/* ファイル先頭 */
	//ヘッダ読取
	wstring szHeader = in.ReadLineW().c_str();
	if(szHeader.length()>=2)szHeader=&szHeader.c_str()[2]; //コメントを抜く
	//比較
	if(wcscmp(szHeader.c_str(),LTEXT(STR_COLORDATA_HEAD3))==0){
		//OK
	}
	else{
		ErrorMessage_A( hwndDlg,
			"色設定ファイルの形式が違います。古い形式はサポートされなくなりました。\n%ts\n\n"	//Jan. 20, 2001 JEPRO 改行を1つ取った
			"色設定Ver3では CI[番号] から C[名前] に変更されました。\n"
			"上記ファイルの設定内容を利用したい場合は、以下の修正を行ってからインポートしてください。\n\n"
			"・UR1.2.24.0 (00/12/04) 以降で使っていた場合は\n"
			"  (1) 一行目を Ver3 と書き換え、CI をすべて C に縮める\n"
			"  (2) (1)の後、番号を( )内の文字列に変更:\n"
			"      00(TXT), 01(RUL), 02(UND), 03(LNO), 04(MOD), 05(TAB), 06(ZEN), 07(CTL), 08(EOL),\n"
			"      09(RAP), 10(EOF), 11(FND), 12(KW1), 13(KW), 14(CMT), 15(SQT), 16(WQT), 17(URL)\n\n"
			"・ur3β10 (00/09/28)〜UR1.2.23.0 (00/11/29) で使っていた場合は\n"
			"  (3) (1)の後、00-12 までは(2)と同じ  13(CMT), 14(SQT), 15(WQT), 16(URL)\n\n"
			"・ur3β9 (00/09/26) 以前で使っていた場合は\n"
			"  (4) (1)の後、(2)と同様:\n"
			"      00(TXT), 01(LNO), 02(EOL), 03(TAB), 04(ZEN), 05(EOF), 06(KW1), 07(CMT), 08(SQT),\n"
			"      09(WQT), 10(UND), 11(RAP), 12(CTL), 13(URL), 14(FND), 15(MOD), 16(RUL)\n\n"
			, szPath
		);
		in.Close();
		return;
	}
	in.Close();


	CDataProfile	cProfile;
	cProfile.SetReadingMode();

	int				nColorInfoArrNum;						/* キー割り当て表の有効データ数 */
	ColorInfo		ColorInfoArr[64];

	/* 色設定Ver2 */
	nColorInfoArrNum = COLORIDX_LAST;
	if( !cProfile.ReadProfile( szPath ) ){
		/* 設定ファイルが存在しない */
		ErrorMessage( hwndDlg, _T("ファイルを開けませんでした。\n\n%ts"), szPath );
		return;
	}
	/* 色設定 I/O */
	for( int i = 0; i < m_Types.m_nColorInfoArrNum; ++i ){
		ColorInfoArr[i] = m_Types.m_ColorInfoArr[i];
		_tcscpy( ColorInfoArr[i].m_szName, m_Types.m_ColorInfoArr[i].m_szName );
	}
	CShareData::IO_ColorSet( &cProfile, LTEXT(STR_COLORDATA_SECTION), ColorInfoArr );


//complete:;
	/* データのコピー */
	m_Types.m_nColorInfoArrNum = nColorInfoArrNum;
	for( int i = 0; i < m_Types.m_nColorInfoArrNum; ++i ){
		m_Types.m_ColorInfoArr[i] =  ColorInfoArr[i];
		_tcscpy( m_Types.m_ColorInfoArr[i].m_szName, ColorInfoArr[i].m_szName );
	}
	/* ダイアログデータの設定 p5 */
	SetData_p3_new( hwndDlg );
	return;
}


/* 色の設定をエクスポート */
void CPropTypes::p3_Export_Colors( HWND hwndDlg )
{
	/* ファイルオープンダイアログの初期化 */
	CDlgOpenFile	cDlgOpenFile;
	TCHAR*			pszMRU                = NULL;
	TCHAR*			pszOPENFOLDER         = NULL;
	TCHAR			szPath[_MAX_PATH + 1] = _T("");
	cDlgOpenFile.Create2(
		m_hInstance,
		hwndDlg,
		_T("*.col"),
		m_pShareData->m_szIMPORTFOLDER, // インポート用フォルダ
		&pszMRU,
		&pszOPENFOLDER
	);
	if( !cDlgOpenFile.DoModal_GetSaveFileName( szPath ) ){
		return;
	}

	/* ファイルのフルパスをフォルダとファイル名に分割 */
	/* [c:\work\test\aaa.txt] → [c:\work\test] + [aaa.txt] */
	::SplitPath_FolderAndFile( szPath, m_pShareData->m_szIMPORTFOLDER, NULL );
	_tcscat( m_pShareData->m_szIMPORTFOLDER, _T("\\") );

	/* 色設定 I/O */
	CDataProfile	cProfile;
	cProfile.SetWritingMode();
	CShareData::IO_ColorSet( &cProfile, LTEXT(STR_COLORDATA_SECTION), m_Types.m_ColorInfoArr );
	cProfile.WriteProfile( szPath, LTEXT(STR_COLORDATA_HEAD3) );	//Jan. 15, 2001 Stonee

	return;
}


LRESULT APIENTRY ColorList_SubclassProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	int			xPos;
	int			yPos;
	int			nIndex;
	int			nItemNum;
	RECT		rcItem;
	int			i;
	POINT		poMouse;
	ColorInfo*	pColorInfo;

	switch( uMsg ){
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONUP:
//		fwKeys = wParam;		// key flags
		xPos = LOWORD(lParam);	// horizontal position of cursor
		yPos = HIWORD(lParam);	// vertical position of cursor

		poMouse.x = xPos;
		poMouse.y = yPos;
		nItemNum = ::SendMessageAny( hwnd, LB_GETCOUNT, 0, 0 );
		nIndex = -1;
		for( i = 0; i < nItemNum; ++i ){
			::SendMessageAny( hwnd, LB_GETITEMRECT, i, (LPARAM)&rcItem );
			if( ::PtInRect( &rcItem, poMouse ) ){
//				MYTRACE_A( "hit at i==%d\n", i );
//				MYTRACE_A( "\n" );
				nIndex = i;
				break;
			}
		}
		break;
	}
	switch( uMsg ){
	case WM_RBUTTONDOWN:

		if( -1 == nIndex ){
			break;
		}
		::SendMessageAny( hwnd, LB_SETCURSEL, nIndex, 0 );
		::SendMessageCmd( ::GetParent( hwnd ), WM_COMMAND, MAKELONG( IDC_LIST_COLORS, LBN_SELCHANGE ), (LPARAM)hwnd );
		pColorInfo = (ColorInfo*)::SendMessageAny( hwnd, LB_GETITEMDATA, nIndex, 0 );
		/* 下線 */
		if( 0 == (g_ColorAttributeArr[nIndex].fAttribute & COLOR_ATTRIB_NO_UNDERLINE) )	// 2006.12.18 ryoji フラグ利用で簡素化
		{
			if( pColorInfo->m_bUnderLine ){	/* 下線で表示 */
				pColorInfo->m_bUnderLine = FALSE;
				::CheckDlgButton( ::GetParent( hwnd ), IDC_CHECK_UNDERLINE, FALSE );
			}else{
				pColorInfo->m_bUnderLine = TRUE;
				::CheckDlgButton( ::GetParent( hwnd ), IDC_CHECK_UNDERLINE, TRUE );
			}
			::InvalidateRect( hwnd, &rcItem, TRUE );
		}
		break;

	case WM_LBUTTONDBLCLK:
		if( -1 == nIndex ){
			break;
		}
		pColorInfo = (ColorInfo*)::SendMessageAny( hwnd, LB_GETITEMDATA, nIndex, 0 );
		/* 太字で表示 */
		if( 0 == (g_ColorAttributeArr[nIndex].fAttribute & COLOR_ATTRIB_NO_BOLD) )	// 2006.12.18 ryoji フラグ利用で簡素化
		{
			if( pColorInfo->m_bFatFont ){	/* 太字で表示 */
				pColorInfo->m_bFatFont = FALSE;
				::CheckDlgButton( ::GetParent( hwnd ), IDC_CHECK_FAT, FALSE );
			}else{
				pColorInfo->m_bFatFont = TRUE;
				::CheckDlgButton( ::GetParent( hwnd ), IDC_CHECK_FAT, TRUE );
			}
			::InvalidateRect( hwnd, &rcItem, TRUE );
		}
		break;
	case WM_LBUTTONUP:
		if( -1 == nIndex ){
			break;
		}
		pColorInfo = (ColorInfo*)::SendMessageAny( hwnd, LB_GETITEMDATA, nIndex, 0 );
		/* 色分け/表示 する */
		if( 2 <= xPos && xPos <= 16
			&& ( 0 == (g_ColorAttributeArr[nIndex].fAttribute & COLOR_ATTRIB_FORCE_DISP) )	// 2006.12.18 ryoji フラグ利用で簡素化
			)
		{
			if( pColorInfo->m_bDisp ){	/* 色分け/表示する */
				pColorInfo->m_bDisp = FALSE;
			}else{
				pColorInfo->m_bDisp = TRUE;
			}
			if( COLORIDX_GYOU == nIndex ){
				pColorInfo = (ColorInfo*)::SendMessageAny( hwnd, LB_GETITEMDATA, nIndex, 0 );

			}

			::InvalidateRect( hwnd, &rcItem, TRUE );
		}else
		/* 前景色見本 矩形 */
		if( rcItem.right - 27 <= xPos && xPos <= rcItem.right - 27 + 12 ){
			/* 色選択ダイアログ */
			// 2005.11.30 Moca カスタム色保持
			DWORD* pColors = (DWORD*)::GetProp( hwnd, _T("ptrCustomColors") );
			if( CPropTypes::SelectColor( hwnd, &pColorInfo->m_colTEXT, pColors ) ){
				::InvalidateRect( hwnd, &rcItem, TRUE );
				::InvalidateRect( ::GetDlgItem( ::GetParent( hwnd ), IDC_BUTTON_TEXTCOLOR ), NULL, TRUE );
			}
		}else
		/* 前景色見本 矩形 */
		if( rcItem.right - 13 <= xPos && xPos <= rcItem.right - 13 + 12
			&& ( 0 == (g_ColorAttributeArr[nIndex].fAttribute & COLOR_ATTRIB_NO_BACK) )	// 2006.12.18 ryoji フラグ利用で簡素化
			)
		{
			/* 色選択ダイアログ */
			// 2005.11.30 Moca カスタム色保持
			DWORD* pColors = (DWORD*)::GetProp( hwnd, _T("ptrCustomColors") );
			if( CPropTypes::SelectColor( hwnd, &pColorInfo->m_colBACK, pColors ) ){
				::InvalidateRect( hwnd, &rcItem, TRUE );
				::InvalidateRect( ::GetDlgItem( ::GetParent( hwnd ), IDC_BUTTON_BACKCOLOR ), NULL, TRUE );
			}
		}
		break;
	// 2005.11.30 Moca カスタム色保持
	case WM_DESTROY:
		if( ::GetProp( hwnd, _T("ptrCustomColors") ) ){
			::RemoveProp( hwnd, _T("ptrCustomColors") );
		}
		break;
	}
//	return CallWindowProc( (int (__stdcall *)(void))(WNDPROC)m_wpColorListProc, hwnd, uMsg, wParam, lParam );
	return CallWindowProc( (WNDPROC)m_wpColorListProc, hwnd, uMsg, wParam, lParam );
}





/* p3 メッセージ処理 */
INT_PTR CPropTypes::DispatchEvent_p3_new(
	HWND				hwndDlg,	// handle to dialog box
	UINT				uMsg,		// message
	WPARAM				wParam,		// first message parameter
	LPARAM				lParam 		// second message parameter
)
{
	WORD				wNotifyCode;
	WORD				wID;
	HWND				hwndCtl;
	NMHDR*				pNMHDR;
	NM_UPDOWN*			pMNUD;
	int					idCtrl;
	int					nVal;
	int					nIndex;
	static HWND			hwndListColor;
	LPDRAWITEMSTRUCT	pDis;

	switch( uMsg ){
	case WM_INITDIALOG:
		// Modified by KEITA for WIN64 2003.9.6
		::SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

		hwndListColor = ::GetDlgItem( hwndDlg, IDC_LIST_COLORS );

		/* ダイアログデータの設定 p3 */
		SetData_p3_new( hwndDlg );

		/* ユーザーがエディット コントロールに入力できるテキストの長さを制限する */
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_LINETERMCHAR ), EM_LIMITTEXT, (WPARAM)1, 0 );


		/* 色リストをフック */
		// Modified by KEITA for WIN64 2003.9.6
		m_wpColorListProc = (WNDPROC) ::SetWindowLongPtr( hwndListColor, GWLP_WNDPROC, (LONG_PTR)ColorList_SubclassProc );
		// 2005.11.30 Moca カスタム色を保持
		::SetProp( hwndListColor, _T("ptrCustomColors"), m_dwCustColors );
		
		return TRUE;

	case WM_COMMAND:
		wNotifyCode	= HIWORD( wParam );	/* 通知コード */
		wID			= LOWORD( wParam );	/* 項目ID､ コントロールID､ またはアクセラレータID */
		hwndCtl		= (HWND) lParam;	/* コントロールのハンドル */
		if( hwndListColor == hwndCtl ){
			switch( wNotifyCode ){
			case LBN_SELCHANGE:
				nIndex = ::SendMessageAny( hwndListColor, LB_GETCURSEL, 0, 0 );
				m_nCurrentColorType = nIndex;		/* 現在選択されている色タイプ */

				{
					// 各種コントロールの有効／無効を切り替える	// 2006.12.18 ryoji フラグ利用で簡素化
					unsigned int fAttribute = g_ColorAttributeArr[nIndex].fAttribute;
					::EnableWindow( ::GetDlgItem( hwndDlg, IDC_CHECK_DISP ),			(0 == (fAttribute & COLOR_ATTRIB_FORCE_DISP))? TRUE: FALSE );
					::EnableWindow( ::GetDlgItem( hwndDlg, IDC_CHECK_FAT ),				(0 == (fAttribute & COLOR_ATTRIB_NO_BOLD))? TRUE: FALSE );
					::EnableWindow( ::GetDlgItem( hwndDlg, IDC_CHECK_UNDERLINE ),		(0 == (fAttribute & COLOR_ATTRIB_NO_UNDERLINE))? TRUE: FALSE );
					::EnableWindow( ::GetDlgItem( hwndDlg, IDC_BUTTON_SAMETEXTCOLOR ),	TRUE );
					::EnableWindow( ::GetDlgItem( hwndDlg, IDC_STATIC_HAIKEI ),			(0 == (fAttribute & COLOR_ATTRIB_NO_BACK))? TRUE: FALSE );
					::EnableWindow( ::GetDlgItem( hwndDlg, IDC_BUTTON_BACKCOLOR ),		(0 == (fAttribute & COLOR_ATTRIB_NO_BACK))? TRUE: FALSE );
					::EnableWindow( ::GetDlgItem( hwndDlg, IDC_BUTTON_SAMEBKCOLOR ),	(0 == (fAttribute & COLOR_ATTRIB_NO_BACK))? TRUE: FALSE );

					//::ShowWindow( ::GetDlgItem( hwndDlg, IDC_CHECK_DISP ),				(0 == (fAttribute & COLOR_ATTRIB_FORCE_DISP))? SW_SHOW: SW_HIDE );
					//::ShowWindow( ::GetDlgItem( hwndDlg, IDC_CHECK_FAT ),				(0 == (fAttribute & COLOR_ATTRIB_NO_BOLD))? SW_SHOW: SW_HIDE );
					//::ShowWindow( ::GetDlgItem( hwndDlg, IDC_CHECK_UNDERLINE ),			(0 == (fAttribute & COLOR_ATTRIB_NO_UNDERLINE))? SW_SHOW: SW_HIDE );
					//::ShowWindow( ::GetDlgItem( hwndDlg, IDC_BUTTON_SAMETEXTCOLOR ),	TRUE );
					//::ShowWindow( ::GetDlgItem( hwndDlg, IDC_STATIC_HAIKEI ),			(0 == (fAttribute & COLOR_ATTRIB_NO_BACK))? SW_SHOW: SW_HIDE );
					//::ShowWindow( ::GetDlgItem( hwndDlg, IDC_BUTTON_BACKCOLOR ),		(0 == (fAttribute & COLOR_ATTRIB_NO_BACK))? SW_SHOW: SW_HIDE );
					//::ShowWindow( ::GetDlgItem( hwndDlg, IDC_BUTTON_SAMEBKCOLOR ),		(0 == (fAttribute & COLOR_ATTRIB_NO_BACK))? SW_SHOW: SW_HIDE );
				}

				/* 色分け/表示 をする */
				if( m_Types.m_ColorInfoArr[m_nCurrentColorType].m_bDisp ){
					::CheckDlgButton( hwndDlg, IDC_CHECK_DISP, TRUE );
				}else{
					::CheckDlgButton( hwndDlg, IDC_CHECK_DISP, FALSE );
				}
				/* 太字で表示 */
				if( m_Types.m_ColorInfoArr[m_nCurrentColorType].m_bFatFont ){
					::CheckDlgButton( hwndDlg, IDC_CHECK_FAT, TRUE );
				}else{
					::CheckDlgButton( hwndDlg, IDC_CHECK_FAT, FALSE );
				}
				/* 下線を表示 */
				if( m_Types.m_ColorInfoArr[m_nCurrentColorType].m_bUnderLine ){
					::CheckDlgButton( hwndDlg, IDC_CHECK_UNDERLINE, TRUE );
				}else{
					::CheckDlgButton( hwndDlg, IDC_CHECK_UNDERLINE, FALSE );
				}


				::InvalidateRect( ::GetDlgItem( hwndDlg, IDC_BUTTON_TEXTCOLOR ), NULL, TRUE );
				::InvalidateRect( ::GetDlgItem( hwndDlg, IDC_BUTTON_BACKCOLOR ), NULL, TRUE );
				return TRUE;
			}
		}
		switch( wNotifyCode ){
		/* ボタン／チェックボックスがクリックされた */
		case BN_CLICKED:
			switch( wID ){
			case IDC_BUTTON_SAMETEXTCOLOR: /* 文字色統一 */
				{
					// 2006.04.26 ryoji 文字色／背景色統一ダイアログを使う
					CDlgSameColor cDlgSameColor;
					COLORREF cr = m_Types.m_ColorInfoArr[m_nCurrentColorType].m_colTEXT;
					cDlgSameColor.DoModal( ::GetModuleHandle(NULL), hwndDlg, wID, &m_Types, cr );
				}
				::InvalidateRect( hwndListColor, NULL, TRUE );
				return TRUE;

			case IDC_BUTTON_SAMEBKCOLOR:	/* 背景色統一 */
				{
					// 2006.04.26 ryoji 文字色／背景色統一ダイアログを使う
					CDlgSameColor cDlgSameColor;
					COLORREF cr = m_Types.m_ColorInfoArr[m_nCurrentColorType].m_colBACK;
					cDlgSameColor.DoModal( ::GetModuleHandle(NULL), hwndDlg, wID, &m_Types, cr );
				}
				::InvalidateRect( hwndListColor, NULL, TRUE );
				return TRUE;

			case IDC_BUTTON_TEXTCOLOR:	/* テキスト色 */
				/* 色選択ダイアログ */
				if( SelectColor( hwndDlg, &m_Types.m_ColorInfoArr[m_nCurrentColorType].m_colTEXT, m_dwCustColors ) ){
					::InvalidateRect( ::GetDlgItem( hwndDlg, IDC_BUTTON_TEXTCOLOR ), NULL, TRUE );
				}
				/* 現在選択されている色タイプ */
				::SendMessageAny( hwndListColor, LB_SETCURSEL, m_nCurrentColorType, 0 );
				return TRUE;
			case IDC_BUTTON_BACKCOLOR:	/* 背景色 */
				/* 色選択ダイアログ */
				if( SelectColor( hwndDlg, &m_Types.m_ColorInfoArr[m_nCurrentColorType].m_colBACK, m_dwCustColors ) ){
					::InvalidateRect( ::GetDlgItem( hwndDlg, IDC_BUTTON_BACKCOLOR ), NULL, TRUE );
				}
				/* 現在選択されている色タイプ */
				::SendMessageAny( hwndListColor, LB_SETCURSEL, m_nCurrentColorType, 0 );
				return TRUE;
			case IDC_CHECK_DISP:	/* 色分け/表示 をする */
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_DISP ) ){
					m_Types.m_ColorInfoArr[m_nCurrentColorType].m_bDisp = TRUE;
				}else{
					m_Types.m_ColorInfoArr[m_nCurrentColorType].m_bDisp = FALSE;
				}
				/* 現在選択されている色タイプ */
				::SendMessageAny( hwndListColor, LB_SETCURSEL, m_nCurrentColorType, 0 );
				m_Types.m_nRegexKeyMagicNumber++;	//Need Compile	//@@@ 2001.11.17 add MIK 正規表現キーワードのため
				return TRUE;
			case IDC_CHECK_FAT:	/* 太字か */
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_FAT ) ){
					m_Types.m_ColorInfoArr[m_nCurrentColorType].m_bFatFont = TRUE;
				}else{
					m_Types.m_ColorInfoArr[m_nCurrentColorType].m_bFatFont = FALSE;
				}
				/* 現在選択されている色タイプ */
				::SendMessageAny( hwndListColor, LB_SETCURSEL, m_nCurrentColorType, 0 );
				return TRUE;
			case IDC_CHECK_UNDERLINE:	/* 下線を表示 */
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_UNDERLINE ) ){
					m_Types.m_ColorInfoArr[m_nCurrentColorType].m_bUnderLine = TRUE;
				}else{
					m_Types.m_ColorInfoArr[m_nCurrentColorType].m_bUnderLine = FALSE;
				}
				/* 現在選択されている色タイプ */
				::SendMessageAny( hwndListColor, LB_SETCURSEL, m_nCurrentColorType, 0 );
				return TRUE;

			case IDC_BUTTON_IMPORT:	/* 色の設定をインポート */
				p3_Import_Colors( hwndDlg );
				m_Types.m_nRegexKeyMagicNumber++;	//Need Compile	//@@@ 2001.11.17 add MIK 正規表現キーワードのため
				return TRUE;

			case IDC_BUTTON_EXPORT:	/* 色の設定をエクスポート */
				p3_Export_Colors( hwndDlg );
				return TRUE;

			//	From Here Sept. 10, 2000 JEPRO
			//	行番号区切りを任意の半角文字にするときだけ指定文字入力をEnableに設定
			case IDC_RADIO_LINETERMTYPE0: /* 行番号区切り 0=なし 1=縦線 2=任意 */
			case IDC_RADIO_LINETERMTYPE1:
			case IDC_RADIO_LINETERMTYPE2:
			//	From Here Jun. 6, 2001 genta
			//	行コメント開始桁指定のON/OFF
			case IDC_CHECK_LCPOS:
			case IDC_CHECK_LCPOS2:
			case IDC_CHECK_LCPOS3:
			//	To Here Jun. 6, 2001 genta
				EnableTypesPropInput( hwndDlg );
				return TRUE;
			//	To Here Sept. 10, 2000

			//強調キーワードの選択
			case IDC_BUTTON_KEYWORD_SELECT:
				{
					CDlgKeywordSelect cDlgKeywordSelect;
					//強調キーワード1を取得する。
					HWND hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_SET );
					int nIdx = ::SendMessageAny( hwndCombo, CB_GETCURSEL, 0, 0 );
					if( CB_ERR == nIdx || 0 == nIdx ){
						m_nSet[ 0 ] = -1;
					}else{
						m_nSet[ 0 ] = nIdx - 1;
					}
					cDlgKeywordSelect.DoModal( ::GetModuleHandle(NULL), hwndDlg, m_nSet );
					RearrangeKeywordSet( hwndDlg );	//	Jan. 23, 2005 genta キーワードセット再配置
					//強調キーワード1を反映する。
					if( -1 == m_nSet[ 0 ] ){
						::SendMessageAny( hwndCombo, CB_SETCURSEL, (WPARAM)0, 0 );
					}else{
						::SendMessageAny( hwndCombo, CB_SETCURSEL, (WPARAM)m_nSet[ 0 ] + 1, 0 );
					}
				}
				break;
			}
			break;	/* BN_CLICKED */
		}
		break;	/* WM_COMMAND */
	case WM_NOTIFY:
		idCtrl = (int)wParam;
		pNMHDR = (NMHDR*)lParam;
		pMNUD  = (NM_UPDOWN*)lParam;
		switch( idCtrl ){
		//	From Here May 21, 2001 genta activate spin control
		case IDC_SPIN_LCColNum:
			/* 行コメント桁位置 */
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_LINECOMMENTPOS, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 1 ){
				nVal = 1;
			}
			if( nVal > 1000 ){
				nVal = 1000;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_LINECOMMENTPOS, nVal, FALSE );
			return TRUE;
		case IDC_SPIN_LCColNum2:
			/* 行コメント桁位置 */
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_LINECOMMENTPOS2, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 1 ){
				nVal = 1;
			}
			if( nVal > 1000 ){
				nVal = 1000;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_LINECOMMENTPOS2, nVal, FALSE );
			return TRUE;
		//	To Here May 21, 2001 genta activate spin control

		//	From Here Jun. 01, 2001 JEPRO 3つ目を追加
		case IDC_SPIN_LCColNum3:
			/* 行コメント桁位置 */
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_LINECOMMENTPOS3, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 1 ){
				nVal = 1;
			}
			if( nVal > 1000 ){
				nVal = 1000;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_LINECOMMENTPOS3, nVal, FALSE );
			return TRUE;
		//	To Here Jun. 01, 2001
		default:
			switch( pNMHDR->code ){
			case PSN_HELP:
//	Sept. 10, 2000 JEPRO ID名を実際の名前に変更するため以下の行はコメントアウト
//				OnHelp( hwndDlg, IDD_PROP1P3 );
				OnHelp( hwndDlg, IDD_PROP_COLOR );
				return TRUE;
			case PSN_KILLACTIVE:
//				MYTRACE_A( "p3 PSN_KILLACTIVE\n" );
				/* ダイアログデータの取得 p3 */
				GetData_p3_new( hwndDlg );
				return TRUE;
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
			case PSN_SETACTIVE:
				m_nPageNum = 1;
				return TRUE;
			}
			break;	/* default */
		}
		break;	/* WM_NOTIFY */
	case WM_DRAWITEM:
		idCtrl = (UINT) wParam;				/* コントロールのID */
		pDis = (LPDRAWITEMSTRUCT) lParam;	/* 項目描画情報 */
		switch( idCtrl ){

		case IDC_BUTTON_TEXTCOLOR:	/* テキスト色 */
			DrawColorButton( pDis, m_Types.m_ColorInfoArr[m_nCurrentColorType].m_colTEXT );
			return TRUE;
		case IDC_BUTTON_BACKCOLOR:	/* 背景色 */
			DrawColorButton( pDis, m_Types.m_ColorInfoArr[m_nCurrentColorType].m_colBACK );
			return TRUE;
		case IDC_LIST_COLORS:		/* 色種別リスト */
			DrawColorListItem( pDis );
			return TRUE;
		}
		break;

//@@@ 2001.02.04 Start by MIK: Popup Help
	case WM_HELP:
		{
			HELPINFO *p = (HELPINFO *)lParam;
			MyWinHelp( (HWND)p->hItemHandle, m_szHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)p_helpids2 );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		}
		return TRUE;
		/*NOTREACHED*/
//		break;
//@@@ 2001.02.04 End

//@@@ 2001.11.17 add start MIK
	//Context Menu
	case WM_CONTEXTMENU:
		MyWinHelp( hwndDlg, m_szHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)p_helpids2 );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		return TRUE;
//@@@ 2001.11.17 add end MIK

	}
	return FALSE;
}





/* ダイアログデータの設定 p3 */
void CPropTypes::SetData_p3_new( HWND hwndDlg )
{

	HWND	hwndWork;
	int		i;
	int		nItem;

	m_nCurrentColorType = 0;	/* 現在選択されている色タイプ */

	/* ユーザーがエディット コントロールに入力できるテキストの長さを制限する */	//@@@ 2002.09.22 YAZAKI
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_LINECOMMENT )		, EM_LIMITTEXT, (WPARAM)( COMMENT_DELIMITER_BUFFERSIZE - 1 ), 0 );
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_LINECOMMENT2 )		, EM_LIMITTEXT, (WPARAM)( COMMENT_DELIMITER_BUFFERSIZE - 1 ), 0 );
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_LINECOMMENT3 )		, EM_LIMITTEXT, (WPARAM)( COMMENT_DELIMITER_BUFFERSIZE - 1 ), 0 );	//Jun. 01, 2001 JEPRO 追加
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_BLOCKCOMMENT_FROM )	, EM_LIMITTEXT, (WPARAM)( BLOCKCOMMENT_BUFFERSIZE - 1 ), 0 );
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_BLOCKCOMMENT_TO )	, EM_LIMITTEXT, (WPARAM)( BLOCKCOMMENT_BUFFERSIZE - 1 ), 0 );
//#ifdef COMPILE_BLOCK_COMMENT2	//@@@ 2001.03.10 by MIK
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_BLOCKCOMMENT_FROM2 ), EM_LIMITTEXT, (WPARAM)( BLOCKCOMMENT_BUFFERSIZE - 1 ), 0 );
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_BLOCKCOMMENT_TO2 )	, EM_LIMITTEXT, (WPARAM)( BLOCKCOMMENT_BUFFERSIZE - 1 ), 0 );
//#endif

	::DlgItem_SetText( hwndDlg, IDC_EDIT_BLOCKCOMMENT_FROM	, m_Types.m_cBlockComment.getBlockCommentFrom(0) );	/* ブロックコメントデリミタ(From) */
	::DlgItem_SetText( hwndDlg, IDC_EDIT_BLOCKCOMMENT_TO		, m_Types.m_cBlockComment.getBlockCommentTo(0) );	/* ブロックコメントデリミタ(To) */
//#ifdef COMPILE_BLOCK_COMMENT2	//@@@ 2001.03.10 by MIK
	::DlgItem_SetText( hwndDlg, IDC_EDIT_BLOCKCOMMENT_FROM2	, m_Types.m_cBlockComment.getBlockCommentFrom(1) );	/* ブロックコメントデリミタ2(From) */
	::DlgItem_SetText( hwndDlg, IDC_EDIT_BLOCKCOMMENT_TO2	, m_Types.m_cBlockComment.getBlockCommentTo(1) );	/* ブロックコメントデリミタ2(To) */
//#endif

	/* 行コメントデリミタ @@@ 2002.09.22 YAZAKI*/
	//	From Here May 12, 2001 genta
	//	行コメントの開始桁位置設定
	//	May 21, 2001 genta 桁位置を1から数えるように
	for ( i=0; i<COMMENT_DELIMITER_NUM; i++ ){
		//	テキスト
		::DlgItem_SetText( hwndDlg, cLineComment[i].nEditID, m_Types.m_cLineComment.getLineComment(i) );	

		//	桁数チェックと、数値
		int nPos = m_Types.m_cLineComment.getLineCommentPos(i);
		if( nPos >= 0 ){
			::CheckDlgButton( hwndDlg, cLineComment[i].nCheckBoxID, TRUE );
			::SetDlgItemInt( hwndDlg, cLineComment[i].nTextID, nPos + 1, FALSE );
		}
		else {
			::CheckDlgButton( hwndDlg, cLineComment[i].nCheckBoxID, FALSE );
			::SetDlgItemInt( hwndDlg, cLineComment[i].nTextID, (~nPos) + 1, FALSE );
		}
	}

	if( 0 == m_Types.m_nStringType ){	/* 文字列区切り記号エスケープ方法  0=[\"][\'] 1=[""][''] */
		::CheckDlgButton( hwndDlg, IDC_RADIO_ESCAPETYPE_1, TRUE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_ESCAPETYPE_2, FALSE );
	}else{
		::CheckDlgButton( hwndDlg, IDC_RADIO_ESCAPETYPE_1, FALSE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_ESCAPETYPE_2, TRUE );
	}

	/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
	if( !m_Types.m_bLineNumIsCRLF ){
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINENUM_LAYOUT, TRUE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINENUM_CRLF, FALSE );
	}else{
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINENUM_LAYOUT, FALSE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINENUM_CRLF, TRUE );
	}


	/* セット名コンボボックスの値セット */
	hwndWork = ::GetDlgItem( hwndDlg, IDC_COMBO_SET );
	::SendMessageAny( hwndWork, CB_RESETCONTENT, 0, 0 );  /* コンボボックスを空にする */
	/* 一行目は空白 */
	Combo_AddString( hwndWork, L" " );
	//	Mar. 31, 2003 genta KeyWordSetMgrをポインタに
	if( 0 < m_pCKeyWordSetMgr->m_nKeyWordSetNum ){
		for( i = 0; i < m_pCKeyWordSetMgr->m_nKeyWordSetNum; ++i ){
			Combo_AddString( hwndWork, m_pCKeyWordSetMgr->GetTypeName( i ) );
		}
		if( -1 == m_Types.m_nKeyWordSetIdx[0] ){
			/* セット名コンボボックスのデフォルト選択 */
			::SendMessageAny( hwndWork, CB_SETCURSEL, (WPARAM)0, 0 );
		}else{
			/* セット名コンボボックスのデフォルト選択 */
			::SendMessageAny( hwndWork, CB_SETCURSEL, (WPARAM)m_Types.m_nKeyWordSetIdx[0] + 1, 0 );
		}
	}

	//強調キーワード1〜10の設定
	for( i = 0; i < MAX_KEYWORDSET_PER_TYPE; i++ ){
		m_nSet[ i ] = m_Types.m_nKeyWordSetIdx[i];
	}

	/* 色をつける文字種類のリスト */
	hwndWork = ::GetDlgItem( hwndDlg, IDC_LIST_COLORS );
	::SendMessageAny( hwndWork, LB_RESETCONTENT, 0, 0 );  /* コンボボックスを空にする */
	for( i = 0; i < COLORIDX_LAST; ++i ){
		nItem = ::List_AddString( hwndWork, m_Types.m_ColorInfoArr[i].m_szName );
		::SendMessageAny( hwndWork, LB_SETITEMDATA, nItem, (LPARAM)&m_Types.m_ColorInfoArr[i] );
	}
	/* 現在選択されている色タイプ */
	::SendMessageAny( hwndWork, LB_SETCURSEL, m_nCurrentColorType, 0 );
	::SendMessageCmd( hwndDlg, WM_COMMAND, MAKELONG( IDC_LIST_COLORS, LBN_SELCHANGE ), (LPARAM)hwndWork );

	/* 行番号区切り  0=なし 1=縦線 2=任意 */
	if( 0 == m_Types.m_nLineTermType ){
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINETERMTYPE0, TRUE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINETERMTYPE1, FALSE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINETERMTYPE2, FALSE );
	}else
	if( 1 == m_Types.m_nLineTermType ){
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINETERMTYPE0, FALSE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINETERMTYPE1, TRUE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINETERMTYPE2, FALSE );
	}else
	if( 2 == m_Types.m_nLineTermType ){
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINETERMTYPE0, FALSE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINETERMTYPE1, FALSE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_LINETERMTYPE2, TRUE );
	}

	/* 行番号区切り文字 */
	wchar_t	szLineTermChar[2];
	auto_sprintf( szLineTermChar, L"%lc", m_Types.m_cLineTermChar );
	::DlgItem_SetText( hwndDlg, IDC_EDIT_LINETERMCHAR, szLineTermChar );

	//	From Here Sept. 10, 2000 JEPRO
	//	行番号区切りを任意の半角文字にするときだけ指定文字入力をEnableに設定
	EnableTypesPropInput( hwndDlg );
	//	To Here Sept. 10, 2000


	// from here 2005.11.30 Moca 指定位置縦線の設定
	WCHAR szVertLine[MAX_VERTLINES * 15] = L"";
	int offset = 0;
	for( i = 0; i < MAX_VERTLINES && m_Types.m_nVertLineIdx[i] != 0; i++ ){
		CLayoutInt nXCol = m_Types.m_nVertLineIdx[i];
		CLayoutInt nXColEnd = nXCol;
		CLayoutInt nXColAdd = CLayoutInt(1);
		if( nXCol < 0 ){
			if( i < MAX_VERTLINES - 2 ){
				nXCol = -nXCol;
				nXColEnd = m_Types.m_nVertLineIdx[++i];
				nXColAdd = m_Types.m_nVertLineIdx[++i];
				if( nXColEnd < nXCol || nXColAdd <= 0 ){
					continue;
				}
				if(offset){
					szVertLine[offset] = ',';
					szVertLine[offset+1] = '\0';
					offset += 1;
				}
				offset += auto_sprintf( &szVertLine[offset], L"%d(%d,%d)", nXColAdd, nXCol, nXColEnd );
			}
		}
		else{
			if(offset){
				szVertLine[offset] = ',';
				szVertLine[offset+1] = '\0';
				offset += 1;
			}
			offset += auto_sprintf( &szVertLine[offset], L"%d", nXCol );
		}
	}
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_VERTLINE ), EM_LIMITTEXT, (WPARAM)(MAX_VERTLINES * 15), 0 );
	::DlgItem_SetText( hwndDlg, IDC_EDIT_VERTLINE, szVertLine );
	// to here 2005.11.30 Moca 指定位置縦線の設定
	return;
}





/* ダイアログデータの取得 p3 */
int CPropTypes::GetData_p3_new( HWND hwndDlg )
{
	int		nIdx;
	HWND	hwndWork;

//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
//	m_nPageNum = 1;

	//	From Here May 12, 2001 genta
	//	コメントの開始桁位置の取得
	//	May 21, 2001 genta 桁位置を1から数えるように
	wchar_t buffer[COMMENT_DELIMITER_BUFFERSIZE];	//@@@ 2002.09.22 YAZAKI LineCommentを取得するためのバッファ
	int pos;
	UINT en;
	BOOL bTranslated;

	int i;
	for( i=0; i<COMMENT_DELIMITER_NUM; i++ ){
		en = ::IsDlgButtonChecked( hwndDlg, cLineComment[i].nCheckBoxID );
		pos = ::GetDlgItemInt( hwndDlg, cLineComment[i].nTextID, &bTranslated, FALSE );
		if( bTranslated != TRUE ){
			en = 0;
			pos = 0;
		}
		//	pos == 0のときは無効扱い
		if( pos == 0 )	en = 0;
		else			--pos;
		//	無効のときは1の補数で格納

		::DlgItem_GetText( hwndDlg, cLineComment[i].nEditID		, buffer	, COMMENT_DELIMITER_BUFFERSIZE );		/* 行コメントデリミタ */
		m_Types.m_cLineComment.CopyTo( i, buffer, en ? pos : ~pos );
	}

	wchar_t szFromBuffer[BLOCKCOMMENT_BUFFERSIZE];	//@@@ 2002.09.22 YAZAKI
	wchar_t szToBuffer[BLOCKCOMMENT_BUFFERSIZE];	//@@@ 2002.09.22 YAZAKI

	::DlgItem_GetText( hwndDlg, IDC_EDIT_BLOCKCOMMENT_FROM	, szFromBuffer	, BLOCKCOMMENT_BUFFERSIZE );	/* ブロックコメントデリミタ(From) */
	::DlgItem_GetText( hwndDlg, IDC_EDIT_BLOCKCOMMENT_TO		, szToBuffer	, BLOCKCOMMENT_BUFFERSIZE );	/* ブロックコメントデリミタ(To) */
	m_Types.m_cBlockComment.CopyTo( 0, szFromBuffer, szToBuffer );

	::DlgItem_GetText( hwndDlg, IDC_EDIT_BLOCKCOMMENT_FROM2	, szFromBuffer	, BLOCKCOMMENT_BUFFERSIZE );	/* ブロックコメントデリミタ(From) */
	::DlgItem_GetText( hwndDlg, IDC_EDIT_BLOCKCOMMENT_TO2	, szToBuffer	, BLOCKCOMMENT_BUFFERSIZE );	/* ブロックコメントデリミタ(To) */
	m_Types.m_cBlockComment.CopyTo( 1, szFromBuffer, szToBuffer );

	/* 文字列区切り記号エスケープ方法  0=[\"][\'] 1=[""][''] */
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_ESCAPETYPE_1 ) ){
		m_Types.m_nStringType = 0;
	}else{
		m_Types.m_nStringType = 1;
	}
	/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_LINENUM_LAYOUT ) ){
		m_Types.m_bLineNumIsCRLF = FALSE;
	}else{
		m_Types.m_bLineNumIsCRLF = TRUE;
	}

	/* セット名コンボボックスの値セット */
	hwndWork = ::GetDlgItem( hwndDlg, IDC_COMBO_SET );
	nIdx = ::SendMessageAny( hwndWork, CB_GETCURSEL, 0, 0 );
	if( CB_ERR == nIdx ||
		0 == nIdx ){
		m_Types.m_nKeyWordSetIdx[0] = -1;
	}else{
		m_Types.m_nKeyWordSetIdx[0] = nIdx - 1;

	}

	//強調キーワード2〜10の取得(1は別)
	for( nIdx = 1; nIdx < MAX_KEYWORDSET_PER_TYPE; nIdx++ ){
		m_Types.m_nKeyWordSetIdx[nIdx] = m_nSet[nIdx];
	}

	/* 行番号区切り  0=なし 1=縦線 2=任意 */
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_LINETERMTYPE0 ) ){
		m_Types.m_nLineTermType = 0;
	}else
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_LINETERMTYPE1 ) ){
		m_Types.m_nLineTermType = 1;
	}else
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_LINETERMTYPE2 ) ){
		m_Types.m_nLineTermType = 2;
	}


	/* 行番号区切り文字 */
	wchar_t	szLineTermChar[2];
	::DlgItem_GetText( hwndDlg, IDC_EDIT_LINETERMCHAR, szLineTermChar, 2 );
	m_Types.m_cLineTermChar = szLineTermChar[0];


	// from here 2005.11.30 Moca 指定位置縦線の設定
	WCHAR szVertLine[MAX_VERTLINES * 15];
	::DlgItem_GetText( hwndDlg, IDC_EDIT_VERTLINE, szVertLine, MAX_VERTLINES * 15 );

	int offset = 0;
	i = 0;
	while( i < MAX_VERTLINES ){
		int value = 0;
		for(; '0' <= szVertLine[offset] && szVertLine[offset] <= '9';  offset++){
			value = szVertLine[offset] - '0' + value * 10;
		}
		if( value <= 0 ){
			break;
		}
		if( szVertLine[offset] == '(' ){
			offset++;
			int valueBegin = 0;
			int valueEnd = 0;
			for(; '0' <= szVertLine[offset] && szVertLine[offset] <= '9';  offset++){
				valueBegin = szVertLine[offset] - '0' + valueBegin * 10;
			}
			if( valueBegin <= 0 ){
				break;
			}
			if( szVertLine[offset] == ',' ){
				offset++;
			}else if( szVertLine[offset] != ')' ){
				break;
			}
			for(; '0' <= szVertLine[offset] && szVertLine[offset] <= '9';  offset++){
				valueEnd = szVertLine[offset] - '0' + valueEnd * 10;
			}
			if( valueEnd <= 0 ){
				valueEnd = MAXLINEKETAS;
			}
			if( szVertLine[offset] != ')' ){
				break;
			}
			offset++;
			if(i + 2 < MAX_VERTLINES){
				m_Types.m_nVertLineIdx[i++] = CLayoutInt(-valueBegin);
				m_Types.m_nVertLineIdx[i++] = CLayoutInt(valueEnd);
				m_Types.m_nVertLineIdx[i++] = CLayoutInt(value);
			}
			else{
				break;
			}
		}
		else{
			m_Types.m_nVertLineIdx[i++] = CLayoutInt(value);
		}
		if( szVertLine[offset] != ',' ){
			break;
		}
		offset++;
	}
	if( i < MAX_VERTLINES ){
		m_Types.m_nVertLineIdx[i] = CLayoutInt(0);
	}
	// to here 2005.11.30 Moca 指定位置縦線の設定
	return TRUE;
}





/* 色種別リスト オーナー描画 */
void CPropTypes::DrawColorListItem( DRAWITEMSTRUCT* pDis )
{
	HBRUSH		hBrush;
	HBRUSH		hBrushOld;
	HPEN		hPen;
	HPEN		hPenOld;
	ColorInfo*	pColorInfo;
//	RECT		rc0,rc1,rc2;
	RECT		rc1;
	COLORREF	cRim = (COLORREF)::GetSysColor( COLOR_3DSHADOW );

	if( pDis == NULL || pDis->itemData == NULL ) return;

//	rc0 = pDis->rcItem;
	rc1 = pDis->rcItem;
//	rc2 = pDis->rcItem;

	/* アイテムデータの取得 */
	pColorInfo = (ColorInfo*)pDis->itemData;

	/* アイテム矩形塗りつぶし */
	hBrush = ::CreateSolidBrush( ::GetSysColor( COLOR_WINDOW ) );
	::FillRect( pDis->hDC, &pDis->rcItem, hBrush );
	::DeleteObject( hBrush );


	/* アイテムが選択されている */
	if( pDis->itemState & ODS_SELECTED ){
		hBrush = ::CreateSolidBrush( ::GetSysColor( COLOR_HIGHLIGHT ) );
		::SetTextColor( pDis->hDC, ::GetSysColor( COLOR_HIGHLIGHTTEXT ) );
	}else{
		hBrush = ::CreateSolidBrush( ::GetSysColor( COLOR_WINDOW ) );
		::SetTextColor( pDis->hDC, ::GetSysColor( COLOR_WINDOWTEXT ) );
	}

//	/* 選択ハイライト矩形 */
//	::FillRect( pDis->hDC, &rc1, hBrush );
//	::DeleteObject( hBrush );
//	/* テキスト */
//	::SetBkMode( pDis->hDC, TRANSPARENT );
//	::TextOutW_AnyBuild( pDis->hDC, rc1.left, rc1.top, pColorInfo->m_szName, wcslen( pColorInfo->m_szName ) );
//	if( pColorInfo->m_bFatFont ){	/* 太字か */
//		::TextOutW_AnyBuild( pDis->hDC, rc1.left + 1, rc1.top, pColorInfo->m_szName, wcslen( pColorInfo->m_szName ) );
//	}
//	return;


	rc1.left+= (2 + 16);
	rc1.top += 2;
	rc1.right -= ( 2 + 27 );
	rc1.bottom -= 2;
	/* 選択ハイライト矩形 */
	::FillRect( pDis->hDC, &rc1, hBrush );
	::DeleteObject( hBrush );
	/* テキスト */
	::SetBkMode( pDis->hDC, TRANSPARENT );
	::TextOut( pDis->hDC, rc1.left, rc1.top, pColorInfo->m_szName, _tcslen( pColorInfo->m_szName ) );
	if( pColorInfo->m_bFatFont ){	/* 太字か */
		::TextOut( pDis->hDC, rc1.left + 1, rc1.top, pColorInfo->m_szName, _tcslen( pColorInfo->m_szName ) );
	}
	if( pColorInfo->m_bUnderLine ){	/* 下線か */
		SIZE	sz;
		::GetTextExtentPoint32( pDis->hDC, pColorInfo->m_szName, _tcslen( pColorInfo->m_szName ), &sz );
		::MoveToEx( pDis->hDC, rc1.left,		rc1.bottom - 2, NULL );
		::LineTo( pDis->hDC, rc1.left + sz.cx,	rc1.bottom - 2 );
		::MoveToEx( pDis->hDC, rc1.left,		rc1.bottom - 1, NULL );
		::LineTo( pDis->hDC, rc1.left + sz.cx,	rc1.bottom - 1 );
	}

	/* アイテムにフォーカスがある */	// 2006.05.01 ryoji 描画条件の不正を修正
	if( pDis->itemState & ODS_FOCUS ){
		::DrawFocusRect( pDis->hDC, &pDis->rcItem );
	}

	/* 「色分け/表示する」のチェック */
	rc1 = pDis->rcItem;
	rc1.left += 2;
	rc1.top += 3;
	rc1.right = rc1.left + 12;
	rc1.bottom = rc1.top + 12;
	if( pColorInfo->m_bDisp ){	/* 色分け/表示する */
		// 2006.04.26 ryoji テキスト色を使う（「ハイコントラスト黒」のような設定でも見えるように）
		hPen = ::CreatePen( PS_SOLID, 1, ::GetSysColor( COLOR_WINDOWTEXT ) );
		hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );

		::MoveToEx( pDis->hDC,	rc1.left + 2, rc1.top + 6, NULL );
		::LineTo( pDis->hDC,	rc1.left + 5, rc1.bottom - 3 );
		::LineTo( pDis->hDC,	rc1.right - 2, rc1.top + 4 );
		rc1.top -= 1;
		rc1.bottom -= 1;
		::MoveToEx( pDis->hDC,	rc1.left + 2, rc1.top + 6, NULL );
		::LineTo( pDis->hDC,	rc1.left + 5, rc1.bottom - 3 );
		::LineTo( pDis->hDC,	rc1.right - 2, rc1.top + 4 );
		rc1.top -= 1;
		rc1.bottom -= 1;
		::MoveToEx( pDis->hDC,	rc1.left + 2, rc1.top + 6, NULL );
		::LineTo( pDis->hDC,	rc1.left + 5, rc1.bottom - 3 );
		::LineTo( pDis->hDC,	rc1.right - 2, rc1.top + 4 );

		::SelectObject( pDis->hDC, hPenOld );
		::DeleteObject( hPen );
	}
//	return;


	// 2002/11/02 Moca 比較方法変更
//	if( 0 != strcmp( "カーソル行アンダーライン", pColorInfo->m_szName ) )
	if ( 0 == (g_ColorAttributeArr[pColorInfo->m_nColorIdx].fAttribute & COLOR_ATTRIB_NO_BACK) )	// 2006.12.18 ryoji フラグ利用で簡素化
	{
		/* 背景色 見本矩形 */
		rc1 = pDis->rcItem;
		rc1.left = rc1.right - 13;
		rc1.top += 2;
		rc1.right = rc1.left + 12;
		rc1.bottom -= 2;

		hBrush = ::CreateSolidBrush( pColorInfo->m_colBACK );
		hBrushOld = (HBRUSH)::SelectObject( pDis->hDC, hBrush );
		hPen = ::CreatePen( PS_SOLID, 1, cRim );
		hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
		::RoundRect( pDis->hDC, rc1.left, rc1.top, rc1.right, rc1.bottom , 3, 3 );
		::SelectObject( pDis->hDC, hPenOld );
		::SelectObject( pDis->hDC, hBrushOld );
		::DeleteObject( hPen );
		::DeleteObject( hBrush );
	}


	/* 前景色 見本矩形 */
	rc1 = pDis->rcItem;
	rc1.left = rc1.right - 27;
	rc1.top += 2;
	rc1.right = rc1.left + 12;
	rc1.bottom -= 2;
	hBrush = ::CreateSolidBrush( pColorInfo->m_colTEXT );
	hBrushOld = (HBRUSH)::SelectObject( pDis->hDC, hBrush );
	hPen = ::CreatePen( PS_SOLID, 1, cRim );
	hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
	::RoundRect( pDis->hDC, rc1.left, rc1.top, rc1.right, rc1.bottom , 3, 3 );
	::SelectObject( pDis->hDC, hPenOld );
	::SelectObject( pDis->hDC, hBrushOld );
	::DeleteObject( hPen );
	::DeleteObject( hBrush );





//	::SetTextColor( pDis->hDC, ::GetSysColor( COLOR_WINDOWTEXT ) );
//	::SetBkColor( pDis->hDC, ::GetSysColor( COLOR_WINDOW ) );
//	::TextOutW_AnyBuild( pDis->hDC, pDis->rcItem.left + 3, pDis->rcItem.top + 3, pColorInfo->m_szName, wcslen( pColorInfo->m_szName ) );

//	::SetTextColor( pDis->hDC, pColorInfo->m_colTEXT );
//	::SetBkColor( pDis->hDC, pColorInfo->m_colBACK );
//	::TextOutW_AnyBuild( pDis->hDC, pDis->rcItem.left + 3 + 128, pDis->rcItem.top + 3, gpColorInfo->m_szName, wcslen( pColorInfo->m_szName ) );
	return;
}


/* ヘルプ */
//Stonee, 2001/05/18 機能番号からヘルプトピック番号を調べるようにした
void CPropTypes::OnHelp( HWND hwndParent, int nPageID )
{
	int		nContextID;
	switch( nPageID ){
	case IDD_PROPTYPESP1:
		nContextID = ::FuncID_To_HelpContextID(F_TYPE_SCREEN);
		break;
//	Sept. 10, 2000 JEPRO ID名を実際の名前に変更するため以下の行はコメントアウト
//	case IDD_PROP1P3:
	case IDD_PROP_COLOR:
		nContextID = ::FuncID_To_HelpContextID(F_TYPE_COLOR);
		break;
//	Jul. 03, 2001 JEPRO 支援タブのヘルプを有効化
		case IDD_PROPTYPESP2:
		nContextID = ::FuncID_To_HelpContextID(F_TYPE_HELPER);
		break;
//@@@ 2001.11.17 add start MIK
	case IDD_PROP_REGEX:
		nContextID = ::FuncID_To_HelpContextID(F_TYPE_REGEX_KEYWORD);
		break;
//@@@ 2001.11.17 add end MIK
	case IDD_PROP_KEYHELP:
		nContextID = ::FuncID_To_HelpContextID(F_TYPE_KEYHELP);
		break;
	default:
		nContextID = -1;
		break;
	}
	if( -1 != nContextID ){
		MyWinHelp( hwndParent, m_szHelpFile, HELP_CONTEXT, nContextID );	// 2006.10.10 ryoji MyWinHelpに変更に変更
	}
	return;
}





//	From Here Sept. 10, 2000 JEPRO
//	チェック状態に応じてダイアログボックス要素のEnable/Disableを
//	適切に設定する
void CPropTypes::EnableTypesPropInput( HWND hwndDlg )
{
	//	行番号区切りを任意の半角文字にするかどうか
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_LINETERMTYPE2 ) ){
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_LINETERMCHAR ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_LINETERMCHAR ), TRUE );
	}else{
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_LINETERMCHAR ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_LINETERMCHAR ), FALSE );
	}

	//	From Here Jun. 6, 2001 genta
	//	行コメント開始桁位置入力ボックスのEnable/Disable設定
	//	1つ目
	if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_LCPOS ) ){
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_LINECOMMENTPOS ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_LCPOS ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_LCColNum ), TRUE );
	}else{
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_LINECOMMENTPOS ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_LCPOS ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_LCColNum ), FALSE );
	}
	//	2つ目
	if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_LCPOS2 ) ){
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_LINECOMMENTPOS2 ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_LCPOS2 ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_LCColNum2 ), TRUE );
	}else{
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_LINECOMMENTPOS2 ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_LCPOS2 ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_LCColNum2 ), FALSE );
	}
	//	3つ目
	if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_LCPOS3 ) ){
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_LINECOMMENTPOS3 ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_LCPOS3 ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_LCColNum3 ), TRUE );
	}else{
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_LINECOMMENTPOS3 ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_LCPOS3 ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_LCColNum3 ), FALSE );
	}
	//	To Here Jun. 6, 2001 genta
}
//	To Here Sept. 10, 2000

/*!	@brief キーワードセットの再配列

	キーワードセットの色分けでは未指定のキーワードセット以降はチェックを省略する．
	そのためセットの途中に未指定のものがある場合はそれ以降を前に詰めることで
	指定された全てのキーワードセットが有効になるようにする．
	その際，色分けの設定も同時に移動する．

	m_nSet, m_Types.m_ColorInfoArr[]が変更される．

	@param hwndDlg [in] ダイアログボックスのウィンドウハンドル

	@author	genta 
	@date	2005.01.23 genta new

*/
void CPropTypes::RearrangeKeywordSet( HWND hwndDlg )
{
	int i, j;
	for( i = 0; i < MAX_KEYWORDSET_PER_TYPE; i++ ){
		if( m_nSet[ i ] != -1 )
			continue;

		//	未設定の場合
		for( j = i; j < MAX_KEYWORDSET_PER_TYPE; j++ ){
			if( m_nSet[ j ] != -1 ){
				//	後ろに設定済み項目があった場合
				m_nSet[ i ] = m_nSet[ j ];
				m_nSet[ j ] = -1;

				//	色設定を入れ替える
				//	構造体ごと入れ替えると名前が変わってしまうので注意
				ColorInfo colT;
				ColorInfo &col1 = m_Types.m_ColorInfoArr[ COLORIDX_KEYWORD1 + i ];
				ColorInfo &col2   = m_Types.m_ColorInfoArr[ COLORIDX_KEYWORD1 + j ];

				colT.m_bDisp		= col1.m_bDisp;
				colT.m_bFatFont		= col1.m_bFatFont;
				colT.m_bUnderLine	= col1.m_bUnderLine;
				colT.m_colTEXT		= col1.m_colTEXT;
				colT.m_colBACK		= col1.m_colBACK;

				col1.m_bDisp		= col2.m_bDisp;
				col1.m_bFatFont		= col2.m_bFatFont;
				col1.m_bUnderLine	= col2.m_bUnderLine;
				col1.m_colTEXT		= col2.m_colTEXT;
				col1.m_colBACK		= col2.m_colBACK;

				col2.m_bDisp		= colT.m_bDisp;
				col2.m_bFatFont		= colT.m_bFatFont;
				col2.m_bUnderLine	= colT.m_bUnderLine;
				col2.m_colTEXT		= colT.m_colTEXT;
				col2.m_colBACK		= colT.m_colBACK;
				
				break;
			}
		}
		if( j == MAX_KEYWORDSET_PER_TYPE ){
			//	後ろには設定済み項目がなかった
			break;
		}
	}
	
	//	リストボックス及び色設定ボタンを再描画
	::InvalidateRect( ::GetDlgItem( hwndDlg, IDC_BUTTON_TEXTCOLOR ), NULL, TRUE );
	::InvalidateRect( ::GetDlgItem( hwndDlg, IDC_BUTTON_BACKCOLOR ), NULL, TRUE );
	::InvalidateRect( ::GetDlgItem( hwndDlg, IDC_LIST_COLORS ), NULL, TRUE );
}


/*[EOF]*/
