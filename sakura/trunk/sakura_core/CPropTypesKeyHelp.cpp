/*!	@file
	タイプ別設定キーワードヘルプページ

	@author fon
	@date 2006/04/10 新規作成
*/
/*
	Copyright (C) 2006, fon, ryoji, genta
	Copyright (C) 2007, ryoji, genta

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

#include "StdAfx.h"
#include "global.h"
#include "CPropTypes.h"
#include "Debug.h"
#include "CDlgOpenFile.h"
#include "etc_uty.h"
#include "shell.h"
#include "file.h"
#include "charcode.h"
#include "sakura_rc.h"
#include "sakura.hh"

static const DWORD p_helpids[] = {	// 2006.10.10 ryoji
	IDC_CHECK_KEYHELP,				HIDC_CHECK_KEYHELP,				//キーワードヘルプ機能を使う
	IDC_LIST_KEYHELP,				HIDC_LIST_KEYHELP,				//SysListView32
	IDC_BUTTON_KEYHELP_UPD,			HIDC_BUTTON_KEYHELP_UPD,		//更新(&E)
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

//SDKにしか定義されていない。
#ifndef	ListView_SetCheckState
//#if (_WIN32_IE >= 0x0300)
#define ListView_SetCheckState(hwndLV, i, fCheck) \
  ListView_SetItemState(hwndLV, i, INDEXTOSTATEIMAGEMASK((fCheck)?2:1), LVIS_STATEIMAGEMASK)
//#endif
#endif

static TCHAR* strcnv(TCHAR *str);
static TCHAR* GetFileName(const TCHAR *fullpath);

/*! キーワード辞書ファイル設定 メッセージ処理

	@date 2006.04.10 fon 新規作成
*/
INT_PTR CPropTypesKeyHelp::DispatchEvent(
	HWND		hwndDlg,	// handle to dialog box
	UINT		uMsg,		// message
	WPARAM		wParam,		// first message parameter
	LPARAM		lParam 		// second message parameter
)
{
	WORD	wNotifyCode;
	WORD	wID;
	HWND	hwndList;
	NMHDR*	pNMHDR;
	int		nIndex, nIndex2;
	LV_ITEM	lvi;
	LV_COLUMN	col;
	RECT		rc;

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
		col.pszText  = const_cast<TCHAR*>(_T("   辞書ファイル"));	/* 指定辞書ファイルの使用可否 */
		col.iSubItem = 0;
		ListView_InsertColumn( hwndList, 0, &col );
		col.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		col.fmt      = LVCFMT_LEFT;
		col.cx       = (rc.right - rc.left) * 55 / 100;
		col.pszText  = const_cast<TCHAR*>(_T("辞書の説明"));		/* 指定辞書の１行目を取得 */
		col.iSubItem = 1;
		ListView_InsertColumn( hwndList, 1, &col );
		col.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		col.fmt      = LVCFMT_LEFT;
		col.cx       = (rc.right - rc.left) * 18 / 100;
		col.pszText  = const_cast<TCHAR*>(_T("パス"));				/* 指定辞書ファイルパス */
		col.iSubItem = 2;
		ListView_InsertColumn( hwndList, 2, &col );
		SetData( hwndDlg );	/* ダイアログデータの設定 辞書ファイル一覧 */

		/* リストがあれば先頭をフォーカスする */
		if(ListView_GetItemCount(hwndList) > 0){
			ListView_SetItemState( hwndList, 0, LVIS_SELECTED /*| LVIS_FOCUSED*/, LVIS_SELECTED /*| LVIS_FOCUSED*/ );
		}
		/* リストがなければ初期値として用途を表示 */
		else{
			::SetDlgItemText( hwndDlg, IDC_LABEL_KEYHELP_ABOUT, _T("辞書ファイルの１行目の文字列") );
			::SetDlgItemText( hwndDlg, IDC_EDIT_KEYHELP, _T("キーワード辞書ファイル パス") );
		}

		/* 初期状態を設定 */
		SendMessage(hwndDlg, WM_COMMAND, (WPARAM)MAKELONG(IDC_CHECK_KEYHELP,BN_CLICKED), 0 );

		return TRUE;

	case WM_COMMAND:
		wNotifyCode = HIWORD(wParam);	/* 通知コード */
		wID	= LOWORD(wParam);			/* 項目ID､ コントロールID､ またはアクセラレータID */

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
					}
				}else{								/* 更新 */
					if( -1 == nIndex ){
						ErrorMessage( hwndDlg, _T("更新する辞書をリストから選択してください。"));
						return FALSE;
					}
				}
				/* 更新するキー情報を取得する。 */
				memset(szPath, 0, _countof(szPath));
				::GetDlgItemText( hwndDlg, IDC_EDIT_KEYHELP, szPath, _countof(szPath) );
				if( szPath[0] == _T('\0') ) return FALSE;
				/* 重複検査 */
				nIndex2 = ListView_GetItemCount(hwndList);
				TCHAR szPath2[_MAX_PATH];
				int i;
				for(i = 0; i < nIndex2; i++){
					memset(szPath2, 0, _countof(szPath2));
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
					FILE* fp;
					if( (fp=_tfopen_absini(szPath,"r")) == NULL ){	// 2006.02.01 genta 本体からの相対パスを受け付ける	// 2007.05.19 ryoji 相対パスは設定ファイルからのパスを優先
						ErrorMessage( hwndDlg, _T("ファイルを開けませんでした。\n\n%s"), szPath );
						return FALSE;
					}
					// 開けたなら1行目を取得してから閉じる
					fgets( szAbout, _countof(szAbout), fp );
					fclose( fp );
				}
				strcnv(szAbout);

				/* ついでに辞書の説明を更新 */
				::SetDlgItemText( hwndDlg, IDC_LABEL_KEYHELP_ABOUT, szAbout );	/* 辞書ファイルの概要 */
				
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
				GetData( hwndDlg );
				return TRUE;

			case IDC_BUTTON_KEYHELP_DEL:	/* 削除 */
				/* 選択中のキー番号を探す。 */
				nIndex = ListView_GetNextItem( hwndList, -1, LVNI_ALL | LVNI_SELECTED );
				if( -1 == nIndex ) return FALSE;
				/* 削除する。 */
				ListView_DeleteItem( hwndList, nIndex );
				/* リストがなくなったら初期値として用途を表示 */
				if(ListView_GetItemCount(hwndList) == 0){
					::SetDlgItemText( hwndDlg, IDC_LABEL_KEYHELP_ABOUT, _T("辞書ファイルの１行目の文字列") );
					::SetDlgItemText( hwndDlg, IDC_EDIT_KEYHELP, _T("キーワード辞書ファイル パス") );
				}/* リストの最後を削除した場合は、削除後のリストの最後を選択する。 */
				else if(nIndex > ListView_GetItemCount(hwndList)-1){
					ListView_SetItemState( hwndList, ListView_GetItemCount(hwndList)-1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
				}/* 同じ位置のキーを選択状態にする。 */
				else{
					ListView_SetItemState( hwndList, nIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
				}
				GetData( hwndDlg );
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
				GetData( hwndDlg );
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
				GetData( hwndDlg );
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
				GetData( hwndDlg );
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
				GetData( hwndDlg );
				return TRUE;

			case IDC_BUTTON_KEYHELP_REF:	/* キーワードヘルプ 辞書ファイルの「参照...」ボタン */
				{
					CDlgOpenFile	cDlgOpenFile;
					/* ファイルオープンダイアログの初期化 */
					// 2007.05.19 ryoji 相対パスは設定ファイルからのパスを優先
					TCHAR szWk[_MAX_PATH];
					::GetDlgItemText( hwndDlg, IDC_EDIT_KEYHELP, szWk, _MAX_PATH );
					if( _IS_REL_PATH( szWk ) ){
						GetInidirOrExedir( szPath, szWk );
					}else{
						::lstrcpy( szPath, szWk );
					}
					cDlgOpenFile.Create( m_hInstance, hwndDlg, _T("*.khp"), szPath );
					if( cDlgOpenFile.DoModal_GetOpenFileName( szPath ) ){
						::SetDlgItemText( hwndDlg, IDC_EDIT_KEYHELP, szPath );
					}
				}
				return TRUE;

			case IDC_BUTTON_KEYHELP_IMPORT:	/* インポート */
				Import(hwndDlg);
				m_Types.m_nKeyHelpNum = ListView_GetItemCount( hwndList );
				return TRUE;

			case IDC_BUTTON_KEYHELP_EXPORT:	/* エクスポート */
				Export(hwndDlg);
				return TRUE;
			}
			break;
		}
		break;

	case WM_NOTIFY:
		pNMHDR = (NMHDR*)lParam;

		switch( pNMHDR->code ){
		case PSN_HELP:
			OnHelp( hwndDlg, IDD_PROP_KEYHELP );
			return TRUE;

		case PSN_KILLACTIVE:
			/* ダイアログデータの取得 辞書ファイルリスト */
			GetData( hwndDlg );
			return TRUE;

//@@@ 2002.01.03 YAZAKI 最後に表示していたシートを正しく覚えていないバグ修正
		case PSN_SETACTIVE:
			m_nPageNum = ID_PROPTYPE_PAGENUM_KEYHELP;
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
				::SetDlgItemText( hwndDlg, IDC_LABEL_KEYHELP_ABOUT, szAbout );	/* 辞書の説明 */
				::SetDlgItemText( hwndDlg, IDC_EDIT_KEYHELP, szPath );			/* ファイルパス */
			}
			break;
		}
		break;

	case WM_HELP:
		{	HELPINFO *p = (HELPINFO *)lParam;
			MyWinHelp( (HWND)p->hItemHandle, HELP_WM_HELP, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		}return TRUE;

	case WM_CONTEXTMENU:	/* Context Menu */
		MyWinHelp( hwndDlg, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		return TRUE;
	}
	return FALSE;
}

