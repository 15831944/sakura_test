//	$Id$
/*!	@file
	編集ウィンドウ（外枠）管理クラス

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

#include <stdio.h>
#include <windows.h>
#include <winuser.h>
//#include <stdio.h>
#include <io.h>
#include <mbctype.h>
#include <mbstring.h>

#include "CEditApp.h"
#include "CEditWnd.h"
#include "sakura_rc.h"
#include "CEditDoc.h"
#include "debug.h"
//@@#include "CProp1.h"
#include "CDlgAbout.h"
#include "mymessage.h"
#include "CShareData.h"
#include "CPrint.h"
#include "etc_uty.h"
#include "charcode.h"
#include "global.h"
#include "CDlgPrintSetting.h"
#include "CDlgPrintPage.h"
#include "funccode.h"		// Stonee, 2001/03/12


#define IDT_TOOLBAR		456
#define ID_TOOLBAR		100


#define MIN_PREVIEW_ZOOM 10
#define MAX_PREVIEW_ZOOM 400

#ifndef TBSTYLE_ALTDRAG
	#define TBSTYLE_ALTDRAG	0x0400
#endif
#ifndef TBSTYLE_FLAT
	#define TBSTYLE_FLAT	0x0800
#endif
#ifndef TBSTYLE_LIST
	#define TBSTYLE_LIST	0x1000
#endif

#ifndef	WM_MOUSEWHEEL
	#define WM_MOUSEWHEEL	0x020A
#endif

#define		YOHAKU_X		4		/* ウィンドウ内の枠と紙の隙間最小値 */
#define		YOHAKU_Y		4		/* ウィンドウ内の枠と紙の隙間最小値 */
#define		LINE_RANGE_X	48		/* 水平方向の１回のスクロール幅 */
#define		LINE_RANGE_Y	24		/* 垂直方向の１回のスクロール幅 */
#define		PAGE_RANGE_X	160		/* 水平方向の１回のページスクロール幅 */
#define		PAGE_RANGE_Y	160		/* 垂直方向の１回のページスクロール幅 */



//	/* メッセージループ */
//	DWORD MessageLoop_Thread( DWORD pCEditWndObject );


