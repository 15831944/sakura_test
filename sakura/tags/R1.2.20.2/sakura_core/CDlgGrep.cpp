//	$Id$
/************************************************************************

	CDlgGrep.cpp
	Copyright (C) 1998-2000, Norio Nakatani

************************************************************************/
#include <windows.h>
//#include <stdio.h>


#include "sakura_rc.h"
#include "CDlgGrep.h"
#include "debug.h"

#include "CJre.h"
//#include "_global_fio.h"
#include "etc_uty.h"
#include "global.h"

CDlgGrep::CDlgGrep()
{
	m_bSubFolder = FALSE;	/* サブフォルダからも検索する */
	m_bFromThisText = FALSE;	/* この編集中のテキストから検索する */
	m_bLoHiCase = FALSE;	/* 英大文字と英小文字を区別する */
	m_bRegularExp = FALSE;	/* 正規表現 */
	m_bKanjiCode_AutoDetect = FALSE;	/* 文字コード自動判別 */
	m_bGrepOutputLine = TRUE;	/* 行を出力するか該当部分だけ出力するか */
	m_nGrepOutputStyle = 1;				/* Grep: 出力形式 */

	strcpy( m_szText, m_pShareData->m_szSEARCHKEYArr[0] );		/* 検索文字列 */
	strcpy( m_szFile, m_pShareData->m_szGREPFILEArr[0] );		/* 検索ファイル */
	strcpy( m_szFolder, m_pShareData->m_szGREPFOLDERArr[0] );	/* 検索フォルダ */
	return;
}



/* モーダルダイアログの表示 */
int CDlgGrep::DoModal( HINSTANCE hInstance, HWND hwndParent, const char* pszCurrentFilePath )
{
	m_bSubFolder = m_pShareData->m_Common.m_bGrepSubFolder;	/* Grep: サブフォルダも検索 */
	m_bRegularExp = m_pShareData->m_Common.m_bRegularExp;	/* 1==正規表現 */
	m_bKanjiCode_AutoDetect = m_pShareData->m_Common.m_bGrepKanjiCode_AutoDetect;	/* 文字コード自動判別 */
	m_bLoHiCase = m_pShareData->m_Common.m_bLoHiCase;		/* 1==大文字小文字の区別 */
	m_bGrepOutputLine = m_pShareData->m_Common.m_bGrepOutputLine;	/* 行を出力するか該当部分だけ出力するか */
	m_nGrepOutputStyle = m_pShareData->m_Common.m_nGrepOutputStyle;	/* Grep: 出力形式 */
	lstrcpy( m_szCurrentFilePath, pszCurrentFilePath );

	return CDialog::DoModal( hInstance, hwndParent, IDD_GREP, NULL );
}
	
//	/* モードレスダイアログの表示 */
//	HWND CDlgGrep::DoModeless( HINSTANCE hInstance, HWND hwndParent, const char* pszCurrentFilePath )
//	{
//		m_bSubFolder = m_pShareData->m_Common.m_bGrepSubFolder;	/* Grep: サブフォルダも検索 */
//		m_bRegularExp = m_pShareData->m_Common.m_bRegularExp;	/* 1==正規表現 */
//		m_bKanjiCode_AutoDetect = m_pShareData->m_Common.m_bGrepKanjiCode_AutoDetect;	/* 文字コード自動判別 */
//		m_bLoHiCase = m_pShareData->m_Common.m_bLoHiCase;		/* 1==英大文字小文字の区別 */
//		m_bGrepOutputLine = m_pShareData->m_Common.m_bGrepOutputLine;	/* 行を出力するか該当部分だけ出力するか */
//		m_nGrepOutputStyle = m_pShareData->m_Common.m_nGrepOutputStyle;	/* Grep: 出力形式 */
//		lstrcpy( m_szCurrentFilePath, pszCurrentFilePath );
//  
//		return CDialog::DoModeless( hInstance, hwndParent, IDD_GREP, NULL );
//	}


