/*! @file
	@brief 共通設定ダイアログボックス、「書式」ページ

	@author Norio Nakatani
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000-2001, genta, MIK, jepro
	Copyright (C) 2002, YAZAKI, MIK, aroka, hor
	Copyright (C) 2004, genta, ryoji
	Copyright (C) 2006, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/

#include "StdAfx.h"
#include "CPropCommon.h"
#include "CPropTypes.h"
#include "Debug.h" // 2002/2/10 aroka
#include "sakura.hh"


static const DWORD p_helpids[] = {	//01310
	IDC_COMBO_FILESHAREMODE,				HIDC_COMBO_FILESHAREMODE,				//排他制御
	IDC_CHECK_bCheckFileTimeStamp,			HIDC_CHECK_bCheckFileTimeStamp,			//更新の監視
	IDC_CHECK_ENABLEUNMODIFIEDOVERWRITE,	HIDC_CHECK_ENABLEUNMODIFIEDOVERWRITE,	//無変更でも上書き
	IDC_CHECK_AUTOSAVE,						HIDC_CHECK_AUTOSAVE,					//自動的に保存
	IDC_CHECK_bDropFileAndClose,			HIDC_CHECK_bDropFileAndClose,			//閉じて開く
	IDC_CHECK_RestoreCurPosition,			HIDC_CHECK_RestoreCurPosition,			//カーソル位置の復元
	IDC_CHECK_AutoMIMEDecode,				HIDC_CHECK_AutoMIMEDecode,				//MIMEデコード
	IDC_EDIT_AUTOBACKUP_INTERVAL,			HIDC_EDIT_AUTOBACKUP_INTERVAL,			//自動保存間隔
	IDC_EDIT_nDropFileNumMax,				HIDC_EDIT_nDropFileNumMax,				//ファイルドロップ最大数
	IDC_SPIN_AUTOBACKUP_INTERVAL,			HIDC_EDIT_AUTOBACKUP_INTERVAL,
	IDC_SPIN_nDropFileNumMax,				HIDC_EDIT_nDropFileNumMax,
	IDC_CHECK_RestoreBookmarks,				HIDC_CHECK_RestoreBookmarks,			// 2002.01.16 hor ブックマークの復元
	IDC_CHECK_QueryIfCodeChange,			HIDC_CHECK_QueryIfCodeChange,			//前回と異なる文字コードのとき問い合わせを行う	// 2006.08.06 ryoji
	IDC_CHECK_AlertIfFileNotExist,			HIDC_CHECK_AlertIfFileNotExist,			//開こうとしたファイルが存在しないとき警告する	// 2006.08.06 ryoji
	IDC_CHECK_ALERT_IF_LARGEFILE,			HIDC_CHECK_ALERT_IF_LARGEFILE,			//開こうとしたファイルが大きい場合に警告する
	IDC_CHECK_NoFilterSaveNew,				HIDC_CHECK_NoFilterSaveNew,				// 新規から保存時は全ファイル表示	// 2006.11.16 ryoji
	IDC_CHECK_NoFilterSaveFile,				HIDC_CHECK_NoFilterSaveFile,			// 新規以外から保存時は全ファイル表示	// 2006.11.16 ryoji
//	IDC_STATIC,								-1,
	0, 0
};

TYPE_NAME<EShareMode> ShareModeArr[] = {
	{ SHAREMODE_NOT_EXCLUSIVE,	_T("しない") },
	{ SHAREMODE_DENY_WRITE,		_T("上書きを禁止する") },
	{ SHAREMODE_DENY_READWRITE,	_T("読み書きを禁止する") },
};

//	From Here Jun. 2, 2001 genta
/*!
	@param hwndDlg ダイアログボックスのWindow Handle
	@param uMsg メッセージ
	@param wParam パラメータ1
	@param lParam パラメータ2
*/
INT_PTR CALLBACK CPropFile::DlgProc_page(
	HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return DlgProc( reinterpret_cast<pDispatchPage>(&CPropFile::DispatchEvent), hwndDlg, uMsg, wParam, lParam );
}
//	To Here Jun. 2, 2001 genta

