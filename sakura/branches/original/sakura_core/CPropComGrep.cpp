//	$Id$
//	Copyright (C) 1998-2000, Norio Nakatani

#include "CPropCommon.h"





/* メッセージ処理 */
BOOL CPropCommon::DispatchEvent_PROP_GREP( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
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
		::SetWindowLong( hwndDlg, DWL_USER, (LONG)lParam );

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
			}
			break;
//		}
		break;
	}
	return FALSE;
}


/* ダイアログデータの設定 */
void CPropCommon::SetData_PROP_GREP( HWND hwndDlg )
{
//	BOOL	bRet;

//	BOOL				m_bGrepExitConfirm;	/* Grepモードで保存確認するか */


	/* Grepモードで保存確認するか */
	::CheckDlgButton( hwndDlg, IDC_CHECK_bGrepExitConfirm, m_Common.m_bGrepExitConfirm );

	/* Grepモード: エンターキーでタグジャンプ */
	::CheckDlgButton( hwndDlg, IDC_CHECK_GTJW_RETURN, m_Common.m_bGTJW_RETURN );

	/* Grepモード: ダブルクリックでタグジャンプ */
	::CheckDlgButton( hwndDlg, IDC_CHECK_GTJW_LDBLCLK, m_Common.m_bGTJW_LDBLCLK );

	return;
}








/* ダイアログデータの取得 */
int CPropCommon::GetData_PROP_GREP( HWND hwndDlg )
{
	m_nPageNum = ID_PAGENUM_GREP;



	/* Grepモードで保存確認するか */
	m_Common.m_bGrepExitConfirm = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_bGrepExitConfirm );
	
	/* Grepモード: エンターキーでタグジャンプ */
	m_Common.m_bGTJW_RETURN = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_GTJW_RETURN );

	/* Grepモード: ダブルクリックでタグジャンプ */
	m_Common.m_bGTJW_LDBLCLK = ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_GTJW_LDBLCLK );

	return TRUE;
}


