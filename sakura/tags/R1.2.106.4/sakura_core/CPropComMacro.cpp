//	$Id$
/*!	@file
	共通設定ダイアログボックス、「マクロ」ページ

	@author genta
	@date Jun. 2, 2001 genta
	$Revision$

*/
/*
	Copyright (C) 2001, genta

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose, 
	including commercial applications, and to alter it and redistribute it 
	freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented;
		   you must not claim that you wrote the original software.
		   If you use this software in a product, an acknowledgment
		   in the product documentation would be appreciated but is
		   not required.

		2. Altered source versions must be plainly marked as such, 
		   and must not be misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		   distribution.
*/


#include "CPropCommon.h"
#include "memory.h"
#include "stdlib.h"
#include <Shlobj.h>

//! Popup Help用ID
//@@@ 2001.12.22 Start by MIK: Popup Help
#if 1	//@@@ 2002.01.03 add MIK
#include "sakura.hh"
static const DWORD p_helpids[] = {	//11700
	IDC_MACRODIRREF,	HIDC_MACRODIRREF,	//マクロディレクトリ参照
	IDC_MACRO_REG,		HIDC_MACRO_REG,		//マクロ設定
	IDC_COMBO_MACROID,	HIDC_COMBO_MACROID,	//ID
	IDC_MACROPATH,		HIDC_MACROPATH,		//File
	IDC_MACRONAME,		HIDC_MACRONAME,		//マクロ名
	IDC_MACROLIST,		HIDC_MACROLIST,		//マクロリスト
	IDC_MACRODIR,		HIDC_MACRODIR,		//マクロ一覧
//	IDC_STATIC,			-1,
	0, 0
};
#else
static const DWORD p_helpids[] = {	//11700
	IDC_MACRODIRREF,	11700,	//参照
	IDC_MACRO_REG,		11701,	//設定
	IDC_COMBO_MACROID,	11730,	//ID
	IDC_MACROPATH,		11731,	//パス
	IDC_MACRONAME,		11740,	//マクロ名
	IDC_MACROLIST,		11741,	//リスト
	IDC_MACRODIR,		11750,	//マクロ一覧
//	IDC_STATIC,			-1,
	0, 0
};
#endif
//@@@ 2001.12.22 End

/*!
	@param hwndDlg ダイアログボックスのWindow Handle
	@param uMsg メッセージ
	@param wParam パラメータ1
	@param lParam パラメータ2
*/
BOOL CALLBACK CPropCommon::DlgProc_PROP_MACRO(
	HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return DlgProc(DispatchEvent_PROP_Macro, hwndDlg, uMsg, wParam, lParam );
}