/* ダイアログプロシージャ */
BOOL CALLBACK PrintPreviewBar_DlgProc(
	HWND hwndDlg,	// handle to dialog box
	UINT uMsg,		// message
	WPARAM wParam,	// first message parameter
	LPARAM lParam 	// second message parameter
)
{
	CEditWnd* pCEditWnd;
	switch( uMsg ){
	case WM_INITDIALOG:
		pCEditWnd = ( CEditWnd* )lParam;
		if( NULL != pCEditWnd ){
			return pCEditWnd->DispatchEvent_PPB( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	default:
		pCEditWnd = ( CEditWnd* )::GetWindowLong( hwndDlg, DWL_USER );
		if( NULL != pCEditWnd ){
			return pCEditWnd->DispatchEvent_PPB( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	}
}




LRESULT CALLBACK CEditWndProc(
	HWND	hwnd,	// handle of window
	UINT	uMsg,	// message identifier
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
)
{
	CEditWnd* pSEWnd;
	pSEWnd = ( CEditWnd* )::GetWindowLong( hwnd, GWL_USERDATA );
	if( NULL != pSEWnd ){
		return pSEWnd->DispatchEvent( hwnd, uMsg, wParam, lParam );
	}
	return ::DefWindowProc( hwnd, uMsg, wParam, lParam );
}


//	BOOL CALLBACK CEditWnd_EnumChildProc(
//		HWND hwnd,		// handle to child window
//		LPARAM lParam 	// application-defined value
//	)
//	{
//		CEditWnd*	pCEditWnd;
//		pCEditWnd = (CEditWnd*)lParam;
//		char		szClassName[256];
//		if( pCEditWnd->m_hWnd == ::GetParent( hwnd ) ){
//			::GetClassName( hwnd, szClassName, sizeof(szClassName) - 1 );
//			if( 0 == strcmp( "#32770", szClassName ) ){
//				pCEditWnd->m_hwndChildArr[pCEditWnd->m_nChildArrNum] = hwnd;
//				pCEditWnd->m_nChildArrNum++;
//				return FALSE;
//			}
//		}
//		return TRUE;
//	}


/*
||  タイマーメッセージのコールバック関数
||
||	ツールバーの状態更新のためにタイマーを使用しています
*/
VOID CALLBACK CEditWndTimerProc(
	HWND hwnd,		// handle of window for timer messages
	UINT uMsg,		// WM_TIMER message
	UINT idEvent,	// timer identifier
	DWORD dwTime 	// current system time
)
{
	CEditWnd*	pCEdit;
	pCEdit = ( CEditWnd* )::GetWindowLong( hwnd, GWL_USERDATA );
	if( NULL != pCEdit ){
		pCEdit->OnTimer( hwnd, uMsg, idEvent, dwTime );
	}
	return;
}





CEditWnd::CEditWnd() :
	m_hWnd( NULL ),
	m_bDragMode( FALSE ),
	m_hwndParent( NULL ),
	m_hwndToolBar( NULL ),
	m_hwndStatusBar( NULL ),
	m_hwndProgressBar( NULL ),
	m_hdcCompatDC( NULL ),			/* 再描画用コンパチブルＤＣ */
	m_hbmpCompatBMP( NULL ),		/* 再描画用メモリＢＭＰ */
	m_hbmpCompatBMPOld( NULL ),		/* 再描画用メモリＢＭＰ(OLD) */

	m_nPreview_Zoom( 100 ),			/* 印刷プレビュー倍率 */
	m_nPreviewVScrollPos( 0 ),
	m_nPreviewHScrollPos( 0 ),
	m_nCurPageNum( 0 ),				/* 現在のページ */

	m_pPrintSetting( NULL ),		/* 現在の印刷設定 */

	m_hwndPrintPreviewBar( NULL ),	/* 印刷プレビュー 操作バー */
	m_hwndVScrollBar( NULL ),		/* 印刷プレビュー 垂直スクロールバーウィンドウハンドル */
	m_hwndHScrollBar( NULL ),		/* 印刷プレビュー 水平スクロールバーウィンドウハンドル */
	m_hwndSizeBox( NULL ),			/* 印刷プレビュー サイズボックスウィンドウハンドル */
	m_pszAppName( GSTR_EDITWINDOWNAME ),
	m_hbmpOPENED( NULL ),
	m_hbmpOPENED_THIS( NULL )
{

	/* 共有データ構造体のアドレスを返す */
	m_cShareData.Init();
	m_pShareData = m_cShareData.GetShareData( NULL, NULL );

//	MYTRACE( "CEditWnd::CEditWnd()おわり\n" );
	return;
}




CEditWnd::~CEditWnd()
{
	if( NULL != m_hbmpOPENED ){
		::DeleteObject( m_hbmpOPENED );
		m_hbmpOPENED = NULL;
	}
	if( NULL != m_hbmpOPENED_THIS ){
		::DeleteObject( m_hbmpOPENED_THIS );
		m_hbmpOPENED_THIS = NULL;
	}

	/* 再描画用メモリＢＭＰ */
	if( m_hbmpCompatBMP != NULL ){
		/* 再描画用メモリＢＭＰ(OLD) */
		::SelectObject( m_hdcCompatDC, m_hbmpCompatBMPOld );
		::DeleteObject( m_hbmpCompatBMP );
	}
	/* 再描画用コンパチブルＤＣ */
	if( m_hdcCompatDC != NULL ){
		::DeleteDC( m_hdcCompatDC );
	}

	if( NULL != m_hWnd ){
		m_hWnd = NULL;
	}
//	if( NULL != m_hThread ){
//		DWORD dwRes;
//		::SetThreadPriority( m_hThread, THREAD_PRIORITY_HIGHEST );
//		dwRes = ::WaitForSingleObject( m_hThread, INFINITE );
//		m_hThread = NULL;
//	}
	return;
}





/* 作成 */
HWND CEditWnd::Create(
	HINSTANCE	hInstance,
	HWND		hwndParent,
	const char*	pszPath,
	int			nCharCode,
	BOOL		bReadOnly
)
{
	WNDCLASS	wc;
	HWND		hWnd;
	ATOM		atom;
//	char		szMutexName[260];
	BOOL		bOpened;
	char szMsg[512];

	if( m_pShareData->m_nEditArrNum + 1 > MAX_EDITWINDOWS ){
		wsprintf( szMsg, "編集ウィンドウ数の上限は%dです。\nこれ以上は同時に開けません。", MAX_EDITWINDOWS );
		::MessageBox( NULL, szMsg, GSTR_APPNAME, MB_OK );
		return NULL;
	}


	m_hInstance = hInstance;
	m_hwndParent = hwndParent;
	m_hbmpOPENED = ::LoadBitmap( m_hInstance, MAKEINTRESOURCE( IDB_OPENED ) );
	m_hbmpOPENED_THIS = ::LoadBitmap( m_hInstance, MAKEINTRESOURCE( IDB_OPENED_THIS ) );
//	sprintf( szMutexName, "%sIsAlreadyExist", m_pszAppName );
	//	Apr. 27, 2000 genta
	//	サイズ変更時のちらつきを抑えるためCS_HREDRAW | CS_VREDRAW を外した
	wc.style			= CS_DBLCLKS | CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
	wc.lpfnWndProc		= CEditWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 32;
	wc.hInstance		= m_hInstance;
#ifdef _DEBUG
	wc.hIcon			= LoadIcon( m_hInstance, MAKEINTRESOURCE( IDI_ICON_DEBUG ) );
#else
	wc.hIcon			= LoadIcon( m_hInstance, MAKEINTRESOURCE( IDI_ICON_STD ) );
#endif
//	wc.hIcon			= NULL;

	wc.hCursor			= NULL/*LoadCursor( NULL, IDC_ARROW )*/;
	wc.hbrBackground	= (HBRUSH)NULL/*(COLOR_3DSHADOW + 1)*/;
	wc.lpszMenuName		= MAKEINTRESOURCE( IDR_MENU1 );
	wc.lpszClassName	= m_pszAppName;
	if( 0 == ( atom = RegisterClass( &wc ) ) ){
//		return NULL;
	}

	/* ウィンドウサイズ継承 */
	int	nWinCX, nWinCY;
	if( m_pShareData->m_Common.m_bSaveWindowSize ){
//		if( m_pShareData->m_Common.m_nWinSizeType != SIZE_MAXIMIZED ){
			nWinCX = m_pShareData->m_Common.m_nWinSizeCX;
			nWinCY = m_pShareData->m_Common.m_nWinSizeCY;
//		}else{
//			nWinCX = CW_USEDEFAULT;
//			nWinCY = 0;
//		}
	}else{
		nWinCX = CW_USEDEFAULT;
		nWinCY = 0;
	}

	hWnd = ::CreateWindowEx(
		0 	// extended window style
//		| WS_EX_CLIENTEDGE
		,
		m_pszAppName,		// pointer to registered class name
		m_pszAppName,		// pointer to window name
//		WS_VISIBLE |
		WS_OVERLAPPEDWINDOW |
		WS_CLIPCHILDREN	|
//		WS_HSCROLL | WS_VSCROLL	|
		0,	// window style

		CW_USEDEFAULT,		// horizontal position of window
		0,					// vertical position of window
		nWinCX,				// window width
		nWinCY,				// window height
		NULL,				// handle to parent or owner window
		NULL,				// handle to menu or child-window identifier
		m_hInstance,		// handle to application instance
		NULL				// pointer to window-creation data
	);
	m_hWnd = hWnd;

	m_cIcons.Create( m_hInstance, m_hWnd );	//	CreateImage List

	m_CMenuDrawer.Create( m_hInstance, m_hWnd, &m_cIcons );

	if( NULL != m_hWnd ){
		::SetWindowLong( m_hWnd, GWL_USERDATA, (LONG)this );

		/* 再描画用コンパチブルＤＣ */
		HDC hdc = ::GetDC( m_hWnd );
		m_hdcCompatDC = ::CreateCompatibleDC( hdc );
		::ReleaseDC( m_hWnd, hdc );

	}

//	BOOL	bDispToolBar;
//	bDispToolBar = TRUE;
	if( m_pShareData->m_Common.m_bDispTOOLBAR ){	/* 次回ウィンドウを開いたときツールバーを表示する */
 		/* ツールバー作成 */
		CreateToolBar();
	}

	/* ステータスバー */
	if( m_pShareData->m_Common.m_bDispSTATUSBAR ){	/* 次回ウィンドウを開いたときステータスバーを表示する */
		/* ステータスバー作成 */
		CreateStatusBar();
	}

	/* ファンクションキー バー */
	if( m_pShareData->m_Common.m_bDispFUNCKEYWND ){	/* 次回ウィンドウを開いたときファンクションキーを表示する */
		BOOL bSizeBox;
		if( m_pShareData->m_Common.m_nFUNCKEYWND_Place == 0 ){	/* ファンクションキー表示位置／0:上 1:下 */
			bSizeBox = FALSE;
		}else{
			bSizeBox = TRUE;
			/* ステータスパーを表示している場合はサイズボックスを表示しない */
			if( NULL != m_hwndStatusBar ){
				bSizeBox = FALSE;
			}
		}
		m_CFuncKeyWnd.Open( hInstance, m_hWnd, &m_cEditDoc, bSizeBox );
	}


	if( FALSE == m_cEditDoc.Create( m_hInstance, m_hWnd, &m_cIcons/*, 1, 1, 0, 0*/ ) ){
		::MessageBox(
			m_hWnd,
			"クライアントウィンドウの作成に失敗しました", GSTR_APPNAME,
			MB_OK
		);
	}
	/* デスクトップからはみ出さないようにする */
	RECT	rcOrg;
	RECT	rcDesktop;
//	int		nWork;
	::SystemParametersInfo( SPI_GETWORKAREA, NULL, &rcDesktop, 0 );
	::GetWindowRect( m_hWnd, &rcOrg );
	/* ウィンドウ位置調整 */
	if( rcOrg.bottom >= rcDesktop.bottom ){
		if( 0 > rcOrg.top - (rcOrg.bottom - rcDesktop.bottom ) ){
			rcOrg.top = 0;
		}else{
			rcOrg.top -= rcOrg.bottom - rcDesktop.bottom;
		}
		rcOrg.bottom = rcDesktop.bottom - 1;
	}
	if( rcOrg.right >= rcDesktop.right ){
		if( 0 > rcOrg.left - (rcOrg.right - rcDesktop.right ) ){
			rcOrg.left = 0;
		}else{
			rcOrg.left -= rcOrg.right - rcDesktop.right;
		}
		rcOrg.right = rcDesktop.right - 1;
	}
	/* ウィンドウサイズ調整 */
	if( rcOrg.top < rcDesktop.top ){
		rcOrg.top = rcDesktop.top;
	}
	if( rcOrg.left < rcDesktop.left ){
		rcOrg.left = rcDesktop.left;
	}
	if( rcOrg.bottom >= rcDesktop.bottom ){
		rcOrg.bottom = rcDesktop.bottom - 1;
	}
	if( rcOrg.right >= rcDesktop.right ){
		rcOrg.right = rcDesktop.right - 1;
	}
	::SetWindowPos(
		m_hWnd, 0,
		rcOrg.left, rcOrg.top,
		rcOrg.right - rcOrg.left, rcOrg.bottom - rcOrg.top,
		SWP_NOOWNERZORDER | SWP_NOZORDER
	);

	/* ウィンドウサイズ継承 */
	if( m_pShareData->m_Common.m_bSaveWindowSize &&
		m_pShareData->m_Common.m_nWinSizeType == SIZE_MAXIMIZED ){
		::ShowWindow( m_hWnd, SW_SHOWMAXIMIZED );
	}else{
		::ShowWindow( m_hWnd, SW_SHOW );
	}
	if( NULL != m_hWnd ){
		/* ドロップされたファイルを受け入れる */
		::DragAcceptFiles( m_hWnd, TRUE );
		/* 編集ウィンドウリストへの登録 */
		if( FALSE == m_cShareData.AddEditWndList( m_hWnd ) ){
			wsprintf( szMsg, "編集ウィンドウ数の上限は%dです。\nこれ以上は同時に開けません。", MAX_EDITWINDOWS );
			::MessageBox( m_hWnd, szMsg, GSTR_APPNAME, MB_OK );
			::DestroyWindow( m_hWnd );
			m_hWnd = hWnd = NULL;
			return hWnd;
		}
		/* タイマーを起動 */
		if( 0 == ::SetTimer( m_hWnd, IDT_TOOLBAR, 300, (TIMERPROC)CEditWndTimerProc ) ){
			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
				"CEditWnd::Create()\nタイマーが起動できません。\nシステムリソースが不足しているのかもしれません。"
			);
		}
	}
	::InvalidateRect( m_hWnd, NULL, TRUE );
	if( NULL != pszPath ){
		char*	pszPathNew = new char[_MAX_PATH];
		strcpy( pszPathNew, pszPath );
		::ShowWindow( m_hWnd, SW_SHOW );
		if( !m_cEditDoc.FileRead( pszPathNew, &bOpened, nCharCode, bReadOnly, TRUE ) ){
			/* ファイルが既に開かれている */
			if( bOpened ){
				::PostMessage( m_hWnd, WM_CLOSE, 0, 0 );
				delete [] pszPathNew;
				return NULL;
			}
			else {
				//	Nov. 20, 2000 genta
				m_cEditDoc.SetImeMode( m_pShareData->m_Types[0].m_nImeState );
			}
		}
		delete [] pszPathNew;
	}
	else {
		//	Nov. 20, 2000 genta
		m_cEditDoc.SetImeMode( m_pShareData->m_Types[0].m_nImeState );
	}
	return m_hWnd;
}


//	キーワード：ステータスバー順序
/* ステータスバー作成 */
void CEditWnd::CreateStatusBar( void )
{
//	int		nStArr[] = { 300, 400, 500. -1 };
//	int		nStArrNum = sizeof( nStArr ) / sizeof( nStArr[0] );

	/* ステータスバー */
	m_hwndStatusBar = ::CreateStatusWindow(
		WS_CHILD | WS_VISIBLE | WS_EX_RIGHT | SBARS_SIZEGRIP,
		"",
		m_hWnd,
		IDW_STATUSBAR
	);

	/* プログレスバー */
	m_hwndProgressBar = ::CreateWindowEx(
		WS_EX_TOOLWINDOW,
		PROGRESS_CLASS,
		(LPSTR) NULL,
		WS_CHILD /*|  WS_VISIBLE*/,
		3,
		5,
		150,
		13,
		m_hwndStatusBar,
		NULL,
		m_hInstance,
		0
	);
//	::ShowWindow( m_hwndProgressBar, SW_SHOW );

	if( NULL != m_CFuncKeyWnd.m_hWnd ){
		m_CFuncKeyWnd.SizeBox_ONOFF( FALSE );
	}
	//スプリッターの、サイズボックスの位置を変更
	m_cEditDoc.m_cSplitterWnd.DoSplit( -1, -1);
	return;
}


/* ステータスバー破棄 */
void CEditWnd::DestroyStatusBar( void )
{
	if( NULL != m_hwndProgressBar ){
		::DestroyWindow( m_hwndProgressBar );
		m_hwndProgressBar = NULL;
	}
	::DestroyWindow( m_hwndStatusBar );
	m_hwndStatusBar = NULL;

	if( NULL != m_CFuncKeyWnd.m_hWnd ){
		BOOL bSizeBox;
		if( m_pShareData->m_Common.m_nFUNCKEYWND_Place == 0 ){	/* ファンクションキー表示位置／0:上 1:下 */
			/* サイズボックスの表示／非表示切り替え */
			bSizeBox = FALSE;
		}else{
			bSizeBox = TRUE;
			/* ステータスパーを表示している場合はサイズボックスを表示しない */
			if( NULL != m_hwndStatusBar ){
				bSizeBox = FALSE;
			}
		}
		m_CFuncKeyWnd.SizeBox_ONOFF( bSizeBox );
	}
	//スプリッターの、サイズボックスの位置を変更
	m_cEditDoc.m_cSplitterWnd.DoSplit( -1, -1 );

	return;
}

/* ツールバー作成 */
void CEditWnd::CreateToolBar( void )
{
	int				nFlag;
//	TBADDBITMAP		tb;
	TBBUTTON		tbb;
	int				i;
	int				nIdx;
	UINT			uToolType;
	nFlag = 0;
//	if( m_pShareData->m_Common.m_bToolBarIsFlat ){	/* フラットツールバーにする／しない */
//		nFlag |= TBSTYLE_FLAT;
//		/* ボタンの位置を調整 */
//	}
	/* ツールバーウィンドウの作成 */
	m_hwndToolBar = ::CreateWindowEx(
		0,
		TOOLBARCLASSNAME,
		NULL,
		WS_CHILD | WS_VISIBLE | /*WS_BORDER | */
/*		WS_EX_WINDOWEDGE| */
		TBSTYLE_TOOLTIPS |
//		TBSTYLE_WRAPABLE |
//		TBSTYLE_ALTDRAG |
//		CCS_ADJUSTABLE |
		nFlag,
		0, 0,
		0, 0,
		m_hWnd,
		(HMENU)ID_TOOLBAR,
		m_hInstance,
		NULL
	);
	if( NULL == m_hwndToolBar ){
		if( m_pShareData->m_Common.m_bToolBarIsFlat ){	/* フラットツールバーにする／しない */
			m_pShareData->m_Common.m_bToolBarIsFlat = FALSE;
		}
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST, GSTR_APPNAME,
			"ツールバーの作成に失敗しました。"
		);
	}else{
		::SendMessage( m_hwndToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0 );
		//	Oct. 12, 2000 genta
		//	既に用意されているImage Listをアイコンとして登録
		m_cIcons.SetToolBarImages( m_hwndToolBar );
		/* ツールバーにボタンを追加 */
		for( i = 0; i < m_pShareData->m_Common.m_nToolBarButtonNum; ++i ){
			nIdx = m_pShareData->m_Common.m_nToolBarButtonIdxArr[i];
			tbb = m_cShareData.m_tbMyButton[m_pShareData->m_Common.m_nToolBarButtonIdxArr[i]];
			::SendMessage( m_hwndToolBar, TB_ADDBUTTONS, (WPARAM)1, (LPARAM)&tbb );
		}
		if( m_pShareData->m_Common.m_bToolBarIsFlat ){	/* フラットツールバーにする／しない */
			uToolType = (UINT)::GetWindowLong(m_hwndToolBar, GWL_STYLE);
			uToolType |= (TBSTYLE_FLAT);
			::SetWindowLong(m_hwndToolBar, GWL_STYLE, (LONG)uToolType);
			::InvalidateRect(m_hwndToolBar, NULL, TRUE);
		}
	}
	return;
}

/* 印刷プレビュー 操作バー 作成 */
void CEditWnd::CreatePrintPreviewBar( void )
{
	RECT		rc1;
//	RECT		rc2;
	SCROLLINFO	si;
	int			nCxHScroll;
	int			nCyHScroll;
	int			nCxVScroll;
	int			nCyVScroll;
	nCxHScroll = ::GetSystemMetrics( SM_CXHSCROLL );
	nCyHScroll = ::GetSystemMetrics( SM_CYHSCROLL );
	nCxVScroll = ::GetSystemMetrics( SM_CXVSCROLL );
	nCyVScroll = ::GetSystemMetrics( SM_CYVSCROLL );

	/* 印刷プレビュー 操作バー */
	m_hwndPrintPreviewBar = ::CreateDialogParam(
		m_hInstance,							// handle to application instance
		MAKEINTRESOURCE( IDD_PRINTPREVIEWBAR ),	// identifies dialog box template name
		m_hWnd,									// handle to owner window
		(DLGPROC)PrintPreviewBar_DlgProc, 		// pointer to dialog box procedure
		(LPARAM)this
	);
//	::GetWindowRect( m_hWnd, &rc1 );
//	::GetWindowRect( m_hwndPrintPreviewBar, &rc2 );
//	::MoveWindow( m_hwndPrintPreviewBar, 0, 0, rc1.right - rc1.left, rc2.bottom - rc2.top, TRUE );


	/* スクロールバーの作成 */
	m_hwndVScrollBar = ::CreateWindowEx(
		0L,									/* no extended styles			*/
		"SCROLLBAR",						/* scroll bar control class		*/
		(LPSTR) NULL,						/* text for window title bar	*/
		WS_VISIBLE | WS_CHILD | SBS_VERT,	/* scroll bar styles			*/
		0,									/* horizontal position			*/
		0,									/* vertical position			*/
		200,								/* width of the scroll bar		*/
		CW_USEDEFAULT,						/* default height				*/
		m_hWnd,								/* handle of main window		*/
		(HMENU) NULL,						/* no menu for a scroll bar		*/
		m_hInstance,						/* instance owning this window	*/
		(LPVOID) NULL						/* pointer not needed			*/
	);
	si.cbSize = sizeof( si );
	si.fMask = SIF_ALL;
	si.nMin	 = 0;
	si.nMax	 = 29;
	si.nPage = 10;
	si.nPos	 = 0;
	si.nTrackPos = 1;
	::SetScrollInfo( m_hwndVScrollBar, SB_CTL, &si, TRUE );
	::ShowScrollBar( m_hwndVScrollBar, SB_CTL, TRUE );

	/* スクロールバーの作成 */
	m_hwndHScrollBar = NULL;

	m_hwndHScrollBar = ::CreateWindowEx(
		0L,									/* no extended styles			*/
		"SCROLLBAR",						/* scroll bar control class		*/
		(LPSTR) NULL,						/* text for window title bar	*/
		WS_VISIBLE | WS_CHILD | SBS_HORZ,	/* scroll bar styles			*/
		0,									/* horizontal position			*/
		0,									/* vertical position			*/
		200,								/* width of the scroll bar		*/
		CW_USEDEFAULT,						/* default height				*/
		m_hWnd,								/* handle of main window		*/
		(HMENU) NULL,						/* no menu for a scroll bar		*/
		m_hInstance,						/* instance owning this window	*/
		(LPVOID) NULL						/* pointer not needed			*/
	);
	si.cbSize = sizeof( si );
	si.fMask = SIF_ALL;
	si.nMin	 = 0;
	si.nMax	 = 29;
	si.nPage = 10;
	si.nPos	 = 0;
	si.nTrackPos = 1;
	::SetScrollInfo( m_hwndHScrollBar, SB_CTL, &si, TRUE );
	::ShowScrollBar( m_hwndHScrollBar, SB_CTL, TRUE );

	/* サイズボックス */
	m_hwndSizeBox = ::CreateWindowEx(
		WS_EX_CONTROLPARENT/*0L*/, 							/* no extended styles			*/
		"SCROLLBAR",										/* scroll bar control class		*/
		(LPSTR) NULL,										/* text for window title bar	*/
		WS_VISIBLE | WS_CHILD | SBS_SIZEBOX | SBS_SIZEGRIP, /* scroll bar styles			*/
		0,													/* horizontal position			*/
		0,													/* vertical position			*/
		200,												/* width of the scroll bar		*/
		CW_USEDEFAULT,										/* default height				*/
		m_hWnd, 											/* handle of main window		*/
		(HMENU) NULL,										/* no menu for a scroll bar 	*/
		m_hInstance,										/* instance owning this window	*/
		(LPVOID) NULL										/* pointer not needed			*/
	);
	::ShowWindow( m_hwndPrintPreviewBar, SW_SHOW );


	/* WM_SIZE 処理 */
	::GetClientRect( m_hWnd, &rc1 );
	OnSize( SIZE_RESTORED, MAKELONG( rc1.right - rc1.left, rc1.bottom - rc1.top ) );
	return;

}

//マルチスレッド版
//
//	/* メッセージループ */
//	HANDLE CEditWnd::MessageLoop( void )
//	{
//		return NULL;
//		DWORD	IDThread;
//		HANDLE	hThrd;
//
//	//	::MYMESSAGEBOX( NULL, MB_OK, GSTR_APPNAME, "CEditWnd::MessageLoop() START m_hWnd=%xh", m_hWnd );
//		hThrd = ::CreateThread(NULL,	// no security attributes
//			0,							// use default stack size
//			(LPTHREAD_START_ROUTINE) MessageLoop_Thread,
//			(LPVOID)this,				// param to thread func
//			CREATE_SUSPENDED,			// creation flag
//			&IDThread					// thread identifier
//		);
//		if (hThrd == NULL){
//			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST, GSTR_APPNAME,
//				"CreateThread Failed!"
//			);
//			return NULL;
//		}
//	//	AddThreadToList( hThrd );
//	//	dwCount++;
//
//		// Set the priority lower than the primary (input)
//		// thread, so that the process is responsive to user
//		// input.  Then resume the thread.
//		if ( !SetThreadPriority(hThrd, /*THREAD_PRIORITY_LOWEST*/THREAD_PRIORITY_BELOW_NORMAL) ){
//			::MYMESSAGEBOX(m_hWnd, MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST, GSTR_APPNAME,
//				"SetThreadPriority failed!"
//			);
//			return NULL;
//		}
//
//		if ( (ResumeThread( hThrd )) == -1 ){
//			::MYMESSAGEBOX( m_hWnd,	MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST, GSTR_APPNAME,
//				"ResumeThread failed!"
//			);
//			return NULL;
//		}
//	//	while( NULL != m_hWnd ){
//	//	}
//
//
//		return hThrd;
//	}
//	/* メッセージループ */
//	DWORD MessageLoop_Thread( DWORD pCEditWndObject )
//	{
//		CEditWnd*	pCEditWnd = (CEditWnd*)pCEditWndObject;
//	//	MSG			msg;
//	//	MSG			msg2;
//	//	::MYMESSAGEBOX( NULL, MB_OK, GSTR_APPNAME, "MessageLoop_Thread() START pCEditWnd->m_hWnd=%xh", pCEditWnd->m_hWnd );
//		HWND		hWnd;
//		hWnd = pCEditWnd->m_hWnd;
//		while ( 1 ){
//			if( NULL == pCEditWnd->m_hWnd ){
//	//			::MYMESSAGEBOX( NULL, MB_OK, GSTR_APPNAME, "NULL == pCEditWnd->m_hWnd" );
//	//			::MessageBeep( MB_ICONHAND );
//				break;
//
//			}
//			if( FALSE == ::IsWindow( pCEditWnd->m_hWnd ) ){
//	//			::MessageBeep( MB_ICONHAND );
//	//			::MessageBeep( MB_ICONHAND );
//	//			::MYMESSAGEBOX( NULL, MB_OK, GSTR_APPNAME, "FALSE == ::IsWindow( %xh ) -1", hWnd );
//				break;
//			}
//			if( FALSE == ::IsWindow( hWnd ) ){
//	//			::MessageBeep( MB_ICONHAND );
//	//			::MessageBeep( MB_ICONHAND );
//	//			::MessageBeep( MB_ICONHAND );
//	//			::MYMESSAGEBOX( NULL, MB_OK, GSTR_APPNAME, "FALSE == ::IsWindow( %xh ) -2", hWnd );
//				break;
//			}
//		}
//	//	::MYMESSAGEBOX( NULL, MB_OK, GSTR_APPNAME, "MessageLoop_Thread() END" );
//		return 0;
//
//	//x	while ( NULL != pCEditWnd->m_hWnd && GetMessage(&msg, NULL, 0, 0 ) ){
//	//x		if( NULL != pCEditWnd->m_hwndPrintPreviewBar && ::IsDialogMessage( pCEditWnd->m_hwndPrintPreviewBar, &msg ) ){	/* 印刷プレビュー 操作バー */
//	//x		}else{
//	//x			if( NULL != pCEditWnd->m_pShareData->m_hAccel ){
//	//x				msg2 = msg;
//	//x				if(TranslateAccelerator( msg.hwnd, pCEditWnd->m_pShareData->m_hAccel, &msg ) ){
//	//x				}else{
//	//x					TranslateMessage( &msg );
//	//x					DispatchMessage( &msg );
//	//x				}
//	//x			}else{
//	//x				TranslateMessage( &msg );
//	//x				DispatchMessage( &msg );
//	//x			}
//	//x		}
//	//x	}
//	//x	return 0;
//	}



//複数プロセス版
/* メッセージループ */
void CEditWnd::MessageLoop( void )
{
	MSG	msg;
//	MSG	msg2;
	while ( NULL != m_hWnd && GetMessage( &msg, NULL, 0, 0 ) ){
		if( NULL != m_hwndPrintPreviewBar && ::IsDialogMessage( m_hwndPrintPreviewBar, &msg ) ){	/* 印刷プレビュー 操作バー */
		}else
		if( NULL != m_cEditDoc.m_cDlgFind.m_hWnd && ::IsDialogMessage( m_cEditDoc.m_cDlgFind.m_hWnd, &msg ) ){	/* 「検索」ダイアログ */
		}else
		if( NULL != m_cEditDoc.m_cDlgFuncList.m_hWnd && ::IsDialogMessage( m_cEditDoc.m_cDlgFuncList.m_hWnd, &msg ) ){	/* 「アウトライン」ダイアログ */
		}else
		if( NULL != m_cEditDoc.m_cDlgReplace.m_hWnd && ::IsDialogMessage( m_cEditDoc.m_cDlgReplace.m_hWnd, &msg ) ){	/* 「置換」ダイアログ */
		}else
		if( NULL != m_cEditDoc.m_cDlgGrep.m_hWnd && ::IsDialogMessage( m_cEditDoc.m_cDlgGrep.m_hWnd, &msg ) ){	/* 「Grep」ダイアログ */
		}else
		if( NULL != m_cEditDoc.m_cHokanMgr.m_hWnd && ::IsDialogMessage( m_cEditDoc.m_cHokanMgr.m_hWnd, &msg ) ){	/* 「Grep」ダイアログ */
		}else
		if( NULL != m_cEditDoc.m_cHokanMgr.m_hWnd && ::IsDialogMessage( m_cEditDoc.m_cHokanMgr.m_hWnd, &msg ) ){	/* 「Grep」ダイアログ */
		}else
		if( NULL != m_cEditDoc.m_cHokanMgr.m_hWnd && ::IsDialogMessage( m_cEditDoc.m_cHokanMgr.m_hWnd, &msg ) ){	/* 「Grep」ダイアログ */
		}else
		if( NULL != m_cEditDoc.m_cHokanMgr.m_hWnd && ::IsDialogMessage( m_cEditDoc.m_cHokanMgr.m_hWnd, &msg ) ){	/* 「Grep」ダイアログ */
		}else
		{
			if( NULL != m_pShareData->m_hAccel ){
//				msg2 = msg;
				if( TranslateAccelerator( msg.hwnd, m_pShareData->m_hAccel, &msg ) ){
				}else{
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
			}else{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
	}
	return;
}


LRESULT CEditWnd::DispatchEvent(
	HWND	hwnd,	// handle of window
	UINT	uMsg,	// message identifier
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
)
{
	int					nRet;
//	RECT				rc;
//	RECT				rcClient;
//	int					nToolBarHeight;
//	int					nStatusBarHeight;
//	int					nFuncKeyWndHeight;
	int					idCtrl;
	LPNMHDR				pnmh;
	LPTOOLTIPTEXT		lptip;
	int					nPane;
	FileInfo*			pfi;
//	int					nJumpToLine;
	WORD				fActive;
	BOOL				fMinimized;
	HWND				hwndTarget;
	HWND				hwndActive;
	BOOL				bIsActive;
	int					nCaretPosX;
	int					nCaretPosY;
	POINT*				ppoCaret;
	LPHELPINFO			lphi;
	const char*			pLine;
	int					nLineLen;
//	WINDOWPOS*			lpwp;


	UINT				idCtl;	/* コントロールのID */
	MEASUREITEMSTRUCT*	lpmis;
	char				szLabel[1024];
//	LPMEASUREITEMSTRUCT	lpmis;	/* 項目サイズ情報 */
//	char*				pszwork;
	LPDRAWITEMSTRUCT	lpdis;	/* 項目描画情報 */
	int					nItemWidth;
	int					nItemHeight;
//	WORD				fwKeys;
//	short				zDelta;
//	short				xPos;
//	short				yPos;
//	int					i;
//	int					j;
//	int					nScrollCode;
//	int					nAdd;
//	int					nPos;
//	SCROLLINFO			si;
	UINT				uItem;
	UINT				fuFlags;
	HMENU				hmenu;
	const char*			pszItemStr;
//	UINT		idCtl;
//	DRAWITEMSTRUCT*	lpdis;
//	TCHAR				chUser;
//	UINT				fuFlag;
//	HMENU				hMenu;
//	MENUITEMINFO		mii;
	LRESULT				lRes;

	switch( uMsg ){
	case WM_PAINTICON:
//		MYTRACE( "WM_PAINTICON\n" );
		return 0;
	case WM_ICONERASEBKGND:
//		MYTRACE( "WM_ICONERASEBKGND\n" );
		return 0;
	case WM_LBUTTONDOWN:
		return OnLButtonDown( wParam, lParam );
	case WM_MOUSEMOVE:
		return OnMouseMove( wParam, lParam );
	case WM_LBUTTONUP:
		return OnLButtonUp( wParam, lParam );
	case WM_MOUSEWHEEL:
		return OnMouseWheel( wParam, lParam );
	case WM_HSCROLL:
		return OnHScroll( wParam, lParam );
	case WM_VSCROLL:
		return OnVScroll( wParam, lParam );


	case WM_MENUCHAR:
		/* メニューアクセスキー押下時の処理(WM_MENUCHAR処理) */
		return m_CMenuDrawer.OnMenuChar( hwnd, uMsg, wParam, lParam );






	case WM_MENUSELECT:
		if( NULL == m_hwndStatusBar ){
			return 1;
		}
		uItem = (UINT) LOWORD(wParam);		// menu item or submenu index
		fuFlags = (UINT) HIWORD(wParam);	// menu flags
		hmenu = (HMENU) lParam;				// handle to menu clicked
		{
			/* メニュー機能のテキストをセット */
//			char		szWork[256];
//			int			j;
//			char*		pszKey;
//			int			nKeyLen;
			CMemory		cmemWork;
			pszItemStr = "";
//			pszItemStr = (const char *)szLabel;
//			strcpy( szLabel, "" );
//			if( 0 != uItem && 0 < ::LoadString( m_hInstance, uItem, szWork, sizeof( szWork ) - 1 ) ){
//				strcat( szLabel, szWork );
//			}else
//				for( i = 0; i < m_CMenuDrawer.m_nMenuItemNum; ++i ){
//					if( (int)uItem == m_CMenuDrawer.m_nMenuItemFuncArr[i] ){
//						break;
//					}
//				}
//				if( i >= m_CMenuDrawer.m_nMenuItemNum ){
//					pszItemStr = (const char *)"";
//				}else{
//					cmemWork = m_CMenuDrawer.m_cmemMenuItemStrArr[i];
//					cmemWork.Replace( "&", "" );
//					pszItemStr = cmemWork.GetPtr( NULL );

					/* 機能に対応するキー名の取得(複数) */
					CMemory**	ppcAssignedKeyList;
					int			nAssignedKeyNum;
					int			j;
//					char*		pszKey;
//					int			nKeyLen;
					nAssignedKeyNum = CKeyBind::GetKeyStrList(
						m_hInstance, m_pShareData->m_nKeyNameArrNum,
						(KEYDATA*)m_pShareData->m_pKeyNameArr, &ppcAssignedKeyList, uItem
					);
					if( 0 < nAssignedKeyNum ){
						for( j = 0; j < nAssignedKeyNum; ++j ){
							if( j > 0 ){
								cmemWork.AppendSz( " , " );
							}
//							strcat( szLabel, "\n        " );
							cmemWork.Append( ppcAssignedKeyList[j] );
//							pszKey = ppcAssignedKeyList[j]->GetPtr( &nKeyLen );
//							strcat( szLabel, pszKey );
							delete ppcAssignedKeyList[j];
						}
						delete [] ppcAssignedKeyList;
					}
					pszItemStr = cmemWork.GetPtr( NULL );

//				}
//			}

			::SendMessage( m_hwndStatusBar, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM) (LPINT)pszItemStr );




		}
		return 0;




//	/*	WM_MEASUREITEMオーナー描画コントロール
//	(ボタン、コンボボックス、リストボックス、メニュー項目)やメニューが作成された */
//	case WM_MEASUREITEM:
//		idCtl = (UINT) wParam;	/* コントロールのID */
//		lpmis = (LPMEASUREITEMSTRUCT) lParam;	/* 項目サイズ情報 */
//		pszwork = (char*)lpmis->itemData;
//		switch( lpmis->CtlType ){
//		case ODT_MENU:	/* オーナー描画メニュー */
//			if( IDM_EXITALL == lpmis->itemID ){
//				lpmis->itemWidth = 200;
//				lpmis->itemHeight = 64;
//				return TRUE;
//			}
//		}
//		return FALSE;
	case WM_DRAWITEM:
		idCtl = (UINT) wParam;				/* コントロールのID */
		lpdis = (DRAWITEMSTRUCT*) lParam;	/* 項目描画情報 */
		if( IDW_STATUSBAR == idCtl ){
			if( 4 == lpdis->itemID ){
				int	nColor;
				if( m_pShareData->m_bRecordingKeyMacro	/* キーボードマクロの記録中 */
				 && m_pShareData->m_hwndRecordingKeyMacro == m_hWnd	/* キーボードマクロを記録中のウィンドウ */
				){
					nColor = COLOR_BTNTEXT;
				}else{
					nColor = COLOR_3DSHADOW;
				}
				::SetTextColor( lpdis->hDC, ::GetSysColor( nColor ) );
				::SetBkMode( lpdis->hDC, TRANSPARENT );
				::TextOut( lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top + 2, "REC", lstrlen( "REC" ) );
				if( COLOR_BTNTEXT == nColor ){
					::TextOut( lpdis->hDC, lpdis->rcItem.left + 1, lpdis->rcItem.top + 2, "REC", lstrlen( "REC" ) );
				}
			}
			return 0;
		}else{
			switch( lpdis->CtlType ){
			case ODT_MENU:	/* オーナー描画メニュー */
				/* メニューアイテム描画 */
				m_CMenuDrawer.DrawItem( lpdis );
				return TRUE;
			}
		}
//		return FALSE;
	case WM_MEASUREITEM:
		idCtl = (UINT) wParam;					// control identifier
		lpmis = (MEASUREITEMSTRUCT*) lParam;	// item-size information
		switch( lpmis->CtlType ){
		case ODT_MENU:	/* オーナー描画メニュー */
			CMenuDrawer* pCMenuDrawer;
			pCMenuDrawer = (CMenuDrawer*)lpmis->itemData;


//			MYTRACE( "WM_MEASUREITEM  lpmis->itemID=%d\n", lpmis->itemID );
			/* メニューアイテムの描画サイズを計算 */
			nItemWidth = m_CMenuDrawer.MeasureItem( lpmis->itemID, &nItemHeight );
			if( -1 == nItemWidth ){
			}else{
				lpmis->itemWidth = nItemWidth;
				lpmis->itemHeight = nItemHeight;
			}
			return TRUE;
		}
		return FALSE;




	case WM_PAINT:
		return OnPaint( hwnd, uMsg, wParam, lParam );

	case WM_HELP:
		lphi = (LPHELPINFO) lParam;
		switch( lphi->iContextType ){
		case HELPINFO_MENUITEM:
			OnHelp_MenuItem( hwnd, lphi->iCtrlId );
			break;
		}
		return TRUE;

		//	Jun. 2, 2000 genta
	case WM_ACTIVATEAPP:
	//	::MessageBox( NULL, "OK", "WM_ACTIVATEAPP", MB_OK );
		fActive = LOWORD(wParam);				// activation flag
		if( fActive ){
			::SetFocus( m_hWnd );
		}
		return 0;	//	should return zero. / Jun. 23, 2000 genta
	case WM_ACTIVATE:
		fActive = LOWORD( wParam );				// activation flag
		fMinimized = (BOOL) HIWORD( wParam );	// minimized flag
		hwndTarget = (HWND) lParam;				// window handle

//		MYTRACE( "WM_ACTIVATE " );
 		switch( fActive ){
		case WA_ACTIVE:
//			MYTRACE( " WA_ACTIVE\n" );
			bIsActive = TRUE;
			break;
		case WA_CLICKACTIVE:
//			MYTRACE( " WA_CLICKACTIVE\n" );
			bIsActive = TRUE;
			break;
		case WA_INACTIVE:
//			MYTRACE( " WA_INACTIVE\n" );
			bIsActive = FALSE;
			break;
		}
		if( !bIsActive ){
			hwndActive = hwndTarget;
			while( hwndActive != NULL ){
				hwndActive = ::GetParent( hwndActive );
				if( hwndActive == m_hWnd ){
					bIsActive = TRUE;
					break;
				}
			}
		}
		if( !bIsActive ){
			if( m_hWnd == ::GetWindow( hwndTarget, GW_OWNER ) ){
				bIsActive = TRUE;
			}
		}
		m_cEditDoc.SetParentCaption( !bIsActive );


//		/* スレッドのサスペンド・レジューム */
//		if( bIsActive ){
//		}else{
//			::SuspendThread();
//		}


		return 0L;



	case WM_SIZE:
//		MYTRACE( "WM_SIZE\n" );
		/* WM_SIZE 処理 */
		if( SIZE_MINIMIZED == wParam ){
			m_cEditDoc.SetParentCaption();
		}
		return OnSize( wParam, lParam );

	case WM_IME_COMPOSITION:
		if ( lParam & GCS_RESULTSTR ) {
			/* メッセージの配送 */
			return m_cEditDoc.DispatchEvent( hwnd, uMsg, wParam, lParam );
			return 0;
		}else{
			return DefWindowProc( hwnd, uMsg, wParam, lParam );
		}
	case WM_KILLFOCUS:
	case WM_CHAR:
	case WM_IME_CHAR:
	case WM_KEYUP:
	case WM_ENTERMENULOOP:
		/* メッセージの配送 */
		return m_cEditDoc.DispatchEvent( hwnd, uMsg, wParam, lParam );

	case WM_EXITMENULOOP:
//		MYTRACE( "WM_EXITMENULOOP\n" );
		if( NULL != m_hwndStatusBar ){
			::SendMessage( m_hwndStatusBar, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM) (LPINT)"" );
		}

		/* メッセージの配送 */
		return m_cEditDoc.DispatchEvent( hwnd, uMsg, wParam, lParam );
	case WM_SETFOCUS:
//		MYTRACE( "WM_SETFOCUS\n" );

		/* ファイルのタイムスタンプのチェック処理 */
		m_cEditDoc.CheckFileTimeStamp();


		/* 編集ウィンドウリストへの登録 */
		m_cShareData.AddEditWndList( m_hWnd );
		/* メッセージの配送 */
		lRes = m_cEditDoc.DispatchEvent( hwnd, uMsg, wParam, lParam );

		/* 印刷プレビューモードか */
		if( TRUE == m_cEditDoc.m_bPrintPreviewMode ){
			if( NULL != m_hwndPrintPreviewBar ){
				::SetFocus( m_hwndPrintPreviewBar );
			}
		}

		return lRes;


	case WM_NOTIFY:
		idCtrl = (int) wParam;
		pnmh = (LPNMHDR) lParam;
		switch( pnmh->code ){
		case TTN_NEEDTEXT:
			lptip = (LPTOOLTIPTEXT)pnmh;
			{
				/* ツールバーのツールチップのテキストをセット */
//				CMemory		cmemWork;
				char		szWork[256];
				CMemory**	ppcAssignedKeyList;
				int			nAssignedKeyNum;
				int			j;
				char*		pszKey;
				int			nKeyLen;
				strcpy( szLabel, "" );
				if( 0 < ::LoadString( m_hInstance, lptip->hdr.idFrom, szWork, sizeof( szWork ) - 1 ) ){
//					cmemWork.Append( szWork, lstrlen( szWork ) );
					strcat( szLabel, szWork );
				}
				/* 機能に対応するキー名の取得(複数) */
				nAssignedKeyNum = CKeyBind::GetKeyStrList(
					m_hInstance, m_pShareData->m_nKeyNameArrNum,
					(KEYDATA*)m_pShareData->m_pKeyNameArr, &ppcAssignedKeyList, lptip->hdr.idFrom
				);
				if( 0 < nAssignedKeyNum ){
					for( j = 0; j < nAssignedKeyNum; ++j ){
//						cmemWork.Append( "\n        ", lstrlen( "\n        " ) );
						strcat( szLabel, "\n        " );
						pszKey = ppcAssignedKeyList[j]->GetPtr( &nKeyLen );
//						cmemWork.Append( pszKey, nKeyLen );
						strcat( szLabel, pszKey );
						delete ppcAssignedKeyList[j];
					}
					delete [] ppcAssignedKeyList;
				}
//				lptip->hinst = m_hInstance;
//				lptip->lpszText	= MAKEINTRESOURCE( lptip->hdr.idFrom );
				lptip->hinst = NULL;
//				lptip->lpszText	= cmemWork.GetPtr( NULL );
				lptip->lpszText	= szLabel;
			}
			break;
		}
		return 0L;
	case WM_COMMAND:
		OnCommand( HIWORD(wParam), LOWORD(wParam), (HWND) lParam );
		return 0L;
	case WM_INITMENUPOPUP:
		InitMenu( (HMENU)wParam, (UINT)LOWORD( lParam ), (BOOL)HIWORD( lParam ) );
		return 0L;
	case WM_DROPFILES:
		/* ファイルがドロップされた */
		OnDropFiles( (HDROP) wParam );
		return 0L;
	case WM_QUERYENDSESSION:
		if( OnClose() ){
			DestroyWindow( hwnd );
			return TRUE;
		}else{
			return FALSE;
		}
	case WM_CLOSE:
		if( OnClose() ){
			DestroyWindow( hwnd );
		}
		return 0L;
	case WM_DESTROY:
//		::MYMESSAGEBOX( NULL, MB_OK, GSTR_APPNAME, "WM_DESTROY START" );
		if( m_pShareData->m_bRecordingKeyMacro ){					/* キーボードマクロの記録中 */
			if( m_pShareData->m_hwndRecordingKeyMacro == m_hWnd ){	/* キーボードマクロを記録中のウィンドウ */
				m_pShareData->m_bRecordingKeyMacro = FALSE;			/* キーボードマクロの記録中 */
				m_pShareData->m_hwndRecordingKeyMacro = NULL;		/* キーボードマクロを記録中のウィンドウ */
			}
		}

		/* タイマーを削除 */
		::KillTimer( m_hWnd, IDT_TOOLBAR );

		/* ドロップされたファイルを受け入れるのを解除 */
		::DragAcceptFiles( hwnd, FALSE );
		/* 編集ウィンドウリストからの削除 */
		m_cShareData.DeleteEditWndList( m_hWnd );

		if( m_pShareData->m_hwndDebug == m_hWnd ){
			m_pShareData->m_hwndDebug = NULL;
		}
		m_hWnd = NULL;

//		::MYMESSAGEBOX( NULL, MB_OK, GSTR_APPNAME, "WM_DESTROY step-1" );

//		/* スレッドの後始末 */
//		if( NULL != m_hThread ){
//			DWORD dwRes;
//			::SetThreadPriority( m_hThread, THREAD_PRIORITY_HIGHEST );
//			dwRes = ::WaitForSingleObject( m_hThread, 1000 * 10 );
//			if( WAIT_TIMEOUT == dwRes ){
//				::MYMESSAGEBOX( NULL, MB_OK, GSTR_APPNAME, "WAIT_TIMEOUT == WaitForSingleObject()" );
//				::SuspendThread( m_hThread );
//			}
//			::CloseHandle( m_hThread );
//			m_hThread = NULL;
//		}


		/* 編集ウィンドウオブジェクトからのオブジェクト削除要求 */
//シングルプロセス版
//		::PostMessage( m_pShareData->m_hwndTray, MYWM_DELETE_ME, 0, (LPARAM)this );
		::PostMessage( m_pShareData->m_hwndTray, MYWM_DELETE_ME, 0, 0 );

		/* Windows にスレッドの終了を要求します */
		::PostQuitMessage( 0 );

		return 0L;
	case MYWM_CLOSE:
		/* エディタへの全終了要求 */
		if( ( nRet = OnClose() ) ){
			DestroyWindow( hwnd );
		}
		return nRet;
//	case MYWM_AREYOUGREP:
//		if( m_cEditDoc.m_bGrepMode ){
//			strcpy( m_pShareData->m_szWork, m_cEditDoc.m_szGrepKey );
//			return TRUE;
//		}else{
//			return FALSE;
//		}


	case MYWM_GETFILEINFO:
		/* トレイからエディタへの編集ファイル名要求通知 */
//		pfi = (FileInfo*)m_pShareData->m_szWork;
		pfi = (FileInfo*)&m_pShareData->m_FileInfo_MYWM_GETFILEINFO;

		/* 編集ファイル情報を格納 */
		m_cEditDoc.SetFileInfo( pfi );
		return 0L;
	case MYWM_CHANGESETTING:
		/* 設定変更の通知 */
// Oct 10, 2000 ao
/* 設定変更時、ツールバーを再作成するようにする */
		if( NULL != m_hwndToolBar ){
			::DestroyWindow(m_hwndToolBar );
			m_hwndToolBar = NULL;
			CreateToolBar();
#if 0
			UINT	uToolType;
			uToolType = (UINT)::GetWindowLong( m_hwndToolBar, GWL_STYLE );
			if( !( uToolType & TBSTYLE_FLAT ) &&
				TRUE == m_pShareData->m_Common.m_bToolBarIsFlat ){	/* フラットツールバーにする／しない */
				uToolType |= (TBSTYLE_FLAT);
				::SetWindowLong( m_hwndToolBar, GWL_STYLE, (LONG)uToolType );
				::InvalidateRect( m_hwndToolBar, NULL, TRUE );
			}else
			if( ( uToolType & TBSTYLE_FLAT ) &&
				FALSE == m_pShareData->m_Common.m_bToolBarIsFlat ){	/* フラットツールバーにする／しない */
				uToolType = uToolType ^ (TBSTYLE_FLAT);
				::SetWindowLong( m_hwndToolBar, GWL_STYLE, (LONG)uToolType );
				::InvalidateRect( m_hwndToolBar, NULL, TRUE );
			}
#endif
		}
// Oct 10, 2000 ao ここまで

		if( NULL != m_CFuncKeyWnd.m_hWnd ){
			BOOL bSizeBox;
			if( m_pShareData->m_Common.m_nFUNCKEYWND_Place == 0 ){	/* ファンクションキー表示位置／0:上 1:下 */
				/* サイズボックスの表示／非表示切り替え */
				bSizeBox = FALSE;
			}else{
				bSizeBox = TRUE;
				/* ステータスパーを表示している場合はサイズボックスを表示しない */
				if( NULL != m_hwndStatusBar ){
					bSizeBox = FALSE;
				}
			}
			m_CFuncKeyWnd.SizeBox_ONOFF( bSizeBox );
		}

		//	Aug, 21, 2000 genta
		m_cEditDoc.ReloadAutoSaveParam();

//1999.09.03 なーんかうまく動かんので、やめた。運用でカバーしてほしい。
//
//ファンクションキー窓位置変更したとき、サイズボックスの位置を変更
//		m_cEditDoc.m_cSplitterWnd.DoSplit( -1, -1);
//ファンクションキー窓位置変更したとき、ファンクションキー窓位置変更
//		::GetWindowRect( hwnd, &rc );
//		::SetWindowPos( hwnd, 0, 0, 0, rc.right - rc.left, rc.bottom - rc.top + 1, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER );
//		::SetWindowPos( hwnd, 0, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER );

		m_cEditDoc.OnChangeSetting();	/* ビューに設定変更を反映させる */
		return 0L;
	case MYWM_SETACTIVEPANE:
		if( -1 == (int)wParam ){
			if( 0 == lParam ){
				nPane = m_cEditDoc.m_cSplitterWnd.GetFirstPane();
			}else{
				nPane = m_cEditDoc.m_cSplitterWnd.GetLastPane();
			}
		}
		m_cEditDoc.SetActivePane( nPane );
		return 0L;


	case MYWM_SETCARETPOS:	/* カーソル位置変更通知 */
		ppoCaret = (POINT*)m_pShareData->m_szWork;
		/* 範囲選択中か */
		if( m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在の選択範囲を非選択状態に戻す */
			m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].DisableSelectArea( TRUE );
		}
		/*
		カーソル位置変換
		 物理位置(行頭からのバイト数、折り返し無し行位置)
		→
		 レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
		*/
		m_cEditDoc.m_cLayoutMgr.CaretPos_Phys2Log(
			ppoCaret->x,
			ppoCaret->y,
			&nCaretPosX,
			&nCaretPosY
		);
		/* カーソル移動 */
		if( nCaretPosY >= m_cEditDoc.m_cLayoutMgr.GetLineCount() ){
			/*ファイルの最後に移動 */
//			m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].Command_GOFILEEND(FALSE);
			m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].HandleCommand( F_GOFILEEND, FALSE, 0, 0, 0, 0 );
		}else{
			m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].MoveCursor( nCaretPosX, nCaretPosY, TRUE, _CARETMARGINRATE / 3 );
			m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].m_nCaretPosX_Prev =
			m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].m_nCaretPosX;
		}
		return 0L;


	case MYWM_GETCARETPOS:	/* カーソル位置取得要求 */
		ppoCaret = (POINT*)m_pShareData->m_szWork;
		/*
		カーソル位置変換
		 レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
		→
		物理位置(行頭からのバイト数、折り返し無し行位置)
		*/
		m_cEditDoc.m_cLayoutMgr.CaretPos_Log2Phys(
			m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].m_nCaretPosX,
			m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].m_nCaretPosY,
			(int*)&ppoCaret->x,
			(int*)&ppoCaret->y
		);
		return 0L;

	case MYWM_GETLINEDATA:	/* 行(改行単位)データの要求 */
		pLine = m_cEditDoc.m_cDocLineMgr.GetLineStr( (int)wParam, &nLineLen );
		if( NULL == pLine ){
			return 0;
		}
		if( nLineLen > sizeof( m_pShareData->m_szWork ) ){
			memcpy( m_pShareData->m_szWork, pLine, sizeof( m_pShareData->m_szWork ) );
		}else{
			memcpy( m_pShareData->m_szWork, pLine, nLineLen );
		}
		return nLineLen;


	case MYWM_ADDSTRING:
//		MYTRACE( "MYWM_ADDSTRING[%s]\n", m_pShareData->m_szWork );
		m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].HandleCommand( F_ADDTAIL, TRUE, (LPARAM)m_pShareData->m_szWork, (LPARAM)lstrlen( m_pShareData->m_szWork ), 0, 0 );
		m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].HandleCommand( F_GOFILEEND, TRUE, 0, 0, 0, 0 );
//		m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].Command_ADDTAIL(
//			m_pShareData->m_szWork,
//			lstrlen( m_pShareData->m_szWork )
//		);
		return 0L;
	case MYWM_SETREFERER:
		ppoCaret = (POINT*)m_pShareData->m_szWork;

