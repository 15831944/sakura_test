//	$Id$
//	Copyright (C) 1998-2000, Norio Nakatani


#include "CPropCommon.h"
#include "CDlgOpenFile.h"
#include "etc_uty.h"

//	From Here Sept. 5, 2000 JEPRO 半角カタカナの全角化に伴い文字長を変更(21→34)
#define STR_KEYDATA_HEAD_LEN  34
//	To Here Sept. 5, 2000
#define STR_KEYDATA_HEAD      "テキストエディタ キー設定ファイル\x1a"





/* From Here Oct. 13, 2000 Studio CでMr.コーヒー氏に教わったやり方ですがうまくいってません */
// ウィンドウプロシージャの中で・・・
LRESULT CALLBACK CPropComKeybindWndProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg ) {
	// WM_CTLCOLORSTATIC メッセージに対して
	case WM_CTLCOLORSTATIC:
	// 白色のブラシハンドルを返す
		return (LRESULT)GetStockObject(WHITE_BRUSH);
//	default:
//		break;
	}
	return 0;
}
/* To Here Oct. 13, 2000 */







/* p5 メッセージ処理 */
BOOL CPropCommon::DispatchEvent_p5(
    HWND	hwndDlg,	// handle to dialog box
    UINT	uMsg,	// message
    WPARAM	wParam,	// first message parameter
    LPARAM	lParam 	// second message parameter
)
{
	WORD		wNotifyCode;
	WORD		wID;
	HWND		hwndCtl;
	NMHDR*		pNMHDR;
	NM_UPDOWN*	pMNUD;
	int			idCtrl;
	static HWND	hwndCombo;
	static HWND	hwndFuncList;
	static HWND	hwndKeyList;
	static HWND	hwndCheckShift;
	static HWND	hwndCheckCtrl;
	static HWND	hwndCheckAlt;
	static HWND	hwndAssignedkeyList;
//	static HWND hwndLIST_KEYSFUNC;
	static HWND hwndEDIT_KEYSFUNC;
	CMemory**	ppcAssignedKeyList;
	int			nLength;
	int			nAssignedKeyNum;
	const char*	cpszString;

	int			nIndex;
	int			nIndex2;
	int			nIndex3;
	int			i;
	int			j;
//	int			nNum;
	int			nFuncCode;
	static char pszLabel[256];
	static char	szKeyState[64];
	int			nIndexTop;

	switch( uMsg ){
	case WM_INITDIALOG:
		/* ダイアログデータの設定 p5 */
		SetData_p5( hwndDlg );
		::SetWindowLong( hwndDlg, DWL_USER, (LONG)lParam );

		/* コントロールのハンドルを取得 */
		hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_FUNCKIND );
		hwndFuncList = ::GetDlgItem( hwndDlg, IDC_LIST_FUNC );
		hwndAssignedkeyList = ::GetDlgItem( hwndDlg, IDC_LIST_ASSIGNEDKEYS );
		hwndCheckShift = ::GetDlgItem( hwndDlg, IDC_CHECK_SHIFT );
		hwndCheckCtrl = ::GetDlgItem( hwndDlg, IDC_CHECK_CTRL );
		hwndCheckAlt = ::GetDlgItem( hwndDlg, IDC_CHECK_ALT );
		hwndKeyList = ::GetDlgItem( hwndDlg, IDC_LIST_KEY );
//		hwndLIST_KEYSFUNC = ::GetDlgItem( hwndDlg, IDC_LIST_KEYSFUNC );
		hwndEDIT_KEYSFUNC = ::GetDlgItem( hwndDlg, IDC_EDIT_KEYSFUNC );

		/* キー選択時の処理 */
//	From Here Oct. 14, 2000 JEPRO わかりにくいので選択しないように変更	//Oct. 17, 2000 JEPRO 復活！
//	/* キーリストの先頭の項目を選択（リストボックス）*/
		::SendMessage( hwndKeyList, LB_SETCURSEL, (WPARAM)0, (LPARAM)0 );	//Oct. 14, 2000 JEPRO ここをコメントアウトすると先頭項目が選択されなくなる
		::SendMessage( hwndDlg, WM_COMMAND, MAKELONG( IDC_LIST_KEY, LBN_SELCHANGE ), (LPARAM)hwndKeyList );	//Oct. 14, 2000 JEPRO ここはどっちでもいい？(わからん)
//	To Here Oct. 14, 2000
		::SendMessage( hwndDlg, WM_COMMAND, MAKELONG( IDC_COMBO_FUNCKIND, CBN_SELCHANGE ), (LPARAM)hwndCombo );

//		/* コンボボックスのユーザー インターフェイスを拡張インターフェースにする */
//		::SendMessage( hwndCombo, CB_SETEXTENDEDUI, (WPARAM) (BOOL) TRUE, 0 );

		return TRUE;

	case WM_NOTIFY:
		idCtrl = (int)wParam;
		pNMHDR = (NMHDR*)lParam;
		pMNUD  = (NM_UPDOWN*)lParam;
		switch( pNMHDR->code ){
		case PSN_HELP:
//			OnHelp( hwndDlg, IDD_PROP1P5 );		// Sept. 9, 2000 JEPRO 実際のID名に変更
			OnHelp( hwndDlg, IDD_PROP_KEYBIND );
			return TRUE;
		case PSN_KILLACTIVE:
//			MYTRACE( "p5 PSN_KILLACTIVE\n" );
			/* ダイアログデータの取得 p5 */
			GetData_p5( hwndDlg );
			return TRUE;
		}
		break;

	case WM_COMMAND:
		wNotifyCode = HIWORD(wParam);	/* 通知コード */
		wID = LOWORD(wParam);	/* 項目ID､ コントロールID､ またはアクセラレータID */
		hwndCtl = (HWND) lParam;	/* コントロールのハンドル */

		switch( wNotifyCode ){
		/* ボタン／チェックボックスがクリックされた */
		case BN_CLICKED:
			switch( wID ){
			case IDC_BUTTON_IMPORT:	/* インポート */
				/* p5:キー割り当て設定をインポートする */
				p5_Import_KeySetting( hwndDlg );
				return TRUE;
			case IDC_BUTTON_EXPORT:	/* エクスポート */
				/* p5:キー割り当て設定をエクスポートする */
				p5_Export_KeySetting( hwndDlg );
				return TRUE;
			case IDC_BUTTON_ASSIGN:	/* 割付 */
				nIndex = ::SendMessage( hwndKeyList, LB_GETCURSEL, 0, 0 );
				nIndex2 = ::SendMessage( hwndCombo, CB_GETCURSEL, 0, 0 );
				nIndex3 = ::SendMessage( hwndFuncList, LB_GETCURSEL, 0, 0 );
				nFuncCode = (nsFuncCode::ppnFuncListArr[nIndex2])[nIndex3];
				i = 0;
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_SHIFT ) ){
					i |= _SHIFT;
				}
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_CTRL ) ){
					i |= _CTRL;
				}
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_ALT ) ){
					i |= _ALT;
				}
				m_pKeyNameArr[nIndex].m_nFuncCodeArr[i] = nFuncCode;
				::SendMessage( hwndDlg, WM_COMMAND, MAKELONG( IDC_LIST_KEY, LBN_SELCHANGE ), (LPARAM)hwndKeyList );
				::SendMessage( hwndDlg, WM_COMMAND, MAKELONG( IDC_LIST_FUNC, LBN_SELCHANGE ), (LPARAM)hwndFuncList );
				return TRUE;
			case IDC_BUTTON_RELEASE:	/* 解除 */
				nIndex = ::SendMessage( hwndKeyList, LB_GETCURSEL, 0, 0 );
				nIndex2 = ::SendMessage( hwndCombo, CB_GETCURSEL, 0, 0 );
				nIndex3 = ::SendMessage( hwndFuncList, LB_GETCURSEL, 0, 0 );
				nFuncCode = 0;
				i = 0;
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_SHIFT ) ){
					i |= _SHIFT;
				}
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_CTRL ) ){
					i |= _CTRL;
				}
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_ALT ) ){
					i |= _ALT;
				}
				m_pKeyNameArr[nIndex].m_nFuncCodeArr[i] = nFuncCode;
				::SendMessage( hwndDlg, WM_COMMAND, MAKELONG( IDC_LIST_KEY, LBN_SELCHANGE ), (LPARAM)hwndKeyList );
				::SendMessage( hwndDlg, WM_COMMAND, MAKELONG( IDC_LIST_FUNC, LBN_SELCHANGE ), (LPARAM)hwndFuncList );
				return TRUE;
			}
		}
		if( hwndCheckShift == hwndCtl 
		 || hwndCheckCtrl == hwndCtl 
		 || hwndCheckAlt == hwndCtl 
		){
			switch( wNotifyCode ){
			case BN_CLICKED:
				nIndex = ::SendMessage( hwndKeyList, LB_GETCURSEL, 0, 0 );
				nIndexTop = ::SendMessage( hwndKeyList, LB_GETTOPINDEX, 0, 0 );
				strcpy( szKeyState, "" );
				i = 0;
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_SHIFT ) ){
					i |= _SHIFT;
					strcat( szKeyState, "Shift+" );
				}
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_CTRL ) ){
					i |= _CTRL;
					strcat( szKeyState, "Ctrl+" );
				}
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_ALT ) ){
					i |= _ALT;
					strcat( szKeyState, "Alt+" );
				}
				/* キー一覧に文字列をセット（リストボックス）*/
				::SendMessage( hwndKeyList, LB_RESETCONTENT, 0, 0 );
				for( i = 0; i < m_nKeyNameArrNum; ++i ){
					wsprintf( pszLabel, "%s%s", szKeyState, m_pKeyNameArr[i].m_szKeyName );
					::SendMessage( hwndKeyList, LB_ADDSTRING, 0, (LPARAM)pszLabel );
				}
				::SendMessage( hwndKeyList, LB_SETCURSEL, nIndex, 0 );
				::SendMessage( hwndKeyList, LB_SETTOPINDEX, nIndexTop, 0 );
				::SendMessage( hwndDlg, WM_COMMAND, MAKELONG( IDC_LIST_KEY, LBN_SELCHANGE ), (LPARAM)hwndKeyList );
				return TRUE;
			}
		}else
		if( hwndKeyList == hwndCtl ){
			switch( wNotifyCode ){
			case LBN_SELCHANGE:
				nIndex = ::SendMessage( hwndKeyList, LB_GETCURSEL, 0, 0 );
				i = 0;
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_SHIFT ) ){
					i |= _SHIFT;
				}
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_CTRL ) ){
					i |= _CTRL;
				}
				if( ::IsDlgButtonChecked( hwndDlg, IDC_CHECK_ALT ) ){
					i |= _ALT;
				}
				nFuncCode = m_pKeyNameArr[nIndex].m_nFuncCodeArr[i];
				if( 0 < ::LoadString( m_hInstance, nFuncCode, pszLabel, 255 )  ){
					::SetWindowText( hwndEDIT_KEYSFUNC, pszLabel );
				}else{
					::SetWindowText( hwndEDIT_KEYSFUNC, "--不明--" );
				}
				return TRUE;
			}
		}else 
		if( hwndFuncList == hwndCtl ){
			switch( wNotifyCode ){
			case LBN_SELCHANGE:
				nIndex = ::SendMessage( hwndKeyList, LB_GETCURSEL, 0, 0 );
				nIndex2 = ::SendMessage( hwndCombo, CB_GETCURSEL, 0, 0 );
				nIndex3 = ::SendMessage( hwndFuncList, LB_GETCURSEL, 0, 0 );
				nFuncCode = (nsFuncCode::ppnFuncListArr[nIndex2])[nIndex3];
				/* 機能に対応するキー名の取得(複数) */
				nAssignedKeyNum =  CKeyBind::GetKeyStrList( m_hInstance, m_nKeyNameArrNum, (KEYDATA*)m_pKeyNameArr, &ppcAssignedKeyList, nFuncCode );	/* 機能に対応するキー名の取得(複数) */
				/* 割り当てキーリストをクリアして値の設定 */
				::SendMessage( hwndAssignedkeyList, LB_RESETCONTENT, 0, 0 );
				if( 0 < nAssignedKeyNum){
					for( j = 0; j < nAssignedKeyNum; ++j ){
						/* デバッグモニタに出力 */
						cpszString = ppcAssignedKeyList[j]->GetPtr( &nLength );
						::SendMessage( hwndAssignedkeyList, LB_ADDSTRING, 0, (LPARAM)cpszString );
						delete ppcAssignedKeyList[j];
					}
					delete [] ppcAssignedKeyList;
				}
				return TRUE;
			}
		}else 
		if( hwndCombo == hwndCtl){
			switch( wNotifyCode ){
			case CBN_SELCHANGE:
				nIndex = ::SendMessage( hwndKeyList, LB_GETCURSEL, 0, 0 );
				nIndex2 = ::SendMessage( hwndCombo, CB_GETCURSEL, 0, 0 );
				/* 機能一覧に文字列をセット（リストボックス）*/
				::SendMessage( hwndFuncList, LB_RESETCONTENT, 0, 0 );
				for( i = 0; i < nsFuncCode::pnFuncListNumArr[nIndex2]; ++i ){
					if( 0 < ::LoadString( m_hInstance, (nsFuncCode::ppnFuncListArr[nIndex2])[i], pszLabel, 255 ) ){
						::SendMessage( hwndFuncList, LB_ADDSTRING, 0, (LPARAM)pszLabel );
					}else{
						::SendMessage( hwndFuncList, LB_ADDSTRING, 0, (LPARAM)"--未定義--" );
					}
				}

//	From Here Sept. 7, 2000 JEPRO わかりにくいので選択しないように変更
//				::SendMessage( hwndFuncList, LB_SETCURSEL, (WPARAM)0, 0 );
//	To Here Sept. 7, 2000 
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}







/* ダイアログデータの設定 p5 */
void CPropCommon::SetData_p5( HWND hwndDlg )
{
    HWND		hwndCombo;
    HWND		hwndKeyList;
	int			i;

	/* 機能種別一覧に文字列をセット（コンボボックス）*/
	hwndCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_FUNCKIND );
	for( i = 0; i < nsFuncCode::nFuncKindNum; ++i ){
		::SendMessage( hwndCombo, CB_ADDSTRING, 0, (LPARAM)nsFuncCode::ppszFuncKind[i] );
	}
	/* 種別の先頭の項目を選択（コンボボックス）*/
	::SendMessage( hwndCombo, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );	//Oct. 14, 2000 JEPRO JEPRO 「--未定義--」を表示させないように大元 Funcode.cpp で変更してある
	/* キー一覧に文字列をセット（リストボックス）*/
	hwndKeyList = ::GetDlgItem( hwndDlg, IDC_LIST_KEY );
	for( i = 0; i < m_nKeyNameArrNum; ++i ){
			::SendMessage( hwndKeyList, LB_ADDSTRING, 0, (LPARAM)m_pKeyNameArr[i].m_szKeyName );
	}
	return;
}



