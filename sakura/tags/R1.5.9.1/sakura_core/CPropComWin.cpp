//	$Id$
/*!	@file
	@brief 共通設定ダイアログボックス、「ウィンドウ」ページ

	@author Norio Nakatani
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000-2001, jepro
	Copyright (C) 2001, genta, MIK, asa-o
	Copyright (C) 2003, MIK

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/

#include "stdafx.h"
#include "CPropCommon.h"
#include "debug.h" // 2002/2/10 aroka
#include "global.h"
#include "CDlgWinSize.h"	//	2004.05.13 Moca

//@@@ 2001.02.04 Start by MIK: Popup Help
#include "sakura.hh"
static const DWORD p_helpids[] = {	//11200
	IDC_CHECK_DispFUNCKEYWND,		HIDC_CHECK_DispFUNCKEYWND,		//ファンクションキー表示
	IDC_CHECK_DispTabWnd,			HIDC_CHECK_DispTabWnd,			//タブウインドウ表示	//@@@ 2003.05.31 MIK
	IDC_CHECK_DispTabWndMultiWin,	HIDC_CHECK_DispTabWndMultiWin,	//タブウインドウ表示	//@@@ 2003.05.31 MIK
	IDC_CHECK_DispSTATUSBAR,		HIDC_CHECK_DispSTATUSBAR,		//ステータスバー表示
	IDC_CHECK_DispTOOLBAR,			HIDC_CHECK_DispTOOLBAR,			//ツールバー表示
	IDC_CHECK_bScrollBarHorz,		HIDC_CHECK_bScrollBarHorz,		//水平スクロールバー
	IDC_CHECK_bMenuIcon,			HIDC_CHECK_bMenuIcon,			//アイコン付きメニュー
	IDC_CHECK_SplitterWndVScroll,	HIDC_CHECK_SplitterWndVScroll,	//垂直スクロールの同期	//Jul. 05, 2001 JEPRO 追加
	IDC_CHECK_SplitterWndHScroll,	HIDC_CHECK_SplitterWndHScroll,	//水平スクロールの同期	//Jul. 05, 2001 JEPRO 追加
	IDC_EDIT_nRulerBottomSpace,		HIDC_EDIT_nRulerBottomSpace,	//ルーラーの高さ
	IDC_EDIT_nRulerHeight,			HIDC_EDIT_nRulerHeight,			//ルーラーとテキストの間隔
	IDC_EDIT_nLineNumberRightSpace,	HIDC_EDIT_nLineNumberRightSpace,	//行番号とテキストの隙間
	IDC_RADIO_FUNCKEYWND_PLACE1,	HIDC_RADIO_FUNCKEYWND_PLACE1,	//ファンクションキー表示位置
	IDC_RADIO_FUNCKEYWND_PLACE2,	HIDC_RADIO_FUNCKEYWND_PLACE2,	//ファンクションキー表示位置
	IDC_EDIT_FUNCKEYWND_GROUPNUM,	HIDC_EDIT_FUNCKEYWND_GROUPNUM,	//ファンクションキーのグループボタン数
	IDC_SPIN_nRulerBottomSpace,		HIDC_EDIT_nRulerBottomSpace,
	IDC_SPIN_nRulerHeight,			HIDC_EDIT_nRulerHeight,
	IDC_SPIN_nLineNumberRightSpace,	HIDC_EDIT_nLineNumberRightSpace,
	IDC_SPIN_FUNCKEYWND_GROUPNUM,	HIDC_EDIT_FUNCKEYWND_GROUPNUM,
	IDC_WINCAPTION_ACTIVE,			HIDC_WINCAPTION_ACTIVE,			//アクティブ時	//@@@ 2003.06.15 MIK
	IDC_WINCAPTION_INACTIVE,		HIDC_WINCAPTION_INACTIVE,		//非アクティブ時	//@@@ 2003.06.15 MIK
	IDC_TABWND_CAPTION,				HIDC_TABWND_CAPTION,			//タブウインドウキャプション	//@@@ 2003.06.15 MIK
//	IDC_STATIC,						-1,
	0, 0
};
//@@@ 2001.02.04 End

//	From Here Jun. 2, 2001 genta
/*!
	@param hwndDlg ダイアログボックスのWindow Handle
	@param uMsg メッセージ
	@param wParam パラメータ1
	@param lParam パラメータ2
*/
INT_PTR CALLBACK CPropCommon::DlgProc_PROP_WIN(
	HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return DlgProc( &CPropCommon::DispatchEvent_PROP_WIN, hwndDlg, uMsg, wParam, lParam );
}
//	To Here Jun. 2, 2001 genta


