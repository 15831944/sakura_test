/*!	@file
	@brief 共通設定ダイアログボックス、「バックアップ」ページ

	@author Norio Nakatani
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, genta, jepro
	Copyright (C) 2001, MIK, asa-o, genta, jepro
	Copyright (C) 2002, MIK, YAZAKI, genta, Moca
	Copyright (C) 2003, KEITA
	Copyright (C) 2004, genta
	Copyright (C) 2005, genta, aroka
	Copyright (C) 2006, ryoji
	Copyright (C) 2009, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/
/* LMP (Lucien Murray-Pitts) : 2011-02-26 Added Basic English Translation Resources */

#include "stdafx.h"
#include "CPropCommon.h"

#include "etc_uty.h"

//@@@ 2001.02.04 Start by MIK: Popup Help
#if 1	//@@@ 2002.01.03 MIK
#include "sakura.hh"
static const DWORD p_helpids[] = {	//10000
	IDC_BUTTON_BACKUP_FOLDER_REF,	HIDC_BUTTON_BACKUP_FOLDER_REF,	//バックアップフォルダ参照
	IDC_CHECK_BACKUP,				HIDC_CHECK_BACKUP,				//バックアップの作成
	IDC_CHECK_BACKUP_YEAR,			HIDC_CHECK_BACKUP_YEAR,			//バックアップファイル名（西暦年）
	IDC_CHECK_BACKUP_MONTH,			HIDC_CHECK_BACKUP_MONTH,		//バックアップファイル名（月）
	IDC_CHECK_BACKUP_DAY,			HIDC_CHECK_BACKUP_DAY,			//バックアップファイル名（日）
	IDC_CHECK_BACKUP_HOUR,			HIDC_CHECK_BACKUP_HOUR,			//バックアップファイル名（時）
	IDC_CHECK_BACKUP_MIN,			HIDC_CHECK_BACKUP_MIN,			//バックアップファイル名（分）
	IDC_CHECK_BACKUP_SEC,			HIDC_CHECK_BACKUP_SEC,			//バックアップファイル名（秒）
	IDC_CHECK_BACKUPDIALOG,			HIDC_CHECK_BACKUPDIALOG,		//作成前に確認
	IDC_CHECK_BACKUPFOLDER,			HIDC_CHECK_BACKUPFOLDER,		//指定フォルダに作成
	IDC_CHECK_BACKUP_DUSTBOX,		HIDC_CHECK_BACKUP_DUSTBOX,		//バックアップファイルをごみ箱に放り込む	//@@@ 2001.12.11 add MIK
	IDC_EDIT_BACKUPFOLDER,			HIDC_EDIT_BACKUPFOLDER,			//保存フォルダ名
	IDC_EDIT_BACKUP_3,				HIDC_EDIT_BACKUP_3,				//世代数
	IDC_RADIO_BACKUP_TYPE1,			HIDC_RADIO_BACKUP_TYPE1,		//バックアップの種類（拡張子）
//	IDC_RADIO_BACKUP_TYPE2,			HIDC_RADIO_BACKUP_TYPE2NEWHID,		//バックアップの種類（日付・時刻） // 2002.11.09 Moca HIDが.._TYPE3と逆だった	// Jun.  5, 2004 genta 廃止
	IDC_RADIO_BACKUP_TYPE3,			HIDC_RADIO_BACKUP_TYPE3NEWHID,		//バックアップの種類（連番）// 2002.11.09 Moca HIDが.._TYPE2と逆だった
	IDC_RADIO_BACKUP_DATETYPE1,		HIDC_RADIO_BACKUP_DATETYPE1,	//付加する日時の種類（作成日時）	//Jul. 05, 2001 JEPRO 追加
	IDC_RADIO_BACKUP_DATETYPE2,		HIDC_RADIO_BACKUP_DATETYPE2,	//付加する日時の種類（更新日時）	//Jul. 05, 2001 JEPRO 追加
	IDC_SPIN_BACKUP_GENS,			HIDC_EDIT_BACKUP_3,				//保存する世代数のスピン
	IDC_CHECK_BACKUP_RETAINEXT,		HIDC_CHECK_BACKUP_RETAINEXT,	//元の拡張子を保存	// 2006.08.06 ryoji
	IDC_CHECK_BACKUP_ADVANCED,		HIDC_CHECK_BACKUP_ADVANCED,		//詳細設定	// 2006.08.06 ryoji
	IDC_EDIT_BACKUPFILE,			HIDC_EDIT_BACKUPFILE,			//詳細設定のエディットボックス	// 2006.08.06 ryoji
	IDC_RADIO_BACKUP_DATETYPE1A,	HIDC_RADIO_BACKUP_DATETYPE1A,	//付加する日時の種類（作成日時）※詳細設定ON用	// 2009.02.20 ryoji
	IDC_RADIO_BACKUP_DATETYPE2A,	HIDC_RADIO_BACKUP_DATETYPE2A,	//付加する日時の種類（更新日時）※詳細設定ON用	// 2009.02.20 ryoji
//	IDC_STATIC,						-1,
	0, 0
};
#else
static const DWORD p_helpids[] = {	//10000
	IDC_BUTTON_BACKUP_FOLDER_REF,	10000,	//バックアップフォルダ参照
	IDC_CHECK_BACKUP,				10010,	//バックアップの作成
	IDC_CHECK_BACKUP_YEAR,			10011,	//バックアップファイル名（西暦年）
	IDC_CHECK_BACKUP_MONTH,			10012,	//バックアップファイル名（月）
	IDC_CHECK_BACKUP_DAY,			10013,	//バックアップファイル名（日）
	IDC_CHECK_BACKUP_HOUR,			10014,	//バックアップファイル名（時）
	IDC_CHECK_BACKUP_MIN,			10015,	//バックアップファイル名（分）
	IDC_CHECK_BACKUP_SEC,			10016,	//バックアップファイル名（秒）
	IDC_CHECK_BACKUPDIALOG,			10017,	//作成前に確認
	IDC_CHECK_BACKUPFOLDER,			10018,	//指定フォルダに作成
	IDC_CHECK_BACKUP_DUSTBOX,		10019,	//バックアップファイルをごみ箱に放り込む	//@@@ 2001.12.11 add MIK
	IDC_EDIT_BACKUPFOLDER,			10040,	//保存フォルダ名
	IDC_EDIT_BACKUP_3,				10041,	//世代数
	IDC_RADIO_BACKUP_TYPE1,			10060,	//バックアップの種類（拡張子）
//	IDC_RADIO_BACKUP_TYPE2,			10062,	//バックアップの種類（日付・時刻	// Jun.  5, 2004 genta 廃止
	IDC_RADIO_BACKUP_TYPE3,			10061,	//バックアップの種類（連番）
	IDC_RADIO_BACKUP_DATETYPE1,		10063,	//付加する日時の種類（作成日時）	//Jul. 05, 2001 JEPRO 追加
	IDC_RADIO_BACKUP_DATETYPE2,		10064,	//付加する日時の種類（更新日時）	//Jul. 05, 2001 JEPRO 追加
	IDC_SPIN_BACKUP_GENS,			-1,
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
INT_PTR CALLBACK CPropCommon::DlgProc_PROP_BACKUP(
	HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return DlgProc( &CPropCommon::DispatchEvent_PROP_BACKUP, hwndDlg, uMsg, wParam, lParam );
}
//	To Here Jun. 2, 2001 genta


/* メッセージ処理 */
INT_PTR CPropCommon::DispatchEvent_PROP_BACKUP( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	WORD		wNotifyCode;
	WORD		wID;
	HWND		hwndCtl;
	NMHDR*		pNMHDR;
	NM_UPDOWN*	pMNUD;
	int			idCtrl;
//	int			nVal;
	int			nVal;	//Sept.21, 2000 JEPRO スピン要素を加えたので復活させた
	char		szFolder[_MAX_PATH];
//	int			nDummy;
//	int			nCharChars;

	switch( uMsg ){

	case WM_INITDIALOG:
		/* ダイアログデータの設定 p1 */
		SetData_PROP_BACKUP( hwndDlg );
		// Modified by KEITA for WIN64 2003.9.6
		::SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

		/* ユーザーがエディット コントロールに入力できるテキストの長さを制限する */
		//	Oct. 5, 2002 genta バックアップフォルダ名の入力サイズを指定
		//	Oct. 8, 2002 genta 最後に付加される\の領域を残すためバッファサイズ-1しか入力させない
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_BACKUPFOLDER ),  EM_LIMITTEXT, (WPARAM)( sizeof( m_Common.m_szBackUpFolder ) - 1 - 1 ), 0 );
		// 20051107 aroka
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_BACKUPFILE ),  EM_LIMITTEXT, (WPARAM)( sizeof( m_Common.m_szBackUpPathAdvanced ) - 1 - 1 ), 0 );
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
//				OnHelp( hwndDlg, IDD_PROP_BACKUP );
//				return TRUE;
//			case PSN_KILLACTIVE:
//				/* ダイアログデータの取得 p1 */
//				GetData_PROP_BACKUP( hwndDlg );
//				return TRUE;
//			}
//			break;
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
				OnHelp( hwndDlg, IDD_PROP_BACKUP );
				return TRUE;
			case PSN_KILLACTIVE:
				/* ダイアログデータの取得 p1 */
				GetData_PROP_BACKUP( hwndDlg );
				return TRUE;
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
			case PSN_SETACTIVE:
				m_nPageNum = ID_PAGENUM_BACKUP;	//Oct. 25, 2000 JEPRO ZENPAN1→ZENPAN に変更(参照しているのはCPropCommon.cppのみの1箇所)
				return TRUE;
			}
			break;

		case IDC_SPIN_BACKUP_GENS:
			/* バックアップファイルの世代数 */
			nVal = ::GetDlgItemInt( hwndDlg, IDC_EDIT_BACKUP_3, NULL, FALSE );
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
			::SetDlgItemInt( hwndDlg, IDC_EDIT_BACKUP_3, nVal, FALSE );
			return TRUE;
		}