/* ダイアログデータの取得 p5 */
int CPropCommon::GetData_p5( HWND hwndDlg )
{
	m_nPageNum = ID_PAGENUM_KEYBOARD;
	return TRUE;
}



/* p5:キー割り当て設定をインポートする */
void CPropCommon::p5_Import_KeySetting( HWND hwndDlg )
{
	CDlgOpenFile	cDlgOpenFile;
	char*			pszMRU = NULL;;
	char*			pszOPENFOLDER = NULL;;
	char			szPath[_MAX_PATH + 1];
	HFILE			hFile;
//	char			szLine[1024];
//	int				i;
	
	char			pHeader[STR_KEYDATA_HEAD_LEN + 1];
	short			nKeyNameArrNum;				/* キー割り当て表の有効データ数 */
	KEYDATA			pKeyNameArr[100];				/* キー割り当て表 */
	HWND			hwndCtrl;
	char			szInitDir[_MAX_PATH + 1];

	strcpy( szPath, "" );
	strcpy( szInitDir, m_pShareData->m_szIMPORTFOLDER );	/* インポート用フォルダ */
	/* ファイルオープンダイアログの初期化 */
	cDlgOpenFile.Create( 
		m_hInstance, 
		hwndDlg, 
		"*.key", 
		szInitDir, 
		(const char **)&pszMRU, 
		(const char **)&pszOPENFOLDER
	);
	if( !cDlgOpenFile.DoModal_GetOpenFileName( szPath ) ){
		return;
	}
	/* ファイルのフルパスを、フォルダとファイル名に分割 */
	/* [c:\work\test\aaa.txt] → [c:\work\test] + [aaa.txt] */
	::SplitPath_FolderAndFile( szPath, m_pShareData->m_szIMPORTFOLDER, NULL );
	strcat( m_pShareData->m_szIMPORTFOLDER, "\\" );

	hFile = _lopen( szPath, OF_READ );
	if( HFILE_ERROR == hFile ){
		::MYMESSAGEBOX(	hwndDlg, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"ファイルを開けませんでした。\n\n%s", szPath
		);
		return;
	}
	if( STR_KEYDATA_HEAD_LEN     != _lread( hFile, pHeader, STR_KEYDATA_HEAD_LEN ) ||
		sizeof( nKeyNameArrNum ) != _lread( hFile, &nKeyNameArrNum, sizeof( nKeyNameArrNum ) ) || 
		sizeof( pKeyNameArr    ) != _lread( hFile,  pKeyNameArr   , sizeof( pKeyNameArr    ) ) ||
		0 != memcmp( pHeader, STR_KEYDATA_HEAD, STR_KEYDATA_HEAD_LEN )
	){
		::MYMESSAGEBOX(	hwndDlg, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"キー設定ファイルの形式が違います。\n\n%s", szPath
		);
		return;
	}
	_lclose( hFile );

	/* データのコピー */
	m_nKeyNameArrNum = nKeyNameArrNum;
	memcpy( m_pKeyNameArr, pKeyNameArr, sizeof( pKeyNameArr ) );

//	CShareData::SetKeyNames( m_pShareData );	/* キー名称のセット */


//	/* ダイアログデータの設定 p5 */
//	SetData_p5( hwndDlg );
	hwndCtrl = ::GetDlgItem( hwndDlg, IDC_LIST_KEY );
	::SendMessage( hwndDlg, WM_COMMAND, MAKELONG( IDC_LIST_KEY, LBN_SELCHANGE ), (LPARAM)hwndCtrl );
	
	return;
}