/* メッセージ処理 */
INT_PTR CPropCommon::DispatchEvent_PROP_WIN(
	HWND	hwndDlg,	// handle to dialog box
	UINT	uMsg,	// message
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
)
{
//	WORD		wNotifyCode;
//	WORD		wID;
//	HWND		hwndCtl;

// From Here Sept. 9, 2000 JEPRO
	WORD		wNotifyCode;
	WORD		wID;
	HWND		hwndCtl;
// To Here Sept. 9, 2000

	NMHDR*		pNMHDR;
	NM_UPDOWN*	pMNUD;
	int			idCtrl;
//	int			nVal;
	int			nVal;	//Sept.21, 2000 JEPRO スピン要素を加えたので復活させた
//	LPDRAWITEMSTRUCT pDis;

	switch( uMsg ){

	case WM_INITDIALOG:
		/* ダイアログデータの設定 p1 */
		SetData_PROP_WIN( hwndDlg );
		// Modified by KEITA for WIN64 2003.9.6
		::SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

		/* ユーザーがエディット コントロールに入力できるテキストの長さを制限する */
		/* ルーラー高さ */
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_nRulerHeight ), EM_LIMITTEXT, (WPARAM)2, 0 );
		/* ルーラーとテキストの隙間 */
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_nRulerBottomSpace ), EM_LIMITTEXT, (WPARAM)2, 0 );

		return TRUE;
//****	From Here Sept. 21, 2000 JEPRO ダイアログ要素にスピンを入れるので以下のWM_NOTIFYをコメントアウトにし下に修正を置いた
//	case WM_NOTIFY:
//		idCtrl = (int)wParam;
//		pNMHDR = (NMHDR*)lParam;
//		pMNUD  = (NM_UPDOWN*)lParam;
////		switch( idCtrl ){
////		default:
//			switch( pNMHDR->code ){
//			case PSN_HELP:
//				OnHelp( hwndDlg, IDD_PROP_WIN );
//				return TRUE;
//			case PSN_KILLACTIVE:
////				MYTRACE( "p1 PSN_KILLACTIVE\n" );
//				/* ダイアログデータの取得 p1 */
//				GetData_PROP_WIN( hwndDlg );
//				return TRUE;
//			}
////			break;	//	Sept. 9, 2000 JEPRO この行は下のbreakとダブっていて冗長なので削除してよいはず
////		}
//		break;

	case WM_NOTIFY:
		idCtrl = (int)wParam;
		pNMHDR = (NMHDR*)lParam;
		pMNUD  = (NM_UPDOWN*)lParam;
		switch( idCtrl ){
		default:
			switch( pNMHDR->code ){
			case PSN_HELP:
				OnHelp( hwndDlg, IDD_PROP_WIN );
				return TRUE;
			case PSN_KILLACTIVE:
//				MYTRACE( "p1 PSN_KILLACTIVE\n" );
				/* ダイアログデータの取得 p1 */
				GetData_PROP_WIN( hwndDlg );
				return TRUE;
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
			case PSN_SETACTIVE:
				m_nPageNum = ID_PAGENUM_WIN;
				return TRUE;
			}
			break;
		case IDC_SPIN_nRulerHeight:
			/* ルーラ−の高さ */
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_nRulerHeight, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < IDC_SPIN_nRulerHeight_MIN ){
				nVal = IDC_SPIN_nRulerHeight_MIN;
			}
			if( nVal > IDC_SPIN_nRulerHeight_MAX ){
				nVal = IDC_SPIN_nRulerHeight_MAX;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_nRulerHeight, nVal, FALSE );
			return TRUE;
		case IDC_SPIN_nRulerBottomSpace:
			/* ルーラーとテキストの隙間 */
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_nRulerBottomSpace, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 0 ){
				nVal = 0;
			}
			if( nVal > 32 ){
				nVal = 32;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_nRulerBottomSpace, nVal, FALSE );
			return TRUE;
		case IDC_SPIN_nLineNumberRightSpace:
			/* ルーラーとテキストの隙間 */
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_nLineNumberRightSpace, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 0 ){
				nVal = 0;
			}
			if( nVal > 32 ){
				nVal = 32;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_nLineNumberRightSpace, nVal, FALSE );
			return TRUE;
		case IDC_SPIN_FUNCKEYWND_GROUPNUM:
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_FUNCKEYWND_GROUPNUM, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 1 ){
				nVal = 1;
			}
			if( nVal > 12 ){
				nVal = 12;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_FUNCKEYWND_GROUPNUM, nVal, FALSE );
			return TRUE;
		}
		break;
