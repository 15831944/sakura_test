/*!	@file
	@brief アウトライン解析ダイアログボックス

	@author Norio Nakatani

	@date 2001/06/23 N.Nakatani Visual Basicのアウトライン解析
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, genta
	Copyright (C) 2001, Stonee, JEPRO, genta, hor
	Copyright (C) 2002, MIK, aroka, hor, genta, YAZAKI, Moca, frozen
	Copyright (C) 2003, zenryaku, Moca, naoh, little YOSHI, genta,
	Copyright (C) 2004, zenryaku, Moca, novice
	Copyright (C) 2005, genta, zenryaku, ぜっと, D.S.Koba
	Copyright (C) 2006, genta, aroka, ryoji, Moca
	Copyright (C) 2006, genta, ryoji
	Copyright (C) 2007, ryoji
	Copyright (C) 2010, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "StdAfx.h"
#include "outline/CDlgFuncList.h"
#include "outline/CFuncInfoArr.h"// 2002/2/3 aroka
#include "window/CEditWnd.h"	//	2006/2/11 aroka 追加
#include "util/shell.h"
#include "util/os.h"
#include "util/input.h"
#include "util/window.h"
#include "view/colors/CColorStrategy.h"
#include "env/CAppNodeManager.h"
#include "CUxTheme.h"
#include "sakura_rc.h"
#include "sakura.hh"

// 画面ドッキング用の定義	// 2010.06.05 ryoji
#define DEFINE_SYNCCOLOR
#define DOCK_SPLITTER_WIDTH		DpiScaleX(5)
#define DOCK_MIN_SIZE			DpiScaleX(60)
#define DOCK_BUTTON_NUM			(3)

//アウトライン解析 CDlgFuncList.cpp	//@@@ 2002.01.07 add start MIK
const DWORD p_helpids[] = {	//12200
	IDC_BUTTON_COPY,					HIDC_FL_BUTTON_COPY,	//コピー
	IDOK,								HIDOK_FL,				//ジャンプ
	IDCANCEL,							HIDCANCEL_FL,			//キャンセル
	IDC_BUTTON_HELP,					HIDC_FL_BUTTON_HELP,	//ヘルプ
	IDC_CHECK_bAutoCloseDlgFuncList,	HIDC_FL_CHECK_bAutoCloseDlgFuncList,	//自動的に閉じる
	IDC_LIST_FL,						HIDC_FL_LIST1,			//トピックリスト	IDC_LIST1->IDC_LIST_FL	2008/7/3 Uchi
	IDC_TREE_FL,						HIDC_FL_TREE1,			//トピックツリー	IDC_TREE1->IDC_TREE_FL	2008/7/3 Uchi
	IDC_CHECK_bFunclistSetFocusOnJump,	HIDC_FL_CHECK_bFunclistSetFocusOnJump,	//ジャンプでフォーカス移動する
	IDC_CHECK_bMarkUpBlankLineEnable,	HIDC_FL_CHECK_bMarkUpBlankLineEnable,	//空行を無視する
	IDC_COMBO_nSortType,				HIDC_COMBO_nSortType,	//順序
	IDC_BUTTON_WINSIZE,					HIDC_FL_BUTTON_WINSIZE,	//ウィンドウ位置保存	// 2006.08.06 ryoji
	IDC_BUTTON_MENU,					HIDC_FL_BUTTON_MENU,	//ウィンドウの位置メニュー
//	IDC_STATIC,							-1,
	0, 0
};	//@@@ 2002.01.07 add end MIK

//関数リストの列
enum EFuncListCol {
	FL_COL_ROW		= 0,	//行
	FL_COL_COL		= 1,	//桁
	FL_COL_NAME		= 2,	//関数名
	FL_COL_REMARK	= 3		//備考
};

/*! ソート比較用プロシージャ */
int CALLBACK _CompareFunc_( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	CFuncInfo*		pcFuncInfo1;
	CFuncInfo*		pcFuncInfo2;
	CDlgFuncList*	pcDlgFuncList;
	pcDlgFuncList = (CDlgFuncList*)lParamSort;

	pcFuncInfo1 = pcDlgFuncList->m_pcFuncInfoArr->GetAt( lParam1 );
	if( NULL == pcFuncInfo1 ){
		return -1;
	}
	pcFuncInfo2 = pcDlgFuncList->m_pcFuncInfoArr->GetAt( lParam2 );
	if( NULL == pcFuncInfo2 ){
		return -1;
	}
	//	Apr. 23, 2005 genta 行番号を左端へ
	if( FL_COL_NAME == pcDlgFuncList->m_nSortCol){	/* 名前でソート */
		return auto_stricmp( pcFuncInfo1->m_cmemFuncName.GetStringPtr(), pcFuncInfo2->m_cmemFuncName.GetStringPtr() );
	}
	//	Apr. 23, 2005 genta 行番号を左端へ
	if( FL_COL_ROW == pcDlgFuncList->m_nSortCol){	/* 行（＋桁）でソート */
		if( pcFuncInfo1->m_nFuncLineCRLF < pcFuncInfo2->m_nFuncLineCRLF ){
			return -1;
		}else
		if( pcFuncInfo1->m_nFuncLineCRLF == pcFuncInfo2->m_nFuncLineCRLF ){
			if( pcFuncInfo1->m_nFuncColCRLF < pcFuncInfo2->m_nFuncColCRLF ){
				return -1;
			}else
			if( pcFuncInfo1->m_nFuncColCRLF == pcFuncInfo2->m_nFuncColCRLF ){
				return 0;
			}else{
				return 1;
			}
		}else{
			return 1;
		}
	}
	if( FL_COL_COL == pcDlgFuncList->m_nSortCol){	/* 桁でソート */
		if( pcFuncInfo1->m_nFuncColCRLF < pcFuncInfo2->m_nFuncColCRLF ){
			return -1;
		}else
		if( pcFuncInfo1->m_nFuncColCRLF == pcFuncInfo2->m_nFuncColCRLF ){
			return 0;
		}else{
			return 1;
		}
	}
	// From Here 2001.12.07 hor
	if( FL_COL_REMARK == pcDlgFuncList->m_nSortCol){	/* 備考でソート */
		if( pcFuncInfo1->m_nInfo < pcFuncInfo2->m_nInfo ){
			return -1;
		}else
		if( pcFuncInfo1->m_nInfo == pcFuncInfo2->m_nInfo ){
			return 0;
		}else{
			return 1;
		}
	}
	// To Here 2001.12.07 hor
	return -1;
}

LPDLGTEMPLATE CDlgFuncList::m_pDlgTemplate = NULL;
DWORD CDlgFuncList::m_dwDlgTmpSize = 0;

CDlgFuncList::CDlgFuncList()
{
	m_pcFuncInfoArr = NULL;		/* 関数情報配列 */
	m_nCurLine = CLayoutInt(0);				/* 現在行 */
	m_nListType = OUTLINE_DEFAULT;
	//	Apr. 23, 2005 genta 行番号を左端へ
	m_nSortCol = 0;				/* ソートする列番号 2004.04.06 zenryaku 標準は行番号(1列目) */
	m_bLineNumIsCRLF = false;	/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
	m_bWaitTreeProcess = false;	// 2002.02.16 hor Treeのダブルクリックでフォーカス移動できるように 2/4
	m_nSortType = 0;
	m_cFuncInfo = NULL;			/* 現在の関数情報 */
	m_bEditWndReady = false;	/* エディタ画面の準備完了 */
	m_bInChangeLayout = false;
}


/*!
	標準以外のメッセージを捕捉する

	@date 2007.11.07 ryoji 新規
*/
INT_PTR CDlgFuncList::DispatchEvent( HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
	INT_PTR result;
	result = CDialog::DispatchEvent( hWnd, wMsg, wParam, lParam );

	switch( wMsg ){
	case WM_ACTIVATEAPP:
		if( IsDocking() )
			break;

		// 自分が最初にアクティブ化された場合は一旦編集ウィンドウをアクティブ化して戻す
		//
		// Note. このダイアログは他とは異なるウィンドウスタイルのため閉じたときの挙動が異なる．
		// 他はスレッド内最近アクティブなウィンドウがアクティブになるが，このダイアログでは
		// セッション内全体での最近アクティブウィンドウがアクティブになってしまう．
		// それでは都合が悪いので，特別に以下の処理を行って他と同様な挙動が得られるようにする．
		if( (BOOL)wParam ){
			CEditView* pcEditView = (CEditView*)m_lParam;
			CEditWnd* pcEditWnd = pcEditView->m_pcEditDoc->m_pcEditWnd;
			if( ::GetActiveWindow() == GetHwnd() ){
				::SetActiveWindow( pcEditWnd->GetHwnd() );
				BlockingHook( NULL );	// キュー内に溜まっているメッセージを処理
				::SetActiveWindow( GetHwnd() );
				return 0L;
			}
		}
		break;

	case WM_NCPAINT:
		return OnNcPaint( hWnd, wMsg, wParam, lParam );
	case WM_NCCALCSIZE:
		return OnNcCalcSize( hWnd, wMsg, wParam, lParam );
	case WM_NCHITTEST:
		return OnNcHitTest( hWnd, wMsg, wParam, lParam );
	case WM_NCMOUSEMOVE:
		return OnNcMouseMove( hWnd, wMsg, wParam, lParam );
	case WM_MOUSEMOVE:
		return OnMouseMove( hWnd, wMsg, wParam, lParam );
	case WM_NCLBUTTONDOWN:
		return OnNcLButtonDown( hWnd, wMsg, wParam, lParam );
	case WM_LBUTTONUP:
		return OnLButtonUp( hWnd, wMsg, wParam, lParam );
	case WM_NCRBUTTONUP:
		if( IsDocking() && wParam == HTCAPTION ){
			// ドッキングのときはコンテキストメニューを明示的に呼び出す必要があるらしい
			::SendMessage( GetHwnd(), WM_CONTEXTMENU, (WPARAM)GetHwnd(), lParam );
			return 1L;
		}
	case WM_TIMER:
		return OnTimer( hWnd, wMsg, wParam, lParam );
	case WM_SETTEXT:
		if( IsDocking() ){
			// キャプションを再描画する
			// ※ この時点ではまだテキスト設定されていないので RDW_UPDATENOW では NG
			::RedrawWindow( hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_NOINTERNALPAINT );
		}
		break;
	case WM_MOUSEACTIVATE:
		if( IsDocking() ){
			// 分割バー以外の場所ならフォーカス移動
			if( !(HTLEFT <= LOWORD(lParam) && LOWORD(lParam) <= HTBOTTOMRIGHT) ){
				::SetFocus( GetHwnd() );
			}
		}
		break;
	case WM_COMMAND:
		if( IsDocking() ){
			// コンボボックスのフォーカスが変化したらキャプションを再描画する（アクティブ／非アクティブ切替）
			if( LOWORD(wParam) == IDC_COMBO_nSortType ){
				if( HIWORD(wParam) == CBN_SETFOCUS || HIWORD(wParam) == CBN_KILLFOCUS ){
					::RedrawWindow( hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOINTERNALPAINT );
				}
			}
		}
		break;
	case WM_NOTIFY:
		if( IsDocking() ){
			// ツリーやリストのフォーカスが変化したらキャプションを再描画する（アクティブ／非アクティブ切替）
			NMHDR* pNMHDR = (NMHDR*)lParam;
			if( pNMHDR->code == NM_SETFOCUS || pNMHDR->code == NM_KILLFOCUS ){
				::RedrawWindow( hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOINTERNALPAINT );
			}
		}
		break;
	}

	return result;
}


/* モードレスダイアログの表示 */
HWND CDlgFuncList::DoModeless(
	HINSTANCE		hInstance,
	HWND			hwndParent,
	LPARAM			lParam,
	CFuncInfoArr*	pcFuncInfoArr,
	CLayoutInt		nCurLine,
	CLayoutInt		nCurCol,
	int				nListType,
	bool			bLineNumIsCRLF		/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
)
{
	CEditView* pcEditView=(CEditView*)lParam;
	if( !pcEditView ) return NULL;
	m_pcFuncInfoArr = pcFuncInfoArr;	/* 関数情報配列 */
	m_nCurLine = nCurLine;				/* 現在行 */
	m_nCurCol = nCurCol;				/* 現在桁 */
	m_nListType = nListType;			/* 一覧の種類 */
	m_bLineNumIsCRLF = bLineNumIsCRLF;	/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
	m_nDocType = pcEditView->GetDocument()->m_cDocType.GetDocumentType().GetIndex();
	m_nSortCol = pcEditView->GetDocument()->m_cDocType.GetDocumentAttribute().m_nOutlineSortCol;
	m_nSortType = pcEditView->GetDocument()->m_cDocType.GetDocumentAttribute().m_nOutlineSortType;

	// 2007.04.18 genta : 「フォーカスを移す」と「自動的に閉じる」がチェックされている場合に
	// ダブルクリックを行うと，trueのまま残ってしまうので，ウィンドウを開いたときにリセットする．
	m_bWaitTreeProcess = false;

	m_eDockSide = ProfDockSide();
	HWND hwndRet;
	if( IsDocking() ){
		// ドッキング用にダイアログテンプレートに手を加えてから表示する（WS_CHILD化）
		if( !m_pDlgTemplate ){
			HRSRC hResInfo = ::FindResource( NULL, MAKEINTRESOURCE(IDD_FUNCLIST), RT_DIALOG );
			if( !hResInfo ) return NULL;
			HGLOBAL hResData = ::LoadResource( NULL, hResInfo );
			if( !hResData ) return NULL;
			m_pDlgTemplate = (LPDLGTEMPLATE)::LockResource( hResData );
			if( !m_pDlgTemplate ) return NULL;
			m_dwDlgTmpSize = ::SizeofResource( NULL, hResInfo );
		}
		LPDLGTEMPLATE pDlgTemplate = (LPDLGTEMPLATE)::GlobalAlloc( GMEM_FIXED, m_dwDlgTmpSize );
		if( !pDlgTemplate ) return NULL;
		::CopyMemory( pDlgTemplate, m_pDlgTemplate, m_dwDlgTmpSize );
		pDlgTemplate->style = (WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | DS_SETFONT);
		hwndRet = CDialog::DoModeless( hInstance, MyGetAncestor(hwndParent, GA_ROOT), pDlgTemplate, lParam, SW_HIDE );
		::GlobalFree( pDlgTemplate );
		pcEditView->m_pcEditWnd->EndLayoutBars( m_bEditWndReady );	// 画面の再レイアウト
	}else{
		hwndRet = CDialog::DoModeless( hInstance, MyGetAncestor(hwndParent, GA_ROOT), IDD_FUNCLIST, lParam, SW_SHOW );
	}
	return hwndRet;
}

/* モードレス時：検索対象となるビューの変更 */
void CDlgFuncList::ChangeView( LPARAM pcEditView )
{
	m_lParam = pcEditView;
	return;
}