//****	To Here Sept. 21, 2000 JEPRO ダイアログ要素にスピンを入れるので以下のWM_NOTIFYをコメントアウトにし下に修正を置いた
		break;

	case WM_COMMAND:
		wNotifyCode	= HIWORD(wParam);	/* 通知コード */
		wID			= LOWORD(wParam);	/* 項目ID､ コントロールID､ またはアクセラレータID */
		hwndCtl		= (HWND) lParam;	/* コントロールのハンドル */
		switch( wNotifyCode ){
		/* ボタン／チェックボックスがクリックされた */
		case BN_CLICKED:
			switch( wID ){
			case IDC_RADIO_BACKUP_TYPE1:
				//	Aug. 16, 2000 genta
				//	バックアップ方式追加
			case IDC_RADIO_BACKUP_TYPE3:
			case IDC_CHECK_BACKUP:
			case IDC_CHECK_BACKUPFOLDER:
				//	Aug. 21, 2000 genta
			case IDC_CHECK_AUTOSAVE:
			//	Jun.  5, 2004 genta IDC_RADIO_BACKUP_TYPE2を廃止して，
			//	IDC_RADIO_BACKUP_DATETYPE1, IDC_RADIO_BACKUP_DATETYPE2を同列に持ってきた
			case IDC_RADIO_BACKUP_DATETYPE1:
			case IDC_RADIO_BACKUP_DATETYPE2:
			// 20051107 aroka
			case IDC_CHECK_BACKUP_ADVANCED:
				GetData_PROP_BACKUP( hwndDlg );
				UpdateBackupFile( hwndDlg );
				EnableBackupInput(hwndDlg);
				return TRUE;
			case IDC_BUTTON_BACKUP_FOLDER_REF:	/* フォルダ参照 */
//				strcpy( szFolder, m_Common.m_szBackUpFolder );
				/* バックアップを作成するフォルダ */
				::GetDlgItemText( hwndDlg, IDC_EDIT_BACKUPFOLDER, szFolder, sizeof( szFolder ));

				// LMP: Added
				char _pszLabel[257];
				::LoadString( m_hInstance, STR_ERR_DLGPROPCOMBK1, _pszLabel, 255 );  // LMP: Added

				if( SelectDir( hwndDlg, _pszLabel /*"バックアップを作成するフォルダを選んでください"*/, (const char *)szFolder, (char *)szFolder ) ){
					strcpy( m_Common.m_szBackUpFolder, szFolder );
					::SetDlgItemText( hwndDlg, IDC_EDIT_BACKUPFOLDER, m_Common.m_szBackUpFolder );
				}
				UpdateBackupFile( hwndDlg );
				return TRUE;
			default: // 20051107 aroka Default節 追加
				GetData_PROP_BACKUP( hwndDlg );
				UpdateBackupFile( hwndDlg );
			}
			break;	/* BN_CLICKED */
		case EN_CHANGE: // 20051107 aroka フォルダが変更されたらリアルタイムにエディットボックス内を更新
			switch( wID ){
			case IDC_EDIT_BACKUPFOLDER:
				// 2009.02.21 ryoji 後ろに\が追加されるので，1文字余裕をみる必要がある．
				::GetDlgItemText( hwndDlg, IDC_EDIT_BACKUPFOLDER, m_Common.m_szBackUpFolder, sizeof( m_Common.m_szBackUpFolder ) - 1 );
				UpdateBackupFile( hwndDlg );
				break;
			}
			break;	/* EN_CHANGE */
		}
		break;	/* WM_COMMAND */

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