//		m_cEditDoc.SetReferer( (HWND)wParam, (int)lParam );
		m_cEditDoc.SetReferer( (HWND)wParam, ppoCaret->x, ppoCaret->y );
		return 0L;
	default:
		return DefWindowProc( hwnd, uMsg, wParam, lParam );
	}
	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}




/* 終了時の処理 */
int	CEditWnd::OnClose( void )
{
	/* ウィンドウをアクティブにする */
	/* アクティブにする */
	ActivateFrameWindow( m_hWnd );
//	if( ::IsIconic( m_hWnd ) ){
//		::ShowWindow( m_hWnd, SW_RESTORE );
//	}else{
//		::ShowWindow( m_hWnd, SW_SHOW );
//	}
//	::SetForegroundWindow( m_hWnd );
//	::SetActiveWindow( m_hWnd );
	/* ファイルを閉じるときのMRU登録 & 保存確認 & 保存実行 */
	return m_cEditDoc.OnFileClose();
}






void CEditWnd::OnCommand( WORD wNotifyCode, WORD wID , HWND hwndCtl )
{
//	MYTRACE( "CEditWnd::OnCommand()\n" );

	static			CHOOSEFONT cf;
//	static			LOGFONT lf;
	int				i;
	CMemory			cMemKeyList;
	HGLOBAL			hgClip;
	char*			pszClip;
//	CProp1			cProp1;	/* 設定プロパティシート */
	int				nFuncCode;
	HWND			hwndWork;
	BOOL			bOpened;
//	RECT			rc;
	FileInfo*		pfi;
	HWND			hWndOwner;
//	int				nMenuIdx;
//	HINSTANCE		hInstBrowser;
	static char		szURL[1024];

	switch( wNotifyCode ){
	/* メニューからのメッセージ */
	case 0:
		switch( wID ){
//		case IDM_EXITALL:
		case F_EXITALL:	//Dec. 26, 2000 JEPRO F_に変更
			/* サクラエディタの全終了 */
			CEditApp::TerminateApplication();
			break;
		/* キー割り当て一覧を作成 */
		/* 割り当てられているキーストロークの数を返す */
//From Here Sept. 15, 2000 JEPRO
// IDM_TESTをFに変更したがショートカットキーがうまく働かない	//Dec. 25, 2000 復活
//		case IDM_TEST_CREATEKEYBINDLIST:
		case F_CREATEKEYBINDLIST:
//To Here Sept. 15, 2000
			i = CKeyBind::CreateKeyBindList(
				m_hInstance,
				m_pShareData->m_nKeyNameArrNum,
				m_pShareData->m_pKeyNameArr,
				cMemKeyList
			 );

			/* Windowsクリップボードにコピー */
			hgClip = ::GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, cMemKeyList.GetLength() + 1 );
			pszClip = (char*)::GlobalLock( hgClip );
			memcpy( pszClip, cMemKeyList.GetPtr( NULL ), cMemKeyList.GetLength() + 1 );
			::GlobalUnlock( hgClip );
			::OpenClipboard( m_cEditDoc.m_hWnd );
			::EmptyClipboard();
			::SetClipboardData( CF_OEMTEXT, hgClip );
			::CloseClipboard();
			break;

//Sept. 15, 2000→Nov. 25, 2000 JEPRO //ショートカットキーがうまく働かないので殺してあった下の2行を修正・復活
//		case IDM_HELP_CONTENTS:
		case F_HELP_CONTENTS:
			/* ヘルプ目次 */
			{
				char	szHelp[_MAX_PATH + 1];
				/* ヘルプファイルのフルパスを返す */
				::GetHelpFilePath( szHelp );
			//From Here Jan. 13, 2001 JEPRO HELP_FINDERでは前回アクティブだったトピックの検索のタブになってしまう
			//一方 HELP_CONTENTS (あるいは HELP＿INDEX) だと目次ページが出てくる。それもいいが...
			//	::WinHelp( m_hWnd, szHelp, HELP_FINDER, 0 );
				::WinHelp( m_hWnd, szHelp, HELP_COMMAND, (unsigned long)"CONTENTS()" );	//[目次]タブの表示
			//To Here Jan. 13, 2001
			}
			break;
//		case IDM_HELP_SEARCH:
		case F_HELP_SEARCH:
			/* ヘルプキーワード検索 */
			{
				char	szHelp[_MAX_PATH + 1];
				/* ヘルプファイルのフルパスを返す */
				::GetHelpFilePath( szHelp );
				::WinHelp( m_hWnd, szHelp, HELP_KEY, (unsigned long)"" );
			}
			break;
//		case IDM_ABOUT:
		case F_ABOUT:	//Dec. 25, 2000 JEPRO F_に変更
			/* バージョン情報 */
			{
//				m_cEditDoc.m_pcDlgTest->DoModeless( m_hInstance, m_hWnd, (LPARAM)&m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex] );

				CDlgAbout cDlgAbout;
				cDlgAbout.DoModal( m_hInstance, m_hWnd );

#ifdef _DEBUG
//				m_cEditDoc.m_cDocLineMgr.DUMP();
//				m_cEditDoc.m_cLayoutMgr.DUMP();
				HRESULT hres;
				char szPath[_MAX_PATH + 1];
				/* ショートカット(.lnk)の解決 */
				hres = ::ResolveShortcutLink( m_hWnd, "C:\\WINDOWS\\All Users\\ﾃﾞｽｸﾄｯﾌﾟ\\Outlook Express.LNK", szPath );
				MYTRACE( "hres=%xh, szPath=[%s]\n", hres, szPath );

				/* ショートカット(.lnk)の解決 */
				hres = ::ResolveShortcutLink( m_hWnd, "C:\\My Documents\\develop\\EDIT\\sakura\\CDlgCompare.cpp", szPath );
				MYTRACE( "hres=%xh, szPath=[%s]\n", hres, szPath );

				/* パス名に対するアイテムＩＤリストを作成する */
				BOOL bRes;
				ITEMIDLIST* pIDL;
//				char szPath[MAX_PATH + 1];
				pIDL = CreateItemIDList( "C:\\WINDOWS\\ALLUSE~1\\ﾃﾞｽｸﾄｯﾌﾟ\\OUTLOO~1.LNK" );
//				pIDL = CreateItemIDList( "\\\\NPSV-NT5\\newpat-src\\nwmsc4\\ZENBUN~1\\ZENBUN~1.RC" );
//				pIDL = CreateItemIDList( "C:\\MYDOCU~1\\DEVELOP\\ZENBUN~1\\CDLGSE~2.CPP" );

				// SHGetPathFromIDList()関数はアイテムＩＤリストの物理パスを探してくれる
				bRes = ::SHGetPathFromIDList( pIDL, szPath );
				MYTRACE( "szPath=[%s]\n", szPath );

				/* アイテムＩＤリストを削除する */
				bRes = DeleteItemIDList( pIDL );
				MYTRACE( "bRes=%d\n", bRes );
#endif
			}
			break;
		default:
			if( wID - IDM_SELWINDOW >= 0 &&
				wID - IDM_SELWINDOW < m_pShareData->m_nEditArrNum ){
				hwndWork = m_pShareData->m_pEditArr[wID - IDM_SELWINDOW].m_hWnd;
				/* アクティブにする */
				ActivateFrameWindow( hwndWork );
			}else
			if( wID - IDM_SELMRU >= 0 &&
//				wID - IDM_SELMRU < (( m_pShareData->m_nMRUArrNum < m_pShareData->m_Common.m_nMRUArrNum_MAX )?m_pShareData->m_nMRUArrNum :m_pShareData->m_Common.m_nMRUArrNum_MAX ) ){
//				wID - IDM_SELMRU < __max( m_pShareData->m_nMRUArrNum, m_pShareData->m_Common.m_nMRUArrNum_MAX )
				wID - IDM_SELMRU < 999
			){
				/* 指定ファイルが開かれているか調べる */
				if( m_cShareData.IsPathOpened( m_pShareData->m_fiMRUArr[wID - IDM_SELMRU].m_szPath, &hWndOwner ) ){

					::SendMessage( hWndOwner, MYWM_GETFILEINFO, 0, 0 );
//					pfi = (FileInfo*)m_pShareData->m_szWork;
					pfi = (FileInfo*)&m_pShareData->m_FileInfo_MYWM_GETFILEINFO;

					/* アクティブにする */
					ActivateFrameWindow( hWndOwner );
					/* MRUリストへの登録 */
					m_cShareData.AddMRUList( pfi );
				}else{
					/* 変更フラグがオフで、ファイルを読み込んでいない場合 */
					if( !m_cEditDoc.m_bIsModified &&
						0 == lstrlen( m_cEditDoc.m_szFilePath )	/* 現在編集中のファイルのパス */
					){
						/* ファイル読み込み */
						m_cEditDoc.FileRead(
							m_pShareData->m_fiMRUArr[wID - IDM_SELMRU].m_szPath,
							&bOpened,
							m_pShareData->m_fiMRUArr[wID - IDM_SELMRU].m_nCharCode,
							FALSE,	/* 読み取り専用か */
							TRUE	/* 文字コード変更時の確認をするかどうか */
						);
					}else{
						/* 新たな編集ウィンドウを起動 */
						//	From Here Oct. 27, 2000 genta	カーソル位置を復元しない機能
						if( m_pShareData->m_Common.GetRestoreCurPosition() ){
							CEditApp::OpenNewEditor2( m_hInstance, m_hWnd, &(m_pShareData->m_fiMRUArr[wID - IDM_SELMRU]), FALSE );
						}
						else {
							CEditApp::OpenNewEditor( m_hInstance, m_hWnd,
								m_pShareData->m_fiMRUArr[wID - IDM_SELMRU].m_szPath,
								m_pShareData->m_fiMRUArr[wID - IDM_SELMRU].m_nCharCode,
								FALSE );

						}
						//	To Here Oct. 27, 2000 genta
					}
				}
			}else
			if( wID - IDM_SELOPENFOLDER >= 0 &&
//				wID - IDM_SELOPENFOLDER < (( m_pShareData->m_nOPENFOLDERArrNum < m_pShareData->m_Common.m_nOPENFOLDERArrNum_MAX )?m_pShareData->m_nOPENFOLDERArrNum:m_pShareData->m_Common.m_nOPENFOLDERArrNum_MAX ) )
//				wID - IDM_SELOPENFOLDER < __max( m_pShareData->m_nOPENFOLDERArrNum, m_pShareData->m_Common.m_nOPENFOLDERArrNum_MAX )
				wID - IDM_SELOPENFOLDER < 999
			){
				{
					char*		pszPath = new char[_MAX_PATH];
					BOOL		bOpened;
					int			nCharCode;
					BOOL		bReadOnly;
					FileInfo*	pfi;
					HWND		hWndOwner;

					strcpy( pszPath, "" );

					/* 「ファイルを開く」ダイアログ */
					nCharCode = CODE_AUTODETECT;	/* 文字コード自動判別 */
					bReadOnly = FALSE;
					if( !m_cEditDoc.OpenFileDialog( m_hWnd, m_pShareData->m_szOPENFOLDERArr[wID - IDM_SELOPENFOLDER], pszPath, &nCharCode, &bReadOnly ) ){
						return;
					}
					/* 指定ファイルが開かれているか調べる */
					if( m_cShareData.IsPathOpened( pszPath, &hWndOwner ) ){
						::SendMessage( hWndOwner, MYWM_GETFILEINFO, 0, 0 );
//						pfi = (FileInfo*)m_pShareData->m_szWork;
						pfi = (FileInfo*)&m_pShareData->m_FileInfo_MYWM_GETFILEINFO;

						int nCharCodeNew;
						if( CODE_AUTODETECT == nCharCode ){	/* 文字コード自動判別 */
							/*
							|| ファイルの日本語コードセット判別
							||
							|| 【戻り値】
							||	SJIS	0
							||	JIS		1
							||	EUC		2
							||	Unicode	3
							||	エラー	-1
							*/
							nCharCodeNew = CMemory::CheckKanjiCodeOfFile( pszPath );
							if( -1 == nCharCodeNew ){

							}else{
								nCharCode = nCharCodeNew;
							}
						}
						if( nCharCode != pfi->m_nCharCode ){	/* 文字コード種別 */
							char*	pszCodeNameCur;
							char*	pszCodeNameNew;
							switch( pfi->m_nCharCode ){
							case CODE_SJIS:		/* SJIS */		pszCodeNameCur = "SJIS";break;	//Sept. 1, 2000 jepro 'シフト'を'S'に変更
							case CODE_JIS:		/* JIS */		pszCodeNameCur = "JIS";break;
							case CODE_EUC:		/* EUC */		pszCodeNameCur = "EUC";break;
							case CODE_UNICODE:	/* Unicode */	pszCodeNameCur = "Unicode";break;
							case CODE_UTF8:		/* UTF-8 */		pszCodeNameCur = "UTF-8";break;
							case CODE_UTF7:		/* UTF-7 */		pszCodeNameCur = "UTF-7";break;
							}
							switch( nCharCode ){
							case CODE_SJIS:		/* SJIS */		pszCodeNameNew = "SJIS";break;	//Sept. 1, 2000 jepro 'シフト'を'S'に変更
							case CODE_JIS:		/* JIS */		pszCodeNameNew = "JIS";break;
							case CODE_EUC:		/* EUC */		pszCodeNameNew = "EUC";break;
							case CODE_UNICODE:	/* Unicode */	pszCodeNameNew = "Unicode";break;
							case CODE_UTF8:		/* UTF-8 */		pszCodeNameNew = "UTF-8";break;
							case CODE_UTF7:		/* UTF-7 */		pszCodeNameNew = "UTF-7";break;
							}
							::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST, GSTR_APPNAME,
								"%s\n\n\n既に開いているファイルを違う文字コードで開く場合は、\n一旦閉じてから開いてください。\n\n現在の文字コードセット=[%s]\n新しい文字コードセット=[%s]",
								pszPath, pszCodeNameCur, pszCodeNameNew
							);
						}
						/* 自分が開いているか */
						if( 0 == strcmp( m_cEditDoc.m_szFilePath, pszPath ) ){
							/* 何もしない */
						}else{
							/* 開いているウィンドウをアクティブにする */
							/* アクティブにする */
							ActivateFrameWindow( hWndOwner );
						}
					}else{
						/* ファイルが開かれていない */
						/* 変更フラグがオフで、ファイルを読み込んでいない場合 */
						if( !m_cEditDoc.m_bIsModified &&
							0 == lstrlen( m_cEditDoc.m_szFilePath )		/* 現在編集中のファイルのパス */
						){
							/* ファイル読み込み */
							m_cEditDoc.FileRead( pszPath, &bOpened, nCharCode, bReadOnly,
								TRUE	/* 文字コード変更時の確認をするかどうか */
							);
						}else{
							if( strchr( pszPath, ' ' ) ){
								char	szFile2[_MAX_PATH + 3];
								wsprintf( szFile2, "\"%s\"", pszPath );
								strcpy( pszPath, szFile2 );
							}
							/* 新たな編集ウィンドウを起動 */
							CEditApp::OpenNewEditor( m_hInstance, m_hWnd, pszPath, nCharCode, bReadOnly );
						}
					}
					delete [] pszPath;
//					return;
				}
			}else{
				/* コマンドコードによる処理振り分け */
				m_cEditDoc.HandleCommand( wID );
			}
			break;
		}
		break;
	/* アクセラレータからのメッセージ */
	case 1:
		nFuncCode = CKeyBind::GetFuncCode(
			wID,
			m_pShareData->m_nKeyNameArrNum,
			m_pShareData->m_pKeyNameArr
		);
//		MYTRACE( "CEditWnd::OnCommand()  nFuncCode=%d\n", nFuncCode );
		m_cEditDoc.HandleCommand( nFuncCode );
		break;

	/* コントロールからのメッセージには通知コード */
	default:
		break;
	}
	return;
}


//	/* メニュー項目を追加 */
//	void CEditWnd::m_CMenuDrawer.MyAppendMenu( HMENU hMenu, int nFlag, int nFuncId, char* pszLabel )
//	{
//		char	szLabel[256];
//		int		nFlagAdd;
//		int		i;
//
//		strcpy( szLabel, "" );
//		if( NULL != pszLabel ){
//			strcpy( szLabel, pszLabel );
//		}
//		if( MF_OWNERDRAW & nFlag ){
//
//		}else{
//
//		}
//		if( nFuncId != 0 ){
//			/* メニューラベルの作成 */
//			CKeyBind::GetMenuLabel(
//				m_hInstance,
//				m_pShareData->m_nKeyNameArrNum,
//				m_pShareData->m_pKeyNameArr,
//				nFuncId,
//				szLabel
//			 );
//		}
//		/* メニュー項目を挿入 */
//		MYTRACE( "::InsertMenu()\n" );
//		nFlagAdd = 0;
//		for( i = 0; i < m_cShareData.m_nMyButtonNum; ++i ){
//			if( nFuncId == m_cShareData.m_tbMyButton[i].idCommand ){
//				nFlagAdd = MF_OWNERDRAW;
//				break;
//			}
//		}
//		::InsertMenu(
//			hMenu,
//			0xFFFFFFFF,
//			nFlag | nFlagAdd,
//			nFuncId,
//			szLabel
//		);
//		return;
//	}

//	キーワード：メニューバー順序
//	Sept.14, 2000 Jepro note: メニューバーの項目のキャプションや順番設定などは以下で行っているらしい
//	Sept.16, 2000 Jepro note: アイコンとの関連付けはCShareData_new2.cppファイルで行っている
void CEditWnd::InitMenu( HMENU hMenu, UINT uPos, BOOL fSystemMenu )
{
	int			cMenuItems;
	int			nPos;
	UINT		id;
	UINT		fuFlags;
	int			i;
	int			j;
//	int			k;
	BOOL		bRet;
	char		szMemu[280];
	HWND		hwndDummy;
	int			nRowNum;
	EditNode*	pEditNodeArr;
	FileInfo*	pfi;

	HMENU		hMenuPopUp;
	HMENU		hMenuPopUp_2;
	const char*	pszLabel;

//	char**		ppszFolderArr;
//	int			nFolderArrNum;
//	char		szFolder[_MAX_PATH];

//	MENUITEMINFO miif;
//	memset( &miif, 0, sizeof( MENUITEMINFO ) );
//	cbSize = sizeof( MENUITEMINFO );
//	::GetMenuItemInfo( ::GetMenu( m_hWnd ), uPos, TRUE, &miif );

	if( hMenu != ::GetSubMenu( ::GetMenu( m_hWnd ), uPos ) ){
		goto end_of_func_IsEnable;
	}


	if( fSystemMenu ){
	}else{
//		MYTRACE( "hMenu=%08xh uPos=%d, fSystemMenu=%s\n", hMenu, uPos, fSystemMenu ? "TRUE":"FALSE" );
		switch( uPos ){
		case 0:
			/* 「ファイル」メニュー */
			m_CMenuDrawer.ResetContents();
			cMenuItems = ::GetMenuItemCount( hMenu );
			for( i = cMenuItems - 1; i >= 0; i-- ){
				bRet = ::DeleteMenu( hMenu, i, MF_BYPOSITION );
			}
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_FILENEW			, "新規作成(&N)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_FILEOPEN		, "開く(&O)..." );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_FILESAVE		, "上書き保存(&S)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_FILESAVEAS		, "名前を付けて保存(&A)..." );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_FILECLOSE		, "閉じて(無題) (&B)" );	//Oct. 17, 2000 jepro キャプションを「閉じる」から変更	//Feb. 18, 2001 JEPRO アクセスキー変更(C→B; Blankの意味)
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_FILECLOSE_OPEN	, "閉じて開く(&L)..." );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WINCLOSE		, "ウィンドウを閉じる(&C)" );	//Feb. 18, 2001	JEPRO 追加

			// 「文字コードセット」ポップアップメニュー
			hMenuPopUp_2 = ::CreateMenu();
			int	nWork;
			if( m_cEditDoc.m_nCharCode == CODE_SJIS ){
				nWork = MF_BYPOSITION | MF_STRING | MF_CHECKED;
			}else{
				nWork = MF_BYPOSITION | MF_STRING;
			}
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp_2, nWork, F_FILE_REOPEN_SJIS, "&SJISで開き直す" );	//Nov. 7, 2000 jepro キャプションに'で開き直す'を追加
			if( m_cEditDoc.m_nCharCode == CODE_JIS ){
				nWork = MF_BYPOSITION | MF_STRING | MF_CHECKED;
			}else{
				nWork = MF_BYPOSITION | MF_STRING;
			}
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp_2, nWork, F_FILE_REOPEN_JIS, "&JISで開き直す" );	//Nov. 7, 2000 jepro キャプションに'で開き直す'を追加
			if( m_cEditDoc.m_nCharCode == CODE_EUC ){
				nWork = MF_BYPOSITION | MF_STRING | MF_CHECKED;
			}else{
				nWork = MF_BYPOSITION | MF_STRING;
			}
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp_2, nWork, F_FILE_REOPEN_EUC, "&EUCで開き直す" );	//Nov. 7, 2000 jepro キャプションに'で開き直す'を追加
			if( m_cEditDoc.m_nCharCode == CODE_UNICODE ){
				nWork = MF_BYPOSITION | MF_STRING | MF_CHECKED;
			}else{
				nWork = MF_BYPOSITION | MF_STRING;
			}
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp_2, nWork, F_FILE_REOPEN_UNICODE, "&Unicodeで開き直す" );	//Nov. 7, 2000 jepro キャプションに'で開き直す'を追加
			if( m_cEditDoc.m_nCharCode == CODE_UTF8 ){
				nWork = MF_BYPOSITION | MF_STRING | MF_CHECKED;
			}else{
				nWork = MF_BYPOSITION | MF_STRING;
			}
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp_2, nWork, F_FILE_REOPEN_UTF8, "UTF-&8で開き直す" );	//Nov. 7, 2000 jepro キャプションに'で開き直す'を追加
			if( m_cEditDoc.m_nCharCode == CODE_UTF7 ){
				nWork = MF_BYPOSITION | MF_STRING | MF_CHECKED;
			}else{
				nWork = MF_BYPOSITION | MF_STRING;
			}
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp_2, nWork, F_FILE_REOPEN_UTF7, "UTF-&7で開き直す" );	//Nov. 7, 2000 jepro キャプションに'で開き直す'を追加

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp_2 , "文字コードセット(&H)" );//Oct. 11, 2000 JEPRO アクセスキー変更(M→H)
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );


			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_PRINT				, "印刷(&P)..." );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_PRINT_PREVIEW		, "印刷プレビュー(&V)..." );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_PRINT_PAGESETUP		, "ページ設定(&U)..." );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );

			// 「ファイル操作」ポップアップメニュー
			hMenuPopUp = ::CreateMenu();
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_OPEN_HfromtoC				, "同名のC/C++ヘッダ(ソース)を開く(&C)" );	//Feb. 7, 2001 JEPRO 追加
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_OPEN_HHPP					, "同名のC/C++ヘッダファイルを開く(&H)" );	//Sept. 11, 2000 JEPRO キャプションとアクセスキー変更(I→H)	//Feb. 9, 2001 jepro「.cまたは.cppと同名の.hを開く」から変更 & 統合
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_OPEN_CCPP					, "同名のC/C++ソースファイルを開く(&C)" );	//Sept. 11, 2000 JEPRO キャプションとアクセスキー変更(J→C)	//Feb. 9, 2001 jepro「.hと同名の.c(なければ.cpp)を開く」から変更 & 統合
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_ACTIVATE_SQLPLUS			, "SQL*Plusをアクティブ表示(&A)" );	//Sept. 11, 2000 JEPRO アクセスキー付与	説明の「アクティブ化」を「アクティブ表示」に統一
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_PLSQL_COMPILE_ON_SQLPLUS	, "SQL*Plusで実行(&X)" );			//Sept. 11, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_BROWSE						, "ブラウズ(&B)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_PROPERTY_FILE				, "ファイルのプロパティ(&R)" );		//Nov. 7, 2000 jepro キャプションに'ファイルの'を追加
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_SENDMAIL					, "メール送信(&E)..." );

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp , "ファイル操作(&R)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );



			/* MRUリストのファイルのリストをメニューにする */
			j = 0;
			if( m_pShareData->m_nMRUArrNum > 0 ){
				for( i = 0; i < m_pShareData->m_nMRUArrNum; ++i ){
					if( m_pShareData->m_Common.m_nMRUArrNum_MAX <= i ){
						break;
					}
					j++;
				}
			}
			if( j > 0 ){
				hMenuPopUp = ::CreateMenu();
				j = 0;
				for( i = 0; i < m_pShareData->m_nMRUArrNum; ++i ){
					if( m_pShareData->m_Common.m_nMRUArrNum_MAX <= j ){
						break;
					}
					//	Aug. 23, 2000 genta

//	From Here Oct. 4, 2000 JEPRO added, commented out & modified
//		ファイル名やパス名に'&'が使われているときに履歴等でキチンと表示されない問題を修正(&を&&に置換するだけ)
					char	szFile2[_MAX_PATH + 3];	//	'+1'かな？ ようわからんので多めにしとこ。わかる人修正報告ください！
					char	*p;
					strcpy( szFile2, m_pShareData->m_fiMRUArr[i].m_szPath );
					if( (p = strchr( szFile2, '&' )) != NULL ){
						char	buf[_MAX_PATH + 3];	//	'+1'かな？ ようわからんので多めにしとこ。わかる人修正報告ください！
						do {
							*p = '\0';
							strcpy( buf, p + strlen("&") );
							strcat( szFile2, "&&" );
							strcat( szFile2, buf );
							p = strchr( p + strlen("&&"), '&' );
						} while ( p != NULL );
					}

//					if( j < 10 ){
//						wsprintf( szMemu, "&%c %s", j + '0', m_pShareData->m_fiMRUArr[i].m_szPath );
//					}
//					else if( j < 10 + 26 ){
//						wsprintf( szMemu, "&%c %s", j - 10 + 'A', m_pShareData->m_fiMRUArr[i].m_szPath );
//					}else{
//						wsprintf( szMemu, "  %s", m_pShareData->m_fiMRUArr[i].m_szPath );
//					}
//		修正の都合上作業変数で書くように変更
//		j >= 10 + 26 の時の考慮を省いた(に近い)がファイルの履歴MAXを36個にしてあるので事実上OKでしょう
					wsprintf( szMemu, "&%c %s", (j < 10)?('0' + j):('A' + j - 10), szFile2 );
//	To Here Oct. 4, 2000
					if( 0 != m_pShareData->m_fiMRUArr[i].m_nCharCode ){		/* 文字コード種別 */
						switch( m_pShareData->m_fiMRUArr[i].m_nCharCode ){
//	From Here Oct. 5, 2000 JEPRO commented out & modified
//						case 1:
//							strcat( szMemu, "  [JIS]" );
//							break;
//						case 2:
//							strcat( szMemu, "  [EUC]" );
//							break;
//						case 3:
//							strcat( szMemu, "  [Unicode]" );
//							break;
						case CODE_JIS:		/* JIS */
							strcat( szMemu, "  [JIS]" );
							break;
						case CODE_EUC:		/* EUC */
							strcat( szMemu, "  [EUC]" );
							break;
						case CODE_UNICODE:	/* Unicode */
							strcat( szMemu, "  [Unicode]" );
							break;
						case CODE_UTF8:		/* UTF-8 */
							strcat( szMemu, "  [UTF-8]" );
							break;
						case CODE_UTF7:		/* UTF-7 */
							strcat( szMemu, "  [UTF-7]" );
							break;
//	To Here Oct. 5, 2000
						}
					}
					m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, IDM_SELMRU + i, szMemu );
					j++;
					if( m_cShareData.IsPathOpened( m_pShareData->m_fiMRUArr[i].m_szPath, &hwndDummy ) ){
						if( 0 != _stricmp( m_cEditDoc.m_szFilePath, m_pShareData->m_fiMRUArr[i].m_szPath ) ){
							::SetMenuItemBitmaps( hMenuPopUp, IDM_SELMRU + i, MF_BYCOMMAND | MF_CHECKED, NULL, m_hbmpOPENED );
							::CheckMenuItem( hMenuPopUp, IDM_SELMRU + i, MF_BYCOMMAND | MF_CHECKED );
						}else{
							::SetMenuItemBitmaps( hMenuPopUp, IDM_SELMRU + i, MF_BYCOMMAND | MF_CHECKED, NULL, m_hbmpOPENED_THIS );
							::CheckMenuItem( hMenuPopUp, IDM_SELMRU + i, MF_BYCOMMAND | MF_CHECKED );
						}
					}
				}
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp , "最近使ったファイル(&F)" );
			}else{
				hMenuPopUp = ::CreateMenu();
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP | MF_GRAYED, (UINT)hMenuPopUp , "最近使ったファイル(&F)" );
			}


			/* 最近使ったフォルダのメニューを作成 */
			if( m_pShareData->m_nOPENFOLDERArrNum > 0 ){
				hMenuPopUp = ::CreateMenu();
				j = 0;
				for( i = 0; i < m_pShareData->m_nOPENFOLDERArrNum; ++i ){
					if( m_pShareData->m_Common.m_nOPENFOLDERArrNum_MAX <= j ){
						break;
					}
					//	Aug. 23, 2000 genta

//	From Here Oct. 4, 2000 JEPRO added, commented out & modified
//		ファイル名やパス名に'&'が使われているときに履歴等でキチンと表示されない問題を修正(&を&&に置換するだけ)
					char	szFolder2[_MAX_PATH + 3];	//	'+1'かな？ ようわからんので多めにしとこ。わかる人修正報告ください！
					char	*p;
					strcpy( szFolder2, m_pShareData->m_szOPENFOLDERArr[i] );
					if( (p = strchr( szFolder2, '&' )) != NULL ){
						char	buf[_MAX_PATH + 3];	//	'+1'かな？ ようわからんので多めにしとこ。わかる人修正報告ください！
						do {
							*p = '\0';
							strcpy( buf, p + strlen("&") );
							strcat( szFolder2, "&&" );
							strcat( szFolder2, buf );
							p = strchr( p + strlen("&&"), '&' );
						} while ( p != NULL );
					}

//					if( j < 10 ){
//						wsprintf( szMemu, "&%c %s", j + '0', m_pShareData->m_szOPENFOLDERArr[i] );
//					}
//					else if( j < 10 + 26 ){
//						wsprintf( szMemu, "&%c %s", j - 10 + 'A', m_pShareData->m_szOPENFOLDERArr[i] );
//					}else{
//						wsprintf( szMemu, "  %s", m_pShareData->m_szOPENFOLDERArr[i] );
//					}
//		修正の都合上作業変数で書くように変更
//		j >= 10 + 26 の時の考慮を省いた(に近い)がフォルダの履歴MAXを36個にしてあるので事実上OKでしょう
					wsprintf( szMemu, "&%c %s", (j < 10)?('0' + j):('A' + j - 10), szFolder2 );
//	To Here Oct. 4, 2000
					m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, IDM_SELOPENFOLDER + i, szMemu );
					j++;
				}
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp				, "最近使ったフォルダ(&D)" );
			}else{
				hMenuPopUp = ::CreateMenu();
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP | MF_GRAYED, (UINT)hMenuPopUp	, "最近使ったフォルダ(&D)" );
			}
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WIN_CLOSEALL								, "すべてのウィンドウを閉じる(&Q)" );	//Feb/ 19, 2001 JEPRO 追加
			//	Jun. 9, 2001 genta ソフトウェア名改称
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING /*| MF_OWNERDRAW*/, F_EXITALL					, "サクラエディタの全終了(&X)" );	//Sept. 11, 2000 jepro キャプションを「アプリケーション終了」から変更	//Dec. 26, 2000 JEPRO F_に変更

