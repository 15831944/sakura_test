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
	Copyright (C) 2006, ryoji, fon, novice
	Copyright (C) 2007, ryoji, genta
	Copyright (C) 2008, nasukoji
	Copyright (C) 2009, ryoji, genta

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "StdAfx.h"
#include <commctrl.h>
#include <assert.h>
#include "CPropTypes.h"
#include "CEditApp.h"
#include "Debug.h"
#include "CDlgOpenFile.h"
#include "CDlgKeywordSelect.h"
#include "etc_uty.h"
#include "shell.h"
#include "global.h"
#include "CProfile.h"
#include "CShareData.h"
#include "Funccode.h"	//Stonee, 2001/05/18
#include "CDlgSameColor.h"	// 2006.04.26 ryoji
#include "sakura_rc.h"
#include "sakura.hh"

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                      メッセージ処理                         //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/* Screen ダイアログプロシージャ */
INT_PTR CALLBACK PropTypesScreen(
	HWND	hwndDlg,	// handle to dialog box
	UINT	uMsg,		// message
	WPARAM	wParam,		// first message parameter
	LPARAM	lParam 		// second message parameter
)
{
	PROPSHEETPAGE*	pPsp;
	CPropTypesScreen* pCPropTypes;
	switch( uMsg ){
	case WM_INITDIALOG:
		pPsp = (PROPSHEETPAGE*)lParam;
		pCPropTypes = ( CPropTypesScreen* )(pPsp->lParam);
		if( NULL != pCPropTypes ){
			return (pCPropTypes->DispatchEvent)( hwndDlg, uMsg, wParam, pPsp->lParam );
		}else{
			return FALSE;
		}
	default:
		// Modified by KEITA for WIN64 2003.9.6
		pCPropTypes = ( CPropTypesScreen* )::GetWindowLongPtr( hwndDlg, DWLP_USER );
		if( NULL != pCPropTypes ){
			return (pCPropTypes->DispatchEvent)( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	}
}

/* color ダイアログプロシージャ */
INT_PTR CALLBACK PropTypesColor(
	HWND	hwndDlg,	// handle to dialog box
	UINT	uMsg,		// message
	WPARAM	wParam,		// first message parameter
	LPARAM	lParam 		// second message parameter
)
{
	PROPSHEETPAGE*	pPsp;
	CPropTypesColor*		pCPropTypes;
	switch( uMsg ){
	case WM_INITDIALOG:
		pPsp = (PROPSHEETPAGE*)lParam;
		pCPropTypes = ( CPropTypesColor* )(pPsp->lParam);
		if( NULL != pCPropTypes ){
			return (pCPropTypes->DispatchEvent)( hwndDlg, uMsg, wParam, pPsp->lParam );
		}else{
			return FALSE;
		}
	default:
		// Modified by KEITA for WIN64 2003.9.6
		pCPropTypes = ( CPropTypesColor* )::GetWindowLongPtr( hwndDlg, DWLP_USER );
		if( NULL != pCPropTypes ){
			return (pCPropTypes->DispatchEvent)( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	}
}

/* support ダイアログプロシージャ */
INT_PTR CALLBACK PropTypesSupport(
	HWND	hwndDlg,	// handle to dialog box
	UINT	uMsg,		// message
	WPARAM	wParam,		// first message parameter
	LPARAM	lParam 		// second message parameter
)
{
	PROPSHEETPAGE*	pPsp;
	CPropTypesSupport* pCPropTypes;
	switch( uMsg ){
	case WM_INITDIALOG:
		pPsp = (PROPSHEETPAGE*)lParam;
		pCPropTypes = ( CPropTypesSupport* )(pPsp->lParam);
		if( NULL != pCPropTypes ){
			return (pCPropTypes->DispatchEvent)( hwndDlg, uMsg, wParam, pPsp->lParam );
		}else{
			return FALSE;
		}
	default:
		// Modified by KEITA for WIN64 2003.9.6
		pCPropTypes = ( CPropTypesSupport* )::GetWindowLongPtr( hwndDlg, DWLP_USER );
		if( NULL != pCPropTypes ){
			return (pCPropTypes->DispatchEvent)( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	}
}

/* 正規表現キーワード ダイアログプロシージャ */
INT_PTR CALLBACK PropTypesRegex(
	HWND	hwndDlg,	// handle to dialog box
	UINT	uMsg,		// message
	WPARAM	wParam,		// first message parameter
	LPARAM	lParam 		// second message parameter
)
{
	PROPSHEETPAGE*	pPsp;
	CPropTypesRegex* pCPropTypes;
	switch( uMsg ){
	case WM_INITDIALOG:
		pPsp = (PROPSHEETPAGE*)lParam;
		pCPropTypes = ( CPropTypesRegex* )(pPsp->lParam);
		if( NULL != pCPropTypes ){
			return (pCPropTypes->DispatchEvent)( hwndDlg, uMsg, wParam, pPsp->lParam );
		}
		break;
	default:
		// Modified by KEITA for WIN64 2003.9.6
		pCPropTypes = ( CPropTypesRegex* )::GetWindowLongPtr( hwndDlg, DWLP_USER );
		if( NULL != pCPropTypes ){
			return (pCPropTypes->DispatchEvent)( hwndDlg, uMsg, wParam, lParam );
		}
		break;
	}
	return FALSE;
}

/*! キーワード辞書ファイル設定 ダイアログプロシージャ

	@date 2006.04.10 fon 新規作成
*/
INT_PTR CALLBACK PropTypesKeyHelp(
	HWND	hwndDlg,	// handle to dialog box
	UINT	uMsg,		// message
	WPARAM	wParam,		// first message parameter
	LPARAM	lParam 		// second message parameter
)
{
	PROPSHEETPAGE*	pPsp;
	CPropTypesKeyHelp* pCPropTypes;
	switch( uMsg ){
	case WM_INITDIALOG:
		pPsp = (PROPSHEETPAGE*)lParam;
		pCPropTypes = ( CPropTypesKeyHelp* )(pPsp->lParam);
		if( NULL != pCPropTypes ){
			return (pCPropTypes->DispatchEvent)( hwndDlg, uMsg, wParam, pPsp->lParam );
		}break;
	default:
		pCPropTypes = ( CPropTypesKeyHelp* )::GetWindowLongPtr( hwndDlg, DWLP_USER );
		if( NULL != pCPropTypes ){
			return (pCPropTypes->DispatchEvent)( hwndDlg, uMsg, wParam, lParam );
		}break;
	}
	return FALSE;
}

CPropTypes::CPropTypes()
{
	{
		assert( sizeof(CPropTypesScreen)  - sizeof(CPropTypes) == 0 );
		assert( sizeof(CPropTypesColor)   - sizeof(CPropTypes) == 0 );
		assert( sizeof(CPropTypesSupport) - sizeof(CPropTypes) == 0 );
		assert( sizeof(CPropTypesRegex)   - sizeof(CPropTypes) == 0 );
		assert( sizeof(CPropTypesKeyHelp) - sizeof(CPropTypes) == 0 );
	}

	/* 共有データ構造体のアドレスを返す */
	m_pShareData = CShareData::getInstance()->GetShareData();

	// Mar. 31, 2003 genta メモリ削減のためポインタに変更
	m_pCKeyWordSetMgr = &(m_pShareData->m_Common.m_sSpecialKeyword.m_CKeyWordSetMgr);

	m_hInstance = NULL;		/* アプリケーションインスタンスのハンドル */
	m_hwndParent = NULL;	/* オーナーウィンドウのハンドル */
	m_hwndThis  = NULL;		/* このダイアログのハンドル */
	m_nPageNum = ID_PROPTYPE_PAGENUM_SCREEN;
}

CPropTypes::~CPropTypes()
{
}

/* 初期化 */
void CPropTypes::Create( HINSTANCE hInstApp, HWND hwndParent )
{
	m_hInstance = hInstApp;		/* アプリケーションインスタンスのハンドル */
	m_hwndParent = hwndParent;	/* オーナーウィンドウのハンドル */
}

struct TypePropSheetInfo {
	const TCHAR* szTabname;									//!< TABの表示名
	unsigned int resId;										//!< Property sheetに対応するDialog resource
	INT_PTR (CALLBACK *DProc)(HWND, UINT, WPARAM, LPARAM);	//!< Dialog Procedure
};

// キーワード：タイプ別設定タブ順序(プロパティシート)
/* プロパティシートの作成 */
INT_PTR CPropTypes::DoPropertySheet( int nPageNum )
{
	INT_PTR				nRet;
	int					nIdx;

	// 2001/06/14 Start by asa-o: タイプ別設定に支援タブ追加
	// 2001.11.17 add start MIK タイプ別設定に正規表現キーワードタブ追加
	// 2006.04.10 fon ADD-start タイプ別設定に「キーワードヘルプ」タブを追加
	static const TypePropSheetInfo TypePropSheetInfoList[] = {
		{ _T("スクリーン"), 		IDD_PROP_SCREEN,	PropTypesScreen },
		{ _T("カラー"), 			IDD_PROP_COLOR,		PropTypesColor },
		{ _T("支援"), 				IDD_PROP_SUPPORT,	PropTypesSupport },
		{ _T("正規表現キーワード"),	IDD_PROP_REGEX,		PropTypesRegex },
		{ _T("キーワードヘルプ"),	IDD_PROP_KEYHELP,	PropTypesKeyHelp }
	};

	// カスタム色を共有メモリから取得
	memcpy( m_dwCustColors, m_pShareData->m_dwCustColors, sizeof(m_dwCustColors) );
	// 2005.11.30 Moca カスタム色の先頭にテキスト色を設定しておく
	m_dwCustColors[0] = m_Types.m_ColorInfoArr[COLORIDX_TEXT].m_sColorAttr.m_cTEXT;
	m_dwCustColors[1] = m_Types.m_ColorInfoArr[COLORIDX_TEXT].m_sColorAttr.m_cBACK;

	PROPSHEETPAGE		psp[_countof(TypePropSheetInfoList)];
	for( nIdx = 0; nIdx < _countof(TypePropSheetInfoList); nIdx++ ){
		assert( TypePropSheetInfoList[nIdx].szTabname != NULL );

		PROPSHEETPAGE *p = &psp[nIdx];
		memset( p, 0, sizeof( *p ) );
		p->dwSize      = sizeof( *p );
		p->dwFlags     = PSP_USETITLE | PSP_HASHELP;
		p->hInstance   = m_hInstance;
		p->pszTemplate = MAKEINTRESOURCE( TypePropSheetInfoList[nIdx].resId );
		p->pszIcon     = NULL;
		p->pfnDlgProc  = TypePropSheetInfoList[nIdx].DProc;
		p->pszTitle    = TypePropSheetInfoList[nIdx].szTabname;
		p->lParam      = (LPARAM)this;
		p->pfnCallback = NULL;
	}

	PROPSHEETHEADER	psh;
	memset( &psh, 0, sizeof( psh ) );
	
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
	}
	else if( 0 > nPageNum ){			//- 20020106 aroka
		psh.nStartPage = 0;
	}
	else{
		psh.nStartPage = nPageNum;
	}

	if( psh.nPages - 1 < psh.nStartPage ){
		psh.nStartPage = psh.nPages - 1;
	}
	psh.ppsp = psp;
	psh.pfnCallback = NULL;

	nRet = MyPropertySheet( &psh );	// 2007.05.24 ryoji 独自拡張プロパティシート

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
		PleaseReportToAuthor(
			NULL,
			_T("CPropTypes::DoPropertySheet()内でエラーが出ました。\n")
			_T("psh.nStartPage=[%d]\n")
			_T("::PropertySheet()失敗。\n")
			_T("\n")
			_T("%s\n"),
			psh.nStartPage,
			pszMsgBuf
		);
		::LocalFree( pszMsgBuf );
	}

	// カスタム色を共有メモリに設定
	memcpy( m_pShareData->m_dwCustColors, m_dwCustColors, sizeof(m_dwCustColors) );

	return nRet;
}



// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                         イベント                            //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/* ヘルプ */
//2001.05.18 Stonee 機能番号からヘルプトピック番号を調べるようにした
//2001.07.03 JEPRO  支援タブのヘルプを有効化
//2001.11.17 MIK    IDD_PROP_REGEX
void CPropTypes::OnHelp( HWND hwndParent, int nPageID )
{
	int		nContextID;
	switch( nPageID ){
	case IDD_PROP_SCREEN:	nContextID = ::FuncID_To_HelpContextID(F_TYPE_SCREEN);			break;
	case IDD_PROP_COLOR:	nContextID = ::FuncID_To_HelpContextID(F_TYPE_COLOR);			break;
	case IDD_PROP_SUPPORT:	nContextID = ::FuncID_To_HelpContextID(F_TYPE_HELPER);			break;
	case IDD_PROP_REGEX:	nContextID = ::FuncID_To_HelpContextID(F_TYPE_REGEX_KEYWORD);	break;
	case IDD_PROP_KEYHELP:	nContextID = ::FuncID_To_HelpContextID(F_TYPE_KEYHELP);			break;
	default:				nContextID = -1;												break;
	}
	if( -1 != nContextID ){
		MyWinHelp( hwndParent, HELP_CONTEXT, nContextID );	// 2006.10.10 ryoji MyWinHelpに変更に変更
	}
}

/*[EOF]*/