/*! ダイアログデータの設定
	@date 2004.06.05 genta 元の拡張子を残す設定を追加．
		日時指定でチェックボックスが空欄で残ると設定されない問題を避けるため，
		IDC_RADIO_BACKUP_TYPE2
		を廃止してレイアウト変更
*/
void CPropCommon::SetData_PROP_BACKUP( HWND hwndDlg )
{
//	BOOL	bRet;

//	BOOL				m_bGrepExitConfirm;	/* Grepモードで保存確認するか */


	/* バックアップの作成 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUP, m_Common.m_bBackUp );
	/* バックアップの作成前に確認 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUPDIALOG, m_Common.m_bBackUpDialog );
//	/* 指定フォルダにバックアップを作成する */ //	20051107 aroka 「バックアップの作成」に連動させる
//	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUPFOLDER, m_Common.m_bBackUpFolder );

	/* バックアップファイル名のタイプ 1=(.bak) 2=*_日付.* */
	//	Jun.  5, 2004 genta 元の拡張子を残す設定(5,6)を追加．
	switch( m_Common.GetBackupType() ){
	case 2:
		::CheckDlgButton( hwndDlg, IDC_RADIO_BACKUP_DATETYPE1, 1 );	// 付加する日付のタイプ(現時刻)
		break;
	case 3:
	case 6:
		::CheckDlgButton( hwndDlg, IDC_RADIO_BACKUP_TYPE3, 1 );
		break;
	case 4:
		::CheckDlgButton( hwndDlg, IDC_RADIO_BACKUP_DATETYPE2, 1 );	// 付加する日付のタイプ(前回の保存時刻)
		break;
	case 5:
	case 1:
	default:
		::CheckDlgButton( hwndDlg, IDC_RADIO_BACKUP_TYPE1, 1 );
		break;
	}
	
	//	Jun.  5, 2004 genta 元の拡張子を残す設定(5,6)を追加．
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUP_RETAINEXT,
		( m_Common.GetBackupType() == 5 || m_Common.GetBackupType() == 6 ) ? 1 : 0
	 );

	/* バックアップファイル名：日付の年 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUP_YEAR, m_Common.GetBackupOpt(BKUP_YEAR) );
	/* バックアップファイル名：日付の月 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUP_MONTH, m_Common.GetBackupOpt(BKUP_MONTH) );
	/* バックアップファイル名：日付の日 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUP_DAY, m_Common.GetBackupOpt(BKUP_DAY) );
	/* バックアップファイル名：日付の時 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUP_HOUR, m_Common.GetBackupOpt(BKUP_HOUR) );
	/* バックアップファイル名：日付の分 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUP_MIN, m_Common.GetBackupOpt(BKUP_MIN) );
	/* バックアップファイル名：日付の秒 */
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUP_SEC, m_Common.GetBackupOpt(BKUP_SEC) );

	/* 指定フォルダにバックアップを作成する */ // 20051107 aroka 移動：連動対象にする。
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUPFOLDER, m_Common.m_bBackUpFolder );

	/* バックアップを作成するフォルダ */
	::SetDlgItemText( hwndDlg, IDC_EDIT_BACKUPFOLDER, m_Common.m_szBackUpFolder );

	/* バックアップファイルをごみ箱に放り込む */	//@@@ 2001.12.11 add MIK
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUP_DUSTBOX, m_Common.m_bBackUpDustBox );	//@@@ 2001.12.11 add MIK

	/* バックアップ先フォルダを詳細設定する */ // 20051107 aroka
	::CheckDlgButton( hwndDlg, IDC_CHECK_BACKUP_ADVANCED, m_Common.m_bBackUpPathAdvanced );

	/* バックアップを作成するフォルダの詳細設定 */ // 20051107 aroka
	::SetDlgItemText( hwndDlg, IDC_EDIT_BACKUPFILE, m_Common.m_szBackUpPathAdvanced );

	/* バックアップを作成するフォルダの詳細設定 */ // 20051128 aroka
	switch( m_Common.GetBackupTypeAdv() ){
	case 2:
		::CheckDlgButton( hwndDlg, IDC_RADIO_BACKUP_DATETYPE1A, 1 );	// 付加する日付のタイプ(現時刻)
		break;
	case 4:
		::CheckDlgButton( hwndDlg, IDC_RADIO_BACKUP_DATETYPE2A, 1 );	// 付加する日付のタイプ(前回の保存時刻)
		break;
	default:
		::CheckDlgButton( hwndDlg, IDC_RADIO_BACKUP_DATETYPE1A, 1 );
		break;
	}

	//	From Here Aug. 16, 2000 genta
	int nN = m_Common.GetBackupCount();
	nN = nN < 1  ?  1 : nN;
	nN = nN > 99 ? 99 : nN;

	::SetDlgItemInt( hwndDlg, IDC_EDIT_BACKUP_3, nN, FALSE );	//	Oct. 29, 2001 genta
	//	To Here Aug. 16, 2000 genta

	UpdateBackupFile( hwndDlg );

	EnableBackupInput(hwndDlg);
	return;
}