BOOL CDlgGrep::OnInitDialog( HWND hwndDlg, WPARAM wParam, LPARAM lParam )
{
	m_hWnd = hwndDlg;

	/* ユーザーがコンボ ボックスのエディット コントロールに入力できるテキストの長さを制限する */
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_COMBO_TEXT ), CB_LIMITTEXT, (WPARAM)_MAX_PATH - 1, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_COMBO_FILE ), CB_LIMITTEXT, (WPARAM)_MAX_PATH - 1, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_COMBO_FOLDER ), CB_LIMITTEXT, (WPARAM)_MAX_PATH - 1, 0 );

	/* コンボボックスのユーザー インターフェイスを拡張インターフェースにする */
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_COMBO_TEXT ), CB_SETEXTENDEDUI, (WPARAM) (BOOL) TRUE, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_COMBO_FILE ), CB_SETEXTENDEDUI, (WPARAM) (BOOL) TRUE, 0 );
	::SendMessage( ::GetDlgItem( m_hWnd, IDC_COMBO_FOLDER ), CB_SETEXTENDEDUI, (WPARAM) (BOOL) TRUE, 0 );

	/* ダイアログのアイコン */
//	::SendMessage( m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)::LoadIcon( m_hInstance, IDI_QUESTION ) );
	HICON	hIcon;
//	hIcon = ::LoadIcon( NULL, IDI_QUESTION );
	hIcon = ::LoadIcon( m_hInstance, MAKEINTRESOURCE( IDI_ICON_GREP ) );
	::SendMessage( m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)NULL );
	::SendMessage( m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon );
	::SendMessage( m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)NULL );
	::SendMessage( m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon );


	/* 基底クラスメンバ */
//	CreateSizeBox();
	return CDialog::OnInitDialog( hwndDlg, wParam, lParam );
}