/*! ダイアログデータの設定 */
void CDlgFuncList::SetData()
{
	int				i;
	TCHAR			szText[2048];
	CFuncInfo*		pcFuncInfo;
	LV_ITEM		item;
	HWND			hwndList;
	HWND			hwndTree;
	int				bSelected;
	CLayoutInt		nFuncLineOld;
	int				nSelectedLine;
	RECT			rc;
	hwndList = ::GetDlgItem( GetHwnd(), IDC_LIST_FL );
	hwndTree = ::GetDlgItem( GetHwnd(), IDC_TREE_FL );

	//2002.02.08 hor 隠しといてアイテム削除→あとで表示
	::ShowWindow( hwndList, SW_HIDE );
	::ShowWindow( hwndTree, SW_HIDE );
	ListView_DeleteAllItems( hwndList );
	TreeView_DeleteAllItems( hwndTree );

	m_cmemClipText.SetString(L"");	/* クリップボードコピー用テキスト */

	if( OUTLINE_CPP == m_nListType ){	/* C++メソッドリスト */
		m_nViewType = 1;
		SetTreeJava( GetHwnd(), TRUE );	// Jan. 04, 2002 genta Java Method Treeに統合
		::SetWindowText( GetHwnd(), _T("C++ メソッドツリー") );
	}
	else if( OUTLINE_FILE == m_nListType ){	//@@@ 2002.04.01 YAZAKI アウトライン解析にルールファイル導入
		m_nViewType = 1;
		SetTree();
		::SetWindowText( GetHwnd(), _T("ルールファイル") );
	}
	else if( OUTLINE_WZTXT == m_nListType ){ //@@@ 2003.05.20 zenryaku 階層付テキストアウトライン解析
		m_nViewType = 1;
		SetTree();
		::SetWindowText( GetHwnd(), _T("WZ階層付テキスト") ); //	2003.06.22 Moca 名前変更
	}
	else if( OUTLINE_HTML == m_nListType ){ //@@@ 2003.05.20 zenryaku HTMLアウトライン解析
		m_nViewType = 1;
		SetTree();
		::SetWindowText( GetHwnd(), _T("HTML") );
	}
	else if( OUTLINE_TEX == m_nListType ){ //@@@ 2003.07.20 naoh TeXアウトライン解析
		m_nViewType = 1;
		SetTree();
		::SetWindowText( GetHwnd(), _T("TeX") );
	}
	else if( OUTLINE_TEXT == m_nListType ){ /* テキスト・トピックリスト */
		m_nViewType = 1;
		SetTree();	//@@@ 2002.04.01 YAZAKI テキストトピックツリーも、汎用SetTreeを呼ぶように変更。
		::SetWindowText( GetHwnd(), _T("テキスト トピックツリー") );
	}
	else if( OUTLINE_JAVA == m_nListType ){ /* Javaメソッドツリー */
		m_nViewType = 1;
		SetTreeJava( GetHwnd(), TRUE );
		::SetWindowText( GetHwnd(), _T("Java メソッドツリー") );
	}
	//	2007.02.08 genta Python追加
	else if( OUTLINE_PYTHON == m_nListType ){ /* Python メソッドツリー */
		m_nViewType = 1;
		SetTree( true );
		::SetWindowText( GetHwnd(), _T("Python メソッドツリー") );
	}
	else if( OUTLINE_COBOL == m_nListType ){ /* COBOL アウトライン */
		m_nViewType = 1;
		SetTreeJava( GetHwnd(), FALSE );
		::SetWindowText( GetHwnd(), _T("COBOL アウトライン") );
	}
	else if( OUTLINE_VB == m_nListType ){	/* VisualBasic アウトライン */
		m_nViewType = 0;
		SetListVB();
		::SetWindowText( GetHwnd(), _T("Visual Basic アウトライン") );
	}
	else if( OUTLINE_TREE == m_nListType ){ /* 汎用ツリー */
		m_nViewType = 1;
		SetTree();
		::SetWindowText( GetHwnd(), _T("") );
	}
	else if( OUTLINE_CLSTREE == m_nListType ){ /* 汎用クラスツリー */
		m_nViewType = 1;
		SetTreeJava( GetHwnd(), TRUE );
		::SetWindowText( GetHwnd(), _T("") );
	}
	else{
		m_nViewType = 0;
		switch( m_nListType ){
		case OUTLINE_C:
			::SetWindowText( GetHwnd(), _T("C 関数一覧") );
			break;
		case OUTLINE_PLSQL:
			::SetWindowText( GetHwnd(), _T("PL/SQL 関数一覧") );
			break;
		case OUTLINE_ASM:
			::SetWindowText( GetHwnd(), _T("アセンブラ アウトライン") );
			break;
		case OUTLINE_PERL:	//	Sep. 8, 2000 genta
			::SetWindowText( GetHwnd(), _T("Perl 関数一覧") );
			break;
// Jul 10, 2003  little YOSHI  上に移動しました--->>
//		case OUTLINE_VB:	// 2001/06/23 N.Nakatani for Visual Basic
//			::SetWindowText( GetHwnd(), "Visual Basic アウトライン" );
//			break;
// <<---ここまで
		case OUTLINE_ERLANG:	//	2009.08.10 genta
			::SetWindowText( GetHwnd(), _T("Erlang 関数一覧") );
			break;
		case OUTLINE_BOOKMARK:
			LV_COLUMN col;
			col.mask = LVCF_TEXT;
			col.pszText = _T("テキスト");
			col.iSubItem = 0;
			//	Apr. 23, 2005 genta 行番号を左端へ
			ListView_SetColumn( hwndList, FL_COL_NAME, &col );
			::SetWindowText( GetHwnd(), _T("ブックマーク") );
			break;
		case OUTLINE_LIST:	// 汎用リスト 2010.03.28 syat
			::SetWindowText( GetHwnd(), _T("") );
			break;
//		case OUTLINE_COBOL:
//			::SetWindowText( GetHwnd(), "COBOLアウトライン" );
//			break;
		}
		//	May 18, 2001 genta
		//	Windowがいなくなると後で都合が悪いので、表示しないだけにしておく
		//::DestroyWindow( hwndTree );
//		::ShowWindow( hwndTree, SW_HIDE );
		::EnableWindow( ::GetDlgItem( GetHwnd() , IDC_BUTTON_COPY ), TRUE );
		nFuncLineOld = CLayoutInt(0);
		bSelected = FALSE;
		for( i = 0; i < m_pcFuncInfoArr->GetNum(); ++i ){
			pcFuncInfo = m_pcFuncInfoArr->GetAt( i );
			if( !bSelected ){
				if( i == 0 && m_nCurLine < pcFuncInfo->m_nFuncLineLAYOUT ){
					bSelected = TRUE;
					nSelectedLine = i;
				}
				else if( i > 0 && nFuncLineOld <= m_nCurLine && m_nCurLine < pcFuncInfo->m_nFuncLineLAYOUT ){
					bSelected = TRUE;
					nSelectedLine = i - 1;
				}
			}
			nFuncLineOld = pcFuncInfo->m_nFuncLineLAYOUT;
		}
		if( 0 < m_pcFuncInfoArr->GetNum() && !bSelected ){
			bSelected = TRUE;
			nSelectedLine =  m_pcFuncInfoArr->GetNum() - 1;
		}
		for( i = 0; i < m_pcFuncInfoArr->GetNum(); ++i ){
			/* 現在の解析結果要素 */
			pcFuncInfo = m_pcFuncInfoArr->GetAt( i );

			//	From Here Apr. 23, 2005 genta 行番号を左端へ
			/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
			if(m_bLineNumIsCRLF ){
				auto_sprintf( szText, _T("%d"), pcFuncInfo->m_nFuncLineCRLF );
			}else{
				auto_sprintf( szText, _T("%d"), pcFuncInfo->m_nFuncLineLAYOUT );
			}
			item.mask = LVIF_TEXT | LVIF_PARAM;
			item.pszText = szText;
			item.iItem = i;
			item.lParam	= i;
			item.iSubItem = FL_COL_ROW;
			ListView_InsertItem( hwndList, &item);

			// 2010.03.17 syat 桁追加
			/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
			if(m_bLineNumIsCRLF ){
				auto_sprintf( szText, _T("%d"), pcFuncInfo->m_nFuncColCRLF );
			}else{
				auto_sprintf( szText, _T("%d"), pcFuncInfo->m_nFuncColLAYOUT );
			}
			item.mask = LVIF_TEXT;
			item.pszText = szText;
			item.iItem = i;
			item.iSubItem = FL_COL_COL;
			ListView_SetItem( hwndList, &item);

			item.mask = LVIF_TEXT;
			item.pszText = pcFuncInfo->m_cmemFuncName.GetStringPtr();
			item.iItem = i;
			item.iSubItem = FL_COL_NAME;
			ListView_SetItem( hwndList, &item);
			//	To Here Apr. 23, 2005 genta 行番号を左端へ

			item.mask = LVIF_TEXT;
			if(  1 == pcFuncInfo->m_nInfo ){item.pszText = _T("宣言");}else
			if( 10 == pcFuncInfo->m_nInfo ){item.pszText = _T("関数宣言");}else
			if( 20 == pcFuncInfo->m_nInfo ){item.pszText = _T("プロシージャ宣言");}else
			if( 11 == pcFuncInfo->m_nInfo ){item.pszText = _T("関数");}else
			if( 21 == pcFuncInfo->m_nInfo ){item.pszText = _T("プロシージャ");}else
			if( 31 == pcFuncInfo->m_nInfo ){item.pszText = _T("■パッケージ仕様部");}else
			if( 41 == pcFuncInfo->m_nInfo ){item.pszText = _T("■パッケージ本体部");}else
			if( 50 == pcFuncInfo->m_nInfo ){item.pszText = _T("PROC");}else
			if( 51 == pcFuncInfo->m_nInfo ){item.pszText = _T("ラベル");}else
			if( 52 == pcFuncInfo->m_nInfo ){item.pszText = _T("ENDP");}else{
				// Jul 10, 2003  little YOSHI
				// ここにあったVB関係の処理はSetListVB()メソッドに移動しました。

				item.pszText = _T("");
			}
			item.iItem = i;
			item.iSubItem = FL_COL_REMARK;
			ListView_SetItem( hwndList, &item);

			/* クリップボードにコピーするテキストを編集 */
			if(_tcslen(item.pszText)){
				// 検出結果の種類(関数,,,)があるとき
				auto_sprintf(
					szText,
					_T("%ts(%d,%d): %ts(%ts)\r\n"),
					m_pcFuncInfoArr->m_szFilePath.c_str(),		/* 解析対象ファイル名 */
					pcFuncInfo->m_nFuncLineCRLF,		/* 検出行番号 */
					pcFuncInfo->m_nFuncColCRLF,		/* 検出桁番号 */
					pcFuncInfo->m_cmemFuncName.GetStringPtr(),	/* 検出結果 */
					item.pszText								/* 検出結果の種類 */
				);
			}else{
				// 検出結果の種類(関数,,,)がないとき
				auto_sprintf(
					szText,
					_T("%ts(%d,%d): %ts\r\n"),
					m_pcFuncInfoArr->m_szFilePath.c_str(),		/* 解析対象ファイル名 */
					pcFuncInfo->m_nFuncLineCRLF,		/* 検出行番号 */
					pcFuncInfo->m_nFuncColCRLF,		/* 検出桁番号 */
					pcFuncInfo->m_cmemFuncName.GetStringPtr()	/* 検出結果 */
				);
			}
			m_cmemClipText.AppendStringT(szText);				/* クリップボードコピー用テキスト */
		}
		//2002.02.08 hor Listは列幅調整とかを実行する前に表示しとかないと変になる
		::ShowWindow( hwndList, SW_SHOW );
		/* 列の幅をデータに合わせて調整 */
		ListView_SetColumnWidth( hwndList, FL_COL_ROW, LVSCW_AUTOSIZE );
		ListView_SetColumnWidth( hwndList, FL_COL_COL, LVSCW_AUTOSIZE );
		ListView_SetColumnWidth( hwndList, FL_COL_NAME, LVSCW_AUTOSIZE );
		ListView_SetColumnWidth( hwndList, FL_COL_REMARK, LVSCW_AUTOSIZE );
		ListView_SetColumnWidth( hwndList, FL_COL_ROW, ListView_GetColumnWidth( hwndList, FL_COL_ROW ) + 16 );
		ListView_SetColumnWidth( hwndList, FL_COL_COL, ListView_GetColumnWidth( hwndList, FL_COL_COL ) + 16 );
		ListView_SetColumnWidth( hwndList, FL_COL_NAME, ListView_GetColumnWidth( hwndList, FL_COL_NAME ) + 16 );
		ListView_SetColumnWidth( hwndList, FL_COL_REMARK, ListView_GetColumnWidth( hwndList, FL_COL_REMARK ) + 16 );

		// 2005.07.05 ぜっと
		DWORD dwExStyle  = ListView_GetExtendedListViewStyle( hwndList );
		dwExStyle |= LVS_EX_FULLROWSELECT;
		ListView_SetExtendedListViewStyle( hwndList, dwExStyle );

		if( bSelected ){
			ListView_GetItemRect( hwndList, 0, &rc, LVIR_BOUNDS );
			ListView_Scroll( hwndList, 0, nSelectedLine * ( rc.bottom - rc.top ) );
			ListView_SetItemState( hwndList, nSelectedLine, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
		}
	}
	/* アウトライン ダイアログを自動的に閉じる */
	::CheckDlgButton( GetHwnd(), IDC_CHECK_bAutoCloseDlgFuncList, m_pShareData->m_Common.m_sOutline.m_bAutoCloseDlgFuncList );
	/* アウトライン ブックマーク一覧で空行を無視する */
	::CheckDlgButton( GetHwnd(), IDC_CHECK_bMarkUpBlankLineEnable, m_pShareData->m_Common.m_sOutline.m_bMarkUpBlankLineEnable );
	/* アウトライン ジャンプしたらフォーカスを移す */
	::CheckDlgButton( GetHwnd(), IDC_CHECK_bFunclistSetFocusOnJump, m_pShareData->m_Common.m_sOutline.m_bFunclistSetFocusOnJump );

	/* アウトライン ■位置とサイズを記憶する */ // 20060201 aroka
	::CheckDlgButton( GetHwnd(), IDC_BUTTON_WINSIZE, m_pShareData->m_Common.m_sOutline.m_bRememberOutlineWindowPos );
	// ボタンが押されているかはっきりさせる 2008/6/5 Uchi
	::DlgItem_SetText( GetHwnd(), IDC_BUTTON_WINSIZE, 
		m_pShareData->m_Common.m_sOutline.m_bRememberOutlineWindowPos ? _T("■") : _T("□") );

	/* ダイアログを自動的に閉じるならフォーカス移動オプションは関係ない */
	if(m_pShareData->m_Common.m_sOutline.m_bAutoCloseDlgFuncList){
		::EnableWindow( ::GetDlgItem( GetHwnd(), IDC_CHECK_bFunclistSetFocusOnJump ), FALSE );
	}else{
		::EnableWindow( ::GetDlgItem( GetHwnd(), IDC_CHECK_bFunclistSetFocusOnJump ), TRUE );
	}

	//2002.02.08 hor
	//（IDC_LIST_FLもIDC_TREE_FLも常に存在していて、m_nViewTypeによって、どちらを表示するかを選んでいる）
	HWND hwndShow = (0 == m_nViewType)? hwndList: hwndTree;
	::ShowWindow( hwndShow, SW_SHOW );
	if( ::GetForegroundWindow() == MyGetAncestor( GetHwnd(), GA_ROOT ) && IsChild( GetHwnd(), GetFocus()) )
		::SetFocus( hwndShow );

	//2002.02.08 hor
	//空行をどう扱うかのチェックボックスはブックマーク一覧のときだけ表示する
	if(OUTLINE_BOOKMARK == m_nListType){
		::EnableWindow( ::GetDlgItem( GetHwnd(), IDC_CHECK_bMarkUpBlankLineEnable ), TRUE );
		if( !IsDocking() ) ::ShowWindow( GetDlgItem( GetHwnd(), IDC_CHECK_bMarkUpBlankLineEnable ), SW_SHOW );
	}else{
		::ShowWindow( GetDlgItem( GetHwnd(), IDC_CHECK_bMarkUpBlankLineEnable ), SW_HIDE );
		::EnableWindow( ::GetDlgItem( GetHwnd(), IDC_CHECK_bMarkUpBlankLineEnable ), FALSE );
	}
	// 2002/11/1 frozen 項目のソート基準を設定するコンボボックスはブックマーク一覧の以外の時に表示する
	// Nov. 5, 2002 genta ツリー表示の時だけソート基準コンボボックスを表示
	CEditView* pcEditView = (CEditView*)m_lParam;
	int nDocType = pcEditView->GetDocument()->m_cDocType.GetDocumentType().GetIndex();
	if( nDocType != m_nDocType ){
		// 以前とはドキュメントタイプが変わったので初期化する
		m_nDocType = nDocType;
		m_nSortCol = pcEditView->GetDocument()->m_cDocType.GetDocumentAttribute().m_nOutlineSortCol;
		m_nSortType = pcEditView->GetDocument()->m_cDocType.GetDocumentAttribute().m_nOutlineSortType;
	}
	if( m_nViewType == 1 ){
		HWND hWnd_Combo_Sort = ::GetDlgItem( GetHwnd(), IDC_COMBO_nSortType );
		::EnableWindow( hWnd_Combo_Sort , TRUE );
		::ShowWindow( hWnd_Combo_Sort , SW_SHOW );
		Combo_ResetContent( hWnd_Combo_Sort ); // 2002.11.10 Moca 追加
		Combo_AddString( hWnd_Combo_Sort , _WINT("デフォルト"));
		Combo_AddString( hWnd_Combo_Sort , _WINT("アルファベット順"));
		Combo_SetCurSel( hWnd_Combo_Sort , m_nSortType );
		::ShowWindow( GetDlgItem( GetHwnd(), IDC_STATIC_nSortType ), SW_SHOW );
		// 2002.11.10 Moca 追加 ソートする
		if( 1 == m_nSortType ){
			SortTree(::GetDlgItem( GetHwnd() , IDC_TREE_FL),TVI_ROOT);
		}
	}
	else {
		::EnableWindow( ::GetDlgItem( GetHwnd(), IDC_COMBO_nSortType ), FALSE );
		::ShowWindow( GetDlgItem( GetHwnd(), IDC_COMBO_nSortType ), SW_HIDE );
		::ShowWindow( GetDlgItem( GetHwnd(), IDC_STATIC_nSortType ), SW_HIDE );
		//ListView_SortItems( hwndList, _CompareFunc_, (LPARAM)this );  // 2005.04.05 zenryaku ソート状態を保持
		SortListView( hwndList, m_nSortCol );	// 2005.04.23 genta 関数化(ヘッダ書き換えのため)
	}
}




/* ダイアログデータの取得 */
/* 0==条件未入力   0より大きい==正常   0より小さい==入力エラー */
int CDlgFuncList::GetData( void )
{
	HWND			hwndList;
	HWND			hwndTree;
	int				nItem;
	LV_ITEM			item;
	HTREEITEM		htiItem;
	TV_ITEM		tvi;
	TCHAR		szLabel[32];

	m_cFuncInfo = NULL;
	hwndList = ::GetDlgItem( GetHwnd(), IDC_LIST_FL );
	if( m_nViewType == 0 ){
		//	List
		nItem = ListView_GetNextItem( hwndList, -1, LVNI_ALL | LVNI_SELECTED );
		if( -1 == nItem ){
			return -1;
		}
		item.mask = LVIF_PARAM;
		item.iItem = nItem;
		item.iSubItem = 0;
		ListView_GetItem( hwndList, &item );
		m_cFuncInfo = m_pcFuncInfoArr->GetAt( item.lParam );
	}else{
		hwndTree = ::GetDlgItem( GetHwnd(), IDC_TREE_FL );
		if( NULL != hwndTree ){
			htiItem = TreeView_GetSelection( hwndTree );

			tvi.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_PARAM;
			tvi.hItem = htiItem;
			tvi.pszText = szLabel;
			tvi.cchTextMax = _countof( szLabel );
			if( TreeView_GetItem( hwndTree, &tvi ) ){
				// lParamが-1以下は pcFuncInfoArrには含まれない項目
				if( 0 <= tvi.lParam ){
					m_cFuncInfo = m_pcFuncInfoArr->GetAt( tvi.lParam );
				}
			}
		}
	}
	return 1;
}

/* Java/C++メソッドツリーの最大ネスト深さ */
#define MAX_JAVA_TREE_NEST 16

/*! ツリーコントロールの初期化：Javaメソッドツリー

	Java Method Treeの構築: 関数リストを元にTreeControlを初期化する。

	@date 2002.01.04 genta C++ツリーを統合
*/
void CDlgFuncList::SetTreeJava( HWND hwndDlg, BOOL bAddClass )
{
	int				i;
	CFuncInfo*		pcFuncInfo;
	HWND			hwndTree;
	int				bSelected;
	CLayoutInt		nFuncLineOld;
	CLayoutInt		nFuncColOld;
	int				nSelectedLine;
	TV_INSERTSTRUCT	tvis;
	const TCHAR*	pPos;
    TCHAR           szLabel[64+6];  // Jan. 07, 2001 genta クラス名エリアの拡大
	HTREEITEM		htiGlobal = NULL;	// Jan. 04, 2001 genta C++と統合
	HTREEITEM		htiClass;
	HTREEITEM		htiItem;
	HTREEITEM		htiItemOld;
	HTREEITEM		htiSelected;
	TV_ITEM		tvi;
	int				nClassNest;
	int				nDummylParam = -64000;	// 2002.11.10 Moca クラス名のダミーlParam ソートのため
	TCHAR			szClassArr[MAX_JAVA_TREE_NEST][64];	// Jan. 04, 2001 genta クラス名エリアの拡大 //2009.9.21 syat ネストが深すぎる際のBOF対策

	::EnableWindow( ::GetDlgItem( GetHwnd() , IDC_BUTTON_COPY ), TRUE );

	hwndTree = ::GetDlgItem( GetHwnd(), IDC_TREE_FL );

	nFuncLineOld = CLayoutInt(0);
	nFuncColOld = CLayoutInt(0);
	bSelected = FALSE;
	htiItemOld = NULL;
	for( i = 0; i < m_pcFuncInfoArr->GetNum(); ++i ){
		pcFuncInfo = m_pcFuncInfoArr->GetAt( i );
		const TCHAR*		pWork;
		pWork = pcFuncInfo->m_cmemFuncName.GetStringPtr();
		/* クラス名::メソッドの場合 */
		if( NULL != ( pPos = _tcsstr( pWork, _T("::") ) ) ){
			/* インナークラスのネストレベルを調べる */
			int	k, m;
			int	nWorkLen;
			int	nCharChars;
			nClassNest = 0;
			m = 0;
			nWorkLen = _tcslen( pWork );
			for( k = 0; k < nWorkLen; ++k ){
				//2009.9.21 syat ネストが深すぎる際のBOF対策
				if( nClassNest == MAX_JAVA_TREE_NEST ){
					k = nWorkLen;
					break;
				}
				// 2005-09-02 D.S.Koba GetSizeOfChar
				nCharChars = CNativeT::GetSizeOfChar( pWork, nWorkLen, k );
				if( 1 == nCharChars && _T(':') == pWork[k] ){
					//	Jan. 04, 2001 genta
					//	C++の統合のため、\に加えて::をクラス区切りとみなすように
					if( k < nWorkLen - 1 && _T(':') == pWork[k+1] ){
						auto_memcpy( szClassArr[nClassNest], &pWork[m], k - m );
						szClassArr[nClassNest][k - m] = _T('\0');
						++nClassNest;
						m = k + 2;
						++k;
					}
					else 
						break;
				}
				else if( 1 == nCharChars && _T('\\') == pWork[k] ){
					auto_memcpy( szClassArr[nClassNest], &pWork[m], k - m );
					szClassArr[nClassNest][k - m] = _T('\0');
					++nClassNest;
					m = k + 1;
				}
				if( 2 == nCharChars ){
					++k;
				}
			}

			//	Jan. 04, 2001 genta
			//	関数先頭のセット(ツリー構築で使う)
			pWork = pWork + m; // 2 == lstrlen( "::" );

			/* クラス名のアイテムが登録されているか */
			htiClass = TreeView_GetFirstVisible( hwndTree );
			HTREEITEM htiParent = TVI_ROOT;
			for( k = 0; k < nClassNest; ++k ){
				//	Apr. 1, 2001 genta
				//	追加文字列を全角にしたのでメモリもそれだけ必要
				//	6 == strlen( "クラス" ), 1 == strlen( L'\0' )

				// 2002/10/30 frozen
				// bAddClass == true の場合の仕様変更
				// 既存の項目は　「(クラス名)(半角スペース一個)(追加文字列)」
				// となっているとみなし、szClassArr[k] が 「クラス名」と一致すれば、それを親ノードに設定。
				// ただし、一致する項目が複数ある場合は最初の項目を親ノードにする。
				// 一致しない場合は「(クラス名)(半角スペース一個)クラス」のノードを作成する。
				size_t nClassNameLen = _tcslen( szClassArr[k] );
				for( ; NULL != htiClass ; htiClass = TreeView_GetNextSibling( hwndTree, htiClass ))
				{
					tvi.mask = TVIF_HANDLE | TVIF_TEXT;
					tvi.hItem = htiClass;
					tvi.pszText = szLabel;
					tvi.cchTextMax = _countof(szLabel);
					if( TreeView_GetItem( hwndTree, &tvi ) )
					{
						if( 0 == _tcsncmp( szClassArr[k],szLabel,nClassNameLen) )
						{
							if( _countof(szLabel) < (nClassNameLen +1) )
								break;// バッファ不足では無条件にマッチする
							else
							{
								if(bAddClass)
								{
									if(szLabel[nClassNameLen]==L' ')
										break;
								}
								else
								{
									if(szLabel[nClassNameLen]==L'\0')
										break;
								}
							}
						}
					}
				}

				/* クラス名のアイテムが登録されていないので登録 */
				if( NULL == htiClass ){
					// 2002/10/28 frozen 上からここへ移動
					TCHAR*	pClassName;
					pClassName = new TCHAR[ _tcslen( szClassArr[k] ) + 1 + 9 ]; // 2002/10/28 frozen +9は追加する文字列の最大長（" 名前空間"が最大）
					_tcscpy( pClassName, szClassArr[k] );

					tvis.item.lParam = -1;
					if( bAddClass )
					{
						if( pcFuncInfo->m_nInfo == 7 )
						{
							_tcscat( pClassName, _T(" 名前空間") );
							tvis.item.lParam = i;
						}
						else
							_tcscat( pClassName, _T(" クラス") );
							tvis.item.lParam = nDummylParam;
							nDummylParam++;
					}

					tvis.hParent = htiParent;
					tvis.hInsertAfter = TVI_LAST;
					tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
					tvis.item.pszText = const_cast<TCHAR*>(to_tchar(pClassName));

					htiClass = TreeView_InsertItem( hwndTree, &tvis );
					//	Jan. 04, 2001 genta
					//	不要になったらさっさと削除
					delete [] pClassName; // 2002/10/28 frozen 下からここへ移動

				}else{
					//none
				}
				htiParent = htiClass;
				//if( k + 1 >= nClassNest ){
				//	break;
				//}
				htiClass = TreeView_GetChild( hwndTree, htiClass );
			}
			htiClass = htiParent;
		}else{
			//	Jan. 04, 2001 genta
			//	Global空間の場合 (C++のみ)

			// 2002/10/27 frozen ここから
			// 2007.05.26 genta "__interface" をクラスに類する扱いにする
			if( 3 <= pcFuncInfo->m_nInfo  && pcFuncInfo->m_nInfo <= 8 )
				htiClass = TVI_ROOT;
			else
			{
			// 2002/10/27 frozen ここまで
				if( htiGlobal == NULL ){
					TV_INSERTSTRUCT	tvg;
					
					::ZeroMemory( &tvg, sizeof(tvg));
					tvg.hParent = TVI_ROOT;
					tvg.hInsertAfter = TVI_LAST;
					tvg.item.mask = TVIF_TEXT | TVIF_PARAM;
					tvg.item.pszText = _T("グローバル");
//					tvg.item.lParam = -1;
					tvg.item.lParam = nDummylParam;
					htiGlobal = TreeView_InsertItem( hwndTree, &tvg );
					nDummylParam++;
				}
				htiClass = htiGlobal;
			}
		}
		TCHAR*		pFuncName;
		pFuncName = new TCHAR[ _tcslen(pWork) + 32 ];	// ↓で追加する文字列が収まるだけ確保
		_tcscpy( pFuncName, pWork );

		// 2002/10/27 frozen 追加文字列の種類を増やした
		switch(pcFuncInfo->m_nInfo)
		{// case 4以上の各追加文字列の最初にある半角スペースを省略することはできない。
		case 1: _tcscat( pFuncName, _T("(宣言)") );break;
		case 3: _tcscat( pFuncName, _T(" クラス") );break;
		case 4: _tcscat( pFuncName, _T(" 構造体") );break;
		case 5: _tcscat( pFuncName, _T(" 列挙体") );break;
		case 6: _tcscat( pFuncName, _T(" 共用体") );break;
//		case 7: _tcscat( pFuncName, _T(" 名前空間") );break;
		
		case 8: _tcscat( pFuncName, _T(" インターフェース") );break; // 2007.05.26 genta : "__interface"
		};
//		}
		/* 該当クラス名のアイテムの子として、メソッドのアイテムを登録 */
		tvis.hParent = htiClass;
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
		tvis.item.pszText = pFuncName;
		tvis.item.lParam = i;
		htiItem = TreeView_InsertItem( hwndTree, &tvis );

		/* クリップボードにコピーするテキストを編集 */
		WCHAR szText[2048];
		auto_sprintf(
			szText,
			L"%ts(%d,%d): %ts %ls\r\n",
			m_pcFuncInfoArr->m_szFilePath.c_str(),		/* 解析対象ファイル名 */
			pcFuncInfo->m_nFuncLineCRLF,		/* 検出行番号 */
			pcFuncInfo->m_nFuncColCRLF,		/* 検出桁番号 */
			pcFuncInfo->m_cmemFuncName.GetStringPtr(), 	/* 検出結果 */
			( 1 == pcFuncInfo->m_nInfo ? L"(宣言)" : L"" ) 	//	Jan. 04, 2001 genta C++で使用
		);
		m_cmemClipText.AppendString( szText ); /* クリップボードコピー用テキスト */
		delete [] pFuncName;

		/* 現在カーソル位置のメソッドかどうか調べる */
		if( !bSelected ){
			if( i == 0 &&
				( m_nCurLine < pcFuncInfo->m_nFuncLineLAYOUT
				|| ( m_nCurLine == pcFuncInfo->m_nFuncLineLAYOUT && m_nCurCol < pcFuncInfo->m_nFuncColLAYOUT ) ) ){
				bSelected = TRUE;
				nSelectedLine = i;
				htiSelected = htiItem;
			}else
			if( i > 0 &&
				( nFuncLineOld < m_nCurLine || ( nFuncLineOld == m_nCurLine && nFuncColOld <= m_nCurCol ) ) &&
				( m_nCurLine < pcFuncInfo->m_nFuncLineLAYOUT
				|| ( m_nCurLine == pcFuncInfo->m_nFuncLineLAYOUT && m_nCurCol < pcFuncInfo->m_nFuncColLAYOUT ) ) ){
				bSelected = TRUE;
				nSelectedLine = i - 1;
				htiSelected = htiItemOld;
			}
		}
		nFuncLineOld = pcFuncInfo->m_nFuncLineLAYOUT;
		nFuncColOld = pcFuncInfo->m_nFuncColLAYOUT;
		htiItemOld = htiItem;
		//	Jan. 04, 2001 genta
		//	deleteはその都度行うのでここでは不要
		}
	/* ソート、ノードの展開をする */
//	TreeView_SortChildren( hwndTree, TVI_ROOT, 0 );
	htiClass = TreeView_GetFirstVisible( hwndTree );
	while( NULL != htiClass ){
//		TreeView_SortChildren( hwndTree, htiClass, 0 );
		TreeView_Expand( hwndTree, htiClass, TVE_EXPAND );
		htiClass = TreeView_GetNextSibling( hwndTree, htiClass );
	}
	/* 現在カーソル位置のメソッドを選択状態にする */
	if( bSelected ){
		TreeView_SelectItem( hwndTree, htiSelected );
	}else{
		if( NULL != htiItemOld ){
			TreeView_SelectItem( hwndTree, htiItemOld );
		}
	}
//	GetTreeTextNext( hwndTree, NULL, 0 );
	return;
}


/*! リストビューコントロールの初期化：VisualBasic

  長くなったので独立させました。

  @date Jul 10, 2003  little YOSHI
*/
void CDlgFuncList::SetListVB (void)
{
	int				i;
	wchar_t			szType[64];
	wchar_t			szOption[64];
	CFuncInfo*		pcFuncInfo;
	LV_ITEM		item;
	HWND			hwndList;
	int				bSelected;
	CLayoutInt		nFuncLineOld;
	CLayoutInt		nFuncColOld;
	int				nSelectedLine;
	RECT			rc;

	::EnableWindow( ::GetDlgItem( GetHwnd() , IDC_BUTTON_COPY ), TRUE );

	hwndList = ::GetDlgItem( GetHwnd(), IDC_LIST_FL );

	nFuncLineOld = CLayoutInt(0);
	nFuncColOld = CLayoutInt(0);
	bSelected = FALSE;
	for( i = 0; i < m_pcFuncInfoArr->GetNum(); ++i ){
		pcFuncInfo = m_pcFuncInfoArr->GetAt( i );
		if( !bSelected ){
			if( i == 0 &&
				( m_nCurLine < pcFuncInfo->m_nFuncLineLAYOUT
				|| ( m_nCurLine == pcFuncInfo->m_nFuncLineLAYOUT && m_nCurCol < pcFuncInfo->m_nFuncColLAYOUT ) ) ){
				bSelected = TRUE;
				nSelectedLine = i;
			} else
			if( i > 0 &&
				( nFuncLineOld < m_nCurLine || ( nFuncLineOld == m_nCurLine && nFuncColOld <= m_nCurCol ) ) &&
				( m_nCurLine < pcFuncInfo->m_nFuncLineLAYOUT
				|| ( m_nCurLine == pcFuncInfo->m_nFuncLineLAYOUT && m_nCurCol < pcFuncInfo->m_nFuncColLAYOUT ) ) ){
				bSelected = TRUE;
				nSelectedLine = i - 1;
			}
		}
		nFuncLineOld = pcFuncInfo->m_nFuncLineLAYOUT;
		nFuncColOld = pcFuncInfo->m_nFuncColLAYOUT;
	}
	if( 0 < m_pcFuncInfoArr->GetNum() && !bSelected ){
		bSelected = TRUE;
		nSelectedLine =  m_pcFuncInfoArr->GetNum() - 1;
	}

	TCHAR			szText[2048];
	for( i = 0; i < m_pcFuncInfoArr->GetNum(); ++i ){
		/* 現在の解析結果要素 */
		pcFuncInfo = m_pcFuncInfoArr->GetAt( i );

		//	From Here Apr. 23, 2005 genta 行番号を左端へ
		/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
		if(m_bLineNumIsCRLF ){
			auto_sprintf( szText, _T("%d"), pcFuncInfo->m_nFuncLineCRLF );
		}else{
			auto_sprintf( szText, _T("%d"), pcFuncInfo->m_nFuncLineLAYOUT );
		}
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.pszText = szText;
		item.iItem = i;
		item.iSubItem = FL_COL_ROW;
		item.lParam	= i;
		ListView_InsertItem( hwndList, &item);

		// 2010.03.17 syat 桁追加
		/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
		if(m_bLineNumIsCRLF ){
			auto_sprintf( szText, _T("%d"), pcFuncInfo->m_nFuncColCRLF );
		}else{
			auto_sprintf( szText, _T("%d"), pcFuncInfo->m_nFuncColLAYOUT );
		}
		item.mask = LVIF_TEXT;
		item.pszText = szText;
		item.iItem = i;
		item.iSubItem = FL_COL_COL;
		ListView_SetItem( hwndList, &item);

		item.mask = LVIF_TEXT;
		item.pszText = pcFuncInfo->m_cmemFuncName.GetStringPtr();
		item.iItem = i;
		item.iSubItem = FL_COL_NAME;
		ListView_SetItem( hwndList, &item);
		//	To Here Apr. 23, 2005 genta 行番号を左端へ

		item.mask = LVIF_TEXT;

		// 2001/06/23 N.Nakatani for Visual Basic
		//	Jun. 26, 2001 genta 半角かな→全角に
		auto_memset(szText, _T('\0'), _countof(szText));
		auto_memset(szType, _T('\0'), _countof(szType));
		auto_memset(szOption, _T('\0'), _countof(szOption));
		if( 1 == ((pcFuncInfo->m_nInfo >> 8) & 0x01) ){
			// スタティック宣言(Static)
			// 2006.12.12 Moca 末尾にスペース追加
			wcscpy(szOption, L"静的 ");
		}
		switch ((pcFuncInfo->m_nInfo >> 4) & 0x0f) {
			case 2  :	// プライベート(Private)
				wcsncat(szOption, L"プライベート", _countof(szOption) - wcslen(szOption)); //	2006.12.17 genta サイズ誤り修正
				break;

			case 3  :	// フレンド(Friend)
				wcsncat(szOption, L"フレンド", _countof(szOption) - wcslen(szOption)); //	2006.12.17 genta サイズ誤り修正
				break;

			default :	// パブリック(Public)
				wcsncat(szOption, L"パブリック", _countof(szOption) - wcslen(szOption)); //	2006.12.17 genta サイズ誤り修正
		}
		switch (pcFuncInfo->m_nInfo & 0x0f) {
			case 1:		// 関数(Function)
				wcscpy(szType, L"関数");
				break;

			// 2006.12.12 Moca ステータス→プロシージャに変更
			case 2:		// プロシージャ(Sub)
				wcscpy(szType, L"プロシージャ");
				break;

			case 3:		// プロパティ 取得(Property Get)
				wcscpy(szType, L"プロパティ 取得");
				break;

			case 4:		// プロパティ 設定(Property Let)
				wcscpy(szType, L"プロパティ 設定");
				break;

			case 5:		// プロパティ 参照(Property Set)
				wcscpy(szType, L"プロパティ 参照");
				break;

			case 6:		// 定数(Const)
				wcscpy(szType, L"定数");
				break;

			case 7:		// 列挙型(Enum)
				wcscpy(szType, L"列挙型");
				break;

			case 8:		// ユーザ定義型(Type)
				wcscpy(szType, L"ユーザ定義型");
				break;

			case 9:		// イベント(Event)
				wcscpy(szType, L"イベント");
				break;

			default:	// 未定義なのでクリア
				pcFuncInfo->m_nInfo	= 0;

		}
		if ( 2 == ((pcFuncInfo->m_nInfo >> 8) & 0x02) ) {
			// 宣言(Declareなど)
			wcsncat(szType, L"宣言", _countof(szType) - wcslen(szType));
		}

		TCHAR szTypeOption[256]; // 2006.12.12 Moca auto_sprintfの入出力で同一変数を使わないための作業領域追加
		if ( 0 == pcFuncInfo->m_nInfo ) {
			szTypeOption[0] = _T('\0');	//	2006.12.17 genta 全体を0で埋める必要はない
		} else
		if ( 0 == wcslen(szOption) ) {
			auto_sprintf(szTypeOption, _T("%ls"), szType);
		} else {
			auto_sprintf(szTypeOption, _T("%ls（%ls）"), szType, szOption);
		}
		item.pszText = szTypeOption;
		item.iItem = i;
		item.iSubItem = FL_COL_REMARK;
		ListView_SetItem( hwndList, &item);

		/* クリップボードにコピーするテキストを編集 */
		if(_tcslen(item.pszText)){
			// 検出結果の種類(関数,,,)があるとき
			// 2006.12.12 Moca szText を自分自身にコピーしていたバグを修正
			auto_sprintf(
				szText,
				_T("%ts(%d,%d): %ts(%ts)\r\n"),
				m_pcFuncInfoArr->m_szFilePath.c_str(),		/* 解析対象ファイル名 */
				pcFuncInfo->m_nFuncLineCRLF,		/* 検出行番号 */
				pcFuncInfo->m_nFuncColCRLF,		/* 検出桁番号 */
				pcFuncInfo->m_cmemFuncName.GetStringPtr(),	/* 検出結果 */
				item.pszText								/* 検出結果の種類 */
			);
		}else{
			// 検出結果の種類(関数,,,)がないとき
			auto_sprintf(
				szText,
				_T("%ts(%d,%d): %ts\r\n"),
				m_pcFuncInfoArr->m_szFilePath.c_str(),		/* 解析対象ファイル名 */
				pcFuncInfo->m_nFuncLineCRLF,		/* 検出行番号 */
				pcFuncInfo->m_nFuncColCRLF,		/* 検出桁番号 */
				pcFuncInfo->m_cmemFuncName.GetStringPtr()	/* 検出結果 */
			);
		}
		m_cmemClipText.AppendStringT( szText );	/* クリップボードコピー用テキスト */
	}

	//2002.02.08 hor Listは列幅調整とかを実行する前に表示しとかないと変になる
	::ShowWindow( hwndList, SW_SHOW );
	/* 列の幅をデータに合わせて調整 */
	ListView_SetColumnWidth( hwndList, FL_COL_ROW, LVSCW_AUTOSIZE );
	ListView_SetColumnWidth( hwndList, FL_COL_COL, LVSCW_AUTOSIZE );
	ListView_SetColumnWidth( hwndList, FL_COL_NAME, LVSCW_AUTOSIZE );
	ListView_SetColumnWidth( hwndList, FL_COL_REMARK, LVSCW_AUTOSIZE );
	ListView_SetColumnWidth( hwndList, FL_COL_ROW, ListView_GetColumnWidth( hwndList, FL_COL_ROW ) + 16 );
	ListView_SetColumnWidth( hwndList, FL_COL_COL, ListView_GetColumnWidth( hwndList, FL_COL_COL ) + 16 );
	ListView_SetColumnWidth( hwndList, FL_COL_NAME, ListView_GetColumnWidth( hwndList, FL_COL_NAME ) + 16 );
	ListView_SetColumnWidth( hwndList, FL_COL_REMARK, ListView_GetColumnWidth( hwndList, FL_COL_REMARK ) + 16 );
	if( bSelected ){
		ListView_GetItemRect( hwndList, 0, &rc, LVIR_BOUNDS );
		ListView_Scroll( hwndList, 0, nSelectedLine * ( rc.bottom - rc.top ) );
		ListView_SetItemState( hwndList, nSelectedLine, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
	}

	return;
}

/*! 汎用ツリーコントロールの初期化：CFuncInfo::m_nDepthを利用して親子を設定

	@param[in] tagjump タグジャンプ形式で出力する

	@date 2002.04.01 YAZAKI
	@date 2002.11.10 Moca 階層の制限をなくした
	@date 2007.02.25 genta クリップボード出力をタブジャンプ可能な書式に変更
	@date 2007.03.04 genta タブジャンプ可能な書式に変更するかどうかのフラグを追加
*/
void CDlgFuncList::SetTree(bool tagjump)
{
	HTREEITEM hItemSelected = NULL;
	HWND hwndTree = ::GetDlgItem( GetHwnd(), IDC_TREE_FL );

	int i;
	int nFuncInfoArrNum = m_pcFuncInfoArr->GetNum();
	int nStackPointer = 0;
	int nStackDepth = 32; // phParentStack の確保している数
	HTREEITEM* phParentStack;
	phParentStack = (HTREEITEM*)malloc( nStackDepth * sizeof( HTREEITEM ) );
	phParentStack[ nStackPointer ] = TVI_ROOT;

	for (i = 0; i < nFuncInfoArrNum; i++){
		CFuncInfo* pcFuncInfo = m_pcFuncInfoArr->GetAt(i);

		/*	新しいアイテムを作成
			現在の親の下にぶら下げる形で、最後に追加する。
		*/
		HTREEITEM hItem;
		TV_INSERTSTRUCT cTVInsertStruct;
		cTVInsertStruct.hParent = phParentStack[ nStackPointer ];
		cTVInsertStruct.hInsertAfter = TVI_LAST;	//	必ず最後に追加。
		cTVInsertStruct.item.mask = TVIF_TEXT | TVIF_PARAM;
		cTVInsertStruct.item.pszText = pcFuncInfo->m_cmemFuncName.GetStringPtr();
		cTVInsertStruct.item.lParam = i;	//	あとでこの数値（＝m_pcFuncInfoArrの何番目のアイテムか）を見て、目的地にジャンプするぜ!!。

		/*	親子関係をチェック
		*/
		if (nStackPointer != pcFuncInfo->m_nDepth){
			//	レベルが変わりました!!
			//	※が、2段階深くなることは考慮していないので注意。
			//	　もちろん、2段階以上浅くなることは考慮済み。

			// 2002.11.10 Moca 追加 確保したサイズでは足りなくなった。再確保
			if( nStackDepth <= pcFuncInfo->m_nDepth + 1 ){
				nStackDepth = pcFuncInfo->m_nDepth + 4; // 多めに確保しておく
				HTREEITEM* phTi;
				phTi = (HTREEITEM*)realloc( phParentStack, nStackDepth * sizeof( HTREEITEM ) );
				if( NULL != phTi ){
					phParentStack = phTi;
				}else{
					goto end_of_func;
				}
			}
			nStackPointer = pcFuncInfo->m_nDepth;
			cTVInsertStruct.hParent = phParentStack[ nStackPointer ];
		}
		hItem = TreeView_InsertItem( hwndTree, &cTVInsertStruct );
		phParentStack[ nStackPointer+1 ] = hItem;

		/*	pcFuncInfoに登録されている行数、桁を確認して、選択するアイテムを考える
		*/
		if ( pcFuncInfo->m_nFuncLineLAYOUT < m_nCurLine
			|| ( pcFuncInfo->m_nFuncLineLAYOUT == m_nCurLine && pcFuncInfo->m_nFuncColLAYOUT <= m_nCurCol ) ){
			hItemSelected = hItem;
		}

		/* クリップボードコピー用テキストを作成する */
		//	2003.06.22 Moca dummy要素はツリーに入れるがTAGJUMPには加えない
		if( pcFuncInfo->IsAddClipText() ){
			CNativeT text;

			if( tagjump ){

				text.AllocStringBuffer(
					  pcFuncInfo->m_cmemFuncName.GetStringLength()
					+ nStackPointer * 2 + 1
					+ _tcslen( m_pcFuncInfoArr->m_szFilePath )
					+ 20
				);
				
				//	2007.03.04 genta タグジャンプできる形式で書き込む
				text.AppendString( m_pcFuncInfoArr->m_szFilePath );
				
				TCHAR linenum[32];
				int len = auto_sprintf( linenum, _T("(%d,%d): "),
					pcFuncInfo->m_nFuncLineCRLF,				/* 検出行番号 */
					pcFuncInfo->m_nFuncColCRLF					/* 検出桁番号 */
				);
				text.AppendString( linenum );
			}
			else {
				//	先に十分なサイズの領域を取っておく
				text.AllocStringBuffer(
					pcFuncInfo->m_cmemFuncName.GetStringLength() + nStackPointer * 2 + 1 + 5
				);
			}

			for( int cnt = 0; cnt < nStackPointer; cnt++ ){
				text.AppendString(_T("  "));
			}
			text.AppendString(_T(" "));
			
			text.AppendNativeData( pcFuncInfo->m_cmemFuncName );
			text.AppendString( _T("\r\n") );
			m_cmemClipText.AppendNativeDataT( text );	/* クリップボードコピー用テキスト */
		}
	}

end_of_func:;

	::EnableWindow( ::GetDlgItem( GetHwnd() , IDC_BUTTON_COPY ), TRUE );

	if( NULL != hItemSelected ){
		/* 現在カーソル位置のメソッドを選択状態にする */
		TreeView_SelectItem( hwndTree, hItemSelected );
	}

	free( phParentStack );
}



BOOL CDlgFuncList::OnInitDialog( HWND hwndDlg, WPARAM wParam, LPARAM lParam )
{
	m_bStretching = false;
	m_bHovering = false;
	m_nHilightedBtn = -1;
	m_nCapturingBtn = -1;

	_SetHwnd( hwndDlg );

	HWND		hwndList;
	int			nCxVScroll;
	int			nColWidthArr[] = { 0, 10, 46, 80 };
	RECT		rc;
	LV_COLUMN	col;
	hwndList = ::GetDlgItem( hwndDlg, IDC_LIST_FL );
	::SetWindowLongPtr(hwndList, GWL_STYLE, ::GetWindowLongPtr(hwndList, GWL_STYLE) | LVS_SHOWSELALWAYS );
	// 2005.10.21 zenryaku 1行選択
	ListView_SetExtendedListViewStyle(hwndList,
		ListView_GetExtendedListViewStyle(hwndList) | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	::GetWindowRect( hwndList, &rc );
	nCxVScroll = ::GetSystemMetrics( SM_CXVSCROLL );

	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	col.fmt = LVCFMT_LEFT;
	col.cx = rc.right - rc.left - ( nColWidthArr[1] + nColWidthArr[2] + nColWidthArr[3] ) - nCxVScroll - 8;
	//	Apr. 23, 2005 genta 行番号を左端へ
	col.pszText = _T("行 *");
	col.iSubItem = FL_COL_ROW;
	ListView_InsertColumn( hwndList, FL_COL_ROW, &col);

	// 2010.03.17 syat 桁追加
	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	col.fmt = LVCFMT_LEFT;
	col.cx = nColWidthArr[FL_COL_COL];
	col.pszText = _T("桁");
	col.iSubItem = FL_COL_COL;
	ListView_InsertColumn( hwndList, FL_COL_COL, &col);

	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	col.fmt = LVCFMT_LEFT;
	col.cx = nColWidthArr[FL_COL_NAME];
	//	Apr. 23, 2005 genta 行番号を左端へ
	col.pszText = _T("関数名");
	col.iSubItem = FL_COL_NAME;
	ListView_InsertColumn( hwndList, FL_COL_NAME, &col);

	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	col.fmt = LVCFMT_LEFT;
	col.cx = nColWidthArr[FL_COL_REMARK];
	col.pszText = _T(" ");
	col.iSubItem = FL_COL_REMARK;
	ListView_InsertColumn( hwndList, FL_COL_REMARK, &col);

	/* アウトライン位置とサイズを初期化する */ // 20060201 aroka
	CEditView* pcEditView=(CEditView*)m_lParam;
	if( m_lParam != NULL ){
		if( !IsDocking() && m_pShareData->m_Common.m_sOutline.m_bRememberOutlineWindowPos ){
			WINDOWPLACEMENT cWindowPlacement;
			cWindowPlacement.length = sizeof( cWindowPlacement );
			if (::GetWindowPlacement( pcEditView->m_pcEditDoc->m_pcEditWnd->GetHwnd(), &cWindowPlacement )){
				/* ウィンドウ位置・サイズを-1以外の値にしておくと、CDialogで使用される． */
				m_xPos = m_pShareData->m_Common.m_sOutline.m_xOutlineWindowPos + cWindowPlacement.rcNormalPosition.left;
				m_yPos = m_pShareData->m_Common.m_sOutline.m_yOutlineWindowPos + cWindowPlacement.rcNormalPosition.top;
				m_nWidth =  m_pShareData->m_Common.m_sOutline.m_widthOutlineWindow;
				m_nHeight = m_pShareData->m_Common.m_sOutline.m_heightOutlineWindow;
			}
		}else if( IsDocking() ){
			m_xPos = 0;
			m_yPos = 0;
			m_nShowCmd = SW_HIDE;
			::GetWindowRect( ::GetParent(pcEditView->GetHwnd()), &rc );	// ここではまだ GetDockSpaceRect() は使えない
			EDockSide eDockSide = GetDockSide();
			switch( eDockSide ){
			case DOCKSIDE_LEFT:		m_nWidth = ProfDockLeft();		break;
			case DOCKSIDE_TOP:		m_nHeight = ProfDockTop();		break;
			case DOCKSIDE_RIGHT:	m_nWidth = ProfDockRight();		break;
			case DOCKSIDE_BOTTOM:	m_nHeight = ProfDockBottom();	break;
			}
			if( eDockSide == DOCKSIDE_LEFT || eDockSide == DOCKSIDE_RIGHT ){
				if( m_nWidth == 0 )	// 初回
					m_nWidth = (rc.right - rc.left) / 3;
				if( m_nWidth > rc.right - rc.left - DOCK_MIN_SIZE ) m_nWidth = rc.right - rc.left - DOCK_MIN_SIZE;
				if( m_nWidth < DOCK_MIN_SIZE ) m_nWidth = DOCK_MIN_SIZE;
			}else{
				if( m_nHeight == 0 )	// 初回
					m_nHeight = (rc.bottom - rc.top) / 3;
				if( m_nHeight > rc.bottom - rc.top - DOCK_MIN_SIZE ) m_nHeight = rc.bottom - rc.top - DOCK_MIN_SIZE;
				if( m_nHeight < DOCK_MIN_SIZE ) m_nHeight = DOCK_MIN_SIZE;
			}
		}
	}

	if( !m_bInChangeLayout ){	// ChangeLayout() 処理中は設定変更しない
		ProfDockDisp() = TRUE;
		// 他ウィンドウに変更を通知する
		if( ProfDockSync() ){
			HWND hwndEdit = pcEditView->m_pcEditWnd->GetHwnd();
			PostOutlineNotifyToAllEditors( (WPARAM)0, (LPARAM)hwndEdit );
		}
	}

	if( !IsDocking() ){
		/* 基底クラスメンバ */
		CreateSizeBox();

		LONG style = ::GetWindowLong( GetHwnd(), GWL_STYLE );
		::SetWindowLong( GetHwnd(), GWL_STYLE, style | WS_THICKFRAME );
		::SetWindowPos( GetHwnd(), NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );
	}

	m_hwndToolTip = NULL;
	if( IsDocking() ){
		//ツールチップを作成する。（「閉じる」などのボタン用）
		m_hwndToolTip = ::CreateWindowEx(
			0,
			TOOLTIPS_CLASS,
			NULL,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			GetHwnd(),
			NULL,
			m_hInstance,
			NULL
			);

		// ツールチップをマルチライン可能にする（SHRT_MAX: Win95でINT_MAXだと表示されない）
		Tooltip_SetMaxTipWidth( m_hwndToolTip, SHRT_MAX );

		// アウトラインにツールチップを追加する
		TOOLINFO	ti;
		ti.cbSize      = sizeof( ti );
		ti.uFlags      = TTF_SUBCLASS | TTF_IDISHWND;	// TTF_IDISHWND: uId は HWND で rect は無視（HWND 全体）
		ti.hwnd        = GetHwnd();
		ti.hinst       = m_hInstance;
		ti.uId         = (UINT)GetHwnd();
		ti.lpszText    = NULL;
		ti.rect.left   = 0;
		ti.rect.top    = 0;
		ti.rect.right  = 0;
		ti.rect.bottom = 0;
		Tooltip_AddTool( m_hwndToolTip, &ti );

		// 不要なコントロールを隠す
		HWND hwndPrev;
		HWND hwnd = ::GetWindow( GetHwnd(), GW_CHILD );
		while( hwnd ){
			int nId = ::GetDlgCtrlID( hwnd );
			hwndPrev = hwnd;
			hwnd = ::GetWindow( hwnd, GW_HWNDNEXT );
			switch( nId ){
			case IDC_STATIC_nSortType:
			case IDC_COMBO_nSortType:
			case IDC_LIST_FL:
			case IDC_TREE_FL:
				continue;
			}
			ShowWindow( hwndPrev, SW_HIDE );
		}
	}

	SyncColor();

	return CDialog::OnInitDialog( hwndDlg, wParam, lParam );
}


BOOL CDlgFuncList::OnBnClicked( int wID )
{
	switch( wID ){
	case IDC_BUTTON_MENU:
		RECT rcMenu;
		GetWindowRect( ::GetDlgItem( GetHwnd(), IDC_BUTTON_MENU ), &rcMenu );
		POINT ptMenu;
		ptMenu.x = rcMenu.left;
		ptMenu.y = rcMenu.bottom;
		DoMenu( ptMenu, GetHwnd() );
		return TRUE;
	case IDC_BUTTON_HELP:
		/* 「アウトライン解析」のヘルプ */
		//Apr. 5, 2001 JEPRO 修正漏れを追加 (Stonee, 2001/03/12 第四引数を、機能番号からヘルプトピック番号を調べるようにした)
		MyWinHelp( GetHwnd(), m_szHelpFile, HELP_CONTEXT, ::FuncID_To_HelpContextID(F_OUTLINE) );	// 2006.10.10 ryoji MyWinHelpに変更に変更
		return TRUE;
	case IDOK:
		return OnJump();
	case IDCANCEL:
		if( m_bModal ){		/* モーダル ダイアログか */
			::EndDialog( GetHwnd(), 0 );
		}else{
			if( IsDocking() ){
				::SetFocus( ((CEditView*)m_lParam)->GetHwnd() );
			}else{
				::DestroyWindow( GetHwnd() );
			}
		}
		return TRUE;
	case IDC_BUTTON_COPY:
		// Windowsクリップボードにコピー 
		// 2004.02.17 Moca 関数化
		SetClipboardText( GetHwnd(), m_cmemClipText.GetStringPtr(), m_cmemClipText.GetStringLength() );
		return TRUE;
	case IDC_BUTTON_WINSIZE:
		{// ウィンドウの位置とサイズを記憶 // 20060201 aroka
			m_pShareData->m_Common.m_sOutline.m_bRememberOutlineWindowPos = ::IsDlgButtonChecked( GetHwnd(), IDC_BUTTON_WINSIZE );
		}
		// ボタンが押されているかはっきりさせる 2008/6/5 Uchi
		::DlgItem_SetText( GetHwnd(), IDC_BUTTON_WINSIZE,
			m_pShareData->m_Common.m_sOutline.m_bRememberOutlineWindowPos ? _T("■") : _T("□") );
		return TRUE;
	//2002.02.08 オプション切替後List/Treeにフォーカス移動
	case IDC_CHECK_bAutoCloseDlgFuncList:
	case IDC_CHECK_bMarkUpBlankLineEnable:
	case IDC_CHECK_bFunclistSetFocusOnJump:
		m_pShareData->m_Common.m_sOutline.m_bAutoCloseDlgFuncList = ::IsDlgButtonChecked( GetHwnd(), IDC_CHECK_bAutoCloseDlgFuncList );
		m_pShareData->m_Common.m_sOutline.m_bMarkUpBlankLineEnable = ::IsDlgButtonChecked( GetHwnd(), IDC_CHECK_bMarkUpBlankLineEnable );
		m_pShareData->m_Common.m_sOutline.m_bFunclistSetFocusOnJump = ::IsDlgButtonChecked( GetHwnd(), IDC_CHECK_bFunclistSetFocusOnJump );
		if(m_pShareData->m_Common.m_sOutline.m_bAutoCloseDlgFuncList){
			::EnableWindow( ::GetDlgItem( GetHwnd(), IDC_CHECK_bFunclistSetFocusOnJump ), FALSE );
		}else{
			::EnableWindow( ::GetDlgItem( GetHwnd(), IDC_CHECK_bFunclistSetFocusOnJump ), TRUE );
		}
		if(wID==IDC_CHECK_bMarkUpBlankLineEnable&&m_nListType==OUTLINE_BOOKMARK){
			CEditView* pcEditView=(CEditView*)m_lParam;
			pcEditView->GetCommander().HandleCommand( F_BOOKMARK_VIEW, TRUE, TRUE, 0, 0, 0 );
			m_nCurLine=pcEditView->GetCaret().GetCaretLayoutPos().GetY2() + CLayoutInt(1);
			SetData();
		}else
		if(m_nViewType){
			::SetFocus( ::GetDlgItem( GetHwnd(), IDC_TREE_FL ) );
		}else{
			::SetFocus( ::GetDlgItem( GetHwnd(), IDC_LIST_FL ) );
		}
		return TRUE;
	}
	/* 基底クラスメンバ */
	return CDialog::OnBnClicked( wID );
}

BOOL CDlgFuncList::OnNotify( WPARAM wParam, LPARAM lParam )
{
//	int				idCtrl;
	LPNMHDR			pnmh;
	NM_LISTVIEW*	pnlv;
	HWND			hwndList;
	HWND			hwndTree;
	NM_TREEVIEW*	pnmtv;
//	int				nLineTo;

//	idCtrl = (int) wParam;
	pnmh = (LPNMHDR) lParam;
	pnlv = (NM_LISTVIEW*)lParam;

	CEditView* pcEditView=(CEditView*)m_lParam;
	hwndList = ::GetDlgItem( GetHwnd(), IDC_LIST_FL );
	hwndTree = ::GetDlgItem( GetHwnd(), IDC_TREE_FL );

	if( hwndTree == pnmh->hwndFrom ){
		pnmtv = (NM_TREEVIEW *) lParam;
		switch( pnmtv->hdr.code ){
		case NM_CLICK:
			if( IsDocking() ){
				// この時点ではまだ選択変更されていないが OnJump() の予備動作として先に選択変更しておく
				TVHITTESTINFO tvht = {0};
				::GetCursorPos( &tvht.pt );
				::ScreenToClient( hwndTree, &tvht.pt );
				TreeView_HitTest( hwndTree, &tvht );
				if( (tvht.flags & TVHT_ONITEM) && tvht.hItem ){
					TreeView_SelectItem( hwndTree, tvht.hItem );
					OnJump( false );
					return TRUE;
				}
			}
			break;
		case NM_DBLCLK:
			// 2002.02.16 hor Treeのダブルクリックでフォーカス移動できるように 3/4
			OnJump();
			m_bWaitTreeProcess=true;
			::SetWindowLongPtr( GetHwnd(), DWLP_MSGRESULT, TRUE );	// ツリーの展開／縮小をしない
			return TRUE;
			//return OnJump();
		case TVN_KEYDOWN:
			if( ((TV_KEYDOWN *)lParam)->wVKey == VK_SPACE ){
				OnJump( false );
				return TRUE;
			}
			Key2Command( ((TV_KEYDOWN *)lParam)->wVKey );
			return TRUE;
		case NM_KILLFOCUS:
			// 2002.02.16 hor Treeのダブルクリックでフォーカス移動できるように 4/4
			if(m_bWaitTreeProcess){
				if(m_pShareData->m_Common.m_sOutline.m_bFunclistSetFocusOnJump){
					::SetFocus( pcEditView->GetHwnd() );
				}
				m_bWaitTreeProcess=false;
			}
			return TRUE;
		}
	}else
	if( hwndList == pnmh->hwndFrom ){
		switch( pnmh->code ){
		case LVN_COLUMNCLICK:
//				MYTRACE_A( "LVN_COLUMNCLICK\n" );
			m_nSortCol =  pnlv->iSubItem;
			pcEditView->GetDocument()->m_cDocType.GetDocumentAttribute().m_nOutlineSortCol = m_nSortCol;
			//	Apr. 23, 2005 genta 関数として独立させた
			SortListView( hwndList, m_nSortCol );
			return TRUE;
		case NM_CLICK:
			if( IsDocking() ){
				OnJump( false );
				return TRUE;
			}
			break;
		case NM_DBLCLK:
				OnJump();
			return TRUE;
		case LVN_KEYDOWN:
			if( ((LV_KEYDOWN *)lParam)->wVKey == VK_SPACE ){
				OnJump( false );
				return TRUE;
			}
			Key2Command( ((LV_KEYDOWN *)lParam)->wVKey );
			return TRUE;
		}
	}

#ifdef DEFINE_SYNCCOLOR
	if( IsDocking() ){
		if( hwndList == pnmh->hwndFrom || hwndTree == pnmh->hwndFrom ){
			if( pnmh->code == NM_CUSTOMDRAW ){
				LPNMCUSTOMDRAW lpnmcd = (LPNMCUSTOMDRAW)lParam;
				switch( lpnmcd->dwDrawStage ){
				case CDDS_PREPAINT:
					::SetWindowLongPtr( GetHwnd(), DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW );
					break;
				case CDDS_ITEMPREPAINT:
					{	// 選択アイテムを反転表示にする
						STypeConfig	*TypeDataPtr = &(pcEditView->m_pcEditDoc->m_cDocType.GetDocumentAttribute());
						COLORREF clrText = TypeDataPtr->m_ColorInfoArr[COLORIDX_TEXT].m_colTEXT;
						COLORREF clrTextBk = TypeDataPtr->m_ColorInfoArr[COLORIDX_TEXT].m_colBACK;
						if( hwndList == pnmh->hwndFrom ){
							//if( lpnmcd->uItemState & CDIS_SELECTED ){	// 非選択のアイテムもすべて CDIS_SELECTED で来る？
							if( ListView_GetItemState( hwndList, lpnmcd->dwItemSpec, LVIS_SELECTED ) ){
								((LPNMLVCUSTOMDRAW)lpnmcd)->clrText = clrText ^ RGB(255, 255, 255);
								((LPNMLVCUSTOMDRAW)lpnmcd)->clrTextBk = clrTextBk ^ RGB(255, 255, 255);
								lpnmcd->uItemState = 0;	// リストビューには選択としての描画をさせないようにする？
							}
						}else{
							if( lpnmcd->uItemState & CDIS_SELECTED ){
								((LPNMTVCUSTOMDRAW)lpnmcd)->clrText = clrText ^ RGB(255, 255, 255);
								((LPNMTVCUSTOMDRAW)lpnmcd)->clrTextBk = clrTextBk ^ RGB(255, 255, 255);
							}
						}
					}
					::SetWindowLongPtr( GetHwnd(), DWLP_MSGRESULT, CDRF_DODEFAULT );
					break;
				}

				return TRUE;
			}
		}
	}
#endif

	return FALSE;
}
/*!
	指定されたカラムでリストビューをソートする．
	同時にヘッダも書き換える．

	ソート後はフォーカスが画面内に現れるように表示位置を調整する．

	@par 表示位置調整の小技
	EnsureVisibleの結果は，上スクロールの場合は上端に，下スクロールの場合は
	下端に目的の項目が現れる．端から少し離したい場合はオフセットを与える必要が
	あるが，スクロール方向がわからないと±がわからない
	そのため最初に一番下に一回スクロールさせることでEnsureVisibleでは
	かならず上スクロールになるようにすることで，ソート後の表示位置を
	固定する

	@param[in] hwndList	リストビューのウィンドウハンドル
	@param[in] sortcol	ソートするカラム番号(0-2)

	@date 2005.04.23 genta 関数として独立させた
	@date 2005.04.29 genta ソート後の表示位置調整
	@date 2010.03.17 syat 桁追加
*/
void CDlgFuncList::SortListView(HWND hwndList, int sortcol)
{
	LV_COLUMN		col;
	int col_no;

	//	Apr. 23, 2005 genta 行番号を左端へ

//	if( sortcol == 1 ){
	{
		col_no = FL_COL_NAME;
		col.mask = LVCF_TEXT;
	// From Here 2001.12.03 hor
	//	col.pszText = _T("関数名 *");
		if(OUTLINE_BOOKMARK == m_nListType){
			col.pszText = ( sortcol == col_no ? _T("テキスト *") : _T("テキスト") );
		}else{
			col.pszText = ( sortcol == col_no ? _T("関数名 *") : _T("関数名") );
		}
	// To Here 2001.12.03 hor
		col.iSubItem = 0;
		ListView_SetColumn( hwndList, col_no, &col );

		col_no = FL_COL_ROW;
		col.mask = LVCF_TEXT;
		col.pszText = ( sortcol == col_no ? _T("行 *") : _T("行") );
		col.iSubItem = 0;
		ListView_SetColumn( hwndList, col_no, &col );

		// 2010.03.17 syat 桁追加
		col_no = FL_COL_COL;
		col.mask = LVCF_TEXT;
		col.pszText = ( sortcol == col_no ? _T("桁 *") : _T("桁") );
		col.iSubItem = 0;
		ListView_SetColumn( hwndList, col_no, &col );

		col_no = FL_COL_REMARK;
	// From Here 2001.12.07 hor
		col.mask = LVCF_TEXT;
		col.pszText = ( sortcol == col_no ? _T("*") : _T("") );
		col.iSubItem = 0;
		ListView_SetColumn( hwndList, col_no, &col );
	// To Here 2001.12.07 hor

		ListView_SortItems( hwndList, _CompareFunc_, (LPARAM)this );
	}
	//	2005.04.23 zenryaku 選択された項目が見えるようにする

	//	Apr. 29, 2005 genta 一旦一番下にスクロールさせる
	ListView_EnsureVisible( hwndList,
		ListView_GetItemCount(hwndList) - 1,
		FALSE );
	
	//	Jan.  9, 2006 genta 先頭から1つ目と2つ目の関数が
	//	選択された場合にスクロールされなかった
	int keypos = ListView_GetNextItem(hwndList, -1, LVNI_FOCUSED) - 2;
	ListView_EnsureVisible( hwndList,
		keypos >= 0 ? keypos : 0,
		FALSE );
}

/*!	ウィンドウサイズが変更された

	@date 2003.06.22 Moca コードの整理(コントロールの処理方法をテーブルに持たせる)
	@date 2003.08.16 genta 配列はstaticに(無駄な初期化を行わないため)
*/
BOOL CDlgFuncList::OnSize( WPARAM wParam, LPARAM lParam )
{
	// 今のところ CEditWnd::OnSize() からの呼び出しでは lParam は CEditWnd 側 の lParam のまま渡される	// 2010.06.05 ryoji
	RECT rcDlg;
	::GetClientRect( GetHwnd(), &rcDlg );
	lParam = MAKELONG(rcDlg.right - rcDlg.left, rcDlg.bottom -  rcDlg.top);	// 自前で補正

	/* 基底クラスメンバ */
	CDialog::OnSize( wParam, lParam );

	static const int Controls[][2] = {
		{IDC_CHECK_bFunclistSetFocusOnJump, 1},
		{IDC_CHECK_bMarkUpBlankLineEnable , 1},
		{IDC_CHECK_bAutoCloseDlgFuncList, 1},
		{IDC_BUTTON_MENU, 2},
		{IDC_BUTTON_WINSIZE, 2}, // 20060201 aroka
		{IDC_BUTTON_COPY, 2},
		{IDOK, 2},
		{IDCANCEL, 2},
		{IDC_BUTTON_HELP, 2},
		{IDC_LIST_FL, 3},
		{IDC_TREE_FL, 3},
		{IDC_COMBO_nSortType, 4},
	};
	int		nControls = _countof( Controls );
//	int		fwSizeType;
	int		nWidth;
	int		nHeight;
	int		i;
	int		nHeightCheckBox;
	int		nHeightButton;
	const int	nHeightMargin = 3;
	RECT	rc;
	HWND	hwndCtrl;
	POINT	po;

	nWidth = LOWORD(lParam);	// width of client area
	nHeight = HIWORD(lParam);	// height of client area


	::GetWindowRect( ::GetDlgItem( GetHwnd(), IDC_CHECK_bAutoCloseDlgFuncList ), &rc );
	nHeightCheckBox = rc.bottom -  rc.top;
	::GetWindowRect( ::GetDlgItem( GetHwnd(), IDOK ), &rc );	
	nHeightButton = rc.bottom - rc.top;

	for ( i = 0; i < nControls; ++i ){
		hwndCtrl = ::GetDlgItem( GetHwnd(), Controls[i][0] );
		if( !hwndCtrl ) continue;	// 存在しないモノは無視（NULLウィンドウで継続するとスクリーン全体がちらつく）
		::GetWindowRect( hwndCtrl, &rc );
		po.x = rc.left;
		po.y = rc.top;
		::ScreenToClient( GetHwnd(), &po );
		rc.left = po.x;
		rc.top  = po.y;
		po.x = rc.right;
		po.y = rc.bottom;
		::ScreenToClient( GetHwnd(), &po );
		rc.right = po.x;
		rc.bottom  = po.y;
		//	2003.06.22 Moca テーブル上の種別によって処理方法を変える
		switch( Controls[i][1] ){
		case 1:
			::SetWindowPos( hwndCtrl, NULL, 
				rc.left,
				nHeight - nHeightCheckBox - nHeightMargin,
				0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER );
			break;
// 2002/11/1 frozen ここから
		case 2:
			::SetWindowPos( hwndCtrl, NULL,
				rc.left,
				nHeight - nHeightCheckBox - nHeightButton - nHeightMargin * 2,
				0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER );
			break;
		case 3:
			::SetWindowPos( hwndCtrl, NULL, 0, 0, 
				nWidth - 2 * rc.left,
								nHeight - rc.top - (IsDocking()? 0: nHeightCheckBox + nHeightButton + 3 * nHeightMargin),
				SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER );
			break;
		case 4:
			if( !IsDocking() ) break;
			::SetWindowPos( hwndCtrl, NULL, 0, 0, 
				nWidth - rc.left,
				rc.bottom - rc.top,
				SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER );
			break;
		}
// 2002/11/1 frozen ここまで
		// workaround
		// ツリーやリストはちらつきを抑えるように UpdateWindow()
		// ツリーやリスト以外は UpdateWindow() だとゴミが残るので InvalidateRect()
		(Controls[i][1] == 3)? ::UpdateWindow( hwndCtrl ): ::InvalidateRect( hwndCtrl, NULL, TRUE );
	}

	if( IsDocking() ){
		// ダイアログ部分を再描画（ツリー／リストの範囲はちらつかないように除外）
		::InvalidateRect( GetHwnd(), NULL, FALSE );
		POINT pt;
		::GetWindowRect( ::GetDlgItem( GetHwnd(), IDC_TREE_FL ), &rc );
		pt.x = rc.left;
		pt.y = rc.top;
		::ScreenToClient( GetHwnd(), &pt );
		::OffsetRect( &rc, pt.x - rc.left, pt.y - rc.top );
		::ValidateRect( GetHwnd(), &rc );
	}
	return TRUE;
}

int CALLBACK Compare_by_ItemData(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	if( lParam1< lParam2 )
		return -1;
	if( lParam1 > lParam2 )
		return 1;
	else
		return 0;
}

BOOL CDlgFuncList::OnDestroy( void )
{
	CDialog::OnDestroy();

	/* アウトライン ■位置とサイズを記憶する */ // 20060201 aroka
	// 前提条件：m_lParam が CDialog::OnDestroy でクリアされないこと
	CEditView* pcEditView=(CEditView*)m_lParam;
	HWND hwndEdit = pcEditView->m_pcEditWnd->GetHwnd();
	if( !IsDocking() && m_pShareData->m_Common.m_sOutline.m_bRememberOutlineWindowPos ){
		/* 親のウィンドウ位置・サイズを記憶 */
		WINDOWPLACEMENT cWindowPlacement;
		cWindowPlacement.length = sizeof( cWindowPlacement );
		if (::GetWindowPlacement( hwndEdit, &cWindowPlacement )){
			/* ウィンドウ位置・サイズを記憶 */
			m_pShareData->m_Common.m_sOutline.m_xOutlineWindowPos = m_xPos - cWindowPlacement.rcNormalPosition.left;
			m_pShareData->m_Common.m_sOutline.m_yOutlineWindowPos = m_yPos - cWindowPlacement.rcNormalPosition.top;
			m_pShareData->m_Common.m_sOutline.m_widthOutlineWindow = m_nWidth;
			m_pShareData->m_Common.m_sOutline.m_heightOutlineWindow = m_nHeight;
		}
	}

	// ドッキング画面を閉じるときは画面を再レイアウトする
	// ドッキングでアプリ終了時には hwndEdit は NULL になっている（親に先に WM_DESTROY が送られるため）
	if( IsDocking() && hwndEdit )
		pcEditView->m_pcEditWnd->EndLayoutBars();

	// 明示的にアウトライン画面を閉じたときだけアウトライン表示フラグを OFF にする
	// フローティングでアプリ終了時やタブモードで裏にいる場合は ::IsWindowVisible( hwndEdit ) が FALSE を返す
	if( hwndEdit && ::IsWindowVisible( hwndEdit ) && !m_bInChangeLayout ){	// ChangeLayout() 処理中は設定変更しない
		ProfDockDisp() = FALSE;
		// 他ウィンドウに変更を通知する
		if( ProfDockSync() ){
			PostOutlineNotifyToAllEditors( (WPARAM)0, (LPARAM)hwndEdit );
		}
	}

	if( m_hwndToolTip ){
		::DestroyWindow( m_hwndToolTip );
		m_hwndToolTip = NULL;
	}
	::KillTimer( GetHwnd(), 1 );

	return TRUE;
}


BOOL CDlgFuncList::OnCbnSelChange( HWND hwndCtl, int wID )
{
	CEditView* pcEditView=(CEditView*)m_lParam;
	int nSelect = Combo_GetCurSel( hwndCtl );
	switch(wID)
	{
	case IDC_COMBO_nSortType:
		if( m_nSortType != nSelect )
		{
			m_nSortType = nSelect;
			pcEditView->GetDocument()->m_cDocType.GetDocumentAttribute().m_nOutlineSortType = m_nSortType;
			SortTree(::GetDlgItem( GetHwnd() , IDC_TREE_FL),TVI_ROOT);
		}
		return TRUE;
	};
	return FALSE;

}
void  CDlgFuncList::SortTree(HWND hWndTree,HTREEITEM htiParent)
{
	if( m_nSortType == 1 )
		TreeView_SortChildren(hWndTree,htiParent,TRUE);
	else
	{
		TVSORTCB sort;
		sort.hParent =  htiParent;
		sort.lpfnCompare = Compare_by_ItemData;
		sort.lParam = 0;
		TreeView_SortChildrenCB(hWndTree , &sort , TRUE);
	}
	for(HTREEITEM htiItem = TreeView_GetChild( hWndTree, htiParent ); NULL != htiItem ; htiItem = TreeView_GetNextSibling( hWndTree, htiItem ))
		SortTree(hWndTree,htiItem);
}



BOOL CDlgFuncList::OnJump( bool bCheckAutoClose )	//2002.02.08 hor 引数追加
{
	int				nLineTo;
	int				nColTo;
	/* ダイアログデータの取得 */
	if( 0 < GetData() && m_cFuncInfo != NULL ){
		nLineTo = m_cFuncInfo->m_nFuncLineCRLF;
		nColTo = m_cFuncInfo->m_nFuncColCRLF;
		if( m_bModal ){		/* モーダル ダイアログか */
			//モーダル表示する場合は、m_cFuncInfoを取得するアクセサを実装して結果取得すること。
			::EndDialog( GetHwnd(), 1 );
		}else{
			/* カーソルを移動させる */
			POINT	poCaret;
			poCaret.x = nColTo - 1;
			poCaret.y = nLineTo - 1;

			memcpy_raw( m_pShareData->m_sWorkBuffer.GetWorkBuffer<void>(), &poCaret, sizeof(poCaret) );

			//	2006.07.09 genta 移動時に選択状態を保持するように
			::SendMessageAny( ((CEditView*)m_lParam)->m_pcEditWnd->GetHwnd(),
				MYWM_SETCARETPOS, 0, PM_SETCARETPOS_KEEPSELECT );
			if( bCheckAutoClose ){
				/* アウトライン ダイアログを自動的に閉じる */
				if( IsDocking() ){
					::PostMessageAny( ((CEditView*)m_lParam)->GetHwnd(), MYWM_SETACTIVEPANE, 0, 0 );
				}
				else if( m_pShareData->m_Common.m_sOutline.m_bAutoCloseDlgFuncList ){
					::DestroyWindow( GetHwnd() );
				}
				else if( m_pShareData->m_Common.m_sOutline.m_bFunclistSetFocusOnJump ){
					::SetFocus( ((CEditView*)m_lParam)->GetHwnd() );
				}
			}
		}
	}
	return TRUE;
}


//@@@ 2002.01.18 add start
LPVOID CDlgFuncList::GetHelpIdTable(void)
{
	return (LPVOID)p_helpids;
}
//@@@ 2002.01.18 add end


/*!	キー操作をコマンドに変換するヘルパー関数
	
*/
void CDlgFuncList::Key2Command(WORD KeyCode)
{
	CEditView*	pcEditView;
// novice 2004/10/10
	/* Shift,Ctrl,Altキーが押されていたか */
	int nIdx = getCtrlKeyState();
	EFunctionCode nFuncCode=CKeyBind::GetFuncCode(
			((WORD)(((BYTE)(KeyCode)) | ((WORD)((BYTE)(nIdx))) << 8)),
			m_pShareData->m_Common.m_sKeyBind.m_nKeyNameArrNum,
			m_pShareData->m_Common.m_sKeyBind.m_pKeyNameArr
	);
	switch( nFuncCode ){
	case F_REDRAW:
		nFuncCode=(m_nListType==OUTLINE_BOOKMARK)?F_BOOKMARK_VIEW:F_OUTLINE;
		/*FALLTHROUGH*/
	case F_OUTLINE:
	case F_OUTLINE_TOGGLE: // 20060201 aroka フォーカスがあるときはリロード
	case F_BOOKMARK_VIEW:
		pcEditView=(CEditView*)m_lParam;
		pcEditView->GetCommander().HandleCommand( nFuncCode, TRUE, SHOW_RELOAD, 0, 0, 0 ); // 引数の変更 20060201 aroka

		break;
	case F_BOOKMARK_SET:
		OnJump( false );
		pcEditView=(CEditView*)m_lParam;
		pcEditView->GetCommander().HandleCommand( nFuncCode, TRUE, 0, 0, 0, 0 );

		break;
	case F_COPY:
	case F_CUT:
		OnBnClicked( IDC_BUTTON_COPY );
		break;
	}
}

/*!
	@date 2002.10.05 genta
*/
void CDlgFuncList::Redraw( int nOutLineType, CFuncInfoArr* pcFuncInfoArr, CLayoutInt nCurLine, CLayoutInt nCurCol )
{
	SyncColor();

	m_nListType = nOutLineType;
	m_pcFuncInfoArr = pcFuncInfoArr;	/* 関数情報配列 */
	m_nCurLine = nCurLine;				/* 現在行 */
	m_nCurCol = nCurCol;				/* 現在桁 */
	SetData();
}

//ダイアログタイトルの設定
void CDlgFuncList::SetWindowText( const TCHAR* szTitle )
{
	::SetWindowText( GetHwnd(), szTitle );
}

/** 配色適用処理
	@date 2010.06.05 ryoji 新規作成
*/
void CDlgFuncList::SyncColor( void )
{
	if( !IsDocking() )
		return;
#ifdef DEFINE_SYNCCOLOR
	// テキスト色・背景色をビューと同色にする
	CEditView* pcEditView = (CEditView*)m_lParam;
	STypeConfig	*TypeDataPtr = &(pcEditView->m_pcEditDoc->m_cDocType.GetDocumentAttribute());
	COLORREF clrText = TypeDataPtr->m_ColorInfoArr[COLORIDX_TEXT].m_colTEXT;
	COLORREF clrBack = TypeDataPtr->m_ColorInfoArr[COLORIDX_TEXT].m_colBACK;

	HWND hwndTree = ::GetDlgItem( GetHwnd(), IDC_TREE_FL );
	TreeView_SetTextColor( hwndTree, clrText );
	TreeView_SetBkColor( hwndTree, clrBack );
	{
		// WinNT4.0 あたりではウィンドウスタイルを強制的に再設定しないと
		// ツリーアイテムの左側が真っ黒になる
		LONG lStyle = (LONG)GetWindowLongPtr(hwndTree, GWL_STYLE);
		SetWindowLongPtr( hwndTree, GWL_STYLE, lStyle & ~(TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT) );
		SetWindowLongPtr( hwndTree, GWL_STYLE, lStyle );
	}
	::SetWindowPos( hwndTree, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );	// なぜかこうしないと四辺１ドット幅分だけ色変更が即時適用されない（←スタイル再設定とは無関係）
	::InvalidateRect( hwndTree, NULL, TRUE );

	HWND hwndList = ::GetDlgItem( GetHwnd(), IDC_LIST_FL );
	ListView_SetTextColor( hwndList, clrText );
	ListView_SetTextBkColor( hwndList, clrBack );
	ListView_SetBkColor( hwndList, clrBack );
	::InvalidateRect( hwndList, NULL, TRUE );
#endif
}

/** ドッキング対象矩形の取得（スクリーン座標）
	@date 2010.06.05 ryoji 新規作成
*/
void CDlgFuncList::GetDockSpaceRect( LPRECT pRect )
{
	CEditView* pcEditView = (CEditView*)m_lParam;
	if( IsDocking() ){
		// CDlgFuncList と CSplitterWnd との外接矩形
		HWND hwnd[2];
		RECT rc[2];
		hwnd[0] = GetHwnd();
		hwnd[1] = ::GetParent( pcEditView->GetHwnd() );	// CSplitterWnd
		for( int i = 0; i < 2; i++ ){
			::GetWindowRect(hwnd[i], &rc[i]);
		}
		::UnionRect(pRect, &rc[0], &rc[1]);
	}else{
		// CCSplitterWnd の矩形
		::GetWindowRect( ::GetParent(pcEditView->GetHwnd()), pRect );
	}
}

/**キャプション矩形取得（スクリーン座標）
	@date 2010.06.05 ryoji 新規作成
*/
void CDlgFuncList::GetCaptionRect( LPRECT pRect )
{
	RECT rc;
	::GetWindowRect( GetHwnd(), &rc );
	EDockSide eDockSide = GetDockSide();
	pRect->left = rc.left + ((eDockSide == DOCKSIDE_RIGHT)? DOCK_SPLITTER_WIDTH: 0);
	pRect->top = rc.top + ((eDockSide == DOCKSIDE_BOTTOM)? DOCK_SPLITTER_WIDTH: 0);
	pRect->right = rc.right - ((eDockSide == DOCKSIDE_LEFT)? DOCK_SPLITTER_WIDTH: 0);
	pRect->bottom = pRect->top + (::GetSystemMetrics( SM_CYSMCAPTION ) + 1);
}

/** キャプション上のボタン矩形取得（スクリーン座標）
	@date 2010.06.05 ryoji 新規作成
*/
bool CDlgFuncList::GetCaptionButtonRect( int nButton, LPRECT pRect )
{
	if( !IsDocking() )
		return false;
	if( nButton >= DOCK_BUTTON_NUM )
		return false;
	GetCaptionRect( pRect );
	::OffsetRect( pRect, 0, 1 );
	int cx = ::GetSystemMetrics( SM_CXSMSIZE );
	pRect->left = pRect->right - cx * (nButton + 1);
	pRect->right = pRect->left + cx;
	pRect->bottom = pRect->top + ::GetSystemMetrics( SM_CYSMSIZE );
	return true;
}

/** 分割バーへのヒットテスト（スクリーン座標）
	@date 2010.06.05 ryoji 新規作成
*/
bool CDlgFuncList::HitTestSplitter( int xPos, int yPos )
{
	if( !IsDocking() )
		return false;

	bool bRet = false;
	RECT rc;
	::GetWindowRect(GetHwnd(), &rc);

	EDockSide eDockSide = GetDockSide();
	switch( eDockSide ){
	case DOCKSIDE_LEFT:		bRet = (rc.right - xPos < DOCK_SPLITTER_WIDTH);		break;
	case DOCKSIDE_TOP:		bRet = (rc.bottom - yPos < DOCK_SPLITTER_WIDTH);	break;
	case DOCKSIDE_RIGHT:	bRet = (xPos - rc.left< DOCK_SPLITTER_WIDTH);		break;
	case DOCKSIDE_BOTTOM:	bRet = (yPos - rc.top < DOCK_SPLITTER_WIDTH);		break;
	}

	return bRet;
}

/** キャプション上のボタンへのヒットテスト（スクリーン座標）
	@date 2010.06.05 ryoji 新規作成
*/
int CDlgFuncList::HitTestCaptionButton( int xPos, int yPos )
{
	if( !IsDocking() )
		return -1;

	POINT pt;
	pt.x = xPos;
	pt.y = yPos;

	RECT rcBtn;
	GetCaptionRect( &rcBtn );
	::OffsetRect( &rcBtn, 0, 1 );
	rcBtn.left = rcBtn.right - ::GetSystemMetrics( SM_CXSMSIZE );
	rcBtn.bottom = rcBtn.top + ::GetSystemMetrics( SM_CYSMSIZE );
	int nBtn = -1;
	for( int i = 0; i < DOCK_BUTTON_NUM; i++ ){
		if( ::PtInRect( &rcBtn, pt ) ){
			nBtn = i;	// 右端から i 番目のボタン上
			break;
		}
		::OffsetRect( &rcBtn, -(rcBtn.right - rcBtn.left), 0 );
	}

	return nBtn;
}

/** WM_NCCALCSIZE 処理
	@date 2010.06.05 ryoji 新規作成
*/
INT_PTR CDlgFuncList::OnNcCalcSize( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( !IsDocking() )
		return 0L;

	// 自ウィンドウのクライアント領域を定義する
	// これでキャプションや分割バーを非クライアント領域にすることができる
	NCCALCSIZE_PARAMS* pNCS = (NCCALCSIZE_PARAMS*)lParam;
	pNCS->rgrc[0].top += (::GetSystemMetrics( SM_CYSMCAPTION ) + 1);
	switch( GetDockSide() ){
	case DOCKSIDE_LEFT:		pNCS->rgrc[0].right -= DOCK_SPLITTER_WIDTH;		break;
	case DOCKSIDE_TOP:		pNCS->rgrc[0].bottom -= DOCK_SPLITTER_WIDTH;	break;
	case DOCKSIDE_RIGHT:	pNCS->rgrc[0].left += DOCK_SPLITTER_WIDTH;		break;
	case DOCKSIDE_BOTTOM:	pNCS->rgrc[0].top += DOCK_SPLITTER_WIDTH;		break;
	}
	return 1L;
}

/** WM_NCHITTEST 処理
	@date 2010.06.05 ryoji 新規作成
*/
INT_PTR CDlgFuncList::OnNcHitTest( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( !IsDocking() )
		return 0L;

	INT_PTR nRet = HTERROR;
	POINT pt;
	pt.x = MAKEPOINTS(lParam).x;
	pt.y = MAKEPOINTS(lParam).y;
	if( HitTestSplitter(pt.x, pt.y) ){
		switch( GetDockSide() ){
		case DOCKSIDE_LEFT:		nRet = HTRIGHT;		break;
		case DOCKSIDE_TOP:		nRet = HTBOTTOM;	break;
		case DOCKSIDE_RIGHT:	nRet = HTLEFT;		break;
		case DOCKSIDE_BOTTOM:	nRet = HTTOP;		break;
		}
	}else {
		RECT rc;
		GetCaptionRect( &rc );
		nRet = ::PtInRect( &rc, pt )? HTCAPTION: HTCLIENT;
	}
	::SetWindowLongPtr( GetHwnd(), DWLP_MSGRESULT, nRet );

	return nRet;
}

/** WM_TIMER 処理
	@date 2010.06.05 ryoji 新規作成
*/
INT_PTR CDlgFuncList::OnTimer( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( !IsDocking() )
		return 0L;

	if( wParam == 1 ){
		// カーソルがウィンドウ外にある場合にも WM_NCMOUSEMOVE を送る
		POINT pt;
		RECT rc;
		::GetCursorPos( &pt );
		::GetWindowRect( hwnd, &rc );
		if( !::PtInRect( &rc, pt ) ){
			::SendMessageAny( hwnd, WM_NCMOUSEMOVE, 0, MAKELONG( pt.x, pt.y ) );
		}
	}

	return 0L;
}

/** WM_NCMOUSEMOVE 処理
	@date 2010.06.05 ryoji 新規作成
*/
INT_PTR CDlgFuncList::OnNcMouseMove( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( !IsDocking() )
		return 0L;

	POINT pt;
	pt.x = MAKEPOINTS(lParam).x;
	pt.y = MAKEPOINTS(lParam).y;

	// カーソルがウィンドウ内に入ったらタイマー起動
	// ウィンドウ外に出たらタイマー削除
	RECT rc;
	::GetWindowRect( GetHwnd(), &rc );
	bool bHovering = ::PtInRect( &rc, pt )? true: false;
	if( bHovering != m_bHovering )
	{
		m_bHovering = bHovering;
		if( m_bHovering )
			::SetTimer( hwnd, 1, 200, NULL );
		else
			::KillTimer( hwnd, 1 );
	}

	// マウスカーソルがボタン上にあればハイライト
	int nHilightedBtn = HitTestCaptionButton(pt.x, pt.y);
	if( nHilightedBtn != m_nHilightedBtn ){
		// ハイライト状態の変更を反映するために再描画する
		m_nHilightedBtn = nHilightedBtn;
		::RedrawWindow( GetHwnd(), NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOINTERNALPAINT );

		// ツールチップ更新
		TOOLINFO ti;
		::ZeroMemory( &ti, sizeof(ti) );
		ti.cbSize       = sizeof(ti);
		ti.hwnd         = GetHwnd();
		ti.hinst        = m_hInstance;
		ti.uId          = (UINT)GetHwnd();
		switch( m_nHilightedBtn ){
		case 0: ti.lpszText = _T("閉じる"); break;
		case 1: ti.lpszText = _T("ウィンドウの位置"); break;
		case 2: ti.lpszText = _T("更新"); break;
		default: ti.lpszText = NULL;	// 消す
		}
		Tooltip_UpdateTipText( m_hwndToolTip, &ti );
	}

	return 0L;
}

/** WM_MOUSEMOVE 処理
	@date 2010.06.05 ryoji 新規作成
*/
INT_PTR CDlgFuncList::OnMouseMove( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( !IsDocking() )
		return 0L;

	if( m_bStretching ){	// マウスのドラッグ位置にあわせてサイズを変更する
		POINT pt;
		pt.x = MAKEPOINTS(lParam).x;
		pt.y = MAKEPOINTS(lParam).y;
		::ClientToScreen( GetHwnd(), &pt );

		RECT rc;
		GetDockSpaceRect(&rc);

		// 画面サイズが小さすぎるときは何もしない
		EDockSide eDockSide = GetDockSide();
		if( eDockSide == DOCKSIDE_LEFT || eDockSide == DOCKSIDE_RIGHT ){
			if( rc.right - rc.left < DOCK_MIN_SIZE )
				return 0L;
		}else{
			if( rc.bottom - rc.top < DOCK_MIN_SIZE )
				return 0L;
		}

		// マウスが上下左右に行き過ぎなら範囲内に調整する
		if( pt.x > rc.right - DOCK_MIN_SIZE ) pt.x = rc.right - DOCK_MIN_SIZE;
		if( pt.x < rc.left + DOCK_MIN_SIZE ) pt.x = rc.left + DOCK_MIN_SIZE;
		if( pt.y > rc.bottom - DOCK_MIN_SIZE ) pt.y = rc.bottom - DOCK_MIN_SIZE;
		if( pt.y < rc.top + DOCK_MIN_SIZE ) pt.y = rc.top + DOCK_MIN_SIZE;

		// クライアント座標系に変換して新しい位置とサイズを計算する
		POINT ptLT;
		ptLT.x = rc.left;
		ptLT.y = rc.top;
		::ScreenToClient( m_hwndParent, &ptLT );
		::OffsetRect( &rc, ptLT.x - rc.left, ptLT.y - rc.top );
		::ScreenToClient( m_hwndParent, &pt );
		switch( eDockSide ){
		case DOCKSIDE_LEFT:		rc.right = pt.x - DOCK_SPLITTER_WIDTH / 2 + DOCK_SPLITTER_WIDTH;	break;
		case DOCKSIDE_TOP:		rc.bottom = pt.y - DOCK_SPLITTER_WIDTH / 2 + DOCK_SPLITTER_WIDTH;	break;
		case DOCKSIDE_RIGHT:	rc.left = pt.x - DOCK_SPLITTER_WIDTH / 2;	break;
		case DOCKSIDE_BOTTOM:	rc.top = pt.y - DOCK_SPLITTER_WIDTH / 2;	break;
		}

		// 以前と同じ配置なら無駄に移動しない
		RECT rcOld;
		::GetWindowRect( GetHwnd(), &rcOld );
		ptLT.x = rcOld.left;
		ptLT.y = rcOld.top;
		::ScreenToClient( m_hwndParent, &ptLT );
		::OffsetRect( &rcOld, ptLT.x - rcOld.left, ptLT.y - rcOld.top );
		if( ::EqualRect( &rcOld, &rc ) )
			return 0L;

		// 移動する
		::SetWindowPos( GetHwnd(), NULL,
			rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE );
		((CEditView*)m_lParam)->m_pcEditWnd->EndLayoutBars( m_bEditWndReady );

		// 移動後の配置情報を記憶する
		GetWindowRect( GetHwnd(), &rc );
		switch( GetDockSide() ){
		case DOCKSIDE_LEFT:		ProfDockLeft() = rc.right - rc.left;	break;
		case DOCKSIDE_TOP:		ProfDockTop() = rc.bottom - rc.top;		break;
		case DOCKSIDE_RIGHT:	ProfDockRight() = rc.right - rc.left;	break;
		case DOCKSIDE_BOTTOM:	ProfDockBottom() = rc.bottom - rc.top;	break;
		}
		return 1L;
	}

	return 0L;
}

/** WM_NCLBUTTONDOWN 処理
	@date 2010.06.05 ryoji 新規作成
*/
INT_PTR CDlgFuncList::OnNcLButtonDown( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	POINT pt;
	pt.x = MAKEPOINTS(lParam).x;
	pt.y = MAKEPOINTS(lParam).y;

	if( !IsDocking() ){
		if( GetDockSide() == DOCKSIDE_FLOAT ){
			if( wParam == HTCAPTION  && !::IsZoomed(GetHwnd()) && !::IsIconic(GetHwnd()) ){
				::SetActiveWindow( GetHwnd() );
				// 上の SetActiveWindow() で WM_ACTIVATEAPP へ行くケースでは、WM_ACTIVATEAPP に入れた特殊処理（エディタ本体を一時的にアクティブ化して戻す）
				// に余計に時間がかかるため、上の SetActiveWindow() 後にはボタンが離されていることがある。その場合は Track() を開始せずに抜ける。
				if( (::GetAsyncKeyState( ::GetSystemMetrics(SM_SWAPBUTTON)? VK_RBUTTON: VK_LBUTTON ) & 0x8000) == 0 )
					return 1L;	// ボタンは既に離されている
				Track( pt );	// タイトルバーのドラッグ＆ドロップによるドッキング配置変更
				return 1L;
			}
		}
		return 0L;
	}

	int nBtn;
	if( HitTestSplitter(pt.x, pt.y) ){	// 分割バー
		m_bStretching = true;
		::SetCapture( GetHwnd() );	// OnMouseMoveでのサイズ制限のために自前のキャプチャが必要
		return 1L;
	}else{
		if( (nBtn = HitTestCaptionButton(pt.x, pt.y)) >= 0 ){	// キャプション上のボタン
			if( nBtn == 1 ){	// メニュー
				RECT rcBtn;
				GetCaptionButtonRect( nBtn, &rcBtn );
				pt.x = rcBtn.left;
				pt.y = rcBtn.bottom;
				DoMenu( pt, GetHwnd() );
				// メニュー選択せずにリストやツリーをクリックしたらボタンがハイライトのままになるので更新
				::RedrawWindow( GetHwnd(), NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOINTERNALPAINT );
			}else{
				m_nCapturingBtn = nBtn;
				::SetCapture( GetHwnd() );
			}
		}else{	// 残りはタイトルバーのみ
			Track( pt );	// タイトルバーのドラッグ＆ドロップによるドッキング配置変更
		}
		return 1L;
	}

	return 0L;
}

/** WM_LBUTTONUP 処理
	@date 2010.06.05 ryoji 新規作成
*/
INT_PTR CDlgFuncList::OnLButtonUp( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( !IsDocking() )
		return 0L;

	if( m_bStretching ){
		::ReleaseCapture();
		m_bStretching = false;

		if( ProfDockSync() ){
			// 他ウィンドウに変更を通知する
			HWND hwndEdit = ((CEditView*)m_lParam)->m_pcEditWnd->GetHwnd();
			PostOutlineNotifyToAllEditors( (WPARAM)0, (LPARAM)hwndEdit );
		}
		return 1L;
	}

	if( m_nCapturingBtn >= 0 ){
		::ReleaseCapture();
		POINT pt;
		pt.x = MAKEPOINTS(lParam).x;
		pt.y = MAKEPOINTS(lParam).y;
		::ClientToScreen( GetHwnd(), &pt );
		int nBtn = HitTestCaptionButton( pt.x, pt.y);
		if( nBtn == m_nCapturingBtn ){
			if( nBtn == 0 ){	// 閉じる
				::DestroyWindow( GetHwnd() );
			}else if( m_nCapturingBtn == 2 ){	// 更新
				EFunctionCode nFuncCode = (m_nListType == OUTLINE_BOOKMARK)? F_BOOKMARK_VIEW: F_OUTLINE;
				CEditView* pcEditView = (CEditView*)m_lParam;
				pcEditView->GetCommander().HandleCommand( nFuncCode, TRUE, SHOW_RELOAD, 0, 0, 0 );
			}
		}
		m_nCapturingBtn = -1;
		return 1L;
	}

	return 0L;
}

/** WM_NCPAINT 処理
	@date 2010.06.05 ryoji 新規作成
*/
INT_PTR CDlgFuncList::OnNcPaint( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( !IsDocking() )
		return 0L;

	EDockSide eDockSide = GetDockSide();

	HDC hdc;
	RECT rc, rcScr, rcWk;

	//描画対象
	hdc = ::GetWindowDC( hwnd );
	CGraphics gr(hdc);
	::GetWindowRect( hwnd, &rcScr );
	rc = rcScr;
	::OffsetRect( &rc, -rcScr.left, -rcScr.top );

	// 背景を描画する
	//::FillRect( gr, &rc, (HBRUSH)(COLOR_3DFACE + 1) );

	// 分割線を描画する
	rcWk = rc;
	switch( eDockSide ){
	case DOCKSIDE_LEFT:		rcWk.left = rcWk.right - DOCK_SPLITTER_WIDTH; break;
	case DOCKSIDE_TOP:		rcWk.top = rcWk.bottom - DOCK_SPLITTER_WIDTH; break;
	case DOCKSIDE_RIGHT:	rcWk.right = rcWk.left + DOCK_SPLITTER_WIDTH; break;
	case DOCKSIDE_BOTTOM:	rcWk.bottom = rcWk.top + DOCK_SPLITTER_WIDTH; break;
	}
	::FillRect( gr, &rcWk, (HBRUSH)(COLOR_3DFACE + 1) );
	::DrawEdge( gr, &rcWk, EDGE_ETCHED, BF_TOPLEFT );

	// タイトルを描画する
	BOOL bThemeActive = CUxTheme::getInstance().IsThemeActive();
	BOOL bGradient = FALSE;
	::SystemParametersInfo( SPI_GETGRADIENTCAPTIONS, 0, &bGradient, 0 );
	if( !bThemeActive ) bGradient = FALSE;	// 適当に調整
	HWND hwndFocus = ::GetFocus();
	BOOL bActive = (GetHwnd() == hwndFocus || ::IsChild(GetHwnd(), hwndFocus));
	RECT rcCaption;
	GetCaptionRect( &rcCaption );
	::OffsetRect( &rcCaption, -rcScr.left, -rcScr.top );
	rcWk = rcCaption;
	rcWk.top += 1;
	rcWk.right -= DOCK_BUTTON_NUM * (::GetSystemMetrics( SM_CXSMSIZE ));
	// ↓DrawCaption() に DC_SMALLCAP を指定してはいけないっぽい
	// ↓DC_SMALLCAP 指定のものを Win7(64bit版) で動かしてみたら描画位置が下にずれて上半分しか見えなかった（x86ビルド/x64ビルドのどちらも NG）
	::DrawCaption( hwnd, gr, &rcWk, DC_TEXT | (bGradient? DC_GRADIENT: 0) /*| DC_SMALLCAP*/ | (bActive? DC_ACTIVE: 0) );
	rcWk.left = rcCaption.right;
	int nClrCaption;
	if( bGradient )
		nClrCaption = ( bActive? COLOR_GRADIENTACTIVECAPTION: COLOR_GRADIENTINACTIVECAPTION );
	else
		nClrCaption = ( bActive? COLOR_ACTIVECAPTION: COLOR_INACTIVECAPTION );
	::FillRect( gr, &rcWk, ::GetSysColorBrush( nClrCaption ) );
	::DrawEdge( gr, &rcCaption, BDR_SUNKENOUTER, BF_TOP );

	// タイトル上のボタンを描画する
	NONCLIENTMETRICS ncm;
	ncm.cbSize = CCSIZEOF_STRUCT( NONCLIENTMETRICS, lfMessageFont );	// 以前のプラットフォームに WINVER >= 0x0600 で定義される構造体のフルサイズを渡すと失敗する
	::SystemParametersInfo( SPI_GETNONCLIENTMETRICS, ncm.cbSize, (PVOID)&ncm, 0 );
	LOGFONT lf;
	memset( &lf, 0, sizeof(LOGFONT) );
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfHeight = ncm.lfCaptionFont.lfHeight;
	::lstrcpy( lf.lfFaceName, _T("Marlett") );
	HFONT hFont = ::CreateFontIndirect( &lf );
	::lstrcpy( lf.lfFaceName, _T("Wingdings") );
	HFONT hFont2 = ::CreateFontIndirect( &lf );
	gr.SetTextBackTransparent( true );

	static const TCHAR szBtn[DOCK_BUTTON_NUM] = { (TCHAR)0x72/* 閉じる */, (TCHAR)0x36/* メニュー */, (TCHAR)0xFF/* 更新 */ };
	HFONT hFontBtn[DOCK_BUTTON_NUM] = { hFont/* 閉じる */, hFont/* メニュー */, hFont2/* 更新 */ };
	POINT pt;
	::GetCursorPos( &pt );
	pt.x -= rcScr.left;
	pt.y -= rcScr.top;
	RECT rcBtn = rcCaption;
	::OffsetRect( &rcBtn, 0, 1 );
	rcBtn.left = rcBtn.right - ::GetSystemMetrics( SM_CXSMSIZE );
	rcBtn.bottom = rcBtn.top + ::GetSystemMetrics( SM_CYSMSIZE );
	for( int i = 0; i < DOCK_BUTTON_NUM; i++ ){
		int nClrCaptionText;
		// マウスカーソルがボタン上にあればハイライト
		if( ::PtInRect( &rcBtn, pt ) ){
			::FillRect( gr, &rcBtn, ::GetSysColorBrush( (bGradient && !bActive)? COLOR_INACTIVECAPTION: COLOR_ACTIVECAPTION ) );
			nClrCaptionText = ( (bGradient && !bActive)? COLOR_INACTIVECAPTIONTEXT: COLOR_CAPTIONTEXT );
		}else{
			nClrCaptionText = ( bActive? COLOR_CAPTIONTEXT: COLOR_INACTIVECAPTIONTEXT );
		}
		gr.PushMyFont( hFontBtn[i] );
		::SetTextColor( gr, ::GetSysColor( nClrCaptionText ) );
		::DrawText( gr, &szBtn[i], 1, &rcBtn, DT_SINGLELINE | DT_CENTER | DT_VCENTER );
		::OffsetRect( &rcBtn, -(rcBtn.right - rcBtn.left), 0 );
		gr.PopMyFont();
	}

	::DeleteObject( hFont );
	::DeleteObject( hFont2 );

	::ReleaseDC( hwnd, hdc );
	return 1L;
}

/** メニュー処理
	@date 2010.06.05 ryoji 新規作成
*/
void CDlgFuncList::DoMenu( POINT pt, HWND hwndFrom )
{
	// メニューを作成する
	CEditView* pcEditView = &CEditDoc::GetInstance(0)->m_pcEditWnd->GetActiveView();
	EDockSide eDockSide = ProfDockSide();	// 設定上の配置
	UINT uFlags = MF_BYPOSITION | MF_STRING;
	HMENU hMenu = ::CreatePopupMenu();
	HMENU hMenuSub = ::CreatePopupMenu();
	int iPos = 0;
	int iPosSub = 0;
	HMENU& hMenuRef = ( hwndFrom == GetHwnd() )? hMenu: hMenuSub;
	int& iPosRef = ( hwndFrom == GetHwnd() )? iPos: iPosSub;

	if( hwndFrom != GetHwnd() ){
		// 将来、ここに hwndFrom に応じた状況依存メニューを追加するといいかも
		// （ツリーなら「すべて展開」／「すべて縮小」とか、そういうの）
		::InsertMenu( hMenu, iPos++, MF_BYPOSITION | MF_STRING, 450, _T("更新(&U)") );
		::InsertMenu( hMenu, iPos++, MF_BYPOSITION | MF_STRING, 451, _T("コピー(&C)") );
		::InsertMenu( hMenu, iPos++, MF_BYPOSITION | MF_SEPARATOR, 0,	NULL );
		::InsertMenu( hMenu, iPos++, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuSub,	_T("ウィンドウの位置(&W)") );
	}

	int iFrom = iPosRef;
	::InsertMenu( hMenuRef, iPosRef++, uFlags, 100 + DOCKSIDE_LEFT,		_T("左ドッキング(&L)") );
	::InsertMenu( hMenuRef, iPosRef++, uFlags, 100 + DOCKSIDE_RIGHT,	_T("右ドッキング(&R)") );
	::InsertMenu( hMenuRef, iPosRef++, uFlags, 100 + DOCKSIDE_TOP,		_T("上ドッキング(&T)") );
	::InsertMenu( hMenuRef, iPosRef++, uFlags, 100 + DOCKSIDE_BOTTOM,	_T("下ドッキング(&B)") );
	::InsertMenu( hMenuRef, iPosRef++, uFlags, 100 + DOCKSIDE_FLOAT,	_T("フローティング(&F)") );
	::InsertMenu( hMenuRef, iPosRef++, uFlags, 100 + DOCKSIDE_UNDOCKABLE,	_T("ドッキング禁止(&I)") );
	int iTo = iPosRef - 1;
	for( int i = iFrom; i <= iTo; i++ ){
		if( ::GetMenuItemID( hMenuRef, i ) == (100 + eDockSide) ){
			::CheckMenuRadioItem( hMenuRef, iFrom, iTo, i, MF_BYPOSITION );
			break;
		}
	}
	::InsertMenu( hMenuRef, iPosRef++, MF_BYPOSITION | MF_SEPARATOR, 0,	NULL );
	::InsertMenu( hMenuRef, iPosRef++, uFlags, 200,	_T("ドッキング配置を同期(&S)") );
	::CheckMenuItem( hMenuRef, 200, MF_BYCOMMAND | ProfDockSync()? MF_CHECKED: MF_UNCHECKED );
	::InsertMenu( hMenuRef, iPosRef++, MF_BYPOSITION | MF_SEPARATOR, 0,	NULL );
	::InsertMenu( hMenuRef, iPosRef++, MF_BYPOSITION | MF_STRING, 300, _T("ドッキング配置を共通継承(&C)") );
	::InsertMenu( hMenuRef, iPosRef++, MF_BYPOSITION | MF_STRING, 301, _T("ドッキング配置をタイプ別継承(&Y)") );
	::CheckMenuRadioItem( hMenuRef, 300, 301, (ProfDockSet() == 0)? 300: 301, MF_BYCOMMAND );
	::InsertMenu( hMenuRef, iPosRef++, MF_BYPOSITION | MF_SEPARATOR, 0,	NULL );
	::InsertMenu( hMenuRef, iPosRef++, MF_BYPOSITION | MF_STRING, 305, _T("継承情報を統一(&U)") );

	if( hwndFrom != GetHwnd() ){
		::InsertMenu( hMenu, iPos++, MF_BYPOSITION | MF_SEPARATOR, 0,	NULL );
		::InsertMenu( hMenu, iPos++, MF_BYPOSITION | MF_STRING, 452, _T("閉じる(&X)") );
	}

	// メニューを表示する
	RECT rcWork;
	GetMonitorWorkRect( pt, &rcWork );	// モニタのワークエリア
	int nId = ::TrackPopupMenu( hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD,
								( pt.x > rcWork.left )? pt.x: rcWork.left,
								( pt.y < rcWork.bottom )? pt.y: rcWork.bottom,
								0, GetHwnd(), NULL);
	::DestroyMenu( hMenu );	// サブメニューは再帰的に破棄される

	// メニュー選択された状態に切り替える
	HWND hwndEdit = pcEditView->m_pcEditWnd->GetHwnd();
	if( nId == 450 ){	// 更新
		EFunctionCode nFuncCode = (m_nListType == OUTLINE_BOOKMARK)? F_BOOKMARK_VIEW: F_OUTLINE;
		CEditView* pcEditView = (CEditView*)m_lParam;
		pcEditView->GetCommander().HandleCommand( nFuncCode, TRUE, SHOW_RELOAD, 0, 0, 0 );
	}
	else if( nId == 451 ){	// コピー
		// Windowsクリップボードにコピー 
		SetClipboardText( GetHwnd(), m_cmemClipText.GetStringPtr(), m_cmemClipText.GetStringLength() );
	}
	else if( nId == 452 ){	// 閉じる
		::DestroyWindow( GetHwnd() );
	}
	else if( nId == 300 || nId == 301 ){	// ドッキング配置の継承方法
		ProfDockSet() = nId - 300;
		ChangeLayout( OUTLINE_LAYOUT_FOREGROUND );	// 自分自身への強制変更
		if( ProfDockSync() ){
			PostOutlineNotifyToAllEditors( (WPARAM)0, (LPARAM)hwndEdit );	// 他ウィンドウにドッキング配置変更を通知する
		}
	}
	else if( nId == 305 ){	// 設定コピー
		if( IDOK == ::MYMESSAGEBOX( hwndEdit,
						MB_OKCANCEL | MB_ICONINFORMATION, GSTR_APPNAME,
						_T("現在の画面のドッキング配置情報をすべての共通設定・タイプ別設定にコピーして統一します。")
						_T("\n（現在開いている他画面の状態も統一します。）\n") ) ){
			CommonSet().m_bOutlineDockDisp = GetHwnd()? TRUE: FALSE;
			CommonSet().m_eOutlineDockSide = GetDockSide();
			if( GetHwnd() ){
				RECT rc;
				GetWindowRect( GetHwnd(), &rc );
				switch( GetDockSide() ){	// 現在のドッキングモード
					case DOCKSIDE_LEFT:		CommonSet().m_cxOutlineDockLeft = rc.right - rc.left;	break;
					case DOCKSIDE_TOP:		CommonSet().m_cyOutlineDockTop = rc.bottom - rc.top;	break;
					case DOCKSIDE_RIGHT:	CommonSet().m_cxOutlineDockRight = rc.right - rc.left;	break;
					case DOCKSIDE_BOTTOM:	CommonSet().m_cyOutlineDockBottom = rc.bottom - rc.top;	break;
				}
			}
			for( int i = 0; i < MAX_TYPES; i++ ){
				STypeConfig& type = CDocTypeManager().GetTypeSetting( CTypeConfig(i) );
				type.m_bOutlineDockDisp = CommonSet().m_bOutlineDockDisp;
				type.m_eOutlineDockSide = CommonSet().m_eOutlineDockSide;
				type.m_cxOutlineDockLeft = CommonSet().m_cxOutlineDockLeft;
				type.m_cyOutlineDockTop = CommonSet().m_cyOutlineDockTop;
				type.m_cxOutlineDockRight = CommonSet().m_cxOutlineDockRight;
				type.m_cyOutlineDockBottom = CommonSet().m_cyOutlineDockBottom;
			}
			ChangeLayout( OUTLINE_LAYOUT_FOREGROUND );	// 自分自身への強制変更
			PostOutlineNotifyToAllEditors( (WPARAM)0, (LPARAM)hwndEdit );	// 他ウィンドウにドッキング配置変更を通知する
		}
	}
	else if( nId == 200 ){	// ドッキング配置の同期をとる
		ProfDockSync() = !ProfDockSync();
		ChangeLayout( OUTLINE_LAYOUT_FOREGROUND );	// 自分自身への強制変更
		if( ProfDockSync() ){
			PostOutlineNotifyToAllEditors( (WPARAM)0, (LPARAM)hwndEdit );	// 他ウィンドウにドッキング配置変更を通知する
		}
	}
	else if( nId >= 100 - 1 ){	// ドッキングモード （※ DOCKSIDE_UNDOCKABLE は -1 です） */
		int* pnWidth;
		int* pnHeight;
		RECT rc;
		GetDockSpaceRect( &rc );
		eDockSide = EDockSide(nId - 100);	// 新しいドッキングモード
		if( eDockSide > DOCKSIDE_FLOAT ){
			switch( eDockSide ){
			case DOCKSIDE_LEFT:		pnWidth = &ProfDockLeft();		break;
			case DOCKSIDE_TOP:		pnHeight = &ProfDockTop();		break;
			case DOCKSIDE_RIGHT:	pnWidth = &ProfDockRight();		break;
			case DOCKSIDE_BOTTOM:	pnHeight = &ProfDockBottom();	break;
			}
			if( eDockSide == DOCKSIDE_LEFT || eDockSide == DOCKSIDE_RIGHT ){
				if( *pnWidth == 0 )	// 初回
					*pnWidth = (rc.right - rc.left) / 3;
				if( *pnWidth > rc.right - rc.left - DOCK_MIN_SIZE ) *pnWidth = rc.right - rc.left - DOCK_MIN_SIZE;
				if( *pnWidth < DOCK_MIN_SIZE ) *pnWidth = DOCK_MIN_SIZE;
			}else{
				if( *pnHeight == 0 )	// 初回
					*pnHeight = (rc.bottom - rc.top) / 3;
				if( *pnHeight > rc.bottom - rc.top - DOCK_MIN_SIZE ) *pnHeight = rc.bottom - rc.top - DOCK_MIN_SIZE;
				if( *pnHeight < DOCK_MIN_SIZE ) *pnHeight = DOCK_MIN_SIZE;
			}
		}

		// ドッキング配置変更
		ProfDockDisp() = GetHwnd()? TRUE: FALSE;
		ProfDockSide() = eDockSide;	// 新しいドッキングモードを適用
		ChangeLayout( OUTLINE_LAYOUT_FOREGROUND );	// 自分自身への強制変更
		if( ProfDockSync() ){
			PostOutlineNotifyToAllEditors( (WPARAM)0, (LPARAM)hwndEdit );	// 他ウィンドウにドッキング配置変更を通知する
		}
	}
}

/** 現在の設定に応じて表示を刷新する
	@date 2010.06.05 ryoji 新規作成
*/
void CDlgFuncList::Refresh( void )
{
	CEditWnd* pcEditWnd = CEditDoc::GetInstance(0)->m_pcEditWnd;
	BOOL bReloaded = ChangeLayout( OUTLINE_LAYOUT_FILECHANGED );	// 現在設定に従ってアウトライン画面を再配置する
	if( !bReloaded && pcEditWnd->m_cDlgFuncList.GetHwnd() ){
		int nOutlineType = (m_nListType == OUTLINE_BOOKMARK)? OUTLINE_BOOKMARK: OUTLINE_DEFAULT;
		pcEditWnd->GetActiveView().GetCommander().Command_FUNCLIST( SHOW_RELOAD, nOutlineType );	// 開く	※ HandleCommand(F_OUTLINE,...) だと印刷プレビュー状態で実行されないので Command_FUNCLIST()
	}
	if( MyGetAncestor( ::GetForegroundWindow(), GA_ROOTOWNER2 ) == pcEditWnd->GetHwnd() )
		::SetFocus( pcEditWnd->GetActiveView().GetHwnd() );	// フォーカスを戻す
}

/** 現在の設定に応じて配置を変更する（できる限り再解析しない）

	@param nId [in] 動作指定．OUTLINE_LAYOUT_FOREGROUND: 前面用の動作 / OUTLINE_LAYOUT_BACKGROUND: 背後用の動作 / OUTLINE_LAYOUT_FILECHANGED: ファイル切替用の動作（前面だが特殊）
	@retval 解析を実行したかどうか．true: 実行した / false: 実行しなかった

	@date 2010.06.10 ryoji 新規作成
*/
bool CDlgFuncList::ChangeLayout( int nId )
{
	struct SAutoSwitch
	{
		SAutoSwitch( bool* pbSwitch ): m_pbSwitch( pbSwitch ) { *m_pbSwitch = true; }
		~SAutoSwitch() { *m_pbSwitch = false; }
		bool* m_pbSwitch;
	} SAutoSwitch( &m_bInChangeLayout );	// 処理中は m_bInChangeLayout フラグを ON にしておく

	CEditDoc* pDoc = CEditDoc::GetInstance(0);	// 今は非表示かもしれないので (CEditView*)m_lParam は使えない
	BOOL bDockDisp = ProfDockDisp();
	EDockSide eDockSideNew = ProfDockSide();

	if( !GetHwnd() ){	// 現在は非表示
		if( bDockDisp ){	// 新設定は表示
			if( eDockSideNew <= DOCKSIDE_FLOAT ){
				if( nId == OUTLINE_LAYOUT_BACKGROUND ) return false;	// 裏ではフローティングは開かない（従来互換）※無理に開くとタブモード時は画面が切り替わってしまう
				if( nId == OUTLINE_LAYOUT_FILECHANGED ) return false;	// ファイル切替ではフローティングは開かない（従来互換）
			}
			// ※ 裏では一時的に Disable 化しておいて開く（タブモードでの不正な画面切り替え抑止）
			CEditView* pcEditView = &pDoc->m_pcEditWnd->GetActiveView();
			if( nId == OUTLINE_LAYOUT_BACKGROUND ) ::EnableWindow( pcEditView->m_pcEditWnd->GetHwnd(), FALSE );
			int nOutlineType = (m_nListType == OUTLINE_BOOKMARK)? OUTLINE_BOOKMARK: OUTLINE_DEFAULT;	// ブックマークかアウトライン解析かは最後に開いていた時の状態を引き継ぐ（初期状態はアウトライン解析）
			pcEditView->GetCommander().Command_FUNCLIST( SHOW_NORMAL, nOutlineType );	// 開く	※ HandleCommand(F_OUTLINE,...) だと印刷プレビュー状態で実行されないので Command_FUNCLIST()
			if( nId == OUTLINE_LAYOUT_BACKGROUND ) ::EnableWindow( pcEditView->m_pcEditWnd->GetHwnd(), TRUE );
			return true;	// 解析した
		}
	}else{	// 現在は表示
		EDockSide eDockSideOld = GetDockSide();

		CEditView* pcEditView = (CEditView*)m_lParam;
		if( !bDockDisp ){	// 新設定は非表示
			if( eDockSideOld <= DOCKSIDE_FLOAT ){	// 現在はフローティング
				if( nId == OUTLINE_LAYOUT_BACKGROUND ) return false;	// 裏ではフローティングは閉じない（従来互換）
				if( nId == OUTLINE_LAYOUT_FILECHANGED && eDockSideNew <= DOCKSIDE_FLOAT ) return false;	// ファイル切替では新設定もフローティングなら再利用（従来互換）
			}
			::DestroyWindow( GetHwnd() );	// 閉じる
			return false;
		}

		// ドッキング⇔フローティング切替では閉じて開く
		if( (eDockSideOld <= DOCKSIDE_FLOAT) != (eDockSideNew <= DOCKSIDE_FLOAT) ){
			::DestroyWindow( GetHwnd() );	// 閉じる
			if( eDockSideNew <= DOCKSIDE_FLOAT ){	// 新設定はフローティング
				m_xPos = m_yPos = -1;	// 画面位置を初期化する
				if( nId == OUTLINE_LAYOUT_BACKGROUND ) return false;	// 裏ではフローティングは開かない（従来互換）※無理に開くとタブモード時は画面が切り替わってしまう
				if( nId == OUTLINE_LAYOUT_FILECHANGED ) return false;	// ファイル切替ではフローティングは開かない（従来互換）
			}
			// ※ 裏では一時的に Disable 化しておいて開く（タブモードでの不正な画面切り替え抑止）
			if( nId == OUTLINE_LAYOUT_BACKGROUND ) ::EnableWindow( pcEditView->m_pcEditWnd->GetHwnd(), FALSE );
			int nOutlineType = (m_nListType == OUTLINE_BOOKMARK)? OUTLINE_BOOKMARK: OUTLINE_DEFAULT;
			pcEditView->GetCommander().Command_FUNCLIST( SHOW_NORMAL, nOutlineType );	// 開く	※ HandleCommand(F_OUTLINE,...) だと印刷プレビュー状態で実行されないので Command_FUNCLIST()
			if( nId == OUTLINE_LAYOUT_BACKGROUND ) ::EnableWindow( pcEditView->m_pcEditWnd->GetHwnd(), TRUE );
			return true;	// 解析した
		}

		// フローティング→フローティングでは配置同期せずに現状維持
		if( eDockSideOld <= DOCKSIDE_FLOAT ){
			m_eDockSide = eDockSideNew;
			return false;
		}

		// ドッキング→ドッキングでは配置同期
		RECT rc;
		POINT ptLT;
		GetDockSpaceRect( &rc );
		ptLT.x = rc.left;
		ptLT.y = rc.top;
		::ScreenToClient( m_hwndParent, &ptLT );
		::OffsetRect( &rc, ptLT.x - rc.left, ptLT.y - rc.top );

		switch( eDockSideNew ){
		case DOCKSIDE_LEFT:		rc.right = rc.left + ProfDockLeft();	break;
		case DOCKSIDE_TOP:		rc.bottom = rc.top + ProfDockTop();		break;
		case DOCKSIDE_RIGHT:	rc.left = rc.right - ProfDockRight();	break;
		case DOCKSIDE_BOTTOM:	rc.top = rc.bottom - ProfDockBottom();	break;
		}

		// 以前と同じ配置なら無駄に移動しない
		RECT rcOld;
		::GetWindowRect( GetHwnd(), &rcOld );
		ptLT.x = rcOld.left;
		ptLT.y = rcOld.top;
		::ScreenToClient( m_hwndParent, &ptLT );
		::OffsetRect( &rcOld, ptLT.x - rcOld.left, ptLT.y - rcOld.top );
		if( eDockSideOld == eDockSideNew && ::EqualRect( &rcOld, &rc ) ){
			::InvalidateRect( GetHwnd(), NULL, TRUE );	// いちおう再描画だけ
			return false;	// 配置変更不要（例：別のファイルタイプからの通知）
		}

		// 移動する
		m_eDockSide = eDockSideNew;	// 自身のドッキング配置の記憶を更新
		::SetWindowPos( GetHwnd(), NULL,
			rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE | ((eDockSideOld == eDockSideNew)? 0: SWP_FRAMECHANGED) );	// SWP_FRAMECHANGED 指定で WM_NCCALCSIZE（非クライアント領域の再計算）に誘導する
		pcEditView->m_pcEditWnd->EndLayoutBars( m_bEditWndReady );
	}
	return false;
}

/** アウトライン通知(MYWM_OUTLINE_NOTIFY)処理

	wParam: 通知種別
	lParam: 種別毎のパラメータ

	@date 2010.06.07 ryoji 新規作成
*/
void CDlgFuncList::OnOutlineNotify( WPARAM wParam, LPARAM lParam )
{
	CEditDoc* pDoc = CEditDoc::GetInstance(0);	// 今は非表示かもしれないので (CEditView*)m_lParam は使えない
	switch( wParam ){
	case 0:	// 設定変更通知（ドッキングモード or サイズ）, lParam: 通知元の HWND
		if( (HWND)lParam == pDoc->m_pcEditWnd->GetHwnd() )
			return;	// 自分からの通知は無視
		ChangeLayout( OUTLINE_LAYOUT_BACKGROUND );	// アウトライン画面を再配置
		break;
	}
	return;
}

/** 他ウィンドウにアウトライン通知をポストする
	@date 2010.06.10 ryoji 新規作成
*/
BOOL CDlgFuncList::PostOutlineNotifyToAllEditors( WPARAM wParam, LPARAM lParam )
{
	return CAppNodeGroupHandle(0).PostMessageToAllEditors( MYWM_OUTLINE_NOTIFY, (WPARAM)wParam, (LPARAM)lParam, GetHwnd() );
}

/** コンテキストメニュー処理
	@date 2010.06.07 ryoji 新規作成
*/
BOOL CDlgFuncList::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
	// キャプションかリスト／ツリー上ならメニューを表示する
	HWND hwndFrom = (HWND)wParam;
	if( ::SendMessage( GetHwnd(), WM_NCHITTEST, 0, lParam ) == HTCAPTION
			|| hwndFrom == ::GetDlgItem( GetHwnd(), IDC_LIST_FL )
			|| hwndFrom == ::GetDlgItem( GetHwnd(), IDC_TREE_FL )
	){
		POINT pt;
		pt.x = MAKEPOINTS(lParam).x;
		pt.y = MAKEPOINTS(lParam).y;
		if( pt.x == -1 && pt.y == -1 ){	// キーボード（メニューキー や Shift F10）からの呼び出し
			RECT rc;
			::GetWindowRect( hwndFrom, &rc );
			pt.x = rc.left;
			pt.y = rc.top;
		}
		DoMenu( pt, hwndFrom );
		return TRUE;
	}

	return CDialog::OnContextMenu( wParam, lParam );	// その他のコントロール上ではポップアップヘルプを表示する
}

/** タイトルバーのドラッグ＆ドロップでドッキング配置する際の移動先矩形を求める
	@date 2010.06.17 ryoji 新規作成
*/
EDockSide CDlgFuncList::GetDropRect( POINT ptDrag, POINT ptDrop, LPRECT pRect, bool bForceFloat )
{
	struct CDockStretch{
		static int GetIdealStretch( int nStretch, int nMaxStretch )
		{
			if( nStretch == 0 )
				nStretch = nMaxStretch / 3;
			if( nStretch > nMaxStretch - DOCK_MIN_SIZE ) nStretch = nMaxStretch - DOCK_MIN_SIZE;
			if( nStretch < DOCK_MIN_SIZE ) nStretch = DOCK_MIN_SIZE;
			return nStretch;
		}
	};

	// 移動しない矩形を取得する
	RECT rcWnd;
	::GetWindowRect( GetHwnd(), &rcWnd );
	if( IsDocking() && !bForceFloat ){
		if( ::PtInRect( &rcWnd, ptDrop ) ){
			*pRect = rcWnd;
			return GetDockSide();	// 移動しない位置だった
		}
	}

	// ドッキング用の矩形を取得する
	EDockSide eDockSide = DOCKSIDE_FLOAT;	// フローティングに仮決め
	RECT rcDock;
	GetDockSpaceRect( &rcDock );
	if( !bForceFloat && ::PtInRect( &rcDock, ptDrop ) ){
		int cxLeft		= CDockStretch::GetIdealStretch( ProfDockLeft(), rcDock.right - rcDock.left );
		int cyTop		= CDockStretch::GetIdealStretch( ProfDockTop(), rcDock.bottom - rcDock.top );
		int cxRight		= CDockStretch::GetIdealStretch( ProfDockRight(), rcDock.right - rcDock.left );
		int cyBottom	= CDockStretch::GetIdealStretch( ProfDockBottom(), rcDock.bottom - rcDock.top );

		int nDock = ::GetSystemMetrics( SM_CXCURSOR );
		if( ptDrop.x - rcDock.left < nDock ){
			eDockSide = DOCKSIDE_LEFT;
			rcDock.right = rcDock.left + cxLeft;
		}
		else if( rcDock.right - ptDrop.x < nDock ){
			eDockSide = DOCKSIDE_RIGHT;
			rcDock.left = rcDock.right - cxRight;
		}
		else if( ptDrop.y - rcDock.top < nDock ){
			eDockSide = DOCKSIDE_TOP;
			rcDock.bottom = rcDock.top + cyTop;
		}
		else if( rcDock.bottom - ptDrop.y < nDock ){
			eDockSide = DOCKSIDE_BOTTOM;
			rcDock.top = rcDock.bottom - cyBottom;
		}
		if( eDockSide != DOCKSIDE_FLOAT ){
			*pRect = rcDock;
			return eDockSide;	// ドッキング位置だった
		}
	}

	// フローティング用の矩形を取得する
	if( !IsDocking() ){	// フローティング → フローティング
		::OffsetRect( &rcWnd, ptDrop.x - ptDrag.x, ptDrop.y - ptDrag.y );
		*pRect = rcWnd;
	}else{	// ドッキング → フローティング
		int cx, cy;
		RECT rcFloat;
		rcFloat.left = 0;
		rcFloat.top = 0;
		if( m_pShareData->m_Common.m_sOutline.m_bRememberOutlineWindowPos
				&& m_pShareData->m_Common.m_sOutline.m_widthOutlineWindow	// 初期値だと 0 になっている
				&& m_pShareData->m_Common.m_sOutline.m_heightOutlineWindow	// 初期値だと 0 になっている
		){
			// 記憶しているサイズ
			rcFloat.right = m_pShareData->m_Common.m_sOutline.m_widthOutlineWindow;
			rcFloat.bottom = m_pShareData->m_Common.m_sOutline.m_heightOutlineWindow;
			cx = ::GetSystemMetrics( SM_CXMIN );
			cy = ::GetSystemMetrics( SM_CYMIN );
			if( rcFloat.right < cx ) rcFloat.right = cx;
			if( rcFloat.bottom < cy ) rcFloat.bottom = cy;
		}
		else{
			// デフォルトのサイズ（ダイアログテンプレートで決まるサイズ）
			rcFloat.right = m_pDlgTemplate->cx;
			rcFloat.bottom = m_pDlgTemplate->cy;
			::MapDialogRect( GetHwnd(), &rcFloat );
			rcFloat.right += ::GetSystemMetrics( SM_CXDLGFRAME ) * 2;	// ※ Create 時のスタイル変更でサイズ変更不可からサイズ変更可能にしている
			rcFloat.bottom += ::GetSystemMetrics( SM_CYCAPTION ) + ::GetSystemMetrics( SM_CYDLGFRAME ) * 2;
		}
		cy = ::GetSystemMetrics( SM_CYCAPTION );
		::OffsetRect( &rcFloat, ptDrop.x - cy * 2, ptDrop.y - cy / 2 );
		*pRect = rcFloat;
	}

	return DOCKSIDE_FLOAT;	// フローティング位置だった
}

/** タイトルバーのドラッグ＆ドロップでドッキング配置を変更する
	@date 2010.06.17 ryoji 新規作成
*/
BOOL CDlgFuncList::Track( POINT ptDrag )
{
	if( ::GetCapture() )
		return FALSE;

	struct SLockWindowUpdate
	{	// 画面にゴミが残らないように
		SLockWindowUpdate(){ ::LockWindowUpdate( ::GetDesktopWindow() ); }
		~SLockWindowUpdate(){ ::LockWindowUpdate( NULL ); }
	} sLockWindowUpdate;

	const SIZE sizeFull = {8, 8};	// フローティング配置用の枠線の太さ
	const SIZE sizeHalf = {4, 4};	// ドッキング配置用の枠線の太さ
	const SIZE sizeClear = {0, 0};	// 枠線描画しない

	POINT pt;
	RECT rc;
	RECT rcDragLast;
	SIZE sizeLast = sizeClear;
	BOOL bDragging = false;	// まだ本格開始しない
	int cxDragSm = ::GetSystemMetrics( SM_CXDRAG );
	int cyDragSm = ::GetSystemMetrics( SM_CYDRAG );

	::SetCapture( GetHwnd() );	// キャプチャ開始

	while( ::GetCapture() == GetHwnd() )
	{
		MSG msg;
		if (!::GetMessage(&msg, NULL, 0, 0)){
			::PostQuitMessage( (int)msg.wParam );
			break;
		}

		switch (msg.message){
		case WM_MOUSEMOVE:
			::GetCursorPos( &pt );

			bool bStart;
			bStart = false;
			if( !bDragging ){
				// 押した位置からいくらか動いてからドラッグ開始にする
				if( abs(pt.x - ptDrag.x) >= cxDragSm || abs(pt.y - ptDrag.y) >= cyDragSm ){
					bDragging = bStart = true;	// ここから開始
				}
			}
			if( bDragging ){	// ドラッグ中
				// ドロップ先矩形を描画する
				EDockSide eDockSide = GetDropRect( ptDrag, pt, &rc, GetKeyState_Control() );
				SIZE sizeNew = (eDockSide <= DOCKSIDE_FLOAT)? sizeFull: sizeHalf;
				CGraphics::DrawDropRect( &rc, sizeNew, bStart? NULL: &rcDragLast, sizeLast );
				rcDragLast = rc;
				sizeLast = sizeNew;
			}
			break;

		case WM_LBUTTONUP:
			::GetCursorPos( &pt );

			::ReleaseCapture();
			if( bDragging ){
				// ドッキング配置を変更する
				EDockSide eDockSide = GetDropRect( ptDrag, pt, &rc, GetKeyState_Control() );
				CGraphics::DrawDropRect( NULL, sizeClear, &rcDragLast, sizeLast );

				ProfDockDisp() = GetHwnd()? TRUE: FALSE;
				ProfDockSide() = eDockSide;	// 新しいドッキングモードを適用
				switch( eDockSide ){
				case DOCKSIDE_LEFT:		ProfDockLeft() = rc.right - rc.left;	break;
				case DOCKSIDE_TOP:		ProfDockTop() = rc.bottom - rc.top;		break;
				case DOCKSIDE_RIGHT:	ProfDockRight() = rc.right - rc.left;	break;
				case DOCKSIDE_BOTTOM:	ProfDockBottom() = rc.bottom - rc.top;	break;
				}
				ChangeLayout( OUTLINE_LAYOUT_FOREGROUND );	// 自分自身への強制変更
				if( !IsDocking() ){
					::MoveWindow( GetHwnd(), rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );
				}
				if( ProfDockSync() ){
					PostOutlineNotifyToAllEditors( (WPARAM)0, (LPARAM)((CEditView*)m_lParam)->m_pcEditWnd->GetHwnd() );	// 他ウィンドウにドッキング配置変更を通知する
				}
				return TRUE;
			}
			return FALSE;

		case WM_KEYUP:
			if( bDragging ){
				if( msg.wParam == VK_CONTROL ){
					// フローティングを強制するモードを抜ける
					::GetCursorPos( &pt );
					EDockSide eDockSide = GetDropRect( ptDrag, pt, &rc, false );
					SIZE sizeNew = (eDockSide <= DOCKSIDE_FLOAT)? sizeFull: sizeHalf;
					CGraphics::DrawDropRect( &rc, sizeNew, &rcDragLast, sizeLast );
					rcDragLast = rc;
					sizeLast = sizeNew;
				}
			}
			break;

		case WM_KEYDOWN:
			if( bDragging ){
				if( msg.wParam == VK_CONTROL ){
					// フローティングを強制するモードに入る
					::GetCursorPos( &pt );
					GetDropRect( ptDrag, pt, &rc, true );
					CGraphics::DrawDropRect( &rc, sizeFull, &rcDragLast, sizeLast );
					sizeLast = sizeFull;
					rcDragLast = rc;
				}
			}
			if( msg.wParam == VK_ESCAPE ){
				// キャンセル
				::ReleaseCapture();
				if( bDragging )
					CGraphics::DrawDropRect( NULL, sizeClear, &rcDragLast, sizeLast );
				return FALSE;
			}
			break;

		case WM_RBUTTONDOWN:
			// キャンセル
			::ReleaseCapture();
			if( bDragging )
				CGraphics::DrawDropRect( NULL, sizeClear, &rcDragLast, sizeLast );
			return FALSE;

		default:
			::DispatchMessage( &msg );
			break;
		}
	}

	::ReleaseCapture();
	return FALSE;
}