/*! ファイルページ メッセージ処理 */
INT_PTR CPropFile::DispatchEvent(
	HWND	hwndDlg,	//!< handle to dialog box
	UINT	uMsg,	//!< message
	WPARAM	wParam,	//!< first message parameter
	LPARAM	lParam 	//!< second message parameter
)
{
	WORD		wNotifyCode;
	WORD		wID;
	NMHDR*		pNMHDR;
	NM_UPDOWN*	pMNUD;
	int			idCtrl;
//	int			nVal;
	int			nVal;	//Sept.21, 2000 JEPRO スピン要素を加えたので復活させた
//	char		szFolder[_MAX_PATH];

	switch( uMsg ){
	case WM_INITDIALOG:
		/* ダイアログデータの設定 File */
		SetData( hwndDlg );
		::SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

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
//				OnHelp( hwndDlg, IDD_PROP_FILE );
//				return TRUE;
//			case PSN_KILLACTIVE:
////				MYTRACE( _T("p2 PSN_KILLACTIVE\n") );
//				/* ダイアログデータの取得 p2 */
//				GetData_p2( hwndDlg );
//				return TRUE;
//			}
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
				OnHelp( hwndDlg, IDD_PROP_FILE );
				return TRUE;
			case PSN_KILLACTIVE:
//				MYTRACE( _T("File PSN_KILLACTIVE\n") );
				/* ダイアログデータの取得 File */
				GetData( hwndDlg );
				return TRUE;
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
			case PSN_SETACTIVE:
				m_nPageNum = ID_PROPCOM_PAGENUM_FILE;
				return TRUE;
			}
		break;
		case IDC_SPIN_nDropFileNumMax:
			/* 一度にドロップ可能なファイル数 */
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_nDropFileNumMax, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 1 ){
				nVal = 1;
			}
			if( nVal > 99 ){
				nVal = 99;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_nDropFileNumMax, nVal, FALSE );
			return TRUE;
//@@@ 2001.03.21 Start by MIK
			/*NOTREACHED*/
//			break;
		case IDC_SPIN_AUTOBACKUP_INTERVAL:
			/* バックアップ間隔 */
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_AUTOBACKUP_INTERVAL, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 1 ){
				nVal = 1;
			}
			if( nVal > 35791 ){
				nVal = 35791;
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_AUTOBACKUP_INTERVAL, nVal, FALSE );
			return TRUE;
		case IDC_SPIN_ALERT_FILESIZE:
			/* ファイルの警告サイズ */
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_ALERT_FILESIZE, NULL, FALSE );
			if( pMNUD->iDelta < 0 ){
				++nVal;
			}else 
			if( pMNUD->iDelta > 0 ){
				--nVal;
			}
			if( nVal < 1 ){
				nVal = 1;
			}
			if( nVal > 2048 ){
				nVal = 2048;  // 最大 2GB まで
			}
			::SetDlgItemInt( hwndDlg, IDC_EDIT_ALERT_FILESIZE, nVal, FALSE );
			return TRUE;
			/*NOTREACHED*/
//			break;
//@@@ 2001.03.21 End by MIK
		}
//****	To Here Sept. 21, 2000 JEPRO ダイアログ要素にスピンを入れるのでWM_NOTIFYをコメントアウトにしその下に修正を置いた
		break;

	case WM_COMMAND:
		wNotifyCode	= HIWORD(wParam);	/* 通知コード */
		wID			= LOWORD(wParam);	/* 項目ID､ コントロールID､ またはアクセラレータID */

		if( wID == IDC_COMBO_FILESHAREMODE && wNotifyCode == CBN_SELCHANGE ){	// コンボボックスの選択変更
			EnableFilePropInput(hwndDlg);
			break;
		}

		switch( wNotifyCode ){
		/* ボタン／チェックボックスがクリックされた */
		case BN_CLICKED:
			switch( wID ){
			case IDC_CHECK_bDropFileAndClose:/* ファイルをドロップしたときは閉じて開く */
			case IDC_CHECK_AUTOSAVE:
			case IDC_CHECK_ALERT_IF_LARGEFILE:
				EnableFilePropInput(hwndDlg);
				break;
			}
			break;
		}
		break;

//@@@ 2001.02.04 Start by MIK: Popup Help
	case WM_HELP:
		{
			HELPINFO *p = (HELPINFO *)lParam;
			MyWinHelp( (HWND)p->hItemHandle, HELP_WM_HELP, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		}
		return TRUE;
		/*NOTREACHED*/
//		break;
//@@@ 2001.02.04 End

//@@@ 2001.12.22 Start by MIK: Context Menu Help
	//Context Menu
	case WM_CONTEXTMENU:
		MyWinHelp( hwndDlg, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		return TRUE;
//@@@ 2001.12.22 End

	}
	return FALSE;
}