/*! Macroページのメッセージ処理
	@param hwndDlg ダイアログボックスのWindow Handlw
	@param uMsg メッセージ
	@param wParam パラメータ1
	@param lParam パラメータ2
*/
BOOL CPropCommon::DispatchEvent_PROP_Macro( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	NMHDR*		pNMHDR;
	NM_UPDOWN*	pMNUD;
	int			idCtrl;

	WORD		wNotifyCode;
	WORD		wID;
	HWND		hwndCtl;

	switch( uMsg ){

	case WM_INITDIALOG:
		/* ダイアログデータの設定 p1 */
		InitDialog_PROP_Macro( hwndDlg );
		SetData_PROP_Macro( hwndDlg );
		::SetWindowLong( hwndDlg, DWL_USER, (LONG)lParam );

		return TRUE;
	case WM_NOTIFY:
		idCtrl = (int)wParam;
		pNMHDR = (NMHDR*)lParam;
		pMNUD  = (NM_UPDOWN*)lParam;
		switch( idCtrl ){
		case IDC_MACROLIST:
			switch( pNMHDR->code ){
			case LVN_ITEMCHANGED:
				CheckListPosition_Macro( hwndDlg );
				break;
			}
			break;
		default:
			switch( pNMHDR->code ){
			case PSN_HELP:
				OnHelp( hwndDlg, IDD_PROP_MACRO );
				return TRUE;
			case PSN_KILLACTIVE:
				/* ダイアログデータの取得 p1 */
				GetData_PROP_Macro( hwndDlg );
				return TRUE;
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
			case PSN_SETACTIVE:
				m_nPageNum = ID_PAGENUM_MACRO;
				return TRUE;
			}
			break;
		}
		break;

	case WM_COMMAND:
		wNotifyCode = HIWORD(wParam);	/* 通知コード */
		wID = LOWORD(wParam);			/* 項目ID､ コントロールID､ またはアクセラレータID */
		hwndCtl = (HWND) lParam;		/* コントロールのハンドル */

		switch( wNotifyCode ){
		/* ボタン／チェックボックスがクリックされた */
		case BN_CLICKED:
			switch( wID ){
			case IDC_MACRODIRREF:	// マクロディレクトリ参照
				SelectBaseDir_Macro( hwndDlg );
				break;
			case IDC_MACRO_REG:		// マクロ設定
				SetMacro2List_Macro( hwndDlg );
				break;
			}
			break;
		case CBN_DROPDOWN:
			switch( wID ){
			case IDC_MACROPATH:
				OnFileDropdown_Macro( hwndDlg );
				break;
			}
		}

		break;
//@@@ 2001.02.04 Start by MIK: Popup Help
	case WM_HELP:
		{
			HELPINFO *p = (HELPINFO *)lParam;
			::WinHelp( (HWND)p->hItemHandle, m_szHelpFile, HELP_WM_HELP, (DWORD)(LPVOID)p_helpids );
		}
		return TRUE;
		/*NOTREACHED*/
		//break;
//@@@ 2001.02.04 End

//@@@ 2001.12.22 Start by MIK: Context Menu Help
	//Context Menu
	case WM_CONTEXTMENU:
		::WinHelp( hwndDlg, m_szHelpFile, HELP_CONTEXTMENU, (DWORD)(LPVOID)p_helpids );
		return TRUE;
//@@@ 2001.12.22 End

	}
	return FALSE;
}


/*!
	ダイアログ上のコントロールにデータを設定する

	@param hwndDlg ダイアログボックスのウィンドウハンドル
*/
void CPropCommon::SetData_PROP_Macro( HWND hwndDlg )
{
	int index;
	LVITEM sItem;

	//	マクロデータ
	HWND hListView = ::GetDlgItem( hwndDlg, IDC_MACROLIST );
	
	for( index = 0; index < MAX_CUSTMACRO; ++index ){
		memset( &sItem, 0, sizeof( sItem ));
		sItem.iItem = index;
		sItem.mask = LVIF_TEXT;
		sItem.iSubItem = 1;
		sItem.pszText = m_pShareData->m_MacroTable[index].m_szName;
		ListView_SetItem( hListView, &sItem );

		memset( &sItem, 0, sizeof( sItem ));
		sItem.iItem = index;
		sItem.mask = LVIF_TEXT;
		sItem.iSubItem = 2;
		sItem.pszText = m_pShareData->m_MacroTable[index].m_szFile;
		ListView_SetItem( hListView, &sItem );
	}
	
	//	マクロディレクトリ
	::SetDlgItemText( hwndDlg, IDC_MACRODIR, m_pShareData->m_szMACROFOLDER );

	nLastPos_Macro = -1;
	
	//	リストビューの行選択を可能にする．
	//	IE 3.x以降が入っている場合のみ動作する．
	//	これが無くても，番号部分しか選択できないだけで操作自体は可能．
	DWORD dwStyle;
	dwStyle = ListView_GetExtendedListViewStyle( hListView );
	dwStyle |= LVS_EX_FULLROWSELECT;
	ListView_SetExtendedListViewStyle( hListView, dwStyle );

	return;
}

/*!
	ダイアログ上のコントロールからデータを取得してメモリに格納する

	@param hwndDlg ダイアログボックスのウィンドウハンドル
*/