#if 0///////////////////
			/* MRUリストのファイルのリストをメニューにする */
			j = 0;
			if( m_pShareData->m_nMRUArrNum > 0 ){
				for( i = 0; i < m_pShareData->m_nMRUArrNum; ++i ){
					if( m_pShareData->m_Common.m_nMRUArrNum_MAX <= i ){
						break;
					}
					j++;
				}
			}
			if( j > 0 ){
//				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR | MF_MENUBARBREAK, 0, NULL );
//				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_MENUBARBREAK, 0, NULL );
				j = 0;
				for( i = 0; i < m_pShareData->m_nMRUArrNum; ++i ){
					if( m_pShareData->m_Common.m_nMRUArrNum_MAX <= i ){
						break;
					}
					if( (j + 1) <= 9 ){
						wsprintf( szMemu, "&%d %s", (j + 1), m_pShareData->m_fiMRUArr[i].m_szPath );
					}else{
						wsprintf( szMemu,	"  %s", m_pShareData->m_fiMRUArr[i].m_szPath
						);
					}
					if( 0 != m_pShareData->m_fiMRUArr[i].m_nCharCode ){		/* 文字コード種別 */
						switch( m_pShareData->m_fiMRUArr[i].m_nCharCode ){
						case CODE_JIS:		/* JIS */
							strcat( szMemu, "  [JIS]" );
							break;
						case CODE_EUC:		/* EUC */
							strcat( szMemu, "  [EUC]" );
							break;
						case CODE_UNICODE:	/* Unicode */
							strcat( szMemu, "  [Unicode]" );
							break;
						case CODE_UTF8:		/* UTF-8 */
							strcat( szMemu, "  [UTF-8]" );
							break;
						case CODE_UTF7:		/* UTF-7 */
							strcat( szMemu, "  [UTF-7]" );
							break;
						}
					}
					m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | ((i==0)?MF_MENUBARBREAK:0), IDM_SELMRU + i, szMemu );
//					if( 0 == i ){
//						m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
//					}
//					m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, IDM_SELMRU + i, szMemu );
					j++;
					if( m_cShareData.IsPathOpened( m_pShareData->m_fiMRUArr[i].m_szPath, &hwndDummy ) ){
						if( 0 != _stricmp( m_cEditDoc.m_szFilePath, m_pShareData->m_fiMRUArr[i].m_szPath ) ){
							::SetMenuItemBitmaps( hMenu, IDM_SELMRU + i, MF_BYCOMMAND | MF_CHECKED, NULL, m_hbmpOPENED );
							::CheckMenuItem( hMenu, IDM_SELMRU + i, MF_BYCOMMAND | MF_CHECKED );
						}else{
							::SetMenuItemBitmaps( hMenu, IDM_SELMRU + i, MF_BYCOMMAND | MF_CHECKED, NULL, m_hbmpOPENED_THIS );
							::CheckMenuItem( hMenu, IDM_SELMRU + i, MF_BYCOMMAND | MF_CHECKED );
						}
					}
				}
			}
#endif//////////////////////////

			break;

		case 1:
			m_CMenuDrawer.ResetContents();
			/* 「編集」メニュー */
			cMenuItems = ::GetMenuItemCount( hMenu );
			for( i = cMenuItems - 1; i >= 0; i-- ){
				bRet = ::DeleteMenu( hMenu, i, MF_BYPOSITION );
			}
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_UNDO		, "元に戻す(&Undo)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_REDO		, "やり直し(&Redo)" );

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_CUT			, "切り取り(&T)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_COPY		, "コピー(&C)" );
			//	Jul, 3, 2000 genta
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_COPYLINESASPASSAGE, "全行引用コピー(&N)" );
			//	Sept. 14, 2000 JEPRO	キャプションに「記号付き」を追加、アクセスキー変更(N→.)
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_COPYLINESASPASSAGE, "選択範囲内全行引用符付きコピー(&.)" );
//			Sept. 30, 2000 JEPRO	引用符付きコピーのアイコンを作成したので上記メニューは重複を避けて「高度な操作」内におくだけにする
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_PASTE		, "貼り付け(&P)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_DELETE		, "削除(&D)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SELECTALL	, "すべて選択(&A)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_CHGMOD_INS	, "挿入／上書きモード切り替え(&M)" );	//Nov. 9, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_COPY_CRLF	, "CR&LF改行でコピー" );				//Nov. 9, 2000 JEPRO 追加
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_PASTEBOX	, "矩形貼り付け(&X)" );					//Sept. 13, 2000 JEPRO 移動に伴いアクセスキー付与	//Oct. 22, 2000 JEPRO アクセスキー変更(P→X)
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_DELETE_BACK	, "カーソル前を削除(&B)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );

			// 「挿入」ポップアップメニュー
			hMenuPopUp = ::CreateMenu();
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_INS_DATE, "日付(&D)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_INS_TIME, "時刻(&T)" );

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp , "挿入(&I)" );

			// 「高度な操作」ポップアップメニュー
			hMenuPopUp = ::CreateMenu();
//***** 邪魔なので、とりあえず表示しない。
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CHGMOD_INS, "挿入／上書きモード切り替え" );				//Oct. 17, 2000 JEPRO 「高度な操作」ではないので上に移動
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_PASTEBOX, "矩形貼り付け(&P)" );							//Sept. 13, 2000 JEPRO アクセスキー付与	//Oct. 17, 2000 JEPRO 「高度な操作」ではないので上に移動
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_DELETE_BACK, "カーソル前を削除(&B)" );						//Oct. 17, 2000 JEPRO 「高度な操作」ではないので上に移動
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_WordDeleteToStart	,	"単語の左端まで削除(&L)" );			//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_WordDeleteToEnd	,	"単語の右端まで削除(&R)" );			//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_SELECTWORD			,	"現在位置の単語選択(&W)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_WordCut			,	"単語切り取り(&T)" );				//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_WordDelete			,	"単語削除(&D)" );					//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_LineCutToStart		,	"行頭まで切り取り(改行単位) (&U)" );//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_LineCutToEnd		,	"行末まで切り取り(改行単位) (&K)" );//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_LineDeleteToStart	,	"行頭まで削除(改行単位) (&H)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_LineDeleteToEnd	,	"行末まで削除(改行単位) (&E)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUT_LINE			,	"行切り取り(折り返し単位) (&X)" );	//Jan. 16, 2001 JEPRO 行(頭・末)関係の順序を入れ替えた
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_DELETE_LINE		,	"行削除(折り返し単位) (&Y)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_DUPLICATELINE		,	"行の二重化(折り返し単位) (&2)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_INDENT_TAB			,	"TABインデント(&A)" );				//Oct. 22, 2000 JEPRO 追加
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_UNINDENT_TAB		,	"逆TABインデント(&B)" );			//Oct. 22, 2000 JEPRO 追加
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_INDENT_SPACE		,	"SPACEインデント(&S)" );			//Oct. 22, 2000 JEPRO 追加
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_UNINDENT_SPACE		,	"逆SPACEインデント(&P)" );			//Oct. 22, 2000 JEPRO 追加
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_COPYLINES				, "選択範囲内全行コピー(&@)" );		//Sept. 14, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_COPYLINESASPASSAGE		, "選択範囲内全行引用符付きコピー(&.)" );//Sept. 13, 2000 JEPRO キャプションから「記号付き」を追加、アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_COPYLINESWITHLINENUMBER, "選択範囲内全行行番号付きコピー(&:)" );//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_COPYPATH			,	"このファイルのパス名をコピー(&\\)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_COPYTAG			,	"このファイルのパス名とカーソル位置をコピー(&^)" );
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, IDM_TEST_CREATEKEYBINDLIST	, "キー割り当て一覧をコピー(&Q)" );	//Sept. 15, 2000 JEPRO キャプションの「...リスト」、アクセスキー変更(K→Q)
//Sept. 16, 2000 JEPRO ショートカットキーがうまく働かないので次行は殺して元に戻してある		//Dec. 25, 2000 復活
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CREATEKEYBINDLIST	, "キー割り当て一覧をコピー(&Q)" );			//Sept. 15, 2000 JEPRO キャプションの「...リスト」、アクセスキー変更(K→Q) IDM_TEST→Fに変更
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WORDSREFERENCE, "単語リファレンス(&W)" );

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp , "高度な操作(&V)" );

		//From Here Feb. 19, 2001 JEPRO [移動(M)], [選択(R)]メニューを[編集]のサブメニューとして移動
			// 「移動」ポップアップメニュー
			hMenuPopUp = ::CreateMenu();
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_UP2		, "カーソル上移動(２行ごと) (&Q)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_DOWN2		, "カーソル下移動(２行ごと) (&K)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_WORDLEFT	, "単語の左端に移動(&L)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_WORDRIGHT	, "単語の右端に移動(&R)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_GOLINETOP	, "行頭に移動(折り返し単位) (&H)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_GOLINEEND	, "行末に移動(折り返し単位) (&E)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_1PageUp	, "１ページアップ(&U)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_1PageDown	, "１ページダウン(&D)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_GOFILETOP	, "ファイルの先頭に移動(&T)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_GOFILEEND	, "ファイルの最後に移動(&B)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CURLINECENTER, "カーソル行をウィンドウ中央へ(&C)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_JUMP, "指定行へジャンプ(&J)..." );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_JUMPPREV	, "移動履歴: 前へ(&P)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_JUMPNEXT	, "移動履歴: 次へ(&N)" );

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp , "移動(&O)" );

			// 「選択」ポップアップメニュー
			hMenuPopUp = ::CreateMenu();
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_SELECTWORD		, "現在位置の単語選択(&W)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_SELECTALL		, "すべて選択(&A)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_BEGIN_SEL		, "範囲選択開始(&S)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_UP_SEL			, "(選択)カーソル上移動(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_DOWN_SEL		, "(選択)カーソル下移動(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_LEFT_SEL		, "(選択)カーソル左移動(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_RIGHT_SEL		, "(選択)カーソル右移動(&)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_UP2_SEL		, "(選択)カーソル上移動(２行ごと) (&Q)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_DOWN2_SEL		, "(選択)カーソル下移動(２行ごと) (&K)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_WORDLEFT_SEL	, "(選択)単語の左端に移動(&L)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_WORDRIGHT_SEL	, "(選択)単語の右端に移動(&R)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_GOLINETOP_SEL	, "(選択)行頭に移動(折り返し単位) (&H)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_GOLINEEND_SEL	, "(選択)行末に移動(折り返し単位) (&E)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_HalfPageUp_Sel	, "(選択)半ページアップ(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_HalfPageDown_Sel, "(選択)半ページダウン(&)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_1PageUp_Sel	, "(選択)１ページアップ(&U)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_1PageDown_Sel	, "(選択)１ページダウン(&D)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_GOFILETOP_SEL	, "(選択)ファイルの先頭に移動(&T)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_GOFILEEND_SEL	, "(選択)ファイルの最後に移動(&B)" );

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp , "選択(&S)" );

			// 「矩形選択」ポップアップメニュー
			hMenuPopUp = ::CreateMenu();
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_BOXSELALL		, "矩形ですべて選択(&A)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_BEGIN_BOX	, "矩形範囲選択開始(&S)" );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_UP_BOX			, "(矩選)カーソル上移動(&)" );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_DOWN_BOX		, "(矩選)カーソル下移動(&)" );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_LEFT_BOX		, "(矩選)カーソル左移動(&)" );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_RIGHT_BOX		, "(矩選)カーソル右移動(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_UP2_BOX			, "(矩選)カーソル上移動(２行ごと) (&Q)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_DOWN2_BOX		, "(矩選)カーソル下移動(２行ごと) (&K)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WORDLEFT_BOX	, "(矩選)単語の左端に移動(&L)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WORDRIGHT_BOX	, "(矩選)単語の右端に移動(&R)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOLINETOP_BOX	, "(矩選)行頭に移動(折り返し単位) (&H)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOLINEEND_BOX	, "(矩選)行末に移動(折り返し単位) (&E)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HalfPageUp_Box	, "(選択)半ページアップ(&)" );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HalfPageDown_Box, "(選択)半ページダウン(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_1PageUp_Box		, "(矩選)１ページアップ(&U)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_1PageDown_Box	, "(矩選)１ページダウン(&D)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOFILETOP_BOX	, "(矩選)ファイルの先頭に移動(&T)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOFILEEND_BOX	, "(矩選)ファイルの最後に移動(&B)" );

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp , "矩形選択(&E)" );

			break;
#if 0	//From Here Feb. 19, 2001 JEPRO [移動][移動], [選択]を[編集]配下に移したのでコメントアウトした ////////////////////
		//From Here Oct. 22, 2000 JEPRO [移動(M)] 新設
		case 2:
			m_CMenuDrawer.ResetContents();
			/* 「移動」メニュー */
			cMenuItems = ::GetMenuItemCount( hMenu );
			for( i = cMenuItems - 1; i >= 0; i-- ){
				bRet = ::DeleteMenu( hMenu, i, MF_BYPOSITION );
			}
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_UPL			, "カーソル上移動(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_DOWNL		, "カーソル下移動(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_LEFTL		, "カーソル左移動(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_RIGHT		, "カーソル右移動(&)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_UP2			, "カーソル上移動(２行ごと) (&Q)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_DOWN2		, "カーソル下移動(２行ごと) (&K)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WORDLEFT	, "単語の左端に移動(&L)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WORDRIGHT	, "単語の右端に移動(&R)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOLINETOP	, "行頭に移動(折り返し単位) (&H)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOLINEEND	, "行末に移動(折り返し単位) (&E)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HalfPageUp	, "半ページアップ(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HalfPageDown, "半ページダウン(&)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_1PageUp		, "１ページアップ(&U)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_1PageDown	, "１ページダウン(&D)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOFILETOP	, "ファイルの先頭に移動(&T)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOFILEEND	, "ファイルの最後に移動(&B)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_CURLINECENTER, "カーソル行をウィンドウ中央へ(&C)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_JUMP, "指定行へジャンプ(&J)..." );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_JUMPPREV	, "移動履歴: 前へ(&P)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_JUMPNEXT	, "移動履歴: 次へ(&N)" );

			break;
		//	To Here Oct. 22, 2000

		//	From Here Oct. 22, 2000 JEPRO [選択(R)] 新設
		case 3:
			m_CMenuDrawer.ResetContents();
			/* 「選択」メニュー */
			cMenuItems = ::GetMenuItemCount( hMenu );
			for( i = cMenuItems - 1; i >= 0; i-- ){
				bRet = ::DeleteMenu( hMenu, i, MF_BYPOSITION );
			}
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SELECTWORD		, "現在位置の単語選択(&W)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SELECTALL		, "すべて選択(&A)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_BEGIN_SEL		, "範囲選択開始(&S)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_UP_SEL			, "(選択)カーソル上移動(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_DOWN_SEL		, "(選択)カーソル下移動(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_LEFT_SEL		, "(選択)カーソル左移動(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_RIGHT_SEL		, "(選択)カーソル右移動(&)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_UP2_SEL			, "(選択)カーソル上移動(２行ごと) (&Q)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_DOWN2_SEL		, "(選択)カーソル下移動(２行ごと) (&K)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WORDLEFT_SEL	, "(選択)単語の左端に移動(&L)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WORDRIGHT_SEL	, "(選択)単語の右端に移動(&R)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOLINETOP_SEL	, "(選択)行頭に移動(折り返し単位) (&H)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOLINEEND_SEL	, "(選択)行末に移動(折り返し単位) (&E)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HalfPageUp_Sel	, "(選択)半ページアップ(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HalfPageDown_Sel, "(選択)半ページダウン(&)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_1PageUp_Sel		, "(選択)１ページアップ(&U)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_1PageDown_Sel	, "(選択)１ページダウン(&D)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOFILETOP_SEL	, "(選択)ファイルの先頭に移動(&T)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOFILEEND_SEL	, "(選択)ファイルの最後に移動(&B)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );

			// 「矩形選択」ポップアップメニュー
			hMenuPopUp = ::CreateMenu();
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_BOXSELALL		, "矩形ですべて選択(&A)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_BEGIN_BOX	, "矩形範囲選択開始(&S)" );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_UP_BOX			, "(矩選)カーソル上移動(&)" );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_DOWN_BOX		, "(矩選)カーソル下移動(&)" );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_LEFT_BOX		, "(矩選)カーソル左移動(&)" );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_RIGHT_BOX		, "(矩選)カーソル右移動(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_UP2_BOX			, "(矩選)カーソル上移動(２行ごと) (&Q)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_DOWN2_BOX		, "(矩選)カーソル下移動(２行ごと) (&K)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WORDLEFT_BOX	, "(矩選)単語の左端に移動(&L)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WORDRIGHT_BOX	, "(矩選)単語の右端に移動(&R)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOLINETOP_BOX	, "(矩選)行頭に移動(折り返し単位) (&H)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOLINEEND_BOX	, "(矩選)行末に移動(折り返し単位) (&E)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HalfPageUp_Box	, "(選択)半ページアップ(&)" );
////			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HalfPageDown_Box, "(選択)半ページダウン(&)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_1PageUp_Box		, "(矩選)１ページアップ(&U)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_1PageDown_Box	, "(矩選)１ページダウン(&D)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOFILETOP_BOX	, "(矩選)ファイルの先頭に移動(&T)" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GOFILEEND_BOX	, "(矩選)ファイルの最後に移動(&B)" );

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp , "矩形選択(&X)" );
		//To Here Feb. 19, 2001 JEPRO [移動(M)], [選択(R)]メニューを[編集]のサブメニューとして移動

			break;
			//	To Here Oct. 22, 2000
#endif	//To Here Feb. 19, 2001 JEPRO [移動], [選択]を[編集]配下に移したのでコメントアウトした ////////////////////

//		case 4://case 2: (Oct. 22, 2000 JEPRO [移動]と[選択]を新設したため番号を2つシフトした)
		case 2://Feb. 19, 2001 JEPRO [移動]と[選択]を[編集]配下に移動したため番号を元に戻した
			m_CMenuDrawer.ResetContents();
			/* 「変換」メニュー */
			cMenuItems = ::GetMenuItemCount( hMenu );
			for( i = cMenuItems - 1; i >= 0; i-- ){
				bRet = ::DeleteMenu( hMenu, i, MF_BYPOSITION );
			}
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TOLOWER				, "英大文字→英小文字(&L)" );			//Sept. 10, 2000 jepro キャプションを英語から変更
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TOUPPER				, "英小文字→英大文字(&U)" );			//Sept. 10, 2000 jepro キャプションを英語から変更
//	From Here Sept. 18, 2000 JEPRO
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TOHANKAKU			, "全角→半角" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TOZENKAKUKATA		, "半角→全角カタカナ" );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TOZENKAKUHIRA		, "半角→全角ひらがな" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TOHANKAKU			, "全角→半角(&F)" );					//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TOZENEI				, "半角英数→全角英数" );				//July. 29, 2001 Misaka アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TOZENKAKUKATA		, "半角＋全ひら→全角・カタカナ(&Z)" );	//Sept. 13, 2000 JEPRO キャプション変更 & アクセスキー付与 //Oct. 11, 2000 JEPRO キャプション変更
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TOZENKAKUHIRA		, "半角＋全カタ→全角・ひらがな(&N)" );	//Sept. 13, 2000 JEPRO キャプション変更 & アクセスキー付与 //Oct. 11, 2000 JEPRO キャプション変更
//	To Here Sept. 18, 2000
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HANKATATOZENKAKUKATA, "半角カタカナ→全角カタカナ(&K)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HANKATATOZENKAKUHIRA, "半角カタカナ→全角ひらがな(&H)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TABTOSPACE			, "TAB→空白(&S)" );	//Feb. 19, 2001 JEPRO 下から移動した
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SPACETOTAB			, "空白→TAB(&T)" );	//#### Stonee, 2001/05/27
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );

			//「文字コード変換」ポップアップ
			hMenuPopUp = ::CreateMenu();
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CODECNV_AUTO2SJIS		, "自動判別→SJISコード変換(&A)" );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CODECNV_EMAIL			, "E-Mail(JIS→SJIS)コード変換(&M)" );//Sept. 11, 2000 JEPRO キャプションに「E-Mail」を追加しアクセスキー変更(V→M:Mail)
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CODECNV_EUC2SJIS		, "EUC→SJISコード変換(&W)" );		//Sept. 11, 2000 JEPRO アクセスキー変更(E→W:Work Station)
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CODECNV_UNICODE2SJIS	, "Unicode→SJISコード変換(&U)" );	//Sept. 11, 2000 JEPRO アクセスキー変更候補はI:shIft
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CODECNV_UTF82SJIS		, "UTF-8→SJISコード変換(&T)" );	//Sept. 11, 2000 JEPRO アクセスキー付与(T:uTF/shifT)	//Oct. 6, 2000 簡潔表示にした
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CODECNV_UTF72SJIS		, "UTF-7→SJISコード変換(&F)" );	//Sept. 11, 2000 JEPRO アクセスキー付与(F:utF/shiFt)	//Oct. 6, 2000 簡潔表示にした
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CODECNV_SJIS2JIS		, "SJIS→JISコード変換(&J)" );		//Sept. 11, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CODECNV_SJIS2EUC		, "SJIS→EUCコード変換(&E)" );		//Sept. 11, 2000 JEPRO アクセスキー付与
//			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CODECNV_SJIS2UNICODE	, "SJIS→&Unicodeコード変換" );		//Sept. 11, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CODECNV_SJIS2UTF8		, "SJIS→UTF-8コード変換(&8)" );	//Sept. 11, 2000 JEPRO アクセスキー付与 //Oct. 6, 2000 簡潔表示にした
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CODECNV_SJIS2UTF7		, "SJIS→UTF-7コード変換(&7)" );	//Sept. 11, 2000 JEPRO アクセスキー付与 //Oct. 6, 2000 簡潔表示にした
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_BASE64DECODE			, "Base64デコードして保存(&B)" );	//Oct. 6, 2000 JEPRO アクセスキー変更(6→B)
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_UUDECODE				, "uudecodeして保存(&D)" );			//Sept. 11, 2000 JEPRO アクセスキー変更(U→D)

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp, "文字コード変換(&C)" );

			break;

//		case 5://case 3: (Oct. 22, 2000 JEPRO [移動]と[選択]を新設したため番号を2つシフトした)
		case 3://Feb. 19, 2001 JEPRO [移動]と[選択]を[編集]配下に移動したため番号を元に戻した
			m_CMenuDrawer.ResetContents();
			/* 「検索」メニュー */
			cMenuItems = ::GetMenuItemCount( hMenu );
			for( i = cMenuItems - 1; i >= 0; i-- ){
				bRet = ::DeleteMenu( hMenu, i, MF_BYPOSITION );
			}
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SEARCH_DIALOG	, "検索(&F)..." );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SEARCH_NEXT		, "次を検索(&N)" );				//Sept. 11, 2000 JEPRO "次"を"前"の前に移動
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SEARCH_PREV		, "前を検索(&P)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_REPLACE			, "置換(&R)..." );				//Oct. 7, 2000 JEPRO 下のセクションからここに移動
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SEARCH_CLEARMARK, "検索マークのクリア(&C)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_GREP			, "&Grep..." );					//Oct. 7, 2000 JEPRO 下からここに移動
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_JUMP			, "指定行へジャンプ(&J)..." );	//Sept. 11, 2000 jepro キャプションに「 ジャンプ」を追加
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_OUTLINE			, "アウトライン解析(&L)..." );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TAGJUMP			, "タグジャンプ(&T)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TAGJUMPBACK		, "タグジャンプバック(&B)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HOKAN			, "入力補完(&/)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_COMPARE			, "ファイル内容比較(&@)..." );
//	From Here Sept. 1, 2000 JEPRO	対括弧の検索をメニューに追加
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_BRACKETPAIR		, "対括弧の検索(&[)" );
//	To Here Sept. 1, 2000

			break;

//		case 6://case 4: (Oct. 22, 2000 JEPRO [移動]と[選択]を新設したため番号を2つシフトした)
		case 4://Feb. 19, 2001 JEPRO [移動]と[選択]を[編集]配下に移動したため番号を元に戻した
			m_CMenuDrawer.ResetContents();
			/* 「オプション」メニュー */
			cMenuItems = ::GetMenuItemCount( hMenu );
			for( i = cMenuItems - 1; i >= 0; i-- ){
				bRet = ::DeleteMenu( hMenu, i, MF_BYPOSITION );
			}
//	From Here Sept. 17, 2000 JEPRO
//	やはりWin標準に合わせてチェックマークだけで表示／非表示を判断するようにした方がいいので変更
			if ( FALSE == m_pShareData->m_Common.m_bMenuIcon ){
				pszLabel = "ツールバーを表示(&T)";				//これのみ表示
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SHOWTOOLBAR, pszLabel );	//これのみ
				pszLabel = "ファンクションキーを表示(&K)";		//これのみ表示
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SHOWFUNCKEY, pszLabel );	//これのみ
				pszLabel = "ステータスバーを表示(&S)";			//これのみ表示
//	To Here Sept.17, 2000 JEPRO
//	From Here Oct. 28, 2000 JEPRO
//	3つボタンのアイコンができたことに伴い表示／非表示のメッセージを変えるように再び変更
			}else{
				if( m_hwndToolBar == NULL ){
					pszLabel = "ツールバーを表示(&T)";			//これのみ表示
				}else{
					pszLabel = "表示中のツールバーを隠す(&T)";			//Sept. 9, 2000 jepro キャプションに「表示中の」を追加
				}
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SHOWTOOLBAR, pszLabel );	//これのみ
				if( NULL == m_CFuncKeyWnd.m_hWnd ){
					pszLabel = "ファンクションキーを表示(&K)";	//これのみ表示
				}else{
					pszLabel = "表示中のファンクションキーを隠す(&K)";	//Sept. 9, 2000 jepro キャプションに「表示中の」を追加
				}
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SHOWFUNCKEY, pszLabel );	//これのみ
				if( m_hwndStatusBar == NULL ){
					pszLabel = "ステータスバーを表示(&S)";		//これのみ表示
				}else{
					pszLabel = "表示中のステータスバーを隠す(&S)";		//Sept. 9, 2000 jepro キャプションに「表示中の」を追加
				}
			}