BOOL CDlgGrep::OnBnClicked( int wID )
{
	switch( wID ){
	case IDC_BUTTON_HELP:
		/* 「Grep」のヘルプ */
		::WinHelp( m_hWnd, m_szHelpFile, HELP_CONTEXT, 67 );
		return TRUE;
	case IDC_CHK_FROMTHISTEXT:	/* この編集中のテキストから検索する */
		if( 0 < (int)lstrlen(m_szCurrentFilePath ) ){
			if( ::IsDlgButtonChecked( m_hWnd, IDC_CHK_FROMTHISTEXT ) ){
				::EnableWindow( ::GetDlgItem( m_hWnd, IDC_COMBO_FILE ), FALSE );
				::EnableWindow( ::GetDlgItem( m_hWnd, IDC_COMBO_FOLDER ), FALSE );
				::EnableWindow( ::GetDlgItem( m_hWnd, IDC_BUTTON_FOLDER ), FALSE );
				::EnableWindow( ::GetDlgItem( m_hWnd, IDC_CHK_SUBFOLDER ), FALSE );
				::CheckDlgButton( m_hWnd, IDC_CHK_SUBFOLDER, 0 );
			}else{
				::EnableWindow( ::GetDlgItem( m_hWnd, IDC_COMBO_FILE ), TRUE );
				::EnableWindow( ::GetDlgItem( m_hWnd, IDC_COMBO_FOLDER ), TRUE );
				::EnableWindow( ::GetDlgItem( m_hWnd, IDC_BUTTON_FOLDER ), TRUE );
				::EnableWindow( ::GetDlgItem( m_hWnd, IDC_CHK_SUBFOLDER ), TRUE );
			}
			char	szWorkFolder[MAX_PATH];
			char	szWorkFile[MAX_PATH];
			SplitPath_FolderAndFile( m_szCurrentFilePath, szWorkFolder, szWorkFile );
			::SetDlgItemText( m_hWnd, IDC_COMBO_FOLDER, szWorkFolder );
			::SetDlgItemText( m_hWnd, IDC_COMBO_FILE, szWorkFile );
		}
		return TRUE;
	case IDC_BUTTON_CURRENTFOLDER:	/* 現在編集中のファイルのフォルダ */
		/* ファイルを開いているか */
		if( 0 < lstrlen( m_szCurrentFilePath ) ){
			char	szWorkFolder[MAX_PATH];
			char	szWorkFile[MAX_PATH];
			SplitPath_FolderAndFile( m_szCurrentFilePath, szWorkFolder, szWorkFile );
			::SetDlgItemText( m_hWnd, IDC_COMBO_FOLDER, szWorkFolder );
		}else{
			/* 現在のプロセスのカレントディレクトリを取得します */
			char	szWorkFolder[MAX_PATH];
			::GetCurrentDirectory( sizeof( szWorkFolder ) - 1, szWorkFolder );
			::SetDlgItemText( m_hWnd, IDC_COMBO_FOLDER, szWorkFolder );
//			::MessageBeep( MB_ICONEXCLAMATION );
		}
		return TRUE;


//	case IDC_CHK_LOHICASE:	/* 英大文字と英小文字を区別する */
//		MYTRACE( "IDC_CHK_LOHICASE\n" );
//		return TRUE;
	case IDC_CHK_REGULAREXP:	/* 正規表現 */
//		MYTRACE( "IDC_CHK_REGULAREXP ::IsDlgButtonChecked( m_hWnd, IDC_CHK_REGULAREXP ) = %d\n", ::IsDlgButtonChecked( m_hWnd, IDC_CHK_REGULAREXP ) );
		if( ::IsDlgButtonChecked( m_hWnd, IDC_CHK_REGULAREXP ) ){
			/* CJreクラスの初期化 */
			CJre	cJre;
//			int		wJreVersion;
			cJre.Init();
			if( FALSE == CJre::IsExist() ){
				/* JRE32.DLLのバージョン */
				::SetDlgItemText( m_hWnd, IDC_STATIC_JRE32VER, "" );

				::MessageBeep( MB_ICONEXCLAMATION );
				::MessageBox( m_hWnd, "jre32.dllが見つかりません。\n正規表現を利用するには、jre32.dllが必要です。\n", "情報", MB_OK | MB_ICONEXCLAMATION );
				::CheckDlgButton( m_hWnd, IDC_CHK_REGULAREXP, 0 );
			}else{
				CJre	cJre;
				int		wJreVersion;
				char	szMsg[256];
				cJre.Init();
				/* JRE32.DLLのバージョン */
				wJreVersion = cJre.GetVersion();
				wsprintf( szMsg, "jre32.dll Ver%x.%x", wJreVersion / 0x100, wJreVersion % 0x100 );
				::SetDlgItemText( m_hWnd, IDC_STATIC_JRE32VER, szMsg );

				/* 英大文字と英小文字を区別する */
				::CheckDlgButton( m_hWnd, IDC_CHK_LOHICASE, 1 );
				::EnableWindow( ::GetDlgItem( m_hWnd, IDC_CHK_LOHICASE ), FALSE );
			}
		}else{
			/* 英大文字と英小文字を区別する */
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_CHK_LOHICASE ), TRUE );
			::CheckDlgButton( m_hWnd, IDC_CHK_LOHICASE, 0 );
		}
		return TRUE;

	case IDC_BUTTON_FOLDER:
		/* フォルダ参照ボタン */
		{
			char	szFolder[MAX_PATH];
			/* 検索フォルダ */
			::GetDlgItemText( m_hWnd, IDC_COMBO_FOLDER, szFolder, _MAX_PATH - 1 );
			if( 0 == lstrlen( szFolder ) ){
				::GetCurrentDirectory( sizeof( szFolder ), szFolder );
			}
			if( SelectDir( m_hWnd, "検索するフォルダを選んでください", szFolder, szFolder ) ){
				::SetDlgItemText( m_hWnd, IDC_COMBO_FOLDER, szFolder );
			}
		}

		return TRUE;
	case IDOK:
		/* ダイアログデータの取得 */
		if( GetData() ){
//			::EndDialog( hwndDlg, TRUE );
			CloseDialog( TRUE );
		}
		return TRUE;
	case IDCANCEL:
//		::EndDialog( hwndDlg, FALSE );
		CloseDialog( FALSE );
		return TRUE;
	}	
	
	/* 基底クラスメンバ */
	return CDialog::OnBnClicked( wID );
}