/* ダイアログデータの取得 */
int CPropCommon::GetData_PROP_BACKUP( HWND hwndDlg )
{
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
//	m_nPageNum = ID_PAGENUM_BACKUP;

	/* バックアップの作成 */
	m_Common.m_bBackUp = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP );
	/* バックアップの作成前に確認 */
	m_Common.m_bBackUpDialog = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUPDIALOG );
//	/* 指定フォルダにバックアップを作成する */ // 20051107 aroka 「バックアップの作成」に連動させる
//	m_Common.m_bBackUpFolder = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUPFOLDER );


	/* バックアップファイル名のタイプ 1=(.bak) 2=*_日付.* */
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_TYPE1 ) ){
		//	Jun.  5, 2005 genta 拡張子を残すパターンを追加
		if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP_RETAINEXT )){
			m_Common.SetBackupType(5);
		}
		else {
			m_Common.SetBackupType(1);
		}
	}
//	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_TYPE2 ) ){
		// 2001/06/05 Start by asa-o: 日付のタイプ
		if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_DATETYPE1 ) ){
			m_Common.SetBackupType(2);	// 現時刻
		}
		if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_DATETYPE2 ) ){
			m_Common.SetBackupType(4);	// 前回の保存時刻
		}
		// 2001/06/05 End