//	To Here Oct. 28, 2000

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SHOWSTATUSBAR, pszLabel );

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TYPE_LIST		, "タイプ別設定一覧(&L)..." );	//Sept. 13, 2000 JEPRO 設定より上に移動
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_OPTION_TYPE		, "タイプ別設定(&Y)..." );		//Sept. 13, 2000 JEPRO アクセスキー変更(S→Y)
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_OPTION			, "共通設定(&C)..." );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_FONT			, "フォント設定(&F)..." );		//Sept. 17, 2000 jepro キャプションに「設定」を追加
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WRAPWINDOWWIDTH , "現在のウィンドウ幅で折り返し(&W)" );	//Sept. 13, 2000 JEPRO アクセスキー付与	//Oct. 7, 2000 JEPRO WRAPWINDIWWIDTH を WRAPWINDOWWIDTH に変更
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );

			if( !m_pShareData->m_bRecordingKeyMacro ){	/* キーボードマクロの記録中 */
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_RECKEYMACRO	, "キーマクロの記録開始(&R)" );
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SAVEKEYMACRO, "キーマクロの保存(&M)" );
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_LOADKEYMACRO, "キーマクロの読み込み(&A)" );
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_EXECKEYMACRO, "キーマクロの実行(&D)" );
			}else{
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_RECKEYMACRO	, "キーマクロの記録終了(&R)" );
				::CheckMenuItem( hMenu, F_RECKEYMACRO, MF_BYCOMMAND | MF_CHECKED );
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SAVEKEYMACRO, "キーマクロの記録終了&&保存(&M)" );
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_LOADKEYMACRO, "キーマクロの記録終了&&読み込み(&A)" );
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_EXECKEYMACRO, "キーマクロの記録終了&&実行(&D)" );
			}
			
			//	From Here Sep. 14, 2001 genta
			//「カスタムメニュー」ポップアップ
			hMenuPopUp = ::CreateMenu();
			
			for( i = 0; i < MAX_CUSTMACRO; ++i ){
				MacroRec *mp = &m_pShareData->m_MacroTable[i];
				if( mp->m_bEnabled && mp->m_szFile[0] ){
					if(  mp->m_szName[0] ){
						m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_USERMACRO_0 + i, mp->m_szName );
					}
					else {
						m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_USERMACRO_0 + i, mp->m_szFile );
					}
				}
			}
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp , "登録済みマクロ(&B)" );
			//	To Here Sep. 14, 2001 genta

			if( m_pShareData->m_bRecordingKeyMacro ){	/* キーボードマクロの記録中 */
				::CheckMenuItem( hMenu, F_RECKEYMACRO, MF_BYCOMMAND | MF_CHECKED );
			}
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );

			//From Here Sept. 20, 2000 JEPRO 名称CMMANDをCOMMANDに変更
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_EXECCMMAND, "外部コマンド実行(&X)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_EXECCOMMAND, "外部コマンド実行(&X)" );	//Mar. 10, 2001 JEPRO 機能しないのでメニューから隠した	//Mar.21, 2001 JEPRO 標準出力しないで復活
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			//To Here Sept. 20, 2000

			//「カスタムメニュー」ポップアップ
			hMenuPopUp = ::CreateMenu();
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_MENU_RBUTTON, "右クリックメニュー(&R)" );	//Oct. 20, 2000 JEPRO 追加
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_1 , "カスタムメニュー&1 " );		//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_2 , "カスタムメニュー&2 " );		//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_3 , "カスタムメニュー&3 " );		//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_4 , "カスタムメニュー&4 " );		//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_5 , "カスタムメニュー&5 " );		//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_6 , "カスタムメニュー&6 " );		//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_7 , "カスタムメニュー&7 " );		//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_8 , "カスタムメニュー&8 " );		//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_9 , "カスタムメニュー&9 " );		//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_10, "カスタムメニュー10(&A)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_11, "カスタムメニュー11(&B)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_12, "カスタムメニュー12(&C)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_13, "カスタムメニュー13(&D)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_14, "カスタムメニュー14(&E)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_15, "カスタムメニュー15(&F)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_16, "カスタムメニュー16(&G)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_17, "カスタムメニュー17(&H)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_18, "カスタムメニュー18(&I)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_19, "カスタムメニュー19(&J)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_20, "カスタムメニュー20(&K)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_21, "カスタムメニュー21(&L)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_22, "カスタムメニュー22(&M)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_23, "カスタムメニュー23(&N)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, F_CUSTMENU_24, "カスタムメニュー24(&O)" );	//Sept. 13, 2000 JEPRO アクセスキー付与

			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp , "カスタムメニュー(&U)" );

//		m_pShareData->m_hwndRecordingKeyMacro = NULL;	/* キーボードマクロを記録中のウィンドウ */

			break;

//		case 7://case 5: (Oct. 22, 2000 JEPRO [移動]と[選択]を新設したため番号を2つシフトした)
		case 5://Feb. 19, 2001 JEPRO [移動]と[選択]を[編集]配下に移動したため番号を元に戻した
			m_CMenuDrawer.ResetContents();
			/* 「ウィンドウ」メニュー */
			cMenuItems = ::GetMenuItemCount( hMenu );
			for( i = cMenuItems - 1; i >= 0; i-- ){
				bRet = ::DeleteMenu( hMenu, i, MF_BYPOSITION );
			}

			if( 1 == m_cEditDoc.m_cSplitterWnd.m_nAllSplitRows ){
				pszLabel = "上下に分割(&-)";	//Oct. 7, 2000 JEPRO アクセスキーを変更(T→-)
			}else{
				pszLabel = "上下分割の解除(&-)";
			}
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SPLIT_V , pszLabel );

			if( 1 == m_cEditDoc.m_cSplitterWnd.m_nAllSplitCols ){
				pszLabel = "左右に分割(&I)";	//Oct. 7, 2000 JEPRO アクセスキーを変更(Y→I)
			}else{
				pszLabel = "左右分割の解除(&I)";
			}
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SPLIT_H , pszLabel );
//	From Here Sept. 17, 2000 JEPRO	縦横分割の場合も状態によってメニューメッセージが変わるように変更
			if( (1 < m_cEditDoc.m_cSplitterWnd.m_nAllSplitRows) && (1 < m_cEditDoc.m_cSplitterWnd.m_nAllSplitCols) ){
				pszLabel = "縦横分割の解除(&S)";	//Feb. 18, 2001 JEPRO アクセスキー変更(Q→S)
			}else{
				pszLabel = "縦横に分割(&S)";	//Sept. 17, 2000 jepro 説明に「に」を追加	//Oct. 7, 2000 JEPRO アクセスキーを変更(S→Q)	//Feb. 18, 2001 JEPRO アクセスキーを元に戻した(Q→S)
			}
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SPLIT_VH , pszLabel );
//	To Here Sept. 17, 2000
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_SPLIT_VH, "縦横に分割(&S)" );				//Sept. 17, 2000 JEPRO 上の変更によりこの行はコメントアウト
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );	/* セパレータ */
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WINCLOSE		, "閉じる(&C)" );			//Feb. 18, 2001 JEPRO アクセスキー変更(O→C)
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WIN_CLOSEALL	, "すべて閉じる(&Q)" );		//Oct. 17, 2000 JEPRO 名前を変更(F_FILECLOSEALL→F_WIN_CLOSEALL)	//Feb. 18, 2001 JEPRO アクセスキー変更(L→Q)
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_NEXTWINDOW		, "次のウィンドウ(&N)" );	//Sept. 11, 2000 JEPRO "次"を"前"の前に移動
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_PREVWINDOW		, "前のウィンドウ(&P)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );	/* セパレータ */
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_CASCADE			, "重ねて表示(&E)" );		//Oct. 7, 2000 JEPRO アクセスキー変更(C→E)
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TILE_V			, "上下に並べて表示(&H)" );	//Sept. 13, 2000 JEPRO 分割に合わせてメニューの左右と上下を入れ替えた //Oct. 7, 2000 JEPRO アクセスキー変更(V→H)
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_TILE_H			, "左右に並べて表示(&T)" );	//Oct. 7, 2000 JEPRO アクセスキー変更(H→T)
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );	/* セパレータ */
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_MAXIMIZE_V		, "縦方向に最大化(&X)" );	//Sept. 13, 2000 JEPRO アクセスキー付与
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_MAXIMIZE_H		, "横方向に最大化(&Y)" );	//2001.02.10 by MIK
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_MINIMIZE_ALL	, "すべて最小化(&M)" );		//Sept. 17, 2000 jepro 説明の「全て」を「すべて」に統一
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );	/* セパレータ */				//Oct. 22, 2000 JEPRO 下の「再描画」復活に伴いセパレータを追加
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_REDRAW			, "再描画(&R)" );			//Oct. 22, 2000 JEPRO コメントアウトされていたのを復活させた
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );	/* セパレータ */
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_WIN_OUTPUT		, "アウトプット(&U)" );		//Sept. 13, 2000 JEPRO アクセスキー変更(O→U)

			/* 現在開いている編集窓のリストをメニューにする */
			nRowNum = m_cShareData.GetOpenedWindowArr( &pEditNodeArr, TRUE );
			if( nRowNum > 0 ){
				/* セパレータ */
				m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
				for( i = 0; i < nRowNum; ++i ){
					/* トレイからエディタへの編集ファイル名要求通知 */
					::SendMessage( pEditNodeArr[i].m_hWnd, MYWM_GETFILEINFO, 0, 0 );
//					pfi = (FileInfo*)m_pShareData->m_szWork;
					pfi = (FileInfo*)&m_pShareData->m_FileInfo_MYWM_GETFILEINFO;
					if( pfi->m_bIsGrep ){
						/* データを指定バイト数以内に切り詰める */
						CMemory		cmemDes;
						int			nDesLen;
						const char*	pszDes;
						LimitStringLengthB( pfi->m_szGrepKey, lstrlen( pfi->m_szGrepKey ), 64, cmemDes );
						pszDes = cmemDes.GetPtr( NULL );
						nDesLen = lstrlen( pszDes );
//	From Here Oct. 4, 2000 JEPRO commented out & modified	開いているファイル数がわかるように履歴とは違って1から数える
//						wsprintf( szMemu, "&%d 【Grep】\"%s%s\"", ((i + 1) <= 9)? (i + 1):9,
//							pszDes, ( (int)lstrlen( pfi->m_szGrepKey ) > nDesLen ) ? "・・・":""
//						);
//					}else
//					if( pEditNodeArr[i].m_hWnd == m_pShareData->m_hwndDebug ){
//						wsprintf( szMemu, "&%d アウトプット", ((i + 1) <= 9)? (i + 1):9 );
//					}else{
//						wsprintf( szMemu, "&%d %s %s", ((i + 1) <= 9)? (i + 1):9,
//							(0 < lstrlen( pfi->m_szPath ))?pfi->m_szPath:"(無題)",
//							pfi->m_bIsModified ? "*":" "
//						);
//		i >= 10 + 26 の時の考慮を省いた(に近い)が開くファイル数が36個を越えることはまずないので事実上OKでしょう
						wsprintf( szMemu, "&%c 【Grep】\"%s%s\"", ((1 + i) <= 9)?('1' + i):('A' + i - 9),
							pszDes, ( (int)lstrlen( pfi->m_szGrepKey ) > nDesLen ) ? "・・・":""
						);
					}else
					if( pEditNodeArr[i].m_hWnd == m_pShareData->m_hwndDebug ){
//		i >= 10 + 26 の時の考慮を省いた(に近い)が出力ファイル数が36個を越えることはまずないので事実上OKでしょう
						wsprintf( szMemu, "&%c アウトプット", ((1 + i) <= 9)?('1' + i):('A' + i - 9) );

					}else{
//		From Here Jan. 23, 2001 JEPRO
//		ファイル名やパス名に'&'が使われているときに履歴等でキチンと表示されない問題を修正(&を&&に置換するだけ)
//<----- From Here Commented out
#if 0
//		i >= 10 + 26 の時の考慮を省いた(に近い)が出力ファイル数が36個を越えることはまずないので事実上OKでしょう
						wsprintf( szMemu, "&%c %s %s", ((1 + i) <= 9)?('1' + i):('A' + i - 9),
							(0 < lstrlen( pfi->m_szPath ))?pfi->m_szPath:"(無題)",
							pfi->m_bIsModified ? "*":" "
						);
#endif
//-----> To Here Commented out
//<----- From Here Added
						char	szFile2[_MAX_PATH + 3];	//	'+1'かな？ ようわからんので多めにしとこ。わかる人修正報告ください！
						if( 0 == lstrlen( pfi->m_szPath ) ){
							strcpy( szFile2, "(無題)" );
						}else{
							char	*p;
							strcpy( szFile2, pfi->m_szPath );
							if( (p = strchr( szFile2, '&' )) != NULL ){
								char	buf[_MAX_PATH + 3];	//	'+1'かな？ ようわからんので多めにしとこ。わかる人修正報告ください！
								do {
									*p = '\0';
									strcpy( buf, p + strlen("&") );
									strcat( szFile2, "&&" );
									strcat( szFile2, buf );
									p = strchr( p + strlen("&&"), '&' );
								} while ( p != NULL );
							}
						}
						wsprintf( szMemu, "&%c %s %s", ((1 + i) <= 9)?('1' + i):('A' + i - 9),
							szFile2,
							pfi->m_bIsModified ? "*":" "
						);
//-----> To Here Added
//		To Here Jan. 23, 2001

//	To Here Oct. 4, 2000
						if( 0 != pfi->m_nCharCode ){		/* 文字コード種別 */
							switch( pfi->m_nCharCode ){
							case CODE_JIS:		/* JIS */
								strcat( szMemu, "  [JIS]" );
								break;
							case CODE_EUC:		/* EUC */
								strcat( szMemu, "  [EUC]" );
								break;
							case CODE_UNICODE:	/* Unicode */
								strcat( szMemu, "  [Unicode]" );
								break;
							case CODE_UTF8:		/* UTF-8 */
								strcat( szMemu, "  [UTF-8]" );
								break;
							case CODE_UTF7:		/* UTF-7 */
								strcat( szMemu, "  [UTF-7]" );
								break;
							}
						}
					}
					m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, IDM_SELWINDOW + pEditNodeArr[i].m_nIndex, szMemu );
					if( m_hWnd == pEditNodeArr[i].m_hWnd ){
						::CheckMenuItem( hMenu, IDM_SELWINDOW + pEditNodeArr[i].m_nIndex, MF_BYCOMMAND | MF_CHECKED );
					}
				}
				delete [] pEditNodeArr;
			}

			break;

//		case 8://case 6: (Oct. 22, 2000 JEPRO [移動]と[選択]を新設したため番号を2つシフトした)
		case 6://Feb. 19, 2001 JEPRO [移動]と[選択]を[編集]配下に移動したため番号を元に戻した
			m_CMenuDrawer.ResetContents();
			/* 「ヘルプ」メニュー */
			cMenuItems = ::GetMenuItemCount( hMenu );
			for( i = cMenuItems - 1; i >= 0; i-- ){
				bRet = ::DeleteMenu( hMenu, i, MF_BYPOSITION );
			}
//Sept. 15, 2000→Nov. 25, 2000 JEPRO //ショートカットキーがうまく働かないので殺してあった下の2行を修正・復活
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, IDM_HELP_CONTENTS	, "目次(&O)" );				//Sept. 7, 2000 jepro キャプションを「ヘルプ目次」から変更	Oct. 13, 2000 JEPRO アクセスキーを「トレイ右ボタン」のために変更(C→O)
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HELP_CONTENTS , "目次(&O)" );				//Sept. 7, 2000 jepro キャプションを「ヘルプ目次」から変更	Oct. 13, 2000 JEPRO アクセスキーを「トレイ右ボタン」のために変更(C→O)
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, IDM_HELP_SEARCH	, "トピックの検索(&S)" );	//Sept. 7, 2000 jepro キャプションを「ヘルプトピックの検索」から変更
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_HELP_SEARCH	,	 "キーワード検索(&S)" );	//Sept. 7, 2000 jepro キャプションを「ヘルプトピックの検索」から変更 //Nov. 25, 2000 jepro「トピックの」→「キーワード」に変更
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_MENU_ALLFUNC	, "コマンド一覧(&M)" );		//Oct. 13, 2000 JEPRO アクセスキーを「トレイ右ボタン」のために変更(L→M)
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_EXTHELP1		, "外部ヘルプ１(&E)" );		//Sept. 7, 2000 JEPRO このメニューの順番をトップから下に移動
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_EXTHTMLHELP		, "外部HTMLヘルプ(&H)" );	//Sept. 7, 2000 JEPRO このメニューの順番を２番目から下に移動
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
//			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, IDM_ABOUT			, "バージョン情報(&A)" );
			m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING, F_ABOUT			, "バージョン情報(&A)" );	//Dec. 25, 2000 JEPRO F_に変更
			break;
		}
	}

end_of_func_IsEnable:;
	cMenuItems = ::GetMenuItemCount( hMenu );
	for (nPos = 0; nPos < cMenuItems; nPos++) {
		id = ::GetMenuItemID(hMenu, nPos);
		/* 機能が利用可能か調べる */
		if( IsFuncEnable( &m_cEditDoc, m_pShareData, id ) ){
			fuFlags = MF_BYCOMMAND | MF_ENABLED;
		}else{
			fuFlags = MF_BYCOMMAND | MF_GRAYED;
		}
		::EnableMenuItem(hMenu, id, fuFlags);

		/* 機能がチェック状態か調べる */
		if( IsFuncChecked( &m_cEditDoc, m_pShareData, id ) ){
			fuFlags = MF_BYCOMMAND | MF_CHECKED;
		}else{
			fuFlags = MF_BYCOMMAND | MF_UNCHECKED;
		}
		::CheckMenuItem(hMenu, id, fuFlags);

	}
	return;
}




/* ファイルがドロップされた */
void CEditWnd::OnDropFiles( HDROP hDrop )
{
		POINT		pt;
		WORD		cFiles, i;
		char		szFile[_MAX_PATH + 1];
		char		szWork[_MAX_PATH + 1];
		BOOL		bOpened;
		FileInfo*	pfi;
		HWND		hWndOwner;

//		/* 編集ウィンドウの上限チェック */
//		if( m_pShareData->m_nEditArrNum + 1 > MAX_EDITWINDOWS ){
//				char szMsg[512];
//				sprintf( szMsg, "編集ウィンドウ数の上限は%dです。\nこれ以上は同時に開けません。", MAX_EDITWINDOWS );
//				::MessageBox( NULL, szMsg, GSTR_APPNAME, MB_OK );
//				return;
//		}

		::DragQueryPoint( hDrop, &pt );
		cFiles = ::DragQueryFile( hDrop, 0xFFFFFFFF, (LPSTR) NULL, 0);
		/* ファイルをドロップしたときは閉じて開く */
		if( m_pShareData->m_Common.m_bDropFileAndClose ){
			cFiles = 1;
		}
		/* 一度にドロップ可能なファイル数 */
		if( cFiles > m_pShareData->m_Common.m_nDropFileNumMax ){
				cFiles = m_pShareData->m_Common.m_nDropFileNumMax;
		}
		for( i = 0; i < cFiles; i++ ) {
				::DragQueryFile( hDrop, i, szFile, sizeof(szFile) );
				/* ショートカット(.lnk)の解決 */
				if( TRUE == ResolveShortcutLink( NULL, szFile, szWork ) ){
					strcpy( szFile, szWork );
				}
				/* ロングファイル名を取得する */
				if( TRUE == ::GetLongFileName( szFile, szWork ) ){
						strcpy( szFile, szWork );
				}

				/* 指定ファイルが開かれているか調べる */
				if( m_cShareData.IsPathOpened( szFile, &hWndOwner ) ){
					::SendMessage( hWndOwner, MYWM_GETFILEINFO, 0, 0 );
//					pfi = (FileInfo*)m_pShareData->m_szWork;
					pfi = (FileInfo*)&m_pShareData->m_FileInfo_MYWM_GETFILEINFO;
					/* アクティブにする */
					ActivateFrameWindow( hWndOwner );
					/* MRUリストへの登録 */
					m_cShareData.AddMRUList( pfi );
				}else{
						/* 変更フラグがオフで、ファイルを読み込んでいない場合 */
						if( !m_cEditDoc.m_bIsModified &&
							0 == strlen( m_cEditDoc.m_szFilePath )	/* 現在編集中のファイルのパス */
						){
								/* ファイル読み込み */
								m_cEditDoc.FileRead(
										szFile,
										&bOpened,
										CODE_AUTODETECT,	/* 文字コード自動判別 */
										FALSE,				/* 読み取り専用か */
										TRUE				/* 文字コード変更時の確認をするかどうか */
								);
								hWndOwner = m_hWnd;
								/* アクティブにする */
								ActivateFrameWindow( hWndOwner );
						}else{
								/* ファイルをドロップしたときは閉じて開く */
								if( m_pShareData->m_Common.m_bDropFileAndClose ){
										/* ファイルを閉じるときのMRU登録 & 保存確認 & 保存実行 */
										if( m_cEditDoc.OnFileClose() ){
												/* 既存データのクリア */
												m_cEditDoc.Init();

												/* 全ビューの初期化 */
												m_cEditDoc.InitAllView();

												/* 親ウィンドウのタイトルを更新 */
												m_cEditDoc.SetParentCaption();

												/* ファイル読み込み */
														m_cEditDoc.FileRead(
														szFile,
														&bOpened,
														CODE_AUTODETECT,	/* 文字コード自動判別 */
														FALSE,				/* 読み取り専用か */
														TRUE				/* 文字コード変更時の確認をするかどうか */
												);
												hWndOwner = m_hWnd;
												/* アクティブにする */
												ActivateFrameWindow( hWndOwner );
										}
										goto end_of_drop_query;
								}else{
										/* 編集ウィンドウの上限チェック */
										if( m_pShareData->m_nEditArrNum + 1 > MAX_EDITWINDOWS ){
												char szMsg[512];
												wsprintf( szMsg, "編集ウィンドウ数の上限は%dです。\nこれ以上は同時に開けません。", MAX_EDITWINDOWS );
												::MessageBox( NULL, szMsg, GSTR_APPNAME, MB_OK );
												::DragFinish( hDrop );
												return;
										}
										char	szFile2[_MAX_PATH + 3];
										if( strchr( szFile, ' ' ) ){
												wsprintf( szFile2, "\"%s\"", szFile );
												strcpy( szFile, szFile2 );
										}
										/* 新たな編集ウィンドウを起動 */
										CEditApp::OpenNewEditor(
												m_hInstance,
												m_hWnd,
												szFile,
												CODE_AUTODETECT,	/* 文字コード自動判別 */
												FALSE				/* 読み取り専用か */
										);
								}
						}
				}
		}
end_of_drop_query:;
		::DragFinish( hDrop );
		return;
}


/* タイマーの処理 */
void CEditWnd::OnTimer(
	HWND		hwnd,		// handle of window for timer messages
	UINT		uMsg,		// WM_TIMER message
	UINT		idEvent,	// timer identifier
	DWORD		dwTime 		// current system time
)
{
//	HWND	hwndFocus;
//	hwndFocus = ::GetFocus();
//	MYTRACE( "===hwndFocus == %xh ==\n", hwndFocus );
	static	int	nLoopCount = 0;
	int			i;
	TBBUTTON	tbb;
	nLoopCount++;
	if( 10 < nLoopCount ){
		nLoopCount = 0;
	}

	/* ツールバーの状態更新 */
	if( NULL != m_hwndToolBar ){
		for( i = 0; i < m_pShareData->m_Common.m_nToolBarButtonNum; ++i ){
			tbb = m_cShareData.m_tbMyButton[m_pShareData->m_Common.m_nToolBarButtonIdxArr[i]];
			/* 機能が利用可能か調べる */
			::PostMessage(
				m_hwndToolBar, TB_ENABLEBUTTON, tbb.idCommand,
				(LPARAM) MAKELONG( (IsFuncEnable( &m_cEditDoc, m_pShareData, tbb.idCommand ) ) , 0 )
			);
			/* 機能がチェック状態か調べる */
			::PostMessage(
				m_hwndToolBar, TB_CHECKBUTTON, tbb.idCommand,
				(LPARAM) MAKELONG( IsFuncChecked( &m_cEditDoc, m_pShareData, tbb.idCommand ), 0 )
			);
		}
	}

	if( nLoopCount == 0 ){
		/* ファイルのタイムスタンプのチェック処理 */
		m_cEditDoc.CheckFileTimeStamp() ;
	}

	m_cEditDoc.CheckAutoSave();
	return;
}