/* ダイアログデータの設定 */
void CDlgGrep::SetData( void )
{
	int		i;
	HWND	hwndCombo;
//	char	szWorkPath[_MAX_PATH + 1];
//	m_hWnd = hwndDlg;	/* このダイアログのハンドル */

	m_pShareData = m_cShareData.GetShareData( NULL, NULL );

	/* 検索文字列 */
	::SetDlgItemText( m_hWnd, IDC_COMBO_TEXT, m_szText );
	hwndCombo = ::GetDlgItem( m_hWnd, IDC_COMBO_TEXT );
	for( i = 0; i < m_pShareData->m_nSEARCHKEYArrNum; ++i ){
		::SendMessage( hwndCombo, CB_ADDSTRING, 0, (LPARAM)m_pShareData->m_szSEARCHKEYArr[i] );
	}

	/* 検索ファイル */
	::SetDlgItemText( m_hWnd, IDC_COMBO_FILE, m_szFile );
	hwndCombo = ::GetDlgItem( m_hWnd, IDC_COMBO_FILE );
	for( i = 0; i < m_pShareData->m_nGREPFILEArrNum; ++i ){
		::SendMessage( hwndCombo, CB_ADDSTRING, 0, (LPARAM)m_pShareData->m_szGREPFILEArr[i] );
	}

	/* 検索フォルダ */
	::SetDlgItemText( m_hWnd, IDC_COMBO_FOLDER, m_szFolder );
	hwndCombo = ::GetDlgItem( m_hWnd, IDC_COMBO_FOLDER );
	for( i = 0; i < m_pShareData->m_nGREPFOLDERArrNum; ++i ){
		::SendMessage( hwndCombo, CB_ADDSTRING, 0, (LPARAM)m_pShareData->m_szGREPFOLDERArr[i] );
	}

	if( 0 == lstrlen( m_pShareData->m_szGREPFOLDERArr[0] ) &&
		0 < lstrlen( m_szCurrentFilePath )
	){
		char	szWorkFolder[MAX_PATH];
		char	szWorkFile[MAX_PATH];
		SplitPath_FolderAndFile( m_szCurrentFilePath, szWorkFolder, szWorkFile );
		::SetDlgItemText( m_hWnd, IDC_COMBO_FOLDER, szWorkFolder );
	}

	/* この編集中のテキストから検索する */
	::CheckDlgButton( m_hWnd, IDC_CHK_FROMTHISTEXT, m_bFromThisText );
	if( 0 < lstrlen( m_szCurrentFilePath ) ){
		if( m_bFromThisText ){
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_COMBO_FILE ), FALSE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_COMBO_FOLDER ), FALSE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_BUTTON_FOLDER ), FALSE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_CHK_SUBFOLDER ), FALSE );
			char	szWorkFolder[MAX_PATH];
			char	szWorkFile[MAX_PATH];
			SplitPath_FolderAndFile( m_szCurrentFilePath, szWorkFolder, szWorkFile );
			::SetDlgItemText( m_hWnd, IDC_COMBO_FOLDER, szWorkFolder );
			::SetDlgItemText( m_hWnd, IDC_COMBO_FILE, szWorkFile );
		}else{
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_COMBO_FILE ), TRUE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_COMBO_FOLDER ), TRUE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_BUTTON_FOLDER ), TRUE );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_CHK_SUBFOLDER ), TRUE );
		}
	}

	/* サブフォルダからも検索する */
	::CheckDlgButton( m_hWnd, IDC_CHK_SUBFOLDER, m_bSubFolder );

	/* 英大文字と英小文字を区別する */
	::CheckDlgButton( m_hWnd, IDC_CHK_LOHICASE, m_bLoHiCase );