//****	To Here Sept. 21, 2000
//	From Here Sept. 9, 2000 JEPRO
	case WM_COMMAND:
		wNotifyCode	= HIWORD(wParam);	/* 通知コード */
		wID			= LOWORD(wParam);	/* 項目ID､ コントロールID､ またはアクセラレータID */
		hwndCtl		= (HWND) lParam;	/* コントロールのハンドル */
		switch( wNotifyCode ){
		/* ボタン／チェックボックスがクリックされた */
		case BN_CLICKED:
			switch( wID ){
			//	ファンクションキーを表示する時だけその位置指定をEnableに設定
			case IDC_CHECK_DispFUNCKEYWND:
				EnableWinPropInput( hwndDlg );
				break;

			//@@@ 2003.06.13 MIK
			case IDC_CHECK_DispTabWnd:
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_DispTabWnd ) )
				{
					::EnableWindow( ::GetDlgItem( hwndDlg, IDC_CHECK_DispTabWndMultiWin ), TRUE );
					//::EnableWindow( ::GetDlgItem( hwndDlg, IDC_TABWND_CAPTION           ), TRUE );
				}
				else
				{
					::EnableWindow( ::GetDlgItem( hwndDlg, IDC_CHECK_DispTabWndMultiWin ), FALSE );
					//::EnableWindow( ::GetDlgItem( hwndDlg, IDC_TABWND_CAPTION           ), FALSE );
				}
				break;
			// From Here 2004.05.13 Moca 「位置と大きさの設定」ボタン
			//	ウィンドウ設定ダイアログにて起動時のウィンドウ状態指定
			case IDC_BUTTON_WINSIZE:
				{
					CDlgWinSize cDlgWinSize;
					RECT rc;
					rc.right  = m_Common.m_nWinSizeCX;
					rc.bottom = m_Common.m_nWinSizeCY;
					rc.top    = m_Common.m_nWinPosX;
					rc.left   = m_Common.m_nWinPosY;
					cDlgWinSize.DoModal( ::GetModuleHandle(NULL), hwndDlg,
						m_Common.m_nSaveWindowSize, m_Common.m_nSaveWindowPos,
						m_Common.m_nWinSizeType, rc
					);
					m_Common.m_nWinSizeCX = rc.right;
					m_Common.m_nWinSizeCY = rc.bottom;
					m_Common.m_nWinPosX = rc.top;
					m_Common.m_nWinPosY = rc.left;
				}
				break;
			// To Here 2004.05.13 Moca
			}
			break;
		}
		break;
//	To Here Sept. 9, 2000

