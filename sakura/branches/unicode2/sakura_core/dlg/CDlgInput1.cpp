/*!	@file
	@brief 1行入力ダイアログボックス

	@author Norio Nakatani
	@date	1998/05/31 作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2002, MIK
	Copyright (C) 2003, KEITA
	Copyright (C) 2006, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/
#include "stdafx.h"
#include "sakura_rc.h"
#include "dlg/CDlgInput1.h"
#include "debug/Debug.h"
#include "CEditApp.h"
#include "util/shell.h"

// 入力 CDlgInput1.cpp	//@@@ 2002.01.07 add start MIK
#include "sakura.hh"
static const DWORD p_helpids[] = {	//13000
	IDOK,					HIDOK_DLG1,
	IDCANCEL,				HIDCANCEL_DLG1,
	IDC_EDIT_INPUT1,		HIDC_DLG1_EDIT1,	//入力フィールド	IDC_EDIT1->IDC_EDIT_INPUT1	2008/7/3 Uchi
	IDC_STATIC_MSG,			HIDC_DLG1_EDIT1,	//メッセージ
//	IDC_STATIC,				-1,
	0, 0
};	//@@@ 2002.01.07 add end MIK


/* ダイアログプロシージャ */
INT_PTR CALLBACK CDlgInput1Proc(
	HWND hwndDlg,	// handle to dialog box
	UINT uMsg,		// message
	WPARAM wParam,	// first message parameter
	LPARAM lParam 	// second message parameter
)
{
	CDlgInput1* pCDlgInput1;
	switch( uMsg ){
	case WM_INITDIALOG:
		pCDlgInput1 = ( CDlgInput1* )lParam;
		if( NULL != pCDlgInput1 ){
			return pCDlgInput1->DispatchEvent( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	default:
		// Modified by KEITA for WIN64 2003.9.6
		pCDlgInput1 = ( CDlgInput1* )::GetWindowLongPtr( hwndDlg, DWLP_USER );
		if( NULL != pCDlgInput1 ){
			return pCDlgInput1->DispatchEvent( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	}
}



CDlgInput1::CDlgInput1()
{
	/* ヘルプファイルのフルパスを返す */
	m_szHelpFile = CEditApp::Instance()->GetHelpFilePath();	//@@@ 2002.01.07 add

	return;
}



CDlgInput1::~CDlgInput1()
{
	return;
}



/* モードレスダイアログの表示 */
BOOL CDlgInput1::DoModal(
	HINSTANCE		hInstApp,
	HWND			hwndParent,
	const TCHAR*	pszTitle,
	const TCHAR*	pszMessage,
	int				nMaxTextLen,
	TCHAR*			pszText
)
{
	BOOL bRet;
	m_hInstance = hInstApp;		/* アプリケーションインスタンスのハンドル */
	m_hwndParent = hwndParent;	/* オーナーウィンドウのハンドル */
	m_pszTitle = pszTitle;		/* ダイアログタイトル */
	m_pszMessage = pszMessage;		/* メッセージ */
	m_nMaxTextLen = nMaxTextLen;	/* 入力サイズ上限 */
//	m_pszText = pszText;			/* テキスト */
	m_cmemText.SetString( pszText );
	bRet = (BOOL)::DialogBoxParam(
		m_hInstance,
		MAKEINTRESOURCE( IDD_INPUT1 ),
		m_hwndParent,
		CDlgInput1Proc,
		(LPARAM)this
	);
	_tcscpy( pszText, m_cmemText.GetStringPtr() );
	return bRet;
}

BOOL CDlgInput1::DoModal(
	HINSTANCE		hInstApp,
	HWND			hwndParent,
	const TCHAR*	pszTitle,
	const TCHAR*	pszMessage,
	int				nMaxTextLen,
	NOT_TCHAR*		pszText
)
{
	TCHAR buf[1024];
	buf[0] = _T('\0');
	BOOL ret=DoModal(hInstApp, hwndParent, pszTitle, pszMessage, nMaxTextLen, buf);
	if(ret){
		auto_strcpy(pszText,to_not_tchar(buf));
	}
	return ret;
}



/* ダイアログのメッセージ処理 */
INT_PTR CDlgInput1::DispatchEvent(
	HWND hwndDlg,	// handle to dialog box
	UINT uMsg,		// message
	WPARAM wParam,	// first message parameter
	LPARAM lParam 	// second message parameter
)
{
	WORD	wNotifyCode;
	WORD	wID;
	HWND	hwndCtl;
//	int		nRet;
	switch( uMsg ){
	case WM_INITDIALOG:
		/* ダイアログデータの設定 */
		// Modified by KEITA for WIN64 2003.9.6
		::SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

		::SetWindowText( hwndDlg, m_pszTitle );	/* ダイアログタイトル */
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_INPUT1 ), EM_LIMITTEXT, m_nMaxTextLen, 0 );	/* 入力サイズ上限 */
		DlgItem_SetText( hwndDlg, IDC_EDIT_INPUT1, m_cmemText.GetStringPtr() );	/* テキスト */
		::SetWindowText( ::GetDlgItem( hwndDlg, IDC_STATIC_MSG ), m_pszMessage );	/* メッセージ */

		return TRUE;
	case WM_COMMAND:
		wNotifyCode = HIWORD(wParam);	/* 通知コード */
		wID			= LOWORD(wParam);	/* 項目ID､ コントロールID､ またはアクセラレータID */
		hwndCtl		= (HWND) lParam;	/* コントロールのハンドル */
		switch( wNotifyCode ){
		/* ボタン／チェックボックスがクリックされた */
		case BN_CLICKED:
			switch( wID ){
			case IDOK:
				m_cmemText.AllocStringBuffer( ::GetWindowTextLength( ::GetDlgItem( hwndDlg, IDC_EDIT_INPUT1 ) ) );
				::GetWindowText( ::GetDlgItem( hwndDlg, IDC_EDIT_INPUT1 ), m_cmemText.GetStringPtr(), m_nMaxTextLen + 1 );	/* テキスト */
				::EndDialog( hwndDlg, TRUE );
				return TRUE;
			case IDCANCEL:
				::EndDialog( hwndDlg, FALSE );
				return TRUE;
			}
			break;	//@@@ 2002.01.07 add
		}
		break;	//@@@ 2002.01.07 add
	//@@@ 2002.01.07 add start
	case WM_HELP:
		{
			HELPINFO *p = (HELPINFO *)lParam;
			MyWinHelp( (HWND)p->hItemHandle, m_szHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		}
		return TRUE;

	//Context Menu
	case WM_CONTEXTMENU:
		MyWinHelp( hwndDlg, m_szHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		return TRUE;
	//@@@ 2002.01.07 add end
	}
	return FALSE;
}