//	/* 一致する単語のみ検索する */
//	::CheckDlgButton( m_hWnd, IDC_CHK_WORDONLY, m_bWordOnly );
	/* 正規表現 */
	::CheckDlgButton( m_hWnd, IDC_CHK_REGULAREXP, m_bRegularExp );

	/* 文字コード自動判別 */
	::CheckDlgButton( m_hWnd, IDC_CHK_KANJICODEAUTODETECT, m_bKanjiCode_AutoDetect );


	/* 行を出力するか該当部分だけ出力するか */
	if( m_bGrepOutputLine ){
		::CheckDlgButton( m_hWnd, IDC_RADIO_OUTPUTLINE, TRUE );
	}else{
		::CheckDlgButton( m_hWnd, IDC_RADIO_OUTPUTMARKED, TRUE );
	}

	/* Grep: 出力形式 */
	if( 1 == m_nGrepOutputStyle ){
		::CheckDlgButton( m_hWnd, IDC_RADIO_OUTPUTSTYLE1, TRUE );
	}else
	if( 2 == m_nGrepOutputStyle ){
		::CheckDlgButton( m_hWnd, IDC_RADIO_OUTPUTSTYLE2, TRUE );
	}else{
		::CheckDlgButton( m_hWnd, IDC_RADIO_OUTPUTSTYLE1, TRUE );
	}

	if( m_bRegularExp ){
		/* CJreクラスの初期化 */
		CJre	cJre;
		cJre.Init();
		if( FALSE == cJre.IsExist() ){
			::MessageBeep( MB_ICONEXCLAMATION );
			::MessageBox( m_hWnd, "jre32.dllが見つかりません。\n正規表現を利用するには、jre32.dllが必要です。\n", "情報", MB_OK | MB_ICONEXCLAMATION );
			::CheckDlgButton( m_hWnd, IDC_CHK_REGULAREXP, 0 );
		}else{
			/* JRE32.DLLのバージョン */
			int	wJreVersion;
			char	szMsg[256];

			wJreVersion = cJre.GetVersion();
			wsprintf( szMsg, "jre32.dll Ver%x.%x", wJreVersion / 0x100, wJreVersion % 0x100 );
			::SetDlgItemText( m_hWnd, IDC_STATIC_JRE32VER, szMsg );

			/* 英大文字と英小文字を区別する */
			::CheckDlgButton( m_hWnd, IDC_CHK_LOHICASE, 1 );
			::EnableWindow( ::GetDlgItem( m_hWnd, IDC_CHK_LOHICASE ), FALSE );
		}
	}

	if( 0 < lstrlen( m_szCurrentFilePath ) ){
		::EnableWindow( ::GetDlgItem( m_hWnd, IDC_CHK_FROMTHISTEXT ), TRUE ); 
	}else{
		::EnableWindow( ::GetDlgItem( m_hWnd, IDC_CHK_FROMTHISTEXT ), FALSE ); 
	}


	return;
}