int CPropCommon::GetData_PROP_Macro( HWND hwndDlg )
{
//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
//	m_nPageNum = ID_PAGENUM_MACRO;

	int index;
	LVITEM sItem;

	//	マクロデータ
	HWND hListView = ::GetDlgItem( hwndDlg, IDC_MACROLIST );

	for( index = 0; index < MAX_CUSTMACRO; ++index ){
		memset( &sItem, 0, sizeof( sItem ));
		sItem.iItem = index;
		sItem.mask = LVIF_TEXT;
		sItem.iSubItem = 1;
		sItem.cchTextMax = MACRONAME_MAX - 1;
//@@@ 2002.01.03 YAZAKI 共通設定『マクロ』がタブを切り替えるだけで設定が保存されないように。
		sItem.pszText = /*m_pShareData->*/m_MacroTable[index].m_szName;
		ListView_GetItem( hListView, &sItem );

		memset( &sItem, 0, sizeof( sItem ));
		sItem.iItem = index;
		sItem.mask = LVIF_TEXT;
		sItem.iSubItem = 2;
		sItem.cchTextMax = _MAX_PATH;
//@@@ 2002.01.03 YAZAKI 共通設定『マクロ』がタブを切り替えるだけで設定が保存されないように。
		sItem.pszText = /*m_pShareData->*/m_MacroTable[index].m_szFile;
		ListView_GetItem( hListView, &sItem );
	}

	//	マクロディレクトリ
//@@@ 2002.01.03 YAZAKI 共通設定『マクロ』がタブを切り替えるだけで設定が保存されないように。
	::GetDlgItemText( hwndDlg, IDC_MACRODIR, /*m_pShareData->*/m_szMACROFOLDER, _MAX_PATH );

	return TRUE;
}