/* p5:キー割り当て設定をエクスポートする */
void CPropCommon::p5_Export_KeySetting( HWND hwndDlg )
{
	CDlgOpenFile	cDlgOpenFile;
	char*			pszMRU = NULL;;
	char*			pszOPENFOLDER = NULL;;
	char			szPath[_MAX_PATH + 1];
	HFILE			hFile;
//	char			szLine[1024];
//	int				i;
//	char			pHeader[STR_KEYDATA_HEAD_LEN + 1];
//	short			nKeyNameArrNum;				/* キー割り当て表の有効データ数 */
//	KEYDATA			pKeyNameArr[100];				/* キー割り当て表 */
	char			szInitDir[_MAX_PATH + 1];

	strcpy( szPath, "" );
	strcpy( szInitDir, m_pShareData->m_szIMPORTFOLDER );	/* インポート用フォルダ */
	/* ファイルオープンダイアログの初期化 */
	cDlgOpenFile.Create( 
		m_hInstance, 
		hwndDlg, 
		"*.key", 
		szInitDir, 
		(const char **)&pszMRU, 
		(const char **)&pszOPENFOLDER
	);
	if( !cDlgOpenFile.DoModal_GetSaveFileName( szPath ) ){
		return;
	}
	/* ファイルのフルパスを、フォルダとファイル名に分割 */
	/* [c:\work\test\aaa.txt] → [c:\work\test] + [aaa.txt] */
	::SplitPath_FolderAndFile( szPath, m_pShareData->m_szIMPORTFOLDER, NULL );
	strcat( m_pShareData->m_szIMPORTFOLDER, "\\" );

	hFile = _lcreat( szPath, 0 );
	if( HFILE_ERROR == hFile ){
		::MYMESSAGEBOX(	hwndDlg, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"ファイルを開けませんでした。\n\n%s", szPath
		);
		return;
	}
	if( STR_KEYDATA_HEAD_LEN     != _lwrite( hFile, (LPCSTR)STR_KEYDATA_HEAD, STR_KEYDATA_HEAD_LEN ) ||
		sizeof( m_nKeyNameArrNum ) != _lwrite( hFile, (LPCSTR)&m_nKeyNameArrNum, sizeof( m_nKeyNameArrNum ) ) || 
		sizeof( m_pKeyNameArr    ) != _lwrite( hFile, (LPCSTR) m_pKeyNameArr   , sizeof( m_pKeyNameArr    ) )
	){
		::MYMESSAGEBOX(	hwndDlg, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"ファイルの書き込みに失敗しました。\n\n%s", szPath
		);
		return;
	}
	_lclose( hFile );

	return;
}