/* ダイアログデータの取得 */
/* 　TRUE==正常　 FALSE==入力エラー  */
int CDlgGrep::GetData( void )
{
	int			i;
	int			j;
	CMemory*	pcmWork;
	CJre		cJre;

	m_pShareData = m_cShareData.GetShareData( NULL, NULL );

	/* サブフォルダからも検索する*/
	m_bSubFolder = ::IsDlgButtonChecked( m_hWnd, IDC_CHK_SUBFOLDER );

	m_pShareData->m_Common.m_bGrepSubFolder = m_bSubFolder;		/* Grep：サブフォルダも検索 */

	/* この編集中のテキストから検索する */
	m_bFromThisText = ::IsDlgButtonChecked( m_hWnd, IDC_CHK_FROMTHISTEXT );
	/* 英大文字と英小文字を区別する */
	m_bLoHiCase = ::IsDlgButtonChecked( m_hWnd, IDC_CHK_LOHICASE );
//	/* 一致する単語のみ検索する */
//	m_bWordOnly = ::IsDlgButtonChecked( m_hWnd, IDC_CHK_WORDONLY );
	/* 正規表現 */
	m_bRegularExp = ::IsDlgButtonChecked( m_hWnd, IDC_CHK_REGULAREXP );

	/* 文字コード自動判別 */
	m_bKanjiCode_AutoDetect = ::IsDlgButtonChecked( m_hWnd, IDC_CHK_KANJICODEAUTODETECT );


	/* 行を出力するか該当部分だけ出力するか */
	m_bGrepOutputLine = ::IsDlgButtonChecked( m_hWnd, IDC_RADIO_OUTPUTLINE );

	/* Grep: 出力形式 */
	if( TRUE == ::IsDlgButtonChecked( m_hWnd, IDC_RADIO_OUTPUTSTYLE1 ) ){
		m_nGrepOutputStyle = 1;				/* Grep: 出力形式 */
	}
	if( TRUE == ::IsDlgButtonChecked( m_hWnd, IDC_RADIO_OUTPUTSTYLE2 ) ){
		m_nGrepOutputStyle = 2;				/* Grep: 出力形式 */
	}



	/* 検索文字列 */
	::GetDlgItemText( m_hWnd, IDC_COMBO_TEXT, m_szText, _MAX_PATH - 1 );
	/* 検索ファイル */
	::GetDlgItemText( m_hWnd, IDC_COMBO_FILE, m_szFile, _MAX_PATH - 1 );
	/* 検索フォルダ */
	::GetDlgItemText( m_hWnd, IDC_COMBO_FOLDER, m_szFolder, _MAX_PATH - 1 );

	m_pShareData->m_Common.m_bRegularExp = m_bRegularExp;	/* 1==正規表現 */
	m_pShareData->m_Common.m_bGrepKanjiCode_AutoDetect = m_bKanjiCode_AutoDetect;	/* 文字コード自動判別 */
	m_pShareData->m_Common.m_bLoHiCase = m_bLoHiCase;		/* 1==英大文字小文字の区別 */
	m_pShareData->m_Common.m_bGrepOutputLine = m_bGrepOutputLine;		/* 行を出力するか該当部分だけ出力するか */
	m_pShareData->m_Common.m_nGrepOutputStyle = m_nGrepOutputStyle;		/* Grep: 出力形式 */



//	m_pShareData->m_bWordOnly = m_bWordOnly;		/* 1==単語のみ検索 */


//やめました
//	if( 0 == lstrlen( m_szText ) ){
//		::MYMESSAGEBOX(	m_hWnd,	MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
//			"検索のキーワードを指定してください。"
//		);
//		return FALSE;
//	}
	/* この編集中のテキストから検索する */
	if( 0 == lstrlen( m_szFile ) ){
		::MYMESSAGEBOX(	m_hWnd,	MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
			"検索対象ファイルを指定してください。"
		);
		return FALSE;
	}
	if( 0 == lstrlen( m_szFolder ) ){
		::MYMESSAGEBOX(	m_hWnd,	MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
			"検索対象フォルダを指定してください。"
		);
		return FALSE;
	}

	char szCurDirOld[MAX_PATH];
	::GetCurrentDirectory( MAX_PATH, szCurDirOld );
	/* 相対パス→絶対パス */
	if( 0 == ::SetCurrentDirectory( m_szFolder ) ){
		::MYMESSAGEBOX(	m_hWnd,	MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
			"検索対象フォルダが正しくありません。"
		);
		::SetCurrentDirectory( szCurDirOld );
		return FALSE;
	}
	::GetCurrentDirectory( MAX_PATH, m_szFolder );
	::SetCurrentDirectory( szCurDirOld );

	
//	if( 0 < lstrlen( m_szText ) ){
		/* 正規表現？ */
		if( m_bRegularExp ){
			/* CJreクラスの初期化 */
			cJre.Init();
			/* jre32.dllの存在チェック */
			if( FALSE == cJre.IsExist() ){
				::MessageBox( m_hWnd, "jre32.dllが見つかりません。\n正規表現を利用するには、jre32.dllが必要です。\n", "情報", MB_OK | MB_ICONEXCLAMATION );
				return FALSE;
			}
			/* 検索パターンのコンパイル */
			if( !cJre.Compile( m_szText ) ){
				char*	pszMsg = new char[512];
				cJre.GetJreMessage( GJM_JPN, pszMsg );
				::MessageBox( m_hWnd, pszMsg, "正規表現エラー情報", MB_OK | MB_ICONEXCLAMATION );
				delete [] pszMsg;
				return FALSE;
			}
		}

		/* 検索文字列 */
		if( 0 < lstrlen( m_szText ) ){
			pcmWork = new CMemory( m_szText, lstrlen( m_szText ) );
			for( i = 0; i < m_pShareData->m_nSEARCHKEYArrNum; ++i ){
				if( 0 == strcmp( m_szText, m_pShareData->m_szSEARCHKEYArr[i] ) ){
					break;
				}
			}
			if( i < m_pShareData->m_nSEARCHKEYArrNum ){
				for( j = i; j > 0; j-- ){
					strcpy( m_pShareData->m_szSEARCHKEYArr[j], m_pShareData->m_szSEARCHKEYArr[j - 1] ); 
				}
			}else{
				for( j = MAX_SEARCHKEY - 1; j > 0; j-- ){
					strcpy( m_pShareData->m_szSEARCHKEYArr[j], m_pShareData->m_szSEARCHKEYArr[j - 1] ); 
				}
				++m_pShareData->m_nSEARCHKEYArrNum;
				if( m_pShareData->m_nSEARCHKEYArrNum > MAX_SEARCHKEY ){
					m_pShareData->m_nSEARCHKEYArrNum = MAX_SEARCHKEY;
				}
			}
			strcpy( m_pShareData->m_szSEARCHKEYArr[0], pcmWork->GetPtr( NULL ) );
			delete pcmWork;
		}

		/* 検索ファイル */
		pcmWork = new CMemory( m_szFile, lstrlen( m_szFile ) );
		for( i = 0; i < m_pShareData->m_nGREPFILEArrNum; ++i ){
			if( 0 == strcmp( m_szFile, m_pShareData->m_szGREPFILEArr[i] ) ){
				break;
			}
		}
		if( i < m_pShareData->m_nGREPFILEArrNum ){
			for( j = i; j > 0; j-- ){
				strcpy( m_pShareData->m_szGREPFILEArr[j], m_pShareData->m_szGREPFILEArr[j - 1] ); 
			}
		}else{
			for( j = MAX_GREPFILE - 1; j > 0; j-- ){
				strcpy( m_pShareData->m_szGREPFILEArr[j], m_pShareData->m_szGREPFILEArr[j - 1] ); 
			}
			++m_pShareData->m_nGREPFILEArrNum;
			if( m_pShareData->m_nGREPFILEArrNum > MAX_GREPFILE ){
				m_pShareData->m_nGREPFILEArrNum = MAX_GREPFILE;
			}
		}
		strcpy( m_pShareData->m_szGREPFILEArr[0], pcmWork->GetPtr( NULL ) );
		delete pcmWork;

		/* 検索フォルダ */
		pcmWork = new CMemory( m_szFolder, lstrlen( m_szFolder ) );
		for( i = 0; i < m_pShareData->m_nGREPFOLDERArrNum; ++i ){
			if( 0 == strcmp( m_szFolder, m_pShareData->m_szGREPFOLDERArr[i] ) ){
				break;
			}
		}
		if( i < m_pShareData->m_nGREPFOLDERArrNum ){
			for( j = i; j > 0; j-- ){
				strcpy( m_pShareData->m_szGREPFOLDERArr[j], m_pShareData->m_szGREPFOLDERArr[j - 1] ); 
			}
		}else{
			for( j = MAX_GREPFOLDER - 1; j > 0; j-- ){
				strcpy( m_pShareData->m_szGREPFOLDERArr[j], m_pShareData->m_szGREPFOLDERArr[j - 1] ); 
			}
			++m_pShareData->m_nGREPFOLDERArrNum;
			if( m_pShareData->m_nGREPFOLDERArrNum > MAX_GREPFOLDER ){
				m_pShareData->m_nGREPFOLDERArrNum = MAX_GREPFOLDER;
			}
		}
		strcpy( m_pShareData->m_szGREPFOLDERArr[0], pcmWork->GetPtr( NULL ) );
		delete pcmWork;
//	}
	return TRUE;
}

/*[EOF]*/