/* 機能がチェック状態か調べる */
int CEditWnd::IsFuncChecked( CEditDoc* pcEditDoc, DLLSHAREDATA*	pShareData, int nId )
{
//#ifdef _DEBUG
//	return TRUE;
//#endif
	CEditWnd* pCEditWnd;
	pCEditWnd = ( CEditWnd* )::GetWindowLong( pcEditDoc->m_hwndParent, GWL_USERDATA );

	/* 印刷プレビューモードか */
	if( TRUE == pcEditDoc->m_bPrintPreviewMode ){
		return FALSE;
	}
	switch( nId ){
	case F_FILE_REOPEN_SJIS:
		if( CODE_SJIS == pcEditDoc->m_nCharCode ){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_FILE_REOPEN_JIS:
		if( CODE_JIS == pcEditDoc->m_nCharCode ){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_FILE_REOPEN_EUC:
		if( CODE_EUC == pcEditDoc->m_nCharCode ){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_FILE_REOPEN_UNICODE:
		if( CODE_UNICODE == pcEditDoc->m_nCharCode ){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_FILE_REOPEN_UTF8:
		if( CODE_UTF8 == pcEditDoc->m_nCharCode ){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_FILE_REOPEN_UTF7:
		if( CODE_UTF7 == pcEditDoc->m_nCharCode ){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_RECKEYMACRO:	/* キーマクロの記録開始／終了 */
		if( pShareData->m_bRecordingKeyMacro ){	/* キーボードマクロの記録中 */
			if( pShareData->m_hwndRecordingKeyMacro == pcEditDoc->m_hwndParent ){	/* キーボードマクロを記録中のウィンドウ */
				return TRUE;
			}else{
				return FALSE;
			}
		}else{
			return FALSE;
		}
//	case F_SAVEKEYMACRO:	/* キーマクロの保存 */
//	case F_EXECKEYMACRO:	/* キーマクロの実行 */
//		if( pShareData->m_bRecordingKeyMacro ){	/* キーボードマクロの記録中 */
//			if( pShareData->m_hwndRecordingKeyMacro == pcEditDoc->m_hwndParent ){	/* キーボードマクロを記録中のウィンドウ */
//				return TRUE;
//			}else{
//				return FALSE;
//			}
//		}else{
//			if( 0 < pShareData->m_CKeyMacroMgr.m_nKeyMacroDataArrNum ){
//				return TRUE;
//			}else{
//				return FALSE;
//			}
//		}
//	case F_LOADKEYMACRO:	/* キーマクロの読み込み */
//		if( pShareData->m_bRecordingKeyMacro ){	/* キーボードマクロの記録中 */
//			if( pShareData->m_hwndRecordingKeyMacro == pcEditDoc->m_hwndParent ){	/* キーボードマクロを記録中のウィンドウ */
//				return TRUE;
//			}else{
//				return FALSE;
//			}
//		}else{
//			return TRUE;
//		}
	case F_SHOWTOOLBAR:
		if( pCEditWnd->m_hwndToolBar != NULL ){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_SHOWFUNCKEY:
		if( pCEditWnd->m_CFuncKeyWnd.m_hWnd != NULL ){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_SHOWSTATUSBAR:
		if( pCEditWnd->m_hwndStatusBar != NULL ){
			return TRUE;
		}else{
			return FALSE;
		}

	}
	return FALSE;
}






/* 機能が利用可能か調べる */
int CEditWnd::IsFuncEnable( CEditDoc* pcEditDoc, DLLSHAREDATA* pShareData, int nId )
{
	/* 印刷プレビューモードか */
	if( TRUE == pcEditDoc->m_bPrintPreviewMode ){
		return FALSE;
	}


//#ifdef _DEBUG
//	return TRUE;
//#endif
	if( pcEditDoc->IsModificationForbidden( nId ) )
		return FALSE;

	switch( nId ){
	case F_RECKEYMACRO:	/* キーマクロの記録開始／終了 */
		if( pShareData->m_bRecordingKeyMacro ){	/* キーボードマクロの記録中 */
			if( pShareData->m_hwndRecordingKeyMacro == pcEditDoc->m_hwndParent ){	/* キーボードマクロを記録中のウィンドウ */
				return TRUE;
			}else{
				return FALSE;
			}
		}else{
			return TRUE;
		}
	case F_SAVEKEYMACRO:	/* キーマクロの保存 */
	case F_EXECKEYMACRO:	/* キーマクロの実行 */
		if( pShareData->m_bRecordingKeyMacro ){	/* キーボードマクロの記録中 */
			if( pShareData->m_hwndRecordingKeyMacro == pcEditDoc->m_hwndParent ){	/* キーボードマクロを記録中のウィンドウ */
				return TRUE;
			}else{
				return FALSE;
			}
		}else{
			if( 0 < pShareData->m_CKeyMacroMgr.m_nKeyMacroDataArrNum ){
				return TRUE;
			}else{
				return FALSE;
			}
		}
	case F_LOADKEYMACRO:	/* キーマクロの読み込み */
		if( pShareData->m_bRecordingKeyMacro ){	/* キーボードマクロの記録中 */
			if( pShareData->m_hwndRecordingKeyMacro == pcEditDoc->m_hwndParent ){	/* キーボードマクロを記録中のウィンドウ */
				return TRUE;
			}else{
				return FALSE;
			}
		}else{
			return TRUE;
		}

	case F_SEARCH_CLEARMARK:	//検索マークのクリア
		/* 検索文字列のマーク */
		if( pcEditDoc->m_cEditViewArr[pcEditDoc->m_nActivePaneIndex].m_bCurSrchKeyMark ){
			return TRUE;
		}else{
			return FALSE;
		}

	case F_COMPARE:	/* ファイル内容比較 */
		if( 2 <= pShareData->m_nEditArrNum ){
			return TRUE;
		}else{
			return FALSE;
		}

	case F_BEGIN_BOX:	//矩形範囲選択開始
		if( TRUE == pShareData->m_Common.m_bFontIs_FIXED_PITCH ){	/* 現在のフォントは固定幅フォントである */
			return TRUE;
		}else{
			return FALSE;
		}
	case F_PASTEBOX:
		/* クリップボードから貼り付け可能か？ */
		if( pcEditDoc->IsEnablePaste() &&
			TRUE == pShareData->m_Common.m_bFontIs_FIXED_PITCH
		){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_PASTE:
		/* クリップボードから貼り付け可能か？ */
		if( pcEditDoc->IsEnablePaste() ){
			return TRUE;
		}else{
			return FALSE;
		}

	case F_FILENEW:	/* 新規作成 */
	case F_GREP:	/* Grep */
		/* 編集ウィンドウの上限チェック */
		if( pShareData->m_nEditArrNum + 1 > MAX_EDITWINDOWS ){
			return FALSE;
		}else{
			return TRUE;
		}

	case F_FILESAVE:	/* 上書き保存 */
		if( !pcEditDoc->m_bReadOnly ){	/* 読み取り専用モード */
			if( TRUE == pcEditDoc->m_bIsModified ){	/* 変更フラグ */
				return TRUE;
			}else{
				/* 無変更でも上書きするか */
				if( FALSE == pShareData->m_Common.m_bEnableUnmodifiedOverwrite ){
					return FALSE;
				}else{
					return TRUE;
				}
			}
		}else{
			return FALSE;
		}
	case F_COPYLINES:				//選択範囲内全行コピー
	case F_COPYLINESASPASSAGE:		//選択範囲内全行引用符付きコピー
	case F_COPYLINESWITHLINENUMBER:	//選択範囲内全行行番号付きコピー
		if( pcEditDoc->IsTextSelected( ) ){/* テキストが選択されているか */
			return TRUE;
		}else{
			return FALSE;
		}

//	case F_COPY:					/* コピー */
//	case F_CUT:						/* 切り取り */
	case F_DELETE:					/* 削除 */
	case F_TOLOWER:					/* 英大文字→英小文字 */
	case F_TOUPPER:					/* 英小文字→英大文字 */
	case F_TOHANKAKU:				/* 全角→半角 */
	case F_TOZENEI:					/* 半角英数→全角英数 */			//July. 30, 2001 Misaka
	case F_TOZENKAKUKATA:			/* 半角＋全ひら→全角・カタカナ */	//Sept. 17, 2000 jepro 説明を「半角→全角カタカナ」から変更
	case F_TOZENKAKUHIRA:			/* 半角＋全カタ→全角・ひらがな */	//Sept. 17, 2000 jepro 説明を「半角→全角ひらがな」から変更
	case F_HANKATATOZENKAKUKATA:	/* 半角カタカナ→全角カタカナ */
	case F_HANKATATOZENKAKUHIRA:	/* 半角カタカナ→全角ひらがな */
	case F_TABTOSPACE:				/* TAB→空白 */
	case F_SPACETOTAB:				/* 空白→TAB */  //#### Stonee, 2001/05/27
	case F_CODECNV_AUTO2SJIS:		/* 自動判別→SJISコード変換 */
	case F_CODECNV_EMAIL:			/* E-Mail(JIS→SJIS)コード変換 */
	case F_CODECNV_EUC2SJIS:		/* EUC→SJISコード変換 */
	case F_CODECNV_UNICODE2SJIS:	//Unicode→SJISコード変換
	case F_CODECNV_UTF82SJIS:		/* UTF-8→SJISコード変換 */
	case F_CODECNV_UTF72SJIS:		/* UTF-7→SJISコード変換 */
	case F_CODECNV_SJIS2JIS:		/* SJIS→JISコード変換 */
	case F_CODECNV_SJIS2EUC:		/* SJIS→EUCコード変換 */
	case F_CODECNV_SJIS2UTF8:		/* SJIS→UTF-8コード変換 */
	case F_CODECNV_SJIS2UTF7:		/* SJIS→UTF-7コード変換 */
	case F_BASE64DECODE:			/* Base64デコードして保存 */
	case F_UUDECODE:				//uudecodeして保存	//Oct. 17, 2000 jepro 説明を「選択部分をUUENCODEデコード」から変更

		/* テキストが選択されているか */
		if( pcEditDoc->IsTextSelected( ) ){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_SELECTWORD:	/* 現在位置の単語選択 */
	case F_CUT_LINE:	//行切り取り(折り返し単位)
	case F_DELETE_LINE:	//行削除(折り返し単位)
		/* テキストが選択されているか */
		if( pcEditDoc->IsTextSelected( ) ){
			return FALSE;
		}else{
			return TRUE;
		}
	case F_UNDO:
		/* Undo(元に戻す)可能な状態か？ */
		if( pcEditDoc->IsEnableUndo() ){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_REDO:
		/* Redo(やり直し)可能な状態か？ */
		if( pcEditDoc->IsEnableRedo() ){
			return TRUE;
		}else{
			return FALSE;
		}

	case F_COPYPATH:
	case F_COPYTAG:
	case F_OPEN_HfromtoC:				//同名のC/C++ヘッダ(ソース)を開く	//Feb. 7, 2001 JEPRO 追加
	case F_OPEN_HHPP:					//同名のC/C++ヘッダファイルを開く	//Feb. 9, 2001 jepro「.cまたは.cppと同名の.hを開く」から変更
	case F_OPEN_CCPP:					//同名のC/C++ソースファイルを開く	//Feb. 9, 2001 jepro「.hと同名の.c(なければ.cpp)を開く」から変更
	case F_PLSQL_COMPILE_ON_SQLPLUS:	/* Oracle SQL*Plusで実行 */
	case F_BROWSE:						//ブラウズ
	case F_PROPERTY_FILE:
		/* 現在編集中のファイルのパス名をクリップボードにコピーできるか */
//		if( 0 < lstrlen( pcEditDoc->m_szFilePath ) ){
		if( '\0' != pcEditDoc->m_szFilePath[0] ){
			return TRUE;
		}else{
			return FALSE;
		}
	case F_JUMPPREV:	//	移動履歴: 前へ
		if( pcEditDoc->ActiveView().m_cHistory->CheckPrev() )
			return TRUE;
		else
			return FALSE;
	case F_JUMPNEXT:	//	移動履歴: 次へ
		if( pcEditDoc->ActiveView().m_cHistory->CheckNext() )
			return TRUE;
		else
			return FALSE;

// 	case F_TAGJUMP:
//		/* タグジャンプできるか */
//		if( pcEditDoc->m_cEditViewArr[pcEditDoc->m_nActivePaneIndex].HandleCommand( F_TAGJUMP, FALSE, (LPARAM)TRUE, 0, 0, 0 ) ){
//			return TRUE;
//		}else{
//			return FALSE;
//		}
// 	case F_TAGJUMPBACK:
//		/* タグジャンプバックできるか */
//		if( pcEditDoc->m_cEditViewArr[pcEditDoc->m_nActivePaneIndex].HandleCommand( F_TAGJUMPBACK, FALSE, (LPARAM)TRUE, 0, 0, 0 ) ){
//			return TRUE;
//		}else{
//			return FALSE;
//		}
//	case F_OPEN_HHPP:		/* 同名のC/C++ヘッダファイルを開く */
//		if( pcEditDoc->m_cEditViewArr[pcEditDoc->m_nActivePaneIndex].HandleCommand( F_OPEN_HHPP, FALSE, (LPARAM)TRUE, 0, 0, 0 ) ){
//			return TRUE;
//		}else{
//			return FALSE;
//		}
// 	case F_OPEN_CCPP:		/* 同名のC/C++ソースファイルを開く */
//		if( pcEditDoc->m_cEditViewArr[pcEditDoc->m_nActivePaneIndex].HandleCommand( F_OPEN_CCPP, FALSE, (LPARAM)TRUE, 0, 0, 0 ) ){
//			return TRUE;
//		}else{
//			return FALSE;
//		}
//	case F_OUTLINE:			/* C/C++関数一覧 */
//		if( pcEditDoc->m_cEditViewArr[pcEditDoc->m_nActivePaneIndex].HandleCommand( F_OUTLINE, FALSE, (LPARAM)TRUE, 0, 0, 0 ) ){
//			return TRUE;
//		}else{
//			return FALSE;
//		}
	}
	return TRUE;
}

//#ifdef _DEBUG
	/* デバッグモニタモードに設定 */
	void CEditWnd::SetDebugModeON( void )
	{
		if( NULL != m_pShareData->m_hwndDebug ){
			if( CShareData::IsEditWnd( m_pShareData->m_hwndDebug ) ){
				return;
			}
		}
		m_pShareData->m_hwndDebug = m_hWnd;
		m_cEditDoc.m_bDebugMode = TRUE;

// 2001/06/23 N.Nakatani アウトプット窓への出力テキストの追加F_ADDTAILが抑止されるのでとりあえず読み取り専用モードは辞めました
//		m_cEditDoc.m_bReadOnly = TRUE;		/* 読み取り専用モード */
		m_cEditDoc.m_bReadOnly = FALSE;		/* 読み取り専用モード */
		/* 親ウィンドウのタイトルを更新 */
		m_cEditDoc.SetParentCaption();
		return;
	}
//#endif


/* 機能IDに対応するヘルプコンテキスト番号を返す */
// Modified by Stonee, 2001/02/23
// etc_uty.cpp内に中身を移動
int CEditWnd::FuncID_To_HelpContextID( int nFuncID )
{
	return ::FuncID_To_HelpContextID(nFuncID);
}


/* メニューアイテムに対応するヘルプを表示 */
void CEditWnd::OnHelp_MenuItem( HWND hwndParent, int nFuncID )
{
	char	szHelpFile[_MAX_PATH + 1];
	int		nHelpContextID;

	/* ヘルプファイルのフルパスを返す */
	::GetHelpFilePath( szHelpFile );

	/* 機能IDに対応するヘルプコンテキスト番号を返す */
	nHelpContextID = FuncID_To_HelpContextID( nFuncID );
	if( 0 != nHelpContextID ){
		::WinHelp( hwndParent, szHelpFile, HELP_CONTEXT, nHelpContextID );
	}
	return;
}



/* 印刷プレビュー 操作バー ダイアログのメッセージ処理 */
BOOL CEditWnd::DispatchEvent_PPB(
	HWND				hwndDlg,	// handle to dialog box
	UINT				uMsg,		// message
	WPARAM				wParam,		// first message parameter
	LPARAM				lParam 		// second message parameter
)
{
	WORD				wNotifyCode;
	WORD				wID;
	HWND				hwndCtl;
//	char*				pszMsg;
	CMemory				cMemBuf;
//	WIN32_FIND_DATA		wfd;
//	char				szFile[_MAX_PATH];
//	SYSTEMTIME			systimeL;
	char				szHelpFile[_MAX_PATH];
//	CDlgPrintSetting	cDlgPrintSetting;
//	int					nWork;



	switch( uMsg ){

	case WM_INITDIALOG:
		::SetWindowLong( hwndDlg, DWL_USER, (LONG)lParam );
		return TRUE;
	case WM_COMMAND:
		wNotifyCode = HIWORD(wParam);	/* 通知コード */
		wID			= LOWORD(wParam);	/* 項目ID、コントロールID またはアクセラレータID */
		hwndCtl		= (HWND) lParam;	/* コントロールのハンドル */
		switch( wNotifyCode ){
		/* ボタン／チェックボックスがクリックされた */
		case BN_CLICKED:
			switch( wID ){
			case IDC_BUTTON_PRINTSETTING:
				OnPrintPageSetting();
				break;
			case IDC_BUTTON_ZOOMUP:
				/* プレビュー拡大縮小 */
				OnPreviewZoom( TRUE );
				::UpdateWindow( m_hwndPrintPreviewBar );
				break;
			case IDC_BUTTON_ZOOMDOWN:
				/* プレビュー拡大縮小 */
				OnPreviewZoom( FALSE );
				::UpdateWindow( m_hwndPrintPreviewBar );
				break;
			case IDC_BUTTON_PREVPAGE:
				/* 前ページ */
				OnPreviewGoPage( m_nCurPageNum - 1 );
				::UpdateWindow( m_hwndPrintPreviewBar );
				break;
			case IDC_BUTTON_NEXTPAGE:
				/* 次ページ */
				OnPreviewGoPage( m_nCurPageNum + 1 );
				::UpdateWindow( m_hwndPrintPreviewBar );
				break;

			case IDC_BUTTON_HELP:
				/* ヘルプファイルのフルパスを返す */
				::GetHelpFilePath( szHelpFile );
				/* 印刷プレビューのヘルプ */
				//Stonee, 2001/03/12 第四引数を、機能番号からヘルプトピック番号を調べるようにした
				::WinHelp( hwndDlg, szHelpFile, HELP_CONTEXT, ::FuncID_To_HelpContextID(F_PRINT_PREVIEW) );
				::UpdateWindow( m_hwndPrintPreviewBar );
				break;
			case IDOK:
				/* 印刷実行 */
				OnPrint();
//				/* ダイアログデータの取得 */
//				::EndDialog( hwndDlg, 0 );
				::UpdateWindow( m_hwndPrintPreviewBar );
				return TRUE;
			case IDCANCEL:
				/* 印刷プレビューモードのオン/オフ */
				PrintPreviewModeONOFF();
//				/* ダイアログデータの取得 */
//				::EndDialog( hwndDlg, 0 );
				::UpdateWindow( m_hwndPrintPreviewBar );
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}




/* 印刷プレビューモードのオン/オフ */
void CEditWnd::PrintPreviewModeONOFF( void )
{
//	char	szErrMsg[1024];
	int		nToolBarHeight;
	RECT	rc;
//	HDC		hdc;
//	POINT	po;
//	int		i;
	HMENU	hMenu;

	/* 印刷プレビューモードか */
	if( m_cEditDoc.m_bPrintPreviewMode ){
		/* 印刷用のレイアウト情報の変更 */
		m_CLayoutMgr_Print.Empty();
		m_CLayoutMgr_Print.Init();
		m_cEditDoc.m_bPrintPreviewMode = FALSE;


		/* 印刷プレビュー 操作バー 削除 */
		::DestroyWindow( m_hwndPrintPreviewBar );
		m_hwndPrintPreviewBar = NULL;
		/* 印刷プレビュー 垂直スクロールバーウィンドウ 削除 */
		if( NULL != m_hwndVScrollBar ){
			::DestroyWindow( m_hwndVScrollBar );
		}
		m_hwndVScrollBar = NULL;
		/* 印刷プレビュー 水平スクロールバーウィンドウ 削除 */
		if( NULL != m_hwndHScrollBar ){
			::DestroyWindow( m_hwndHScrollBar );
		}
		m_hwndHScrollBar = NULL;
		/* 印刷プレビュー サイズボックスウィンドウ 削除 */
		::DestroyWindow( m_hwndSizeBox );
		m_hwndSizeBox = NULL;
		::ShowWindow( m_cEditDoc.m_hWnd, SW_SHOW );
		::ShowWindow( m_hwndToolBar, SW_SHOW );
		::ShowWindow( m_hwndStatusBar, SW_SHOW );
		::ShowWindow( m_CFuncKeyWnd.m_hWnd, SW_SHOW );

		::SetFocus( m_hWnd );
//		::SetFocus( m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].m_hWnd );
//		m_cEditDoc.m_cEditViewArr[m_cEditDoc.m_nActivePaneIndex].OnChangeSetting();

		hMenu = ::LoadMenu( m_hInstance, MAKEINTRESOURCE( IDR_MENU1 ) );
		::SetMenu( m_hWnd, hMenu );
//		::SetMenu( m_hWnd, NULL );	//Jan. 12, 2001 JEPROtestnow
		::DrawMenuBar( m_hWnd );
	}else{
		/* 現在の印刷設定 */
		m_pPrintSetting =
			&m_pShareData->m_PrintSettingArr[
				m_cEditDoc.GetDocumentAttribute().m_nCurrentPrintSetting];
		/* 現在のデフォルトプリンタの情報を取得 */
		BOOL bRes;
		bRes = CPrint::GetDefaultPrinterInfo( &m_pPrintSetting->m_mdmDevMode );
		if( FALSE == bRes ){
			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONINFORMATION | MB_TOPMOST, GSTR_APPNAME,
				"印刷プレビューを実行する前に、プリンタをインストールしてください。\n"
			);
			return;
		}


		hMenu = ::GetMenu( m_hWnd );
		//	Jun. 18, 2001 genta Print Previewではメニューを削除
		::SetMenu( m_hWnd, NULL );
		::DestroyMenu( hMenu );
//		::RedrawWindow( m_hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW );
		::DrawMenuBar( m_hWnd );

		::ShowWindow( m_cEditDoc.m_hWnd, SW_HIDE );
		::ShowWindow( m_hwndToolBar, SW_HIDE );
		::ShowWindow( m_hwndStatusBar, SW_HIDE );
		::ShowWindow( m_CFuncKeyWnd.m_hWnd, SW_HIDE );



		m_cEditDoc.m_bPrintPreviewMode = TRUE;



		/* 印刷プレビュー 操作バー 作成 */
		CreatePrintPreviewBar();

		/* 印刷プレビュー 操作バー */
		nToolBarHeight = 0;
		if( NULL != m_hwndPrintPreviewBar ){
			::GetWindowRect( m_hwndPrintPreviewBar, &rc );
			nToolBarHeight = rc.bottom - rc.top;
		}

		/* 印刷設定の反映 */
		OnChangePrintSetting();


//		/* プレビュー ページ指定 */
//		void CEditWnd::OnPreviewGoPage( int nPage/*, BOOL bPrevPage*/ )

	}
	::InvalidateRect( m_hWnd, NULL, TRUE );
//	::UpdateWindow( m_hWnd );
	return;

}




/* WM_SIZE 処理 */
LRESULT CEditWnd::OnSize( WPARAM wParam, LPARAM lParam )
{
	HDC			hdc;
	int			cx;
	int			cy;
	int			nToolBarHeight;
	int			nStatusBarHeight;
	int			nFuncKeyWndHeight;
	RECT		rc, rcClient;
	int			nCxHScroll;
	int			nCyHScroll;
	int			nCxVScroll;
	int			nCyVScroll;
	POINT		po;
//	SCROLLINFO	si;
	SIZE		sz;
	int			nCx, nCy;
	RECT		rcWin;



	cx = LOWORD( lParam );
	cy = HIWORD( lParam );

	/* ウィンドウサイズ継承 */
	if( wParam != SIZE_MINIMIZED &&						/* 最小化は継承しない */
		m_pShareData->m_Common.m_bSaveWindowSize		/* ウィンドウサイズ継承をするか */
	){
		if( wParam == SIZE_MAXIMIZED ){					/* 最大化はサイズを記録しない */
			if( m_pShareData->m_Common.m_nWinSizeType != (int)wParam ){
				m_pShareData->m_Common.m_nWinSizeType = wParam;
//				m_pShareData->m_nCommonModify = TRUE;	/* 共通設定変更フラグ */
			}
		}else{
			::GetWindowRect( m_hWnd, &rcWin );
			/* ウィンドウサイズに関するデータが変更されたか */
			if( m_pShareData->m_Common.m_nWinSizeType != (int)wParam ||
				m_pShareData->m_Common.m_nWinSizeCX != rcWin.right - rcWin.left ||
				m_pShareData->m_Common.m_nWinSizeCY != rcWin.bottom - rcWin.top
			){
				m_pShareData->m_Common.m_nWinSizeType = wParam;
				m_pShareData->m_Common.m_nWinSizeCX = rcWin.right - rcWin.left;
				m_pShareData->m_Common.m_nWinSizeCY = rcWin.bottom - rcWin.top;
//				m_pShareData->m_nCommonModify = TRUE;	/* 共通設定変更フラグ */
			}
		}
	}

	m_nWinSizeType = wParam;	/* サイズ変更のタイプ */
	nCxHScroll = ::GetSystemMetrics( SM_CXHSCROLL );
	nCyHScroll = ::GetSystemMetrics( SM_CYHSCROLL );
	nCxVScroll = ::GetSystemMetrics( SM_CXVSCROLL );
	nCyVScroll = ::GetSystemMetrics( SM_CYVSCROLL );




	nToolBarHeight = 0;
	if( NULL != m_hwndToolBar ){
		::SendMessage( m_hwndToolBar, WM_SIZE, wParam, lParam );
		::GetWindowRect( m_hwndToolBar, &rc );
		nToolBarHeight = rc.bottom - rc.top;
	}
	nFuncKeyWndHeight = 0;
	if( NULL != m_CFuncKeyWnd.m_hWnd ){
		::SendMessage( m_CFuncKeyWnd.m_hWnd, WM_SIZE, wParam, lParam );
		::GetWindowRect( m_CFuncKeyWnd.m_hWnd, &rc );
		nFuncKeyWndHeight = rc.bottom - rc.top;
	}
	nStatusBarHeight = 0;
	if( NULL != m_hwndStatusBar ){
		::SendMessage( m_hwndStatusBar, WM_SIZE, wParam, lParam );
		::GetClientRect( m_hwndStatusBar, &rc );
		//	May 12, 2000 genta
		//	2カラム目に改行コードの表示を挿入
		//	From Here
		int			nStArr[8];
//		const char*	pszLabel[7] = { "", "99999999行 99999列", "RR0RR0","FFFFFFFF", "SJIS  ", "REC", "上書" };
		const char*	pszLabel[7] = { "", "999999行 99999列", "RR0RR0","FFFFFFFF", "SJIS  ", "REC", "上書" };	//Oct. 30, 2000 JEPRO 千万行も要らん
		int			nStArrNum = 7;
		//	To Here
		int			nAllWidth;
		SIZE		sz;
		HDC			hdc;
		int			i;
		nAllWidth = rc.right - rc.left;
		hdc = ::GetDC( m_hwndStatusBar );
		nStArr[nStArrNum - 1] = nAllWidth;
		if( wParam != SIZE_MAXIMIZED ){
			nStArr[nStArrNum - 1] -= 16;
		}
		for( i = nStArrNum - 1; i > 0; i-- ){
			::GetTextExtentPoint32( hdc, pszLabel[i], lstrlen( pszLabel[i] ), &sz );
			nStArr[i - 1] = nStArr[i] - ( sz.cx + ::GetSystemMetrics( SM_CXEDGE ) );
		}
//		nStArr[0] = 8;
		nStArr[0] += 16;	//Nov. 2, 2000 JEPRO よくわからないがともかくここを増やしてみる
//		nStArr[1] -= 16;	//Nov. 2, 2000 JEPRO よくわからないがともかくここを減らしてみる(失敗)

		::SendMessage( m_hwndStatusBar, SB_SETPARTS, nStArrNum, (LPARAM) (LPINT)nStArr );
//		for( i = 0; i < nStArrNum; ++i ){
//			::SendMessage( m_hwndStatusBar, SB_SETTEXT, i | 0, (LPARAM) (LPINT)pszLabel[i] );
//		}
		::ReleaseDC( m_hwndStatusBar, hdc );


		::GetWindowRect( m_hwndStatusBar, &rc );
		nStatusBarHeight = rc.bottom - rc.top;
	}
	::GetClientRect( m_hWnd, &rcClient );

	if( m_pShareData->m_Common.m_nFUNCKEYWND_Place == 0 ){	/* ファンクションキー表示位置／0:上 1:下 */
		m_cEditDoc.OnMove(
			0,
			nToolBarHeight + nFuncKeyWndHeight,
			cx,
			cy - nToolBarHeight - nFuncKeyWndHeight - nStatusBarHeight
		);
		::MoveWindow( m_CFuncKeyWnd.m_hWnd, 0, nToolBarHeight, cx, nFuncKeyWndHeight, TRUE );
	}else
	if( m_pShareData->m_Common.m_nFUNCKEYWND_Place == 1 ){	/* ファンクションキー表示位置／0:上 1:下 */
		m_cEditDoc.OnMove( 0, nToolBarHeight, cx, cy - nToolBarHeight - nFuncKeyWndHeight - nStatusBarHeight );
		::MoveWindow(
			m_CFuncKeyWnd.m_hWnd,
			0,
			cy - nFuncKeyWndHeight - nStatusBarHeight,
			cx,
			nFuncKeyWndHeight, TRUE
		);
		BOOL	bSizeBox = TRUE;
		if( NULL != m_hwndStatusBar ){
			bSizeBox = FALSE;
		}
		if( wParam == SIZE_MAXIMIZED ){
			bSizeBox = FALSE;
		}
		m_CFuncKeyWnd.SizeBox_ONOFF( bSizeBox );
	}
	/* 印刷プレビューモードか */
	if( FALSE == m_cEditDoc.m_bPrintPreviewMode ){
		return 0L;
	}
	/* 印刷プレビュー 操作バー */
	nToolBarHeight = 0;
	if( NULL != m_hwndPrintPreviewBar ){
		::GetWindowRect( m_hwndPrintPreviewBar, &rc );
		nToolBarHeight = rc.bottom - rc.top;
		::MoveWindow( m_hwndPrintPreviewBar, 0, 0, cx, nToolBarHeight, TRUE );
	}
	/* 印刷プレビュー 垂直スクロールバーウィンドウハンドル */
	if( NULL != m_hwndVScrollBar ){
		::MoveWindow( m_hwndVScrollBar, cx - nCxVScroll, nToolBarHeight, nCxVScroll, cy - nCyVScroll - nToolBarHeight, TRUE );
	}
	/* 印刷プレビュー 水平スクロールバーウィンドウハンドル */
	if( NULL != m_hwndHScrollBar ){
		::MoveWindow( m_hwndHScrollBar, 0, cy - nCyHScroll, cx - nCxVScroll, nCyHScroll, TRUE );
	}
	/* 印刷プレビュー サイズボックスウィンドウハンドル */
	if( NULL != m_hwndSizeBox ){
		::MoveWindow( m_hwndSizeBox, cx - nCxVScroll, cy - nCyHScroll, nCxHScroll, nCyVScroll, TRUE );
	}else{
	}


	hdc = ::GetDC( m_hWnd );
	::SetMapMode(hdc, MM_LOMETRIC );
	::SetMapMode( hdc, MM_ANISOTROPIC );
	/* 出力倍率の変更 */
	::GetWindowExtEx( hdc, &sz );
	nCx = sz.cx;
	nCy = sz.cy;
	nCx = (int)( ((long)nCx) * 100L / ((long)m_nPreview_Zoom) );
	nCy = (int)( ((long)nCy) * 100L / ((long)m_nPreview_Zoom) );
	::SetWindowExtEx( hdc, nCx, nCy, &sz );

	/* ビューのサイズ */
	po.x = m_nPreview_PaperAllWidth + m_nPreview_ViewMarginLeft * 2;
	po.y = m_nPreview_PaperAllHeight + m_nPreview_ViewMarginTop * 2;
	::LPtoDP( hdc, &po, 1 );



	/* 再描画用メモリＢＭＰ */
	if( m_hbmpCompatBMP != NULL ){
		::SelectObject( m_hdcCompatDC, m_hbmpCompatBMPOld );	/* 再描画用メモリＢＭＰ(OLD) */
		::DeleteObject( m_hbmpCompatBMP );
	}
	m_hbmpCompatBMP = ::CreateCompatibleBitmap( hdc, cx, cy );
	m_hbmpCompatBMPOld = (HBITMAP)::SelectObject( m_hdcCompatDC, m_hbmpCompatBMP );


	::ReleaseDC( m_hWnd, hdc );

	/* 印刷プレビュー：ビュー幅(ピクセル) */
	m_nPreview_ViewWidth = abs( po.x );
	/* 印刷プレビュー：ビュー高さ(ピクセル) */
	m_nPreview_ViewHeight = abs( po.y );

	/* 印刷プレビュー スクロールバー初期化 */
	InitPreviewScrollBar();

	/* 印刷プレビュー スクロールバーの初期化 */

	m_nDragPosOrgX = 0;
	m_nDragPosOrgY = 0;
	m_bDragMode = TRUE;
	OnMouseMove( 0, MAKELONG( 0, 0 ) );
	m_bDragMode = FALSE;
	return 0L;
}











/* WM_PAINT 描画処理 */
LRESULT CEditWnd::OnPaint(
	HWND			hwnd,	// handle of window
	UINT			uMsg,	// message identifier
	WPARAM			wParam,	// first message parameter
	LPARAM			lParam 	// second message parameter
)
{
	HDC				hdc;
	HDC				hdcOld;
	PAINTSTRUCT		ps;
	RECT			rc;
//	char*			pszText;
//	int				nY;
	int				nMapModeOld;
	int				nToolBarHeight;
//	POINT			poViewportOrgOld;
//	POINT			po;
	HFONT			hFont, hFontOld;
//	int				i, j;
//	int				m_nPreview_Zoom;
	SIZE			sz;
	int				nCx, nCy;
	int				nDirectY;
//	LOGFONT			lf;
//	int				nLineHeight;
//	HBRUSH			hBrush, hBrushOld;
	HPEN			hPen, hPenOld;
	char			szWork[260];
//	CHOOSEFONT		cf;
	POINT			poViewPortOld;
	HFONT			hFontZen;
	HFONT			hFontZenOld;

	/* 印刷プレビューモードか */
	if( FALSE == m_cEditDoc.m_bPrintPreviewMode ){
		PAINTSTRUCT		ps;
		hdc = ::BeginPaint( hwnd, &ps );
		::EndPaint( hwnd, &ps );
		return 0L;
	}

	hdcOld = ::BeginPaint( hwnd, &ps );
	hdc = m_hdcCompatDC;

	::GetClientRect( hwnd, &rc );
	::FillRect( hdc, &rc, (HBRUSH)::GetStockObject( GRAY_BRUSH ) );

	/* 印刷プレビュー 操作バー */
	nToolBarHeight = 0;
	if( NULL != m_hwndPrintPreviewBar ){
		::GetWindowRect( m_hwndPrintPreviewBar, &rc );
		nToolBarHeight = rc.bottom - rc.top;
	}

	/* プリンタ情報の表示 */
	::SetDlgItemText( m_hwndPrintPreviewBar, IDC_STATIC_PRNDEV, m_pPrintSetting->m_mdmDevMode.m_szPrinterDeviceName );
	char	szText[1024];
	char	szPaperName[256];
	CPrint::GetPaperName( m_pPrintSetting->m_mdmDevMode.dmPaperSize , (char*)szPaperName );
	wsprintf( szText, "%s  %s",
		szPaperName,
		(m_pPrintSetting->m_mdmDevMode.dmOrientation & DMORIENT_LANDSCAPE)?"横":"縦"
	);
	::SetDlgItemText( m_hwndPrintPreviewBar, IDC_STATIC_PAPER, szText );

	/* バックグラウンド モードを変更 */
	::SetBkMode( hdc, TRANSPARENT );

	/* マッピングモードの変更 */
	nMapModeOld =
	::SetMapMode(hdc, MM_LOMETRIC );
	::SetMapMode( hdc, MM_ANISOTROPIC );
	nDirectY = -1;
	/* 出力倍率の変更 */
	::GetWindowExtEx( hdc, &sz );
	nCx = sz.cx;
	nCy = sz.cy;
	nCx = (int)( ((long)nCx) * 100L / ((long)m_nPreview_Zoom) );
	nCy = (int)( ((long)nCy) * 100L / ((long)m_nPreview_Zoom) );
	::SetWindowExtEx( hdc, nCx, nCy, &sz );

	/* 印刷フォント作成 & 設定 */
	m_lfPreviewHan.lfHeight	= m_pPrintSetting->m_nPrintFontHeight;
	m_lfPreviewHan.lfWidth	= m_pPrintSetting->m_nPrintFontWidth;
	strcpy( m_lfPreviewHan.lfFaceName, m_pPrintSetting->m_szPrintFontFaceHan );

	m_lfPreviewZen.lfHeight	= m_pPrintSetting->m_nPrintFontHeight;
	m_lfPreviewZen.lfWidth	= m_pPrintSetting->m_nPrintFontWidth;
	strcpy( m_lfPreviewZen.lfFaceName, m_pPrintSetting->m_szPrintFontFaceZen );

	hFont = CreateFontIndirect( &m_lfPreviewHan );
	hFontOld = (HFONT)::SelectObject( hdc, hFont );
	hFontZen = CreateFontIndirect( &m_lfPreviewZen );

	/* 操作ウィンドウの下に物理座標原点を移動 */
	::SetViewportOrgEx( hdc, -1 * m_nPreviewHScrollPos, nToolBarHeight + m_nPreviewVScrollPos, &poViewPortOld );

	/* 用紙の描画 */
	::Rectangle( hdc,
		m_nPreview_ViewMarginLeft,
		nDirectY * ( m_nPreview_ViewMarginTop ),
		m_nPreview_ViewMarginLeft + m_nPreview_PaperAllWidth + 1,
		nDirectY * (m_nPreview_ViewMarginTop + m_nPreview_PaperAllHeight + 1 )
	);
	/* マージン枠の表示 */
	hPen = ::CreatePen( PS_SOLID, 0, RGB(127,127,127) );
	hPenOld = (HPEN)::SelectObject( hdc, hPen );
	::Rectangle( hdc,
		m_nPreview_ViewMarginLeft + m_pPrintSetting->m_nPrintMarginLX,
		nDirectY * ( m_nPreview_ViewMarginTop + m_pPrintSetting->m_nPrintMarginTY ),
		m_nPreview_ViewMarginLeft + m_nPreview_PaperAllWidth - m_pPrintSetting->m_nPrintMarginRX + 1,
		nDirectY * ( m_nPreview_ViewMarginTop + m_nPreview_PaperAllHeight - m_pPrintSetting->m_nPrintMarginBY )
	);
	::SelectObject( hdc, hPenOld );
	::DeleteObject( hPen );

	::SetTextColor( hdc, RGB( 0, 0, 0 ) );

	hFontZenOld = (HFONT)::SelectObject( hdc, hFontZen );

	/* ファイル名の表示 */
	if( 0 == lstrlen( m_cEditDoc.m_szFilePath ) ){	/* 現在編集中のファイルのパス */
		strcpy( szWork, "(無題)" );
	}else{
		char	szFileName[_MAX_FNAME];
		char	szExt[_MAX_EXT];
		_splitpath( m_cEditDoc.m_szFilePath, NULL, NULL, szFileName, szExt );
		wsprintf( szWork, "%s%s", szFileName, szExt );
	}
	::TextOut( hdc,
		m_nPreview_ViewMarginLeft + m_pPrintSetting->m_nPrintMarginLX,
		nDirectY * ( m_nPreview_ViewMarginTop + m_pPrintSetting->m_nPrintMarginTY ),
		szWork, lstrlen( szWork )
	);
	::SelectObject( hdc, hFontZenOld );

	/* 印刷/印刷プレビュー ページテキストの描画 */
	DrawPageText(
		hdc,
		m_nPreview_ViewMarginLeft + m_pPrintSetting->m_nPrintMarginLX,
		m_nPreview_ViewMarginTop  + m_pPrintSetting->m_nPrintMarginTY + 2 * ( m_pPrintSetting->m_nPrintFontHeight + (m_pPrintSetting->m_nPrintFontHeight * m_pPrintSetting->m_nPrintLineSpacing / 100) ),
		m_nCurPageNum,
		hFontZen,
		NULL
	);
	/* ページ番号の表示 */
	wsprintf( szWork, "- %d -", m_nCurPageNum + 1 );
	::TextOut( hdc,
		m_nPreview_ViewMarginLeft
//		+ m_pPrintSetting->m_nPrintMarginLX
		+ m_nPreview_PaperWidth / 2
		- ( 5 * 10 )
//		- m_nPreview_PaperOffsetLeft
		,
		nDirectY * ( m_nPreview_ViewMarginTop + m_nPreview_PaperAllHeight - m_pPrintSetting->m_nPrintMarginBY - m_pPrintSetting->m_nPrintFontHeight/* - m_nPreview_PaperOffsetTop*/ ),
		szWork, lstrlen( szWork )
	);

	/* 印刷フォント解除 & 破棄 */
	::SelectObject( hdc, hFontOld );
	::DeleteObject( hFont );
	::DeleteObject( hFontZen );
	/* マッピングモードの変更 */
	::SetMapMode( hdc, nMapModeOld );

	/* 物理座標原点をもとに戻す */
	::SetViewportOrgEx( hdc, poViewPortOld.x, poViewPortOld.y, NULL );


	/* メモリＤＣを利用した再描画の場合はメモリＤＣに描画した内容を画面へコピーする */
	rc = ps.rcPaint;
	::DPtoLP( hdc, (POINT*)&rc, 2 );
	::BitBlt(
		hdcOld,
		ps.rcPaint.left,
		ps.rcPaint.top,
		ps.rcPaint.right - ps.rcPaint.left,
		ps.rcPaint.bottom - ps.rcPaint.top,
		hdc,
		ps.rcPaint.left,
		ps.rcPaint.top,
		SRCCOPY
	);
	::EndPaint( hwnd, &ps );
	return 0L;
}



/* 印刷プレビュー スクロールバー初期化 */
void CEditWnd::InitPreviewScrollBar( void )
{
	SCROLLINFO	si;
	RECT		rc;
	int			cx, cy;
	int			nToolBarHeight;
	nToolBarHeight = 0;
	if( NULL != m_hwndPrintPreviewBar ){
		::GetWindowRect( m_hwndPrintPreviewBar, &rc );
		nToolBarHeight = rc.bottom - rc.top;
	}
	::GetClientRect( m_hWnd, &rc );
	cx = rc.right - rc.left;
	cy = rc.bottom - rc.top - nToolBarHeight;
	if( NULL != m_hwndVScrollBar ){
		/* 垂直スクロールバー */
		si.cbSize = sizeof( SCROLLINFO );
		si.fMask = SIF_ALL;
		si.nMin  = 0;
		if( m_nPreview_ViewHeight <= cy - nToolBarHeight ){
			si.nMax  = cy - nToolBarHeight;			/* 全幅 */
			si.nPage = cy - nToolBarHeight;			/* 表示域の桁数 */
			si.nPos  = -1 * m_nPreviewVScrollPos;	/* 表示域の一番左の位置 */
			si.nTrackPos = 0;
			m_SCROLLBAR_VERT = FALSE;
		}else{
			si.nMax  = m_nPreview_ViewHeight;		/* 全幅 */
			si.nPage = cy - nToolBarHeight;			/* 表示域の桁数 */
			si.nPos  = -1 * m_nPreviewVScrollPos;	/* 表示域の一番左の位置 */
			si.nTrackPos = 100;
			m_SCROLLBAR_VERT = TRUE;
		}
		::SetScrollInfo( m_hwndVScrollBar, SB_CTL, &si, TRUE );
	}
	/* 印刷プレビュー 水平スクロールバーウィンドウハンドル */
	if( NULL != m_hwndHScrollBar ){
		si.cbSize = sizeof( si );
		si.fMask = SIF_ALL;
		/* 水平スクロールバー */
		si.cbSize = sizeof( si );
		si.fMask = SIF_ALL;
		si.nMin  = 0;
		if( m_nPreview_ViewWidth <= cx ){
			si.nMax  = cx;							/* 全幅 */
			si.nPage = cx;							/* 表示域の桁数 */
			si.nPos  = m_nPreviewHScrollPos;		/* 表示域の一番左の位置 */
			si.nTrackPos = 0;
			m_SCROLLBAR_HORZ = FALSE;
		}else{
			si.nMax  = m_nPreview_ViewWidth;		/* 全幅 */
			si.nPage = cx;							/* 表示域の桁数 */
			si.nPos  = m_nPreviewHScrollPos;		/* 表示域の一番左の位置 */
			si.nTrackPos = 100;
			m_SCROLLBAR_HORZ = TRUE;
		}
		::SetScrollInfo( m_hwndHScrollBar, SB_CTL, &si, TRUE );
	}
	return;
}




/* プレビュー拡大縮小 */
void CEditWnd::OnPreviewZoom( BOOL bZoomUp )
{
	char		szEdit[1024];
	RECT		rc1;
	if( bZoomUp ){
		m_nPreview_Zoom += 10;	/* 印刷プレビュー倍率 */
		if( MAX_PREVIEW_ZOOM < m_nPreview_Zoom ){
			m_nPreview_Zoom = MAX_PREVIEW_ZOOM;
		}
	}else{
		/* スクロール位置を調整 */
		m_nPreviewVScrollPos = 0;
		m_nPreviewHScrollPos = 0;

		m_nPreview_Zoom -= 10;	/* 印刷プレビュー倍率 */
		if( MIN_PREVIEW_ZOOM > m_nPreview_Zoom ){
			m_nPreview_Zoom = MIN_PREVIEW_ZOOM;
		}
	}
	if( MIN_PREVIEW_ZOOM == m_nPreview_Zoom ){
		::EnableWindow( ::GetDlgItem( m_hwndPrintPreviewBar, IDC_BUTTON_ZOOMDOWN ), FALSE );
	}else{
		::EnableWindow( ::GetDlgItem( m_hwndPrintPreviewBar, IDC_BUTTON_ZOOMDOWN ), TRUE );
	}
	if( MAX_PREVIEW_ZOOM == m_nPreview_Zoom ){
		::EnableWindow( ::GetDlgItem( m_hwndPrintPreviewBar, IDC_BUTTON_ZOOMUP ), FALSE );
	}else{
		::EnableWindow( ::GetDlgItem( m_hwndPrintPreviewBar, IDC_BUTTON_ZOOMUP ), TRUE );
	}
	wsprintf( szEdit, "%d %%", m_nPreview_Zoom );
	::SetDlgItemText( m_hwndPrintPreviewBar, IDC_STATIC_ZOOM, szEdit );


	/* WM_SIZE 処理 */
	::GetClientRect( m_hWnd, &rc1 );
	OnSize( SIZE_RESTORED, MAKELONG( rc1.right - rc1.left, rc1.bottom - rc1.top ) );


	/* 印刷プレビュー スクロールバー初期化 */
	InitPreviewScrollBar();
	/* 再描画 */
	::InvalidateRect( m_hWnd, NULL, TRUE );
	return;
}




/* 印刷プレビュー 垂直スクロールバーメッセージ処理 WM_VSCROLL */
LRESULT CEditWnd::OnVScroll( WPARAM wParam, LPARAM lParam )
{
	/* 印刷プレビューモードか */
	if( FALSE == m_cEditDoc.m_bPrintPreviewMode ){
		return 0;
	}
	int			nPreviewVScrollPos;
	SCROLLINFO	si;
	int			nNowPos;
	int			nMove;
	int			nNewPos;
	int			nScrollCode;
	int			nPos;
	HWND		hwndScrollBar;
	nScrollCode = (int) LOWORD(wParam);
	nPos = (int) HIWORD(wParam);
	hwndScrollBar = (HWND) lParam;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	::GetScrollInfo( hwndScrollBar, SB_CTL, (SCROLLINFO*)&si );
	nNowPos = ::GetScrollPos( hwndScrollBar, SB_CTL );
	nNewPos = 0;
	nMove = 0;
	switch( nScrollCode ){
	case SB_LINEUP:
		nMove = -1 * LINE_RANGE_Y;
		break;
	case SB_LINEDOWN:
		nMove = LINE_RANGE_Y;
		break;
	case SB_PAGEUP:
		nMove = -1 * PAGE_RANGE_Y;
		break;
	case SB_PAGEDOWN:
		nMove = PAGE_RANGE_Y;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nMove = nPos - nNowPos;
		break;
	default:
		return 0;
	}
	nNewPos = nNowPos + nMove;
	if( nNewPos < 0 ){
		nNewPos = 0;
	}else
	if( nNewPos > (int)(si.nMax - si.nPage + 1) ){
		nNewPos = (int)(si.nMax - si.nPage + 1);
	}
	nMove = nNowPos - nNewPos;
	nPreviewVScrollPos = -1 * nNewPos;
	if( nPreviewVScrollPos != m_nPreviewVScrollPos ){
		::SetScrollPos( hwndScrollBar, SB_CTL, nNewPos, TRUE);
		m_nPreviewVScrollPos = nPreviewVScrollPos;
		/* 描画 */
		::ScrollWindowEx( m_hWnd, 0, nMove, NULL, NULL, NULL , NULL, SW_ERASE | SW_INVALIDATE );
		::UpdateWindow( m_hWnd );
	}
	return 0;
}




/* 印刷プレビュー 水平スクロールバーメッセージ処理 */
LRESULT CEditWnd::OnHScroll( WPARAM wParam, LPARAM lParam )
{
	/* 印刷プレビューモードか */
	if( FALSE == m_cEditDoc.m_bPrintPreviewMode ){
		return 0;
	}
	int			nPreviewHScrollPos;
	SCROLLINFO	si;
	int			nNowPos;
	int			nMove;
	int			nNewPos;
	int			nScrollCode;
	int			nPos;
	HWND		hwndScrollBar;
	nScrollCode = (int) LOWORD(wParam);
	nPos = (int) HIWORD(wParam);
	hwndScrollBar = (HWND) lParam;
	si.cbSize = sizeof( SCROLLINFO );
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	::GetScrollInfo( hwndScrollBar, SB_CTL, (SCROLLINFO*)&si );
	nNowPos = ::GetScrollPos( hwndScrollBar, SB_CTL );
	nNewPos = 0;
	nMove = 0;
	switch( nScrollCode ){
	case SB_LINEUP:
		nMove = -1 * LINE_RANGE_Y;
		break;
	case SB_LINEDOWN:
		nMove = LINE_RANGE_Y;
		break;
	case SB_PAGEUP:
		nMove = -1 * PAGE_RANGE_Y;
		break;
	case SB_PAGEDOWN:
		nMove = PAGE_RANGE_Y;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nMove = nPos - nNowPos;
		break;
	default:
		return 0;
	}
	nNewPos = nNowPos + nMove;
	if( nNewPos < 0 ){
		nNewPos = 0;
	}else
	if( nNewPos > (int)(si.nMax - si.nPage + 1) ){
		nNewPos = (int)(si.nMax - si.nPage + 1);
	}
	nMove = nNowPos - nNewPos;
	nPreviewHScrollPos = nNewPos;
	if( nPreviewHScrollPos != m_nPreviewHScrollPos ){
		::SetScrollPos( hwndScrollBar, SB_CTL, nNewPos, TRUE);
		m_nPreviewHScrollPos = nPreviewHScrollPos;
		/* 描画 */
		::ScrollWindowEx( m_hWnd, nMove, 0, NULL, NULL, NULL , NULL, SW_ERASE | SW_INVALIDATE );
		::UpdateWindow( m_hWnd );
	}
	return 0;
}




/* プレビュー ページ指定 */
void CEditWnd::OnPreviewGoPage( int nPage/*, BOOL bPrevPage*/ )
{
	char	szEdit[1024];
//	if( -1 == nPage ){
//		if( bPrevPage ){
//			if( 0 < m_nCurPageNum ){					/* 現在のページ */
//				m_nCurPageNum--;
//				::InvalidateRect( m_hWnd, NULL, TRUE );
//			}
//		}else{
//			if( m_nCurPageNum + 1 < m_nAllPageNum ){	/* 現在のページ */
//				m_nCurPageNum++;
//				::InvalidateRect( m_hWnd, NULL, TRUE );
//			}
//		}
//	}else{
		if( m_nAllPageNum <= nPage ){	/* 現在のページ */
			nPage = m_nAllPageNum - 1;
		}
		if( 0 > nPage ){				/* 現在のページ */
			nPage = 0;
		}
		m_nCurPageNum = nPage;
		::InvalidateRect( m_hWnd, NULL, TRUE );


		if( 0 == m_nCurPageNum ){
			//	Jul. 18, 2001 genta FocusのあるWindowをDisableにすると操作できなくなるのを回避
			::SetFocus( ::GetDlgItem( m_hwndPrintPreviewBar, IDC_BUTTON_NEXTPAGE ));
			::EnableWindow( ::GetDlgItem( m_hwndPrintPreviewBar, IDC_BUTTON_PREVPAGE ), FALSE );
		}else{
			::EnableWindow( ::GetDlgItem( m_hwndPrintPreviewBar, IDC_BUTTON_PREVPAGE ), TRUE );
		}
		if( m_nAllPageNum <= m_nCurPageNum + 1 ){
			//	Jul. 18, 2001 genta FocusのあるWindowをDisableにすると操作できなくなるのを回避
			::SetFocus( ::GetDlgItem( m_hwndPrintPreviewBar, IDC_BUTTON_PREVPAGE ));
			::EnableWindow( ::GetDlgItem( m_hwndPrintPreviewBar, IDC_BUTTON_NEXTPAGE ), FALSE );
		}else{
			::EnableWindow( ::GetDlgItem( m_hwndPrintPreviewBar, IDC_BUTTON_NEXTPAGE ), TRUE );
		}

		wsprintf( szEdit, "%d/%d頁", m_nCurPageNum + 1, m_nAllPageNum );
		::SetDlgItemText( m_hwndPrintPreviewBar, IDC_STATIC_PAGENUM, szEdit );

		wsprintf( szEdit, "%d %%", m_nPreview_Zoom );
		::SetDlgItemText( m_hwndPrintPreviewBar, IDC_STATIC_ZOOM, szEdit );

//	}
	return;
}




/* 印刷／プレビュー 行描画 */
void CEditWnd::Print_DrawLine(
	HDC			hdc,
	int			x,
	int			y,
	const char*	pLine,
	int			nLineLen,
	HFONT		hFontZen
)
{
	int			nPosX;
	int			nBgn;
	int			i;
	int			nCharChars;
	int			nMode = 1;
	HFONT		hFontZenOld;
	TEXTMETRIC	tm;
	int			nAscent;
	int			nAscentZen;

	nPosX = 0;
	nBgn = 0;
	::GetTextMetrics( hdc, &tm );
	nAscent = tm.tmAscent;
	hFontZenOld = (HFONT)::SelectObject( hdc, hFontZen );
	::GetTextMetrics( hdc, &tm );
	nAscentZen = tm.tmAscent;
	::SelectObject( hdc, hFontZenOld );


	for( i = 0; i < nLineLen; ++i ){
		nCharChars = CMemory::MemCharNext( pLine, nLineLen, &pLine[i] ) - &pLine[i];
		if( 0 == nCharChars ){
			nCharChars = 1;
		}
		if( nCharChars == 1 ){
			if( 1 == nMode ){
				if( TAB == pLine[i] ){
					if( 0 < i - nBgn ){
						::ExtTextOut(
							hdc,
							x + nPosX * m_pPrintSetting->m_nPrintFontWidth,
							y - ( m_pPrintSetting->m_nPrintFontHeight - nAscent ),
							0,
							NULL,
							&pLine[nBgn], i - nBgn,
							m_pnDx
						);
						nPosX += ( i - nBgn );
					}
					nPosX += ( m_cEditDoc.GetDocumentAttribute().m_nTabSpace - nPosX % m_cEditDoc.GetDocumentAttribute().m_nTabSpace );
					nBgn = i + 1;
				}else{
				}
			}else{
				if( 0 < i - nBgn ){
					hFontZenOld = (HFONT)::SelectObject( hdc, hFontZen );
					::ExtTextOut(
						hdc,
						x + nPosX * m_pPrintSetting->m_nPrintFontWidth,
						y - ( m_pPrintSetting->m_nPrintFontHeight - nAscentZen ),
						0,
						NULL,
						&pLine[nBgn], i - nBgn,
						m_pnDx
					);
					::SelectObject( hdc, hFontZenOld );
					nPosX += ( i - nBgn );
				}
//				nPosX += ( i - nBgn );
				nBgn = i;
				nMode = 1;
				if( TAB == pLine[i] ){
					nPosX += ( m_cEditDoc.GetDocumentAttribute().m_nTabSpace - nPosX % m_cEditDoc.GetDocumentAttribute().m_nTabSpace );
					nBgn = i + 1;
				}else{
				}
			}
		}else{
			if( 1 == nMode ){
				if( 0 < i - nBgn ){
					::ExtTextOut(
						hdc,
						x + nPosX * m_pPrintSetting->m_nPrintFontWidth,
						y - ( m_pPrintSetting->m_nPrintFontHeight - nAscent ),
						0,
						NULL,
						&pLine[nBgn], i - nBgn,
						m_pnDx
					);
					nPosX += ( i - nBgn );
				}
//				nPosX += ( m_cEditDoc.GetDocumentAttribute().m_nTabSpace - nPosX % m_cEditDoc.GetDocumentAttribute().m_nTabSpace );
				nBgn = i;
				nMode = 2;
			}else{
			}
			++i;
		}
	}
	if( 0 < i - nBgn ){
		if( 2 == nMode ){
			hFontZenOld = (HFONT)::SelectObject( hdc, hFontZen );
			::ExtTextOut(
				hdc,
				x + nPosX * m_pPrintSetting->m_nPrintFontWidth,
				y - ( m_pPrintSetting->m_nPrintFontHeight - nAscentZen ),
				0,
				NULL,
				&pLine[nBgn], i - nBgn,
				m_pnDx
			);
			::SelectObject( hdc, hFontZenOld );
		}else{
			::ExtTextOut(
				hdc,
				x + nPosX * m_pPrintSetting->m_nPrintFontWidth,
				y - ( m_pPrintSetting->m_nPrintFontHeight - nAscent ),
				0,
				NULL,
				&pLine[nBgn], i - nBgn,
				m_pnDx
			);
		}
	}
	return;
}


/* 印刷/印刷プレビュー ページテキストの描画 */
void CEditWnd::DrawPageText(
	HDC				hdc,
	int				nOffX,
	int				nOffY,
	int				nPageNum,
	HFONT			hFontZen,
	CDlgCancel*		pCDlgCancel
)
{
	int				nLineHeight;
	int				i;
	int				nDirectY;
	int				nDan;
	char			szLineNum[64];
	const CLayout*	pcLayout;
	int				nLineNum;
	int				nLineCols;
	const char*		pLine;
	int				nLineLen;
	TEXTMETRIC		tm;
	int				nAscent;
	int				nAscentZen;
	HFONT			hFontZenOld;

	nDirectY = -1;

	nLineHeight = m_pPrintSetting->m_nPrintFontHeight + ( m_pPrintSetting->m_nPrintFontHeight * m_pPrintSetting->m_nPrintLineSpacing / 100 );


	::GetTextMetrics( hdc, &tm );
	nAscent = tm.tmAscent;
	hFontZenOld = (HFONT)::SelectObject( hdc, hFontZen );
	::GetTextMetrics( hdc, &tm );
	nAscentZen = tm.tmAscent;
	::SelectObject( hdc, hFontZenOld );

	for( nDan = 0; nDan < m_pPrintSetting->m_nPrintDansuu; ++nDan ){
		for( i = 0; i < m_bPreview_EnableLines; ++i ){
			if( NULL != pCDlgCancel ){
				/* 処理中のユーザー操作を可能にする */
				if( !::BlockingHook( pCDlgCancel->m_hWnd ) ){
					return;
				}
			}


			nLineNum = nPageNum * ( m_bPreview_EnableLines * m_pPrintSetting->m_nPrintDansuu ) + m_bPreview_EnableLines * nDan + i;
			pLine = m_CLayoutMgr_Print.GetLineStr( nLineNum, &nLineLen );
			if( NULL == pLine ){
				break;
			}
			/* 行番号を表示するか */
			if( m_pPrintSetting->m_bPrintLineNumber ){
				/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
				if( m_cEditDoc.GetDocumentAttribute().m_bLineNumIsCRLF ){
					/* 論理行番号表示モード */
//					pcLayout = m_CLayoutMgr_Print.GetLineData( nLineNum );
					pcLayout = m_CLayoutMgr_Print.Search( nLineNum );
					if( NULL == pcLayout || 0 != pcLayout->m_nOffset ){
						strcpy( szLineNum, " " );
					}else{
						_itoa( pcLayout->m_nLinePhysical + 1, szLineNum, 10 );	/* 対応する論理行番号 */
					}
				}else{
					/* 物理行(レイアウト行)番号表示モード */
					_itoa( nLineNum + 1, szLineNum, 10 );
				}
		//		/* 行番号表示に必要な桁数を計算 */
		//		m_nPreview_LineNumberColmns = m_cEditDoc.m_cEditViewArr[0].DetectWidthOfLineNumberArea_calculate();
				/* 行番号区切り  0=なし 1=縦線 2=任意 */
				if( 2 == m_cEditDoc.GetDocumentAttribute().m_nLineTermType ){
					char szLineTerm[2];
					wsprintf( szLineTerm, "%c", m_cEditDoc.GetDocumentAttribute().m_cLineTermChar );	/* 行番号区切り文字 */
					strcat( szLineNum, szLineTerm );
				}else{
					strcat( szLineNum, " " );
				}
				nLineCols = lstrlen( szLineNum );
				::ExtTextOut( hdc,
					nOffX +
					( m_bPreview_EnableColms * m_pPrintSetting->m_nPrintFontWidth + m_pPrintSetting->m_nPrintDanSpace )
					* nDan +
					m_nPreview_LineNumberColmns * m_pPrintSetting->m_nPrintFontWidth * (nDan) +
					( m_nPreview_LineNumberColmns - nLineCols) * m_pPrintSetting->m_nPrintFontWidth,
					nDirectY * ( nOffY + nLineHeight * i + ( m_pPrintSetting->m_nPrintFontHeight - nAscent ) ), 0, NULL,
					szLineNum, nLineCols, m_pnDx
				);
				/* 行番号区切り  0=なし 1=縦線 2=任意 */
				if( 1 == m_cEditDoc.GetDocumentAttribute().m_nLineTermType ){
//					HPEN	hPen, hPenOld;
					::MoveToEx( hdc,
//						m_nViewAlignLeft - 4,
						nOffX +
						( m_bPreview_EnableColms * m_pPrintSetting->m_nPrintFontWidth + m_pPrintSetting->m_nPrintDanSpace )
						* nDan +
						m_nPreview_LineNumberColmns * m_pPrintSetting->m_nPrintFontWidth * (nDan) +
						( m_nPreview_LineNumberColmns ) * m_pPrintSetting->m_nPrintFontWidth
						- (m_pPrintSetting->m_nPrintFontWidth / 2 )
						,
						nDirectY * ( nOffY + nLineHeight * i ),
						NULL );
					::LineTo( hdc,
//						m_nViewAlignLeft - 4,
						nOffX +
						( m_bPreview_EnableColms * m_pPrintSetting->m_nPrintFontWidth + m_pPrintSetting->m_nPrintDanSpace )
						* nDan +
						m_nPreview_LineNumberColmns * m_pPrintSetting->m_nPrintFontWidth * (nDan) +
						( m_nPreview_LineNumberColmns ) * m_pPrintSetting->m_nPrintFontWidth
						- (m_pPrintSetting->m_nPrintFontWidth / 2 )
						,
						nDirectY * ( nOffY + nLineHeight * i + nLineHeight )
					);
				}

			}

			if( 0 <= nLineLen - 1 && ( pLine[nLineLen - 1] == CR || pLine[nLineLen - 1] == LF ) ){
				nLineLen--;
				if( 0 <= nLineLen - 1 && ( pLine[nLineLen - 1] == CR || pLine[nLineLen - 1] == LF ) ){
					nLineLen--;
				}
			}
			if( 0 == nLineLen ){
				continue;
			}

			/* 印刷／プレビュー 行描画 */
			Print_DrawLine(
				hdc,
				nOffX + ( m_bPreview_EnableColms * m_pPrintSetting->m_nPrintFontWidth + m_pPrintSetting->m_nPrintDanSpace ) * nDan + m_nPreview_LineNumberColmns * m_pPrintSetting->m_nPrintFontWidth * (nDan + 1),
				nDirectY * ( nOffY + nLineHeight * i ),
				pLine,
				nLineLen,
				hFontZen
			);
		}
	}
	return;
}





int CALLBACK MyEnumFontFamProc(
	ENUMLOGFONT*	pelf,		// pointer to logical-font data
	NEWTEXTMETRIC*	pntm,		// pointer to physical-font data
	int				nFontType,	// type of font
	LPARAM			lParam 		// address of application-defined data
)
{
	CEditWnd* pCEditWnd;
	pCEditWnd = (CEditWnd*)lParam;
	if( 0 == strcmp( pelf->elfLogFont.lfFaceName, pCEditWnd->m_pPrintSetting->m_szPrintFontFaceHan ) ){
		pCEditWnd->m_lfPreviewHan = pelf->elfLogFont;
		pCEditWnd->m_lfPreviewHan.lfHeight	= pCEditWnd->m_pPrintSetting->m_nPrintFontHeight;
		pCEditWnd->m_lfPreviewHan.lfWidth	= pCEditWnd->m_pPrintSetting->m_nPrintFontWidth;
		strcpy(pCEditWnd->m_lfPreviewHan.lfFaceName, pCEditWnd->m_pPrintSetting->m_szPrintFontFaceHan);
//		return 0;
	}
	if( 0 == strcmp( pelf->elfLogFont.lfFaceName, pCEditWnd->m_pPrintSetting->m_szPrintFontFaceZen ) ){
		pCEditWnd->m_lfPreviewZen = pelf->elfLogFont;
		pCEditWnd->m_lfPreviewZen.lfHeight	= pCEditWnd->m_pPrintSetting->m_nPrintFontHeight;
		pCEditWnd->m_lfPreviewZen.lfWidth	= pCEditWnd->m_pPrintSetting->m_nPrintFontWidth;
		strcpy(pCEditWnd->m_lfPreviewZen.lfFaceName, pCEditWnd->m_pPrintSetting->m_szPrintFontFaceZen );
//		return 0;
	}


//	/* LOGFONT */
//	if( FIXED_PITCH & pelf->elfLogFont.lfPitchAndFamily ){
//		MYTRACE( pelf->elfLogFont.lfFaceName, "%s\n\n", pelf->elfLogFont.lfFaceName );
//	}
	return 1;
}




/* 印刷設定の反映 */
void CEditWnd::OnChangePrintSetting( void )
{
	char	szErrMsg[1024];
//	int		nToolBarHeight;
	RECT	rc;
	HDC		hdc;
//	POINT	po;
	int		i;
	hdc = ::GetDC( m_hWnd );
	::EnumFontFamilies(
		hdc,
		NULL,
		(FONTENUMPROC)MyEnumFontFamProc,
		(LPARAM)this
	);
	/* 印刷プレビュー表示情報 */
//	m_nPreview_Zoom = 100;				/* 印刷プレビュー倍率 */
	m_nPreview_CurPage = 1;				/* 印刷プレビュー：ページ */
	m_nPreview_AllPageNum = 1;			/* 印刷プレビュー：全ページ数 */
//	m_nPreviewVScrollPos = 0;
//	m_nPreviewHScrollPos = 0;
	m_nPreview_LineNumberColmns = 0;	/* 行番号エリアの幅(文字数) */
	/* 行番号を表示するか */
	if( m_pPrintSetting->m_bPrintLineNumber ){
		/* 行番号表示に必要な桁数を計算 */
		m_nPreview_LineNumberColmns = m_cEditDoc.m_cEditViewArr[0].DetectWidthOfLineNumberArea_calculate();
	}
	/* 現在のページ設定の、用紙サイズと用紙方向を反映させる */
	m_pPrintSetting->m_mdmDevMode.dmPaperSize = m_pPrintSetting->m_nPrintPaperSize;
	m_pPrintSetting->m_mdmDevMode.dmOrientation = m_pPrintSetting->m_nPrintPaperOrientation;
	m_pPrintSetting->m_mdmDevMode.dmFields |= ( DM_ORIENTATION | DM_PAPERSIZE );
	if( m_pPrintSetting->m_mdmDevMode.dmFields & DM_PAPERLENGTH ){
		m_pPrintSetting->m_mdmDevMode.dmFields &= (~DM_PAPERLENGTH );
	}
	if( m_pPrintSetting->m_mdmDevMode.dmFields & DM_PAPERWIDTH ){
		m_pPrintSetting->m_mdmDevMode.dmFields &= (~DM_PAPERWIDTH);
	}
	if( m_pPrintSetting->m_mdmDevMode.dmFields & DM_PAPERLENGTH ){
		m_pPrintSetting->m_mdmDevMode.dmFields &= (~DM_PAPERLENGTH );
	}
	if( m_pPrintSetting->m_mdmDevMode.dmFields & DM_PAPERWIDTH ){
		m_pPrintSetting->m_mdmDevMode.dmFields &= (~DM_PAPERWIDTH);
	}

	/* 印刷/プレビューに必要な情報を取得 */
	if( FALSE == CPrint::GetPrintMetrics(
		&m_pPrintSetting->m_mdmDevMode,	/* プリンタ設定 DEVMODE用*/
		&m_nPreview_PaperAllWidth,		/* 用紙幅 */
		&m_nPreview_PaperAllHeight,		/* 用紙高さ */
		&m_nPreview_PaperWidth,			/* 用紙印刷有効幅 */
		&m_nPreview_PaperHeight,		/* 用紙印刷有効高さ */
		&m_nPreview_PaperOffsetLeft,	/* 印刷可能位置左端 */
		&m_nPreview_PaperOffsetTop,		/* 印刷可能位置上端 */
		szErrMsg						/* エラーメッセージ格納場所 */
	) ){
//		::TextOut( hdc, 30, nY, szErrMsg, lstrlen( szErrMsg ) );
//		nY += 20;
//		goto end_of_func;
		/* エラーの場合、A4縦(210mm×297mm)で初期化 */
		m_nPreview_PaperAllWidth = 210 * 10;	/* 用紙幅 */
		m_nPreview_PaperAllHeight = 297 * 10;	/* 用紙高さ */
		m_nPreview_PaperWidth = 210 * 10;		/* 用紙印刷有効幅 */
		m_nPreview_PaperHeight = 297 * 10;		/* 用紙印刷有効高さ */
		m_nPreview_PaperOffsetLeft = 0;			/* 印刷可能位置左端 */
		m_nPreview_PaperOffsetTop = 0;			/* 印刷可能位置上端 */
	}else{
		if( m_pPrintSetting->m_nPrintPaperSize != m_pPrintSetting->m_mdmDevMode.dmPaperSize ){
			char	szPaperNameOld[256];
			char	szPaperNameNew[256];
			/* 用紙の名前を取得 */
			CPrint::GetPaperName( m_pPrintSetting->m_nPrintPaperSize , (char*)szPaperNameOld );
			CPrint::GetPaperName( m_pPrintSetting->m_mdmDevMode.dmPaperSize , (char*)szPaperNameNew );

			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST, GSTR_APPNAME,
				"現在のプリンタ %s では、\n指定された用紙 %s は使用できません。\n利用可能な用紙 %s に変更しました。",
				m_pPrintSetting->m_mdmDevMode.m_szPrinterDeviceName,
				szPaperNameOld, szPaperNameNew
			);
		}
		/* 現在のページ設定の、用紙サイズと用紙方向を反映させる */
		m_pPrintSetting->m_nPrintPaperSize = m_pPrintSetting->m_mdmDevMode.dmPaperSize;
	}
	m_nPreview_PaperOffsetRight = m_nPreview_PaperAllWidth - (m_nPreview_PaperOffsetLeft + m_nPreview_PaperWidth);		/* 用紙余白右端(1/10mm単位) */
	m_nPreview_PaperOffsetBottom = m_nPreview_PaperAllHeight - ( m_nPreview_PaperOffsetTop + m_nPreview_PaperHeight );	/* 用紙余白下端(1/10mm単位) */

	m_nPreview_ViewMarginLeft = 8 * 10;		/* 印刷プレビュー：ビュー左端と用紙の間隔(1/10mm単位) */
	m_nPreview_ViewMarginTop = 8 * 10;		/* 印刷プレビュー：ビュー左端と用紙の間隔(1/10mm単位) */
//	m_nPreview_ViewMarginLeft = 50 * 10;	/* 印刷プレビュー：ビュー左端と用紙の間隔(1/10mm単位) */
//	m_nPreview_ViewMarginTop = 25 * 10;		/* 印刷プレビュー：ビュー左端と用紙の間隔(1/10mm単位) */

	m_bPreview_EnableColms =
		( m_nPreview_PaperAllWidth - m_pPrintSetting->m_nPrintMarginLX - m_pPrintSetting->m_nPrintMarginRX
		- ( m_pPrintSetting->m_nPrintDansuu - 1 ) * m_pPrintSetting->m_nPrintDanSpace
		- ( m_pPrintSetting->m_nPrintDansuu ) * ( ( m_nPreview_LineNumberColmns /*+ (m_nPreview_LineNumberColmns?1:0)*/ ) * m_pPrintSetting->m_nPrintFontWidth )
		) / m_pPrintSetting->m_nPrintFontWidth / m_pPrintSetting->m_nPrintDansuu;	/* 印字可能桁数/ページ */
	m_bPreview_EnableLines = ( m_nPreview_PaperAllHeight - m_pPrintSetting->m_nPrintMarginTY - m_pPrintSetting->m_nPrintMarginBY ) / ( m_pPrintSetting->m_nPrintFontHeight + ( m_pPrintSetting->m_nPrintFontHeight * m_pPrintSetting->m_nPrintLineSpacing / 100 ) ) - 4;	/* 印字可能行数/ページ */

	/* 印刷用のレイアウト管理情報の初期化 */
	m_CLayoutMgr_Print.Create( &m_cEditDoc.m_cDocLineMgr );

	/* 印刷用のレイアウト情報の変更 */
	m_CLayoutMgr_Print.SetLayoutInfo(
		m_bPreview_EnableColms/*m_cEditDoc.GetDocumentAttribute().m_nMaxLineSize*/,
		m_pPrintSetting->m_bPrintWordWrap/*FALSE*//*TRUE*//*m_cEditDoc.GetDocumentAttribute().m_bWordWrap*/,	/* 英文ワードラップをする */
		m_cEditDoc.GetDocumentAttribute().m_nTabSpace,
		""/*m_cEditDoc.GetDocumentAttribute().m_szLineComment*/,		/* 行コメントデリミタ */
		""/*m_cEditDoc.GetDocumentAttribute().m_szLineComment2*/,		/* 行コメントデリミタ2 */
		""/*m_cEditDoc.GetDocumentAttribute().m_szLineComment3*/,		/* 行コメントデリミタ3 */	//Jun. 01, 2001 JEPRO 追加
		""/*m_cEditDoc.GetDocumentAttribute().m_szBlockCommentFrom*/,	/* ブロックコメントデリミタ(From) */
		""/*m_cEditDoc.GetDocumentAttribute().m_szBlockCommentTo*/,		/* ブロックコメントデリミタ(To) */
//#ifdef COMPILE_BLOCK_COMMENT2	//@@@ 2001.03.10 by MIK
		""/*m_cEditDoc.GetDocumentAttribute().m_szBlockCommentFrom2*/,	/* ブロックコメントデリミタ2(From) */
		""/*m_cEditDoc.GetDocumentAttribute().m_szBlockCommentTo2*/,	/* ブロックコメントデリミタ2(To) */
//#endif
		0/*m_cEditDoc.GetDocumentAttribute().m_nStringType*/,			/* 文字列区切り記号エスケープ方法  0=[\"][\'] 1=[""][''] */
		TRUE,
		NULL,/*hwndProgress*/
		FALSE/*m_cEditDoc.GetDocumentAttribute().m_ColorInfoArr[COLORIDX_SSTRING].m_bDisp*/,/* シングルクォーテーション文字列を表示する */
		FALSE/*m_cEditDoc.GetDocumentAttribute().m_ColorInfoArr[COLORIDX_WSTRING].m_bDisp*/	/* ダブルクォーテーション文字列を表示する */
	);
	m_nAllPageNum = m_CLayoutMgr_Print.GetLineCount() / ( m_bPreview_EnableLines * m_pPrintSetting->m_nPrintDansuu );		/* 全ページ数 */
	if( 0 < m_CLayoutMgr_Print.GetLineCount() % ( m_bPreview_EnableLines * m_pPrintSetting->m_nPrintDansuu ) ){
		m_nAllPageNum++;
	}
	if( m_nAllPageNum <= m_nCurPageNum ){	/* 現在のページ */
		m_nCurPageNum = 0;
	}
	for( i = 0; i < ( sizeof( m_pnDx ) / sizeof( m_pnDx[0]) ); ++i ){
		m_pnDx[i] = m_pPrintSetting->m_nPrintFontWidth;
	}
//	/* 印刷プレビュー 操作バー 作成 */
//	CreatePrintPreviewBar();
//
//	/* 印刷プレビュー 操作バー */
//	nToolBarHeight = 0;
//	if( NULL != m_hwndPrintPreviewBar ){
//		::GetWindowRect( m_hwndPrintPreviewBar, &rc );
//		nToolBarHeight = rc.bottom - rc.top;
//	}
	::SetMapMode( hdc, MM_LOMETRIC ); //MM_HIMETRIC それぞれの論理単位は、0.01 mm にマップされます
	::SetMapMode( hdc, MM_ANISOTROPIC );
	/* WM_SIZE 処理 */
	::GetClientRect( m_hWnd, &rc );
	OnSize( SIZE_RESTORED, MAKELONG( rc.right - rc.left, rc.bottom - rc.top ) );
	::ReleaseDC( m_hWnd, hdc );
	/* プレビュー ページ指定 */
	OnPreviewGoPage( m_nCurPageNum );
	return;
}





LRESULT CEditWnd::OnLButtonDown( WPARAM wParam, LPARAM lParam )
{
	WPARAM		fwKeys;
	int			xPos;
	int			yPos;
	fwKeys = wParam;		// key flags
	xPos = LOWORD(lParam);	// horizontal position of cursor
	yPos = HIWORD(lParam);	// vertical position of cursor
	m_nDragPosOrgX = xPos;
	m_nDragPosOrgY = yPos;
	SetCapture( m_hWnd );
	m_bDragMode = TRUE;

	return 0;
}

LRESULT CEditWnd::OnLButtonUp( WPARAM wParam, LPARAM lParam )
{
	m_bDragMode = FALSE;
	ReleaseCapture();
	::InvalidateRect( m_hWnd, NULL, TRUE );
	return 0;
}


LRESULT CEditWnd::OnMouseMove( WPARAM wParam, LPARAM lParam )
{
	/* 手カーソル */
	::SetCursor( ::LoadCursor( m_hInstance, MAKEINTRESOURCE( IDC_CURSOR_HAND ) ) );
	WPARAM		fwKeys;
	int			xPos;
	int			yPos;
	SCROLLINFO	siV;
	SCROLLINFO	siH;
	int			nNowPosX;
	int			nNowPosY;
	int			nMoveX;
	int			nMoveY;
	int			nNewPosX;
	int			nNewPosY;
	RECT		rc;
	POINT		po;
	if( !m_bDragMode ){
		return 0;
	}
	fwKeys = wParam;			// key flags
	xPos = LOWORD( lParam );	// horizontal position of cursor
	yPos = HIWORD( lParam );	// vertical position of cursor
	GetClientRect( m_hWnd, &rc );
	po.x = xPos;
	po.y = yPos;
	if( !PtInRect( &rc, po ) ){
		return 0;
	}
	siV.cbSize = sizeof( SCROLLINFO );
	siV.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	GetScrollInfo( m_hwndVScrollBar, SB_CTL, (SCROLLINFO*)&siV );
	if( m_SCROLLBAR_VERT ){
		nNowPosY = GetScrollPos( m_hwndVScrollBar, SB_CTL );
		nMoveY = m_nDragPosOrgY - yPos;
		nNewPosY = nNowPosY + nMoveY;
		if( nNewPosY < 0 ){
			nNewPosY = 0;
		}else
		if( nNewPosY > (int)(siV.nMax - siV.nPage + 1) ){
			nNewPosY = (int)(siV.nMax - siV.nPage + 1);
		}
		nMoveY = nNowPosY - nNewPosY;
		SetScrollPos( m_hwndVScrollBar, SB_CTL, nNewPosY, TRUE );
		m_nPreviewVScrollPos = -1 * nNewPosY;
	}else{
		nMoveY = 0;
	}
	siH.cbSize = sizeof( SCROLLINFO );
	siH.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	GetScrollInfo( m_hwndHScrollBar, SB_CTL, (SCROLLINFO*)&siH );
	if( m_SCROLLBAR_HORZ ){
		nNowPosX = GetScrollPos( m_hwndHScrollBar, SB_CTL );
		nMoveX = m_nDragPosOrgX - xPos;
		nNewPosX = nNowPosX + nMoveX;
		if( nNewPosX < 0 ){
			nNewPosX = 0;
		}else
		if( nNewPosX > (int)(siH.nMax - siH.nPage + 1) ){
			nNewPosX = (int)(siH.nMax - siH.nPage + 1);
		}
		nMoveX = nNowPosX - nNewPosX;
		SetScrollPos( m_hwndHScrollBar, SB_CTL, nNewPosX, TRUE );
		m_nPreviewHScrollPos = nNewPosX;
	}else{
		nMoveX = 0;
	}

	m_nDragPosOrgX = xPos;
	m_nDragPosOrgY = yPos;
	/* 描画 */
	ScrollWindowEx( m_hWnd, nMoveX, nMoveY, NULL, NULL, NULL , NULL, SW_ERASE | SW_INVALIDATE );
	::UpdateWindow( m_hWnd );
	return 0;
}




LRESULT CEditWnd::OnMouseWheel( WPARAM wParam, LPARAM lParam )
{
	WORD	fwKeys;
	short	zDelta;
	short	xPos;
	short	yPos;
	int		nScrollCode;
	int		i;

	fwKeys = LOWORD(wParam);			// key flags
	zDelta = (short) HIWORD(wParam);	// wheel rotation
	xPos = (short) LOWORD(lParam);		// horizontal position of pointer
	yPos = (short) HIWORD(lParam);		// vertical position of pointer
//	MYTRACE( "CEditWnd::DispatchEvent() WM_MOUSEWHEEL fwKeys=%xh zDelta=%dxPos=%dyPos=%d\n", fwKeys, zDelta, xPos, yPos );
	/* 印刷プレビューモードか */
	if( FALSE == m_cEditDoc.m_bPrintPreviewMode ){
		/* メッセージの配送 */
		return m_cEditDoc.DispatchEvent( m_hWnd, WM_MOUSEWHEEL, wParam, lParam );
	}
	if( 0 < zDelta ){
		nScrollCode = SB_LINEUP;
//		nAdd = (-16);
	}else{
		nScrollCode = SB_LINEDOWN;
//		nAdd = (16);
	}
//	memset( &si, 0, sizeof( SCROLLINFO ) );
//	si.cbSize = sizeof( SCROLLINFO );
//	si.fMask = SIF_ALL;
//	if( 0 == ::GetScrollInfo( m_hwndVScrollBar, SB_CTL, (SCROLLINFO*)&si ) ){
//		return 0;
//	}
//	nPos = si.nPos;

	for( i = 0; i < 3; ++i ){
//		if( nPos >= si.nMin && nPos < si.nMax ){
//			/* 印刷プレビュー 垂直スクロールバーメッセージ処理 WM_VSCROLL */
//			OnVScroll( nScrollCode, nPos, m_hwndVScrollBar );
			::PostMessage( m_hWnd, WM_VSCROLL, MAKELONG( nScrollCode, 0 ), (WPARAM)m_hwndVScrollBar );
			/* 処理中のユーザー操作を可能にする */
			if( !::BlockingHook( NULL ) ){
				return -1;
			}
			::Sleep( 1 );
//		}else{
//			break;
//		}
//		nPos+=nAdd;
	}
	return 0;
}










/* 印刷実行 */
void CEditWnd::OnPrint( void )
{
	HANDLE		hPrinter;
	HDC			hdc;
	char		szJobName[256 + 1];
	char		szProgress[100];
	char		szErrMsg[1024];
	int			nDirectY;
//	LOGFONT		lf;
	HFONT		hFont, hFontOld;
	int			i;
	char		szWork[260];
	HFONT		hFontZen, hFontZenOld;
	int			nFrom;
	int			nTo;

	if( 0 == m_nAllPageNum ){
		::MessageBeep( MB_ICONHAND );
		return;
	}

	CDlgPrintPage cCDlgPrintPage;
	cCDlgPrintPage.m_bAllPage = TRUE;
	cCDlgPrintPage.m_nPageMin = 1;
	cCDlgPrintPage.m_nPageMax = m_nAllPageNum;
	cCDlgPrintPage.m_nPageFrom = 1;
	cCDlgPrintPage.m_nPageTo = m_nAllPageNum;
	if( FALSE == cCDlgPrintPage.DoModal( m_hInstance, m_hWnd, NULL ) ){
		return;
	}


	CDlgCancel	cDlgPrinting;
	cDlgPrinting.DoModeless( m_hInstance, m_hWnd, IDD_PRINTING );


	if( 0 == lstrlen( m_cEditDoc.m_szFilePath ) ){	/* 現在編集中のファイルのパス */
		strcpy( szJobName, "無題" );
	}else{
		char	szFileName[_MAX_FNAME];
		char	szExt[_MAX_EXT];
		_splitpath( m_cEditDoc.m_szFilePath, NULL, NULL, szFileName, szExt );
		wsprintf( szJobName, "%s%s", szFileName, szExt );
	}
	::SetDlgItemText( cDlgPrinting.m_hWnd, IDC_STATIC_JOBNAME, szJobName );
	::EnableWindow( m_hWnd, FALSE );



	/* 印刷 ジョブ開始 */
	if( FALSE == CPrint::PrintOpen(
		szJobName,
		&m_pPrintSetting->m_mdmDevMode,	/* プリンタ設定 DEVMODE用*/
		&hPrinter,
		&hdc,
		szErrMsg						/* エラーメッセージ格納場所 */
	) ){
//		MYTRACE( "%s\n", szErrMsg );
	}

	hFont = CreateFontIndirect( &m_lfPreviewHan );
	hFontZen = CreateFontIndirect( &m_lfPreviewZen );

	if( FALSE == cCDlgPrintPage.m_bAllPage ){
		nFrom = cCDlgPrintPage.m_nPageFrom;
		nTo   = cCDlgPrintPage.m_nPageTo;
	}else{
		nFrom = 1;
		nTo = m_nAllPageNum;
	}
	for( i = nFrom - 1; i < nTo; ++i ){
		//	Jun. 18, 2001 genta ページ番号表示の計算ミス修正
		sprintf( szProgress, "%d/%d", i - (nFrom - 1) + 1/*i*/, nTo - (nFrom - 1)/*m_nAllPageNum*/ );
		::SetDlgItemText( cDlgPrinting.m_hWnd, IDC_STATIC_PROGRESS, szProgress );

		/* 印刷 ページ開始 */
		CPrint::PrintStartPage( hdc );

		/* マッピングモードの変更 */
		::SetMapMode(hdc, MM_LOMETRIC );					//MM_HIMETRIC それぞれの論理単位は、0.01 mm にマップされます
		::SetMapMode( hdc, MM_ANISOTROPIC/*MM_HIMETRIC*/ );	//MM_HIMETRIC それぞれの論理単位は、0.01 mm にマップされます
		nDirectY = -1;

		/* 印刷フォント設定 */
		hFontOld = (HFONT)::SelectObject( hdc, hFont );
		/* ファイル名の表示 */
		if( 0 == lstrlen( m_cEditDoc.m_szFilePath ) ){	/* 現在編集中のファイルのパス */
			strcpy( szWork, "(無題)" );
		}else{
			char	szFileName[_MAX_FNAME];
			char	szExt[_MAX_EXT];
			_splitpath( m_cEditDoc.m_szFilePath, NULL, NULL, szFileName, szExt );
			wsprintf( szWork, "%s%s", szFileName, szExt );
		}
		hFontZenOld = (HFONT)::SelectObject( hdc, hFontZen );
		::TextOut( hdc,
			m_pPrintSetting->m_nPrintMarginLX - m_nPreview_PaperOffsetLeft,
			nDirectY * ( m_pPrintSetting->m_nPrintMarginTY - m_nPreview_PaperOffsetTop ),
			szWork, lstrlen( szWork )
		);
		::SelectObject( hdc, hFontZenOld );
		/* 印刷/印刷プレビュー ページテキストの描画 */
		DrawPageText(
			hdc,
			m_pPrintSetting->m_nPrintMarginLX - m_nPreview_PaperOffsetLeft,
			m_pPrintSetting->m_nPrintMarginTY - m_nPreview_PaperOffsetTop + 2 * ( m_pPrintSetting->m_nPrintFontHeight + (m_pPrintSetting->m_nPrintFontHeight * m_pPrintSetting->m_nPrintLineSpacing / 100) ),
			i,
			hFontZen,
			&cDlgPrinting
		);
		/* ページ番号の表示 */
		wsprintf( szWork, "- %d -", i + 1 );
		::TextOut( hdc,
//			  m_pPrintSetting->m_nPrintMarginLX
//			- m_nPreview_PaperOffsetLeft
			+ m_nPreview_PaperWidth / 2
			- (5 * 10),
			nDirectY * ( m_nPreview_PaperAllHeight - m_nPreview_PaperOffsetTop - m_pPrintSetting->m_nPrintMarginBY - m_pPrintSetting->m_nPrintFontHeight/* - m_nPreview_PaperOffsetTop*/ ),
//TEST			nDirectY * ( m_pPrintSetting->m_nPrintMarginTY - m_nPreview_PaperOffsetTop ),
			szWork, lstrlen( szWork )
		);
	//	for( i = 0; i < 10; ++i ){
	//		::MoveToEx( hdc, 1000, nDirectY * ( 1000 * i ) , NULL );
	//		::LineTo( hdc, 5000, nDirectY * ( 1000 * i ) );
	//	}
		/* 印刷フォント解除 */
		::SelectObject( hdc, hFontOld );
		/* 印刷 ページ終了 */
		CPrint::PrintEndPage( hdc );

		/* 中断ボタン押下チェック */
		if( cDlgPrinting.IsCanceled() ){
			break;
		}
	}
	sprintf( szProgress, "%d/%d", i - nFrom - 1/*i*/, nTo - nFrom + 1/*m_nAllPageNum*/ );
	::SetDlgItemText( cDlgPrinting.m_hWnd, IDC_STATIC_PROGRESS, szProgress );

	/* 印刷 ジョブ終了 */
	CPrint::PrintClose( hPrinter, hdc );

	::DeleteObject( hFontZen );
	::DeleteObject( hFont );


	::EnableWindow( m_hWnd, TRUE );
	cDlgPrinting.CloseDialog( 0 );

	return;
}



/* 印刷ページ設定 */
BOOL CEditWnd::OnPrintPageSetting( void )
{
	/* 印刷設定 */
	CDlgPrintSetting	cDlgPrintSetting;
	BOOL				bRes;
	PRINTSETTING		PrintSettingArr[MAX_PRINTSETTINGARR];
	int					i;
	int					nCurrentPrintSetting;
	for( i = 0; i < MAX_PRINTSETTINGARR; ++i ){
		PrintSettingArr[i] = m_pShareData->m_PrintSettingArr[i];
	}

//	cDlgPrintSetting.Create( m_hInstance, m_hWnd );
	nCurrentPrintSetting = m_cEditDoc.GetDocumentAttribute().m_nCurrentPrintSetting;
	bRes = cDlgPrintSetting.DoModal(
		m_hInstance,
		m_hwndPrintPreviewBar/*m_hWnd*/,
		&nCurrentPrintSetting, /* 現在選択している印刷設定 */
		PrintSettingArr
	);

	if( TRUE == bRes ){
		/* 現在選択されているページ設定の番号が変更されたか */
		if( nCurrentPrintSetting !=
			m_pShareData->m_Types[m_cEditDoc.GetDocumentType()].m_nCurrentPrintSetting
		){
//			/* 変更フラグ(タイプ別設定) */
//			m_pShareData->m_nTypesModifyArr[m_cEditDoc.m_nSettingType] = TRUE;
			m_cEditDoc.GetDocumentAttribute().m_nCurrentPrintSetting = nCurrentPrintSetting;
		}

//		/* それぞれのページ設定の内容が変更されたか */
//		if( 0 != memcmp(
//			&m_pShareData->m_PrintSettingArr,
//			&PrintSettingArr,
//			sizeof( m_pShareData->m_PrintSettingArr ) )
//		){
//			m_pShareData->m_bPrintSettingModify = TRUE;	/* 変更フラグ(印刷の全体) */
			for( i = 0; i < MAX_PRINTSETTINGARR; ++i ){
//				if( 0 != memcmp(
//					&m_pShareData->m_PrintSettingArr[i],
//					&PrintSettingArr[i],
//					sizeof( m_pShareData->m_PrintSettingArr[i] ) )
//				){
//					/* 変更フラグ(印刷設定ごと) */
//					m_pShareData->m_bPrintSettingModifyArr[i] = TRUE;
					m_pShareData->m_PrintSettingArr[i] = PrintSettingArr[i];
//				}
			}
//		}

		/* 現在の印刷設定 */
		m_pPrintSetting = &m_pShareData->m_PrintSettingArr[m_cEditDoc.GetDocumentAttribute().m_nCurrentPrintSetting];

		m_pPrintSetting->m_mdmDevMode.dmPaperSize = m_pPrintSetting->m_nPrintPaperSize;
		m_pPrintSetting->m_mdmDevMode.dmOrientation = m_pPrintSetting->m_nPrintPaperOrientation;

		/* 印刷プレビュー スクロールバー初期化 */

		m_nPreviewVScrollPos = 0;
		m_nPreviewHScrollPos = 0;
		InitPreviewScrollBar();

		/* 印刷設定の反映 */
		OnChangePrintSetting( );

		::InvalidateRect( m_hWnd, NULL, TRUE );
	}
	::UpdateWindow( m_hwndPrintPreviewBar );
	return bRes;
}


/*[EOF]*/