//	}

	//	Aug. 16, 2000 genta
	//	3 = *.b??
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_TYPE3 ) ){
		if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP_RETAINEXT )){
			m_Common.SetBackupType(6);
		}
		else {
			m_Common.SetBackupType(3);
		}
	}

	/* バックアップファイル名：日付の年 */
	m_Common.SetBackupOpt(BKUP_YEAR, ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP_YEAR ) == BST_CHECKED);
	/* バックアップファイル名：日付の月 */
	m_Common.SetBackupOpt(BKUP_MONTH, ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP_MONTH ) == BST_CHECKED);
	/* バックアップファイル名：日付の日 */
	m_Common.SetBackupOpt(BKUP_DAY, ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP_DAY ) == BST_CHECKED);
	/* バックアップファイル名：日付の時 */
	m_Common.SetBackupOpt(BKUP_HOUR, ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP_HOUR ) == BST_CHECKED);
	/* バックアップファイル名：日付の分 */
	m_Common.SetBackupOpt(BKUP_MIN, ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP_MIN ) == BST_CHECKED);
	/* バックアップファイル名：日付の秒 */
	m_Common.SetBackupOpt(BKUP_SEC, ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP_SEC ) == BST_CHECKED);

	/* 指定フォルダにバックアップを作成する */ // 20051107 aroka 移動
	m_Common.m_bBackUpFolder = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUPFOLDER );

	/* バックアップを作成するフォルダ */
	//	Oct. 5, 2002 genta サイズをsizeof()で指定
	//	Oct. 8, 2002 genta 後ろに\が追加されるので，1文字余裕を見る必要がある．
	::GetDlgItemText( hwndDlg, IDC_EDIT_BACKUPFOLDER, m_Common.m_szBackUpFolder, sizeof( m_Common.m_szBackUpFolder ) - 1);

	/* バックアップファイルをごみ箱に放り込む */	//@@@ 2001.12.11 add MIK
	m_Common.m_bBackUpDustBox = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP_DUSTBOX );	//@@@ 2001.12.11 add MIK

	/* 指定フォルダにバックアップを作成する詳細設定 */ // 20051107 aroka
	m_Common.m_bBackUpPathAdvanced = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP_ADVANCED );
	/* バックアップを作成するフォルダ */ // 20051107 aroka
	::GetDlgItemText( hwndDlg, IDC_EDIT_BACKUPFILE, m_Common.m_szBackUpPathAdvanced, sizeof( m_Common.m_szBackUpPathAdvanced ) - 1);

	// 20051128 aroka 詳細設定の日付のタイプ
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_DATETYPE1A ) ){
		m_Common.SetBackupTypeAdv(2);	// 現時刻
	}
	if( ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_DATETYPE2A ) ){
		m_Common.SetBackupTypeAdv(4);	// 前回の保存時刻
	}