//@@@ 2001.02.04 Start by MIK: Popup Help
	case WM_HELP:
		{
			HELPINFO *p = (HELPINFO *)lParam;
			::WinHelp( (HWND)p->hItemHandle, m_szHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)p_helpids );
		}
		return TRUE;
		/*NOTREACHED*/
		//break;
//@@@ 2001.02.04 End

//@@@ 2001.12.22 Start by MIK: Context Menu Help
	//Context Menu
	case WM_CONTEXTMENU:
		::WinHelp( hwndDlg, m_szHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)p_helpids );
		return TRUE;
//@@@ 2001.12.22 End

	}
	return FALSE;
}

/* ダイアログデータの設定 */
void CPropCommon::SetData_PROP_WIN( HWND hwndDlg )
{
//	BOOL	bRet;

	/* 次回ウィンドウを開いたときツールバーを表示する */
	::CheckDlgButton( hwndDlg, IDC_CHECK_DispTOOLBAR, m_Common.m_bDispTOOLBAR );

	/* 次回ウィンドウを開いたときファンクションキーを表示する */
	::CheckDlgButton( hwndDlg, IDC_CHECK_DispFUNCKEYWND, m_Common.m_bDispFUNCKEYWND );

	/* ファンクションキー表示位置／0:上 1:下 */
	if( 0 == m_Common.m_nFUNCKEYWND_Place ){
		::CheckDlgButton( hwndDlg, IDC_RADIO_FUNCKEYWND_PLACE1, TRUE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_FUNCKEYWND_PLACE2, FALSE );
	}else{
		::CheckDlgButton( hwndDlg, IDC_RADIO_FUNCKEYWND_PLACE1, FALSE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_FUNCKEYWND_PLACE2, TRUE );
	}
	// 2002/11/04 Moca ファンクションキーのグループボタン数
	::SetDlgItemInt( hwndDlg, IDC_EDIT_FUNCKEYWND_GROUPNUM, m_Common.m_nFUNCKEYWND_GroupNum, FALSE );

	//From Here@@@ 2003.06.13 MIK
	/* 次回ウィンドウを開いたときタブを表示する */
	::CheckDlgButton( hwndDlg, IDC_CHECK_DispTabWnd, m_Common.m_bDispTabWnd );	//@@@ 2003.05.31 MIK
	::CheckDlgButton( hwndDlg, IDC_CHECK_DispTabWndMultiWin, m_Common.m_bDispTabWndMultiWin );	//@@@ 2003.05.31 MIK
	if( FALSE == m_Common.m_bDispTabWnd )
	{
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_CHECK_DispTabWndMultiWin ), FALSE );
		//::EnableWindow( ::GetDlgItem( hwndDlg, IDC_TABWND_CAPTION           ), FALSE );
	}
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_TABWND_CAPTION ), EM_LIMITTEXT, (WPARAM)(sizeof( m_Common.m_szTabWndCaption ) - 1 ), (LPARAM)0 );
	::SetDlgItemText( hwndDlg, IDC_TABWND_CAPTION, m_Common.m_szTabWndCaption );
	//To Here@@@ 2003.06.13 MIK

	/* 次回ウィンドウを開いたときステータスバーを表示する */
	::CheckDlgButton( hwndDlg, IDC_CHECK_DispSTATUSBAR, m_Common.m_bDispSTATUSBAR );

	/* ルーラー高さ */
	::SetDlgItemInt( hwndDlg, IDC_EDIT_nRulerHeight, m_Common.m_nRulerHeight, FALSE );
	/* ルーラーとテキストの隙間 */
	::SetDlgItemInt( hwndDlg, IDC_EDIT_nRulerBottomSpace, m_Common.m_nRulerBottomSpace, FALSE );
	//	Sep. 18. 2002 genta 行番号とテキストの隙間
	::SetDlgItemInt( hwndDlg, IDC_EDIT_nLineNumberRightSpace, m_Common.m_nLineNumRightSpace, FALSE );

	/* ルーラーのタイプ */
	if( 0 == m_Common.m_nRulerType ){
		::CheckDlgButton( hwndDlg, IDC_RADIO_nRulerType_0, TRUE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_nRulerType_1, FALSE );
	}else{
		::CheckDlgButton( hwndDlg, IDC_RADIO_nRulerType_0, FALSE );
		::CheckDlgButton( hwndDlg, IDC_RADIO_nRulerType_1, TRUE );
	}

	/* 水平スクロールバー */
	::CheckDlgButton( hwndDlg, IDC_CHECK_bScrollBarHorz, m_Common.m_bScrollBarHorz );

	/* アイコン付きメニュー */
	::CheckDlgButton( hwndDlg, IDC_CHECK_bMenuIcon, m_Common.m_bMenuIcon );

	//	2001/06/20 Start by asa-o:	スクロールの同期
	::CheckDlgButton( hwndDlg, IDC_CHECK_SplitterWndVScroll, m_Common.m_bSplitterWndVScroll );
	::CheckDlgButton( hwndDlg, IDC_CHECK_SplitterWndHScroll, m_Common.m_bSplitterWndHScroll );
	//	2001/06/20 End

	//	Apr. 05, 2003 genta ウィンドウキャプションのカスタマイズ
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_WINCAPTION_ACTIVE   ), EM_LIMITTEXT, (WPARAM)(sizeof( m_Common.m_szWindowCaptionActive   ) - 1 ), (LPARAM)0 );	//@@@ 2003.06.13 MIK
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_WINCAPTION_INACTIVE ), EM_LIMITTEXT, (WPARAM)(sizeof( m_Common.m_szWindowCaptionInactive ) - 1 ), (LPARAM)0 );	//@@@ 2003.06.13 MIK
	::SetDlgItemText( hwndDlg, IDC_WINCAPTION_ACTIVE, m_Common.m_szWindowCaptionActive );
	::SetDlgItemText( hwndDlg, IDC_WINCAPTION_INACTIVE, m_Common.m_szWindowCaptionInactive );

	//	Fronm Here Sept. 9, 2000 JEPRO
	//	ファンクションキーを表示する時だけその位置指定をEnableに設定
	EnableWinPropInput( hwndDlg );
	//	To Here Sept. 9, 2000

	return;
}