/*! ファイルページ: ダイアログデータの設定
	共有メモリからデータを読み出して各コントロールに値を設定する。

	@par バックアップ世代数が妥当な値かどうかのチェックも行う。不適切な値の時は
	最も近い適切な値を設定する。

	@param hwndDlg プロパティページのWindow Handle
*/
void CPropFile::SetData( HWND hwndDlg )
{
	/*--- File ---*/
	/* ファイルの排他制御モード */
	HWND	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_FILESHAREMODE );
	::SendMessage(hwndCombo, CB_RESETCONTENT, 0L, 0L);
	int		nSelPos = 0;
	for( int i = 0; i < _countof( ShareModeArr ); ++i ){
		::SendMessage(hwndCombo, CB_INSERTSTRING, (WPARAM)i, (LPARAM)ShareModeArr[i].pszName);
		if( ShareModeArr[i].nMethod == m_Common.m_sFile.m_nFileShareMode ){
			nSelPos = i;
		}
	}
	::SendMessage(hwndCombo, CB_SETCURSEL, (WPARAM)nSelPos, 0L);

	/* 更新の監視 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_bCheckFileTimeStamp, m_Common.m_sFile.m_bCheckFileTimeStamp ? BST_CHECKED : BST_UNCHECKED );

	/* 無変更でも上書きするか */
	::CheckDlgButton( hwndDlg, IDC_CHECK_ENABLEUNMODIFIEDOVERWRITE, m_Common.m_sFile.m_bEnableUnmodifiedOverwrite ? BST_CHECKED : BST_UNCHECKED );

	/* ファイルをドロップしたときは閉じて開く */
	::CheckDlgButton( hwndDlg, IDC_CHECK_bDropFileAndClose, m_Common.m_sFile.m_bDropFileAndClose ? BST_CHECKED : BST_UNCHECKED );
	/* 一度にドロップ可能なファイル数 */
	::SetDlgItemInt( hwndDlg, IDC_EDIT_nDropFileNumMax, m_Common.m_sFile.m_nDropFileNumMax, FALSE );

	//	From Here Aug. 21, 2000 genta
	//	自動保存の有効・無効
	::CheckDlgButton( hwndDlg, IDC_CHECK_AUTOSAVE, m_Common.m_sBackup.IsAutoBackupEnabled() );

	TCHAR buf[6];
	int nN;

	nN = m_Common.m_sBackup.GetAutoBackupInterval();
	nN = nN < 1  ?  1 : nN;
	nN = nN > 35791 ? 35791 : nN;

	wsprintf( buf, _T("%d"), nN);
	::SetDlgItemText( hwndDlg, IDC_EDIT_AUTOBACKUP_INTERVAL, buf );
	//	To Here Aug. 21, 2000 genta

	//	Oct. 27, 2000 genta	カーソル位置復元フラグ
	::CheckDlgButton( hwndDlg, IDC_CHECK_RestoreCurPosition, m_Common.m_sFile.GetRestoreCurPosition() );
	// 2002.01.16 hor ブックマーク復元フラグ
	::CheckDlgButton( hwndDlg, IDC_CHECK_RestoreBookmarks, m_Common.m_sFile.GetRestoreBookmarks() );
	//	Nov. 12, 2000 genta	MIME Decodeフラグ
	::CheckDlgButton( hwndDlg, IDC_CHECK_AutoMIMEDecode, m_Common.m_sFile.GetAutoMIMEdecode() );
	//	Oct. 03, 2004 genta 前回と異なる文字コードのときに問い合わせを行うかどうかのフラグ
	::CheckDlgButton( hwndDlg, IDC_CHECK_QueryIfCodeChange, m_Common.m_sFile.GetQueryIfCodeChange() );
	//	Oct. 09, 2004 genta 開こうとしたファイルが存在しないとき警告するかどうかのフラグ
	::CheckDlgButton( hwndDlg, IDC_CHECK_AlertIfFileNotExist, m_Common.m_sFile.GetAlertIfFileNotExist() );
	//	ファイルサイズが大きい場合に警告を出す
	::CheckDlgButton( hwndDlg, IDC_CHECK_ALERT_IF_LARGEFILE, m_Common.m_sFile.m_bAlertIfLargeFile );
	::SetDlgItemInt( hwndDlg, IDC_EDIT_ALERT_FILESIZE, m_Common.m_sFile.m_nAlertFileSize, FALSE );

	// ファイル保存ダイアログのフィルタ設定	// 2006.11.16 ryoji
	::CheckDlgButton( hwndDlg, IDC_CHECK_NoFilterSaveNew, m_Common.m_sFile.m_bNoFilterSaveNew ? BST_CHECKED : BST_UNCHECKED );	// 新規から保存時は全ファイル表示
	::CheckDlgButton( hwndDlg, IDC_CHECK_NoFilterSaveFile, m_Common.m_sFile.m_bNoFilterSaveFile ? BST_CHECKED : BST_UNCHECKED );	// 新規以外から保存時は全ファイル表示

	EnableFilePropInput(hwndDlg);
	return;
}

