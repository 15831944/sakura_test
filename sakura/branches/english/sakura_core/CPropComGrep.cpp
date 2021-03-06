/*!	@file
	@brief 共通設定ダイアログボックス、「Grep」ページ

	@author Norio Nakatani
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001, MIK, genta
	Copyright (C) 2002, YAZAKI, MIK
	Copyright (C) 2003, KEITA
	Copyright (C) 2006, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/
/* LMP (Lucien Murray-Pitts) : 2011-02-26 Added Basic English Translation Resources */

#include "stdafx.h"
#include "CPropCommon.h"
#include "CBregexp.h"	// 2007.08/12 genta バージョン取得

//@@@ 2001.02.04 Start by MIK: Popup Help
#if 1	//@@@ 2002.01.03 add MIK
#include "sakura.hh"
static const DWORD p_helpids[] = {	//10500
	IDC_EDIT_REGEXPLIB,				HIDC_EDIT_REGEXPLIB,	//正規表現ライブラリ選択	// 2007.09.02 genta
	IDC_LABEL_REGEXP,				HIDC_EDIT_REGEXPLIB,
	IDC_LABEL_REGEXP_VER,			HIDC_LABEL_REGEXPVER,	//正規表現ライブラリバージョン	// 2007.09.02 genta
	IDC_CHECK_bCaretTextForSearch,	HIDC_CHECK_bCaretTextForSearch,	//カーソル位置の文字列をデフォルトの検索文字列にする	// 2006.08.23 ryoji
	IDC_CHECK_bGrepExitConfirm,		HIDC_CHECK_bGrepExitConfirm,	//GREPの保存確認
	IDC_CHECK_GTJW_RETURN,			HIDC_CHECK_GTJW_RETURN,			//タグジャンプ（エンターキー）
	IDC_CHECK_GTJW_LDBLCLK,			HIDC_CHECK_GTJW_LDBLCLK,		//タグジャンプ（ダブルクリック）
	IDC_CHECK_GREPREALTIME,			HIDC_CHECK_GREPREALTIME,		//リアルタイムで表示する	// 2006.08.08 ryoji
//	IDC_STATIC,						-1,
	0, 0
};
#else
static const DWORD p_helpids[] = {	//10500
	IDC_CHECK_bGrepExitConfirm,		10510,	//GREPの保存確認
	IDC_CHECK_GTJW_RETURN,			10511,	//タグジャンプ（エンターキー）
	IDC_CHECK_GTJW_LDBLCLK,			10512,	//タグジャンプ（ダブルクリック）
//	IDC_STATIC,						-1,
	0, 0
};
#endif
//@@@ 2001.02.04 End

//	From Here Jun. 2, 2001 genta
/*!
	@param hwndDlg ダイアログボックスのWindow Handle
	@param uMsg メッセージ
	@param wParam パラメータ1
	@param lParam パラメータ2
*/
INT_PTR CALLBACK CPropCommon::DlgProc_PROP_GREP(
	HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return DlgProc( &CPropCommon::DispatchEvent_PROP_GREP, hwndDlg, uMsg, wParam, lParam );
}
//	To Here Jun. 2, 2001 genta