/* ダイアログデータの取得 */
int CPropCommon::GetData_PROP_WIN( HWND hwndDlg )
{
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
//	m_nPageNum = ID_PAGENUM_WIN;

	/* 次回ウィンドウを開いたときツールバーを表示する */
	m_Common.m_bDispTOOLBAR = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_DispTOOLBAR );

	/* 次回ウィンドウを開いたときファンクションキーを表示する */
	m_Common.m_bDispFUNCKEYWND = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_DispFUNCKEYWND );

	/* ファンクションキー表示位置／0:上 1:下 */
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_FUNCKEYWND_PLACE1 ) ){
		m_Common.m_nFUNCKEYWND_Place = 0;
	}
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_FUNCKEYWND_PLACE2) ){
		m_Common.m_nFUNCKEYWND_Place = 1;
	}

	// 2002/11/04 Moca ファンクションキーのグループボタン数
	m_Common.m_nFUNCKEYWND_GroupNum = ::GetDlgItemInt( hwndDlg, IDC_EDIT_FUNCKEYWND_GROUPNUM, NULL, FALSE );
	if( m_Common.m_nFUNCKEYWND_GroupNum < 1 ){
		m_Common.m_nFUNCKEYWND_GroupNum = 1;
	}
	if( m_Common.m_nFUNCKEYWND_GroupNum > 12 ){
		m_Common.m_nFUNCKEYWND_GroupNum = 12;
	}

	//From Here@@@ 2003.06.13 MIK
	/* 次回ウィンドウを開いたときタブを表示する */
	m_Common.m_bDispTabWnd = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_DispTabWnd );
	m_Common.m_bDispTabWndMultiWin = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_DispTabWndMultiWin );
	::GetDlgItemText( hwndDlg, IDC_TABWND_CAPTION, m_Common.m_szTabWndCaption, sizeof( m_Common.m_szTabWndCaption ) );
	//To Here@@@ 2003.06.13 MIK


	/* 次回ウィンドウを開いたときステータスバーを表示する */
	m_Common.m_bDispSTATUSBAR = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_DispSTATUSBAR );

	/* ルーラーのタイプ */
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_nRulerType_0 ) ){
		m_Common.m_nRulerType = 0;
	}
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_nRulerType_1 ) ){
		m_Common.m_nRulerType = 1;
	}

	/* ルーラー高さ */
	m_Common.m_nRulerHeight = ::GetDlgItemInt( hwndDlg, IDC_EDIT_nRulerHeight, NULL, FALSE );
	if( m_Common.m_nRulerHeight < IDC_SPIN_nRulerHeight_MIN ){
		m_Common.m_nRulerHeight = IDC_SPIN_nRulerHeight_MIN;
	}
	if( m_Common.m_nRulerHeight > IDC_SPIN_nRulerHeight_MAX ){
		m_Common.m_nRulerHeight = IDC_SPIN_nRulerHeight_MAX;
	}
	/* ルーラーとテキストの隙間 */
	m_Common.m_nRulerBottomSpace = ::GetDlgItemInt( hwndDlg, IDC_EDIT_nRulerBottomSpace, NULL, FALSE );
	if( m_Common.m_nRulerBottomSpace < 0 ){
		m_Common.m_nRulerBottomSpace = 0;
	}
	if( m_Common.m_nRulerBottomSpace > 32 ){
		m_Common.m_nRulerBottomSpace = 32;
	}

	//	Sep. 18. 2002 genta 行番号とテキストの隙間
	m_Common.m_nLineNumRightSpace = ::GetDlgItemInt( hwndDlg, IDC_EDIT_nLineNumberRightSpace, NULL, FALSE );
	if( m_Common.m_nLineNumRightSpace < 0 ){
		m_Common.m_nLineNumRightSpace = 0;
	}
	if( m_Common.m_nLineNumRightSpace > 32 ){
		m_Common.m_nLineNumRightSpace = 32;
	}

	/* 水平スクロールバー */
	m_Common.m_bScrollBarHorz = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_bScrollBarHorz );

	/* アイコン付きメニュー */
	m_Common.m_bMenuIcon = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_bMenuIcon );

	//	2001/06/20 Start by asa-o:	スクロールの同期
	m_Common.m_bSplitterWndVScroll = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_SplitterWndVScroll );
	m_Common.m_bSplitterWndHScroll = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_SplitterWndHScroll );
	//	2001/06/20 End

	//	Apr. 05, 2003 genta ウィンドウキャプションのカスタマイズ
	::GetDlgItemText( hwndDlg, IDC_WINCAPTION_ACTIVE, m_Common.m_szWindowCaptionActive,
		sizeof( m_Common.m_szWindowCaptionActive ) );
	::GetDlgItemText( hwndDlg, IDC_WINCAPTION_INACTIVE, m_Common.m_szWindowCaptionInactive,
		sizeof( m_Common.m_szWindowCaptionInactive ) );

	return TRUE;
}





//	From Here Sept. 9, 2000 JEPRO
//	チェック状態に応じてダイアログボックス要素のEnable/Disableを
//	適切に設定する
void CPropCommon::EnableWinPropInput( HWND hwndDlg )
{
	//	ファクションキーを表示するかどうか
		if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_DispFUNCKEYWND ) ){
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_GROUP_FUNCKEYWND_POSITION ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_RADIO_FUNCKEYWND_PLACE1 ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_RADIO_FUNCKEYWND_PLACE2 ), TRUE );
	}else{
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_GROUP_FUNCKEYWND_POSITION ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_RADIO_FUNCKEYWND_PLACE1 ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_RADIO_FUNCKEYWND_PLACE2 ), FALSE );
	}
}
//	To Here Sept. 9, 2000


/*[EOF]*/