/*! ファイルページ ダイアログデータの取得
	ダイアログボックスに設定されたデータを共有メモリに反映させる

	@par バックアップ世代数が妥当な値かどうかのチェックも行う。不適切な値の時は
	最も近い適切な値を設定する。

	@param hwndDlg プロパティページのWindow Handle
	@return 常にTRUE
*/
int CPropFile::GetData( HWND hwndDlg )
{
	/* ファイルの排他制御モード */
	HWND	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_FILESHAREMODE );
	int		nSelPos = ::SendMessage(hwndCombo, CB_GETCURSEL, 0L, 0L);
	m_Common.m_sFile.m_nFileShareMode = ShareModeArr[nSelPos].nMethod;

	/* 更新の監視 */
	m_Common.m_sFile.m_bCheckFileTimeStamp = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_bCheckFileTimeStamp ) != 0;

	/* 無変更でも上書きするか */
	m_Common.m_sFile.m_bEnableUnmodifiedOverwrite = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_ENABLEUNMODIFIEDOVERWRITE ) != 0;

	/* ファイルをドロップしたときは閉じて開く */
	m_Common.m_sFile.m_bDropFileAndClose = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_bDropFileAndClose ) != 0;
	/* 一度にドロップ可能なファイル数 */
	m_Common.m_sFile.m_nDropFileNumMax = ::GetDlgItemInt( hwndDlg, IDC_EDIT_nDropFileNumMax, NULL, FALSE );
	if( 1 > m_Common.m_sFile.m_nDropFileNumMax ){
		m_Common.m_sFile.m_nDropFileNumMax = 1;
	}
	if( 99 < m_Common.m_sFile.m_nDropFileNumMax ){	//Sept. 21, 2000, JEPRO 16より大きいときに99と制限されていたのを修正(16→99と変更)
		m_Common.m_sFile.m_nDropFileNumMax = 99;
	}

	//	From Here Aug. 16, 2000 genta
	//	自動保存を行うかどうか
	m_Common.m_sBackup.EnableAutoBackup( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_AUTOSAVE ) != FALSE );

	//	自動保存間隔の取得
	TCHAR szNumBuf[/*6*/ 7];	//@@@ 2001.03.21 by MIK
	int	 nN;
	TCHAR *pDigit;

	::GetDlgItemText( hwndDlg, IDC_EDIT_AUTOBACKUP_INTERVAL, szNumBuf, /*5*/ 6 );	//@@@ 2001.03.21 by MIK

	for( nN = 0, pDigit = szNumBuf; *pDigit != _T('\0'); pDigit++ ){
		if( _T('0') <= *pDigit && *pDigit <= _T('9') ){
			nN = nN * 10 + *pDigit - _T('0');
		}
		else
			break;
	}
	nN = nN < 1  ?  1 : nN;
	nN = nN > 35791 ? 35791 : nN;
	m_Common.m_sBackup.SetAutoBackupInterval( nN );

	//	To Here Aug. 16, 2000 genta

	//	Oct. 27, 2000 genta	カーソル位置復元フラグ
	m_Common.m_sFile.SetRestoreCurPosition( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_RestoreCurPosition ) != FALSE );
	// 2002.01.16 hor ブックマーク復元フラグ
	m_Common.m_sFile.SetRestoreBookmarks( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_RestoreBookmarks ) != FALSE );
	//	Nov. 12, 2000 genta	MIME Decodeフラグ
	m_Common.m_sFile.SetAutoMIMEdecode( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_AutoMIMEDecode ) != FALSE );
	//	Oct. 03, 2004 genta 前回と異なる文字コードのときに問い合わせを行うかどうかのフラグ
	m_Common.m_sFile.SetQueryIfCodeChange( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_QueryIfCodeChange ) != FALSE );
	//	Oct. 03, 2004 genta 前回と異なる文字コードのときに問い合わせを行うかどうかのフラグ
	m_Common.m_sFile.SetAlertIfFileNotExist( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_AlertIfFileNotExist ) != FALSE );
	// 開こうとしたファイルが大きい場合に警告する
	m_Common.m_sFile.m_bAlertIfLargeFile = (::IsDlgButtonChecked( hwndDlg, IDC_CHECK_ALERT_IF_LARGEFILE ) != FALSE );
	m_Common.m_sFile.m_nAlertFileSize = ::GetDlgItemInt( hwndDlg, IDC_EDIT_ALERT_FILESIZE, NULL, FALSE );
	if( m_Common.m_sFile.m_nAlertFileSize < 1 ){
		m_Common.m_sFile.m_nAlertFileSize = 1;
	}
	if( m_Common.m_sFile.m_nAlertFileSize > 2048 ){
		m_Common.m_sFile.m_nAlertFileSize = 2048;
	}

	// ファイル保存ダイアログのフィルタ設定	// 2006.11.16 ryoji
	m_Common.m_sFile.m_bNoFilterSaveNew = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_NoFilterSaveNew ) != 0;	// 新規から保存時は全ファイル表示
	m_Common.m_sFile.m_bNoFilterSaveFile = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_NoFilterSaveFile ) != 0;	// 新規以外から保存時は全ファイル表示

	return TRUE;
}