/* メッセージ処理 */
INT_PTR CPropCommon::DispatchEvent_PROP_GREP( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
//	WORD		wNotifyCode;
//	WORD		wID;
//	HWND		hwndCtl;
	NMHDR*		pNMHDR;
	NM_UPDOWN*	pMNUD;
	int			idCtrl;
//	int			nVal;
//    LPDRAWITEMSTRUCT pDis;

	switch( uMsg ){

	case WM_INITDIALOG:
		/* ダイアログデータの設定 p1 */
		SetData_PROP_GREP( hwndDlg );
		// Modified by KEITA for WIN64 2003.9.6
		::SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

		/* ユーザーがエディット コントロールに入力できるテキストの長さを制限する */

		return TRUE;
	case WM_NOTIFY:
		idCtrl = (int)wParam;
		pNMHDR = (NMHDR*)lParam;
		pMNUD  = (NM_UPDOWN*)lParam;
//		switch( idCtrl ){
//		default:
			switch( pNMHDR->code ){
			case PSN_HELP:
				OnHelp( hwndDlg, IDD_PROP_GREP );
				return TRUE;
			case PSN_KILLACTIVE:
				/* ダイアログデータの取得 p1 */
				GetData_PROP_GREP( hwndDlg );
				return TRUE;
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
			case PSN_SETACTIVE:
				m_nPageNum = ID_PAGENUM_GREP;
				return TRUE;
			}
//			break;	/* default */
//		}
		break;	/* WM_NOTIFY */
	case WM_COMMAND:
		//	2007.08.12 genta 正規表現DLLの変更に応じてVersionを再取得する
		if( wParam == MAKEWPARAM( IDC_EDIT_REGEXPLIB, EN_KILLFOCUS )){
			SetRegexpVersion( hwndDlg );
		}
		break;

//@@@ 2001.02.04 Start by MIK: Popup Help
	case WM_HELP:
		{
			HELPINFO *p = (HELPINFO *)lParam;
			MyWinHelp( (HWND)p->hItemHandle, m_szHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		}
		return TRUE;
		/*NOTREACHED*/
		//break;
//@@@ 2001.02.04 End

//@@@ 2001.12.22 Start by MIK: Context Menu Help
	//Context Menu
	case WM_CONTEXTMENU:
		MyWinHelp( hwndDlg, m_szHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		return TRUE;
//@@@ 2001.12.22 End

	}
	return FALSE;
}


/* ダイアログデータの設定 */
void CPropCommon::SetData_PROP_GREP( HWND hwndDlg )
{
//	BOOL	bRet;

//	BOOL	m_bGrepExitConfirm;	/* Grepモードで保存確認するか */

	/* 2006.08.23 ryoji カーソル位置の文字列をデフォルトの検索文字列にする */
	::CheckDlgButton( hwndDlg, IDC_CHECK_bCaretTextForSearch, m_Common.m_bCaretTextForSearch );

	/* Grepモードで保存確認するか */
	::CheckDlgButton( hwndDlg, IDC_CHECK_bGrepExitConfirm, m_Common.m_bGrepExitConfirm );

	/* Grep結果のリアルタイム表示 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_GREPREALTIME, m_Common.m_bGrepRealTimeView );	// 2006.08.08 ryoji ID修正


	/* Grepモード: エンターキーでタグジャンプ */
	::CheckDlgButton( hwndDlg, IDC_CHECK_GTJW_RETURN, m_Common.m_bGTJW_RETURN );

	/* Grepモード: ダブルクリックでタグジャンプ */
	::CheckDlgButton( hwndDlg, IDC_CHECK_GTJW_LDBLCLK, m_Common.m_bGTJW_LDBLCLK );

	//	2007.08.12 genta 正規表現DLL
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_REGEXPLIB ),  EM_LIMITTEXT, (WPARAM)( sizeof(m_Common.m_szRegexpLib ) - 1 ), 0 );
	::SetDlgItemText( hwndDlg, IDC_EDIT_REGEXPLIB, m_Common.m_szRegexpLib);
	SetRegexpVersion( hwndDlg );

	return;
}




/* ダイアログデータの取得 */
int CPropCommon::GetData_PROP_GREP( HWND hwndDlg )
{
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
//	m_nPageNum = ID_PAGENUM_GREP;

	/* 2006.08.23 ryoji カーソル位置の文字列をデフォルトの検索文字列にする */
	m_Common.m_bCaretTextForSearch = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_bCaretTextForSearch );

	/* Grepモードで保存確認するか */
	m_Common.m_bGrepExitConfirm = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_bGrepExitConfirm );

	/* Grep結果のリアルタイム表示 */
	m_Common.m_bGrepRealTimeView = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_GREPREALTIME );	// 2006.08.08 ryoji ID修正

	/* Grepモード: エンターキーでタグジャンプ */
	m_Common.m_bGTJW_RETURN = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_GTJW_RETURN );

	/* Grepモード: ダブルクリックでタグジャンプ */
	m_Common.m_bGTJW_LDBLCLK = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_GTJW_LDBLCLK );

	//	2007.08.12 genta 正規表現DLL
	::GetDlgItemText( hwndDlg, IDC_EDIT_REGEXPLIB, m_Common.m_szRegexpLib, sizeof( m_Common.m_szRegexpLib ));

	return TRUE;
}

void CPropCommon::SetRegexpVersion( HWND hwndDlg )
{
	TCHAR regexp_dll[_MAX_PATH];
	
	::GetDlgItemText( hwndDlg, IDC_EDIT_REGEXPLIB, regexp_dll, sizeof( regexp_dll ));
	CBregexp breg;
	if( ! breg.Init( regexp_dll ) ){
		// LMP: Added
		char _pszLabel[257];
		::LoadString( m_hInstance, STR_ERR_DLGPROPCOMGREP1, _pszLabel, 255 );  // LMP: Added

		::SetDlgItemText( hwndDlg, IDC_LABEL_REGEXP_VER, _pszLabel ) ; // _T("正規表現は使用できません") );
		return;
	}
	::SetDlgItemText( hwndDlg, IDC_LABEL_REGEXP_VER, breg.GetVersion() );
}


/*[EOF]*/