//	int nDummy;
//	int nCharChars;
//	nDummy = strlen( m_Common.m_szBackUpFolder );
//	if( 0 < nDummy ){
//		/* フォルダの最後が「半角かつ'\\'」でない場合は、付加する */
//		nCharChars = &m_Common.m_szBackUpFolder[nDummy] - CMemory::MemCharPrev( m_Common.m_szBackUpFolder, nDummy, &m_Common.m_szBackUpFolder[nDummy] );
//		if( 1 == nCharChars && m_Common.m_szBackUpFolder[nDummy - 1] == '\\' ){
//		}else{
//			strcat( m_Common.m_szBackUpFolder, "\\" );
//		}
//	}

	//	From Here Aug. 16, 2000 genta
	//	世代数の取得
//	char szNumBuf[6];
	int	 nN;
//	char *pDigit;
	nN = ::GetDlgItemInt( hwndDlg, IDC_EDIT_BACKUP_3, NULL, FALSE );	//	Oct. 29, 2001 genta

//	for( nN = 0, pDigit = szNumBuf; *pDigit != '\0'; pDigit++ ){
//		if( '0' <= *pDigit && *pDigit <= '9' ){
//			nN = nN * 10 + *pDigit - '0';
//		}
//		else
//			break;
//	}
	nN = nN < 1  ?  1 : nN;
	nN = nN > 99 ? 99 : nN;
	m_Common.SetBackupCount( nN );
	//	To Here Aug. 16, 2000 genta

	return TRUE;
}

