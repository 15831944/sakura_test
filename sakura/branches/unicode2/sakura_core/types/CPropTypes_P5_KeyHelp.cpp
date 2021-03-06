/*!	@file
	タイプ別設定 - キーワードヘルプ ページ

	@author fon
	@date 2006/04/10 新規作成
*/
/*
	Copyright (C) 2006, fon, ryoji
	Copyright (C) 2007, ryoji

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

#include "stdafx.h"
#include "global.h"
#include "sakura_rc.h"
#include "types/CPropTypes.h"
#include "debug/Debug.h"
#include "dlg/CDlgOpenFile.h"
#include <stdio.h>	//@@@ 2001.11.17 add MIK
#include "charset/charcode.h"
#include "sakura.hh"
#include "charset/CharPointer.h"
#include "io/CTextStream.h"
#include "util/shell.h"
#include "util/file.h"
#include "util/module.h"
using namespace std;

static const DWORD p_helpids[] = {	// 2006.10.10 ryoji
	IDC_CHECK_KEYHELP,				HIDC_CHECK_KEYHELP,				//キーワードヘルプ機能を使う
//	IDC_FRAME_KEYHELP,				HIDC_LIST_KEYHELP,				//辞書ファイル一覧(&L)		
	IDC_LIST_KEYHELP,				HIDC_LIST_KEYHELP,				//SysListView32
	IDC_BUTTON_KEYHELP_UPD,			HIDC_BUTTON_KEYHELP_UPD,		//更新(&E)
//	IDC_LABEL_KEYHELP_KEYWORD,		HIDC_EDIT_KEYHELP,				//辞書ファイル
	IDC_EDIT_KEYHELP,				HIDC_EDIT_KEYHELP,				//EDITTEXT
	IDC_BUTTON_KEYHELP_REF,			HIDC_BUTTON_KEYHELP_REF,		//参照(&O)...
	IDC_BUTTON_KEYHELP_TOP,			HIDC_BUTTON_KEYHELP_TOP,		//先頭(&T)
	IDC_BUTTON_KEYHELP_UP,			HIDC_BUTTON_KEYHELP_UP,			//上へ(&U)
	IDC_BUTTON_KEYHELP_DOWN,		HIDC_BUTTON_KEYHELP_DOWN,		//下へ(&G)
	IDC_BUTTON_KEYHELP_LAST,		HIDC_BUTTON_KEYHELP_LAST,		//最終(&B)
	IDC_BUTTON_KEYHELP_INS,			HIDC_BUTTON_KEYHELP_INS,		//挿入(&S)
	IDC_BUTTON_KEYHELP_DEL,			HIDC_BUTTON_KEYHELP_DEL,		//削除(&D)
	IDC_CHECK_KEYHELP_ALLSEARCH,	HIDC_CHECK_KEYHELP_ALLSEARCH,	//全辞書検索する(&A)
	IDC_CHECK_KEYHELP_KEYDISP,		HIDC_CHECK_KEYHELP_KEYDISP,		//キーワードも表示する(&W)
	IDC_CHECK_KEYHELP_PREFIX,		HIDC_CHECK_KEYHELP_PREFIX,		//前方一致検索(&P)
	IDC_BUTTON_KEYHELP_IMPORT,		HIDC_BUTTON_KEYHELP_IMPORT,		//インポート
	IDC_BUTTON_KEYHELP_EXPORT,		HIDC_BUTTON_KEYHELP_EXPORT,		//エクスポート
	0, 0
};

static TCHAR* strcnv(TCHAR *str);
static TCHAR* GetFileName(const TCHAR *fullpath);

/*! キーワード辞書ファイル設定 メッセージ処理

	@date 2006.04.10 fon 新規作成
*/
INT_PTR CPropTypes::DispatchEvent_KeyHelp(
	HWND		hwndDlg,	// handle to dialog box
	UINT		uMsg,		// message
	WPARAM		wParam,		// first message parameter
	LPARAM		lParam 		// second message parameter
)
{
	WORD	wNotifyCode;
	WORD	wID;
	HWND	hwndCtl, hwndList;
	int		idCtrl;
	NMHDR*	pNMHDR;
	int		nIndex, nIndex2;
	LV_ITEM	lvi;
	LV_COLUMN	col;
	RECT		rc;
	static int	nPrevIndex = -1;	//更新時におかしくなるバグ修正 @@@ 2003.03.26 MIK

	BOOL	bUse;						/* 辞書を 使用する/しない */
	TCHAR	szAbout[DICT_ABOUT_LEN];	/* 辞書の説明(辞書ファイルの1行目から生成) */
	TCHAR	szPath[_MAX_PATH];			/* ファイルパス */
	DWORD	dwStyle;

	hwndList = GetDlgItem( hwndDlg, IDC_LIST_KEYHELP );

	switch( uMsg ){
	case WM_INITDIALOG:
		::SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );
		/* カラム追加 */
		::GetWindowRect( hwndList, &rc );
		/* リストにチェックボックスを追加 */
		dwStyle = ListView_GetExtendedListViewStyle(hwndList);
		dwStyle |= LVS_EX_CHECKBOXES /*| LVS_EX_FULLROWSELECT*/ | LVS_EX_GRIDLINES;
		ListView_SetExtendedListViewStyle(hwndList, dwStyle);

		col.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		col.fmt      = LVCFMT_LEFT;
		col.cx       = (rc.right - rc.left) * 25 / 100;
		col.pszText  = _T("   辞書ファイル");	/* 指定辞書ファイルの使用可否 */
		col.iSubItem = 0;
		ListView_InsertColumn( hwndList, 0, &col );
		col.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		col.fmt      = LVCFMT_LEFT;
		col.cx       = (rc.right - rc.left) * 55 / 100;
		col.pszText  = _T("辞書の説明");		/* 指定辞書の１行目を取得 */
		col.iSubItem = 1;
		ListView_InsertColumn( hwndList, 1, &col );
		col.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		col.fmt      = LVCFMT_LEFT;
		col.cx       = (rc.right - rc.left) * 18 / 100;
		col.pszText  = _T("パス");				/* 指定辞書ファイルパス */
		col.iSubItem = 2;
		ListView_InsertColumn( hwndList, 2, &col );
		nPrevIndex = -1;	//@@@ 2003.05.12 MIK
		SetData_KeyHelp( hwndDlg );	/* ダイアログデータの設定 辞書ファイル一覧 */

		/* リストがあれば先頭をフォーカスする */
		if(ListView_GetItemCount(hwndList) > 0){
			ListView_SetItemState( hwndList, 0, LVIS_SELECTED /*| LVIS_FOCUSED*/, LVIS_SELECTED /*| LVIS_FOCUSED*/ );
		}
		/* リストがなければ初期値として用途を表示 */
		else{
			::DlgItem_SetText( hwndDlg, IDC_LABEL_KEYHELP_ABOUT, _T("辞書ファイルの１行目の文字列") );
			::DlgItem_SetText( hwndDlg, IDC_EDIT_KEYHELP, _T("キーワード辞書ファイル パス") );
		}

		/* 初期状態を設定 */
		SendMessageCmd(hwndDlg, WM_COMMAND, (WPARAM)MAKELONG(IDC_CHECK_KEYHELP,BN_CLICKED), 0 );

		return TRUE;

	case WM_COMMAND:
		wNotifyCode = HIWORD(wParam);	/* 通知コード */
		wID	= LOWORD(wParam);			/* 項目ID､ コントロールID､ またはアクセラレータID */
		hwndCtl	= (HWND) lParam;		/* コントロールのハンドル */

		switch( wNotifyCode ){
		/* ボタン／チェックボックスがクリックされた */
		case BN_CLICKED:

			switch( wID ){
			case IDC_CHECK_KEYHELP:	/* キーワードヘルプ機能を使う */
				if( !IsDlgButtonChecked( hwndDlg, IDC_CHECK_KEYHELP ) ){
					//EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_KEYHELP ), FALSE );			//キーワードヘルプ機能を使う(&K)
					EnableWindow( GetDlgItem( hwndDlg, IDC_FRAME_KEYHELP ), FALSE );		  	//辞書ファイル一覧(&L)
					EnableWindow( GetDlgItem( hwndDlg, IDC_LIST_KEYHELP ), FALSE );         	//SysListView32
					EnableWindow( GetDlgItem( hwndDlg, IDC_LABEL_KEYHELP_TITLE ), FALSE );  	//<辞書の説明>
					EnableWindow( GetDlgItem( hwndDlg, IDC_LABEL_KEYHELP_ABOUT ), FALSE );  	//辞書ファイルの概要
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_UPD ), FALSE );   	//更新(&E)
					EnableWindow( GetDlgItem( hwndDlg, IDC_LABEL_KEYHELP_KEYWORD ), FALSE );	//辞書ファイル
					EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_KEYHELP ), FALSE );         	//EDITTEXT
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_REF ), FALSE );   	//参照(&O)...
					EnableWindow( GetDlgItem( hwndDlg, IDC_LABEL_KEYHELP_PRIOR ), FALSE );  	//↑優先度(高)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_TOP ), FALSE );   	//先頭(&T)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_UP ), FALSE );    	//上へ(&U)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_DOWN ), FALSE );  	//下へ(&G)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_LAST ), FALSE );  	//最終(&B)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_INS ), FALSE );   	//挿入(&S)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_DEL ), FALSE );   	//削除(&D)
					EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_KEYHELP_ALLSEARCH ), FALSE );	//全辞書検索する(&A)
					EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_KEYHELP_KEYDISP ), FALSE );	//キーワードも表示する(&W)
					EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_KEYHELP_PREFIX ), FALSE );		//前方一致検索(&P)
				}else{
					//EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_KEYHELP ), TRUE );			//キーワードヘルプ機能を使う(&K)
					EnableWindow( GetDlgItem( hwndDlg, IDC_FRAME_KEYHELP ), TRUE );				//辞書ファイル一覧(&L)
					EnableWindow( GetDlgItem( hwndDlg, IDC_LIST_KEYHELP ), TRUE );				//SysListView32
					EnableWindow( GetDlgItem( hwndDlg, IDC_LABEL_KEYHELP_TITLE ), TRUE );		//<辞書の説明>
					EnableWindow( GetDlgItem( hwndDlg, IDC_LABEL_KEYHELP_ABOUT ), TRUE );  		//辞書ファイルの概要
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_UPD ), TRUE );   		//更新(&E)
					EnableWindow( GetDlgItem( hwndDlg, IDC_LABEL_KEYHELP_KEYWORD ), TRUE );		//辞書ファイル
					EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_KEYHELP ), TRUE );         		//EDITTEXT
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_REF ), TRUE );   		//参照(&O)...
					EnableWindow( GetDlgItem( hwndDlg, IDC_LABEL_KEYHELP_PRIOR ), TRUE );  		//↑優先度(高)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_TOP ), TRUE );   		//先頭(&T)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_UP ), TRUE );    		//上へ(&U)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_DOWN ), TRUE );  		//下へ(&G)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_LAST ), TRUE );  		//最終(&B)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_INS ), TRUE );   		//挿入(&S)
					EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_KEYHELP_DEL ), TRUE );   		//削除(&D)
					EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_KEYHELP_ALLSEARCH ), TRUE );	//全辞書検索する(&A)
					EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_KEYHELP_KEYDISP ), TRUE );		//キーワードも表示する(&W)
					EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_KEYHELP_PREFIX ), TRUE );		//前方一致検索(&P)
				}
				m_Types.m_nKeyHelpNum = ListView_GetItemCount( hwndList );
				return TRUE;

			/* 挿入・更新イベントを纏めて処理 */
			case IDC_BUTTON_KEYHELP_INS:	/* 挿入 */
			case IDC_BUTTON_KEYHELP_UPD:	/* 更新 */
				nIndex2 = ListView_GetItemCount(hwndList);
				/* 選択中のキーを探す。 */
				nIndex = ListView_GetNextItem( hwndList, -1, LVNI_ALL | LVNI_SELECTED );

				if(wID == IDC_BUTTON_KEYHELP_INS){	/* 挿入 */
					if( nIndex2 >= MAX_KEYHELP_FILE ){
						ErrorMessage( hwndDlg, _T("これ以上登録できません。"));
						return FALSE;
					}if( -1 == nIndex ){
						/* 選択中でなければ最後にする。 */
						nIndex = nIndex2;
						if(nIndex == 0)
							nPrevIndex = 0;
					}
				}else{								/* 更新 */
					if( -1 == nIndex ){
						ErrorMessage( hwndDlg, _T("更新する辞書をリストから選択してください。"));
						return FALSE;
					}
				}
				/* 更新するキー情報を取得する。 */
				auto_memset(szPath, 0, _countof(szPath));
				::DlgItem_GetText( hwndDlg, IDC_EDIT_KEYHELP, szPath, _countof(szPath) );
				if( szPath[0] == _T('\0') ) return FALSE;
				/* 重複検査 */
				nIndex2 = ListView_GetItemCount(hwndList);
				TCHAR szPath2[_MAX_PATH];
				int i;
				for(i = 0; i < nIndex2; i++){
					auto_memset(szPath2, 0, _countof(szPath2));
					ListView_GetItemText(hwndList, i, 2, szPath2, _countof(szPath2));
					if( _tcscmp(szPath, szPath2) == 0 ){
						if( (wID ==IDC_BUTTON_KEYHELP_UPD) && (i == nIndex) ){	/* 更新時、変わっていなかったら何もしない */
						}else{
							ErrorMessage( hwndDlg, _T("既に登録済みの辞書です。"));
							return FALSE;
						}
					}
				}

				/* 指定したパスに辞書があるかチェックする */
				{
					CTextInputStream_AbsIni in(szPath);
					if(!in){
						ErrorMessage( hwndDlg, _T("ファイルを開けませんでした。\n\n%ts"), szPath );
						return FALSE;
					}
					// 開けたなら1行目を取得してから閉じる -> szAbout
					std::wstring line=in.ReadLineW();
					_wcstotcs(szAbout,line.c_str(),_countof(szAbout));
					in.Close();
				}
				strcnv(szAbout);

				/* ついでに辞書の説明を更新 */
				::DlgItem_SetText( hwndDlg, IDC_LABEL_KEYHELP_ABOUT, szAbout );	/* 辞書ファイルの概要 */
				
				/* 更新のときは行削除する。 */
				if(wID == IDC_BUTTON_KEYHELP_UPD){	/* 更新 */
					ListView_DeleteItem( hwndList, nIndex );
				}
				
				/* ON/OFF ファイル名 */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex;
				lvi.iSubItem = 0;
				lvi.pszText = GetFileName(szPath);	/* ファイル名を表示 */
				ListView_InsertItem( hwndList, &lvi );
				/* 辞書の説明 */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex;
				lvi.iSubItem = 1;
				lvi.pszText  = szAbout;
				ListView_SetItem( hwndList, &lvi );
				/* 辞書ファイルパス */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex;
				lvi.iSubItem = 2;
				lvi.pszText  = szPath;
				ListView_SetItem( hwndList, &lvi );
				
				/* デフォルトでチェックON */
				ListView_SetCheckState(hwndList, nIndex, TRUE);

				/* 更新したキーを選択する。 */
				ListView_SetItemState( hwndList, nIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
				GetData_KeyHelp( hwndDlg );
				return TRUE;

			case IDC_BUTTON_KEYHELP_DEL:	/* 削除 */
				/* 選択中のキー番号を探す。 */
				nIndex = ListView_GetNextItem( hwndList, -1, LVNI_ALL | LVNI_SELECTED );
				if( -1 == nIndex ) return FALSE;
				/* 削除する。 */
				ListView_DeleteItem( hwndList, nIndex );
				/* リストがなくなったら初期値として用途を表示 */
				if(ListView_GetItemCount(hwndList) == 0){
					::DlgItem_SetText( hwndDlg, IDC_LABEL_KEYHELP_ABOUT, _T("辞書ファイルの１行目の文字列") );
					::DlgItem_SetText( hwndDlg, IDC_EDIT_KEYHELP, _T("キーワード辞書ファイル パス") );
				}/* リストの最後を削除した場合は、削除後のリストの最後を選択する。 */
				else if(nIndex > ListView_GetItemCount(hwndList)-1){
					ListView_SetItemState( hwndList, ListView_GetItemCount(hwndList)-1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
				}/* 同じ位置のキーを選択状態にする。 */
				else{
					ListView_SetItemState( hwndList, nIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
				}
				GetData_KeyHelp( hwndDlg );
				return TRUE;

			case IDC_BUTTON_KEYHELP_TOP:	/* 先頭 */
				/* 選択中のキーを探す。 */
				nIndex = ListView_GetNextItem( hwndList, -1, LVNI_ALL | LVNI_SELECTED );
				if( -1 == nIndex ) return FALSE;
				if( 0 == nIndex ) return TRUE;	/* すでに先頭にある。 */
				nIndex2 = 0;
				bUse = ListView_GetCheckState(hwndList, nIndex);
				ListView_GetItemText(hwndList, nIndex, 1, szAbout, _countof(szAbout));
				ListView_GetItemText(hwndList, nIndex, 2, szPath, _countof(szPath));
				ListView_DeleteItem(hwndList, nIndex);	/* 古いキーを削除 */
				/* ON-OFF */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 0;
				lvi.pszText = GetFileName(szPath);	/* ファイル名を表示 */
				ListView_InsertItem( hwndList, &lvi );
				/* 辞書の説明 */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 1;
				lvi.pszText  = szAbout;
				ListView_SetItem( hwndList, &lvi );
				/* 辞書ファイルパス */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 2;
				lvi.pszText  = szPath;
				ListView_SetItem( hwndList, &lvi );
				ListView_SetCheckState(hwndList, nIndex2, bUse);
				/* 移動したキーを選択状態にする。 */
				ListView_SetItemState( hwndList, nIndex2, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
				GetData_KeyHelp( hwndDlg );
				return TRUE;

			case IDC_BUTTON_KEYHELP_LAST:	/* 最終 */
				nIndex = ListView_GetNextItem( hwndList, -1, LVNI_ALL | LVNI_SELECTED );
				if( -1 == nIndex ) return FALSE;
				nIndex2 = ListView_GetItemCount(hwndList);
				if( nIndex2 - 1 == nIndex ) return TRUE;	/* すでに最終にある。 */
				bUse = ListView_GetCheckState(hwndList, nIndex);
				ListView_GetItemText(hwndList, nIndex, 1, szAbout, _countof(szAbout));
				ListView_GetItemText(hwndList, nIndex, 2, szPath, _countof(szPath));
				/* キーを追加する。 */
				/* ON-OFF */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 0;
				lvi.pszText = GetFileName(szPath);	/* ファイル名を表示 */
				ListView_InsertItem( hwndList, &lvi );
				/* 辞書の説明 */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 1;
				lvi.pszText  = szAbout;
				ListView_SetItem( hwndList, &lvi );
				/* 辞書ファイルパス */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 2;
				lvi.pszText  = szPath;
				ListView_SetItem( hwndList, &lvi );
				ListView_SetCheckState(hwndList, nIndex2, bUse);
				/* 移動したキーを選択状態にする。 */
				ListView_SetItemState( hwndList, nIndex2, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
				ListView_DeleteItem(hwndList, nIndex);	/* 古いキーを削除 */
				GetData_KeyHelp( hwndDlg );
				return TRUE;

			case IDC_BUTTON_KEYHELP_UP:	/* 上へ */
				nIndex = ListView_GetNextItem( hwndList, -1, LVNI_ALL | LVNI_SELECTED );
				if( -1 == nIndex ) return FALSE;
				if( 0 == nIndex ) return TRUE;	/* すでに先頭にある。 */
				nIndex2 = ListView_GetItemCount(hwndList);
				if( nIndex2 <= 1 ) return TRUE;
				nIndex2 = nIndex - 1;
				bUse = ListView_GetCheckState(hwndList, nIndex);
				ListView_GetItemText(hwndList, nIndex, 1, szAbout, _countof(szAbout));
				ListView_GetItemText(hwndList, nIndex, 2, szPath, _countof(szPath));
				ListView_DeleteItem(hwndList, nIndex);	/* 古いキーを削除 */
				/* キーを追加する。 */
				/* ON-OFF */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 0;
				lvi.pszText = GetFileName(szPath);	/* ファイル名を表示 */
				ListView_InsertItem( hwndList, &lvi );
				/* 辞書の説明 */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 1;
				lvi.pszText  = szAbout;
				ListView_SetItem( hwndList, &lvi );
				/* 辞書ファイルパス */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 2;
				lvi.pszText  = szPath;
				ListView_SetItem( hwndList, &lvi );
				ListView_SetCheckState(hwndList, nIndex2, bUse);
				/* 移動したキーを選択状態にする。 */
				ListView_SetItemState( hwndList, nIndex2, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
				GetData_KeyHelp( hwndDlg );
				return TRUE;

			case IDC_BUTTON_KEYHELP_DOWN:	/* 下へ */
				nIndex = ListView_GetNextItem( hwndList, -1, LVNI_ALL | LVNI_SELECTED );
				if( -1 == nIndex ) return FALSE;
				nIndex2 = ListView_GetItemCount(hwndList);
				if( nIndex2 - 1 == nIndex ) return TRUE;	/* すでに最終にある。 */
				if( nIndex2 <= 1 ) return TRUE;
				nIndex2 = nIndex + 2;
				bUse = ListView_GetCheckState(hwndList, nIndex);
				ListView_GetItemText(hwndList, nIndex, 1, szAbout, _countof(szAbout));
				ListView_GetItemText(hwndList, nIndex, 2, szPath, _countof(szPath));
				/* キーを追加する。 */
				/* ON-OFF */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 0;
				lvi.pszText = GetFileName(szPath);	/* ファイル名を表示 */
				ListView_InsertItem( hwndList, &lvi );
				/* 辞書の説明 */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 1;
				lvi.pszText  = szAbout;
				ListView_SetItem( hwndList, &lvi );
				/* 辞書ファイルパス */
				lvi.mask     = LVIF_TEXT;
				lvi.iItem    = nIndex2;
				lvi.iSubItem = 2;
				lvi.pszText  = szPath;
				ListView_SetItem( hwndList, &lvi );
				ListView_SetCheckState(hwndList, nIndex2, bUse);
				/* 移動したキーを選択状態にする。 */
				ListView_SetItemState( hwndList, nIndex2, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
				ListView_DeleteItem(hwndList, nIndex);	/* 古いキーを削除 */
				GetData_KeyHelp( hwndDlg );
				return TRUE;

			case IDC_BUTTON_KEYHELP_REF:	/* キーワードヘルプ 辞書ファイルの「参照...」ボタン */
				{
					CDlgOpenFile	cDlgOpenFile;
					/* ファイルオープンダイアログの初期化 */
					// 2007.05.19 ryoji 相対パスは設定ファイルからのパスを優先
					TCHAR szWk[_MAX_PATH];
					::DlgItem_GetText( hwndDlg, IDC_EDIT_KEYHELP, szWk, _MAX_PATH );
					if( _IS_REL_PATH( szWk ) ){
						GetInidirOrExedir( szPath, szWk );
					}else{
						::lstrcpy( szPath, szWk );
					}
					cDlgOpenFile.Create( m_hInstance, hwndDlg, _T("*.khp"), szPath );
					if( cDlgOpenFile.DoModal_GetOpenFileName( szPath ) ){
						::DlgItem_SetText( hwndDlg, IDC_EDIT_KEYHELP, szPath );
					}
				}
				return TRUE;

			case IDC_BUTTON_KEYHELP_IMPORT:	/* インポート */
				Import_KeyHelp(hwndDlg);
				m_Types.m_nKeyHelpNum = ListView_GetItemCount( hwndList );
				return TRUE;

			case IDC_BUTTON_KEYHELP_EXPORT:	/* エクスポート */
				Export_KeyHelp(hwndDlg);
				return TRUE;
			}
			break;
		}
		break;

	case WM_NOTIFY:
		idCtrl = (int)wParam;
		pNMHDR = (NMHDR*)lParam;

		switch( pNMHDR->code ){
		case PSN_HELP:
			OnHelp( hwndDlg, IDD_PROP_KEYHELP );
			return TRUE;

		case PSN_KILLACTIVE:
			/* ダイアログデータの取得 辞書ファイルリスト */
			GetData_KeyHelp( hwndDlg );
			return TRUE;

		case PSN_SETACTIVE:
			m_nPageNum = 4;	//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
			return TRUE;

		case LVN_ITEMCHANGED:	/*リストの項目が変更された際の処理*/
			if( pNMHDR->hwndFrom == hwndList ){
				nIndex = ListView_GetNextItem( hwndList, -1, LVNI_ALL | LVNI_SELECTED );
				if( -1 == nIndex ){	//削除、範囲外でクリック時反映されないバグ修正	//@@@ 2003.06.17 MIK
					nIndex = ListView_GetNextItem( hwndList, -1, LVNI_ALL | LVNI_FOCUSED );
					return FALSE;
				}
				ListView_GetItemText(hwndList, nIndex, 1, szAbout, _countof(szAbout));
				ListView_GetItemText(hwndList, nIndex, 2, szPath, _countof(szPath));
				::DlgItem_SetText( hwndDlg, IDC_LABEL_KEYHELP_ABOUT, szAbout );	/* 辞書の説明 */
				::DlgItem_SetText( hwndDlg, IDC_EDIT_KEYHELP, szPath );			/* ファイルパス */
				nPrevIndex = nIndex;
			}
			break;
		}
		break;

	case WM_HELP:
		{	HELPINFO *p = (HELPINFO *)lParam;
			MyWinHelp( (HWND)p->hItemHandle, m_szHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		}return TRUE;

	case WM_CONTEXTMENU:	/* Context Menu */
		MyWinHelp( hwndDlg, m_szHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		return TRUE;
	}
	return FALSE;
}


/*! ダイアログデータの設定 キーワード辞書ファイル設定

	@date 2006.04.10 fon 新規作成
*/
void CPropTypes::SetData_KeyHelp( HWND hwndDlg )
{
	HWND	hwndWork;
	int		i;
	LV_ITEM	lvi;
	DWORD	dwStyle;

	/* ユーザーがエディット コントロールに入力できるテキストの長さを制限する */
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_KEYHELP ), EM_LIMITTEXT, _countof2( m_Types.m_KeyHelpArr[0].m_szPath ) - 1, (LPARAM)0 );
	/* 使用する・使用しない */
	if( m_Types.m_bUseKeyWordHelp == TRUE )
		CheckDlgButton( hwndDlg, IDC_CHECK_KEYHELP, BST_CHECKED );
	else
		CheckDlgButton( hwndDlg, IDC_CHECK_KEYHELP, BST_UNCHECKED );
	if( m_Types.m_bUseKeyHelpAllSearch == TRUE )
		CheckDlgButton( hwndDlg, IDC_CHECK_KEYHELP_ALLSEARCH, BST_CHECKED );
	else
		CheckDlgButton( hwndDlg, IDC_CHECK_KEYHELP_ALLSEARCH, BST_UNCHECKED );
	if( m_Types.m_bUseKeyHelpKeyDisp == TRUE )
		CheckDlgButton( hwndDlg, IDC_CHECK_KEYHELP_KEYDISP, BST_CHECKED );
	else
		CheckDlgButton( hwndDlg, IDC_CHECK_KEYHELP_KEYDISP, BST_UNCHECKED );
	if( m_Types.m_bUseKeyHelpPrefix == TRUE )
		CheckDlgButton( hwndDlg, IDC_CHECK_KEYHELP_PREFIX, BST_CHECKED );
	else
		CheckDlgButton( hwndDlg, IDC_CHECK_KEYHELP_PREFIX, BST_UNCHECKED );
	/* リスト */
	hwndWork = ::GetDlgItem( hwndDlg, IDC_LIST_KEYHELP );
	ListView_DeleteAllItems(hwndWork);  /* リストを空にする */
	/* 行選択 */
	dwStyle = (DWORD)::SendMessageAny( hwndWork, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 );
	dwStyle |= LVS_EX_FULLROWSELECT;
	::SendMessageAny( hwndWork, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle );
	/* データ表示 */
	for(i = 0; i < MAX_KEYHELP_FILE; i++){
		if( m_Types.m_KeyHelpArr[i].m_szPath[0] == '\0' ) break;
		/* ON-OFF */
		lvi.mask     = LVIF_TEXT;
		lvi.iItem    = i;
		lvi.iSubItem = 0;
		lvi.pszText = GetFileName(m_Types.m_KeyHelpArr[i].m_szPath);
		ListView_InsertItem( hwndWork, &lvi );
		/* 辞書の説明 */
		lvi.mask     = LVIF_TEXT;
		lvi.iItem    = i;
		lvi.iSubItem = 1;
		lvi.pszText  = m_Types.m_KeyHelpArr[i].m_szAbout;
		ListView_SetItem( hwndWork, &lvi );
		/* 辞書ファイルパス */
		lvi.mask     = LVIF_TEXT;
		lvi.iItem    = i;
		lvi.iSubItem = 2;
		lvi.pszText  = m_Types.m_KeyHelpArr[i].m_szPath;
		ListView_SetItem( hwndWork, &lvi );
		/* ON/OFFを取得してチェックボックスにセット（とりあえず応急処置） */
		if(m_Types.m_KeyHelpArr[i].m_bUse){	// ON
			ListView_SetCheckState(hwndWork, i, TRUE);
		}
		else{
			ListView_SetCheckState(hwndWork, i, FALSE);
		}
	}
	ListView_SetItemState( hwndWork, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
	return;
}

/*! ダイアログデータの取得 キーワード辞書ファイル設定

	@date 2006.04.10 fon 新規作成
*/
int CPropTypes::GetData_KeyHelp( HWND hwndDlg )
{
	HWND	hwndList;
	int	nIndex, i;
	TCHAR	szAbout[DICT_ABOUT_LEN];	/* 辞書の説明(辞書ファイルの1行目から生成) */
	TCHAR	szPath[_MAX_PATH];			/* ファイルパス */
//	m_nPageNum = 4;	//自分のページ番号

	/* 使用する・使用しない */
	if( TRUE == IsDlgButtonChecked( hwndDlg, IDC_CHECK_KEYHELP ) )
		m_Types.m_bUseKeyWordHelp = TRUE;
	else
		m_Types.m_bUseKeyWordHelp = FALSE;
	if( TRUE == IsDlgButtonChecked( hwndDlg, IDC_CHECK_KEYHELP_ALLSEARCH ) )
		m_Types.m_bUseKeyHelpAllSearch = TRUE;
	else
		m_Types.m_bUseKeyHelpAllSearch = FALSE;
	if( TRUE == IsDlgButtonChecked( hwndDlg, IDC_CHECK_KEYHELP_KEYDISP ) )
		m_Types.m_bUseKeyHelpKeyDisp = TRUE;
	else
		m_Types.m_bUseKeyHelpKeyDisp = FALSE;
	if( TRUE == IsDlgButtonChecked( hwndDlg, IDC_CHECK_KEYHELP_PREFIX ) )
		m_Types.m_bUseKeyHelpPrefix = TRUE;
	else
		m_Types.m_bUseKeyHelpPrefix = FALSE;
	/* リストに登録されている情報を配列に取り込む */
	hwndList = GetDlgItem( hwndDlg, IDC_LIST_KEYHELP );
	nIndex = ListView_GetItemCount( hwndList );
	for(i = 0; i < MAX_KEYHELP_FILE; i++){
		if( i < nIndex ){
			bool		bUse = false;						/* 辞書ON(1)/OFF(0) */
			szAbout[0]	= _T('\0');
			szPath[0]	= _T('\0');
			/* チェックボックス状態を取得してbUseにセット */
			if(ListView_GetCheckState(hwndList, i))
				bUse = true;
			ListView_GetItemText( hwndList, i, 1, szAbout, _countof(szAbout) );
			ListView_GetItemText( hwndList, i, 2, szPath, _countof(szPath) );
			m_Types.m_KeyHelpArr[i].m_bUse = bUse;
			_tcscpy(m_Types.m_KeyHelpArr[i].m_szAbout, szAbout);
			_tcscpy(m_Types.m_KeyHelpArr[i].m_szPath, szPath);
		}else{	/* 未登録部分はクリアする */
			m_Types.m_KeyHelpArr[i].m_szPath[0] = _T('\0');
		}
	}
	/* 辞書の冊数を取得 */
	m_Types.m_nKeyHelpNum = nIndex;
	return TRUE;
}

/*! キーワードヘルプファイルリストのインポート

	@date 2006.04.10 fon 新規作成
*/
BOOL CPropTypes::Import_KeyHelp(HWND hwndDlg)
{
	CDlgOpenFile	cDlgOpenFile;
	TCHAR			szPath[_MAX_PATH + 1]=_T("");
	/* ファイルオープンダイアログの初期化 */
	cDlgOpenFile.Create(
		m_hInstance,
		hwndDlg,
		_T("*.txt"),
		GetDllShareData().m_szIMPORTFOLDER	// インポート用フォルダ
	);
	if( !cDlgOpenFile.DoModal_GetOpenFileName( szPath ) )
		return FALSE;

	/* ファイルのフルパスを、フォルダとファイル名に分割 */	/* [c:\work\test\aaa.txt] → [c:\work\test] + [aaa.txt] */
	::SplitPath_FolderAndFile( szPath, GetDllShareData().m_szIMPORTFOLDER, NULL );
	_tcscat( GetDllShareData().m_szIMPORTFOLDER, _T("\\") );

	CTextInputStream in(szPath);
	if(!in){
		ErrorMessage( hwndDlg, _T("ファイルを開けませんでした。\n\n%ts"), szPath );
		return FALSE;
	}

	/* LIST内のデータ全削除 */
	HWND hwndWork = ::GetDlgItem( hwndDlg, IDC_LIST_KEYHELP );
	ListView_DeleteAllItems(hwndWork);  /* リストを空にする */
	GetData_KeyHelp(hwndDlg);
	
	int invalid_record = 0; // 不正な行
	/* データ取得 */
	int i=0;
	while(in && i<MAX_KEYHELP_FILE){
		wstring buff=in.ReadLineW();
		
		// 2007.02.03 genta コメントみたいな行は黙ってスキップ
		// 2007.10.08 kobake 空行もスキップ
		if( buff[0] == LTEXT('\0') ||
			buff[0] == LTEXT('\n') ||
			buff[0] == LTEXT('#') ||
			buff[0] == LTEXT(';') ||
			( buff[0] == LTEXT('/') && buff[1] == LTEXT('/') )){
				//	2007.02.03 genta 処理を継続
				continue;
			}
		
		//KDct[99]=ON/OFF,DictAbout,KeyHelpPath
		if( buff.length() < 10 ||
			auto_memcmp(buff.c_str(), LTEXT("KDct["), 5) != 0 ||
			auto_memcmp(&buff[7], LTEXT("]="), 2) != 0 ||
			0 ){
			//	2007.02.03 genta 処理を継続
			++invalid_record;
			continue;
		}

		WCHAR *p1, *p2, *p3;
		p1 = &buff[9];
		p3 = p1;					//結果確認用に初期化
		if( NULL != (p2=wcsstr(p1,LTEXT(","))) ){
			*p2 = LTEXT('\0');
			p2 += 1;				//カンマの次が、次の要素
			if( NULL != (p3=wcsstr(p2,LTEXT(","))) ){
				*p3 = LTEXT('\0');
				p3 += 1;			//カンマの次が、次の要素
			}
		}/* 結果の確認 */
		if( (p3==NULL) ||			//カンマが1個足りない
			(p3==p1) ||				//カンマが2個足りない
			//	2007.02.03 genta ファイル名にカンマがあるかもしれない
			0 //(NULL!=wcsstr(p3,","))	//カンマが多すぎる
		){
			//	2007.02.03 genta 処理を継続
			++invalid_record;
			continue;
		}
		/* valueのチェック */
		//ON/OFF
		//	2007.02.03 genta 1でなければ1にする
		unsigned int b_enable_flag = (unsigned int)_wtoi(p1);
		if( b_enable_flag > 1){
			b_enable_flag = 1;
		}
		//Path
		FILE* fp2;
		if( (fp2=_tfopen_absini(to_tchar(p3),_T("r"))) == NULL ){	// 2007.02.03 genta 相対パスはsakura.exe基準で開く	// 2007.05.19 ryoji 相対パスは設定ファイルからのパスを優先
			// 2007.02.03 genta 辞書が見つからない場合の措置．警告を出すが取り込む
			p2 = L"【辞書ファイルが見つかりません】";
			b_enable_flag = 0;
		}
		else
			fclose(fp2);

		//About
		if(wcslen(p2)>DICT_ABOUT_LEN){
			ErrorMessage( hwndDlg, _T("辞書の説明は%d文字以内にしてください。\n"), DICT_ABOUT_LEN );
			++invalid_record;
			continue;
		}

		//良さそうなら
		m_Types.m_KeyHelpArr[i].m_bUse = (b_enable_flag!=0);	// 2007.02.03 genta
		_tcscpy(m_Types.m_KeyHelpArr[i].m_szAbout, to_tchar(p2));
		_tcscpy(m_Types.m_KeyHelpArr[i].m_szPath, to_tchar(p3));
		i++;
	}
	in.Close();

	/*データのセット*/
	SetData_KeyHelp(hwndDlg);
	// 2007.02.03 genta 失敗したら警告する
	if( invalid_record > 0 ){
		::MYMESSAGEBOX( hwndDlg, MB_OK | MB_ICONWARNING, GSTR_APPNAME,
		_T("一部のデータが読み込めませんでした\n不正な行数: %d"),
		invalid_record );
	}
	return TRUE;
}


/*! キーワードヘルプファイルリストのインポートエクスポート

	@date 2006.04.10 fon 新規作成
*/
BOOL CPropTypes::Export_KeyHelp(HWND hwndDlg)
{
	/* ファイルオープンダイアログの初期化 */
	CDlgOpenFile	cDlgOpenFile;
	TCHAR			szXPath[_MAX_PATH + 1]=_T("");
	cDlgOpenFile.Create(
		m_hInstance,
		hwndDlg,
		_T("*.txt"),
		GetDllShareData().m_szIMPORTFOLDER	// インポート用フォルダ
	);
	if( !cDlgOpenFile.DoModal_GetSaveFileName( szXPath ) ){
		return FALSE;
	}

	/* ファイルのフルパスを、フォルダとファイル名に分割 */	/* [c:\work\test\aaa.txt] → [c:\work\test] + [aaa.txt] */
	::SplitPath_FolderAndFile( szXPath, GetDllShareData().m_szIMPORTFOLDER, NULL );
	_tcscat( GetDllShareData().m_szIMPORTFOLDER, _T("\\") );

	CTextOutputStream out(szXPath);
	if(!out){
		ErrorMessage( hwndDlg, _T("ファイルを開けませんでした。\n\n%ts"), szXPath );
		return FALSE;
	}

	out.WriteF( L"// キーワード辞書設定 Ver1\n" );

	GetData_KeyHelp(hwndDlg);
	HWND hwndList = GetDlgItem( hwndDlg, IDC_LIST_KEYHELP );
	int j = ListView_GetItemCount(hwndList);

	for(int i = 0; i < j; i++){
		out.WriteF(
			L"KDct[%02d]=%d,%ts,%ts\n",
			i,
			m_Types.m_KeyHelpArr[i].m_bUse?1:0,
			m_Types.m_KeyHelpArr[i].m_szAbout,
			m_Types.m_KeyHelpArr[i].m_szPath.c_str()
		);
	}
	out.Close();

	InfoMessage( hwndDlg, _T("ファイルへエクスポートしました。\n\n%ts"), szXPath );
	return TRUE;
}


/*! 辞書の説明のフォーマット揃え

	@date 2006.04.10 fon 新規作成
*/
static TCHAR* strcnv(TCHAR *str)
{
	TCHAR* p=str;
	/* 改行コードの削除 */
	if( NULL != (p=_tcschr(p,_T('\n'))) )
		*p=_T('\0');
	p=str;
	if( NULL != (p=_tcschr(p,_T('\r'))) )
		*p=_T('\0');
	/* カンマの置換 */
	p=str;
	for(; (p=_tcschr(p,_T(','))) != NULL; ){
		*p=_T('.');
	}
	return str;
}

/*! フルパスからファイル名を返す

	@date 2006.04.10 fon 新規作成
	@date 2006.09.14 genta ディレクトリがない場合に最初の1文字が切れないように
*/
static TCHAR* GetFileName(const TCHAR* fullpath)
{
	const TCHAR* pszName = fullpath;
	CharPointerT p = fullpath;
	while( *p != _T('\0')  ){
		if( *p == _T('\\') ){
			pszName = p + 1;
			p++;
		}else{
			p++;
		}
	}
	return const_cast<TCHAR*>(pszName);
}


