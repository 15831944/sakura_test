//	$Id$
/*!	@file
	印刷ダイアログボックス
	
	@author Norio Nakatani
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani

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
#include "CDlgPrintPage.h"
#include "sakura_rc.h"

CDlgPrintPage::CDlgPrintPage()
{
	return;
}




/* モーダルダイアログの表示 */
int CDlgPrintPage::DoModal( HINSTANCE hInstance, HWND hwndParent, LPARAM lParam )
{
	return CDialog::DoModal( hInstance, hwndParent, IDD_PRINTPAGE, lParam );
}




/* ダイアログデータの設定 */
void CDlgPrintPage::SetData( void )
{
	char szText[100];
//	From Here Sept. 12, 2000 JEPRO スタイルを少し変更(指定を選んだ時だけ印刷範囲を入力できるようにした)
//	一度設定したページ設定値を保存できるようにしたいがまだできてない
//	::CheckDlgButton( m_hWnd, IDC_RADIO_ALL, m_bAllPage?BST_CHECKED:BST_UNCHECKED );
//	wsprintf( szText, "%d 〜 %d 頁", m_nPageMin, m_nPageMax );
//	::SetDlgItemText( m_hWnd, IDC_STATIC_ALL, szText );
//
//	::SetDlgItemInt( m_hWnd, IDC_EDIT_FROM, m_nPageFrom, FALSE );
//	::SetDlgItemInt( m_hWnd, IDC_EDIT_TO, m_nPageTo, FALSE );
//
	if( TRUE == m_bAllPage ){
		::CheckDlgButton( m_hWnd, IDC_RADIO_ALL, BST_CHECKED );
	}else{
		::CheckDlgButton( m_hWnd, IDC_RADIO_FROMTO, BST_CHECKED );
	}
	wsprintf( szText, "%d 〜 %d ページ", m_nPageMin, m_nPageMax );
	::SetDlgItemText( m_hWnd, IDC_STATIC_ALL, szText );

	::SetDlgItemInt( m_hWnd, IDC_EDIT_FROM, m_nPageFrom, FALSE );
	::SetDlgItemInt( m_hWnd, IDC_EDIT_TO, m_nPageTo, FALSE );

	//	印刷範囲を指定するかどうか
	if( FALSE == m_bAllPage ){
		::EnableWindow( ::GetDlgItem( m_hWnd, IDC_LABEL_FROM ), TRUE );
		::EnableWindow( ::GetDlgItem( m_hWnd, IDC_EDIT_FROM ), TRUE );
		::EnableWindow( ::GetDlgItem( m_hWnd, IDC_LABEL_TO ), TRUE );
		::EnableWindow( ::GetDlgItem( m_hWnd, IDC_EDIT_TO ), TRUE );
	}else{
		::EnableWindow( ::GetDlgItem( m_hWnd, IDC_LABEL_FROM ), FALSE );
		::EnableWindow( ::GetDlgItem( m_hWnd, IDC_EDIT_FROM ), FALSE );
		::EnableWindow( ::GetDlgItem( m_hWnd, IDC_LABEL_TO ), FALSE );
		::EnableWindow( ::GetDlgItem( m_hWnd, IDC_EDIT_TO ), FALSE );
	}
//	To Here Sept. 12, 2000
	
	return;
}




/* ダイアログデータの取得 */
int CDlgPrintPage::GetData( void )
{
	if( BST_CHECKED == ::IsDlgButtonChecked( m_hWnd, IDC_RADIO_ALL ) ){
		m_bAllPage = TRUE;
	}else{
		m_bAllPage = FALSE;
	}
	m_nPageFrom = ::GetDlgItemInt( m_hWnd, IDC_EDIT_FROM, NULL, FALSE ); 
	m_nPageTo   = ::GetDlgItemInt( m_hWnd, IDC_EDIT_TO  , NULL, FALSE ); 
	/* 頁範囲チェック */
	if( FALSE == m_bAllPage ){
		if( m_nPageMin <= m_nPageFrom && m_nPageFrom <= m_nPageMax &&
			m_nPageMin <= m_nPageTo   && m_nPageTo <= m_nPageMax &&
			m_nPageFrom <= m_nPageTo
		){
		}else{
			::MYMESSAGEBOX(	m_hWnd, MB_OK | MB_ICONSTOP | MB_TOPMOST, "入力エラー", "ページ範囲指定が正しくありません。" );
			return FALSE;
		}
	}
	return TRUE;
}




BOOL CDlgPrintPage::OnBnClicked( int wID )
{
	switch( wID ){
//	From Here Sept. 12, 2000 JEPRO スタイルを少し変更(指定を選んだ時だけ印刷範囲を入力できるようにした)
//	一度設定したページ設定値を保存できるようにしたいがまだできてない
	case IDC_RADIO_ALL:
	case IDC_RADIO_FROMTO:
	//	印刷範囲を指定するかどうか
		if( ::IsDlgButtonChecked( m_hWnd, IDC_RADIO_FROMTO ) ){
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_LABEL_FROM ), TRUE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_EDIT_FROM ), TRUE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_LABEL_TO ), TRUE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_EDIT_TO ), TRUE );
		}else{
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_LABEL_FROM ), FALSE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_EDIT_FROM ), FALSE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_LABEL_TO ), FALSE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_EDIT_TO ), FALSE );
		}
		return TRUE;
//	To Here Sept. 12, 2000
	
	case IDOK:			/* 下検索 */
		/* ダイアログデータの取得 */
		if( GetData() ){
			CloseDialog( 1 );
		}
		return TRUE;
	case IDCANCEL:
		CloseDialog( 0 );
		return TRUE;
	}
	return FALSE;
}


/*[EOF]*/