//	From Here Aug. 16, 2000 genta
/*!	チェック状態に応じてダイアログボックス要素のEnable/Disableを
	適切に設定する

	@date 2004.06.05 genta 元の拡張子を残す設定を追加．
		日時指定でチェックボックスが空欄で残ると設定されない問題を避けるため，
		IDC_RADIO_BACKUP_TYPE2
		を廃止してレイアウト変更
	@date 2005.11.07 aroka レイアウトに合わせて順序を入れ替え、インデントを整理
	@date 2005.11.21 aroka 詳細設定モードの制御を追加
	@date 2009.02.20 ryoji IDC_LABEL_BACKUP_HELPによる別コントロール隠しを廃止、if文制御をShowEnableフラグ制御に置き換えて簡素化して下記問題修正。
	                       ・Vista Aeroだと詳細設定ONにしても詳細設定OFF項目が画面から消えない
	                       ・詳細設定OFF項目が非表示ではなかったので隠れていてもTooltipヘルプが表示される
	                       ・詳細設定ONなのにバックアップ作成OFFだと詳細設定OFF項目のほうが表示される
*/
static inline void ShowEnable(HWND hWnd, BOOL bShow, BOOL bEnable)
{
	::ShowWindow( hWnd, bShow? SW_SHOW: SW_HIDE );
	::EnableWindow( hWnd, bEnable );
}

void CPropCommon::EnableBackupInput(HWND hwndDlg)
{
	#define SHOWENABLE(id, show, enable) ShowEnable( ::GetDlgItem( hwndDlg, id ), show, enable )

	BOOL bBackup = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP );
	BOOL bAdvanced = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUP_ADVANCED );
	BOOL bType1 = ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_TYPE1 );
	//BOOL bType2 = ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_TYPE2 );
	BOOL bType3 = ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_TYPE3 );
	BOOL bDate1 = ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_DATETYPE1 );
	BOOL bDate2 = ::IsDlgButtonChecked( hwndDlg, IDC_RADIO_BACKUP_DATETYPE2 );
	BOOL bFolder = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_BACKUPFOLDER );

	SHOWENABLE( IDC_CHECK_BACKUP_ADVANCED,	TRUE, bBackup );	// 20050628 aroka
	SHOWENABLE( IDC_RADIO_BACKUP_TYPE1,		!bAdvanced, bBackup );
	SHOWENABLE( IDC_RADIO_BACKUP_TYPE3,		!bAdvanced, bBackup );
	SHOWENABLE( IDC_RADIO_BACKUP_DATETYPE1,	!bAdvanced, bBackup );
	SHOWENABLE( IDC_RADIO_BACKUP_DATETYPE2,	!bAdvanced, bBackup );
	SHOWENABLE( IDC_LABEL_BACKUP_3,			!bAdvanced, bBackup && bType3);
	SHOWENABLE( IDC_EDIT_BACKUP_3,			!bAdvanced, bBackup && bType3);
	SHOWENABLE( IDC_SPIN_BACKUP_GENS,		!bAdvanced, bBackup && bType3 );	//	20051107 aroka 追加
	SHOWENABLE( IDC_CHECK_BACKUP_RETAINEXT,	!bAdvanced, bBackup && (bType1 || bType3) );	//	Jun.  5, 2005 genta 追加
	SHOWENABLE( IDC_CHECK_BACKUP_YEAR,		!bAdvanced, bBackup && (bDate1 || bDate2) );
	SHOWENABLE( IDC_CHECK_BACKUP_MONTH,		!bAdvanced, bBackup && (bDate1 || bDate2) );
	SHOWENABLE( IDC_CHECK_BACKUP_DAY,		!bAdvanced, bBackup && (bDate1 || bDate2) );
	SHOWENABLE( IDC_CHECK_BACKUP_HOUR,		!bAdvanced, bBackup && (bDate1 || bDate2) );
	SHOWENABLE( IDC_CHECK_BACKUP_MIN,		!bAdvanced, bBackup && (bDate1 || bDate2) );
	SHOWENABLE( IDC_CHECK_BACKUP_SEC,		!bAdvanced, bBackup && (bDate1 || bDate2) );

	// 詳細設定
	SHOWENABLE( IDC_EDIT_BACKUPFILE,		TRUE, bBackup && bAdvanced );