//	From Here Aug. 21, 2000 genta
/*!	チェック状態に応じてダイアログボックス要素のEnable/Disableを
	適切に設定する

	@param hwndDlg プロパティシートのWindow Handle
*/
void CPropFile::EnableFilePropInput(HWND hwndDlg)
{

	//	Drop時の動作
	if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_bDropFileAndClose ) ){
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_AUTOSAVE3 ), FALSE );	// added Sept. 6, JEPRO 自動保存にしたときだけEnableになるように変更
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_AUTOSAVE4 ), FALSE );	// added Sept. 6, JEPRO	同上
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_nDropFileNumMax ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_nDropFileNumMax ), FALSE );// added Oct. 6, JEPRO ファイルオープンを「閉じて開く」にしたときはDisableになるように変更
	}else{
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_AUTOSAVE3 ), TRUE );	// added Sept. 6, JEPRO	同上
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_AUTOSAVE4 ), TRUE );	// added Sept. 6, JEPRO	同上
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_nDropFileNumMax ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_nDropFileNumMax ), TRUE );// added Oct. 6, JEPRO ファイルオープンを「複数ファイルドロップ」にしたときだけEnableになるように変更
	}

	//	排他するかどうか
	int nSelPos = ::SendMessage(::GetDlgItem( hwndDlg, IDC_COMBO_FILESHAREMODE), CB_GETCURSEL, 0L, 0L);
	if( ShareModeArr[nSelPos].nMethod == SHAREMODE_NOT_EXCLUSIVE ){
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_CHECK_bCheckFileTimeStamp ), TRUE );
	}else{
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_CHECK_bCheckFileTimeStamp ), FALSE );
	}

	//	自動保存
	if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_AUTOSAVE ) ){
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_AUTOBACKUP_INTERVAL ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_AUTOSAVE ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_AUTOSAVE2 ), TRUE );	//Sept. 6, 2000 JEPRO 自動保存にしたときだけEnableになるように変更
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_AUTOBACKUP_INTERVAL ), TRUE );	//@@@ 2001.03.21 by MIK
	}else{
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_AUTOBACKUP_INTERVAL ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_AUTOSAVE ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_LABEL_AUTOSAVE2 ), FALSE );	//Sept. 6, 2000 JEPRO 同上
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_AUTOBACKUP_INTERVAL ), FALSE );	//@@@ 2001.03.21 by MIK
	}

	// 「開こうとしたファイルが大きい場合に警告を出す」
	if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_ALERT_IF_LARGEFILE ) ){
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_ALERT_FILESIZE ), TRUE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_ALERT_FILESIZE ), TRUE );
	}else{
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_EDIT_ALERT_FILESIZE ), FALSE );
		::EnableWindow( ::GetDlgItem( hwndDlg, IDC_SPIN_ALERT_FILESIZE ), FALSE );
	}
}
//	To Here Aug. 21, 2000 genta


/*[EOF]*/