void CheckDlgButtonBOOL(HWND hwnd, int id, BOOL bState ){
	CheckDlgButton( hwnd, id, (bState ? BST_CHECKED : BST_UNCHECKED) );
}

/*! ダイアログデータの設定 キーワード辞書ファイル設定

	@date 2006.04.10 fon 新規作成
*/
void CPropTypesKeyHelp::SetData( HWND hwndDlg )
{
	HWND	hwndWork;
	int		i;
	LV_ITEM	lvi;
	DWORD	dwStyle;

	/* ユーザーがエディット コントロールに入力できるテキストの長さを制限する */
	::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT_KEYHELP ), EM_LIMITTEXT, (WPARAM)(sizeof( m_Types.m_KeyHelpArr[0].m_szPath ) - 1 ), (LPARAM)0 );

	// 使用する・使用しない
	CheckDlgButtonBOOL( hwndDlg, IDC_CHECK_KEYHELP, m_Types.m_bUseKeyWordHelp );
	CheckDlgButtonBOOL( hwndDlg, IDC_CHECK_KEYHELP_ALLSEARCH, m_Types.m_bUseKeyHelpAllSearch );
	CheckDlgButtonBOOL( hwndDlg, IDC_CHECK_KEYHELP_KEYDISP, m_Types.m_bUseKeyHelpKeyDisp );
	CheckDlgButtonBOOL( hwndDlg, IDC_CHECK_KEYHELP_PREFIX, m_Types.m_bUseKeyHelpPrefix );

	/* リスト */
	hwndWork = ::GetDlgItem( hwndDlg, IDC_LIST_KEYHELP );
	ListView_DeleteAllItems(hwndWork);  /* リストを空にする */
	/* 行選択 */
	dwStyle = ListView_GetExtendedListViewStyle( hwndWork );
	dwStyle |= LVS_EX_FULLROWSELECT;
	ListView_SetExtendedListViewStyle( hwndWork, dwStyle );
	/* データ表示 */
	for(i = 0; i < MAX_KEYHELP_FILE; i++){
		if( m_Types.m_KeyHelpArr[i].m_szPath[0] == _T('\0') ) break;
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
		if(1 == m_Types.m_KeyHelpArr[i].m_nUse){	// ON
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
int CPropTypesKeyHelp::GetData( HWND hwndDlg )
{
	HWND	hwndList;
	int	nIndex, i;
	TCHAR	szAbout[DICT_ABOUT_LEN];	/* 辞書の説明(辞書ファイルの1行目から生成) */
	TCHAR	szPath[_MAX_PATH];			/* ファイルパス */

	/* 使用する・使用しない */
	m_Types.m_bUseKeyWordHelp = ( BST_CHECKED == IsDlgButtonChecked( hwndDlg, IDC_CHECK_KEYHELP ) ); 	
	m_Types.m_bUseKeyHelpAllSearch = ( BST_CHECKED == IsDlgButtonChecked( hwndDlg, IDC_CHECK_KEYHELP_ALLSEARCH ) );
	m_Types.m_bUseKeyHelpKeyDisp = ( BST_CHECKED == IsDlgButtonChecked( hwndDlg, IDC_CHECK_KEYHELP_KEYDISP ) );
	m_Types.m_bUseKeyHelpPrefix = ( BST_CHECKED == IsDlgButtonChecked( hwndDlg, IDC_CHECK_KEYHELP_PREFIX ) );

	/* リストに登録されている情報を配列に取り込む */
	hwndList = GetDlgItem( hwndDlg, IDC_LIST_KEYHELP );
	nIndex = ListView_GetItemCount( hwndList );
	for(i = 0; i < MAX_KEYHELP_FILE; i++){
		if( i < nIndex ){
			int nUse	= 0;	/* 辞書ON(1)/OFF(0) */
			szAbout[0]	= _T('\0');
			szPath[0]	= _T('\0');
			/* チェックボックス状態を取得してnUseにセット */
			if(ListView_GetCheckState(hwndList, i))
				nUse = 1;
			ListView_GetItemText( hwndList, i, 1, szAbout, _countof(szAbout) );
			ListView_GetItemText( hwndList, i, 2, szPath, _countof(szPath) );
			m_Types.m_KeyHelpArr[i].m_nUse = nUse;
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
bool CPropTypesKeyHelp::Import(HWND hwndDlg)
{
	int		i, j;
	char	buff[sizeof(int)+DICT_ABOUT_LEN+_MAX_PATH+sizeof("KDct[99]=,,\r\n")];

	CDlgOpenFile	cDlgOpenFile;
	char	szPath[_MAX_PATH + 1];
	char	szInitDir[_MAX_PATH + 1];
	_tcscpy( szPath, _T("") );
	_tcscpy( szInitDir, m_pShareData->m_sHistory.m_szIMPORTFOLDER );	/* インポート用フォルダ */
	/* ファイルオープンダイアログの初期化 */
	cDlgOpenFile.Create( m_hInstance, hwndDlg, _T("*.txt"), szInitDir );
	if( !cDlgOpenFile.DoModal_GetOpenFileName( szPath ) )
		return false;
	/* ファイルのフルパスを、フォルダとファイル名に分割 */	/* [c:\work\test\aaa.txt] → [c:\work\test] + [aaa.txt] */
	::SplitPath_FolderAndFile( szPath, m_pShareData->m_sHistory.m_szIMPORTFOLDER, NULL );
	strcat( m_pShareData->m_sHistory.m_szIMPORTFOLDER, "\\" );

	FILE		*fp;
	if( (fp = fopen(szPath, "r")) == NULL ){
		ErrorMessage( hwndDlg, _T("ファイルを開けませんでした。\n\n%s"), szPath );
		return false;
	}
	/* LIST内のデータ全削除 */
	HWND hwndWork = ::GetDlgItem( hwndDlg, IDC_LIST_KEYHELP );
	ListView_DeleteAllItems(hwndWork);  /* リストを空にする */
	GetData(hwndDlg);
	
	int invalid_record = 0; // 不正な行
	/* データ取得 */
	for(i=0; (NULL!=fgets(buff,sizeof(buff),fp))&&(i<MAX_KEYHELP_FILE); ){
		for(j = strlen(buff) - 1; j >= 0; j--){
			if( buff[j] == '\r' || buff[j] == '\n' ) buff[j] = '\0';
		}
		// 2007.02.03 genta コメントみたいな行は黙ってスキップ
		if( buff[0] == '\n' ||
			buff[0] == '#' ||
			buff[0] == ';' ||
			( buff[0] == '/' && buff[1] == '/' )){
				//	2007.02.03 genta 処理を継続
				continue;
			}
		
		//KDct[99]=ON/OFF,DictAbout,KeyHelpPath
		if( strlen(buff) < 10 ||
			memcmp(buff, "KDct[", 5) != 0 ||
			memcmp(&buff[7], "]=", 2) != 0 ||
			0 ){
			//	2007.02.03 genta 処理を継続
			++invalid_record;
			continue;
		}

		char *p1, *p2, *p3;
		p1 = &buff[9];
		p3 = p1;					//結果確認用に初期化
		if( NULL != (p2=strstr(p1,",")) ){
			*p2 = '\0';
			p2 += 1;				//カンマの次が、次の要素
			if( NULL != (p3=strstr(p2,",")) ){
				*p3 = '\0';
				p3 += 1;			//カンマの次が、次の要素
			}
		}/* 結果の確認 */
		if( (p3==NULL) ||			//カンマが1個足りない
			(p3==p1) //||			//カンマが2個足りない
			//	2007.02.03 genta ファイル名にカンマがあるかもしれない
			//(NULL!=strstr(p3,","))	//カンマが多すぎる
		){
			//	2007.02.03 genta 処理を継続
			++invalid_record;
			continue;
		}
		/* valueのチェック */
		//ON/OFF
		//	2007.02.03 genta 1でなければ1にする
		unsigned int b_enable_flag = (unsigned int)atoi(p1);
		if( b_enable_flag > 1){
			b_enable_flag = 1;
		}
		//Path
		FILE* fp2;
		const char* p4 = p2;
		if( (fp2=_tfopen_absini(p3,"r")) == NULL ){	// 2007.02.03 genta 相対パスはsakura.exe基準で開く	// 2007.05.19 ryoji 相対パスは設定ファイルからのパスを優先
			// 2007.02.03 genta 辞書が見つからない場合の措置．警告を出すが取り込む
			p4 = "【辞書ファイルが見つかりません】";
			b_enable_flag = 0;
		}
		else
			fclose(fp2);

		//About
		if(strlen(p2) > DICT_ABOUT_LEN){
			ErrorMessage( hwndDlg, _T("辞書の説明は%d文字以内にしてください。\n"), DICT_ABOUT_LEN );
			++invalid_record;
			continue;
		}

		//良さそうなら
		m_Types.m_KeyHelpArr[i].m_nUse = b_enable_flag;	// 2007.02.03 genta
		_tcscpy(m_Types.m_KeyHelpArr[i].m_szAbout, p4);
		_tcscpy(m_Types.m_KeyHelpArr[i].m_szPath, p3);
		i++;
	}
	fclose(fp);
	/*データのセット*/
	SetData(hwndDlg);
	// 2007.02.03 genta 失敗したら警告する
	if( invalid_record > 0 ){
		WarningMessage( hwndDlg,
		_T("一部のデータが読み込めませんでした\n不正な行数: %d"),
		invalid_record );
	}
	return true;
}


/*! キーワードヘルプファイルリストのインポートエクスポート

	@date 2006.04.10 fon 新規作成
*/
bool CPropTypesKeyHelp::Export(HWND hwndDlg)
{
	int		i, j;

	CDlgOpenFile	cDlgOpenFile;
	char	szXPath[_MAX_PATH + 1];
	char	szInitDir[_MAX_PATH + 1];
	_tcscpy( szXPath, "" );
	_tcscpy( szInitDir, m_pShareData->m_sHistory.m_szIMPORTFOLDER );	/* インポート用フォルダ */
	/* ファイルオープンダイアログの初期化 */
	cDlgOpenFile.Create( m_hInstance, hwndDlg, _T("*.txt"), szInitDir );
	if( !cDlgOpenFile.DoModal_GetSaveFileName( szXPath ) ){
		return false;
	}
	/* ファイルのフルパスを、フォルダとファイル名に分割 */	/* [c:\work\test\aaa.txt] → [c:\work\test] + [aaa.txt] */
	::SplitPath_FolderAndFile( szXPath, m_pShareData->m_sHistory.m_szIMPORTFOLDER, NULL );
	strcat( m_pShareData->m_sHistory.m_szIMPORTFOLDER, "\\" );
	FILE		*fp;
	if( (fp = fopen(szXPath, "w")) == NULL ){
		ErrorMessage( hwndDlg, _T("ファイルを開けませんでした。\n\n%s"), szXPath );
		return false;
	}

	fprintf(fp, "// キーワード辞書設定 Ver1\n");

	GetData(hwndDlg);
	HWND hwndList = GetDlgItem( hwndDlg, IDC_LIST_KEYHELP );
	j = ListView_GetItemCount(hwndList);

	static const char* pszForm = "KDct[%02d]=%d,%s,%s\n";
	for(i = 0; i < j; i++){
		fprintf( fp, pszForm,
			i,
			m_Types.m_KeyHelpArr[i].m_nUse,
			m_Types.m_KeyHelpArr[i].m_szAbout,
			m_Types.m_KeyHelpArr[i].m_szPath
		);
	}
	fclose(fp);

	InfoMessage( hwndDlg,
		_T("ファイルへエクスポートしました。\n\n%s"), szXPath
	);
	return true;
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
	const TCHAR *p, *pszName;
	pszName = p = fullpath;
	while( *p != _T('\0')  ){
		if( _IS_SJIS_1( (unsigned char)*p ) && _IS_SJIS_2( (unsigned char)p[1] ) ){
			p+=2;
		}else if( *p == _T('\\') ){
			pszName = p + 1;
			p++;
		}else{
			p++;
		}
	}
	return const_cast<TCHAR*>(pszName);
}

/*[EOF]*/