void CPropCommon::InitDialog_PROP_Macro( HWND hwndDlg )
{
	struct ColumnData {
		char *title;
		int width;
	} ColumnList[] = {
		{ "番号", 40 },
		{ "マクロ名", 200 },
		{ "ファイル名", 200 },
	};

	//	ListViewの初期化
	HWND hListView = ::GetDlgItem( hwndDlg, IDC_MACROLIST );
	if( hListView == NULL ){
		::MessageBox( hwndDlg, "PropComMacro::InitDlg::NoListView", "バグ報告お願い", MB_OK );
		return;	//	よくわからんけど失敗した	
	}

	LVCOLUMN sColumn;
	int pos;
	
	for( pos = 0; pos < sizeof( ColumnList ) / sizeof( ColumnList[0] ); ++pos ){
		
		memset( &sColumn, 0, sizeof( sColumn ));
		sColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
		sColumn.pszText = ColumnList[pos].title;
		sColumn.cx = ColumnList[pos].width;
		sColumn.iSubItem = pos;
		sColumn.fmt = LVCFMT_LEFT;
		
		if( ListView_InsertColumn( hListView, pos, &sColumn ) < 0 ){
			::MessageBox( hwndDlg, "PropComMacro::InitDlg::ColumnRegistrationFail", "バグ報告お願い", MB_OK );
			return;	//	よくわからんけど失敗した
		}
	}

	//	メモリの確保
	//	必要な数だけ先に確保する．
	ListView_SetItemCount( hListView, MAX_CUSTMACRO );

	//	Index部分の登録
	for( pos = 0; pos < MAX_CUSTMACRO ; ++pos ){
		LVITEM sItem;
		char buf[4];
		memset( &sItem, 0, sizeof( sItem ));
		sItem.mask = LVIF_TEXT | LVIF_PARAM;
		sItem.iItem = pos;
		sItem.iSubItem = 0;
		itoa( pos, buf, 10 );
		sItem.pszText = buf;
		sItem.lParam = pos;
		ListView_InsertItem( hListView, &sItem );
	}
	
	// 登録先指定 ComboBoxの初期化
	HWND hNumCombo = ::GetDlgItem( hwndDlg, IDC_COMBO_MACROID );
	for( pos = 0; pos < MAX_CUSTMACRO ; ++pos ){
		char buf[10];
		wsprintf( buf, "%d", pos );
		int result = ::SendMessage( hNumCombo, CB_ADDSTRING, (WPARAM)0, (LPARAM)buf );
		if( result == CB_ERR ){
			::MessageBox( hwndDlg, "PropComMacro::InitDlg::AddMacroId", "バグ報告お願い", MB_OK );
			return;	//	よくわからんけど失敗した
		}
		else if( result == CB_ERRSPACE ){
			::MessageBox( hwndDlg, "PropComMacro::InitDlg::AddMacroId/InsufficientSpace",
				"バグ報告お願い", MB_OK );
			return;	//	よくわからんけど失敗した
		}
	}
	::SendMessage( hNumCombo, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
}

void CPropCommon::SetMacro2List_Macro( HWND hwndDlg )
{
	int index;
	LVITEM sItem;
	char buf[256];
	
	HWND hListView = ::GetDlgItem( hwndDlg, IDC_MACROLIST );
	HWND hNum = ::GetDlgItem( hwndDlg, IDC_COMBO_MACROID );

	//	設定先取得
	index = ::SendMessage( hNum, CB_GETCURSEL, 0, 0 );
	if( index == CB_ERR ){
		::MessageBox( hwndDlg, "PropComMacro::SetMacro2List::GetCurSel",
			"バグ報告お願い", MB_OK );
		return;	//	よくわからんけど失敗した
	}

	// マクロ名
	memset( &sItem, 0, sizeof( sItem ));
	sItem.iItem = index;
	sItem.mask = LVIF_TEXT;
	sItem.iSubItem = 1;
	
	::GetDlgItemText( hwndDlg, IDC_MACRONAME, buf, MACRONAME_MAX );
	sItem.pszText = buf;
	ListView_SetItem( hListView, &sItem );

	// ファイル名
	memset( &sItem, 0, sizeof( sItem ));
	sItem.iItem = index;
	sItem.mask = LVIF_TEXT;
	sItem.iSubItem = 2;

	::GetDlgItemText( hwndDlg, IDC_MACROPATH, buf, _MAX_PATH );
	sItem.pszText = buf;
	ListView_SetItem( hListView, &sItem );
}

/*!
	Macro格納用ディレクトリを選択する
	
	@param hwndDlg [in] ダイアログボックスのウィンドウハンドル
*/
void CPropCommon::SelectBaseDir_Macro( HWND hwndDlg )
{

	LPMALLOC pMalloc;
	BROWSEINFO bi;
	TCHAR szDir[MAX_PATH + 1]; // 追加する\\用に1足した
	LPITEMIDLIST pidl;

	char szInitial[MAX_PATH];

	if( SHGetMalloc(&pMalloc) == E_FAIL ){
		::MessageBox( hwndDlg, "PropComMacro::SelectBaseDir_Macro::SHGetMalloc",
			"バグ報告お願い", MB_OK );
		return;	//	よくわからんけど失敗した
	}

	::GetDlgItemText( hwndDlg, IDC_MACRODIR, szInitial, MAX_PATH );

	ZeroMemory(&bi,sizeof(bi));
	bi.hwndOwner = hwndDlg;	 // オーナーウィンドウハンドルを設定
	bi.pidlRoot = NULL;
	bi.pszDisplayName = szDir;
	bi.lpszTitle = "Macroディレクトリの選択";
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = CPropCommon::DirCallback_Macro;
	bi.lParam = (LPARAM)szInitial;	// 初期ディレクトリ

	// フォルダの参照ダイアログボックスの表示
	pidl = SHBrowseForFolder(&bi);

	if (pidl)
	{
		if (SHGetPathFromIDList(pidl,szDir))
		{
			//	末尾に\\マークを追加する．
			int pos = strlen( szDir );
			if( szDir[ pos - 1 ] != '\\' ){
				szDir[ pos ] = '\\';
				szDir[ pos + 1 ] = '\0';
			}
			::SetDlgItemText( hwndDlg, IDC_MACRODIR, szDir );
		}

		// SHBrowseForFolder によって割り当てられた PIDL を解放
		pMalloc->Free(pidl);
	}

	// Shell のアロケータを開放
	pMalloc->Release();
}

/*!
	フォルダ選択ダイアログボックス用Callback関数

	SHBrowseForFolderの初期ディレクトリを指定するためのコールバック関数
	
	@param hwnd [in] ダイアログボックスのウィンドウハンドル
	@param uMsg [in] 通知種別
	@param lParam [in] 
	@param lpData [in] BROWSEINFOで渡された値．
					ここでは，初期ディレクトリへのポインタ(const char*)がキャストされて入っている．
*/
int CALLBACK CPropCommon::DirCallback_Macro( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
	if( uMsg == BFFM_INITIALIZED ){	//	初期化完了
		::SendMessage( hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}
	return 0;
}
/*!
	マクロファイル指定用コンボボックスのドロップダウンリストが開かれるときに，
	指定ディレクトリのファイル一覧から候補を生成する．

	@param hwndDlg [in] ダイアログボックスのウィンドウハンドル
*/
void CPropCommon::OnFileDropdown_Macro( HWND hwndDlg )
{
	char path[_MAX_PATH * 2 ];
	WIN32_FIND_DATA wf;
	HANDLE hFind;
	HWND hCombo = ::GetDlgItem( hwndDlg, IDC_MACROPATH );

	::GetDlgItemText( hwndDlg, IDC_MACRODIR, path, _MAX_PATH );
	strcat( path, "*.mac" );

	//	候補の初期化
	::SendMessage( hCombo, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0 );

	//	ファイルの検索
	hFind = FindFirstFile(path, &wf);

	if (hFind == INVALID_HANDLE_VALUE) {
		return;
	}
	
	do {
		//	コンボボックスに設定
		int result = ::SendMessage( hCombo, CB_ADDSTRING, (WPARAM)0, (LPARAM)wf.cFileName );
		if( result == CB_ERR || result == CB_ERRSPACE )
			break;
	} while( FindNextFile( hFind, &wf ));

    FindClose(hFind);
}

void CPropCommon::CheckListPosition_Macro( HWND hwndDlg )
{
	HWND hListView = ::GetDlgItem( hwndDlg, IDC_MACROLIST );
	HWND hNum = ::GetDlgItem( hwndDlg, IDC_COMBO_MACROID );
	
	//	現在のFocus取得
	int current = ListView_GetNextItem( hListView, -1, LVNI_SELECTED);

	if( current == -1 || current == nLastPos_Macro )
		return;

	nLastPos_Macro = current;
	
	//	初期値の設定
	::SendMessage( hNum, CB_SETCURSEL, nLastPos_Macro, 0 );
	
	char buf[MAX_PATH + MACRONAME_MAX];	// MAX_PATHとMACRONAME_MAXの両方より大きい値
	LVITEM sItem;

	memset( &sItem, 0, sizeof( sItem ));
	sItem.iItem = current;
	sItem.mask = LVIF_TEXT;
	sItem.iSubItem = 1;
	sItem.pszText = buf;
	sItem.cchTextMax = MACRONAME_MAX;

	ListView_GetItem( hListView, &sItem );
	::SetDlgItemText( hwndDlg, IDC_MACRONAME, buf );

	memset( &sItem, 0, sizeof( sItem ));
	sItem.iItem = current;
	sItem.mask = LVIF_TEXT;
	sItem.iSubItem = 2;
	sItem.pszText = buf;
	sItem.cchTextMax = MAX_PATH;

	ListView_GetItem( hListView, &sItem );
	::SetDlgItemText( hwndDlg, IDC_MACROPATH, buf );

}