//	SHOWENABLE( IDC_LABEL_BACKUP_HELP,		bAdvanced, bBackup );	// 不可視のまま放置（他コントロール隠しの方式は廃止） 2009.02.20 ryoji
	SHOWENABLE( IDC_LABEL_BACKUP_HELP2,		bAdvanced, bBackup );
	SHOWENABLE( IDC_RADIO_BACKUP_DATETYPE1A,	bAdvanced, bBackup );
	SHOWENABLE( IDC_RADIO_BACKUP_DATETYPE2A,	bAdvanced, bBackup );

	SHOWENABLE( IDC_CHECK_BACKUPFOLDER,		TRUE, bBackup );
	SHOWENABLE( IDC_LABEL_BACKUP_4,			TRUE, bBackup && bFolder );	// added Sept. 6, JEPRO フォルダ指定したときだけEnableになるように変更
	SHOWENABLE( IDC_EDIT_BACKUPFOLDER,		TRUE, bBackup && bFolder );
	SHOWENABLE( IDC_BUTTON_BACKUP_FOLDER_REF,	TRUE, bBackup && bFolder );
	SHOWENABLE( IDC_CHECK_BACKUP_DUSTBOX,	TRUE, bBackup );	//@@@ 2001.12.11 add MIK

	// 作成前に確認
	SHOWENABLE( IDC_CHECK_BACKUPDIALOG,		TRUE, bBackup );

	#undef SHOWENABLE
}
//	To Here Aug. 16, 2000 genta


/*!	バックアップファイルの詳細設定エディットボックスを適切に更新する

	@date 2005.11.07 aroka 新規追加

	@note 詳細設定切り替え時のデフォルトをオプションに合わせるため、
		m_szBackUpPathAdvanced を更新する
*/
void CPropCommon::UpdateBackupFile(HWND hwndDlg)	//	バックアップファイルの詳細設定
{
	char temp[MAX_PATH];
	/* バックアップを作成するファイル */ // 20051107 aroka
	if( !m_Common.m_bBackUp ){
		_tcscpy( temp, _T("") );
	}
	else{
		if( m_Common.m_bBackUpFolder ){
			_tcscpy( temp, _T("") );
		}
		else if( m_Common.m_bBackUpDustBox  ){
			::LoadString( m_hInstance, STR_ERR_DLGPROPCOMBK2, temp, MAX_PATH );  // LMP: Added
//			wsprintf( temp, _T("%s\\"), _T("(ゴミ箱)") );
		}
		else{
			wsprintf( temp, _T(".\\") );
		}

		switch( m_Common.GetBackupType() ){
		case 1: // .bak
			_tcscat( temp, _T("$0.bak") );
			break;
		case 5: // .*.bak
			_tcscat( temp, _T("$0.*.bak") );
			break;
		case 3: // .b??
			_tcscat( temp, _T("$0.b??") );
			break;
		case 6: // .*.b??
			_tcscat( temp, _T("$0.*.b??") );
			break;
		case 2:	//	日付，時刻
		case 4:	//	日付，時刻
			_tcscat( temp, _T("$0_") );

			if( m_Common.GetBackupOpt(BKUP_YEAR) ){	/* バックアップファイル名：日付の年 */
				_tcscat( temp, _T("%Y") );
			}
			if( m_Common.GetBackupOpt(BKUP_MONTH) ){	/* バックアップファイル名：日付の月 */
				_tcscat( temp, _T("%m") );
			}
			if( m_Common.GetBackupOpt(BKUP_DAY) ){	/* バックアップファイル名：日付の日 */
				_tcscat( temp, _T("%d") );
			}
			if( m_Common.GetBackupOpt(BKUP_HOUR) ){	/* バックアップファイル名：日付の時 */
				_tcscat( temp, _T("%H") );
			}
			if( m_Common.GetBackupOpt(BKUP_MIN) ){	/* バックアップファイル名：日付の分 */
				_tcscat( temp, _T("%M") );
			}
			if( m_Common.GetBackupOpt(BKUP_SEC) ){	/* バックアップファイル名：日付の秒 */
				_tcscat( temp, _T("%S") );
			}

			_tcscat( temp, _T(".*") );
			break;
		default:
			break;
		}
	}
	if( !m_Common.m_bBackUpPathAdvanced ){	// 詳細設定モードでないときだけ自動更新する
		wsprintf( m_Common.m_szBackUpPathAdvanced, _T("%s"), temp );
		::SetDlgItemText( hwndDlg, IDC_EDIT_BACKUPFILE, m_Common.m_szBackUpPathAdvanced );
	}
	return;
}
/*[EOF]*/
