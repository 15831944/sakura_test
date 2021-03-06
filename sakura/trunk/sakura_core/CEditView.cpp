/*!	@file
	@brief 文書ウィンドウの管理

	@author Norio Nakatani
	@date	1998/03/13 作成
	@date   2005/09/02 D.S.Koba GetSizeOfCharで書き換え
*/
/*
	Copyright (C) 1998-2002, Norio Nakatani
	Copyright (C) 2000, genta, JEPRO, MIK
	Copyright (C) 2001, genta, GAE, MIK, hor, asa-o, Stonee, Misaka, novice, YAZAKI
	Copyright (C) 2002, YAZAKI, hor, aroka, MIK, Moca, minfu, KK, novice, ai, Azumaiya, genta
	Copyright (C) 2003, MIK, ai, ryoji, Moca, wmlhq, genta
	Copyright (C) 2004, genta, Moca, novice, naoh, isearch, fotomo
	Copyright (C) 2005, genta, MIK, novice, aroka, D.S.Koba, かろと, Moca
	Copyright (C) 2006, Moca, aroka, ryoji, fon, genta, maru
	Copyright (C) 2007, ryoji, じゅうじ, maru, genta, Moca, nasukoji, D.S.Koba
	Copyright (C) 2008, ryoji, nasukoji, bosagami, Moca, genta
	Copyright (C) 2009, nasukoji, ryoji, syat
	Copyright (C) 2010, ryoji, Moca

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/

#include "StdAfx.h"
#include <stdlib.h>
#include <time.h>
#include <io.h>
#include <process.h> // _beginthreadex
#include "CEditApp.h"
#include "CEditView.h"
#include "Debug.h"
#include "CRunningTimer.h"
#include "charcode.h"
#include "mymessage.h"
#include "CWaitCursor.h"
#include "CEditWnd.h"
#include "CDlgCancel.h"
#include "etc_uty.h"
#include "module.h"
#include "os.h"
#include "CLayout.h"/// 2002/2/3 aroka
#include "COpe.h"///
#include "COpeBlk.h"///
#include "CDropTarget.h"///
#include "CSplitBoxWnd.h"///
#include "CRegexKeyword.h"///	//@@@ 2001.11.17 add MIK
#include "CMarkMgr.h"///
#include "COsVersionInfo.h"
#include "CFileLoad.h" // 2002/08/30 Moca
#include "CMemoryIterator.h"	// @@@ 2002.09.28 YAZAKI
#include "my_icmp.h" // 2002/11/30 Moca 追加
#include <vector> // 2008/02/16 bosagami add
#include <algorithm> // 2008/02/16 bosagami add
#include <assert.h>

const int STRNCMP_MAX = 100;	/* MAXキーワード長：strnicmp文字列比較最大値(CEditView::KeySearchCore) */	// 2006.04.10 fon

CEditView*	g_m_pcEditView;
LRESULT CALLBACK EditViewWndProc( HWND, UINT, WPARAM, LPARAM );
VOID CALLBACK EditViewTimerProc( HWND, UINT, UINT_PTR, DWORD );

#define IDT_ROLLMOUSE	1

/* リソースヘッダー */
#define	 BFT_BITMAP		0x4d42	  /* 'BM' */

/* リソースがDIBかどうかを判断するマクロ */
#define	 ISDIB(bft)		((bft) == BFT_BITMAP)

/* 指定された値を最も近いバイト境界に整列させるマクロ */
#define	 WIDTHBYTES(i)	((i+31)/32*4)


//@@@2002.01.14 YAZAKI staticにしてメモリの節約（(10240+10) * 3 バイト）
int CEditView::m_pnDx[MAXLINEKETAS + 10];



/*
|| ウィンドウプロシージャ
||
*/

LRESULT CALLBACK EditViewWndProc(
	HWND		hwnd,	// handle of window
	UINT		uMsg,	// message identifier
	WPARAM		wParam,	// first message parameter
	LPARAM		lParam 	// second message parameter
)
{
	CEditView*	pCEdit;
	switch( uMsg ){
	case WM_CREATE:
		pCEdit = ( CEditView* )g_m_pcEditView;
		return pCEdit->DispatchEvent( hwnd, uMsg, wParam, lParam );
	default:
		pCEdit = ( CEditView* )::GetWindowLongPtr( hwnd, 0 );
		if( NULL != pCEdit ){
			//	May 16, 2000 genta
			//	From Here
			if( uMsg == WM_COMMAND ){
				::SendMessage( ::GetParent( pCEdit->m_hwndParent ), WM_COMMAND, wParam,  lParam );
			}
			else{
				return pCEdit->DispatchEvent( hwnd, uMsg, wParam, lParam );
			}
			//	To Here
		}
		return ::DefWindowProc( hwnd, uMsg, wParam, lParam );
	}
}


/*
||  タイマーメッセージのコールバック関数
||
||	現在は、マウスによる領域選択時のスクロール処理のためにタイマーを使用しています。
*/
VOID CALLBACK EditViewTimerProc(
	HWND hwnd,		// handle of window for timer messages
	UINT uMsg,		// WM_TIMER message
	UINT_PTR idEvent,	// timer identifier
	DWORD dwTime 	// current system time
)
{
	CEditView*	pCEditView;
	pCEditView = ( CEditView* )::GetWindowLongPtr( hwnd, 0 );
	if( NULL != pCEditView ){
		pCEditView->OnTimer( hwnd, uMsg, idEvent, dwTime );
	}
	return;
}



//	@date 2002.2.17 YAZAKI CShareDataのインスタンスは、CProcessにひとつあるのみ。
CEditView::CEditView( CEditWnd* pcEditWnd )
: m_pcEditWnd( pcEditWnd )
, m_cHistory( new CAutoMarkMgr )
, m_cRegexKeyword( NULL )				// 2007.04.08 ryoji
, m_bActivateByMouse( FALSE )	// 2007.10.02 nasukoji
{
	m_pcSMacroMgr = CEditApp::getInstance()->m_pcSMacroMgr;
}


// 2007.10.23 kobake コンストラクタ内の処理をすべてCreateに移しました。(初期化処理が不必要に分散していたため)
BOOL CEditView::Create(
	HINSTANCE	hInstance,
	HWND		hwndParent,		//!< 親
	CEditDoc*	pcEditDoc,		//!< 参照するドキュメント
	int			nMyIndex,		//!< ビューのインデックス
	BOOL		bShow			//!< 作成時に表示するかどうか
)
{
	m_pcViewFont = m_pcEditWnd->m_pcViewFont;

	m_bDrawSWITCH = true;
	m_pcDropTarget = new CDropTarget( this );
	m_bDragMode = FALSE;					/* 選択テキストのドラッグ中か */
	m_bCurSrchKeyMark = false;				/* 検索文字列 */
	//	Jun. 27, 2001 genta
	m_szCurSrchKey[0] = '\0';
	m_sCurSearchOption.Reset();				// 検索／置換 オプション

	m_bPrevCommand = 0;
	m_nMyIndex = 0;

	//	Dec. 4, 2002 genta
	//	メニューバーへのメッセージ表示機能はCEditWndへ移管

	/* 共有データ構造体のアドレスを返す */
	m_pShareData = CShareData::getInstance()->GetShareData();
	m_bCommandRunning = FALSE;	/* コマンドの実行中 */
	m_pcOpeBlk = NULL;			/* 操作ブロック */
	m_bDoing_UndoRedo = false;	/* アンドゥ・リドゥの実行中か */
	m_pcsbwVSplitBox = NULL;	/* 垂直分割ボックス */
	m_pcsbwHSplitBox = NULL;	/* 水平分割ボックス */
	m_hInstance = NULL;
	m_hWnd = NULL;
	m_hwndVScrollBar = NULL;
	m_nVScrollRate = 1;			/* 垂直スクロールバーの縮尺 */
	m_hwndHScrollBar = NULL;
	m_hwndSizeBox = NULL;
	m_ptCaretPos.x = 0;			/* ビュー左端からのカーソル桁位置(０オリジン) */
	m_nCaretPosX_Prev = 0;		/* ビュー左端からのカーソル桁直前の位置(０オリジン) */
	m_ptCaretPos.y = 0;			/* ビュー上端からのカーソル行位置(０オリジン) */

	m_ptCaretPos_PHY.x = 0;		/* カーソル位置 改行単位行先頭からのバイト数(０開始) */
	m_ptCaretPos_PHY.y = 0;		/* カーソル位置 改行単位行の行番号(０開始) */

	m_ptSrchStartPos_PHY.x = -1;	/* 検索/置換開始時のカーソル位置  改行単位行先頭からのバイト数(0開始) */	// 02/06/26 ai
	m_ptSrchStartPos_PHY.y = -1;	/* 検索/置換開始時のカーソル位置  改行単位行の行番号(0開始) */				// 02/06/26 ai
	m_bSearch = FALSE;			/* 検索/置換開始位置を登録するか */											// 02/06/26 ai
	m_ptBracketPairPos_PHY.x = -1;/* 対括弧の位置 改行単位行先頭からのバイト数(0開始) */	// 02/12/13 ai
	m_ptBracketPairPos_PHY.y = -1;/* 対括弧の位置 改行単位行の行番号(0開始) */			// 02/12/13 ai
	m_ptBracketCaretPos_PHY.x = -1;	/* 03/02/18 ai */
	m_ptBracketCaretPos_PHY.y = -1;	/* 03/02/18 ai */
	m_bDrawBracketPairFlag = FALSE;	/* 03/02/18 ai */
	m_bDrawSelectArea = false;	/* 選択範囲を描画したか */	// 02/12/13 ai

	m_nCaretWidth = 0;			/* キャレットの幅 */
	m_nCaretHeight = 0;			/* キャレットの高さ */
	m_crCaret = -1;				/* キャレットの色 */			// 2006.12.16 ryoji
	m_crBack = -1;				/* テキストの背景色 */			// 2006.12.16 ryoji
	m_hbmpCaret = NULL;			/* キャレット用ビットマップ */	// 2006.11.28 ryoji

	m_bSelectingLock = false;	/* 選択状態のロック */
	m_bBeginSelect = false;		/* 範囲選択中 */
	m_bBeginBoxSelect = false;	/* 矩形範囲選択中 */
	m_bBeginLineSelect = false;	/* 行単位選択中 */
	m_bBeginWordSelect = false;	/* 単語単位選択中 */

	m_sSelectBgn.m_ptFrom.y = -1;	/* 範囲選択開始行(原点) */
	m_sSelectBgn.m_ptFrom.x = -1;	/* 範囲選択開始桁(原点) */
	m_sSelectBgn.m_ptTo.y = -1;	/* 範囲選択開始行(原点) */
	m_sSelectBgn.m_ptTo.x = -1;	/* 範囲選択開始桁(原点) */

	m_sSelect.m_ptFrom.y = -1;		/* 範囲選択開始行 */
	m_sSelect.m_ptFrom.x = -1;		/* 範囲選択開始桁 */
	m_sSelect.m_ptTo.y = -1;		/* 範囲選択終了行 */
	m_sSelect.m_ptTo.x = -1;		/* 範囲選択終了桁 */

	m_sSelectOld.m_ptFrom.y = 0;	/* 範囲選択開始行 */
	m_sSelectOld.m_ptFrom.x = 0;	/* 範囲選択開始桁 */
	m_sSelectOld.m_ptTo.y = 0;		/* 範囲選択終了行 */
	m_sSelectOld.m_ptTo.x = 0;		/* 範囲選択終了桁 */
	m_nViewAlignLeft = 0;		/* 表示域の左端座標 */
	m_nViewAlignLeftCols = 0;	/* 行番号域の桁数 */
	m_nTopYohaku = m_pShareData->m_Common.m_sWindow.m_nRulerBottomSpace; 	/* ルーラーとテキストの隙間 */
	m_nViewAlignTop = m_nTopYohaku;		/* 表示域の上端座標 */

	/* ルーラー表示 */
	m_nViewAlignTop += m_pShareData->m_Common.m_sWindow.m_nRulerHeight;	/* ルーラー高さ */
	m_nOldCaretPosX = 0;	// 前回描画したルーラーのキャレット位置 2002.02.25 Add By KK
	m_nOldCaretWidth = 0;	// 前回描画したルーラーのキャレット幅   2002.02.25 Add By KK
	m_bRedrawRuler = true;	// ルーラー全体を描き直す時=true   2002.02.25 Add By KK
	m_nViewCx = 0;				/* 表示域の幅 */
	m_nViewCy = 0;				/* 表示域の高さ */
	m_nViewColNum = 0;			/* 表示域の桁数 */
	m_nViewRowNum = 0;			/* 表示域の行数 */
	m_nViewTopLine = 0;			/* 表示域の一番上の行 */
	m_nViewLeftCol = 0;			/* 表示域の一番左の桁 */
	m_hdcCompatDC = NULL;		/* 再描画用コンパチブルＤＣ */
	m_hbmpCompatBMP = NULL;		/* 再描画用メモリＢＭＰ */
	m_hbmpCompatBMPOld = NULL;	/* 再描画用メモリＢＭＰ(OLD) */
	// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
	m_nCompatBMPWidth = -1;
	m_nCompatBMPHeight = -1;
	// To Here 2007.09.09 Moca
	m_nCharWidth = 10;			/* 半角文字の幅 */
	m_nCharHeight = 18;			/* 文字の高さ */

	//	Jun. 27, 2001 genta	正規表現ライブラリの差し替え
	//	2007.08.12 genta 初期化にShareDataの値が必要になった
	m_CurRegexp.Init(m_pShareData->m_Common.m_sSearch.m_szRegexpLib );

	// 2004.02.08 m_hFont_ZENは未使用により削除
	m_dwTipTimer = ::GetTickCount();	/* 辞書Tip起動タイマー */
	m_bInMenuLoop = FALSE;				/* メニュー モーダル ループに入っています */
//	MYTRACE( _T("CEditView::CEditView()おわり\n") );
	m_bHokan = FALSE;

	m_hFontOld = NULL;

	//	Aug. 31, 2000 genta
	m_cHistory->SetMax( 30 );

	// from here  2002.04.09 minfu OSによって再変換の方式を変える
	//	YAZAKI COsVersionInfoのカプセル化は守りましょ。
	if( !OsSupportReconvert() ){
		// 95 or NTならば
		m_uMSIMEReconvertMsg = ::RegisterWindowMessage( RWM_RECONVERT );
		m_uATOKReconvertMsg = ::RegisterWindowMessage( MSGNAME_ATOK_RECONVERT ) ;
		m_uWM_MSIME_RECONVERTREQUEST = ::RegisterWindowMessage(_T("MSIMEReconvertRequest"));
		
		m_hAtokModule = LoadLibraryExedir(_T("ATOK10WC.DLL"));
		m_AT_ImmSetReconvertString = NULL;
		if ( NULL != m_hAtokModule ) {
			m_AT_ImmSetReconvertString =(BOOL (WINAPI *)( HIMC , int ,PRECONVERTSTRING , DWORD  ) ) GetProcAddress(m_hAtokModule,"AT_ImmSetReconvertString");
		}
	}
	else{ 
		// それ以外のOSのときはOS標準を使用する
		m_uMSIMEReconvertMsg = 0;
		m_uATOKReconvertMsg = 0 ;
		m_hAtokModule = 0;	//@@@ 2002.04.14 MIK
	}
	// to here  2002.04.10 minfu
	
	//2004.10.23 isearch
	m_nISearchMode = SEARCH_NONE;
	m_pcmigemo = NULL;

	// 2007.10.02 nasukoji
	m_dwTripleClickCheck = 0;		// トリプルクリックチェック用時刻初期化



	//↑今までコンストラクタでやってたこと
	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
	//↓今までCreateでやってたこと

	WNDCLASS	wc;
	m_hInstance = hInstance;
	m_hwndParent = hwndParent;
	m_pcEditDoc = pcEditDoc;
	m_nMyIndex = nMyIndex;
	
	m_dwTipTimer = ::GetTickCount();

	//	2007.08.18 genta 初期化にShareDataの値が必要になった
	m_cRegexKeyword = new CRegexKeyword( m_pShareData->m_Common.m_sSearch.m_szRegexpLib );	//@@@ 2001.11.17 add MIK
	m_cRegexKeyword->RegexKeySetTypes(&(m_pcEditDoc->GetDocumentAttribute()));	//@@@ 2001.11.17 add MIK

	m_nTopYohaku = m_pShareData->m_Common.m_sWindow.m_nRulerBottomSpace; 	/* ルーラーとテキストの隙間 */
	m_nViewAlignTop = m_nTopYohaku;								/* 表示域の上端座標 */
	/* ルーラー表示 */
	if( m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_RULER].m_bDisp ){
		m_nViewAlignTop += m_pShareData->m_Common.m_sWindow.m_nRulerHeight;	/* ルーラー高さ */
	}


	/* ウィンドウクラスの登録 */
	//	Apr. 27, 2000 genta
	//	サイズ変更時のちらつきを抑えるためCS_HREDRAW | CS_VREDRAW を外した
	wc.style			= CS_DBLCLKS | CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
	wc.lpfnWndProc		= EditViewWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= sizeof( LONG_PTR );
	wc.hInstance		= m_hInstance;
	wc.hIcon			= LoadIcon( NULL, IDI_APPLICATION );
	wc.hCursor			= NULL/*LoadCursor( NULL, IDC_IBEAM )*/;
	wc.hbrBackground	= (HBRUSH)NULL/*(COLOR_WINDOW + 1)*/;
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= GSTR_VIEWNAME;
	if( 0 == ::RegisterClass( &wc ) ){
	}

	/* エディタウィンドウの作成 */
	g_m_pcEditView = this;
	m_hWnd = ::CreateWindowEx(
		WS_EX_STATICEDGE,	// extended window style
		GSTR_VIEWNAME,			// pointer to registered class name
		GSTR_VIEWNAME,			// pointer to window name
		0						// window style
		| WS_VISIBLE
		| WS_CHILD
		| WS_CLIPCHILDREN
		, 
		CW_USEDEFAULT,			// horizontal position of window
		0,						// vertical position of window
		CW_USEDEFAULT,			// window width
		0,						// window height
		hwndParent,				// handle to parent or owner window
		NULL,					// handle to menu or child-window identifier
		m_hInstance,			// handle to application instance
		(LPVOID)this			// pointer to window-creation data
	);
	if( NULL == m_hWnd ){
		return FALSE;
	}

	m_pcDropTarget->Register_DropTarget( m_hWnd );

	/* 辞書Tip表示ウィンドウ作成 */
	m_cTipWnd.Create( m_hInstance, m_hWnd/*m_pShareData->m_hwndTray*/ );

	/* 再描画用コンパチブルＤＣ */
	// 2007.09.09 Moca 互換BMPによる画面バッファ
	// 2007.09.30 genta 関数化
	UseCompatibleDC( m_pShareData->m_Common.m_sWindow.m_bUseCompatibleBMP );

	/* 垂直分割ボックス */
	m_pcsbwVSplitBox = new CSplitBoxWnd;
	m_pcsbwVSplitBox->Create( m_hInstance, m_hWnd, TRUE );
	/* 水平分割ボックス */
	m_pcsbwHSplitBox = new CSplitBoxWnd;
	m_pcsbwHSplitBox->Create( m_hInstance, m_hWnd, FALSE );

	/* スクロールバー作成 */
	CreateScrollBar();		// 2006.12.19 ryoji

	SetFont();

	if( bShow ){
		ShowWindow( m_hWnd, SW_SHOW );
	}

	/* キーボードの現在のリピート間隔を取得 */
	int nKeyBoardSpeed;
	SystemParametersInfo( SPI_GETKEYBOARDSPEED, 0, &nKeyBoardSpeed, 0 );

	/* タイマー起動 */
	if( 0 == ::SetTimer( m_hWnd, IDT_ROLLMOUSE, nKeyBoardSpeed, EditViewTimerProc ) ){
		WarningMessage( m_hWnd, _T("CEditView::Create()\nタイマーが起動できません。\nシステムリソースが不足しているのかもしれません。") );
	}

	/* アンダーライン */
	m_cUnderLine.SetView( this );
	return TRUE;
}


CEditView::~CEditView()
{
	// キャレット用ビットマップ	// 2006.11.28 ryoji
	if( m_hbmpCaret != NULL )
		DeleteObject( m_hbmpCaret );

	if( m_hWnd != NULL ){
		::DestroyWindow( m_hWnd );
	}

	/* 再描画用コンパチブルＤＣ */
	//	2007.09.30 genta 関数化
	//	m_hbmpCompatBMPもここで削除される．
	UseCompatibleDC(FALSE);

	delete m_pcDropTarget;
	m_pcDropTarget = NULL;

	delete m_cHistory;

	delete m_cRegexKeyword;	//@@@ 2001.11.17 add MIK
	
	//再変換 2002.04.10 minfu
	if(m_hAtokModule)
		FreeLibrary(m_hAtokModule);
}

/** 画面キャッシュ用CompatibleDCを用意する

	@param[in] TRUE: 画面キャッシュON

	@date 2007.09.30 genta 関数化
*/
void CEditView::UseCompatibleDC(BOOL fCache)
{
	// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
	if( fCache ){
		if( m_hdcCompatDC == NULL ){
			HDC			hdc;
			hdc = ::GetDC( m_hWnd );
			m_hdcCompatDC = ::CreateCompatibleDC( hdc );
			::ReleaseDC( m_hWnd, hdc );
			DEBUG_TRACE(_T("CEditView::UseCompatibleDC: Created\n"), fCache);
		}
		else {
			DEBUG_TRACE(_T("CEditView::UseCompatibleDC: Reused\n"), fCache);
		}
	}
	else {
		//	CompatibleBitmapが残っているかもしれないので最初に削除
		DeleteCompatibleBitmap();
		if( m_hdcCompatDC != NULL ){
			::DeleteDC( m_hdcCompatDC );
			DEBUG_TRACE(_T("CEditView::UseCompatibleDC: Deleted.\n"));
			m_hdcCompatDC = NULL;
		}
	}
}

/*! スクロールバー作成
	@date 2006.12.19 ryoji 新規作成（CEditView::Createから分離）
*/
BOOL CEditView::CreateScrollBar()
{
	SCROLLINFO	si;

	/* スクロールバーの作成 */
	m_hwndVScrollBar = ::CreateWindowEx(
		0L,									/* no extended styles */
		_T("SCROLLBAR"),					/* scroll bar control class */
		(LPSTR) NULL,						/* text for window title bar */
		WS_VISIBLE | WS_CHILD | SBS_VERT,	/* scroll bar styles */
		0,									/* horizontal position */
		0,									/* vertical position */
		200,								/* width of the scroll bar */
		CW_USEDEFAULT,						/* default height */
		m_hWnd,								/* handle of main window */
		(HMENU) NULL,						/* no menu for a scroll bar */
		m_hInstance,						/* instance owning this window */
		(LPVOID) NULL						/* pointer not needed */
	);
	si.cbSize = sizeof( si );
	si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
	si.nMin  = 0;
	si.nMax  = 29;
	si.nPage = 10;
	si.nPos  = 0;
	si.nTrackPos = 1;
	::SetScrollInfo( m_hwndVScrollBar, SB_CTL, &si, TRUE );
	::ShowScrollBar( m_hwndVScrollBar, SB_CTL, TRUE );

	/* スクロールバーの作成 */
	m_hwndHScrollBar = NULL;
	if( m_pShareData->m_Common.m_sWindow.m_bScrollBarHorz ){	/* 水平スクロールバーを使う */
		m_hwndHScrollBar = ::CreateWindowEx(
			0L,									/* no extended styles */
			_T("SCROLLBAR"),					/* scroll bar control class */
			(LPSTR) NULL,						/* text for window title bar */
			WS_VISIBLE | WS_CHILD | SBS_HORZ,	/* scroll bar styles */
			0,									/* horizontal position */
			0,									/* vertical position */
			200,								/* width of the scroll bar */
			CW_USEDEFAULT,						/* default height */
			m_hWnd,								/* handle of main window */
			(HMENU) NULL,						/* no menu for a scroll bar */
			m_hInstance,						/* instance owning this window */
			(LPVOID) NULL						/* pointer not needed */
		);
		si.cbSize = sizeof( si );
		si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
		si.nMin  = 0;
		si.nMax  = 29;
		si.nPage = 10;
		si.nPos  = 0;
		si.nTrackPos = 1;
		::SetScrollInfo( m_hwndHScrollBar, SB_CTL, &si, TRUE );
		::ShowScrollBar( m_hwndHScrollBar, SB_CTL, TRUE );
	}


	/* サイズボックス */
	if( m_pShareData->m_Common.m_sWindow.m_nFUNCKEYWND_Place == 0 ){	/* ファンクションキー表示位置／0:上 1:下 */
		m_hwndSizeBox = ::CreateWindowEx(
			WS_EX_CONTROLPARENT/*0L*/, 			/* no extended styles */
			_T("SCROLLBAR"),					/* scroll bar control class */
			(LPSTR) NULL,						/* text for window title bar */
			WS_VISIBLE | WS_CHILD | SBS_SIZEBOX | SBS_SIZEGRIP, /* scroll bar styles */
			0,									/* horizontal position */
			0,									/* vertical position */
			200,								/* width of the scroll bar */
			CW_USEDEFAULT,						/* default height */
			m_hWnd, 							/* handle of main window */
			(HMENU) NULL,						/* no menu for a scroll bar */
			m_hInstance,						/* instance owning this window */
			(LPVOID) NULL						/* pointer not needed */
		);
	}else{
		m_hwndSizeBox = ::CreateWindowEx(
			0L, 								/* no extended styles */
			_T("STATIC"),						/* scroll bar control class */
			(LPSTR) NULL,						/* text for window title bar */
			WS_VISIBLE | WS_CHILD/* | SBS_SIZEBOX | SBS_SIZEGRIP*/, /* scroll bar styles */
			0,									/* horizontal position */
			0,									/* vertical position */
			200,								/* width of the scroll bar */
			CW_USEDEFAULT,						/* default height */
			m_hWnd, 							/* handle of main window */
			(HMENU) NULL,						/* no menu for a scroll bar */
			m_hInstance,						/* instance owning this window */
			(LPVOID) NULL						/* pointer not needed */
		);
	}
	return TRUE;
}



/*! スクロールバー破棄
	@date 2006.12.19 ryoji 新規作成
*/
void CEditView::DestroyScrollBar()
{
	if( m_hwndVScrollBar )
	{
		::DestroyWindow( m_hwndVScrollBar );
		m_hwndVScrollBar = NULL;
	}

	if( m_hwndHScrollBar )
	{
		::DestroyWindow( m_hwndHScrollBar );
		m_hwndHScrollBar = NULL;
	}

	if( m_hwndSizeBox )
	{
		::DestroyWindow( m_hwndSizeBox );
		m_hwndSizeBox = NULL;
	}
}

/*
|| メッセージディスパッチャ
*/
LRESULT CEditView::DispatchEvent(
	HWND	hwnd,	// handle of window
	UINT	uMsg,	// message identifier
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
)
{
	HDC			hdc;
//	int			nPosX;
//	int			nPosY;

	switch ( uMsg ){
	case WM_MOUSEWHEEL:
		if( m_pcEditWnd->DoMouseWheel( wParam, lParam ) ){
			return 0L;
		}
		return OnMOUSEWHEEL( wParam, lParam );

	case WM_CREATE:
		::SetWindowLongPtr( hwnd, 0, (LONG_PTR) this );

		return 0L;

	// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
	case WM_SHOWWINDOW:
		// ウィンドウ非表示の再に互換BMPを廃棄してメモリーを節約する
		if( hwnd == m_hWnd && (BOOL)wParam == FALSE ){
			DeleteCompatibleBitmap();
		}
		return 0L;
	// To Here 2007.09.09 Moca

	case WM_SIZE:
		OnSize( LOWORD( lParam ), HIWORD( lParam ) );
		return 0L;

	case WM_SETFOCUS:
		OnSetFocus();

		/* 親ウィンドウのタイトルを更新 */
		m_pcEditWnd->UpdateCaption();

		return 0L;
	case WM_KILLFOCUS:
		OnKillFocus();

		// 2009.01.12 nasukoji	ホイールスクロール有無状態をクリア
		m_pcEditWnd->ClearMouseState();

		return 0L;
	case WM_CHAR:
		HandleCommand( F_CHAR, true, wParam, 0, 0, 0 );
		return 0L;

	case WM_IME_NOTIFY:	// Nov. 26, 2006 genta
		if( wParam == IMN_SETCONVERSIONMODE || wParam == IMN_SETOPENSTATUS){
			ShowEditCaret();
		}
		return DefWindowProc( hwnd, uMsg, wParam, lParam );

	case WM_IME_COMPOSITION:
		if( IsInsMode() && (lParam & GCS_RESULTSTR)){
			HIMC hIMC;
			DWORD dwSize;
			HGLOBAL hstr;
			hIMC = ImmGetContext( hwnd );

			if( !hIMC ){
				return 0;
//				MyError( ERROR_NULLCONTEXT );
			}

			// Get the size of the result string.
			dwSize = ImmGetCompositionString(hIMC, GCS_RESULTSTR, NULL, 0);

			// increase buffer size for NULL terminator,
			//	maybe it is in Unicode
			dwSize += sizeof( WCHAR );

			hstr = GlobalAlloc( GHND, dwSize );
			if( hstr == NULL ){
				return 0;
//				 MyError( ERROR_GLOBALALLOC );
			}

			LPTSTR lptstr;
			lptstr = (LPTSTR)GlobalLock( hstr );
			if( lptstr == NULL ){
				return 0;
//				 MyError( ERROR_GLOBALLOCK );
			}

			// Get the result strings that is generated by IME into lptstr.
			ImmGetCompositionString(hIMC, GCS_RESULTSTR, lptstr, dwSize);

			/* テキストを貼り付け */
			HandleCommand( F_INSTEXT, true, (LPARAM)lptstr, TRUE, 0, 0 );

			ImmReleaseContext( hwnd, hIMC );

			// add this string into text buffer of application

			GlobalUnlock( hstr );
			GlobalFree( hstr );
			return DefWindowProc( hwnd, uMsg, wParam, lParam );
		}
		return DefWindowProc( hwnd, uMsg, wParam, lParam );

	case WM_IME_CHAR:
		if( ! IsInsMode() /* Oct. 2, 2005 genta */ ){ /* 上書きモードか？ */
			HandleCommand( F_IME_CHAR, true, wParam, 0, 0, 0 );
		}
		return 0L;

	// From Here 2008.03.24 Moca ATOK等の要求にこたえる
	case WM_PASTE:
		return HandleCommand( F_PASTE, true, 0, 0, 0, 0 );

	case WM_COPY:
		return HandleCommand( F_COPY, true, 0, 0, 0, 0 );
	// To Here 2008.03.24 Moca

	case WM_KEYUP:
		/* キーリピート状態 */
		m_bPrevCommand = 0;
		return 0L;

	// 2004.04.27 Moca From Here ALT+xでALTを押したままだとキーリピートがOFFにならない対策
	case WM_SYSKEYUP:
		m_bPrevCommand = 0;
		// 念のため呼ぶ
		return ::DefWindowProc( hwnd, uMsg, wParam, lParam );
	// 2004.04.27 To Here

	case WM_LBUTTONDBLCLK:

		// 2007.10.02 nasukoji	非アクティブウィンドウのダブルクリック時はここでカーソルを移動する
		// 2007.10.12 genta フォーカス移動のため，OnLBUTTONDBLCLKより移動
		if(m_bActivateByMouse){
			/* アクティブなペインを設定 */
			m_pcEditWnd->SetActivePane( m_nMyIndex );
			// カーソルをクリック位置へ移動する
			OnLBUTTONDOWN( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );	
			// 2007.10.02 nasukoji
			m_bActivateByMouse = FALSE;		// マウスによるアクティベートを示すフラグをOFF
		}
		//		MYTRACE( _T(" WM_LBUTTONDBLCLK wParam=%08xh, x=%d y=%d\n"), wParam, LOWORD( lParam ), HIWORD( lParam ) );
		OnLBUTTONDBLCLK( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );
		return 0L;

// novice 2004/10/11 マウス中ボタン対応
	case WM_MBUTTONDOWN:
		OnMBUTTONDOWN( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );

		return 0L;

	case WM_MBUTTONUP:
		// 2009.01.12 nasukoji	ボタンUPでコマンドを起動するように変更
		OnMBUTTONUP( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );
		return 0L;

	case WM_LBUTTONDOWN:
		// 2007.10.02 nasukoji
		m_bActivateByMouse = FALSE;		// マウスによるアクティベートを示すフラグをOFF
//		MYTRACE( _T(" WM_LBUTTONDOWN wParam=%08xh, x=%d y=%d\n"), wParam, LOWORD( lParam ), HIWORD( lParam ) );
		OnLBUTTONDOWN( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );
		return 0L;

	case WM_LBUTTONUP:

//		MYTRACE( _T(" WM_LBUTTONUP wParam=%08xh, x=%d y=%d\n"), wParam, LOWORD( lParam ), HIWORD( lParam ) );
		OnLBUTTONUP( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );
		return 0L;
	case WM_MOUSEMOVE:
		OnMOUSEMOVE( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );
		return 0L;

	case WM_RBUTTONDBLCLK:
//		MYTRACE( _T(" WM_RBUTTONDBLCLK wParam=%08xh, x=%d y=%d\n"), wParam, LOWORD( lParam ), HIWORD( lParam ) );
		return 0L;
//	case WM_RBUTTONDOWN:
//		MYTRACE( _T(" WM_RBUTTONDOWN wParam=%08xh, x=%d y=%d\n"), wParam, LOWORD( lParam ), HIWORD( lParam ) );
//		OnRBUTTONDOWN( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );
//		if( m_nMyIndex != m_pcEditDoc->GetActivePane() ){
//			/* アクティブなペインを設定 */
//			m_pcEditDoc->SetActivePane( m_nMyIndex );
//		}
//		return 0L;
	case WM_RBUTTONUP:
//		MYTRACE( _T(" WM_RBUTTONUP wParam=%08xh, x=%d y=%d\n"), wParam, LOWORD( lParam ), HIWORD( lParam ) );
		OnRBUTTONUP( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );
		return 0L;

// novice 2004/10/10 マウスサイドボタン対応
	case WM_XBUTTONDOWN:
		switch ( HIWORD(wParam) ){
		case XBUTTON1:
			OnXLBUTTONDOWN( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );
			break;
		case XBUTTON2:
			OnXRBUTTONDOWN( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );
			break;
		}

		return TRUE;

	case WM_XBUTTONUP:
		// 2009.01.12 nasukoji	ボタンUPでコマンドを起動するように変更
		switch ( HIWORD(wParam) ){
		case XBUTTON1:
			OnXLBUTTONUP( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );
			break;
		case XBUTTON2:
			OnXRBUTTONUP( wParam, (short)LOWORD( lParam ), (short)HIWORD( lParam ) );
			break;
		}

		return TRUE;

	case WM_VSCROLL:
//		MYTRACE( _T("	WM_VSCROLL nPos=%d\n"), GetScrollPos( m_hwndVScrollBar, SB_CTL ) );
		//	Sep. 11, 2004 genta 同期スクロールの関数化
		{
			int Scroll = OnVScroll(
				(int) LOWORD( wParam ), ((int) HIWORD( wParam )) * m_nVScrollRate );

			//	シフトキーが押されていないときだけ同期スクロール
			if(!GetKeyState_Shift()){
				SyncScrollV( Scroll );
			}
		}

		return 0L;

	case WM_HSCROLL:
//		MYTRACE( _T("	WM_HSCROLL nPos=%d\n"), GetScrollPos( m_hwndHScrollBar, SB_CTL ) );
		//	Sep. 11, 2004 genta 同期スクロールの関数化
		{
			int Scroll = OnHScroll(
				(int) LOWORD( wParam ), ((int) HIWORD( wParam )) );

			//	シフトキーが押されていないときだけ同期スクロール
			if(!GetKeyState_Shift()){
				SyncScrollH( Scroll );
			}
		}

		return 0L;

	case WM_ENTERMENULOOP:
		m_bInMenuLoop = TRUE;	/* メニュー モーダル ループに入っています */

		/* 辞書Tipが起動されている */
		if( 0 == m_dwTipTimer ){
			/* 辞書Tipを消す */
			m_cTipWnd.Hide();
			m_dwTipTimer = ::GetTickCount();	/* 辞書Tip起動タイマー */
		}
		if( m_bHokan ){
			m_pcEditWnd->m_cHokanMgr.Hide();
			m_bHokan = FALSE;
		}
		return 0L;

	case WM_EXITMENULOOP:
		m_bInMenuLoop = FALSE;	/* メニュー モーダル ループに入っています */
		return 0L;


	case WM_PAINT:
		{
			PAINTSTRUCT	ps;
			hdc = ::BeginPaint( hwnd, &ps );
			OnPaint( hdc, &ps, FALSE );
			::EndPaint(hwnd, &ps);
		}
		return 0L;

	case WM_CLOSE:
//		MYTRACE( _T("	WM_CLOSE\n") );
		::DestroyWindow( hwnd );
		return 0L;
	case WM_DESTROY:
		m_pcDropTarget->Revoke_DropTarget();

		/* タイマー終了 */
		::KillTimer( m_hWnd, IDT_ROLLMOUSE );


//		MYTRACE( _T("	WM_DESTROY\n") );
		/*
		||子ウィンドウの破棄
		*/
		if( NULL != m_hwndVScrollBar ){	// Aug. 20, 2005 Aroka
			::DestroyWindow( m_hwndVScrollBar );
			m_hwndVScrollBar = NULL;
		}
		if( NULL != m_hwndHScrollBar ){
			::DestroyWindow( m_hwndHScrollBar );
			m_hwndHScrollBar = NULL;
		}
		if( NULL != m_hwndSizeBox ){
			::DestroyWindow( m_hwndSizeBox );
			m_hwndSizeBox = NULL;
		}
		delete m_pcsbwVSplitBox;	/* 垂直分割ボックス */
		m_pcsbwVSplitBox = NULL;
		delete m_pcsbwHSplitBox;	/* 水平分割ボックス */
		m_pcsbwHSplitBox = NULL;

		m_hWnd = NULL;
		return 0L;

	case MYWM_DOSPLIT:
//		nPosX = (int)wParam;
//		nPosY = (int)lParam;
//		MYTRACE( _T("MYWM_DOSPLIT nPosX=%d nPosY=%d\n"), nPosX, nPosY );
		::SendMessage( m_hwndParent, MYWM_DOSPLIT, wParam, lParam );
		return 0L;

	case MYWM_SETACTIVEPANE:
		m_pcEditWnd->SetActivePane( m_nMyIndex );
		::PostMessage( m_hwndParent, MYWM_SETACTIVEPANE, (WPARAM)m_nMyIndex, 0 );
		return 0L;

	case MYWM_IME_REQUEST:  /* 再変換  by minfu 2002.03.27 */ // 20020331 aroka
		
		// 2002.04.09 switch case に変更  minfu 
		switch ( wParam ){
		case IMR_RECONVERTSTRING:
			return SetReconvertStruct((PRECONVERTSTRING)lParam, false);
			
		case IMR_CONFIRMRECONVERTSTRING:
			return SetSelectionFromReonvert((PRECONVERTSTRING)lParam, false);
			
		}
		
		return 0L;

	case MYWM_DROPFILES:	// 独自のドロップファイル通知	// 2008.06.20 ryoji
		OnMyDropFiles( (HDROP)wParam );
		return 0L;

	// 2007.10.02 nasukoji	マウスクリックにてアクティベートされた時はカーソル位置を移動しない
	case WM_MOUSEACTIVATE:
		LRESULT nRes;
		nRes = ::DefWindowProc( hwnd, uMsg, wParam, lParam );	// 親に先に処理させる
		if( nRes == MA_NOACTIVATE || nRes == MA_NOACTIVATEANDEAT ){
			return nRes;
		}

		// マウスクリックによりバックグラウンドウィンドウがアクティベートされた
		//	2007.10.08 genta オプション追加
		if( m_pShareData->m_Common.m_sGeneral.m_bNoCaretMoveByActivation &&
		   (! m_pcEditWnd->IsActiveApp()))
		{
			m_bActivateByMouse = TRUE;		// マウスによるアクティベート
			return MA_ACTIVATEANDEAT;		// アクティベート後イベントを破棄
		}

		/* アクティブなペインを設定 */
		if( ::GetFocus() != m_hWnd ){
			POINT ptCursor;
			::GetCursorPos( &ptCursor );
			HWND hwndCursorPos = ::WindowFromPoint( ptCursor );
			if( hwndCursorPos == m_hWnd ){
				// ビュー上にマウスがあるので SetActivePane() を直接呼び出す
				// （個別のマウスメッセージが届く前にアクティブペインを設定しておく）
				m_pcEditWnd->SetActivePane( m_nMyIndex );
			}else if( (m_pcsbwVSplitBox && hwndCursorPos == m_pcsbwVSplitBox->m_hWnd)
						|| (m_pcsbwHSplitBox && hwndCursorPos == m_pcsbwHSplitBox->m_hWnd) ){
				// 2010.01.19 ryoji
				// 分割ボックス上にマウスがあるときはアクティブペインを切り替えない
				// （併せて MYWM_SETACTIVEPANE のポストにより分割線のゴミが残っていた問題も修正）
			}else{
				// 2008.05.24 ryoji
				// スクロールバー上にマウスがあるかもしれないので MYWM_SETACTIVEPANE をポストする
				// SetActivePane() にはスクロールバーのスクロール範囲調整処理が含まれているが、
				// このタイミング（WM_MOUSEACTIVATE）でスクロール範囲を変更するのはまずい。
				// 例えば Win XP/Vista だとスクロール範囲が小さくなってスクロールバーが有効から
				// 無効に切り替わるとそれ以後スクロールバーが機能しなくなる。
				::PostMessage( m_hWnd, MYWM_SETACTIVEPANE, (WPARAM)m_nMyIndex, 0 );
			}
		}

		return nRes;
	
	default:
// << 20020331 aroka 再変換対応 for 95/NT
		if( (m_uMSIMEReconvertMsg && (uMsg == m_uMSIMEReconvertMsg)) 
			|| (m_uATOKReconvertMsg && (uMsg == m_uATOKReconvertMsg))){
		// 2002.04.08 switch case に変更 minfu 
			switch ( wParam ){
			case IMR_RECONVERTSTRING:
				return SetReconvertStruct((PRECONVERTSTRING)lParam, true);
				
			case IMR_CONFIRMRECONVERTSTRING:
				return SetSelectionFromReonvert((PRECONVERTSTRING)lParam, true);
				
			}
			return 0L;
		}
// >> by aroka

		return DefWindowProc( hwnd, uMsg, wParam, lParam );
	}
}




void CEditView::OnMove( int x, int y, int nWidth, int nHeight )
{
	MoveWindow( m_hWnd, x, y, nWidth, nHeight, TRUE );
	return;
}


/* ウィンドウサイズの変更処理 */
void CEditView::OnSize( int cx, int cy )
{
	if( NULL == m_hWnd 
		|| ( cx == 0 && cy == 0 ) ){
		// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
		// ウィンドウ無効時にも互換BMPを破棄する
		DeleteCompatibleBitmap();
		// To Here 2007.09.09 Moca
		return;
	}

	int nVSplitHeight = 0;	/* 垂直分割ボックスの高さ */
	int nHSplitWidth = 0;	/* 水平分割ボックスの幅 */

	//スクロールバーのサイズ基準値を取得
	int nCxHScroll = ::GetSystemMetrics( SM_CXHSCROLL );
	int nCyHScroll = ::GetSystemMetrics( SM_CYHSCROLL );
	int nCxVScroll = ::GetSystemMetrics( SM_CXVSCROLL );
	int nCyVScroll = ::GetSystemMetrics( SM_CYVSCROLL );

	/* 垂直分割ボックス */
	if( NULL != m_pcsbwVSplitBox ){
		nVSplitHeight = 7;
		::MoveWindow( m_pcsbwVSplitBox->m_hWnd, cx - nCxVScroll , 0, nCxVScroll, nVSplitHeight, TRUE );
	}
	/* 水平分割ボックス */
	if( NULL != m_pcsbwHSplitBox ){
		nHSplitWidth = 7;
		::MoveWindow( m_pcsbwHSplitBox->m_hWnd,0, cy - nCyHScroll, nHSplitWidth, nCyHScroll, TRUE );
	}
	/* 垂直スクロールバー */
	if( NULL != m_hwndVScrollBar ){
		::MoveWindow( m_hwndVScrollBar, cx - nCxVScroll , 0 + nVSplitHeight, nCxVScroll, cy - nCyVScroll - nVSplitHeight, TRUE );
	}
	/* 水平スクロールバー */
	if( NULL != m_hwndHScrollBar ){
		::MoveWindow( m_hwndHScrollBar, 0 + nHSplitWidth, cy - nCyHScroll, cx - nCxVScroll - nHSplitWidth, nCyHScroll, TRUE );
	}

	/* サイズボックス */
	if( NULL != m_hwndSizeBox ){
		::MoveWindow( m_hwndSizeBox, cx - nCxVScroll, cy - nCyHScroll, nCxHScroll, nCyVScroll, TRUE );
	}

	m_nViewCx = cx - nCxVScroll - m_nViewAlignLeft;														/* 表示域の幅 */
	m_nViewCy = cy - ((NULL != m_hwndHScrollBar)?nCyHScroll:0) - m_nViewAlignTop;						/* 表示域の高さ */
	m_nViewColNum = (m_nViewCx - 1) / ( m_nCharWidth  + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );	/* 表示域の桁数 */
	m_nViewRowNum = (m_nViewCy - 1) / ( m_nCharHeight + m_pcEditDoc->GetDocumentAttribute().m_nLineSpace );	/* 表示域の行数 */

	// 2008.06.06 nasukoji	サイズ変更時の折り返し位置再計算
	BOOL wrapChanged = FALSE;
	if( m_pcEditDoc->m_nTextWrapMethodCur == WRAP_WINDOW_WIDTH ){
		if( m_nMyIndex == 0 ){	// 左上隅のビューのサイズ変更時のみ処理する
			// 右端で折り返すモードなら右端で折り返す	// 2008.06.08 ryoji
			wrapChanged = m_pcEditWnd->WrapWindowWidth( 0 );
		}
	}

	if( !wrapChanged )	// 折り返し位置が変更されていない
		AdjustScrollBars();				// スクロールバーの状態を更新する

	/* 再描画用メモリＢＭＰ */
	// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
	if( m_hdcCompatDC != NULL ){
		CreateOrUpdateCompatibleBitmap( cx, cy );
 	}
	// To Here 2007.09.09 Moca

	/* 親ウィンドウのタイトルを更新 */
	m_pcEditWnd->UpdateCaption();	//	[Q] genta 本当に必要？

	return;
}

/*!
	画面の互換ビットマップを作成または更新する。
		必要の無いときは何もしない。
	
	@param cx ウィンドウの高さ
	@param cy ウィンドウの幅
	@return true: ビットマップを利用可能 / false: ビットマップの作成・更新に失敗

	@date 2007.09.09 Moca CEditView::OnSizeから分離。
		単純に生成するだけだったものを、仕様変更に従い内容コピーを追加。
		サイズが同じときは何もしないように変更

	@par 互換BMPにはキャレット・カーソル位置横縦線・対括弧以外の情報を全て書き込む。
		選択範囲変更時の反転処理は、画面と互換BMPの両方を別々に変更する。
		カーソル位置横縦線変更時には、互換BMPから画面に元の情報を復帰させている。

*/
bool CEditView::CreateOrUpdateCompatibleBitmap( int cx, int cy )
{
	if( NULL == m_hdcCompatDC ){
		return false;
	}
	// サイズを64の倍数で整列
	int nBmpWidthNew  = ((cx + 63) & (0x7fffffff - 63));
	int nBmpHeightNew = ((cy + 63) & (0x7fffffff - 63));
	if( nBmpWidthNew != m_nCompatBMPWidth || nBmpHeightNew != m_nCompatBMPHeight ){
		DEBUG_TRACE( _T("CEditView::CreateOrUpdateCompatibleBitmap( %d, %d ): resized\n"), cx, cy );
		HDC	hdc = ::GetDC( m_hWnd );
		HBITMAP hBitmapNew = NULL;
		if( m_hbmpCompatBMP ){
			// BMPの更新
			HDC hdcTemp = ::CreateCompatibleDC( hdc );
			hBitmapNew = ::CreateCompatibleBitmap( hdc, nBmpWidthNew, nBmpHeightNew );
			if( hBitmapNew ){
				HBITMAP hBitmapOld = (HBITMAP)::SelectObject( hdcTemp, hBitmapNew );
				// 前の画面内容をコピーする
				::BitBlt( hdcTemp, 0, 0,
					t_min( nBmpWidthNew,m_nCompatBMPWidth ),
					t_min( nBmpHeightNew, m_nCompatBMPHeight ),
					m_hdcCompatDC, 0, 0, SRCCOPY );
				::SelectObject( hdcTemp, hBitmapOld );
				::SelectObject( m_hdcCompatDC, m_hbmpCompatBMPOld );
				::DeleteObject( m_hbmpCompatBMP );
			}
			::DeleteDC( hdcTemp );
		}else{
			// BMPの新規作成
			hBitmapNew = ::CreateCompatibleBitmap( hdc, nBmpWidthNew, nBmpHeightNew );
		}
		if( hBitmapNew ){
			m_hbmpCompatBMP = hBitmapNew;
			m_nCompatBMPWidth = nBmpWidthNew;
			m_nCompatBMPHeight = nBmpHeightNew;
			m_hbmpCompatBMPOld = (HBITMAP)::SelectObject( m_hdcCompatDC, m_hbmpCompatBMP );
		}else{
			// 互換BMPの作成に失敗
			// 今後も失敗を繰り返す可能性が高いので
			// m_hdcCompatDCをNULLにすることで画面バッファ機能をこのウィンドウのみ無効にする。
			//	2007.09.29 genta 関数化．既存のBMPも解放
			UseCompatibleDC(FALSE);
		}
		::ReleaseDC( m_hWnd, hdc );
	}
	return NULL != m_hbmpCompatBMP;
}


/*!
	互換メモリBMPを削除

	@note 分割ビューが非表示になった場合と
		親ウィンドウが非表示・最小化された場合に削除される。
	@date 2007.09.09 Moca 新規作成 
*/
void CEditView::DeleteCompatibleBitmap()
{
	if( m_hbmpCompatBMP ){
		::SelectObject( m_hdcCompatDC, m_hbmpCompatBMPOld );
		::DeleteObject( m_hbmpCompatBMP );
		m_hbmpCompatBMP = NULL;
		m_hbmpCompatBMPOld = NULL;
		m_nCompatBMPWidth = -1;
		m_nCompatBMPHeight = -1;
	}
}

/*!	キャレットの作成

	@param nCaretColor [in]	キャレットの色種別 (0:通常, 1:IME ON)
	@param nWidth [in]		キャレット幅
	@param nHeight [in]		キャレット高

	@date 2006.12.07 ryoji 新規作成
*/
void CEditView::CreateEditCaret( COLORREF crCaret, COLORREF crBack, int nWidth, int nHeight )
{
	//
	// キャレット用のビットマップを作成する
	//
	// Note: ウィンドウ互換のメモリ DC 上で PatBlt を用いてキャレット色と背景色を XOR 結合
	//       することで，目的のビットマップを得る．
	//       ※ 256 色環境では RGB 値を単純に直接演算してもキャレット色を出すための正しい
	//          ビットマップ色は得られない．
	//       参考: [HOWTO] キャレットの色を制御する方法
	//             http://support.microsoft.com/kb/84054/ja
	//

	HBITMAP hbmpCaret;	// キャレット用のビットマップ

	HDC hdc = ::GetDC( m_hWnd );

	hbmpCaret = ::CreateCompatibleBitmap( hdc, nWidth, nHeight );
	HDC hdcMem = ::CreateCompatibleDC( hdc );
	HBITMAP hbmpOld = (HBITMAP)::SelectObject( hdcMem, hbmpCaret );
	HBRUSH hbrCaret = ::CreateSolidBrush( crCaret );
	HBRUSH hbrBack = ::CreateSolidBrush( crBack );
	HBRUSH hbrOld = (HBRUSH)::SelectObject( hdcMem, hbrCaret );
	::PatBlt( hdcMem, 0, 0, nWidth, nHeight, PATCOPY );
	::SelectObject( hdcMem, hbrBack );
	::PatBlt( hdcMem, 0, 0, nWidth, nHeight, PATINVERT );
	::SelectObject( hdcMem, hbrOld );
	::SelectObject( hdcMem, hbmpOld );
	::DeleteObject( hbrCaret );
	::DeleteObject( hbrBack );
	::DeleteDC( hdcMem );

	::ReleaseDC( m_hWnd, hdc );

	// 以前のビットマップを破棄する
	if( m_hbmpCaret != NULL )
		::DeleteObject( m_hbmpCaret );
	m_hbmpCaret = hbmpCaret;

	// キャレットを作成する
	::CreateCaret( m_hWnd, hbmpCaret, nWidth, nHeight );
	return;
}


// 2002/07/22 novice
/*!
	キャレットの表示
*/
void CEditView::ShowCaret_( HWND hwnd )
{
	if ( m_bCaretShowFlag == false ){
		::ShowCaret( hwnd );
		m_bCaretShowFlag = true;
	}
}


/*!
	キャレットの非表示
*/
void CEditView::HideCaret_( HWND hwnd )
{
	if ( m_bCaretShowFlag == true ){
		::HideCaret( hwnd );
		m_bCaretShowFlag = false;
	}
}

/* キャレットの表示・更新 */
void CEditView::ShowEditCaret( void )
{
	const char*		pLine;
	int				nLineLen;
	int				nCaretWidth = 0;
	int				nCaretHeight = 0;
	int				nIdxFrom;
	int				nCharChars;

/*
	フォーカスが無いときに内部的にキャレット作成すると暗黙的にキャレット破棄（※）されても
	キャレットがある（m_nCaretWidth != 0）ということになってしまい、フォーカスを取得しても
	キャレットが出てこなくなる場合がある
	フォーカスが無いときはキャレットを作成／表示しないようにする

	※キャレットはスレッドにひとつだけなので例えばエディットボックスがフォーカス取得すれば
	　別形状のキャレットに暗黙的に差し替えられるしフォーカスを失えば暗黙的に破棄される

	2007.12.11 ryoji
	ドラッグアンドドロップ編集中はキャレットが必要で暗黙破棄の要因も無いので例外的に表示する
*/
	if( ::GetFocus() != m_hWnd && !m_bDragMode ){
		m_nCaretWidth = 0;
		return;
	}

	/* キャレットの幅、高さを決定 */
	if( 0 == m_pShareData->m_Common.m_sGeneral.GetCaretType() ){	/* カーソルのタイプ 0=win 1=dos */
		nCaretHeight = m_nCharHeight;					/* キャレットの高さ */
		if( IsInsMode() /* Oct. 2, 2005 genta */ ){
			nCaretWidth = 2;
		}else{
			const CLayout* pcLayout;
			nCaretWidth = ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( m_ptCaretPos.y, &nLineLen, &pcLayout );
			if( NULL != pLine ){
				/* 指定された桁に対応する行のデータ内の位置を調べる */
				nIdxFrom = LineColumnToIndex( pcLayout, m_ptCaretPos.x );
				if( nIdxFrom >= nLineLen ||
					pLine[nIdxFrom] == CR || pLine[nIdxFrom] == LF ||
					pLine[nIdxFrom] == TAB ){
					nCaretWidth = ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
				}else{
					// 2005-09-02 D.S.Koba GetSizeOfChar
					nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, nIdxFrom );
					if( 0 < nCharChars ){
						nCaretWidth = ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace ) * nCharChars;
					}
				}
			}
		}
	}else
	if( 1 == m_pShareData->m_Common.m_sGeneral.GetCaretType() ){	/* カーソルのタイプ 0=win 1=dos */
		if( IsInsMode() /* Oct. 2, 2005 genta */ ){
			nCaretHeight = m_nCharHeight / 2;			/* キャレットの高さ */
		}else{
			nCaretHeight = m_nCharHeight;				/* キャレットの高さ */
		}
		const CLayout* pcLayout;
		nCaretWidth = ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( m_ptCaretPos.y, &nLineLen, &pcLayout );
		if( NULL != pLine ){
			/* 指定された桁に対応する行のデータ内の位置を調べる */
			nIdxFrom = LineColumnToIndex( pcLayout, m_ptCaretPos.x );
			if( nIdxFrom >= nLineLen ||
				pLine[nIdxFrom] == CR || pLine[nIdxFrom] == LF ||
				pLine[nIdxFrom] == TAB ){
				nCaretWidth = ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
			}else{
				// 2005-09-02 D.S.Koba GetSizeOfChar
				nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, nIdxFrom );
				if( 0 < nCharChars ){
					nCaretWidth = ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace ) * nCharChars;
				}
			}
		}

	}

#if 0
	hdc = ::GetDC( m_hWnd );
#endif
	//	キャレット色の取得
	ColorInfo* ColorInfoArr = m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr;
	int nCaretColor = ( ColorInfoArr[COLORIDX_CARET_IME].m_bDisp && IsImeON() )? COLORIDX_CARET_IME: COLORIDX_CARET;
	COLORREF crCaret = ColorInfoArr[nCaretColor].m_sColorAttr.m_cTEXT;
	COLORREF crBack = ColorInfoArr[COLORIDX_TEXT].m_sColorAttr.m_cBACK;

	if( m_nCaretWidth == 0 ){
		/* キャレットがなかった場合 */
		/* キャレットの作成 */
		CreateEditCaret( crCaret, crBack, nCaretWidth, nCaretHeight );	// 2006.12.07 ryoji
		m_bCaretShowFlag = false; // 2002/07/22 novice
	}else{
		if( m_nCaretWidth != nCaretWidth || m_nCaretHeight != nCaretHeight ||
			m_crCaret != crCaret || m_crBack != crBack ){
			/* キャレットはあるが、大きさや色が変わった場合 */
			/* 現在のキャレットを削除 */
			::DestroyCaret();

			/* キャレットの作成 */
			CreateEditCaret( crCaret, crBack, nCaretWidth, nCaretHeight );	// 2006.12.07 ryoji
			m_bCaretShowFlag = false; // 2002/07/22 novice
		}else{
			/* キャレットはあるし、大きさも変わっていない場合 */
			/* キャレットを隠す */
			HideCaret_( m_hWnd ); // 2002/07/22 novice
		}
	}
	/* キャレットの位置を調整 */
	int nPosX = m_nViewAlignLeft + (m_ptCaretPos.x - m_nViewLeftCol) * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
	int nPosY = m_nViewAlignTop  + (m_ptCaretPos.y - m_nViewTopLine) * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight ) + m_nCharHeight - nCaretHeight;
	::SetCaretPos( nPosX, nPosY );
	if ( m_nViewAlignLeft <= nPosX && m_nViewAlignTop <= nPosY ){
		/* キャレットの表示 */
		ShowCaret_( m_hWnd ); // 2002/07/22 novice
	}

	m_nCaretWidth = nCaretWidth;
	m_nCaretHeight = nCaretHeight;	/* キャレットの高さ */
	m_crCaret = crCaret;	//	2006.12.07 ryoji
	m_crBack = crBack;		//	2006.12.07 ryoji
	SetIMECompFormPos();
}





/* 入力フォーカスを受け取ったときの処理 */
void CEditView::OnSetFocus( void )
{
	// 2004.04.02 Moca EOFのみのレイアウト行は、0桁目のみ有効.EOFより下の行のある場合は、EOF位置にする
	{
		CLayoutPoint ptPos = m_ptCaretPos;
		if( GetAdjustCursorPos( &ptPos ) ){
			MoveCursor( ptPos.x, ptPos.y, false );
			m_nCaretPosX_Prev = m_ptCaretPos.x;
		}
	}

	ShowEditCaret();

	SetIMECompFormFont();

	/* ルーラのカーソルをグレーから黒に変更する */
	HDC hdc = ::GetDC( m_hWnd );
	DispRuler( hdc );
	::ReleaseDC( m_hWnd, hdc );

	// 03/02/18 対括弧の強調表示(描画) ai
	m_bDrawBracketPairFlag = TRUE;
	DrawBracketPair( true );

	m_pcEditWnd->AcceptSharedSearchKey();
}


/* 入力フォーカスを失ったときの処理 */
void CEditView::OnKillFocus( void )
{
	// 03/02/18 対括弧の強調表示(消去) ai
	DrawBracketPair( false );
	m_bDrawBracketPairFlag = FALSE;

	DestroyCaret();

	/* ルーラー描画 */
	/* ルーラのカーソルを黒からグレーに変更する */
	HDC	hdc = ::GetDC( m_hWnd );
	DispRuler( hdc );
	::ReleaseDC( m_hWnd, hdc );

	/* 辞書Tipが起動されている */
	if( 0 == m_dwTipTimer ){
		/* 辞書Tipを消す */
		m_cTipWnd.Hide();
		m_dwTipTimer = ::GetTickCount();	/* 辞書Tip起動タイマー */
	}

	if( m_bHokan ){
		m_pcEditWnd->m_cHokanMgr.Hide();
		m_bHokan = FALSE;
	}

	return;
}





/*! 垂直スクロールバーメッセージ処理

	@param nScrollCode [in]	スクロール種別 (Windowsから渡されるもの)
	@param nPos [in]		スクロール位置(THUMBTRACK用)
	@retval	実際にスクロールした行数

	@date 2004.09.11 genta スクロール行数を返すように．
		未使用のhwndScrollBar引数削除．
*/
int CEditView::OnVScroll( int nScrollCode, int nPos )
{
	int nScrollVal = 0;

	switch( nScrollCode ){
	case SB_LINEDOWN:
//		for( i = 0; i < 4; ++i ){
//			ScrollAtV( m_nViewTopLine + 1 );
//		}
		nScrollVal = ScrollAtV( m_nViewTopLine + m_pShareData->m_Common.m_sGeneral.m_nRepeatedScrollLineNum );
		break;
	case SB_LINEUP:
//		for( i = 0; i < 4; ++i ){
//			ScrollAtV( m_nViewTopLine - 1 );
//		}
		nScrollVal = ScrollAtV( m_nViewTopLine - m_pShareData->m_Common.m_sGeneral.m_nRepeatedScrollLineNum );
		break;
	case SB_PAGEDOWN:
		nScrollVal = ScrollAtV( m_nViewTopLine + m_nViewRowNum );
		break;
	case SB_PAGEUP:
		nScrollVal = ScrollAtV( m_nViewTopLine - m_nViewRowNum );
		break;
	case SB_THUMBPOSITION:
		nScrollVal = ScrollAtV( nPos );
		break;
	case SB_THUMBTRACK:
		nScrollVal = ScrollAtV( nPos );
		break;
	case SB_TOP:
		nScrollVal = ScrollAtV( 0 );
		break;
	case SB_BOTTOM:
		nScrollVal = ScrollAtV(( m_pcEditDoc->m_cLayoutMgr.GetLineCount() ) - m_nViewRowNum );
		break;
	default:
		break;
	}
	return nScrollVal;
}

/*! 水平スクロールバーメッセージ処理

	@param nScrollCode [in]	スクロール種別 (Windowsから渡されるもの)
	@param nPos [in]		スクロール位置(THUMBTRACK用)
	@retval	実際にスクロールした桁数

	@date 2004.09.11 genta スクロール桁数を返すように．
		未使用のhwndScrollBar引数削除．
*/
int CEditView::OnHScroll( int nScrollCode, int nPos )
{
	int nScrollVal = 0;

	m_bRedrawRuler = true; // YAZAKI
	switch( nScrollCode ){
	case SB_LINELEFT:
		nScrollVal = ScrollAtH( m_nViewLeftCol - 4 );
		break;
	case SB_LINERIGHT:
		nScrollVal = ScrollAtH( m_nViewLeftCol + 4 );
		break;
	case SB_PAGELEFT:
		nScrollVal = ScrollAtH( m_nViewLeftCol - m_nViewColNum );
		break;
	case SB_PAGERIGHT:
		nScrollVal = ScrollAtH( m_nViewLeftCol + m_nViewColNum );
		break;
	case SB_THUMBPOSITION:
		nScrollVal = ScrollAtH( nPos );
//		MYTRACE( _T("nPos=%d\n"), nPos );
		break;
	case SB_THUMBTRACK:
		nScrollVal = ScrollAtH( nPos );
//		MYTRACE( _T("nPos=%d\n"), nPos );
		break;
	case SB_LEFT:
		nScrollVal = ScrollAtH( 0 );
		break;
	case SB_RIGHT:
		//	Aug. 14, 2005 genta 折り返し幅をLayoutMgrから取得するように
		nScrollVal = ScrollAtH( m_pcEditDoc->m_cLayoutMgr.GetMaxLineKetas() - m_nViewColNum );
		break;
	}
	return nScrollVal;
}

/* 2点を対角とする矩形を求める */
void CEditView::TwoPointToRect(
		RECT*	prcRect,
	int		nLineFrom,
	int		nColmFrom,
	int		nLineTo,
	int		nColmTo
)
{
	if( nLineFrom < nLineTo ){
		prcRect->top	= nLineFrom;
		prcRect->bottom	= nLineTo;
	}else{
		prcRect->top	= nLineTo;
		prcRect->bottom	= nLineFrom;
	}
	if( nColmFrom < nColmTo ){
		prcRect->left	= nColmFrom;
		prcRect->right	= nColmTo;
	}else{
		prcRect->left	= nColmTo;
		prcRect->right	= nColmFrom;
	}
	return;

}

/*! 選択領域の描画

	@date 2006.10.01 Moca 重複コード削除．矩形作画改善．
	@date 2007.09.09 Moca 互換BMPによる画面バッファ
		画面バッファが有効時、画面と互換BMPの両方の反転処理を行う。
*/
void CEditView::DrawSelectArea( void )
{
	if( !m_bDrawSWITCH ){
		return;
	}

	int			nFromLine;
	int			nFromCol;
	int			nToLine;
	int			nToCol;
	int			nLineNum;

	m_bDrawSelectArea = true;	// 2002/12/13 ai

	// 2006.10.01 Moca 重複コード統合
	HDC         hdc = ::GetDC( m_hWnd );
	HBRUSH      hBrush = ::CreateSolidBrush( SELECTEDAREA_RGB );
	HBRUSH      hBrushOld = (HBRUSH)::SelectObject( hdc, hBrush );
	int         nROP_Old = ::SetROP2( hdc, SELECTEDAREA_ROP2 );
	// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
	HBRUSH		hBrushCompatOld;
	int			nROPCompatOld;
	if( m_hbmpCompatBMP ){
		hBrushCompatOld = (HBRUSH)::SelectObject( m_hdcCompatDC, hBrush );
		nROPCompatOld = ::SetROP2( m_hdcCompatDC, SELECTEDAREA_ROP2 );
	}
	// To Here 2007.09.09 Moca

//	MYTRACE( _(T"DrawSelectArea()  m_bBeginBoxSelect=%s\n"), m_bBeginBoxSelect?"true":"false" );
	if( m_bBeginBoxSelect ){		/* 矩形範囲選択中 */
		// 2001.12.21 hor 矩形エリアにEOFがある場合、RGN_XORで結合すると
		// EOF以降のエリアも反転してしまうので、この場合はRedrawを使う
		// 2002.02.16 hor ちらつきを抑止するためEOF以降のエリアが反転したらもう一度反転して元に戻すことにする
		//if((m_nViewTopLine+m_nViewRowNum+1>=m_pcEditDoc->m_cLayoutMgr.GetLineCount()) &&
		//   (m_sSelect.m_ptTo.y+1 >= m_pcEditDoc->m_cLayoutMgr.GetLineCount() ||
		//	m_sSelectOld.m_ptTo.y+1 >= m_pcEditDoc->m_cLayoutMgr.GetLineCount() ) ) {
		//	Redraw();
		//	return;
		//}

		const int nCharWidth = m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace;
		const int nCharHeight = m_nCharHeight + m_pcEditDoc->GetDocumentAttribute().m_nLineSpace;

		RECT  rcOld;
		RECT  rcNew;
		HRGN  hrgnOld = NULL;
		HRGN  hrgnNew = NULL;
		HRGN  hrgnDraw = NULL;

		/* 2点を対角とする矩形を求める */
		TwoPointToRect(
			&rcOld,
			m_sSelectOld.m_ptFrom.y,	/* 範囲選択開始行 */
			m_sSelectOld.m_ptFrom.x,	/* 範囲選択開始桁 */
			m_sSelectOld.m_ptTo.y,		/* 範囲選択終了行 */
			m_sSelectOld.m_ptTo.x		/* 範囲選択終了桁 */
		);
		if( rcOld.left	< m_nViewLeftCol ){
			rcOld.left = m_nViewLeftCol;
		}
		if( rcOld.right	< m_nViewLeftCol ){
			rcOld.right = m_nViewLeftCol;
		}
		if( rcOld.right > m_nViewLeftCol + m_nViewColNum + 1 ){
			rcOld.right = m_nViewLeftCol + m_nViewColNum + 1;
		}
		if( rcOld.top < m_nViewTopLine ){
			rcOld.top = m_nViewTopLine;
		}
		if( rcOld.bottom < m_nViewTopLine - 1 ){	// 2010.11.02 ryoji 追加（画面上端よりも上にある矩形選択を解除するとルーラーが反転表示になる問題の修正）
			rcOld.bottom = m_nViewTopLine - 1;
		}
		if( rcOld.bottom > m_nViewTopLine + m_nViewRowNum ){
			rcOld.bottom = m_nViewTopLine + m_nViewRowNum;
		}
		rcOld.left		= (m_nViewAlignLeft - m_nViewLeftCol * nCharWidth) + rcOld.left  * nCharWidth;
		rcOld.right		= (m_nViewAlignLeft - m_nViewLeftCol * nCharWidth) + rcOld.right * nCharWidth;
		rcOld.top		= ( rcOld.top - m_nViewTopLine ) * nCharHeight + m_nViewAlignTop;
		rcOld.bottom	= ( rcOld.bottom + 1 - m_nViewTopLine ) * nCharHeight + m_nViewAlignTop;
		hrgnOld = ::CreateRectRgnIndirect( &rcOld );

		/* 2点を対角とする矩形を求める */
		TwoPointToRect(
			&rcNew,
			m_sSelect.m_ptFrom.y,		/* 範囲選択開始行 */
			m_sSelect.m_ptFrom.x,		/* 範囲選択開始桁 */
			m_sSelect.m_ptTo.y,		/* 範囲選択終了行 */
			m_sSelect.m_ptTo.x			/* 範囲選択終了桁 */
		);
		if( rcNew.left	< m_nViewLeftCol ){
			rcNew.left = m_nViewLeftCol;
		}
		if( rcNew.right	< m_nViewLeftCol ){
			rcNew.right = m_nViewLeftCol;
		}
		if( rcNew.right > m_nViewLeftCol + m_nViewColNum + 1 ){
			rcNew.right = m_nViewLeftCol + m_nViewColNum + 1;
		}
		if( rcNew.top < m_nViewTopLine ){
			rcNew.top = m_nViewTopLine;
		}
		if( rcNew.bottom < m_nViewTopLine - 1 ){	// 2010.11.02 ryoji 追加（画面上端よりも上にある矩形選択を解除するとルーラーが反転表示になる問題の修正）
			rcNew.bottom = m_nViewTopLine - 1;
		}
		if( rcNew.bottom > m_nViewTopLine + m_nViewRowNum ){
			rcNew.bottom = m_nViewTopLine + m_nViewRowNum;
		}
		rcNew.left		= (m_nViewAlignLeft - m_nViewLeftCol * nCharWidth) + rcNew.left  * nCharWidth;
		rcNew.right		= (m_nViewAlignLeft - m_nViewLeftCol * nCharWidth) + rcNew.right * nCharWidth;
		rcNew.top		= (rcNew.top - m_nViewTopLine) * nCharHeight + m_nViewAlignTop;
		rcNew.bottom	= (rcNew.bottom + 1 - m_nViewTopLine) * nCharHeight + m_nViewAlignTop;
		hrgnNew = ::CreateRectRgnIndirect( &rcNew );

		// 矩形作画。
		{
			/* ::CombineRgn()の結果を受け取るために、適当なリージョンを作る */
			hrgnDraw = ::CreateRectRgnIndirect( &rcNew );

			/* 旧選択矩形と新選択矩形のリージョンを結合し､ 重なりあう部分だけを除去します */
			if( NULLREGION != ::CombineRgn( hrgnDraw, hrgnOld, hrgnNew, RGN_XOR ) ){

				// 2002.02.16 hor
				// 結合後のエリアにEOFが含まれる場合はEOF以降の部分を除去します
				// 2006.10.01 Moca リーソースリークを修正したら、チラつくようになったため、
				// 抑えるために EOF以降をリージョンから削除して1度の作画にする

				// 2006.10.01 Moca Start EOF位置計算をGetEndLayoutPosに書き換え。
				CLayoutPoint ptLast;
				m_pcEditDoc->m_cLayoutMgr.GetEndLayoutPos( &ptLast );
				// 2006.10.01 Moca End
				if(m_sSelect.m_ptFrom.y>=ptLast.y || m_sSelect.m_ptTo.y>=ptLast.y ||
					m_sSelectOld.m_ptFrom.y>=ptLast.y || m_sSelectOld.m_ptTo.y>=ptLast.y){
					//	Jan. 24, 2004 genta nLastLenは物理桁なので変換必要
					//	最終行にTABが入っていると反転範囲が不足する．
					//	2006.10.01 Moca GetEndLayoutPosで処理するためColumnToIndexは不要に。
					rcNew.left = m_nViewAlignLeft + (m_nViewLeftCol + ptLast.x) * nCharWidth;
					rcNew.right = m_nViewAlignLeft + m_nViewCx;
					rcNew.top = (ptLast.y - m_nViewTopLine) * nCharHeight + m_nViewAlignTop;
					rcNew.bottom = rcNew.top + nCharHeight;
					// 2006.10.01 Moca GDI(リージョン)リソースリーク修正
					HRGN hrgnEOFNew = ::CreateRectRgnIndirect( &rcNew );
					::CombineRgn( hrgnDraw, hrgnDraw, hrgnEOFNew, RGN_DIFF );
					::DeleteObject( hrgnEOFNew );
				}
				::PaintRgn( hdc, hrgnDraw );
				// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
				if( m_hbmpCompatBMP ){
					::PaintRgn( m_hdcCompatDC, hrgnDraw );
				}
				// To Here 2007.09.09 Moca
			}
		}

		//////////////////////////////////////////
		/* デバッグ用 リージョン矩形のダンプ */
//@@		TraceRgn( hrgnDraw );


		if( NULL != hrgnDraw ){
			::DeleteObject( hrgnDraw );
		}
		if( NULL != hrgnNew ){
			::DeleteObject( hrgnNew );
		}
		if( NULL != hrgnOld ){
			::DeleteObject( hrgnOld );
		}
	}else{

		/* 現在描画されている範囲と始点が同じ */
		if( m_sSelect.m_ptFrom.y == m_sSelectOld.m_ptFrom.y &&
			m_sSelect.m_ptFrom.x  == m_sSelectOld.m_ptFrom.x ){
			/* 範囲が後方に拡大された */
			if( m_sSelect.m_ptTo.y > m_sSelectOld.m_ptTo.y ||
			   (m_sSelect.m_ptTo.y == m_sSelectOld.m_ptTo.y &&
				m_sSelect.m_ptTo.x > m_sSelectOld.m_ptTo.x ) ){
				nFromLine	= m_sSelectOld.m_ptTo.y;
				nFromCol	= m_sSelectOld.m_ptTo.x;
				nToLine		= m_sSelect.m_ptTo.y;
				nToCol		= m_sSelect.m_ptTo.x;
			}else{
				nFromLine	= m_sSelect.m_ptTo.y;
				nFromCol	= m_sSelect.m_ptTo.x;
				nToLine		= m_sSelectOld.m_ptTo.y;
				nToCol		= m_sSelectOld.m_ptTo.x;
			}
			for( nLineNum = nFromLine; nLineNum <= nToLine; ++nLineNum ){
				if( nLineNum >= m_nViewTopLine && nLineNum <= m_nViewTopLine + m_nViewRowNum + 1 ){
					DrawSelectAreaLine( hdc, nLineNum, nFromLine, nFromCol, nToLine, nToCol );
				}
			}
		}else
		if( m_sSelect.m_ptTo.y == m_sSelectOld.m_ptTo.y &&
			m_sSelect.m_ptTo.x  == m_sSelectOld.m_ptTo.x ){
			/* 範囲が前方に拡大された */
			if( m_sSelect.m_ptFrom.y < m_sSelectOld.m_ptFrom.y ||
			   (m_sSelect.m_ptFrom.y == m_sSelectOld.m_ptFrom.y &&
				m_sSelect.m_ptFrom.x < m_sSelectOld.m_ptFrom.x ) ){
				nFromLine	= m_sSelect.m_ptFrom.y;
				nFromCol	= m_sSelect.m_ptFrom.x;
				nToLine		= m_sSelectOld.m_ptFrom.y;
				nToCol		= m_sSelectOld.m_ptFrom.x;
			}else{
				nFromLine	= m_sSelectOld.m_ptFrom.y;
				nFromCol	= m_sSelectOld.m_ptFrom.x;
				nToLine		= m_sSelect.m_ptFrom.y;
				nToCol		= m_sSelect.m_ptFrom.x;
			}
			for( nLineNum = nFromLine; nLineNum <= nToLine; ++nLineNum ){
				if( nLineNum >= m_nViewTopLine && nLineNum <= m_nViewTopLine + m_nViewRowNum + 1 ){
					DrawSelectAreaLine( hdc, nLineNum, nFromLine, nFromCol, nToLine, nToCol );
				}
			}
		}else{
			nFromLine		= m_sSelectOld.m_ptFrom.y;
			nFromCol		= m_sSelectOld.m_ptFrom.x;
			nToLine			= m_sSelectOld.m_ptTo.y;
			nToCol			= m_sSelectOld.m_ptTo.x;
			for( nLineNum	= nFromLine; nLineNum <= nToLine; ++nLineNum ){
				if( nLineNum >= m_nViewTopLine && nLineNum <= m_nViewTopLine + m_nViewRowNum + 1 ){
					DrawSelectAreaLine( hdc, nLineNum, nFromLine, nFromCol, nToLine, nToCol );
				}
			}
			nFromLine	= m_sSelect.m_ptFrom.y;
			nFromCol	= m_sSelect.m_ptFrom.x;
			nToLine		= m_sSelect.m_ptTo.y;
			nToCol		= m_sSelect.m_ptTo.x;
			for( nLineNum = nFromLine; nLineNum <= nToLine; ++nLineNum ){
				if( nLineNum >= m_nViewTopLine && nLineNum <= m_nViewTopLine + m_nViewRowNum + 1 ){
					DrawSelectAreaLine( hdc, nLineNum, nFromLine, nFromCol, nToLine, nToCol );
				}
			}
		}
	}
	
	// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
	if( m_hbmpCompatBMP ){
		::SetROP2( m_hdcCompatDC, nROPCompatOld );
		::SelectObject( m_hdcCompatDC, hBrushCompatOld );
	}
	// To Here 2007.09.09 Moca
	// 2006.10.01 Moca 重複コード統合
	::SetROP2( hdc, nROP_Old );
	::SelectObject( hdc, hBrushOld );
	::DeleteObject( hBrush );
	::ReleaseDC( m_hWnd, hdc );
	//	Jul. 9, 2005 genta 選択領域の情報を表示
	PrintSelectionInfoMsg();
	return;
}




/*! 選択領域の中の指定行の描画

	@param[in] hdc 描画領域のDevice Context Handle
	@param[in] nLineNum 描画対象行(レイアウト行)
	@param[in] nFromLine 選択開始行(レイアウト座標)
	@param[in] nFromCol  選択開始桁(レイアウト座標)
	@param[in] nToLine   選択終了行(レイアウト座標)
	@param[in] nToCol    選択終了桁(レイアウト座標)

	複数行に渡る選択範囲のうち，nLineNumで指定された1行分だけを描画する．
	選択範囲は固定されたままnLineNumのみが必要行分変化しながら呼びだされる．

	@date 2006.03.29 Moca 3000桁制限を撤廃．

*/
void CEditView::DrawSelectAreaLine(
		HDC hdc, int nLineNum, int nFromLine, int nFromCol, int nToLine, int nToCol
)
{
//	MYTRACE( _T("CEditView::DrawSelectAreaLine()\n") );
	RECT			rcClip;
	int				nSelectFrom;	// 描画行の選択開始桁位置
	int				nSelectTo;		// 描画行の選択開始終了位置

	if( nFromLine == nToLine ){
		nSelectFrom = nFromCol;
		nSelectTo	= nToCol;
	}else{
		// 2006.03.29 Moca 行末までの長さを求める位置を上からここに移動
		int nPosX = 0;
		const CLayout* pcLayout = m_pcEditDoc->m_cLayoutMgr.SearchLineByLayoutY( nLineNum );
		CMemoryIterator<CLayout> it( pcLayout, m_pcEditDoc->m_cLayoutMgr.GetTabSpace() );
		while( !it.end() ){
			it.scanNext();
			if ( it.getIndex() + it.getIndexDelta() > pcLayout->GetLengthWithoutEOL() ){
				nPosX ++;
				break;
			}
			// 2006.03.28 Moca 画面外まで求めたら打ち切る
			if( it.getColumn() > m_nViewLeftCol + m_nViewColNum ){
				break;
			}
			it.addDelta();
		}
		nPosX += it.getColumn();
		
		if( nLineNum == nFromLine ){
			nSelectFrom = nFromCol;
			nSelectTo	= nPosX;
		}else
		if( nLineNum == nToLine ){
			nSelectFrom = pcLayout ? pcLayout->GetIndent() : 0;
			nSelectTo	= nToCol;
		}else{
			nSelectFrom = pcLayout ? pcLayout->GetIndent() : 0;
			nSelectTo	= nPosX;
		}
		// 2006.05.24 Mocaフリーカーソル選択(選択開始/終了行)で
		// To < From になることがある。必ず From < To になるように入れ替える。
		if( nSelectTo < nSelectFrom ){
			int t = nSelectFrom;
			nSelectFrom = nSelectTo;
			nSelectTo = t;
		}
	}
	
	// 2006.03.28 Moca ウィンドウ幅が大きいと正しく反転しない問題を修正
	if( nSelectFrom < m_nViewLeftCol ){
		nSelectFrom = m_nViewLeftCol;
	}
	int		nLineHeight = m_nCharHeight + m_pcEditDoc->GetDocumentAttribute().m_nLineSpace;
	int		nCharWidth = m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace;
	rcClip.left		= (m_nViewAlignLeft - m_nViewLeftCol * nCharWidth) + nSelectFrom * nCharWidth;
	rcClip.right	= (m_nViewAlignLeft - m_nViewLeftCol * nCharWidth) + nSelectTo   * nCharWidth;
	rcClip.top		= (nLineNum - m_nViewTopLine) * nLineHeight + m_nViewAlignTop;
	rcClip.bottom	= rcClip.top + nLineHeight;
	if( rcClip.right > m_nViewAlignLeft + m_nViewCx ){
		rcClip.right = m_nViewAlignLeft + m_nViewCx;
	}
	//	必要なときだけ。
	if ( rcClip.right != rcClip.left ){
		m_cUnderLine.CaretUnderLineOFF( true );	//	YAZAKI
		
		// 2006.03.28 Moca 表示域内のみ処理する
		if( nSelectFrom <= m_nViewLeftCol + m_nViewColNum && m_nViewLeftCol < nSelectTo ){
			HRGN hrgnDraw = ::CreateRectRgn( rcClip.left, rcClip.top, rcClip.right, rcClip.bottom );
			::PaintRgn( hdc, hrgnDraw );
			// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
			if( m_hbmpCompatBMP ){
				::PaintRgn( m_hdcCompatDC, hrgnDraw );
			}
			// To Here 2007.09.09 Moca
			::DeleteObject( hrgnDraw );
		}
	}

//	::Rectangle( hdc, rcClip.left, rcClip.top, rcClip.right + 1, rcClip.bottom + 1);
//	::FillRect( hdc, &rcClip, hBrushTextCol );

//	//	/* デバッグモニタに出力 */
//	m_cShareData.TraceOut( "DrawSelectAreaLine() rcClip.left=%d, rcClip.top=%d, rcClip.right=%d, rcClip.bottom=%d\n", rcClip.left, rcClip.top, rcClip.right, rcClip.bottom );

	return;
}


/* フォントの変更 */
void CEditView::SetFont( void )
{
	HDC			hdc;
	HFONT		hFontOld;
	TEXTMETRIC	tm;
	int			i;
	SIZE		sz;

	hdc = ::GetDC( m_hWnd );
	hFontOld = (HFONT)::SelectObject( hdc, m_pcViewFont->GetFontHan() );
//	hFontOld = (HFONT)::SelectObject( hdc, m_hFont_HAN_BOLD );
	::GetTextMetrics( hdc, &tm );


// 1999.12.9
//	m_nCharWidth = tm.tmAveCharWidth - 1;
//	m_nCharHeight = tm.tmHeight + tm.tmExternalLeading;
	/* 文字の大きさを調べる */
// 2000.2.8
//	::GetTextExtentPoint32( hdc, "X", 1, &sz );
//	m_nCharHeight = sz.cy;
//	m_nCharWidth = sz.cx;
	::GetTextExtentPoint32( hdc, "大", 2, &sz );
	m_nCharHeight = sz.cy;
	m_nCharWidth = sz.cx / 2;


// 行の高さを2の倍数にする
//	if( ( m_nCharHeight + m_pcEditDoc->GetDocumentAttribute().m_nLineSpace ) % 2 ){
//		++m_nCharHeight;
//	}

	m_nViewColNum = (m_nViewCx - 1) / ( m_nCharWidth  + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );	/* 表示域の桁数 */
	m_nViewRowNum = (m_nViewCy - 1) / ( m_nCharHeight + m_pcEditDoc->GetDocumentAttribute().m_nLineSpace );	/* 表示域の行数 */
	/* 行番号表示に必要な幅を設定 */
	DetectWidthOfLineNumberArea( false );
	/* 文字列描画用文字幅配列 */
	for( i = 0; i < ( sizeof(m_pnDx) / sizeof(m_pnDx[0]) ); ++i ){
		m_pnDx[i] = ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
	}
	::SelectObject( hdc, hFontOld );
	::ReleaseDC( m_hWnd, hdc );
	::InvalidateRect( m_hWnd, NULL, TRUE );
//	2002/05/12 YAZAKI 不要と思われたので。
//	if( m_nCaretWidth == 0 ){	/* キャレットがなかった場合 */
//	}else{
//		OnKillFocus();
//		OnSetFocus();
//	}
	//	Oct. 11, 2002 genta IMEのフォントも変更
	SetIMECompFormFont();
	return;
}



/* 行番号表示に必要な桁数を計算 */
int CEditView::DetectWidthOfLineNumberArea_calculate( void )
{
	int			i;
	int			nAllLines;
	int			nWork;
	if( m_pcEditDoc->GetDocumentAttribute().m_bLineNumIsCRLF ){	/* 行番号の表示 false=折り返し単位／true=改行単位 */
		nAllLines = m_pcEditDoc->m_cDocLineMgr.GetLineCount();
	}else{
		nAllLines = m_pcEditDoc->m_cLayoutMgr.GetLineCount();
	}
	if( 0 < nAllLines ){
		nWork = 100;
		for( i = 3; i < 12; ++i ){
			if( nWork > nAllLines ){	// Oct. 18, 2003 genta 式を整理
				break;
			}
			nWork *= 10;
		}
	}else{
		//	2003.09.11 wmlhq 行番号が1桁のときと幅を合わせる
		i = 3;
	}
	return i;

}


/*
行番号表示に必要な幅を設定幅が変更された場合はTRUEを返す
*/
BOOL CEditView::DetectWidthOfLineNumberArea( bool bRedraw )
{
	int				i;
	PAINTSTRUCT		ps;
	HDC				hdc;
//	int				nAllLines;
//	int				nWork;
	int				m_nViewAlignLeftNew;
	int				nCxVScroll;
	RECT			rc;

	if( m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_GYOU].m_bDisp ){
		/* 行番号表示に必要な桁数を計算 */
		i = DetectWidthOfLineNumberArea_calculate();
		m_nViewAlignLeftNew = ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace ) * (i + 1);	/* 表示域の左端座標 */
		m_nViewAlignLeftCols = i + 1;
	}else{
		m_nViewAlignLeftNew = 8;
		m_nViewAlignLeftCols = 0;
	}
	//	Sep 18, 2002 genta
	m_nViewAlignLeftNew += m_pShareData->m_Common.m_sWindow.m_nLineNumRightSpace;
	if( m_nViewAlignLeftNew != m_nViewAlignLeft ){
		m_nViewAlignLeft = m_nViewAlignLeftNew;
		::GetClientRect( m_hWnd, &rc );
		nCxVScroll = ::GetSystemMetrics( SM_CXVSCROLL );
		m_nViewCx = (rc.right - rc.left) - nCxVScroll - m_nViewAlignLeft;	/* 表示域の幅 */
		// 2008.05.23 nasukoji	表示域の桁数も算出する（右端カーソル移動時の表示場所ずれへの対処）
		m_nViewColNum = (m_nViewCx - 1) / ( m_nCharWidth  + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );	/* 表示域の桁数 */


		if( bRedraw ){
			/* 再描画 */
			hdc = ::GetDC( m_hWnd );
			ps.rcPaint.left = 0;
			ps.rcPaint.right = m_nViewAlignLeft + m_nViewCx;
			ps.rcPaint.top = 0;
			ps.rcPaint.bottom = m_nViewAlignTop + m_nViewCy;
//			OnKillFocus();
			m_cUnderLine.Lock();
			// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
			OnPaint( hdc, &ps, FALSE );
			// To Here 2007.09.09 Moca
			m_cUnderLine.UnLock();
//			OnSetFocus();
//			DispRuler( hdc );
			ShowEditCaret();
			::ReleaseDC( m_hWnd, hdc );
		}
		m_bRedrawRuler = true;
		return TRUE;
	}else{
		return FALSE;
	}
}





/** スクロールバーの状態を更新する

	タブバーのタブ切替時は SIF_DISABLENOSCROLL フラグでの有効化／無効化が正常に動作しない
	（不可視でサイズ変更していることによる影響か？）ので SIF_DISABLENOSCROLL で有効／無効
	の切替に失敗した場合には強制切替する

	@date 2008.05.24 ryoji 有効／無効の強制切替を追加
	@date 2008.06.08 ryoji 水平スクロール範囲にぶら下げ余白を追加
	@date 2009.08.28 nasukoji	「折り返さない」選択時のスクロールバー調整
*/
void CEditView::AdjustScrollBars( void )
{
	if( !m_bDrawSWITCH ){
		return;
	}


	int			nAllLines;
	int			nVScrollRate;
	SCROLLINFO	si;
	bool		bEnable;

	if( NULL != m_hwndVScrollBar ){
		/* 垂直スクロールバー */
		/* nAllLines / nVScrollRate < 65535 となる整数nVScrollRateを求める */
		nAllLines = m_pcEditDoc->m_cLayoutMgr.GetLineCount();
		nAllLines+=2;
		nVScrollRate = 1;
		while( nAllLines / nVScrollRate > 65535 ){
			++nVScrollRate;
		}
		si.cbSize = sizeof( si );
		si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
		si.nMin  = 0;
		si.nMax  = nAllLines / nVScrollRate - 1;	/* 全行数 */
		si.nPage = m_nViewRowNum / nVScrollRate;	/* 表示域の行数 */
		si.nPos  = m_nViewTopLine / nVScrollRate;	/* 表示域の一番上の行(0開始) */
		si.nTrackPos = nVScrollRate;
		::SetScrollInfo( m_hwndVScrollBar, SB_CTL, &si, TRUE );
		m_nVScrollRate = nVScrollRate;				/* 垂直スクロールバーの縮尺 */

		//	Nov. 16, 2002 genta
		//	縦スクロールバーがDisableになったときは必ず全体が画面内に収まるように
		//	スクロールさせる
		//	2005.11.01 aroka 判定条件誤り修正 (バーが消えてもスクロールしない)
		bEnable = ( m_nViewRowNum < nAllLines );
		if( bEnable != (::IsWindowEnabled( m_hwndVScrollBar ) != 0) ){
			::EnableWindow( m_hwndVScrollBar, bEnable? TRUE: FALSE );	// SIF_DISABLENOSCROLL 誤動作時の強制切替
		}
		if( !bEnable ){
			ScrollAtV( 0 );
		}
	}
	if( NULL != m_hwndHScrollBar ){
		/* 水平スクロールバー */
		si.cbSize = sizeof( si );
		si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
		si.nMin  = 0;
		//	Aug. 14, 2005 genta 折り返し幅をLayoutMgrから取得するように
		si.nMax  = GetRightEdgeForScrollBar() - 1;
		si.nPage = m_nViewColNum;		/* 表示域の桁数 */
		si.nPos  = m_nViewLeftCol;		/* 表示域の一番左の桁(0開始) */
		si.nTrackPos = 1;
		::SetScrollInfo( m_hwndHScrollBar, SB_CTL, &si, TRUE );

		//	2006.1.28 aroka 判定条件誤り修正 (バーが消えてもスクロールしない)
		bEnable = ( m_nViewColNum < GetRightEdgeForScrollBar() );
		if( bEnable != (::IsWindowEnabled( m_hwndHScrollBar ) != 0) ){
			::EnableWindow( m_hwndHScrollBar, bEnable? TRUE: FALSE );	// SIF_DISABLENOSCROLL 誤動作時の強制切替
		}
		if( !bEnable ){
			ScrollAtH( 0 );
		}
	}

	return;
}

/** 折り返し桁以後のぶら下げ余白計算
	@date 2008.06.08 ryoji 新規作成
*/
int CEditView::GetWrapOverhang( void ) const
{
	int nMargin = 0;
	if( m_pcEditDoc->GetDocumentAttribute().m_bKinsokuRet )
		nMargin += 2;	// 改行ぶら下げ
	if( m_pcEditDoc->GetDocumentAttribute().m_bKinsokuKuto )
		nMargin += 2;	// 句読点ぶら下げ
	return nMargin;
}

/** 「右端で折り返す」用にビューの桁数から折り返し桁数を計算する
	@param nViewColNum	[in] ビューの桁数
	@retval 折り返し桁数
	@date 2008.06.08 ryoji 新規作成
*/
int CEditView::ViewColNumToWrapColNum( int nViewColNum ) const
{
	// ぶら下げ余白を差し引く
	int nWidth = nViewColNum - GetWrapOverhang();

	// MINLINEKETAS未満の時はMINLINEKETASで折り返しとする
	if( nWidth < MINLINEKETAS )
		nWidth = MINLINEKETAS;		// 折り返し幅の最小桁数に設定

	return nWidth;
}

/*!
	@brief  スクロールバー制御用に右端座標を取得する

	「折り返さない」
		フリーカーソル状態の時はテキストの幅よりも右側へカーソルが移動できる
		ので、それを考慮したスクロールバーの制御が必要。
		本関数は、下記の内で最も大きな値（右端の座標）を返す。
		　・テキストの右端
		　・キャレット位置
		　・選択範囲の右端
	
	「指定桁で折り返す」
	「右端で折り返す」
		上記の場合折り返し桁以後のぶら下げ余白計算

	@return     右端のレイアウト座標を返す

	@note   「折り返さない」選択時は、スクロール後にキャレットが見えなく
	        ならない様にするために右マージンとして半角3個分固定で加算する。

	@date 2009.08.28 nasukoji	新規作成
*/
int CEditView::GetRightEdgeForScrollBar( void )
{
	// 折り返し桁以後のぶら下げ余白計算
	int nWidth = m_pcEditDoc->m_cLayoutMgr.GetMaxLineKetas() + GetWrapOverhang();
	
	if( m_pcEditDoc->m_nTextWrapMethodCur == WRAP_NO_TEXT_WRAP ){
		int nRightEdge = m_pcEditDoc->m_cLayoutMgr.GetMaxTextWidth();	// テキストの最大幅

		// 選択範囲あり かつ 範囲の右端がテキストの幅より右側
		if( IsTextSelected() ){
			// 開始位置・終了位置のより右側にある方で比較
			if( m_sSelect.m_ptFrom.x < m_sSelect.m_ptTo.x ){
				if( nRightEdge < m_sSelect.m_ptTo.x )
					nRightEdge = m_sSelect.m_ptTo.x;
			}else{
				if( nRightEdge < m_sSelect.m_ptFrom.x )
					nRightEdge = m_sSelect.m_ptFrom.x;
			}
		}

		// フリーカーソルモード かつ キャレット位置がテキストの幅より右側
		if( m_pShareData->m_Common.m_sGeneral.m_bIsFreeCursorMode && nRightEdge < m_ptCaretPos.x )
			nRightEdge = m_ptCaretPos.x;

		// 右マージン分（3桁）を考慮しつつnWidthを超えないようにする
		nWidth = ( nRightEdge + 3 < nWidth ) ? nRightEdge + 3 : nWidth;
	}

	return nWidth;
}

/*!	@brief 選択を考慮した行桁指定によるカーソル移動

	選択状態チェック→カーソル移動→選択領域更新という処理が
	あちこちのコマンドにあるのでまとめることにした．
	また，戻り値はほとんど使われていないのでvoidにした．

	選択状態を考慮してカーソルを移動する．
	非選択が指定された場合には既存選択範囲を解除して移動する．
	選択が指定された場合には選択範囲の開始・変更を併せて行う．
	インタラクティブ操作を前提とするため，必要に応じたスクロールを行う．
	カーソル移動後は上下移動でもカラム位置を保つよう，
	m_nCaretPosX_Prevの更新も併せて行う．

	@date 2006.07.09 genta 新規作成
*/
void CEditView::MoveCursorSelecting(
	CLayoutPoint	ptWk_CaretPos,		//!< [in] 移動先レイアウト位置
	bool			bSelect,			//!< true: 選択する  false: 選択解除
	int				nCaretMarginRate	//!< 縦スクロール開始位置を決める値
)
{
	if( bSelect ){
		if( !IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在のカーソル位置から選択を開始する */
			BeginSelectArea();
		}
	}else{
		if( IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( true );
		}
	}
	GetAdjustCursorPos(&ptWk_CaretPos);
	if( bSelect ){
		/*	現在のカーソル位置によって選択範囲を変更．
		
			2004.04.02 Moca 
			キャレット位置が不正だった場合にMoveCursorの移動結果が
			引数で与えた座標とは異なることがあるため，
			nPosX, nPosYの代わりに実際の移動結果を使うように．
		*/
		ChangeSelectAreaByCurrentCursor( ptWk_CaretPos );
	}
	MoveCursor( ptWk_CaretPos.x, ptWk_CaretPos.y, true, nCaretMarginRate );	// 2007.08.22 ryoji nCaretMarginRateが使われていなかった
	m_nCaretPosX_Prev = m_ptCaretPos.x;
}




/*!	@brief 行桁指定によるカーソル移動

	必要に応じて縦/横スクロールもする．
	垂直スクロールをした場合はその行数を返す（正／負）．
	
	@param nWk_CaretPosX	[in] 移動先桁位置(0〜)
	@param nWk_CaretPosY	[in] 移動先行位置(0〜)
	@param bScroll			[in] true: 画面位置調整有り/ false: 画面位置調整有り無し
	@param nCaretMarginRate	[in] 縦スクロール開始位置を決める値
	@return 縦スクロール行数(負:上スクロール/正:下スクロール)

	@note 不正な位置が指定された場合には適切な座標値に
		移動するため，引数で与えた座標と移動後の座標は
		必ずしも一致しない．
	
	@note bScrollがfalseの場合にはカーソル位置のみ移動する．
		trueの場合にはスクロール位置があわせて変更される

	@date 2001.10.20 deleted by novice AdjustScrollBar()を呼ぶ位置を変更
	@date 2004.04.02 Moca 行だけ有効な座標に修正するのを厳密に処理する
	@date 2004.09.11 genta bDrawスイッチは動作と名称が一致していないので
		再描画スイッチ→画面位置調整スイッチと名称変更
*/
int CEditView::MoveCursor( int nWk_CaretPosX, int nWk_CaretPosY, bool bScroll, int nCaretMarginRate )
{

	/* スクロール処理 */
	int		nScrollRowNum = 0;
	int		nScrollColNum = 0;
	RECT	rcScrol;
	RECT	rcClip;
	RECT	rcClip2;
//	int		nIndextY = 8;
	int		nCaretMarginY;
	HDC		hdc;
//	HPEN	hPen, hPenOld;
	int		nScrollMarginRight;
	int		nScrollMarginLeft;

	if( 0 >= m_nViewColNum ){
		return 0;
	}
	hdc = ::GetDC( m_hWnd );

	/* カーソル行アンダーラインのOFF */
//	if (IsTextSelected()) { //2002.02.27 Add By KK アンダーラインのちらつきを低減 - ここではテキスト選択時のみアンダーラインを消す。
		m_cUnderLine.CaretUnderLineOFF( bScroll );	//	YAZAKI
//	}	2002/04/04 YAZAKI 半ページスクロール時にアンダーラインが残ったままスクロールしてしまう問題に対処。

	if( m_bBeginSelect ){	/* 範囲選択中 */
		nCaretMarginY = 0;
	}else{
		//	2001/10/20 novice
		nCaretMarginY = m_nViewRowNum / nCaretMarginRate;
		if( 1 > nCaretMarginY ){
			nCaretMarginY = 1;
		}
	}
	// 2004.04.02 Moca 行だけ有効な座標に修正するのを厳密に処理する
	CLayoutPoint ptWk_CaretPos( nWk_CaretPosX, nWk_CaretPosY );
	GetAdjustCursorPos( &ptWk_CaretPos );
	
	
	/* 水平スクロール量（文字数）の算出 */
	nScrollColNum = 0;
	nScrollMarginRight = SCROLLMARGIN_RIGHT;
	nScrollMarginLeft = SCROLLMARGIN_LEFT;
	//	Aug. 14, 2005 genta 折り返し幅をLayoutMgrから取得するように
	if( m_pcEditDoc->m_cLayoutMgr.GetMaxLineKetas() > m_nViewColNum &&
		ptWk_CaretPos.x > m_nViewLeftCol + m_nViewColNum - nScrollMarginRight ){
		nScrollColNum =
			( m_nViewLeftCol + m_nViewColNum - nScrollMarginRight ) - ptWk_CaretPos.x;
	}else
	if( 0 < m_nViewLeftCol &&
		ptWk_CaretPos.x < m_nViewLeftCol + nScrollMarginLeft
	){
		nScrollColNum = m_nViewLeftCol + nScrollMarginLeft - ptWk_CaretPos.x;
		if( 0 > m_nViewLeftCol - nScrollColNum ){
			nScrollColNum = m_nViewLeftCol;
		}

	}

	m_nViewLeftCol -= nScrollColNum;

	//	From Here 2007.07.28 じゅうじ : 表示行数が3行以下の場合の動作改善
	/* 垂直スクロール量（行数）の算出 */
										// 画面が３行以下
	if( m_nViewRowNum <= 3 ){
							// 移動先は、画面のスクロールラインより上か？（up キー）
		if( ptWk_CaretPos.y - m_nViewTopLine < nCaretMarginY ){
			if( ptWk_CaretPos.y < nCaretMarginY ){	//１行目に移動
				nScrollRowNum = m_nViewTopLine;
			}else
			if( m_nViewRowNum <= 1 ){	// 画面が１行
				nScrollRowNum = m_nViewTopLine - ptWk_CaretPos.y;
			}else
#if !(0)	// COMMENTにすると、上下の空きを死守しない為、縦移動はgoodだが、横移動の場合上下にぶれる
			if( m_nViewRowNum <= 2 ){	// 画面が２行
				nScrollRowNum = m_nViewTopLine - ptWk_CaretPos.y;
			}else
#endif
			{						// 画面が３行
				nScrollRowNum = m_nViewTopLine - ptWk_CaretPos.y + 1;
			}
		}else
							// 移動先は、画面の最大行数−２より下か？（down キー）
		if( ptWk_CaretPos.y - m_nViewTopLine >= (m_nViewRowNum - nCaretMarginY - 2) ){
			int ii = m_pcEditDoc->m_cLayoutMgr.GetLineCount();
			if( ii - ptWk_CaretPos.y < nCaretMarginY + 1 &&
				ii - m_nViewTopLine < m_nViewRowNum ) {
			} else
			if( m_nViewRowNum <= 2 ){	// 画面が２行、１行
				nScrollRowNum = m_nViewTopLine - ptWk_CaretPos.y;
			}else{						// 画面が３行
				nScrollRowNum = m_nViewTopLine - ptWk_CaretPos.y + 1;
			}
		}
	}else							// 移動先は、画面のスクロールラインより上か？（up キー）
	if( ptWk_CaretPos.y - m_nViewTopLine < nCaretMarginY ){
		if( ptWk_CaretPos.y < nCaretMarginY ){	//１行目に移動
			nScrollRowNum = m_nViewTopLine;
		}else{
			nScrollRowNum = -(ptWk_CaretPos.y - m_nViewTopLine) + nCaretMarginY;
		}
	} else
							// 移動先は、画面の最大行数−２より下か？（down キー）
	if( ptWk_CaretPos.y - m_nViewTopLine >= m_nViewRowNum - nCaretMarginY - 2 ){
		int ii = m_pcEditDoc->m_cLayoutMgr.GetLineCount();
		if( ii - ptWk_CaretPos.y < nCaretMarginY + 1 &&
			ii - m_nViewTopLine < m_nViewRowNum ) {
		} else
		{
			nScrollRowNum =
				-(ptWk_CaretPos.y - m_nViewTopLine) + (m_nViewRowNum - nCaretMarginY - 2);
		}
	}
	//	To Here 2007.07.28 じゅうじ
	if( bScroll ){
		/* スクロール */
		if( abs( nScrollColNum ) >= m_nViewColNum ||
			abs( nScrollRowNum ) >= m_nViewRowNum ){
			m_nViewTopLine -= nScrollRowNum;
			::InvalidateRect( m_hWnd, NULL, TRUE );
		}else
		if( nScrollRowNum != 0 || nScrollColNum != 0 ){
			rcScrol.left = 0;
			rcScrol.right = m_nViewCx + m_nViewAlignLeft;
			rcScrol.top = m_nViewAlignTop;
			rcScrol.bottom = m_nViewCy + m_nViewAlignTop;
			if( nScrollRowNum > 0 ){
				rcScrol.bottom =
					m_nViewCy + m_nViewAlignTop -
					nScrollRowNum * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
				m_nViewTopLine -= nScrollRowNum;
				rcClip.left = 0;
				rcClip.right = m_nViewCx + m_nViewAlignLeft;
				rcClip.top = m_nViewAlignTop;
				rcClip.bottom =
					m_nViewAlignTop + nScrollRowNum * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
			}else
			if( nScrollRowNum < 0 ){
				rcScrol.top =
					m_nViewAlignTop - nScrollRowNum * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
				m_nViewTopLine -= nScrollRowNum;
				rcClip.left = 0;
				rcClip.right = m_nViewCx + m_nViewAlignLeft;
				rcClip.top =
					m_nViewCy + m_nViewAlignTop +
					nScrollRowNum * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
				rcClip.bottom = m_nViewCy + m_nViewAlignTop;
			}
			if( nScrollColNum > 0 ){
				rcScrol.left = m_nViewAlignLeft;
				rcScrol.right =
					m_nViewCx + m_nViewAlignLeft - nScrollColNum * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
				rcClip2.left = m_nViewAlignLeft;
				rcClip2.right = m_nViewAlignLeft + nScrollColNum * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
				rcClip2.top = m_nViewAlignTop;
				rcClip2.bottom = m_nViewCy + m_nViewAlignTop;
			}else
			if( nScrollColNum < 0 ){
				rcScrol.left = m_nViewAlignLeft - nScrollColNum * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
				rcClip2.left =
					m_nViewCx + m_nViewAlignLeft + nScrollColNum * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
				rcClip2.right = m_nViewCx + m_nViewAlignLeft;
				rcClip2.top = m_nViewAlignTop;
				rcClip2.bottom = m_nViewCy + m_nViewAlignTop;
			}
			if( m_bDrawSWITCH ){
				::ScrollWindowEx(
					m_hWnd,
					nScrollColNum * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace ),	/* 水平スクロール量 */
					nScrollRowNum * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight ),	/* 垂直スクロール量 */
					&rcScrol,	/* スクロール長方形の構造体のアドレス */
					NULL, NULL , NULL, SW_ERASE | SW_INVALIDATE
				);
				// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
				if( m_hbmpCompatBMP ){
					// 互換BMPもスクロール処理のためにBitBltで移動させる
					::BitBlt(
						m_hdcCompatDC,
						rcScrol.left + nScrollColNum * ( m_nCharWidth +  m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace ),
						rcScrol.top  + nScrollRowNum * ( m_nCharHeight + m_pcEditDoc->GetDocumentAttribute().m_nLineSpace ),
						rcScrol.right - rcScrol.left, rcScrol.bottom - rcScrol.top,
						m_hdcCompatDC, rcScrol.left, rcScrol.top, SRCCOPY
					);
				}
				// カーソルの縦線がテキストと行番号の隙間にあるとき、スクロール時に縦線領域を更新
				if( nScrollColNum != 0 && m_nOldCursorLineX == m_nViewAlignLeft - 1 ){
					RECT rcClip3;
					rcClip3.left = m_nOldCursorLineX;
					rcClip3.right = m_nOldCursorLineX + 1;
					rcClip3.top  = m_nViewAlignTop;
					rcClip3.bottom = m_nViewCy + m_nViewAlignTop;
					::InvalidateRect( m_hWnd, &rcClip3, TRUE );
				}
				// To Here 2007.09.09 Moca
				if( nScrollRowNum != 0 ){
					::InvalidateRect( m_hWnd, &rcClip, TRUE );
					if( nScrollColNum != 0 ){
						rcClip.left = 0;
						rcClip.right = m_nViewAlignLeft;
						rcClip.top = 0;
						rcClip.bottom = m_nViewCy + m_nViewAlignTop;
						::InvalidateRect( m_hWnd, &rcClip, TRUE );
					}
				}
				if( nScrollColNum != 0 ){
					::InvalidateRect( m_hWnd, &rcClip2, TRUE );
				}
			}
		}

		// 2009.08.28 nasukoji	「折り返さない」（スクロールバーをテキスト幅に合わせる）
		if( m_pcEditDoc->m_nTextWrapMethodCur == WRAP_NO_TEXT_WRAP ){
			// AdjustScrollBars()で移動後のキャレット位置が必要なため、ここでコピー
			if( IsTextSelected() || m_pShareData->m_Common.m_sGeneral.m_bIsFreeCursorMode ){
				m_ptCaretPos = ptWk_CaretPos;
			}
		}

		/* スクロールバーの状態を更新する */
		AdjustScrollBars(); // 2001/10/20 novice
	}

	/* キャレット移動 */
	m_ptCaretPos = ptWk_CaretPos;

	/* カーソル位置変換
	||  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
	||  →物理位置(行頭からのバイト数、折り返し無し行位置)
	*/
	m_pcEditDoc->m_cLayoutMgr.LayoutToLogic(
		m_ptCaretPos.x,
		m_ptCaretPos.y,
		&m_ptCaretPos_PHY.x,	/* カーソル位置 改行単位行先頭からのバイト数(０開始) */
		&m_ptCaretPos_PHY.y	/* カーソル位置 改行単位行の行番号(０開始) */
	);
	// 横スクロールが発生したら、ルーラー全体を再描画 2002.02.25 Add By KK
	if (nScrollColNum != 0 ){
		//次回DispRuler呼び出し時に再描画。（bDraw=falseのケースを考慮した。）
		m_bRedrawRuler = true;
	}

	/* カーソル行アンダーラインのON */
	//CaretUnderLineON( bDraw ); //2002.02.27 Del By KK アンダーラインのちらつきを低減
	if( bScroll ){
		/* キャレットの表示・更新 */
		ShowEditCaret();

		/* ルーラの再描画 */
		DispRuler( hdc );

		/* アンダーラインの再描画 */
		m_cUnderLine.CaretUnderLineON( true );

		/* キャレットの行桁位置を表示する */
		ShowCaretPosInfo();

		//	Sep. 11, 2004 genta 同期スクロールの関数化
		//	bScroll == FALSEの時にはスクロールしないので，実行しない
		SyncScrollV( -nScrollRowNum );	//	方向が逆なので符号反転が必要
		SyncScrollH( -nScrollColNum );	//	方向が逆なので符号反転が必要

	}
	::ReleaseDC( m_hWnd, hdc );


// 02/09/18 対括弧の強調表示 ai Start	03/02/18 ai mod S
	DrawBracketPair( false );
	SetBracketPairPos( true );
	DrawBracketPair( true );
// 02/09/18 対括弧の強調表示 ai End		03/02/18 ai mod E

	return nScrollRowNum;


}

/*! 正しいカーソル位置を算出する(EOF以降のみ)
	@param pptPosXY [in,out] カーソルのレイアウト座標
	@retval	TRUE 座標を修正した
	@retval	FALSE 座標は修正されなかった
	@note	EOFの直前が改行でない場合は、その行に限りEOF以降にも移動可能
			EOFだけの行は、先頭位置のみ正しい。
	@date 2004.04.02 Moca 関数化
*/
BOOL CEditView::GetAdjustCursorPos(
	CLayoutPoint* pptPosXY
)
{
	// 2004.03.28 Moca EOFのみのレイアウト行は、0桁目のみ有効.EOFより下の行のある場合は、EOF位置にする
	int nLayoutLineCount = m_pcEditDoc->m_cLayoutMgr.GetLineCount();

	CLayoutPoint ptPosXY2 = *pptPosXY;
	BOOL ret = FALSE;
	if( ptPosXY2.y >= nLayoutLineCount ){
		if( 0 < nLayoutLineCount ){
			ptPosXY2.y = nLayoutLineCount - 1;
			const CLayout* pcLayout = m_pcEditDoc->m_cLayoutMgr.SearchLineByLayoutY( ptPosXY2.y );
			if( pcLayout->m_cEol == EOL_NONE ){
				ptPosXY2.x = LineIndexToColumn( pcLayout, pcLayout->GetLength() );
				// EOFだけ折り返されているか
				//	Aug. 14, 2005 genta 折り返し幅をLayoutMgrから取得するように
				if( ptPosXY2.x >= m_pcEditDoc->m_cLayoutMgr.GetMaxLineKetas() ){
					ptPosXY2.y++;
					ptPosXY2.x = 0;
				}
			}else{
				// EOFだけの行
				ptPosXY2.y++;
				ptPosXY2.x = 0;
			}
		}else{
			// 空のファイル
			ptPosXY2.x = 0;
			ptPosXY2.y = 0;
		}
		if( pptPosXY->x != ptPosXY2.x || pptPosXY->y != ptPosXY2.y ){
			*pptPosXY = ptPosXY2;
			ret = TRUE;
		}
	}
	return ret;
}


/*!
	行桁指定によるカーソル移動（座標調整付き）

	@return 縦スクロール行数(負:上スクロール/正:下スクロール)

	@note マウス等による移動で不適切な位置に行かないよう座標調整してカーソル移動する

	@date 2007.08.23 ryoji 関数化（MoveCursorToPoint()から処理を抜き出し）
	@date 2007.09.26 ryoji 半角文字でも中央で左右にカーソルを振り分ける
*/
int CEditView::MoveCursorProperly(
	CLayoutPoint	ptNewXY,			//!< [in] カーソルのレイアウト座標X
	bool			bScroll,			//!< [in] true: 画面位置調整有り/ false: 画面位置調整有り無し
	int				nCaretMarginRate,	//!< [in] 縦スクロール開始位置を決める値
	int				dx					//!< [in] ptNewXY.xとマウスカーソル位置との誤差(カラム幅未満のドット数)
)
{
	int				nLineLen;
	const CLayout*	pcLayout;

	if( 0 > ptNewXY.y ){
		ptNewXY.y = 0;
	}

	/* カーソルがテキスト最下端行にあるか */
	if( ptNewXY.y >= m_pcEditDoc->m_cLayoutMgr.GetLineCount() ){
		// 2004.04.03 Moca EOFより後ろの座標調整は、MoveCursor内でやってもらうので、削除
	}else
	/* カーソルがテキスト最上端行にあるか */
	if( ptNewXY.y < 0 ){
		ptNewXY.x = 0;
		ptNewXY.y = 0;
	}else{
		/* 移動先の行のデータを取得 */
		m_pcEditDoc->m_cLayoutMgr.GetLineStr( ptNewXY.y, &nLineLen, &pcLayout );

		int nColWidth = m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace;
		int nPosX = 0;
		int i = 0;
		CMemoryIterator<CLayout> it( pcLayout, m_pcEditDoc->m_cLayoutMgr.GetTabSpace() );
		while( !it.end() ){
			it.scanNext();
			if ( it.getIndex() + it.getIndexDelta() > pcLayout->GetLengthWithoutEOL() ){
				i = nLineLen;
				break;
			}
			if( it.getColumn() + it.getColumnDelta() > ptNewXY.x ){
				if (ptNewXY.x >= (pcLayout ? pcLayout->GetIndent() : 0) && ((ptNewXY.x - it.getColumn()) * nColWidth + dx) * 2 >= it.getColumnDelta() * nColWidth){
					nPosX += it.getColumnDelta();
				}
				i = it.getIndex();
				break;
			}
			it.addDelta();
		}
		nPosX += it.getColumn();
		if ( it.end() ){
			i = it.getIndex();
			nPosX -= it.getColumnDelta();
		}

		if( i >= nLineLen ){
// From 2001.12.21 hor フリーカーソルOFFでEOFのある行の直前がマウスで選択できないバグの修正
			if( ptNewXY.y +1 == m_pcEditDoc->m_cLayoutMgr.GetLineCount() &&
				EOL_NONE == pcLayout->m_cEol.GetLen() ){
				nPosX = LineIndexToColumn( pcLayout, nLineLen );
			}else
// To 2001.12.21 hor
			/* フリーカーソルモードか */
			if( m_pShareData->m_Common.m_sGeneral.m_bIsFreeCursorMode
			  || ( m_bBeginSelect && m_bBeginBoxSelect )	/* マウス範囲選択中 && 矩形範囲選択中 */
//			  || m_bDragMode /* OLE DropTarget */
			  || ( m_bDragMode && m_bDragBoxData ) /* OLE DropTarget && 矩形データ */
			){
					nPosX = ptNewXY.x;
					//	Aug. 14, 2005 genta 折り返し幅をLayoutMgrから取得するように
					if( nPosX < 0 ){
						nPosX = 0;
					}else
					if( nPosX > m_pcEditDoc->m_cLayoutMgr.GetMaxLineKetas() ){	/* 折り返し文字数 */
						nPosX = m_pcEditDoc->m_cLayoutMgr.GetMaxLineKetas();
					}
//				}
			}
		}
		ptNewXY.x = nPosX;
	}

	return MoveCursor( ptNewXY.x, ptNewXY.y, bScroll, nCaretMarginRate );
}



/* マウス等による座標指定によるカーソル移動
|| 必要に応じて縦/横スクロールもする
|| 垂直スクロールをした場合はその行数を返す(正／負)
*/
//2007.09.11 kobake 関数名変更: MoveCursorToPoint→MoveCursorToClientPoint
int CEditView::MoveCursorToPoint( int xPos, int yPos )
{
	int				nScrollRowNum;
	CLayoutPoint	ptLayoutPos;
	int				dx;

	ptLayoutPos.x = m_nViewLeftCol + (xPos - m_nViewAlignLeft) / ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
	ptLayoutPos.y = m_nViewTopLine + (yPos - m_nViewAlignTop) / ( m_nCharHeight + m_pcEditDoc->GetDocumentAttribute().m_nLineSpace );
	dx = (xPos - m_nViewAlignLeft) % ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );

	nScrollRowNum = MoveCursorProperly( ptLayoutPos, true, 1000, dx );
	m_nCaretPosX_Prev = m_ptCaretPos.x;
	return nScrollRowNum;
}
//_CARETMARGINRATE_CARETMARGINRATE_CARETMARGINRATE



/*!
	指定カーソル位置にURLが有る場合のその範囲を調べる

	2007.01.18 kobake URL文字列の受け取りをwstringで行うように変更
	2009.05.27 ryoji URL色指定の正規表現キーワードにマッチする文字列もURLとみなす
	                 URLの強調表示OFFのチェックはこの関数内で行うように変更
 */
bool CEditView::IsCurrentPositionURL(
	const CLayoutPoint&	ptCaretPos,		//!< [in]  カーソル位置
	CLogicRange*		pUrlRange,		//!< [out] URL範囲。ロジック単位。
	std::tstring*		ptstrURL		//!< [out] URL文字列受け取り先。NULLを指定した場合はURL文字列を受け取らない。
)
{
	MY_RUNNINGTIMER( cRunningTimer, "CEditView::IsCurrentPositionURL" );

	int			nCharChars;

	// URLを強調表示するかどうかチェックする	// 2009.05.27 ryoji
	bool bDispUrl = m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_URL].m_bDisp;
	BOOL bUseRegexKeyword = FALSE;
	STypeConfig	*TypeDataPtr = &(m_pcEditDoc->GetDocumentAttribute());
	if( TypeDataPtr->m_bUseRegexKeyword ){
		for( int i = 0; i < MAX_REGEX_KEYWORD; i++ ){
			if( TypeDataPtr->m_RegexKeywordArr[i].m_szKeyword[0] == L'\0' )
				break;
			if( TypeDataPtr->m_RegexKeywordArr[i].m_nColorIndex == COLORIDX_URL ){
				bUseRegexKeyword = TRUE;	// URL色指定の正規表現キーワードがある
				break;
			}
		}
	}
	if( !bDispUrl && !bUseRegexKeyword ){
		return false;	// URL強調表示しないのでURLではない
	}

	// 正規表現キーワード（URL色指定）行検索開始処理	// 2009.05.27 ryoji
	if( bUseRegexKeyword ){
		m_cRegexKeyword->RegexKeyLineStart();
	}

	/*
	  カーソル位置変換
	  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
	  →
	  物理位置(行頭からのバイト数、折り返し無し行位置)
	*/
	CLogicPoint ptXY;
	m_pcEditDoc->m_cLayoutMgr.LayoutToLogic(
		ptCaretPos.x,
		ptCaretPos.y,
		(int*)&ptXY.x,
		(int*)&ptXY.y
	);
	int			nLineLen;
	const char*	pLine = m_pcEditDoc->m_cDocLineMgr.GetLineStr( ptXY.y, &nLineLen );

	bool		bMatch;
	int			nMatchColor;
	int			nUrlLen = 0;
	int			i = t_max(0, ptXY.x - _MAX_PATH);	// 2009.05.22 ryoji 200->_MAX_PATH（※長い行は精度低下しても性能優先で行頭以外から開始）
	//nLineLen = t_min(nLineLen, nX + _MAX_PATH);
	bool bKeyWordTop = (i == 0);
	while( i <= ptXY.x && i < nLineLen ){
		bMatch = ( bUseRegexKeyword
					&& m_cRegexKeyword->RegexIsKeyword( pLine, i, nLineLen, &nUrlLen, &nMatchColor )
					&& nMatchColor == COLORIDX_URL );
		if( !bMatch ){
			bMatch = ( bDispUrl && bKeyWordTop
						&& IsURL(&pLine[i], (int)(nLineLen - i), &nUrlLen) );	/* 指定アドレスがURLの先頭ならばTRUEとその長さを返す */
		}
		if( bMatch ){
			if( i <= ptXY.x && ptXY.x < i + nUrlLen ){
				/* URLを返す場合 */
				if( ptstrURL ){
					ptstrURL->assign(&pLine[i],nUrlLen);
				}
				pUrlRange->m_ptFrom.y = ptXY.y; pUrlRange->m_ptTo.y = ptXY.y;
				pUrlRange->m_ptFrom.x = i; pUrlRange->m_ptTo.x = i + nUrlLen;
				return true;
			}else{
				i += nUrlLen;
				continue;
			}
		}
		nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, i );
		bKeyWordTop = (nCharChars == 2 || !IS_KEYWORD_CHAR(pLine[i]));
		i += nCharChars;
	}
	return false;
}

VOID CEditView::OnTimer(
	HWND hwnd,		// handle of window for timer messages
	UINT uMsg,		// WM_TIMER message
	UINT_PTR idEvent,	// timer identifier
	DWORD dwTime 	// current system time
	)
{
	POINT		po;
	RECT		rc;

	if( FALSE != m_pShareData->m_Common.m_sEdit.m_bUseOLE_DragDrop ){	/* OLEによるドラッグ & ドロップを使う */
		if( IsDragSource() ){
			return;
		}
	}
	/* 範囲選択中でない場合 */
	if(!m_bBeginSelect){
		if( FALSE != KeyWordHelpSearchDict( LID_SKH_ONTIMER, &po, &rc ) ){	// 2006.04.10 fon
			/* 辞書Tipを表示 */
			m_cTipWnd.Show( po.x, po.y + m_nCharHeight, NULL );
		}
	}else{
		::GetCursorPos( &po );
		::GetWindowRect(m_hWnd, &rc );
		if( !PtInRect( &rc, po ) ){
			OnMOUSEMOVE( 0, m_nMouseRollPosXOld, m_nMouseRollPosYOld );
			return;
		}

		// 1999.12.18 クライアント領域内ではタイマー自動ドラッグ+ロールしない
		return;

// 2001.12.21 hor 以下、実行されないのでコメントアウトします (行頭////はもともとコメント行です)
////		rc.top += m_nViewAlignTop;
//		RECT rc2;
//		rc2 = rc;
//		rc2.bottom = rc.top + m_nViewAlignTop + ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
//		if( PtInRect( &rc2, po )
//		 && 0 < m_nViewTopLine
//		){
//			OnMOUSEMOVE( 0, m_nMouseRollPosXOld, m_nMouseRollPosYOld );
//			return;
//		}
//		rc2 = rc;
//		rc2.top = rc.bottom - ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
//		if( PtInRect( &rc2, po )
//			&& m_pcEditDoc->m_cLayoutMgr.GetLineCount() > m_nViewTopLine + m_nViewRowNum
//		){
//			OnMOUSEMOVE( 0, m_nMouseRollPosXOld, m_nMouseRollPosYOld );
//			return;
//		}
//
////		rc.top += 48;
////		rc.bottom -= 48;
////		if( !PtInRect( &rc, po ) ){
////			OnMOUSEMOVE( 0, m_nMouseRollPosXOld, m_nMouseRollPosYOld );
////		}
	}
	return;
}

/*! キーワード辞書検索の前提条件チェックと、検索

	@date 2006.04.10 fon OnTimer, CreatePopUpMenu_Rから分離
*/
BOOL CEditView::KeyWordHelpSearchDict( LID_SKH nID, POINT* po, RECT* rc )
{
	CMemory		cmemCurText;
	int			i;

	/* キーワードヘルプを使用するか？ */
	if( !m_pcEditDoc->GetDocumentAttribute().m_bUseKeyWordHelp )	/* キーワードヘルプ機能を使用する */	// 2006.04.10 fon
		goto end_of_search;
	/* フォーカスがあるか？ */
	if( !(m_nCaretWidth > 0) ) 
		goto end_of_search;
	/* ウィンドウ内にマウスカーソルがあるか？ */
	GetCursorPos( po );
	GetWindowRect( m_hWnd, rc );
	if( !PtInRect( rc, *po ) )
		goto end_of_search;
	switch(nID){
	case LID_SKH_ONTIMER:
		/* 右コメントの１〜３でない場合 */
		if(!( m_bInMenuLoop == FALSE	&&			/* １．メニュー モーダル ループに入っていない */
			0 != m_dwTipTimer			&&			/* ２．辞書Tipを表示していない */
			300 < ::GetTickCount() - m_dwTipTimer	/* ３．一定時間以上、マウスが固定されている */
		) )	goto end_of_search;
		break;
	case LID_SKH_POPUPMENU_R:
		if(!( m_bInMenuLoop == FALSE	//&&			/* １．メニュー モーダル ループに入っていない */
		//	0 != m_dwTipTimer			&&			/* ２．辞書Tipを表示していない */
		//	1000 < ::GetTickCount() - m_dwTipTimer	/* ３．一定時間以上、マウスが固定されている */
		) )	goto end_of_search;
		break;
	default:
		PleaseReportToAuthor( NULL, _T("CEditView::KeyWordHelpSearchDict\nnID=%d"), (int)nID  );
	}
	/* 選択範囲のデータを取得(複数行選択の場合は先頭の行のみ) */
	if( GetSelectedData( &cmemCurText, TRUE, NULL, FALSE, m_pShareData->m_Common.m_sEdit.m_bAddCRLFWhenCopy ) ){
		char* pszWork = cmemCurText.GetStringPtr();
		int nWorkLength	= lstrlen( pszWork );
		for( i = 0; i < nWorkLength; ++i ){
			if( pszWork[i] == '\0' ||
				pszWork[i] == CR ||
				pszWork[i] == LF ){
				break;
			}
		}
		cmemCurText._SetStringLength( i );
	}
	/* キャレット位置の単語を取得する処理 */	// 2006.03.24 fon
	else if(m_pShareData->m_Common.m_sSearch.m_bUseCaretKeyWord){
		if(!GetCurrentWord(&cmemCurText))
			goto end_of_search;
	}
	else
		goto end_of_search;

	if( CMemory::IsEqual( cmemCurText, m_cTipWnd.m_cKey ) &&	/* 既に検索済みか */
		(!m_cTipWnd.m_KeyWasHit) )								/* 該当するキーがなかった */
		goto end_of_search;
	m_cTipWnd.m_cKey = cmemCurText;

	/* 検索実行 */
	if( FALSE == KeySearchCore(&m_cTipWnd.m_cKey) )
		goto end_of_search;
	m_dwTipTimer = 0;		/* 辞書Tipを表示している */
	m_poTipCurPos = *po;	/* 現在のマウスカーソル位置 */
	return TRUE;			/* ここまで来ていればヒット・ワード */

	/* キーワードヘルプ表示処理終了 */
	end_of_search:
	return FALSE;
}

/*! キーワード辞書検索処理メイン

	@date 2006.04.10 fon KeyWordHelpSearchDictから分離
*/
BOOL CEditView::KeySearchCore( const CMemory* pcmemCurText )
{
	CMemory*	pcmemRefKey;
	CMemory*	pcmemRefText;
	LPSTR		pszWork;
	int			nCmpLen = STRNCMP_MAX; // 2006.04.10 fon
	int			nLine; // 2006.04.10 fon


	int nTypeNo = m_pcEditDoc->GetDocumentType();
	m_cTipWnd.m_cInfo.SetString( "" );	/* tooltipバッファ初期化 */
	/* 1行目にキーワード表示の場合 */
	if(m_pcEditDoc->GetDocumentAttribute().m_bUseKeyHelpKeyDisp){	/* キーワードも表示する */	// 2006.04.10 fon
		m_cTipWnd.m_cInfo.AppendString( "[ " );
		m_cTipWnd.m_cInfo.AppendString( pcmemCurText->GetStringPtr() );
		m_cTipWnd.m_cInfo.AppendString( " ]" );
	}
	/* 途中まで一致を使う場合 */
	if(m_pcEditDoc->GetDocumentAttribute().m_bUseKeyHelpPrefix)
		nCmpLen = lstrlen( pcmemCurText->GetStringPtr() );	// 2006.04.10 fon
	m_cTipWnd.m_KeyWasHit = FALSE;
	for(int i=0;i<m_pShareData->m_Types[nTypeNo].m_nKeyHelpNum;i++){	//最大数：MAX_KEYHELP_FILE
		if( 1 == m_pShareData->m_Types[nTypeNo].m_KeyHelpArr[i].m_nUse ){
			if(m_cDicMgr.Search( pcmemCurText->GetStringPtr(), nCmpLen, &pcmemRefKey, &pcmemRefText, m_pShareData->m_Types[nTypeNo].m_KeyHelpArr[i].m_szPath, &nLine )){	// 2006.04.10 fon (nCmpLen,pcmemRefKey,nSearchLine)引数を追加
				/* 該当するキーがある */
				pszWork = pcmemRefText->GetStringPtr();
				/* 有効になっている辞書を全部なめて、ヒットの都度説明の継ぎ増し */
				if(m_pcEditDoc->GetDocumentAttribute().m_bUseKeyHelpAllSearch){	/* ヒットした次の辞書も検索 */	// 2006.04.10 fon
					/* バッファに前のデータが詰まっていたらseparator挿入 */
					if(m_cTipWnd.m_cInfo.GetStringLength() != 0)
						m_cTipWnd.m_cInfo.AppendString( "\n--------------------\n■" );
					else
						m_cTipWnd.m_cInfo.AppendString( "■" );	/* 先頭の場合 */
					/* 辞書のパス挿入 */
					m_cTipWnd.m_cInfo.AppendString( m_pShareData->m_Types[nTypeNo].m_KeyHelpArr[i].m_szPath );
					m_cTipWnd.m_cInfo.AppendString( "\n" );
					/* 前方一致でヒットした単語を挿入 */
					if(m_pcEditDoc->GetDocumentAttribute().m_bUseKeyHelpPrefix){	/* 選択範囲で前方一致検索 */
						m_cTipWnd.m_cInfo.AppendString( pcmemRefKey->GetStringPtr() );
						m_cTipWnd.m_cInfo.AppendString( " >>\n" );
					}/* 調査した「意味」を挿入 */
					m_cTipWnd.m_cInfo.AppendString( pszWork );
					delete pcmemRefText;
					delete pcmemRefKey;	// 2006.07.02 genta
					/* タグジャンプ用の情報を残す */
					if(FALSE == m_cTipWnd.m_KeyWasHit){
						m_cTipWnd.m_nSearchDict=i;	/* 辞書を開くとき最初にヒットした辞書を開く */
						m_cTipWnd.m_nSearchLine=nLine;
						m_cTipWnd.m_KeyWasHit = TRUE;
					}
				}else{	/* 最初のヒット項目のみ返す場合 */
					/* キーワードが入っていたらseparator挿入 */
					if(m_cTipWnd.m_cInfo.GetStringLength() != 0)
						m_cTipWnd.m_cInfo.AppendString( "\n--------------------\n" );
					/* 前方一致でヒットした単語を挿入 */
					if(m_pcEditDoc->GetDocumentAttribute().m_bUseKeyHelpPrefix){	/* 選択範囲で前方一致検索 */
						m_cTipWnd.m_cInfo.AppendString( pcmemRefKey->GetStringPtr() );
						m_cTipWnd.m_cInfo.AppendString( " >>\n" );
					}/* 調査した「意味」を挿入 */
					m_cTipWnd.m_cInfo.AppendString( pszWork );
					delete pcmemRefText;
					delete pcmemRefKey;	// 2006.07.02 genta
					/* タグジャンプ用の情報を残す */
					m_cTipWnd.m_nSearchDict=i;
					m_cTipWnd.m_nSearchLine=nLine;
					m_cTipWnd.m_KeyWasHit = TRUE;
					return TRUE;
				}
			}
		}
	}
	if( m_cTipWnd.m_KeyWasHit != FALSE ){
			return TRUE;
	}
	/* 該当するキーがなかった場合 */
	return FALSE;
}


/* 現在のカーソル位置から選択を開始する */
void CEditView::BeginSelectArea( void )
{
	m_sSelectBgn.m_ptFrom.y = m_ptCaretPos.y;/* 範囲選択開始行(原点) */
	m_sSelectBgn.m_ptFrom.x = m_ptCaretPos.x;/* 範囲選択開始桁(原点) */
	m_sSelectBgn.m_ptTo.y = m_ptCaretPos.y;	/* 範囲選択開始行(原点) */
	m_sSelectBgn.m_ptTo.x = m_ptCaretPos.x;	/* 範囲選択開始桁(原点) */

	m_sSelect.m_ptFrom.y = m_ptCaretPos.y;	/* 範囲選択開始行 */
	m_sSelect.m_ptFrom.x = m_ptCaretPos.x;	/* 範囲選択開始桁 */
	m_sSelect.m_ptTo.y = m_ptCaretPos.y;		/* 範囲選択終了行 */
	m_sSelect.m_ptTo.x = m_ptCaretPos.x;		/* 範囲選択終了桁 */
	return;
}





/* 現在の選択範囲を非選択状態に戻す */
void CEditView::DisableSelectArea( bool bDraw )
{
	m_sSelectOld.m_ptFrom.y = m_sSelect.m_ptFrom.y;	/* 範囲選択開始行 */
	m_sSelectOld.m_ptFrom.x = m_sSelect.m_ptFrom.x;	/* 範囲選択開始桁 */
	m_sSelectOld.m_ptTo.y = m_sSelect.m_ptTo.y;		/* 範囲選択終了行 */
	m_sSelectOld.m_ptTo.x = m_sSelect.m_ptTo.x;		/* 範囲選択終了桁 */
//	m_sSelect.m_ptFrom.y = 0;
//	m_sSelect.m_ptFrom.x = 0;
//	m_sSelect.m_ptTo.y = 0;
//	m_sSelect.m_ptTo.x = 0;

	m_sSelect.m_ptFrom.y	= -1;
	m_sSelect.m_ptFrom.x	= -1;
	m_sSelect.m_ptTo.y		= -1;
	m_sSelect.m_ptTo.x		= -1;

	if( bDraw ){
		DrawSelectArea();
		m_bDrawSelectArea = false;	// 02/12/13 ai
	}
	m_bSelectingLock	 = false;	/* 選択状態のロック */
	m_sSelectOld.m_ptFrom.y = 0;		/* 範囲選択開始行 */
	m_sSelectOld.m_ptFrom.x = 0; 		/* 範囲選択開始桁 */
	m_sSelectOld.m_ptTo.y = 0;			/* 範囲選択終了行 */
	m_sSelectOld.m_ptTo.x = 0;			/* 範囲選択終了桁 */
	m_bBeginBoxSelect = false;		/* 矩形範囲選択中 */
	m_bBeginLineSelect = false;		/* 行単位選択中 */
	m_bBeginWordSelect = false;		/* 単語単位選択中 */

	// 2002.02.16 hor 直前のカーソル位置をリセット
	m_nCaretPosX_Prev=m_ptCaretPos.x;

	//	From Here Dec. 6, 2000 genta
	//	To Here Dec. 6, 2000 genta

	/* カーソル行アンダーラインのON */
	m_cUnderLine.CaretUnderLineON( bDraw );
	return;
}





/* 現在のカーソル位置によって選択範囲を変更 */
void CEditView::ChangeSelectAreaByCurrentCursor( const CLayoutPoint& ptCaretPos )
{
	m_sSelectOld = m_sSelect; // 範囲選択(Old)

	//	2002/04/08 YAZAKI コードの重複を排除
	ChangeSelectAreaByCurrentCursorTEST(
		ptCaretPos,
		&m_sSelect
	);

	// 選択領域の描画
	DrawSelectArea();
}

/* 現在のカーソル位置によって選択範囲を変更(テストのみ) */
void CEditView::ChangeSelectAreaByCurrentCursorTEST(
	const CLayoutPoint& ptCaretPos,
	CLayoutRange* pSelect
)
{
	if( m_sSelectBgn.m_ptFrom.y == m_sSelectBgn.m_ptTo.y /* 範囲選択開始行(原点) */
	 && m_sSelectBgn.m_ptFrom.x == m_sSelectBgn.m_ptTo.x ){
		if( ptCaretPos.y == m_sSelectBgn.m_ptFrom.y
		 && ptCaretPos.x == m_sSelectBgn.m_ptFrom.x ){
			// 選択解除
			pSelect->m_ptFrom.y = -1;
			pSelect->m_ptFrom.x  = -1;
			pSelect->m_ptTo.y = -1;
			pSelect->m_ptTo.x = -1;
		}else
		if( ptCaretPos.y < m_sSelectBgn.m_ptFrom.y
		 || ( ptCaretPos.y == m_sSelectBgn.m_ptFrom.y && ptCaretPos.x < m_sSelectBgn.m_ptFrom.x ) ){
			pSelect->m_ptFrom = ptCaretPos;
			pSelect->m_ptTo = m_sSelectBgn.m_ptFrom;
		}else{
			pSelect->m_ptFrom = m_sSelectBgn.m_ptFrom;
			pSelect->m_ptTo = ptCaretPos;
		}
	}else{
		// 常時選択範囲の範囲内
		if( ( ptCaretPos.y > m_sSelectBgn.m_ptFrom.y || ( ptCaretPos.y == m_sSelectBgn.m_ptFrom.y && ptCaretPos.x >= m_sSelectBgn.m_ptFrom.x ) )
		 && ( ptCaretPos.y < m_sSelectBgn.m_ptTo.y || ( ptCaretPos.y == m_sSelectBgn.m_ptTo.y && ptCaretPos.x < m_sSelectBgn.m_ptTo.x ) )
		){
			pSelect->m_ptFrom = m_sSelectBgn.m_ptFrom;
			if ( ptCaretPos.y == m_sSelectBgn.m_ptFrom.y && ptCaretPos.x == m_sSelectBgn.m_ptFrom.x ){
				pSelect->m_ptTo = m_sSelectBgn.m_ptTo;
			}
			else {
				pSelect->m_ptTo = ptCaretPos;
			}
		}else
		if( !( ptCaretPos.y > m_sSelectBgn.m_ptFrom.y || ( ptCaretPos.y == m_sSelectBgn.m_ptFrom.y && ptCaretPos.x >= m_sSelectBgn.m_ptFrom.x ) ) ){
			// 常時選択範囲の前方向
			pSelect->m_ptFrom = ptCaretPos;
			pSelect->m_ptTo = m_sSelectBgn.m_ptTo;
		}else{
			// 常時選択範囲の後ろ方向
			pSelect->m_ptFrom = m_sSelectBgn.m_ptFrom;
			pSelect->m_ptTo = ptCaretPos;
		}
	}
}

/* カーソル上下移動処理 */
int CEditView::Cursor_UPDOWN( int nMoveLines, int bSelect )
{
	const char*		pLine;
	int				nLineLen;
	CLayoutPoint 	nPos = CLayoutPoint( 0, m_ptCaretPos.y );
	int				i;
	int				nLineCols;
	int				nScrollLines;
	const CLayout*	pcLayout;
	nScrollLines = 0;
	if( nMoveLines > 0 ){
		/* カーソルがテキスト最下端行にあるか */
		if( m_ptCaretPos.y + nMoveLines >= m_pcEditDoc->m_cLayoutMgr.GetLineCount() ){
			nMoveLines = m_pcEditDoc->m_cLayoutMgr.GetLineCount() - m_ptCaretPos.y  - 1;
		}
		if( nMoveLines <= 0 ){
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( m_ptCaretPos.y, &nLineLen, &pcLayout );
			if( NULL != pLine ){
				nLineCols = LineIndexToColumn( pcLayout, nLineLen );
				/* 改行で終わっているか */
				//	Aug. 14, 2005 genta 折り返し幅をLayoutMgrから取得するように
				if( ( EOL_NONE != pcLayout->m_cEol )
//				if( ( pLine[ nLineLen - 1 ] == '\n' || pLine[ nLineLen - 1 ] == '\r' )
				 || nLineCols >= m_pcEditDoc->m_cLayoutMgr.GetMaxLineKetas()
				){
					if( bSelect ){
						if( !IsTextSelected() ){	/* テキストが選択されているか */
							/* 現在のカーソル位置から選択を開始する */
							BeginSelectArea();
						}
					}else{
						if( IsTextSelected() ){	/* テキストが選択されているか */
							/* 現在の選択範囲を非選択状態に戻す */
							DisableSelectArea( true );
						}
					}
					nPos.x = 0;
					++nPos.y;
					nScrollLines = MoveCursor( nPos.x, nPos.y, m_bDrawSWITCH /* true */ ); // YAZAKI.
					if( bSelect ){
						/* 現在のカーソル位置によって選択範囲を変更 */
						ChangeSelectAreaByCurrentCursor( nPos );
					}
				}
			}
			//	Sep. 11, 2004 genta 同期スクロールの関数化
			//	MoveCursorでスクロール位置調整済み
			//SyncScrollV( nScrollLines );
			return nScrollLines;
		}
	}else{
		/* カーソルがテキスト最上端行にあるか */
		if( m_ptCaretPos.y + nMoveLines < 0 ){
			nMoveLines = - m_ptCaretPos.y;
		}
		if( nMoveLines >= 0 ){
			//	Sep. 11, 2004 genta 同期スクロールの関数化
			SyncScrollV( nScrollLines );
			return nScrollLines;
		}
	}
	if( bSelect ){
		if( !IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在のカーソル位置から選択を開始する */
			BeginSelectArea();
		}
	}else{
		if( IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( true );
		}
	}
	/* 次の行のデータを取得 */
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( m_ptCaretPos.y + nMoveLines, &nLineLen, &pcLayout );
	CMemoryIterator<CLayout> it( pcLayout, m_pcEditDoc->m_cLayoutMgr.GetTabSpace() );
	while( !it.end() ){
		it.scanNext();
		if ( it.getIndex() + it.getIndexDelta() > pcLayout->GetLengthWithoutEOL() ){
			i = nLineLen;
			break;
		}
		if( it.getColumn() + it.getColumnDelta() > m_nCaretPosX_Prev ){
			i = it.getIndex();
			break;
		}
		it.addDelta();
	}
	nPos.x += it.getColumn();
	if ( it.end() ){
		i = it.getIndex();
	}

	if( i >= nLineLen ){
		/* フリーカーソルモードか */
		if( m_pShareData->m_Common.m_sGeneral.m_bIsFreeCursorMode
		 || IsTextSelected() && m_bBeginBoxSelect	/* 矩形範囲選択中 */
		){
			if( m_ptCaretPos.y + nMoveLines + 1 == m_pcEditDoc->m_cLayoutMgr.GetLineCount()  ){
				if( NULL != pLine ){
					if( pLine[nLineLen - 1] == CR || pLine[nLineLen - 1] == LF ){
						nPos.x = m_nCaretPosX_Prev;
					}
				}
			}else{
				nPos.x = m_nCaretPosX_Prev;
			}
		}
	}
	nScrollLines = MoveCursor( nPos.x, m_ptCaretPos.y + nMoveLines, m_bDrawSWITCH /* true */ ); // YAZAKI.
	if( bSelect ){
		CLayoutPoint ptCaretPos = CLayoutPoint( nPos.x, m_ptCaretPos.y );
		ChangeSelectAreaByCurrentCursor( ptCaretPos );
	}

	return nScrollLines;
}





/*! 指定上端行位置へスクロール

	@param nPos [in] スクロール位置
	@retval 実際にスクロールした行数 (正:下方向/負:上方向)

	@date 2004.09.11 genta 行数を戻り値として返すように．(同期スクロール用)
*/
int CEditView::ScrollAtV( int nPos )
{
	int			nScrollRowNum;
	RECT		rcScrol;
	RECT		rcClip;
	if( nPos < 0 ){
		nPos = 0;
	}else
	if( (m_pcEditDoc->m_cLayoutMgr.GetLineCount() + 2 )- m_nViewRowNum < nPos ){
		nPos = ( m_pcEditDoc->m_cLayoutMgr.GetLineCount() + 2 ) - m_nViewRowNum;
		if( nPos < 0 ){
			nPos = 0;
		}
	}
	if( m_nViewTopLine == nPos ){
		return 0;	//	スクロール無し。
	}
	/* 垂直スクロール量（行数）の算出 */
	nScrollRowNum = m_nViewTopLine - nPos;

	/* スクロール */
	if( abs( nScrollRowNum ) >= m_nViewRowNum ){
		m_nViewTopLine = nPos;
		::InvalidateRect( m_hWnd, NULL, TRUE );
	}else{
		rcScrol.left = 0;
		rcScrol.right = m_nViewCx + m_nViewAlignLeft;
		rcScrol.top = m_nViewAlignTop;
		rcScrol.bottom = m_nViewCy + m_nViewAlignTop;
		if( nScrollRowNum > 0 ){
			rcScrol.bottom =
				m_nViewCy + m_nViewAlignTop -
				nScrollRowNum * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
			m_nViewTopLine = nPos;
			rcClip.left = 0;
			rcClip.right = m_nViewCx + m_nViewAlignLeft;
			rcClip.top = m_nViewAlignTop;
			rcClip.bottom =
				m_nViewAlignTop + nScrollRowNum * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
		}else
		if( nScrollRowNum < 0 ){
			rcScrol.top =
				m_nViewAlignTop - nScrollRowNum * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
			m_nViewTopLine = nPos;
			rcClip.left = 0;
			rcClip.right = m_nViewCx + m_nViewAlignLeft;
			rcClip.top =
				m_nViewCy + m_nViewAlignTop +
				nScrollRowNum * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
			rcClip.bottom = m_nViewCy + m_nViewAlignTop;
		}
		if( m_bDrawSWITCH ){
			::ScrollWindowEx(
				m_hWnd,
				0,	/* 水平スクロール量 */
				nScrollRowNum * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight ),		/* 垂直スクロール量 */
				&rcScrol,	/* スクロール長方形の構造体のアドレス */
				NULL, NULL , NULL, SW_ERASE | SW_INVALIDATE
			);
			// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
			// 互換BMPのスクロール
			if( m_hbmpCompatBMP ){
				::BitBlt(
					m_hdcCompatDC, rcScrol.left,
					rcScrol.top + nScrollRowNum * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight ),
					rcScrol.right - rcScrol.left, rcScrol.bottom - rcScrol.top,
					m_hdcCompatDC, rcScrol.left, rcScrol.top, SRCCOPY
				);
			}
			// To Here 2007.09.09 Moca
			::InvalidateRect( m_hWnd, &rcClip, TRUE );
			::UpdateWindow( m_hWnd );
		}
	}

	/* スクロールバーの状態を更新する */
	AdjustScrollBars();

	/* キャレットの表示・更新 */
	ShowEditCaret();

	return -nScrollRowNum;	//方向が逆なので符号反転が必要
}




/*! 指定左端桁位置へスクロール

	@param nPos [in] スクロール位置
	@retval 実際にスクロールした桁数 (正:右方向/負:左方向)

	@date 2004.09.11 genta 桁数を戻り値として返すように．(同期スクロール用)
	@date 2008.06.08 ryoji 水平スクロール範囲にぶら下げ余白を追加
	@date 2009.08.28 nasukoji	「折り返さない」選択時右に行き過ぎないようにする
*/
int CEditView::ScrollAtH( int nPos )
{
	int			nScrollColNum;
	RECT		rcScrol;
	RECT		rcClip2;
	if( nPos < 0 ){
		nPos = 0;
	}else
	//	Aug. 18, 2003 ryoji 変数のミスを修正
	//	ウィンドウの幅をきわめて狭くしたときに編集領域が行番号から離れてしまうことがあった．
	//	Aug. 14, 2005 genta 折り返し幅をLayoutMgrから取得するように
	if( GetRightEdgeForScrollBar() - m_nViewColNum < nPos ){
		nPos = GetRightEdgeForScrollBar() - m_nViewColNum ;
		//	May 29, 2004 genta 折り返し幅よりウィンドウ幅が大きいときにWM_HSCROLLが来ると
		//	nPosが負の値になることがあり，その場合にスクロールバーから編集領域が
		//	離れてしまう．
		if( nPos < 0 )
			nPos = 0;
	}
	if( m_nViewLeftCol == nPos ){
		return 0;
	}
	/* 水平スクロール量（文字数）の算出 */
	nScrollColNum = m_nViewLeftCol - nPos;

	/* スクロール */
	if( abs( nScrollColNum ) >= m_nViewColNum /*|| abs( nScrollRowNum ) >= m_nViewRowNum*/ ){
		m_nViewLeftCol = nPos;
		::InvalidateRect( m_hWnd, NULL, TRUE );
	}else{
		rcScrol.left = 0;
		rcScrol.right = m_nViewCx + m_nViewAlignLeft;
		rcScrol.top = m_nViewAlignTop;
		rcScrol.bottom = m_nViewCy + m_nViewAlignTop;
		if( nScrollColNum > 0 ){
			rcScrol.left = m_nViewAlignLeft;
			rcScrol.right =
				m_nViewCx + m_nViewAlignLeft - nScrollColNum * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
			rcClip2.left = m_nViewAlignLeft;
			rcClip2.right = m_nViewAlignLeft + nScrollColNum * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
			rcClip2.top = m_nViewAlignTop;
			rcClip2.bottom = m_nViewCy + m_nViewAlignTop;
		}else
		if( nScrollColNum < 0 ){
			rcScrol.left = m_nViewAlignLeft - nScrollColNum * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
			rcClip2.left =
				m_nViewCx + m_nViewAlignLeft + nScrollColNum * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
			rcClip2.right = m_nViewCx + m_nViewAlignLeft;
			rcClip2.top = m_nViewAlignTop;
			rcClip2.bottom = m_nViewCy + m_nViewAlignTop;
		}
		m_nViewLeftCol = nPos;
		if( m_bDrawSWITCH ){
			::ScrollWindowEx(
				m_hWnd,
				nScrollColNum * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace ),		/* 水平スクロール量 */
				0,	/* 垂直スクロール量 */
				&rcScrol,	/* スクロール長方形の構造体のアドレス */
				NULL, NULL , NULL, SW_ERASE | SW_INVALIDATE
			);
			// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
			// 互換BMPのスクロール
			if( m_hbmpCompatBMP ){
				::BitBlt(
					m_hdcCompatDC, rcScrol.left + nScrollColNum * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace ),
						rcScrol.top, rcScrol.right - rcScrol.left, rcScrol.bottom - rcScrol.top,
					m_hdcCompatDC, rcScrol.left, rcScrol.top , SRCCOPY
				);
			}
			// カーソルの縦線がテキストと行番号の隙間にあるとき、スクロール時に縦線領域を更新
			if( m_nOldCursorLineX == m_nViewAlignLeft - 1 ){
				RECT rcClip3;
				rcClip3.left = m_nOldCursorLineX;
				rcClip3.right = m_nOldCursorLineX + 1;
				rcClip3.top  = m_nViewAlignTop;
				rcClip3.bottom = m_nViewCy + m_nViewAlignTop;
				::InvalidateRect( m_hWnd, &rcClip3, TRUE );
			}
			// To Here 2007.09.09 Moca
			::InvalidateRect( m_hWnd, &rcClip2, TRUE );
			::UpdateWindow( m_hWnd );
		}
	}
	//	2006.1.28 aroka 判定条件誤り修正 (バーが消えてもスクロールしない)
	// 先にAdjustScrollBarsを呼んでしまうと、二度目はここまでこないので、
	// DispRulerが呼ばれない。そのため、順序を入れ替えた。
	m_bRedrawRuler = true; // ルーラーを再描画する。
	HDC hdc = ::GetDC( m_hWnd );
	DispRuler( hdc );
	::ReleaseDC( m_hWnd, hdc );

	/* スクロールバーの状態を更新する */
	AdjustScrollBars();

	/* キャレットの表示・更新 */
	ShowEditCaret();

	return -nScrollColNum;	//方向が逆なので符号反転が必要
}

/*!	垂直同期スクロール

	垂直同期スクロールがONならば，対応するウィンドウを指定行数同期スクロールする
	
	@param line [in] スクロール行数 (正:下方向/負:上方向/0:何もしない)
	
	@author asa-o
	@date 2001.06.20 asa-o 新規作成
	@date 2004.09.11 genta 関数化

	@note 動作の詳細は設定や機能拡張により変更になる可能性がある

*/
void CEditView::SyncScrollV( int line )
{
	if( m_pShareData->m_Common.m_sWindow.m_bSplitterWndVScroll && line != 0
		&& m_pcEditWnd->IsEnablePane(m_nMyIndex^0x01)
	) {
		CEditView*	pcEditView = m_pcEditWnd->m_pcEditViewArr[m_nMyIndex^0x01];
#if 0
		//	差分を保ったままスクロールする場合
		pcEditView -> ScrollByV( line );
#else
		pcEditView -> ScrollAtV( m_nViewTopLine );
#endif
	}
}

/*!	水平同期スクロール

	水平同期スクロールがONならば，対応するウィンドウを指定行数同期スクロールする．
	
	@param col [in] スクロール桁数 (正:右方向/負:左方向/0:何もしない)
	
	@author asa-o
	@date 2001.06.20 asa-o 新規作成
	@date 2004.09.11 genta 関数化

	@note 動作の詳細は設定や機能拡張により変更になる可能性がある
*/
void CEditView::SyncScrollH( int col )
{
	if( m_pShareData->m_Common.m_sWindow.m_bSplitterWndHScroll && col != 0
		&& m_pcEditWnd->IsEnablePane(m_nMyIndex^0x02)
	) {
		CEditView*	pcEditView = m_pcEditWnd->m_pcEditViewArr[m_nMyIndex^0x02];
		HDC			hdc = ::GetDC( pcEditView->m_hWnd );
		
#if 0
		//	差分を保ったままスクロールする場合
		pcEditView -> ScrollByH( col );
#else
		pcEditView -> ScrollAtH( m_nViewLeftCol );
#endif
		m_bRedrawRuler = true; //2002.02.25 Add By KK スクロール時ルーラー全体を描きなおす。
		DispRuler( hdc );
		::ReleaseDC( m_hWnd, hdc );
	}
}

/* 選択範囲のデータを取得 */
/* 正常時はTRUE,範囲未選択の場合はFALSEを返す */
BOOL CEditView::GetSelectedData(
		CMemory*	cmemBuf,
		BOOL		bLineOnly,
		const char*	pszQuote,			/* 先頭に付ける引用符 */
		BOOL		bWithLineNumber,	/* 行番号を付与する */
		bool		bAddCRLFWhenCopy,	/* 折り返し位置で改行記号を入れる */
//	Jul. 25, 2000 genta
		EEolType	neweol				//	コピー後の改行コード EOL_NONEはコード保存
)
{
	const char*		pLine;
	int				nLineLen;
	int				nLineNum;
	int				nIdxFrom;
	int				nIdxTo;
	RECT			rcSel;
	int				nRowNum;
	int				nLineNumCols;
	char*			pszLineNum;
	const char*		pszSpaces = "                    ";
	const CLayout*	pcLayout;
	CEol			appendEol( neweol );
	bool			addnl = false;

	/* 範囲選択がされていない */
	if( !IsTextSelected() ){
		return FALSE;
	}
	if( bWithLineNumber ){	/* 行番号を付与する */
		/* 行番号表示に必要な桁数を計算 */
		nLineNumCols = DetectWidthOfLineNumberArea_calculate();
		nLineNumCols += 1;
		pszLineNum = new char[nLineNumCols + 1];
	}


	if( m_bBeginBoxSelect ){	/* 矩形範囲選択中 */
		/* 2点を対角とする矩形を求める */
		TwoPointToRect(
			&rcSel,
			m_sSelect.m_ptFrom.y,		/* 範囲選択開始行 */
			m_sSelect.m_ptFrom.x,		/* 範囲選択開始桁 */
			m_sSelect.m_ptTo.y,		/* 範囲選択終了行 */
			m_sSelect.m_ptTo.x			/* 範囲選択終了桁 */
		);
//		cmemBuf.SetData( "", 0 );
		cmemBuf->SetString( "" );

		//<< 2002/04/18 Azumaiya
		// サイズ分だけ要領をとっておく。
		// 結構大まかに見ています。
		int i = rcSel.bottom - rcSel.top;

		// 最初に行数分の改行量を計算してしまう。
		int nBufSize = strlen(CRLF) * i;

		// 実際の文字量。
		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( rcSel.top, &nLineLen, &pcLayout );
		for(; i != 0 && pcLayout != NULL; i--, pcLayout = pcLayout->m_pNext)
		{
			pLine = pcLayout->m_pCDocLine->m_cLine.GetStringPtr() + pcLayout->m_ptLogicPos.x;
			nLineLen = pcLayout->m_nLength;
			if( NULL != pLine )
			{
				/* 指定された桁に対応する行のデータ内の位置を調べる */
				nIdxFrom	= LineColumnToIndex( pcLayout, rcSel.left  );
				nIdxTo		= LineColumnToIndex( pcLayout, rcSel.right );

				nBufSize += nIdxTo - nIdxFrom;
			}
			if( bLineOnly ){	/* 複数行選択の場合は先頭の行のみ */
				break;
			}
		}

		// 大まかに見た容量を元にサイズをあらかじめ確保しておく。
		cmemBuf->AllocStringBuffer(nBufSize);
		//>> 2002/04/18 Azumaiya

		nRowNum = 0;
		for( nLineNum = rcSel.top; nLineNum <= rcSel.bottom; ++nLineNum ){
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum, &nLineLen, &pcLayout );
			if( NULL != pLine ){
				/* 指定された桁に対応する行のデータ内の位置を調べる */
				nIdxFrom	= LineColumnToIndex( pcLayout, rcSel.left  );
				nIdxTo		= LineColumnToIndex( pcLayout, rcSel.right );
				//2002.02.08 hor
				// pLineがNULLのとき(矩形エリアの端がEOFのみの行を含むとき)は以下を処理しない
				if( nIdxTo - nIdxFrom > 0 ){
					if( pLine[nIdxTo - 1] == '\n' || pLine[nIdxTo - 1] == '\r' ){
						cmemBuf->AppendString( &pLine[nIdxFrom], nIdxTo - nIdxFrom - 1 );
					}else{
						cmemBuf->AppendString( &pLine[nIdxFrom], nIdxTo - nIdxFrom );
					}
				}
			}
			++nRowNum;
//			if( nRowNum > 0 ){
				cmemBuf->AppendString( CRLF );
				if( bLineOnly ){	/* 複数行選択の場合は先頭の行のみ */
					break;
				}
//			}
		}
	}else{
		cmemBuf->SetString( "" );

		//<< 2002/04/18 Azumaiya
		//  これから貼り付けに使う領域の大まかなサイズを取得する。
		//  大まかというレベルですので、サイズ計算の誤差が（容量を多く見積もる方に）結構出ると思いますが、
		// まぁ、速さ優先ということで勘弁してください。
		//  無駄な容量確保が出ていますので、もう少し精度を上げたいところですが・・・。
		//  とはいえ、逆に小さく見積もることになってしまうと、かなり速度をとられる要因になってしまうので
		// 困ってしまうところですが・・・。
		m_pcEditDoc->m_cLayoutMgr.GetLineStr( m_sSelect.m_ptFrom.y, &nLineLen, &pcLayout );
		int nBufSize = 0;
		int i = m_sSelect.m_ptTo.y - m_sSelect.m_ptFrom.y;
		// 先頭に引用符を付けるとき。
		if ( NULL != pszQuote )
		{
			nBufSize += strlen(pszQuote);
		}

		// 行番号を付ける。
		if ( bWithLineNumber )
		{
			nBufSize += nLineNumCols;
		}

		// 改行コードについて。
		if ( neweol == EOL_UNKNOWN )
		{
			nBufSize += strlen(CRLF);
		}
		else
		{
			nBufSize += appendEol.GetLen();
		}

		// すべての行について同様の操作をするので、行数倍する。
		nBufSize *= i;

		// 実際の各行の長さ。
		for (; i != 0 && pcLayout != NULL; i--, pcLayout = pcLayout->m_pNext )
		{
			nBufSize += pcLayout->m_nLength + appendEol.GetLen();
			if( bLineOnly ){	/* 複数行選択の場合は先頭の行のみ */
				break;
			}
		}

		// 調べた長さ分だけバッファを取っておく。
		cmemBuf->AllocStringBuffer(nBufSize);
		//>> 2002/04/18 Azumaiya

		for( nLineNum = m_sSelect.m_ptFrom.y; nLineNum <= m_sSelect.m_ptTo.y; ++nLineNum ){
//			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum, &nLineLen );
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum, &nLineLen, &pcLayout );
			if( NULL == pLine ){
				break;
			}
			if( nLineNum == m_sSelect.m_ptFrom.y ){
				/* 指定された桁に対応する行のデータ内の位置を調べる */
				nIdxFrom = LineColumnToIndex( pcLayout, m_sSelect.m_ptFrom.x );
			}else{
				nIdxFrom = 0;
			}
			if( nLineNum == m_sSelect.m_ptTo.y ){
				/* 指定された桁に対応する行のデータ内の位置を調べる */
				nIdxTo = LineColumnToIndex( pcLayout, m_sSelect.m_ptTo.x );
			}else{
				nIdxTo = nLineLen;
			}
			if( nIdxTo - nIdxFrom == 0 ){
				continue;
			}

#if 0
			//	Jul. 25, 2000	genta
			//	改行処理変更のため削除
			/* 改行も処理するんかぃのぉ・・・？ */
			if( EOL_NONE != pcLayout->m_cEol && nIdxTo >= nLineLen ){
				nIdxTo = nLineLen - 1;
			}
#endif

			if( NULL != pszQuote && pszQuote[0] != '\0' ){	/* 先頭に付ける引用符 */
//				cmemBuf->Append( pszQuote, lstrlen( pszQuote ) );
				cmemBuf->AppendString( pszQuote );
			}
			if( bWithLineNumber ){	/* 行番号を付与する */
				wsprintf( pszLineNum, " %d:" , nLineNum + 1 );
				cmemBuf->AppendString( pszSpaces, nLineNumCols - lstrlen( pszLineNum ) );
//				cmemBuf->Append( pszLineNum, lstrlen( pszLineNum ) );
				cmemBuf->AppendString( pszLineNum );
			}


			if( EOL_NONE != pcLayout->m_cEol ){
				if( nIdxTo >= nLineLen ){
					cmemBuf->AppendString( &pLine[nIdxFrom], nLineLen - 1 - nIdxFrom );
					//	Jul. 25, 2000 genta
					cmemBuf->AppendString( ( neweol == EOL_UNKNOWN ) ?
						(pcLayout->m_cEol).GetValue() :	//	コード保存
						appendEol.GetValue() );			//	新規改行コード
				}
				else {
					cmemBuf->AppendString( &pLine[nIdxFrom], nIdxTo - nIdxFrom );
				}
			}else{
				cmemBuf->AppendString( &pLine[nIdxFrom], nIdxTo - nIdxFrom );
					//if( nIdxTo - nIdxFrom >= nLineLen ){ // 2010.11.06 ryoji 行頭以外からの選択時に[折り返し位置に改行を付けてコピー]で最初の折り返しに改行が付くように
					if( nIdxTo >= nLineLen ){
					if( bAddCRLFWhenCopy ||  /* 折り返し行に改行を付けてコピー */
						NULL != pszQuote || /* 先頭に付ける引用符 */
						bWithLineNumber 	/* 行番号を付与する */
					){
//						cmemBuf->Append( CRLF, lstrlen( CRLF ) );
						//	Jul. 25, 2000 genta
						cmemBuf->AppendString(( neweol == EOL_UNKNOWN ) ?
							CRLF :						//	コード保存
							appendEol.GetValue() );		//	新規改行コード
					}
				}
			}
			if( bLineOnly ){	/* 複数行選択の場合は先頭の行のみ */
				break;
			}
		}
	}
	if( bWithLineNumber ){	/* 行番号を付与する */
		delete [] pszLineNum;
	}
	return TRUE;
}




/* 選択範囲内の全行をクリップボードにコピーする */
void CEditView::CopySelectedAllLines(
	const char*	pszQuote,		//!< 先頭に付ける引用符
	BOOL		bWithLineNumber	//!< 行番号を付与する
)
{
	HDC			hdc;
	PAINTSTRUCT	ps;
	CLayoutRange	sSelect;
	CMemory		cmemBuf;

	if( !IsTextSelected() ){	/* テキストが選択されているか */
		return;
	}
	{	// 選択範囲内の全行を選択状態にする
		sSelect.m_ptFrom.y = m_sSelect.m_ptFrom.y;	/* 範囲選択開始行 */
		sSelect.m_ptTo.y = m_sSelect.m_ptTo.y;		/* 範囲選択終了行 */
		const CLayout* pcLayout = m_pcEditDoc->m_cLayoutMgr.SearchLineByLayoutY( m_sSelect.m_ptFrom.y );
		if( !pcLayout ) return;
		sSelect.m_ptFrom.x = pcLayout->GetIndent();	/* 範囲選択開始桁 */
		pcLayout = m_pcEditDoc->m_cLayoutMgr.SearchLineByLayoutY( m_sSelect.m_ptTo.y );
		if( pcLayout && (m_bBeginBoxSelect || m_sSelect.m_ptTo.x > pcLayout->GetIndent()) ){
			// 選択範囲を次行頭まで拡大する
			sSelect.m_ptTo.y++;
			pcLayout = pcLayout->m_pNext;
		}
		sSelect.m_ptTo.x = pcLayout? pcLayout->GetIndent(): 0;	/* 範囲選択終了桁 */
		GetAdjustCursorPos( &sSelect.m_ptTo );	// EOF行を超えていたら座標修正

		DisableSelectArea( false ); // 2011.06.03 true →false
		SetSelectArea( sSelect );

		MoveCursor( m_sSelect.m_ptTo.x, m_sSelect.m_ptTo.y, false );
		ShowEditCaret();
	}
	/* 再描画 */
	//	::UpdateWindow();
	hdc = ::GetDC( m_hWnd );
	ps.rcPaint.left = 0;
	ps.rcPaint.right = m_nViewAlignLeft + m_nViewCx;
	ps.rcPaint.top = m_nViewAlignTop;
	ps.rcPaint.bottom = m_nViewAlignTop + m_nViewCy;
	// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
	OnPaint( hdc, &ps, FALSE );
	// To Here 2007.09.09 Moca
	::ReleaseDC( m_hWnd, hdc );
	/* 選択範囲をクリップボードにコピー */
	/* 選択範囲のデータを取得 */
	/* 正常時はTRUE,範囲未選択の場合は終了する */
	if( !GetSelectedData(
		&cmemBuf,
		FALSE,
		pszQuote, /* 引用符 */
		bWithLineNumber, /* 行番号を付与する */
		m_pShareData->m_Common.m_sEdit.m_bAddCRLFWhenCopy /* 折り返し位置に改行記号を入れる */
	) ){
		ErrorBeep();
		return;
	}
	/* クリップボードにデータを設定 */
	MySetClipboardData( cmemBuf.GetStringPtr(), cmemBuf.GetStringLength(), false );
}

/* 選択エリアのテキストを指定方法で変換 */
void CEditView::ConvSelectedArea( int nFuncCode )
{
	CMemory		cmemBuf;

	CLayoutPoint sPos;

	COpe*		pcOpe = NULL;
	RECT		rcSel;

	int			nIdxFrom;
	int			nIdxTo;
	int			nLineNum;
	int			nDelPos;
	int			nDelLen;
	int			nDelPosNext;
	int			nDelLenNext;
	const char*	pLine;
	int			nLineLen;
	int			nLineLen2;
	int			i;
	CMemory*	pcMemDeleted;
	CWaitCursor cWaitCursor( m_hWnd );

	int			nSelectLineFromOld_PHY;			/* 範囲選択開始行(PHY) */
	int			nSelectColFromOld_PHY; 			/* 範囲選択開始桁(PHY) */

	/* テキストが選択されているか */
	if( !IsTextSelected() ){
		return;
	}

	m_pcEditDoc->m_cLayoutMgr.LayoutToLogic(	// 2009.07.18 ryoji PHYで記憶するように変更
		m_sSelect.m_ptFrom.x,
		m_sSelect.m_ptFrom.y,
		&nSelectColFromOld_PHY,	/* 範囲選択開始桁(PHY) */
		&nSelectLineFromOld_PHY	/* 範囲選択開始行(PHY) */
	);


	/* 矩形範囲選択中か */
	if( m_bBeginBoxSelect ){

		/* 2点を対角とする矩形を求める */
		TwoPointToRect(
			&rcSel,
			m_sSelect.m_ptFrom.y,					/* 範囲選択開始行 */
			m_sSelect.m_ptFrom.x,					/* 範囲選択開始桁 */
			m_sSelect.m_ptTo.y,					/* 範囲選択終了行 */
			m_sSelect.m_ptTo.x						/* 範囲選択終了桁 */
		);

		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( false );	// 2009.07.18 ryoji true -> false 各行にアンダーラインが残る問題の修正

		nIdxFrom = 0;
		nIdxTo = 0;
		for( nLineNum = rcSel.bottom; nLineNum >= rcSel.top - 1; nLineNum-- ){
			const CLayout* pcLayout;
			nDelPosNext = nIdxFrom;
			nDelLenNext	= nIdxTo - nIdxFrom;
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum, &nLineLen, &pcLayout );
			if( NULL != pLine ){
				/* 指定された桁に対応する行のデータ内の位置を調べる */
				nIdxFrom	= LineColumnToIndex( pcLayout, rcSel.left );
				nIdxTo		= LineColumnToIndex( pcLayout, rcSel.right );

				for( i = nIdxFrom; i <= nIdxTo; ++i ){
					if( pLine[i] == CR || pLine[i] == LF ){
						nIdxTo = i;
						break;
					}
				}
			}else{
				nIdxFrom	= 0;
				nIdxTo		= 0;
			}
			nDelPos = nDelPosNext;
			nDelLen	= nDelLenNext;
			if( nLineNum < rcSel.bottom && 0 < nDelLen ){
				m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum + 1, &nLineLen2, &pcLayout );
				sPos.x = LineIndexToColumn( pcLayout, nDelPos );
				sPos.y =  nLineNum + 1;
				if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
					pcOpe = new COpe(OPE_DELETE);
					m_pcEditDoc->m_cLayoutMgr.LayoutToLogic(
						sPos.x,
						sPos.y,
						&pcOpe->m_ptCaretPos_PHY_Before.x,
						&pcOpe->m_ptCaretPos_PHY_Before.y
					);
				}else{
					pcOpe = NULL;
				}

				pcMemDeleted = new CMemory;
				/* 指定位置の指定長データ削除 */
				DeleteData2(
					sPos,
					nDelLen,
					pcMemDeleted,
					pcOpe		/* 編集操作要素 COpe */
//					FALSE,
//					FALSE
				);
				cmemBuf.SetNativeData( pcMemDeleted );
				if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
					m_pcEditDoc->m_cLayoutMgr.LayoutToLogic(
						sPos.x,
						sPos.y,
						&pcOpe->m_ptCaretPos_PHY_After.x,
						&pcOpe->m_ptCaretPos_PHY_After.y
					);
					/* 操作の追加 */
					m_pcOpeBlk->AppendOpe( pcOpe );
				}else{
					delete pcMemDeleted;
					pcMemDeleted = NULL;
				}
				/* 機能種別によるバッファの変換 */
				ConvMemory( &cmemBuf, nFuncCode, rcSel.left );
				if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
					pcOpe = new COpe(OPE_INSERT);
					m_pcEditDoc->m_cLayoutMgr.LayoutToLogic(
						sPos.x,
						sPos.y,
						&pcOpe->m_ptCaretPos_PHY_Before.x,
						&pcOpe->m_ptCaretPos_PHY_Before.y
					);
				}
				/* 現在位置にデータを挿入 */
				CLayoutPoint ptLayoutNew;	// 挿入された部分の次の位置
				InsertData_CEditView(
					sPos,
					cmemBuf.GetStringPtr(),
					cmemBuf.GetStringLength(),
					&ptLayoutNew,
					pcOpe,
					false	// 2009.07.18 ryoji TRUE -> FALSE 各行にアンダーラインが残る問題の修正
				);
				/* カーソルを移動 */
				MoveCursor( ptLayoutNew.x, ptLayoutNew.y, false );
				m_nCaretPosX_Prev = m_ptCaretPos.x;
				if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
					pcOpe->m_ptCaretPos_PHY_After = m_ptCaretPos_PHY;	/* 操作後のキャレット位置 */
					/* 操作の追加 */
					m_pcOpeBlk->AppendOpe( pcOpe );
				}
			}
		}
		/* 挿入データの先頭位置へカーソルを移動 */
		MoveCursor( rcSel.left, rcSel.top, true );
		m_nCaretPosX_Prev = m_ptCaretPos.x;

		if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
			pcOpe = new COpe(OPE_MOVECARET);
			pcOpe->m_ptCaretPos_PHY_Before = m_ptCaretPos_PHY;				/* 操作前のキャレット位置 */
			pcOpe->m_ptCaretPos_PHY_After = pcOpe->m_ptCaretPos_PHY_Before;	/* 操作後のキャレット位置 */
			/* 操作の追加 */
			m_pcOpeBlk->AppendOpe( pcOpe );
		}
	}else{
		/* 選択範囲のデータを取得 */
		/* 正常時はTRUE,範囲未選択の場合はFALSEを返す */
		GetSelectedData( &cmemBuf, FALSE, NULL, FALSE, m_pShareData->m_Common.m_sEdit.m_bAddCRLFWhenCopy );

		/* 機能種別によるバッファの変換 */
		ConvMemory( &cmemBuf, nFuncCode, m_sSelect.m_ptFrom.x );

		/* データ置換 削除&挿入にも使える */
		ReplaceData_CEditView(
			m_sSelect,
			NULL,					/* 削除されたデータのコピー(NULL可能) */
			cmemBuf.GetStringPtr(),	/* 挿入するデータ */ // 2002/2/10 aroka CMemory変更
			cmemBuf.GetStringLength(),		/* 挿入するデータの長さ */ // 2002/2/10 aroka CMemory変更
			false
		);

		// From Here 2001.12.03 hor
		//	選択エリアの復元
		m_pcEditDoc->m_cLayoutMgr.LogicToLayout(	// 2009.07.18 ryoji PHYから戻す
			nSelectColFromOld_PHY,
			nSelectLineFromOld_PHY,
			&m_sSelect.m_ptFrom.x,	/* 範囲選択開始桁 */
			&m_sSelect.m_ptFrom.y	/* 範囲選択開始行 */
		);
		CLayoutRange sRange;
		sRange.m_ptFrom = m_sSelect.m_ptFrom;
		sRange.m_ptTo = m_ptCaretPos;
		SetSelectArea( sRange );	// 2009.07.25 ryoji
		MoveCursor( m_sSelect.m_ptTo.x, m_sSelect.m_ptTo.y, true );
		m_nCaretPosX_Prev = m_ptCaretPos.x;

		if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
			pcOpe = new COpe(OPE_MOVECARET);
			pcOpe->m_ptCaretPos_PHY_Before = m_ptCaretPos_PHY;				/* 操作前のキャレット位置 */
			pcOpe->m_ptCaretPos_PHY_After = pcOpe->m_ptCaretPos_PHY_Before;	/* 操作後のキャレット位置 */
			/* 操作の追加 */
			m_pcOpeBlk->AppendOpe( pcOpe );
		}
		// To Here 2001.12.03 hor
	}
	RedrawAll();	// 2009.07.18 ryoji 対象が矩形だった場合も最後に再描画する
}


/* 機能種別によるバッファの変換 */
void CEditView::ConvMemory( CMemory* pCMemory, int nFuncCode, int nStartColumn )
{
	switch( nFuncCode ){
	case F_TOLOWER: pCMemory->ToLower(); break;						/* 小文字 */
	case F_TOUPPER: pCMemory->ToUpper(); break;						/* 大文字 */
	case F_TOHANKAKU: pCMemory->ToHankaku( 0x0 ); break;					/* 全角→半角 */
	case F_TOHANKATA: pCMemory->ToHankaku( 0x01 ); break;					/* 全角カタカナ→半角カタカナ */	// Aug. 29, 2002 ai
	case F_TOZENEI: pCMemory->ToZenkaku( 2, 0 );				/* 2== 英数専用				*/ break;	/* 半角英数→全角英数 */			//July. 30, 2001 Misaka
	case F_TOHANEI: pCMemory->ToHankaku( 0x4 );						/* 2== 英数専用				*/ break;	/* 全角英数→半角英数 */			//July. 30, 2001 Misaka
	case F_TOZENKAKUKATA: pCMemory->ToZenkaku( 0, 0 );			/* 1== ひらがな 0==カタカナ */ break;	/* 半角＋全ひら→全角・カタカナ */	//Sept. 17, 2000 jepro 説明を「半角→全角カタカナ」から変更
	case F_TOZENKAKUHIRA: pCMemory->ToZenkaku( 1, 0 );			/* 1== ひらがな 0==カタカナ */ break;	/* 半角＋全カタ→全角・ひらがな */	//Sept. 17, 2000 jepro 説明を「半角→全角ひらがな」から変更
	case F_HANKATATOZENKATA: pCMemory->ToZenkaku( 0, 1 );		/* 1== ひらがな 0==カタカナ */ break;	/* 半角カタカナ→全角カタカナ */
	case F_HANKATATOZENHIRA: pCMemory->ToZenkaku( 1, 1 );		/* 1== ひらがな 0==カタカナ */ break;	/* 半角カタカナ→全角ひらがな */
	case F_CODECNV_EMAIL:		pCMemory->JIStoSJIS(); break;		/* E-Mail(JIS→SJIS)コード変換 */
	case F_CODECNV_EUC2SJIS:	pCMemory->EUCToSJIS(); break;		/* EUC→SJISコード変換 */
	case F_CODECNV_UNICODE2SJIS:pCMemory->UnicodeToSJIS(); break;	/* Unicode→SJISコード変換 */
	case F_CODECNV_UNICODEBE2SJIS: pCMemory->UnicodeBEToSJIS(); break;	/* UnicodeBE→SJISコード変換 */
	case F_CODECNV_SJIS2JIS:	pCMemory->SJIStoJIS();break;		/* SJIS→JISコード変換 */
	case F_CODECNV_SJIS2EUC: 	pCMemory->SJISToEUC();break;		/* SJIS→EUCコード変換 */
	case F_CODECNV_UTF82SJIS:	pCMemory->UTF8ToSJIS();break;		/* UTF-8→SJISコード変換 */
	case F_CODECNV_UTF72SJIS:	pCMemory->UTF7ToSJIS();break;		/* UTF-7→SJISコード変換 */
	case F_CODECNV_SJIS2UTF7:	pCMemory->SJISToUTF7();break;		/* SJIS→UTF-7コード変換 */
	case F_CODECNV_SJIS2UTF8:	pCMemory->SJISToUTF8();break;		/* SJIS→UTF-8コード変換 */
	case F_CODECNV_AUTO2SJIS:	pCMemory->AUTOToSJIS();break;		/* 自動判別→SJISコード変換 */
	case F_TABTOSPACE:
		pCMemory->TABToSPACE(
			//	Sep. 23, 2002 genta LayoutMgrの値を使う
			m_pcEditDoc->m_cLayoutMgr.GetTabSpace(), 
			nStartColumn
		);break;	/* TAB→空白 */
	case F_SPACETOTAB:	//#### Stonee, 2001/05/27
		pCMemory->SPACEToTAB(
			//	Sep. 23, 2002 genta LayoutMgrの値を使う
			m_pcEditDoc->m_cLayoutMgr.GetTabSpace(),
			nStartColumn
		);
		break;		/* 空白→TAB */
	case F_LTRIM:	Command_TRIM2( pCMemory , TRUE  );break;	// 2001.12.03 hor
	case F_RTRIM:	Command_TRIM2( pCMemory , FALSE );break;	// 2001.12.03 hor
	}
	return;

}



/* 指定された桁に対応する行のデータ内の位置を調べる Ver1
	
	@@@ 2002.09.28 YAZAKI CDocLine版
*/
int CEditView::LineColumnToIndex( const CDocLine* pcDocLine, int nColumn )
{
	int i2 = 0;
	CMemoryIterator<CDocLine> it( pcDocLine, m_pcEditDoc->m_cLayoutMgr.GetTabSpace() );
	while( !it.end() ){
		it.scanNext();
		if ( it.getColumn() + it.getColumnDelta() > nColumn ){
			break;
		}
		it.addDelta();
	}
	i2 += it.getIndex();
	return i2;
}


/* 指定された桁に対応する行のデータ内の位置を調べる Ver1
	
	@@@ 2002.09.28 YAZAKI CLayoutが必要になりました。
*/
int CEditView::LineColumnToIndex( const CLayout* pcLayout, int nColumn )
{
	int i2 = 0;
	CMemoryIterator<CLayout> it( pcLayout, m_pcEditDoc->m_cLayoutMgr.GetTabSpace() );
	while( !it.end() ){
		it.scanNext();
		if ( it.getColumn() + it.getColumnDelta() > nColumn ){
			break;
		}
		it.addDelta();
	}
	i2 += it.getIndex();
	return i2;
}



/* 指定された桁に対応する行のデータ内の位置を調べる Ver0 */
/* 指定された桁より、行が短い場合はpnLineAllColLenに行全体の表示桁数を返す */
/* それ以外の場合はpnLineAllColLenに０をセットする
	
	@@@ 2002.09.28 YAZAKI CLayoutが必要になりました。
*/
int CEditView::LineColumnToIndex2( const CLayout* pcLayout, int nColumn, int& pnLineAllColLen )
{
	pnLineAllColLen = 0;

	int i2 = 0;
	int nPosX2 = 0;
	CMemoryIterator<CLayout> it( pcLayout, m_pcEditDoc->m_cLayoutMgr.GetTabSpace() );
	while( !it.end() ){
		it.scanNext();
		if ( it.getColumn() + it.getColumnDelta() > nColumn ){
			break;
		}
		it.addDelta();
	}
	i2 += it.getIndex();
	if( i2 >= pcLayout->GetLength() ){
		nPosX2 += it.getColumn();
		pnLineAllColLen = nPosX2;
	}
	return i2;
}





/*
||	指定された行のデータ内の位置に対応する桁の位置を調べる
||
||	@@@ 2002.09.28 YAZAKI CLayoutが必要になりました。
*/
int CEditView::LineIndexToColumn( const CLayout* pcLayout, int nIndex )
{
	//	以下、iterator版
	int nPosX2 = 0;
	CMemoryIterator<CLayout> it( pcLayout, m_pcEditDoc->m_cLayoutMgr.GetTabSpace() );
	while( !it.end() ){
		it.scanNext();
		if ( it.getIndex() + it.getIndexDelta() > nIndex ){
			break;
		}
		it.addDelta();
	}
	nPosX2 += it.getColumn();
	return nPosX2;
}


/*
||	指定された行のデータ内の位置に対応する桁の位置を調べる
||
||	@@@ 2002.09.28 YAZAKI CDocLine版
*/
int CEditView::LineIndexToColumn( const CDocLine* pcDocLine, int nIndex )
{
	int nPosX2 = 0;
	CMemoryIterator<CDocLine> it( pcDocLine, m_pcEditDoc->m_cLayoutMgr.GetTabSpace() );
	while( !it.end() ){
		it.scanNext();
		if ( it.getIndex() + it.getIndexDelta() > nIndex ){
			break;
		}
		it.addDelta();
	}
	nPosX2 += it.getColumn();
	return nPosX2;
}



/* ポップアップメニュー(右クリック) */
int	CEditView::CreatePopUpMenu_R( void )
{
	int			nId;
//	HMENU		hMenuTop;
	HMENU		hMenu;
	POINT		po;
//	UINT		fuFlags;
//	int			cMenuItems;
//	int			nPos;
	RECT		rc;
	CMemory		cmemCurText;
	char*		pszWork;
	int			i;
	int			nMenuIdx;
	char		szLabel[300];
	char		szLabel2[300];
	UINT		uFlags;
//	BOOL		bBool;


	m_pcEditWnd->m_cMenuDrawer.ResetContents();

	/* 右クリックメニューの定義はカスタムメニュー配列の0番目 */
	nMenuIdx = CUSTMENU_INDEX_FOR_RBUTTONUP;	//マジックナンバー排除	//@@@ 2003.06.13 MIK
//	if( nMenuIdx < 0 || MAX_CUSTOM_MENU	<= nMenuIdx ){
//		return 0;
//	}
//	if( 0 == m_pShareData->m_Common.m_sCustomMenu.m_nCustMenuItemNumArr[nMenuIdx] ){
//		return 0;
//	}

	//	Oct. 3, 2001 genta
	CFuncLookup& FuncLookup = m_pcEditDoc->m_cFuncLookup;

	hMenu = ::CreatePopupMenu();
	for( i = 0; i < m_pShareData->m_Common.m_sCustomMenu.m_nCustMenuItemNumArr[nMenuIdx]; ++i ){
		if( 0 == m_pShareData->m_Common.m_sCustomMenu.m_nCustMenuItemFuncArr[nMenuIdx][i] ){
			::AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
		}else{
			//	Oct. 3, 2001 genta
			FuncLookup.Funccode2Name( m_pShareData->m_Common.m_sCustomMenu.m_nCustMenuItemFuncArr[nMenuIdx][i], szLabel, 256 );
//			::LoadString( m_hInstance, m_pShareData->m_Common.m_sCustomMenu.m_nCustMenuItemFuncArr[nMenuIdx][i], szLabel, 256 );
			/* キー */
			if( '\0' == m_pShareData->m_Common.m_sCustomMenu.m_nCustMenuItemKeyArr[nMenuIdx][i] ){
				strcpy( szLabel2, szLabel );
			}else{
				wsprintf( szLabel2, "%s (&%c)", szLabel, m_pShareData->m_Common.m_sCustomMenu.m_nCustMenuItemKeyArr[nMenuIdx][i] );
			}
			/* 機能が利用可能か調べる */
			if( IsFuncEnable( m_pcEditDoc, m_pShareData, m_pShareData->m_Common.m_sCustomMenu.m_nCustMenuItemFuncArr[nMenuIdx][i] ) ){
				uFlags = MF_STRING | MF_ENABLED;
			}else{
				uFlags = MF_STRING | MF_DISABLED | MF_GRAYED;
			}
//			bBool = ::AppendMenu( hMenu, uFlags, m_pShareData->m_Common.m_sCustomMenu.m_nCustMenuItemFuncArr[nMenuIdx][i], szLabel2 );
			m_pcEditWnd->m_cMenuDrawer.MyAppendMenu(
				hMenu, /*MF_BYPOSITION | MF_STRING*/uFlags,
				m_pShareData->m_Common.m_sCustomMenu.m_nCustMenuItemFuncArr[nMenuIdx][i] , szLabel2, _T("") );

		}
	}

	if( !m_bBeginSelect ){	/* 範囲選択中 */
		if( FALSE != KeyWordHelpSearchDict( LID_SKH_POPUPMENU_R, &po, &rc ) ){	// 2006.04.10 fon
			pszWork = m_cTipWnd.m_cInfo.GetStringPtr();
			// 2002.05.25 Moca &の考慮を追加 
			char*	pszShortOut = new char[160 + 1];
			if( 80 < lstrlen( pszWork ) ){
				char*	pszShort = new char[80 + 1];
				memcpy( pszShort, pszWork, 80 );
				pszShort[80] = '\0';
				dupamp( (const char*)pszShort, pszShortOut );
				delete [] pszShort;
			}else{
				dupamp( (const char*)pszWork, pszShortOut );
			}
			::InsertMenu( hMenu, 0, MF_BYPOSITION, IDM_COPYDICINFO, "キーワードの説明をクリップボードにコピー(&K)" );	// 2006.04.10 fon ToolTip内容を直接表示するのをやめた
			delete [] pszShortOut;
			::InsertMenu( hMenu, 1, MF_BYPOSITION, IDM_JUMPDICT, "キーワード辞書を開く(&J)" );	// 2006.04.10 fon
			::InsertMenu( hMenu, 2, MF_BYPOSITION | MF_SEPARATOR, 0, NULL );
		}
	}
	po.x = 0;
	po.y = 0;
	::GetCursorPos( &po );
	po.y -= 4;
	nId = ::TrackPopupMenu(
		hMenu,
		TPM_TOPALIGN
		| TPM_LEFTALIGN
		| TPM_RETURNCMD
		| TPM_LEFTBUTTON
		/*| TPM_RIGHTBUTTON*/
		,
		po.x,
		po.y,
		0,
		::GetParent( m_hwndParent )/*m_hWnd*/,
		NULL
	);
	::DestroyMenu( hMenu );
	return nId;
}

/*! キャレットの行桁位置およびステータスバーの状態表示の更新

	@note ステータスバーの状態の並び方の変更はメッセージを受信する
		CEditWnd::DispatchEvent()のWM_NOTIFYにも影響があることに注意
	
	@note ステータスバーの出力内容の変更はCEditWnd::OnSize()の
		カラム幅計算に影響があることに注意
*/
void CEditView::ShowCaretPosInfo( void )
{
	if( !m_bDrawSWITCH ){
		return;
	}

	char			szText[64];
	unsigned char*	pLine;
	int				nLineLen;
	int				nIdxFrom;
	int				nCharChars;
	const CLayout*	pcLayout;
	// 2002.05.26 Moca  gm_pszCodeNameArr_2 を使う
	LPCTSTR pCodeName = gm_pszCodeNameArr_2[m_pcEditDoc->m_nCharCode];
//	2002/04/08 YAZAKI コードの重複を削除

	/* カーソル位置の文字コード */
//	pLine = (unsigned char*)m_pcEditDoc->m_cLayoutMgr.GetLineStr( m_ptCaretPos.y, &nLineLen );
	pLine = (unsigned char*)m_pcEditDoc->m_cLayoutMgr.GetLineStr( m_ptCaretPos.y, &nLineLen, &pcLayout );

	//	May 12, 2000 genta
	//	改行コードの表示を追加
	//	From Here
	CEol cNlType = m_pcEditDoc->GetNewLineCode();
	const char *nNlTypeName = cNlType.GetName();
	//	To Here

	int nPosX, nPosY;
	if( m_pcEditDoc->GetDocumentAttribute().m_bLineNumIsCRLF ){	/* 行番号の表示 false=折り返し単位／true=改行単位 */
		if (pcLayout && pcLayout->m_ptLogicPos.x){
			char* pLine = pcLayout->m_pCDocLine->GetPtr();
			int nLineLen = m_ptCaretPos_PHY.x;
			nPosX = 0;
			int i;
			//	Oct. 4, 2002 genta
			//	文字位置のカウント方法が誤っていたのを修正
			for( i = 0; i < nLineLen; ){
				// 2005-09-02 D.S.Koba GetSizeOfChar
				int nCharChars = CMemory::GetSizeOfChar( (const char *)pLine, nLineLen, i );
				if ( nCharChars == 1 && pLine[i] == TAB ){
					//	Sep. 23, 2002 genta LayoutMgrの値を使う
					nPosX += m_pcEditDoc->m_cLayoutMgr.GetActualTabSpace( nPosX );
					++i;
				}
				else {
					nPosX += nCharChars;
					i += nCharChars;
				}
			}
			nPosX ++;	//	補正
		}
		else {
			nPosX = m_ptCaretPos.x + 1;
		}
		nPosY = m_ptCaretPos_PHY.y + 1;
	}
	else {
		nPosX = m_ptCaretPos.x + 1;
		nPosY = m_ptCaretPos.y + 1;
	}

	/* ステータス情報を書き出す */
	if( NULL == m_pcEditWnd->m_hwndStatusBar ){
		/* ウィンドウ右上に書き出す */
		//	May 12, 2000 genta
		//	改行コードの表示を追加
		//	From Here
		if( NULL != pLine ){
			/* 指定された桁に対応する行のデータ内の位置を調べる */
			nIdxFrom = LineColumnToIndex( pcLayout, m_ptCaretPos.x );
			if( nIdxFrom >= nLineLen ){
				/* szText */
				wsprintf( szText, "%s(%s)       %6d：%d", pCodeName, nNlTypeName, nPosY, nPosX );	//Oct. 31, 2000 JEPRO //Oct. 31, 2000 JEPRO メニューバーでの表示桁を節約
			}else{
				if( nIdxFrom < nLineLen - (pcLayout->m_cEol.GetLen()?1:0) ){
					// 2005-09-02 D.S.Koba GetSizeOfChar
					nCharChars = CMemory::GetSizeOfChar( (char *)pLine, nLineLen, nIdxFrom );
				}else{
					nCharChars = pcLayout->m_cEol.GetLen();
				}
				switch( nCharChars ){
				case 1:
					/* szText */
					wsprintf( szText, "%s(%s)   [%02x]%6d：%d", pCodeName, nNlTypeName, pLine[nIdxFrom], nPosY, nPosX );//Oct. 31, 2000 JEPRO メニューバーでの表示桁を節約
					break;
				case 2:
					/* szText */
					wsprintf( szText, "%s(%s) [%02x%02x]%6d：%d", pCodeName, nNlTypeName, pLine[nIdxFrom],  pLine[nIdxFrom + 1] , nPosY, nPosX);//Oct. 31, 2000 JEPRO メニューバーでの表示桁を節約
					break;
				case 4:
					/* szText */
					wsprintf( szText, "%s(%s) [%02x%02x%02x%02x]%d：%d", pCodeName, nNlTypeName, pLine[nIdxFrom],  pLine[nIdxFrom + 1] , pLine[nIdxFrom + 2],  pLine[nIdxFrom + 3] , nPosY, nPosX);//Oct. 31, 2000 JEPRO メニューバーでの表示桁を節約
					break;
				default:
					/* szText */
					wsprintf( szText, "%s(%s)       %6d：%d", pCodeName, nNlTypeName, nPosY, nPosX );//Oct. 31, 2000 JEPRO メニューバーでの表示桁を節約
				}
			}
		}else{
			/* szText */
			wsprintf( szText, "%s(%s)       %6d：%d", pCodeName, nNlTypeName, nPosY, nPosX );//Oct. 31, 2000 JEPRO メニューバーでの表示桁を節約
		}
		//	To Here
		//	Dec. 4, 2002 genta メニューバー表示はCEditWndが行う
		m_pcEditWnd->PrintMenubarMessage( szText );
	}else{
		/* ステータスバーに状態を書き出す */
		char	szText_1[64];
		char	szText_3[32]; // szText_2 => szTest_3 に変更 64バイトもいらない 2002.06.05 Moca 
		char	szText_6[16]; // szText_5 => szTest_6 に変更 64バイトもいらない 2002.06.05 Moca
		wsprintf( szText_1, "%5d 行 %4d 桁", nPosY, nPosX );	//Oct. 30, 2000 JEPRO 千万行も要らん

		nCharChars = 0;
		if( NULL != pLine ){
			/* 指定された桁に対応する行のデータ内の位置を調べる */
			nIdxFrom = LineColumnToIndex( pcLayout, m_ptCaretPos.x );
			if( nIdxFrom >= nLineLen ){
			}else{
				if( nIdxFrom < nLineLen - (pcLayout->m_cEol.GetLen()?1:0) ){
					// 2005-09-02 D.S.Koba GetSizeOfChar
					nCharChars = CMemory::GetSizeOfChar( (char *)pLine, nLineLen, nIdxFrom );
				}else{
					nCharChars = pcLayout->m_cEol.GetLen();
				}
			}
		}

		if( 1 == nCharChars ){
			wsprintf( szText_3, "%02x  ", pLine[nIdxFrom] );
		}else
		if( 2 == nCharChars ){
			wsprintf( szText_3, "%02x%02x", pLine[nIdxFrom],  pLine[nIdxFrom + 1] );
		// 2003.08.26 Moca CR0LF0廃止で 4 == nCharChars を削除
		}else{
			wsprintf( szText_3, "    " );
		}

		if( IsInsMode() /* Oct. 2, 2005 genta */ ){
			strcpy( szText_6, "挿入" );
		}else{
			strcpy( szText_6, "上書" );
		}
		::SendMessage( m_pcEditWnd->m_hwndStatusBar, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM) (LPINT)_T("") );
		::SendMessage( m_pcEditWnd->m_hwndStatusBar, SB_SETTEXT, 1 | 0, (LPARAM) (LPINT)szText_1 );
		//	May 12, 2000 genta
		//	改行コードの表示を追加．後ろの番号を1つずつずらす
		//	From Here
		::SendMessage( m_pcEditWnd->m_hwndStatusBar, SB_SETTEXT, 2 | 0, (LPARAM) (LPINT)nNlTypeName );
		//	To Here
		::SendMessage( m_pcEditWnd->m_hwndStatusBar, SB_SETTEXT, 3 | 0, (LPARAM) (LPINT)szText_3 );
		::SendMessage( m_pcEditWnd->m_hwndStatusBar, SB_SETTEXT, 4 | 0, (LPARAM) (LPINT)gm_pszCodeNameArr_1[m_pcEditDoc->m_nCharCode] );
		::SendMessage( m_pcEditWnd->m_hwndStatusBar, SB_SETTEXT, 5 | SBT_OWNERDRAW, (LPARAM) (LPINT)_T("") );
		::SendMessage( m_pcEditWnd->m_hwndStatusBar, SB_SETTEXT, 6 | 0, (LPARAM) (LPINT)szText_6 );
	}

	return;
}

/*!	選択範囲情報メッセージの表示

	@author genta
	@date 2005.07.09 genta 新規作成
	@date 2006.06.06 ryoji 選択範囲の行が実在しない場合の対策を追加
*/
void CEditView::PrintSelectionInfoMsg(void)
{
	//	出力されないなら計算を省略
	if( ! m_pcEditWnd->SendStatusMessage2IsEffective() )
		return;

	if( ! IsTextSelected() ){
		m_pcEditWnd->SendStatusMessage2( "" );
		return;
	}

	char msg[128];
	//	From here 2006.06.06 ryoji 選択範囲の行が実在しない場合の対策
	int nLineCount = m_pcEditDoc->m_cLayoutMgr.GetLineCount();
	if( m_sSelect.m_ptFrom.y >= nLineCount ){	// 先頭行が実在しない
		m_pcEditWnd->SendStatusMessage2( "" );
		return;
	}
	int select_line;
	if( m_sSelect.m_ptTo.y >= nLineCount ){	// 最終行が実在しない
		select_line = nLineCount - m_sSelect.m_ptFrom.y + 1;
	}
	else {
		select_line = m_sSelect.m_ptTo.y - m_sSelect.m_ptFrom.y + 1;
	}
	//	To here 2006.06.06 ryoji 選択範囲の行が実在しない場合の対策
	if( m_bBeginBoxSelect ){
		//	矩形の場合は幅と高さだけでごまかす
		int select_col = m_sSelect.m_ptFrom.x - m_sSelect.m_ptTo.x;
		if( select_col < 0 ){
			select_col = -select_col;
		}
		wsprintf( msg, "%d Columns * %d lines selected.",
			select_col, select_line );
			
	}
	else {
		//	通常の選択では選択範囲の中身を数える
		int select_sum = 0;	//	バイト数合計
		const char *pLine;	//	データを受け取る
		int	nLineLen;		//	行の長さ
		const CLayout*	pcLayout;

		//	1行目
		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( m_sSelect.m_ptFrom.y, &nLineLen, &pcLayout );
		if( pLine ){
			//	1行だけ選択されている場合
			if( m_sSelect.m_ptFrom.y == m_sSelect.m_ptTo.y ){
				select_sum = LineColumnToIndex( pcLayout, m_sSelect.m_ptTo.x )
					- LineColumnToIndex( pcLayout, m_sSelect.m_ptFrom.x );
			}
			else {	//	2行以上選択されている場合
				select_sum = pcLayout->GetLengthWithoutEOL() + pcLayout->m_cEol.GetLen()
					- LineColumnToIndex( pcLayout, m_sSelect.m_ptFrom.x );

				//	GetSelectedDataと似ているが，先頭行と最終行は排除している
				//	Aug. 16, 2005 aroka nLineNumはfor以降でも使われるのでforの前で宣言する
				//	VC .NET以降でもMicrosoft拡張を有効にした標準動作はVC6と同じことに注意
				int nLineNum;
				for( nLineNum = m_sSelect.m_ptFrom.y + 1;
					nLineNum < m_sSelect.m_ptTo.y; ++nLineNum ){
					pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum, &nLineLen, &pcLayout );
					//	2006.06.06 ryoji 指定行のデータが存在しない場合の対策
					if( NULL == pLine )
						break;
					select_sum += pcLayout->GetLengthWithoutEOL() + pcLayout->m_cEol.GetLen();
				}

				//	最終行の処理
				pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum, &nLineLen, &pcLayout );
				if( pLine ){
					int last_line_chars = LineColumnToIndex( pcLayout, m_sSelect.m_ptTo.x );
					select_sum += last_line_chars;
					if( last_line_chars == 0 ){
						//	最終行の先頭にキャレットがある場合は
						//	その行を行数に含めない
						--select_line;
					}
				}
				else
				{
					//	最終行が空行なら
					//	その行を行数に含めない
					--select_line;
				}
			}
		}

#ifdef _DEBUG
		wsprintf( msg, "%d bytes (%d lines) selected. [%d:%d]-[%d:%d]",
			select_sum, select_line,
			m_sSelect.m_ptFrom.x, m_sSelect.m_ptFrom.y,
			m_sSelect.m_ptTo.x, m_sSelect.m_ptTo.y );
#else
		wsprintf( msg, "%d bytes (%d lines) selected.", select_sum, select_line );
#endif
	}
	m_pcEditWnd->SendStatusMessage2( msg );
}


/* 設定変更を反映させる */
void CEditView::OnChangeSetting( void )
{
	RECT		rc;

	m_nTopYohaku = m_pShareData->m_Common.m_sWindow.m_nRulerBottomSpace; 		/* ルーラーとテキストの隙間 */
	m_nViewAlignTop = m_nTopYohaku;									/* 表示域の上端座標 */

	/* ルーラー表示 */
	if( m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_RULER].m_bDisp ){
		m_nViewAlignTop += m_pShareData->m_Common.m_sWindow.m_nRulerHeight;	/* ルーラー高さ */
	}

	/* フォントの変更 */
	SetFont();

	/* フォントが変わっているかもしれないので、カーソル移動 */
	MoveCursor( m_ptCaretPos.x, m_ptCaretPos.y, true );


	/* スクロールバーの状態を更新する */
	AdjustScrollBars();
	
	//	2007.09.30 genta 画面キャッシュ用CompatibleDCを用意する
	UseCompatibleDC( m_pShareData->m_Common.m_sWindow.m_bUseCompatibleBMP );

	/* ウィンドウサイズの変更処理 */
	::GetClientRect( m_hWnd, &rc );
	OnSize( rc.right, rc.bottom );

	/* 再描画 */
	::InvalidateRect( m_hWnd, NULL, TRUE );

	return;
}




/* フォーカス移動時の再描画

	@date 2001/06/21 asa-o 「スクロールバーの状態を更新する」「カーソル移動」削除
*/
void CEditView::RedrawAll( void )
{
	HDC			hdc;
	PAINTSTRUCT	ps;
	/* 再描画 */
	hdc = ::GetDC( m_hWnd );

//	OnKillFocus();

	::GetClientRect( m_hWnd, &ps.rcPaint );

	OnPaint( hdc, &ps, FALSE );
//	OnSetFocus();
	::ReleaseDC( m_hWnd, hdc );

	/* キャレットの表示 */
	ShowEditCaret();

	/* キャレットの行桁位置を表示する */
	ShowCaretPosInfo();

	/* 親ウィンドウのタイトルを更新 */
	m_pcEditWnd->UpdateCaption();

	//	Jul. 9, 2005 genta	選択範囲の情報をステータスバーへ表示
	PrintSelectionInfoMsg();

	/* スクロールバーの状態を更新する */
	AdjustScrollBars();

	return;
}

// 2001/06/21 Start by asa-o 再描画
void CEditView::Redraw( void )
{
	HDC			hdc;
	PAINTSTRUCT	ps;

	hdc = ::GetDC( m_hWnd );

	::GetClientRect( m_hWnd, &ps.rcPaint );

	OnPaint( hdc, &ps, FALSE );

	::ReleaseDC( m_hWnd, hdc );

	return;
}
// 2001/06/21 End

/** 行番号再描画
	@date 2009.03.26 ryoji 新規作成
*/
void CEditView::RedrawLineNumber( void )
{
	//描画
	PAINTSTRUCT	ps;
	ps.rcPaint.left = 0;
	ps.rcPaint.right = m_nViewAlignLeft;
	ps.rcPaint.top = m_nViewAlignTop;
	ps.rcPaint.bottom = m_nViewAlignTop + m_nViewCy;
	HDC hdc = GetDC( m_hWnd );
	OnPaint( hdc, &ps, FALSE );
	ReleaseDC( m_hWnd, hdc );
}

/* 自分の表示状態を他のビューにコピー */
void CEditView::CopyViewStatus( CEditView* pView )
{
	if( pView == NULL ){
		return;
	}
	if( pView == this ){
		return;
	}

	/* 入力状態 */
	pView->m_ptCaretPos.x 			= m_ptCaretPos.x;			/* ビュー左端からのカーソル桁位置（０開始）*/
	pView->m_nCaretPosX_Prev		= m_nCaretPosX_Prev;	/* ビュー左端からのカーソル桁位置（０オリジン）*/
	pView->m_ptCaretPos.y				= m_ptCaretPos.y;			/* ビュー上端からのカーソル行位置（０開始）*/
	pView->m_ptCaretPos_PHY.x			= m_ptCaretPos_PHY.x;		/* カーソル位置  改行単位行先頭からのバイト数（０開始）*/
	pView->m_ptCaretPos_PHY.y			= m_ptCaretPos_PHY.y;		/* カーソル位置  改行単位行の行番号（０開始）*/
//	キャレットの幅・高さはコピーしない。2002/05/12 YAZAKI
//	pView->m_nCaretWidth			= m_nCaretWidth;		/* キャレットの幅 */
//	pView->m_nCaretHeight			= m_nCaretHeight;		/* キャレットの高さ */

	/* キー状態 */
	pView->m_bSelectingLock			= m_bSelectingLock;		/* 選択状態のロック */
	pView->m_bBeginSelect			= m_bBeginSelect;		/* 範囲選択中 */
	pView->m_bBeginBoxSelect		= m_bBeginBoxSelect;	/* 矩形範囲選択中 */

	pView->m_sSelectBgn.m_ptFrom.y		= m_sSelectBgn.m_ptFrom.y;	/* 範囲選択開始行(原点) */
	pView->m_sSelectBgn.m_ptFrom.x		= m_sSelectBgn.m_ptFrom.x;	/* 範囲選択開始桁(原点) */
	pView->m_sSelectBgn.m_ptTo.y		= m_sSelectBgn.m_ptTo.y;	/* 範囲選択開始行(原点) */
	pView->m_sSelectBgn.m_ptTo.x		= m_sSelectBgn.m_ptTo.x;	/* 範囲選択開始桁(原点) */

	pView->m_sSelect.m_ptFrom.y		= m_sSelect.m_ptFrom.y;	/* 範囲選択開始行 */
	pView->m_sSelect.m_ptFrom.x		= m_sSelect.m_ptFrom.x;	/* 範囲選択開始桁 */
	pView->m_sSelect.m_ptTo.y			= m_sSelect.m_ptTo.y;		/* 範囲選択終了行 */
	pView->m_sSelect.m_ptTo.x			= m_sSelect.m_ptTo.x;		/* 範囲選択終了桁 */
	pView->m_sSelectOld.m_ptFrom.y		= m_sSelectOld.m_ptFrom.y;	/* 範囲選択開始行 */
	pView->m_sSelectOld.m_ptFrom.x		= m_sSelectOld.m_ptFrom.x;	/* 範囲選択開始桁 */
	pView->m_sSelectOld.m_ptTo.y		= m_sSelectOld.m_ptTo.y;	/* 範囲選択終了行 */
	pView->m_sSelectOld.m_ptTo.x		= m_sSelectOld.m_ptTo.x;	/* 範囲選択終了桁 */
	pView->m_nMouseRollPosXOld		= m_nMouseRollPosXOld;	/* マウス範囲選択前回位置(X座標) */
	pView->m_nMouseRollPosYOld		= m_nMouseRollPosYOld;	/* マウス範囲選択前回位置(Y座標) */

	/* 画面情報 */
	pView->m_nViewAlignLeft			= m_nViewAlignLeft;		/* 表示域の左端座標 */
	pView->m_nViewAlignLeftCols		= m_nViewAlignLeftCols;	/* 行番号域の桁数 */
	pView->m_nViewAlignTop			= m_nViewAlignTop;		/* 表示域の上端座標 */
//	pView->m_nViewCx				= m_nViewCx;			/* 表示域の幅 */
//	pView->m_nViewCy				= m_nViewCy;			/* 表示域の高さ */
//	pView->m_nViewColNum			= m_nViewColNum;		/* 表示域の桁数 */
//	pView->m_nViewRowNum			= m_nViewRowNum;		/* 表示域の行数 */
	pView->m_nViewTopLine			= m_nViewTopLine;		/* 表示域の一番上の行(0開始) */
	pView->m_nViewLeftCol			= m_nViewLeftCol;		/* 表示域の一番左の桁(0開始) */

	/* 表示方法 */
	pView->m_nCharWidth				= m_nCharWidth;			/* 半角文字の幅 */
	pView->m_nCharHeight			= m_nCharHeight;		/* 文字の高さ */

	return;
}


/* 縦・横の分割ボックス・サイズボックスのＯＮ／ＯＦＦ */
void CEditView::SplitBoxOnOff( BOOL bVert, BOOL bHorz, BOOL bSizeBox )
{
	RECT	rc;
	if( bVert ){
		if( NULL != m_pcsbwVSplitBox ){	/* 垂直分割ボックス */
		}else{
			m_pcsbwVSplitBox = new CSplitBoxWnd;
			m_pcsbwVSplitBox->Create( m_hInstance, m_hWnd, TRUE );
		}
	}else{
		delete m_pcsbwVSplitBox;	/* 垂直分割ボックス */
		m_pcsbwVSplitBox = NULL;
	}
	if( bHorz ){
		if( NULL != m_pcsbwHSplitBox ){	/* 水平分割ボックス */
		}else{
			m_pcsbwHSplitBox = new CSplitBoxWnd;
			m_pcsbwHSplitBox->Create( m_hInstance, m_hWnd, FALSE );
		}
	}else{
		delete m_pcsbwHSplitBox;	/* 水平分割ボックス */
		m_pcsbwHSplitBox = NULL;
	}

	if( bSizeBox ){
		if( NULL != m_hwndSizeBox ){
			::DestroyWindow( m_hwndSizeBox );
			m_hwndSizeBox = NULL;
		}
		m_hwndSizeBox = ::CreateWindowEx(
			0L,													/* no extended styles */
			"SCROLLBAR",										/* scroll bar control class */
			(LPSTR) NULL,										/* text for window title bar */
			WS_VISIBLE | WS_CHILD | SBS_SIZEBOX | SBS_SIZEGRIP, /* scroll bar styles */
			0,													/* horizontal position */
			0,													/* vertical position */
			200,												/* width of the scroll bar */
			CW_USEDEFAULT,										/* default height */
			m_hWnd,												/* handle of main window */
			(HMENU) NULL,										/* no menu for a scroll bar */
			m_hInstance,										/* instance owning this window */
			(LPVOID) NULL										/* pointer not needed */
		);
	}else{
		if( NULL != m_hwndSizeBox ){
			::DestroyWindow( m_hwndSizeBox );
			m_hwndSizeBox = NULL;
		}
		m_hwndSizeBox = ::CreateWindowEx(
			0L,														/* no extended styles */
			"STATIC",												/* scroll bar control class */
			(LPSTR) NULL,											/* text for window title bar */
			WS_VISIBLE | WS_CHILD /*| SBS_SIZEBOX | SBS_SIZEGRIP*/, /* scroll bar styles */
			0,														/* horizontal position */
			0,														/* vertical position */
			200,													/* width of the scroll bar */
			CW_USEDEFAULT,											/* default height */
			m_hWnd,													/* handle of main window */
			(HMENU) NULL,											/* no menu for a scroll bar */
			m_hInstance,											/* instance owning this window */
			(LPVOID) NULL											/* pointer not needed */
		);
	}
	::ShowWindow( m_hwndSizeBox, SW_SHOW );

	::GetClientRect( m_hWnd, &rc );
	OnSize( rc.right, rc.bottom );

	return;
}





/*! Grep実行

  @param[in] pcmGrepKey 検索パターン
  @param[in] pcmGrepFile 検索対象ファイルパターン(!で除外指定))
  @param[in] pcmGrepFolder 検索対象フォルダ

  @date 2008.12.07 nasukoji	ファイル名パターンのバッファオーバラン対策
  @date 2008.12.13 genta 検索パターンのバッファオーバラン対策
  @date 2012.10.13 novice 検索オプションをクラスごと代入
*/
DWORD CEditView::DoGrep(
	const CMemory*			pcmGrepKey,
	const CMemory*			pcmGrepFile,
	const CMemory*			pcmGrepFolder,
	BOOL					bGrepSubFolder,
	const SSearchOption&	sSearchOption,
	ECodeType				nGrepCharSet,	// 2002/09/21 Moca 文字コードセット選択
	BOOL					bGrepOutputLine,
	int						nGrepOutputStyle
)
{
#ifdef _DEBUG
	CRunningTimer cRunningTimer( "CEditView::DoGrep" );
#endif

	// 再入不可
	if( m_pcEditDoc->m_bGrepRunning ){
		assert( false == m_pcEditDoc->m_bGrepRunning );
		return 0xffffffff;
	}

	m_pcEditDoc->m_bGrepRunning = TRUE;

	int			nHitCount = 0;
	CDlgCancel	cDlgCancel;
	HWND		hwndCancel;
	HWND		hwndMainFrame;
	//	Jun. 27, 2001 genta	正規表現ライブラリの差し替え
	CBregexp	cRegexp;
	CMemory		cmemMessage;
	int			nWork;
	int*		pnKey_CharCharsArr = NULL;

	/*
	|| バッファサイズの調整
	*/
	cmemMessage.AllocStringBuffer( 4000 );

	m_bDoing_UndoRedo		= true;


	/* アンドゥバッファの処理 */
	if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
//@@@2002.2.2 YAZAKI NULLじゃないと進まないので、とりあえずコメント。＆NULLのときは、new COpeBlkする。
//		while( NULL != m_pcOpeBlk ){}
//		delete m_pcOpeBlk;
//		m_pcOpeBlk = NULL;
	}
	else {
		m_pcOpeBlk = new COpeBlk;
	}

	m_bCurSrchKeyMark = true;								/* 検索文字列のマーク */
	strcpy( m_szCurSrchKey, pcmGrepKey->GetStringPtr() );	/* 検索文字列 */
	m_sCurSearchOption = sSearchOption;						// 検索オプション

	/* 正規表現 */

	//	From Here Jun. 27 genta
	/*
		Grepを行うに当たって検索・画面色分け用正規表現バッファも
		初期化する．これはGrep検索結果の色分けを行うため．

		Note: ここで強調するのは最後の検索文字列であって
		Grep対象パターンではないことに注意
	*/
	if( m_sCurSearchOption.bRegularExp ){
		//	Jun. 27, 2001 genta	正規表現ライブラリの差し替え
		if( !InitRegexp( m_hWnd, m_CurRegexp, true ) ){
			m_pcEditDoc->m_bGrepRunning = FALSE;
			m_bDoing_UndoRedo = false;
			return 0;
		}

		/* 検索パターンのコンパイル */
		int nFlag = 0x00;
		nFlag |= m_sCurSearchOption.bLoHiCase ? 0x01 : 0x00;
		m_CurRegexp.Compile( m_szCurSrchKey, nFlag );
	}
	//	To Here Jun. 27 genta

	hwndCancel = cDlgCancel.DoModeless( m_hInstance, m_hwndParent, IDD_GREPRUNNING );

	::SetDlgItemInt( hwndCancel, IDC_STATIC_HITCOUNT, 0, FALSE );
	::SetDlgItemText( hwndCancel, IDC_STATIC_CURFILE, " " );	// 2002/09/09 Moca add
	::CheckDlgButton( hwndCancel, IDC_CHECK_REALTIMEVIEW, m_pShareData->m_Common.m_sSearch.m_bGrepRealTimeView );	// 2003.06.23 Moca

	//	2008.12.13 genta パターンが長すぎる場合は登録しない
	//	(正規表現が途中で途切れると困るので)
	if( pcmGrepKey->GetStringLength() < sizeof( m_pcEditDoc->m_szGrepKey )){
		strcpy( m_pcEditDoc->m_szGrepKey, pcmGrepKey->GetStringPtr() );
	}
	m_pcEditDoc->m_bGrepMode = true;

	//	2007.07.22 genta
	//	バージョン番号取得のため，処理を前の方へ移動した
	if( sSearchOption.bRegularExp ){
		if( !InitRegexp( m_hWnd, cRegexp, true ) ){
			m_pcEditDoc->m_bGrepRunning = FALSE;
			m_bDoing_UndoRedo = false;
			return 0;
		}
		/* 検索パターンのコンパイル */
		int nFlag = 0x00;
		nFlag |= sSearchOption.bLoHiCase ? 0x01 : 0x00;
		if( !cRegexp.Compile( pcmGrepKey->GetStringPtr(), nFlag ) ){
			m_pcEditDoc->m_bGrepRunning = FALSE;
			m_bDoing_UndoRedo = false;
			return 0;
		}
	}else{
		/* 検索条件の情報 */
		CDocLineMgr::CreateCharCharsArr(
			(const unsigned char *)pcmGrepKey->GetStringPtr(),
			pcmGrepKey->GetStringLength(),
			&pnKey_CharCharsArr
		);
	}

//2002.02.08 Grepアイコンも大きいアイコンと小さいアイコンを別々にする。
	HICON	hIconBig, hIconSmall;
	//	Dec, 2, 2002 genta アイコン読み込み方法変更
	hIconBig = GetAppIcon( m_hInstance, ICON_DEFAULT_GREP, FN_GREP_ICON, false );
	hIconSmall = GetAppIcon( m_hInstance, ICON_DEFAULT_GREP, FN_GREP_ICON, true );

	//	Sep. 10, 2002 genta
	//	CEditWndに新設した関数を使うように
	m_pcEditWnd->SetWindowIcon( hIconSmall, ICON_SMALL );
	m_pcEditWnd->SetWindowIcon( hIconBig, ICON_BIG );

	TCHAR szPath[_MAX_PATH];
	_tcscpy( szPath, pcmGrepFolder->GetStringPtr() );

	/* フォルダの最後が「半角かつ'\\'」でない場合は、付加する */
	AddLastYenFromDirectoryPath( szPath );

	nWork = pcmGrepKey->GetStringLength(); // 2003.06.10 Moca あらかじめ長さを計算しておく

	/* 最後にテキストを追加 */
	CMemory		cmemWork;
	cmemMessage.AppendString( "\r\n□検索条件  " );
	if( 0 < nWork ){
		CMemory cmemWork2;
		cmemWork2.SetNativeData( pcmGrepKey );
		if( m_pcEditDoc->GetDocumentAttribute().m_nStringType == 0 ){	/* 文字列区切り記号エスケープ方法  0=[\"][\'] 1=[""][''] */
			cmemWork2.Replace_j( "\\", "\\\\" );
			cmemWork2.Replace_j( "\'", "\\\'" );
			cmemWork2.Replace_j( "\"", "\\\"" );
		}else{
			cmemWork2.Replace_j( "\'", "\'\'" );
			cmemWork2.Replace_j( "\"", "\"\"" );
		}
		cmemWork.AppendString( "\"" );
		cmemWork.AppendNativeData( cmemWork2 );
		cmemWork.AppendString( "\"\r\n" );
	}else{
		cmemWork.AppendString( "「ファイル検索」\r\n" );
	}
	cmemMessage += cmemWork;



	cmemMessage.AppendString( "検索対象   " );
	if( m_pcEditDoc->GetDocumentAttribute().m_nStringType == 0 ){	/* 文字列区切り記号エスケープ方法  0=[\"][\'] 1=[""][''] */
	}else{
	}
	cmemMessage += *pcmGrepFile;




	cmemMessage.AppendString( "\r\n" );
	cmemMessage.AppendString( "フォルダ   " );
	cmemWork.SetString( szPath );
	if( m_pcEditDoc->GetDocumentAttribute().m_nStringType == 0 ){	/* 文字列区切り記号エスケープ方法  0=[\"][\'] 1=[""][''] */
	}else{
	}
	cmemMessage += cmemWork;
	cmemMessage.AppendString( "\r\n" );

	const char*	pszWork;
	if( bGrepSubFolder ){
		pszWork = "    (サブフォルダも検索)\r\n";
	}else{
		pszWork = "    (サブフォルダを検索しない)\r\n";
	}
	cmemMessage.AppendString( pszWork );

	if( 0 < nWork ){ // 2003.06.10 Moca ファイル検索の場合は表示しない // 2004.09.26 条件誤り修正
		if( sSearchOption.bWordOnly ){
		/* 単語単位で探す */
			cmemMessage.AppendString( "    (単語単位で探す)\r\n" );
		}

		if( sSearchOption.bLoHiCase ){
			pszWork = "    (英大文字小文字を区別する)\r\n";
		}else{
			pszWork = "    (英大文字小文字を区別しない)\r\n";
		}
		cmemMessage.AppendString( pszWork );

		if( sSearchOption.bRegularExp ){
			//	2007.07.22 genta : 正規表現ライブラリのバージョンも出力する
			cmemMessage.AppendString( "    (正規表現:" );
			cmemMessage.AppendString( cRegexp.GetVersionT() );
			cmemMessage.AppendString( ")\r\n" );
		}
	}

	if( CODE_AUTODETECT == nGrepCharSet ){
		cmemMessage.AppendString( "    (文字コードセットの自動判別)\r\n" );
	}else if(IsValidCodeType(nGrepCharSet)){
		cmemMessage.AppendString( "    (文字コードセット：" );
		cmemMessage.AppendString( gm_pszCodeNameArr_1[nGrepCharSet] );
		cmemMessage.AppendString( ")\r\n" );
	}

	if( 0 < nWork ){ // 2003.06.10 Moca ファイル検索の場合は表示しない // 2004.09.26 条件誤り修正
		if( bGrepOutputLine ){
		/* 該当行 */
			pszWork = "    (一致した行を出力)\r\n";
		}else{
			pszWork = "    (一致した箇所のみ出力)\r\n";
		}
		cmemMessage.AppendString( pszWork );
	}


	cmemMessage.AppendString( "\r\n\r\n" );
	pszWork = cmemMessage.GetStringPtr( &nWork );
//@@@ 2002.01.03 YAZAKI Grep直後はカーソルをGrep直前の位置に動かす
	int tmp_PosY_PHY = m_pcEditDoc->m_cLayoutMgr.GetLineCount();
	if( 0 < nWork ){
		Command_ADDTAIL( pszWork, nWork );
	}
	cmemMessage.SetString("");
	cmemWork.SetString("");

	//	2007.07.22 genta バージョンを取得するために，
	//	正規表現の初期化を上へ移動


	/* 表示処理ON/OFF */
	// 2003.06.23 Moca 共通設定で変更できるように
	// 2008.06.08 ryoji 全ビューの表示ON/OFFを同期させる
//	m_bDrawSWITCH = false;
	if( !m_pcEditWnd->UpdateTextWrap() )	// 折り返し方法関連の更新
		m_pcEditWnd->RedrawAllViews( this );	//	他のペインの表示を更新
	m_pcEditWnd->SetDrawSwitchOfAllViews( 0 != m_pShareData->m_Common.m_sSearch.m_bGrepRealTimeView );


	int nGrepTreeResult = DoGrepTree(
		&cDlgCancel,
		hwndCancel,
		pcmGrepKey->GetStringPtr(),
		pnKey_CharCharsArr,
		pcmGrepFile->GetStringPtr(),
		szPath,
		bGrepSubFolder,
		sSearchOption,
		nGrepCharSet,
		bGrepOutputLine,
		nGrepOutputStyle,
		&cRegexp,
		0,
		&nHitCount
	);
	if( -1 == nGrepTreeResult ){
		wsprintf( szPath, "中断しました。\r\n", nHitCount );
		Command_ADDTAIL( szPath, lstrlen( szPath ) );
	}
	{
		TCHAR  szBuffer[128];
		wsprintf( szBuffer, "%d 個が検索されました。\r\n", nHitCount );
		Command_ADDTAIL( szBuffer, lstrlen( szBuffer ) );
#ifdef _DEBUG
		wsprintf( szBuffer, "処理時間: %dミリ秒\r\n", cRunningTimer.Read() );
		Command_ADDTAIL( szBuffer, lstrlen( szBuffer ) );
#endif
	}
	MoveCursor( 0, tmp_PosY_PHY, true );	//	カーソルをGrep直前の位置に戻す

	cDlgCancel.CloseDialog( 0 );

	/* アクティブにする */
	hwndMainFrame = ::GetParent( m_hwndParent );
	/* アクティブにする */
	ActivateFrameWindow( hwndMainFrame );

	// アンドゥバッファの処理
	SetUndoBuffer();

	//	Apr. 13, 2001 genta
	//	Grep実行後はファイルを変更無しの状態にする．
	m_pcEditDoc->SetModified(false,false);

	m_pcEditDoc->m_bGrepRunning = FALSE;
	m_bDoing_UndoRedo = false;

	if( NULL != pnKey_CharCharsArr ){
		delete [] pnKey_CharCharsArr;
		pnKey_CharCharsArr = NULL;
	}

	/* 表示処理ON/OFF */
	m_pcEditWnd->SetDrawSwitchOfAllViews( true );

	/* 再描画 */
	if( !m_pcEditWnd->UpdateTextWrap() )	// 折り返し方法関連の更新	// 2008.06.10 ryoji
		m_pcEditWnd->RedrawAllViews( NULL );

	return nHitCount;
}



/*
 * SORTED_LIST_BSEARCH
 *   リストの探索にbsearchを使います。
 *   指定しない場合は、線形探索になります。
 * SORTED_LIST
 *   リストをqsortします。
 *
 * メモ：
 *   線形探索でもqsortを使い、文字列比較の大小関係が逆転したところで探索を
 *   打ち切れば少しは速いかもしれません。
 */
//#define SORTED_LIST
//#define SORTED_LIST_BSEARCH

#ifdef SORTED_LIST_BSEARCH
#define SORTED_LIST
#endif

#ifdef SORTED_LIST
typedef int (* COMP)(const void *, const void *);

/*!
	qsort用比較関数
	引数a,bは文字列へのポインタのポインタであることに注意。
	
	@param a [in] 比較文字列へのポインタのポインタ(list)
	@param b [in] 比較文字列へのポインタのポインタ(list)
	@return 比較結果
*/
int grep_compare_pp(const void* a, const void* b)
{
	return _tcscmp( *((const TCHAR**)a), *((const TCHAR**)b) );
}

/*!
	bsearch用比較関数
	引数bは文字列へのポインタのポインタであることに注意。
	
	@param a [in] 比較文字列へのポインタ(key)
	@param b [in] 比較文字列へのポインタのポインタ(list)
	@return 比較結果
*/
int grep_compare_sp(const void* a, const void* b)
{
	return _tcscmp( (const TCHAR*)a, *((const TCHAR**)b) );
}
#endif

/*! @brief Grep実行

	@date 2001.06.27 genta	正規表現ライブラリの差し替え
	@date 2003.06.23 Moca   サブフォルダ→ファイルだったのをファイル→サブフォルダの順に変更
	@date 2003.06.23 Moca   ファイル名から""を取り除くように
	@date 2003.03.27 みく   除外ファイル指定の導入と重複検索防止の追加．
		大部分が変更されたため，個別の変更点記入は無し．
*/
int CEditView::DoGrepTree(
	CDlgCancel*				pcDlgCancel,		//!< [in] Cancelダイアログへのポインタ
	HWND					hwndCancel,			//!< [in] Cancelダイアログのウィンドウハンドル
	const char*				pszKey,				//!< [in] 検索パターン
	int*					pnKey_CharCharsArr,	//!< [in] 文字種配列(2byte/1byte)．単純文字列検索で使用．
	const TCHAR*			pszFile,			//!< [in] 検索対象ファイルパターン(!で除外指定)
	const TCHAR*			pszPath,			//!< [in] 検索対象パス
	BOOL					bGrepSubFolder,		//!< [in] TRUE: サブフォルダを再帰的に探索する / FALSE: しない
	const SSearchOption&	sSearchOption,		//!< [in] 検索オプション
	ECodeType				nGrepCharSet,		//!< [in] 文字コードセット (0:自動認識)〜
	BOOL					bGrepOutputLine,	//!< [in] TRUE: ヒット行を出力 / FALSE: ヒット部分を出力
	int						nGrepOutputStyle,	//!< [in] 出力形式 1: Normal, 2: WZ風(ファイル単位)
	CBregexp*				pRegexp,			//!< [in] 正規表現コンパイルデータ。既にコンパイルされている必要がある
	int						nNest,				//!< [in] ネストレベル
	int*					pnHitCount			//!< [i/o] ヒット数の合計
)
{
	::SetDlgItemText( hwndCancel, IDC_STATIC_CURPATH, pszPath );

	const TCHAR EXCEPT_CHAR = _T('!');	//除外識別子
	const TCHAR* WILDCARD_DELIMITER = _T(" ;,");	//リストの区切り
	const TCHAR* WILDCARD_ANY = _T("*.*");	//サブフォルダ探索用

	int		nWildCardLen;
	int		nPos;
	BOOL	result;
	int		i;
	WIN32_FIND_DATA w32fd;
	CMemory			cmemMessage;
	int				nHitCountOld;
	int				nWork = 0;
	nHitCountOld = -100;

	//解放の対象
	HANDLE handle      = INVALID_HANDLE_VALUE;


	/*
	 * リストの初期化(文字列へのポインタをリスト管理する)
	 */
	int checked_list_size = 256;	//確保済みサイズ
	int checked_list_count = 0;	//登録個数
	TCHAR** checked_list = (TCHAR**)malloc( sizeof( TCHAR* ) * checked_list_size );
	if( ! checked_list ) return FALSE;	//メモリ確保失敗


	/*
	 * 除外ファイルを登録する。
	 */
	nPos = 0;
	TCHAR* pWildCard = _tcsdup( pszFile );	//ワイルドカードリスト作業用
	if( ! pWildCard ) goto error_return;	//メモリ確保失敗
	nWildCardLen = _tcslen( pWildCard );
	TCHAR*	token;
	while( NULL != (token = my_strtok( pWildCard, nWildCardLen, &nPos, WILDCARD_DELIMITER )) )	//トークン毎に繰り返す。
	{
		//除外ファイル指定でないか？
		if( EXCEPT_CHAR != token[0] ) continue;

		//ダブルコーテーションを除き、絶対パス名を作成する。
		TCHAR* p;
		TCHAR* q;
		p = q = ++token;
		while( *p )
		{
			if( *p != _T('"') ) *q++ = *p;
			p++;
		}
		*q = _T('\0');
		{
			std::tstring currentPath = pszPath;	//現在探索中のパス
			currentPath += token;
			//ファイルの羅列を開始する。
			handle = FindFirstFile( currentPath.c_str(), &w32fd );
		}
		result = (INVALID_HANDLE_VALUE != handle) ? TRUE : FALSE;
		while( result )
		{
			if( ! (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )	//フォルダでない場合
			{
				//チェック済みリストに登録する。
				if( checked_list_count >= checked_list_size )
				{
					checked_list_size += 256;
					TCHAR** p = (TCHAR**)realloc( checked_list, sizeof( TCHAR* ) * checked_list_size );
					if( ! p ) goto error_return;	//メモリ確保失敗
					checked_list = p;
				}
				checked_list[ checked_list_count ] = _tcsdup( w32fd.cFileName );
				checked_list_count++;
			}

			//次のファイルを羅列する。
			result = FindNextFile( handle, &w32fd );
		}
		//ハンドルを閉じる。
		if( INVALID_HANDLE_VALUE != handle )
		{
			FindClose( handle );
			handle = INVALID_HANDLE_VALUE;
		}
	}
	free( pWildCard );
	pWildCard = NULL;

	/*
	 * カレントフォルダのファイルを探索する。
	 */
	nPos = 0;
	pWildCard = _tcsdup( pszFile );
	if( ! pWildCard ) goto error_return;	//メモリ確保失敗
	nWildCardLen = _tcslen( pWildCard );
	while( NULL != (token = my_strtok( pWildCard, nWildCardLen, &nPos, WILDCARD_DELIMITER )) )	//トークン毎に繰り返す。
	{
		//除外ファイル指定か？
		if( EXCEPT_CHAR == token[0] ) continue;

		//ダブルコーテーションを除き、絶対パス名を作成する。
		TCHAR* p;
		TCHAR* q;
		p = q = token;
		while( *p )
		{
			if( *p != _T('"') ) *q++ = *p;
			p++;
		}
		*q = _T('\0');
		{
			std::tstring currentPath = pszPath;	//現在探索中のパス
			currentPath += token;
			//ファイルの羅列を開始する。
			handle = FindFirstFile( currentPath.c_str(), &w32fd );
		}
		result = (INVALID_HANDLE_VALUE != handle) ? TRUE : FALSE;
#ifdef SORTED_LIST
		//ソート
		qsort( checked_list, checked_list_count, sizeof( TCHAR* ), (COMP)grep_compare_pp );
#endif
		int current_checked_list_count = checked_list_count;	//前回までのリストの数
		while( result )
		{
			/* 処理中のユーザー操作を可能にする */
			if( !::BlockingHook( pcDlgCancel->m_hWnd ) ){
				goto cancel_return;
			}
			/* 中断ボタン押下チェック */
			if( pcDlgCancel->IsCanceled() ){
				goto cancel_return;
			}

			/* 表示設定をチェック */
			m_pcEditWnd->SetDrawSwitchOfAllViews(
				0 != ::IsDlgButtonChecked( pcDlgCancel->m_hWnd, IDC_CHECK_REALTIMEVIEW )
			);

			if( ! (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )	//フォルダでない場合
			{
				/*
				 * リストにあるか調べる。
				 * 今回探索中のファイル同士が重複することはないので、
				 * 前回までのリスト(current_checked_list_count)から検索すればよい。
				 */
#ifdef SORTED_LIST_BSEARCH
				if( ! bsearch( w32fd.cFileName, checked_list, current_checked_list_count, sizeof( TCHAR* ), (COMP)grep_compare_sp ) )
#else
				bool found = false;
				TCHAR** ptr = checked_list;
				for( i = 0; i < current_checked_list_count; i++, ptr++ )
				{
#ifdef SORTED_LIST
					int n = _tcscmp( *ptr, w32fd.cFileName );
					if( 0 == n )
					{
						found = true; 
						break;
					}
					else if( n > 0 )	//探索打ち切り
					{
						break;
					}
#else
					if( 0 == _tcscmp( *ptr, w32fd.cFileName ) )
					{
						found = true; 
						break;
					}
#endif
				}
				if( ! found )
#endif
				{
					//チェック済みリストに登録する。
					if( checked_list_count >= checked_list_size )
					{
						checked_list_size += 256;
						TCHAR** p = (TCHAR**)realloc( checked_list, sizeof( TCHAR* ) * checked_list_size );
						if( ! p ) goto error_return;	//メモリ確保失敗
						checked_list = p;
					}
					checked_list[ checked_list_count ] = _tcsdup( w32fd.cFileName );
					checked_list_count++;


					//GREP実行！
					::SetDlgItemText( hwndCancel, IDC_STATIC_CURFILE, w32fd.cFileName );

					TCHAR* currentFile = new TCHAR[ _tcslen( pszPath ) + _tcslen( w32fd.cFileName ) + 1 ];
					if( ! currentFile ) goto error_return;	//メモリ確保失敗
					_tcscpy( currentFile, pszPath );
					_tcscat( currentFile, w32fd.cFileName );
					/* ファイル内の検索 */
					int nRet = DoGrepFile(
						pcDlgCancel,
						hwndCancel,
						pszKey,
						pnKey_CharCharsArr,
						w32fd.cFileName,
						sSearchOption,
						nGrepCharSet,
						bGrepOutputLine,
						nGrepOutputStyle,
						pRegexp,
						pnHitCount,
						currentFile,
						cmemMessage
					);
					delete [] currentFile;
					currentFile = NULL;

					// 2003.06.23 Moca リアルタイム表示のときは早めに表示
					if( m_bDrawSWITCH ){
						if( _T('\0') != pszKey[0] ){
							// データ検索のときファイルの合計が最大10MBを超えたら表示
							nWork += ( w32fd.nFileSizeLow + 1023 ) / 1024;
						}
						if( *pnHitCount - nHitCountOld && 
							( *pnHitCount < 20 || 10000 < nWork ) ){
							nHitCountOld = -100; // 即表示
						}
					}
					if( *pnHitCount - nHitCountOld  >= 10 ){
						/* 結果出力 */
						if( 0 < cmemMessage.GetStringLength() ){
							Command_ADDTAIL( cmemMessage.GetStringPtr(), cmemMessage.GetStringLength() );
							Command_GOFILEEND( false );
							if( !m_pcEditWnd->UpdateTextWrap() )	// 折り返し方法関連の更新	// 2008.06.10 ryoji
								m_pcEditWnd->RedrawAllViews( this );	//	他のペインの表示を更新
							cmemMessage.SetString( _T("") );
						}
						nWork = 0;
						nHitCountOld = *pnHitCount;
					}
					if( -1 == nRet ){
						goto cancel_return;
					}
				}
			}

			//次のファイルを羅列する。
			result = FindNextFile( handle, &w32fd );
		}
		//ハンドルを閉じる。
		if( INVALID_HANDLE_VALUE != handle )
		{
			FindClose( handle );
			handle = INVALID_HANDLE_VALUE;
		}
	}
	free( pWildCard );
	pWildCard = NULL;

	for( i = 0; i < checked_list_count; i++ )
	{
		free( checked_list[ i ] );
	}
	free( checked_list );
	checked_list = NULL;
	checked_list_count = 0;
	checked_list_size = 0;

	// 2010.08.25 フォルダ移動前に残りを先に出力
	if( 0 < cmemMessage.GetStringLength() ){
		Command_ADDTAIL( cmemMessage.GetStringPtr(), cmemMessage.GetStringLength() );
		Command_GOFILEEND( false );
		if( !m_pcEditWnd->UpdateTextWrap() )	// 折り返し方法関連の更新
			m_pcEditWnd->RedrawAllViews( this );	//	他のペインの表示を更新
		cmemMessage.SetString( _T("") );
	}

	/*
	 * サブフォルダを検索する。
	 */
	if( bGrepSubFolder ){
		// 2010.08.01 キャンセルでのメモリーリーク修正
		{
			std::tstring subPath = pszPath;
			subPath += WILDCARD_ANY;
			handle = FindFirstFile( subPath.c_str(), &w32fd );
		}
		result = (INVALID_HANDLE_VALUE != handle) ? TRUE : FALSE;
		while( result )
		{
			//サブフォルダの探索を再帰呼び出し。
			/* 処理中のユーザー操作を可能にする */
			if( !::BlockingHook( pcDlgCancel->m_hWnd ) ){
				goto cancel_return;
			}
			/* 中断ボタン押下チェック */
			if( pcDlgCancel->IsCanceled() ){
				goto cancel_return;
			}
			/* 表示設定をチェック */
			m_pcEditWnd->SetDrawSwitchOfAllViews(
				0 != ::IsDlgButtonChecked( pcDlgCancel->m_hWnd, IDC_CHECK_REALTIMEVIEW )
			);

			if( (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	//フォルダの場合
			 && 0 != _tcscmp( w32fd.cFileName, _T("."))
			 && 0 != _tcscmp( w32fd.cFileName, _T("..")) )
			{
				//フォルダ名を作成する。
				// 2010.08.01 キャンセルでメモリーリークしてました
				std::tstring currentPath  = pszPath;
				currentPath += w32fd.cFileName;
				currentPath += _T("\\");

				int nGrepTreeResult = DoGrepTree(
					pcDlgCancel,
					hwndCancel,
					pszKey,
					pnKey_CharCharsArr,
					pszFile,
					currentPath.c_str(),
					bGrepSubFolder,
					sSearchOption,
					nGrepCharSet,
					bGrepOutputLine,
					nGrepOutputStyle,
					pRegexp,
					nNest + 1,
					pnHitCount
				);
				if( -1 == nGrepTreeResult ){
					goto cancel_return;
				}
				::SetDlgItemText( hwndCancel, IDC_STATIC_CURPATH, pszPath );	//@@@ 2002.01.10 add サブフォルダから戻ってきたら...

			}

			//次のファイルを羅列する。
			result = FindNextFile( handle, &w32fd );
		}
		//ハンドルを閉じる。
		if( INVALID_HANDLE_VALUE != handle )
		{
			FindClose( handle );
			handle = INVALID_HANDLE_VALUE;
		}
	}

	::SetDlgItemText( hwndCancel, IDC_STATIC_CURFILE, _T(" ") );	// 2002/09/09 Moca add

	return 0;


cancel_return:;
error_return:;
	/*
	 * エラー時はすべての確保済みリソースを解放する。
	 */
	if( INVALID_HANDLE_VALUE != handle ) FindClose( handle );

	if( pWildCard ) free( pWildCard );

	if( checked_list )
	{
		for( i = 0; i < checked_list_count; i++ )
		{
			free( checked_list[ i ] );
		}
		free( checked_list );
	}

	/* 結果出力 */
	if( 0 < cmemMessage.GetStringLength() ){
		Command_ADDTAIL( cmemMessage.GetStringPtr(), cmemMessage.GetStringLength() );
		Command_GOFILEEND( false );
		if( !m_pcEditWnd->UpdateTextWrap() )	// 折り返し方法関連の更新
			cmemMessage.GetStringPtr( &nWork );	//	他のペインの表示を更新
		cmemMessage.SetString( _T("") );
	}

	return -1;
}




/*!	@brief Grep結果を構築する

	@param pWork [out] Grep出力文字列．充分なメモリ領域を予め確保しておくこと．
		最長で 本文2000 byte＋ファイル名 _MAX_PATH byte＋行・桁位置表示の長さが必要．
		ファイル単位出力の場合は本文2500 byte + _MAX_PATH + 行・桁位置表示の長さが必要．
		

	pWorkは充分なメモリ領域を持っているコト
	@date 2002/08/29 Moca バイナリーデータに対応 pnWorkLen 追加
*/
void CEditView::SetGrepResult(
	/* データ格納先 */
	char*		pWork,
	int*		pnWorkLen,			/*!< [out] Grep出力文字列の長さ */
	/* マッチしたファイルの情報 */
	const TCHAR*		pszFullPath,	/*!< [in] フルパス */
	const TCHAR*		pszCodeName,	/*!< [in] 文字コード情報．" [SJIS]"とか */
	/* マッチした行の情報 */
	int			nLine,				/*!< [in] マッチした行番号(1〜) */
	int			nColumn,			/*!< [in] マッチした桁番号(1〜) */
	const char*		pCompareData,	/*!< [in] 行の文字列 */
	int			nLineLen,			/*!< [in] 行の文字列の長さ */
	int			nEolCodeLen,		/*!< [in] EOLの長さ */
	/* マッチした文字列の情報 */
	const char*		pMatchData,		/*!< [in] マッチした文字列 */
	int			nMatchLen,			/*!< [in] マッチした文字列の長さ */
	/* オプション */
	BOOL		bGrepOutputLine,	/*!< [in] 0: 該当部分のみ, !0: 該当行 */
	int			nGrepOutputStyle	/*!< [in] 1: Normal, 2: WZ風(ファイル単位) */
)
{

	int nWorkLen = 0;
	const char * pDispData;
	int k;
	bool bEOL = true;
	int nMaxOutStr;

	/* ノーマル */
	if( 1 == nGrepOutputStyle ){
		nWorkLen = ::wsprintf( pWork, "%s(%d,%d)%s: ", pszFullPath, nLine, nColumn, pszCodeName );
		nMaxOutStr = 2000; // 2003.06.10 Moca 最大長変更
	}
	/* WZ風 */
	else if( 2 == nGrepOutputStyle ){
		nWorkLen = ::wsprintf( pWork, "・(%6d,%-5d): ", nLine, nColumn );
		nMaxOutStr = 2500; // 2003.06.10 Moca 最大長変更
	}

	/* 該当行 */
	if( bGrepOutputLine ){
		pDispData = pCompareData;
		k = nLineLen - nEolCodeLen;
		if( nMaxOutStr < k ){
			k = nMaxOutStr; // 2003.06.10 Moca 最大長変更
		}
	}
	/* 該当部分 */
	else{
		pDispData = pMatchData;
		k = nMatchLen;
		if( nMaxOutStr < k ){
			k = nMaxOutStr; // 2003.06.10 Moca 最大長変更
		}
		// 該当部分に改行を含む場合はその改行コードをそのまま利用する(次の行に空行を作らない)
		// 2003.06.10 Moca k==0のときにバッファアンダーランしないように
		if( 0 < k && (pMatchData[ k - 1 ] == '\r' || pMatchData[ k - 1 ] == '\n') ){
			bEOL = false;
		}
	}

	memcpy( &pWork[nWorkLen], pDispData, k );
	nWorkLen += k;
	if( bEOL ){
		memcpy( &pWork[nWorkLen], "\r\n", 2 );
		nWorkLen = nWorkLen + 2;
	}
	*pnWorkLen = nWorkLen;
}

/*!
	Grep実行 (CFileLoadを使ったテスト版)

	@retval -1 GREPのキャンセル
	@retval それ以外 ヒット数(ファイル検索時はファイル数)

	@date 2001/06/27 genta	正規表現ライブラリの差し替え
	@date 2002/08/30 Moca CFileLoadを使ったテスト版
	@date 2004/03/28 genta 不要な引数nNest, bGrepSubFolder, pszPathを削除
*/
int CEditView::DoGrepFile(
	CDlgCancel*				pcDlgCancel,		//!< [in] Cancelダイアログへのポインタ
	HWND					hwndCancel,			//!< [in] Cancelダイアログのウィンドウハンドル
	const char*				pszKey,				//!< [in] 検索パターン
	int*					pnKey_CharCharsArr,	//!< [in] 文字種配列(2byte/1byte)．単純文字列検索で使用．
	const char*				pszFile,			//!< [in] 処理対象ファイル名(表示用)
	const SSearchOption&	sSearchOption,		//!< [in] 検索オプション
	ECodeType				nGrepCharSet,		//!< [in] 文字コードセット (0:自動認識)〜
	BOOL					bGrepOutputLine,	//!< [in] TRUE: ヒット行を出力 / FALSE: ヒット部分を出力
	int						nGrepOutputStyle,	//!< [in] 出力形式 1: Normal, 2: WZ風(ファイル単位)
	CBregexp*				pRegexp,			//!< [in] 正規表現コンパイルデータ。既にコンパイルされている必要がある
	int*					pnHitCount,			//!< [i/o] ヒット数の合計．元々の値に見つかった数を加算して返す．
	const TCHAR*			pszFullPath,		//!< [in] 処理対象ファイルパス
	CMemory&				cmemMessage			//!< 
)
{
	int		nHitCount;
//	char	szLine[16000];
	char	szWork[3000]; // ここは SetGrepResult() が返す文字列を格納できるサイズが必要
	char	szWork0[_MAX_PATH + 100];
	int		nLine;
	int		nWorkLen;
	const char*	pszRes; // 2002/08/29 const付加
	ECodeType	nCharCode;
	const char*	pCompareData; // 2002/08/29 const付加
	int		nColumn;
	BOOL	bOutFileName;
	bOutFileName = FALSE;
	int		nLineLen;
	const	char*	pLine;
	CEol	cEol;
	int		nEolCodeLen;
	CFileLoad	cfl;
	int		nOldPercent = 0;

	int	nKeyKen = lstrlen( pszKey );

	//	ここでは正規表現コンパイルデータの初期化は不要

	LPCTSTR	pszCodeName; // 2002/08/29 const付加
	pszCodeName = _T("");
	nHitCount = 0;
	nLine = 0;

	/* 検索条件が長さゼロの場合はファイル名だけ返す */
	// 2002/08/29 行ループの前からここに移動
	if( 0 == nKeyKen ){
		if( CODE_AUTODETECT == nGrepCharSet ){
			// 2003.06.10 Moca コード判別処理をここに移動．
			// 判別エラーでもファイル数にカウントするため
			// ファイルの日本語コードセット判別
			nCharCode = CMemory::CheckKanjiCodeOfFile( pszFullPath );
			if( CODE_NONE == nCharCode ){
				pszCodeName = "  [(DetectError)]";
			}else{
				pszCodeName = gm_pszCodeNameArr_3[nCharCode];
			}
		}
		if( 1 == nGrepOutputStyle ){
		/* ノーマル */
			wsprintf( szWork0, "%s%s\r\n", pszFullPath, pszCodeName );
		}else{
		/* WZ風 */
			wsprintf( szWork0, "■\"%s\"%s\r\n", pszFullPath, pszCodeName );
		}
		cmemMessage.AppendString( szWork0 );
		++(*pnHitCount);
		::SetDlgItemInt( hwndCancel, IDC_STATIC_HITCOUNT, *pnHitCount, FALSE );
		return 1;
	}


	try{
	// ファイルを開く
	// FileCloseで明示的に閉じるが、閉じていないときはデストラクタで閉じる
	// 2003.06.10 Moca 文字コード判定処理もFileOpenで行う
	nCharCode = cfl.FileOpen( pszFullPath, nGrepCharSet, 0 );
	if( CODE_AUTODETECT == nGrepCharSet ){
		pszCodeName = gm_pszCodeNameArr_3[nCharCode];
	}
	wsprintf( szWork0, "■\"%s\"%s\r\n", pszFullPath, pszCodeName );
//	/* 処理中のユーザー操作を可能にする */
	if( !::BlockingHook( pcDlgCancel->m_hWnd ) ){
		return -1;
	}
	/* 中断ボタン押下チェック */
	if( pcDlgCancel->IsCanceled() ){
		return -1;
	}

	/* 検索条件が長さゼロの場合はファイル名だけ返す */
	// 2002/08/29 ファイルオープンの手前へ移動

	// 注意 : cfl.ReadLine が throw する可能性がある
	while( NULL != ( pLine = cfl.ReadLine( &nLineLen, &cEol ) ) ){
		nEolCodeLen = cEol.GetLen();
		++nLine;
		pCompareData = pLine;

		/* 処理中のユーザー操作を可能にする */
		// 2010.08.31 間隔を1/32にする
		if( ((0 == nLine % 32)|| 10000 < nLineLen ) && !::BlockingHook( pcDlgCancel->m_hWnd ) ){
			return -1;
		}
		if( 0 == nLine % 64 ){
			/* 中断ボタン押下チェック */
			if( pcDlgCancel->IsCanceled() ){
				return -1;
			}
			//	2003.06.23 Moca 表示設定をチェック
			m_pcEditWnd->SetDrawSwitchOfAllViews(
				0 != ::IsDlgButtonChecked( pcDlgCancel->m_hWnd, IDC_CHECK_REALTIMEVIEW )
			);
			// 2002/08/30 Moca 進行状態を表示する(5MB以上)
			if( 5000000 < cfl.GetFileSize() ){
				int nPercent = cfl.GetPercent();
				if( 5 <= nPercent - nOldPercent ){
					nOldPercent = nPercent;
					::wsprintf( szWork, "%s (%3d%%)", pszFile, nPercent );
					::SetDlgItemText( hwndCancel, IDC_STATIC_CURFILE, szWork );
				}
			}
		}

		/* 正規表現検索 */
		if( sSearchOption.bRegularExp ){
			int nIndex = 0;
#ifdef _DEBUG
			int nIndexPrev = -1;
#endif

			//	Jun. 21, 2003 genta ループ条件見直し
			//	マッチ箇所を1行から複数検出するケースを標準に，
			//	マッチ箇所を1行から1つだけ検出する場合を例外ケースととらえ，
			//	ループ継続・打ち切り条件(bGrepOutputLine)を逆にした．
			//	Jun. 27, 2001 genta	正規表現ライブラリの差し替え
			// From Here 2005.03.19 かろと もはやBREGEXP構造体に直接アクセスしない
			// 2010.08.25 行頭以外で^にマッチする不具合の修正
			while( nIndex <= nLineLen && pRegexp->Match( pLine, nLineLen, nIndex ) ){

					//	パターン発見
					nIndex = pRegexp->GetIndex();
					int matchlen = pRegexp->GetMatchLen();
#ifdef _DEBUG
					if( nIndex <= nIndexPrev ){
						MYTRACE( _T("ERROR: CEditView::DoGrepFile() nIndex <= nIndexPrev break \n") );
						break;
					}
					nIndexPrev = nIndex;
#endif

					/* Grep結果を、szWorkに格納する */
					SetGrepResult(
						szWork,
						&nWorkLen,
						pszFullPath,
						pszCodeName,
						nLine,
						nIndex + 1,
						pLine,
						nLineLen,
						nEolCodeLen,
						pLine + nIndex,
						matchlen,
						bGrepOutputLine,
						nGrepOutputStyle
					);
					// To Here 2005.03.19 かろと もはやBREGEXP構造体に直接アクセスしない
					if( 2 == nGrepOutputStyle ){
					/* WZ風 */
						if( !bOutFileName ){
							cmemMessage.AppendString( szWork0 );
							bOutFileName = TRUE;
						}
					}
					cmemMessage.AppendString( szWork, nWorkLen );
					++nHitCount;
					++(*pnHitCount);
					if( 0 == ( (*pnHitCount) % 16 ) || *pnHitCount < 16 ){
						::SetDlgItemInt( hwndCancel, IDC_STATIC_HITCOUNT, *pnHitCount, FALSE );
					}
					//	Jun. 21, 2003 genta 行単位で出力する場合は1つ見つかれば十分
					if ( bGrepOutputLine ) {
						break;
					}
					//	探し始める位置を補正
					//	2003.06.10 Moca マッチした文字列の後ろから次の検索を開始する
					if( matchlen <= 0 ){
						matchlen = CMemory::GetSizeOfChar( pLine, nLineLen, nIndex );
						if( matchlen <= 0 ){
							matchlen = 1;
						}
					}
					nIndex += matchlen;
			}
		}
		/* 単語のみ検索 */
		else if( sSearchOption.bWordOnly ){
			/*
				2002/02/23 Norio Nakatani
				単語単位のGrepを試験的に実装。単語はWhereCurrentWord()で判別してますので、
				英単語やC/C++識別子などの検索条件ならヒットします。

				2002/03/06 YAZAKI
				Grepにも試験導入。
				WhereCurrentWordで単語を抽出して、その単語が検索語とあっているか比較する。
			*/
			int nNextWordFrom = 0;
			int nNextWordFrom2;
			int nNextWordTo2;
			// Jun. 26, 2003 genta 無駄なwhileは削除
			while( CDocLineMgr::WhereCurrentWord_2( pCompareData, nLineLen, nNextWordFrom, &nNextWordFrom2, &nNextWordTo2 , NULL, NULL ) ){
				if( nKeyKen == nNextWordTo2 - nNextWordFrom2 ){
					// const char* pData = pCompareData;	// 2002/2/10 aroka CMemory変更 , 2002/08/29 Moca pCompareDataのconst化により不要?
					/* 1==大文字小文字の区別 */
					if( (!sSearchOption.bLoHiCase && 0 == my_memicmp( &(pCompareData[nNextWordFrom2]) , pszKey, nKeyKen ) ) ||
						(sSearchOption.bLoHiCase && 0 ==	 memcmp( &(pCompareData[nNextWordFrom2]) , pszKey, nKeyKen ) )
					){
						/* Grep結果を、szWorkに格納する */
						SetGrepResult(
							szWork, &nWorkLen,
							pszFullPath, pszCodeName,
							//	Jun. 25, 2002 genta
							//	桁位置は1始まりなので1を足す必要がある
							nLine, nNextWordFrom2 + 1, pCompareData, nLineLen, nEolCodeLen,
							&(pCompareData[nNextWordFrom2]), nKeyKen,
							bGrepOutputLine, nGrepOutputStyle
						);
						if( 2 == nGrepOutputStyle ){
						/* WZ風 */
							if( !bOutFileName ){
								cmemMessage.AppendString( szWork0 );
								bOutFileName = TRUE;
							}
						}

						cmemMessage.AppendString( szWork, nWorkLen );
						++nHitCount;
						++(*pnHitCount);
						//	May 22, 2000 genta
						if( 0 == ( (*pnHitCount) % 16 ) || *pnHitCount < 16 ){
							::SetDlgItemInt( hwndCancel, IDC_STATIC_HITCOUNT, *pnHitCount, FALSE );
						}

						// 2010.10.31 ryoji 行単位で出力する場合は1つ見つかれば十分
						if ( bGrepOutputLine ) {
							break;
						}
					}
				}
				/* 現在位置の左右の単語の先頭位置を調べる */
				if( !CDocLineMgr::SearchNextWordPosition( pCompareData, nLineLen, nNextWordFrom, &nNextWordFrom, FALSE ) ){
					break;	//	次の単語が無い。
				}
			}
		}
		else {
			/* 文字列検索 */
			int nColumnPrev = 0;
			//	Jun. 21, 2003 genta ループ条件見直し
			//	マッチ箇所を1行から複数検出するケースを標準に，
			//	マッチ箇所を1行から1つだけ検出する場合を例外ケースととらえ，
			//	ループ継続・打ち切り条件(bGrepOutputLine)を逆にした．
			while(1){
				pszRes = CDocLineMgr::SearchString(
					(const unsigned char *)pCompareData,
					nLineLen,
					0,
					(const unsigned char *)pszKey,
					nKeyKen,
					pnKey_CharCharsArr,
					sSearchOption.bLoHiCase
				);
				if(!pszRes)break;

				nColumn = pszRes - pCompareData + 1;

				/* Grep結果を、szWorkに格納する */
				SetGrepResult(
					szWork, &nWorkLen,
					pszFullPath, pszCodeName,
					nLine, nColumn + nColumnPrev, pCompareData, nLineLen, nEolCodeLen,
					pszRes, nKeyKen,
					bGrepOutputLine, nGrepOutputStyle
				);
				if( 2 == nGrepOutputStyle ){
				/* WZ風 */
					if( !bOutFileName ){
						cmemMessage.AppendString( szWork0 );
						bOutFileName = TRUE;
					}
				}

				cmemMessage.AppendString( szWork, nWorkLen );
				++nHitCount;
				++(*pnHitCount);
				//	May 22, 2000 genta
				if( 0 == ( (*pnHitCount) % 16 ) || *pnHitCount < 16 ){
					::SetDlgItemInt( hwndCancel, IDC_STATIC_HITCOUNT, *pnHitCount, FALSE );
				}
				
				//	Jun. 21, 2003 genta 行単位で出力する場合は1つ見つかれば十分
				if ( bGrepOutputLine ) {
					break;
				}
				//	探し始める位置を補正
				//	2003.06.10 Moca マッチした文字列の後ろから次の検索を開始する
				//	nClom : マッチ位置
				//	matchlen : マッチした文字列の長さ
				int nPosDiff = nColumn += nKeyKen - 1;
				pCompareData += nPosDiff;
				nLineLen -= nPosDiff;
				nColumnPrev += nPosDiff;
			}
		}
	}

	// ファイルを明示的に閉じるが、ここで閉じないときはデストラクタで閉じている
	cfl.FileClose();
	} // try
	catch( CError_FileOpen ){
		wsprintf( szWork, "file open error [%s]\r\n", pszFullPath );
		Command_ADDTAIL( szWork, lstrlen( szWork ) );
		return 0;
	}
	catch( CError_FileRead ){
		wsprintf( szWork, "CEditView::DoGrepFile() ファイルの読み込み中にエラーが発生しました。\r\n");
		Command_ADDTAIL( szWork, lstrlen( szWork ) );
	} // 例外処理終わり

	return nHitCount;
}


/*
	カーソル直前の単語を取得 単語の長さを返します
	単語区切り
*/
int CEditView::GetLeftWord( CMemory* pcmemWord, int nMaxWordLen )
{
	const char*	pLine;
	int			nLineLen;
	int			nIdx;
	int			nIdxTo;
	CMemory		cmemWord;
	int			nCurLine;
	int			nCharChars;
	const CLayout* pcLayout;

	nCurLine = m_ptCaretPos.y;
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nCurLine, &nLineLen, &pcLayout );
	if( NULL == pLine ){
//		return 0;
		nIdxTo = 0;
	}else{
		/* 指定された桁に対応する行のデータ内の位置を調べる Ver1 */
		nIdxTo = LineColumnToIndex( pcLayout, m_ptCaretPos.x );
	}
	if( 0 == nIdxTo || NULL == pLine ){
		if( nCurLine <= 0 ){
			return 0;
		}
		nCurLine--;
		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nCurLine, &nLineLen );
		if( NULL == pLine ){
			return 0;
		}
		if( pLine[nLineLen - 1] == '\r' || pLine[nLineLen - 1] == '\n' ){
			return 0;
		}
		/* 指定された桁に対応する行のデータ内の位置を調べる Ver1 */
//		nIdxTo = LineColumnToIndex( pLine, nLineLen, m_ptCaretPos.x );

		nCharChars = &pLine[nLineLen] - CMemory::MemCharPrev( pLine, nLineLen, &pLine[nLineLen] );
		if( 0 == nCharChars ){
			return 0;
		}
		nIdxTo = nLineLen;
		nIdx = nIdxTo - nCharChars;

//		nIdx = nIdxTo = nLineLen - 1;
	}else{
		nCharChars = &pLine[nIdxTo] - CMemory::MemCharPrev( pLine, nLineLen, &pLine[nIdxTo] );
		if( 0 == nCharChars ){
			return 0;
		}
		nIdx = nIdxTo - nCharChars;
	}
	if( 1 == nCharChars ){
		if( pLine[nIdx] == SPACE || pLine[nIdx] == TAB ){
			return 0;
		}
	}
	if( 2 == nCharChars ){
		if( (unsigned char)pLine[nIdx	 ] == (unsigned char)0x81 &&
			(unsigned char)pLine[nIdx + 1] == (unsigned char)0x40
		){
			return 0;
		}
	}

	/* 現在位置の単語の範囲を調べる */
	CLayoutRange sRange;
	if( m_pcEditDoc->m_cLayoutMgr.WhereCurrentWord(
		nCurLine, nIdx, &sRange, &cmemWord, pcmemWord )
	){
		pcmemWord->AppendString( &pLine[nIdx], nCharChars );

		return pcmemWord->GetStringLength();
	}else{
		return 0;
	}
}
/*!
	キャレット位置の単語を取得
	単語区切り

	@param[out] pcmemWord キャレット位置の単語
	@return true: 成功，false: 失敗
	
	@date 2006.03.24 fon (CEditView::Command_SELECTWORDを流用)
*/
bool CEditView::GetCurrentWord(
	CMemory* pcmemWord
)
{
	const CLayout*	pcLayout = m_pcEditDoc->m_cLayoutMgr.SearchLineByLayoutY( m_ptCaretPos.y );
	if( NULL == pcLayout ){
		return false;	/* 単語選択に失敗 */
	}

	/* 指定された桁に対応する行のデータ内の位置を調べる */
	int nIdx = LineColumnToIndex( pcLayout, m_ptCaretPos.x );

	/* 現在位置の単語の範囲を調べる */
	CLayoutRange sRange;
	bool bResult = m_pcEditDoc->m_cLayoutMgr.WhereCurrentWord(
		m_ptCaretPos.y, nIdx,
		&sRange, pcmemWord, NULL );

	return bResult;
}


/* 指定カーソル位置が選択エリア内にあるか
	【戻り値】
	-1	選択エリアより前方 or 無選択
	0	選択エリア内
	1	選択エリアより後方
*/
int CEditView::IsCurrentPositionSelected(
	CLayoutPoint	ptCaretPos		// カーソル位置
)
{
	if( !IsTextSelected() ){	/* テキストが選択されているか */
		return -1;
	}
	RECT	rcSel;
	POINT	po;

	/* 矩形範囲選択中か */
	if( m_bBeginBoxSelect ){
		/* 2点を対角とする矩形を求める */
		TwoPointToRect(
			&rcSel,
			m_sSelect.m_ptFrom.y,		/* 範囲選択開始行 */
			m_sSelect.m_ptFrom.x,		/* 範囲選択開始桁 */
			m_sSelect.m_ptTo.y,		/* 範囲選択終了行 */
			m_sSelect.m_ptTo.x			/* 範囲選択終了桁 */
		);
		++rcSel.bottom;
		po.x = ptCaretPos.x;
		po.y = ptCaretPos.y;
		if( IsDragSource() ){
			if(GetKeyState_Control()){ /* Ctrlキーが押されていたか */
				++rcSel.left;
			}else{
				++rcSel.right;
			}
		}
		if( PtInRect( &rcSel, po ) ){
			return 0;
		}
		if( rcSel.top > ptCaretPos.y ){
			return -1;
		}
		if( rcSel.bottom < ptCaretPos.y ){
			return 1;
		}
		if( rcSel.left > ptCaretPos.x ){
			return -1;
		}
		if( rcSel.right < ptCaretPos.x ){
			return 1;
		}
	}else{
		if( m_sSelect.m_ptFrom.y > ptCaretPos.y ){
			return -1;
		}
		if( m_sSelect.m_ptTo.y < ptCaretPos.y ){
			return 1;
		}
		if( m_sSelect.m_ptFrom.y == ptCaretPos.y ){
			if( IsDragSource() ){
				if(GetKeyState_Control()){	/* Ctrlキーが押されていたか */
					if( m_sSelect.m_ptFrom.x >= ptCaretPos.x ){
						return -1;
					}
				}else{
					if( m_sSelect.m_ptFrom.x > ptCaretPos.x ){
						return -1;
					}
				}
			}else
			if( m_sSelect.m_ptFrom.x > ptCaretPos.x ){
				return -1;
			}
		}
		if( m_sSelect.m_ptTo.y == ptCaretPos.y ){
			if( IsDragSource() ){
				if(GetKeyState_Control()){	/* Ctrlキーが押されていたか */
					if( m_sSelect.m_ptTo.x <= ptCaretPos.x ){
						return 1;
					}
				}else{
					if( m_sSelect.m_ptTo.x < ptCaretPos.x ){
						return 1;
					}
				}
			}else
			if( m_sSelect.m_ptTo.x <= ptCaretPos.x ){
				return 1;
			}
		}
		return 0;
	}
	return -1;
}

/* 指定カーソル位置が選択エリア内にあるか (テスト)
	【戻り値】
	-1	選択エリアより前方 or 無選択
	0	選択エリア内
	1	選択エリアより後方
*/
int CEditView::IsCurrentPositionSelectedTEST(
	const CLayoutPoint& ptCaretPos,      //カーソル位置
	const CLayoutRange& sSelect
) const
{
	if( !IsTextSelected() ){	/* テキストが選択されているか */
		return -1;
	}

	if( sSelect.m_ptFrom.y > ptCaretPos.y ){
		return -1;
	}
	if( sSelect.m_ptTo.y < ptCaretPos.y ){
		return 1;
	}
	if( sSelect.m_ptFrom.y == ptCaretPos.y ){
		if( sSelect.m_ptFrom.x > ptCaretPos.x ){
			return -1;
		}
	}
	if( sSelect.m_ptTo.y == ptCaretPos.y ){
		if( sSelect.m_ptTo.x <= ptCaretPos.x ){
			return 1;
		}
	}
	return 0;
}

/*! クリップボードからデータを取得
	@date 2005.05.29 novice UNICODE TEXT 対応処理を追加
	@date 2007.10.04 ryoji MSDEVLineSelect対応処理を追加
	@date 2010.11.17 ryoji VS2010の行コピー対応処理を追加
*/
bool CEditView::MyGetClipboardData( CMemory& cmemBuf, bool* pbColumnSelect, bool* pbLineSelect /*= NULL*/ )
{
	HGLOBAL		hglb;
	char*		lptstr;

	if( NULL != pbColumnSelect ){
		*pbColumnSelect = false;
	}
	if( NULL != pbLineSelect ){
		*pbLineSelect = FALSE;
	}


	UINT uFormatSakuraClip;
	UINT uFormat;
	uFormatSakuraClip = ::RegisterClipboardFormat( _T("SAKURAClip") );

	// 2008/02/16 クリップボードからのファイルパス貼り付け対応	bosagami	zlib/libpng license
	if( !::IsClipboardFormatAvailable( CF_OEMTEXT )
	 && !::IsClipboardFormatAvailable( CF_HDROP )
	 && !::IsClipboardFormatAvailable( uFormatSakuraClip )
	){
		return false;
	}
	if ( !::OpenClipboard( m_hWnd ) ){
		return false;
	}

	char	szFormatName[128];

	if( NULL != pbColumnSelect || NULL != pbLineSelect ){
		/* 矩形選択や行選択のテキストデータがクリップボードにあるか */
		uFormat = 0;
		while( 0 != ( uFormat = ::EnumClipboardFormats( uFormat ) ) ){
			// Jul. 2, 2005 genta : check return value of GetClipboardFormatName
			if( ::GetClipboardFormatName( uFormat, szFormatName, sizeof(szFormatName) - 1 ) ){
				if( NULL != pbColumnSelect && 0 == lstrcmpi( _T("MSDEVColumnSelect"), szFormatName ) ){
					*pbColumnSelect = true;
					break;
				}
				if( NULL != pbLineSelect && 0 == lstrcmpi( _T("MSDEVLineSelect"), szFormatName ) ){
					*pbLineSelect = true;
					break;
				}
				if( NULL != pbLineSelect && 0 == lstrcmpi( _T("VisualStudioEditorOperationsLineCutCopyClipboardTag"), szFormatName ) ){
					*pbLineSelect = true;
					break;
				}
			}
		}
	}
	if( ::IsClipboardFormatAvailable( uFormatSakuraClip ) ){
		hglb = ::GetClipboardData( uFormatSakuraClip );
		if (hglb != NULL) {
			lptstr = (char*)::GlobalLock(hglb);
			cmemBuf.SetString( lptstr + sizeof(int), *((int*)lptstr) );
			::GlobalUnlock(hglb);
			::CloseClipboard();
			return true;
		}
	}else if(::IsClipboardFormatAvailable( CF_HDROP )){
		// 2008/02/16 クリップボードからのファイルパス貼り付け対応	bosagami	zlib/libpng license
		HDROP hDrop = (HDROP)::GetClipboardData( CF_HDROP );
		if(hDrop != NULL)
		{
			//クリップボードからコピーしたパスの情報を取得
			char sTmpPath[_MAX_PATH + 1] = {0};
			const int nMaxCnt = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

			std::vector<string> pathList;

			const char* sEol = m_pcEditDoc->GetNewLineCode().GetValue();
			for(int nLoop = 0; nLoop < nMaxCnt; nLoop++)
			{
				DragQueryFile(hDrop, nLoop, sTmpPath, sizeof(sTmpPath) - 1);
				pathList.push_back((string)sTmpPath);
			}

			//並べ替えてバッファに追加
			// 2008.08.06 nasukoji	右ボタンでのファイルドロップと仕様を合わせるため削除
//			stable_sort(pathList.begin(), pathList.end(), sort_string_nocase);

			std::vector<string>::iterator pathListItr = pathList.begin();
			while(pathListItr != pathList.end())
			{
				cmemBuf.AppendString(pathListItr->c_str());
				if(pathList.size() > 1)
				{
					cmemBuf.AppendString(sEol);
				}
				pathListItr++;
			}
			::CloseClipboard();
			return true;
		}
	}else{
		// From Here 2005/05/29 novice UNICODE TEXT 対応処理を追加
		hglb = ::GetClipboardData( CF_UNICODETEXT );
		if( hglb != NULL ){
			lptstr = (char*)::GlobalLock(hglb);
			//	UnicodeToSJISでは後ろに余計な空白が入るので，
			//	一時変数を介した後に\0までを取り出す．
			CMemory cmemUnicode( lptstr, GlobalSize(lptstr) );
			cmemUnicode.UnicodeToSJIS();
			cmemBuf.SetString( cmemUnicode.GetStringPtr() );
			::GlobalUnlock(hglb);
			::CloseClipboard();
			return true;
		}
		//	To Here 2005/05/29 novice

		hglb = ::GetClipboardData( CF_OEMTEXT );
		if( hglb != NULL ){
			lptstr = (char*)::GlobalLock(hglb);
			cmemBuf.SetString( lptstr );
			::GlobalUnlock(hglb);
			::CloseClipboard();
			return true;
		}
	}
	::CloseClipboard();
	return false;
}

/* クリップボードにデータを設定
	@date 2004.02.17 Moca エラーチェックするように
	@date 2007.10.04 ryoji MSDEVLineSelect対応処理を追加
	@date 2010.11.17 ryoji VS2010の行コピー対応処理を追加
 */
bool CEditView::MySetClipboardData( const char* pszText, int nTextLen, bool bColumnSelect, bool bLineSelect /*= false*/ )
{
	HGLOBAL		hgClipText = NULL;
	HGLOBAL		hgClipSakura = NULL;
	HGLOBAL		hgClipMSDEVColumn = NULL;
	HGLOBAL		hgClipMSDEVLine = NULL;
	HGLOBAL		hgClipMSDEVLine2 = NULL;

	char*		pszClip;
	UINT		uFormat;
	/* Windowsクリップボードにコピー */
	if( FALSE == ::OpenClipboard( m_hWnd ) ){
		return false;
	}
	::EmptyClipboard();
	// ヌル終端までの長さ
	int nNullTerminateLen = lstrlen( pszText );

	/* テキスト形式のデータ */
	hgClipText = ::GlobalAlloc(
		GMEM_MOVEABLE | GMEM_DDESHARE,
		nNullTerminateLen + 1
	);
	if( hgClipText ){
		pszClip = (char*)::GlobalLock( hgClipText );
		memcpy( pszClip, pszText, nNullTerminateLen );
		pszClip[nNullTerminateLen] = '\0';
		::GlobalUnlock( hgClipText );
		::SetClipboardData( CF_OEMTEXT, hgClipText );
	}

	/* バイナリ形式のデータ
		(int) 「データ」の長さ
		「データ」
	*/
	UINT	uFormatSakuraClip;
	uFormatSakuraClip = ::RegisterClipboardFormat( _T("SAKURAClip") );
	if( 0 != uFormatSakuraClip ){
		hgClipSakura = ::GlobalAlloc(
			GMEM_MOVEABLE | GMEM_DDESHARE,
			nTextLen + sizeof( int ) + 1
		);
		if( hgClipSakura ){
			pszClip = (char*)::GlobalLock( hgClipSakura );
			*((int*)pszClip) = nTextLen;
			memcpy( pszClip + sizeof( int ), pszText, nTextLen );
			::GlobalUnlock( hgClipSakura );
			::SetClipboardData( uFormatSakuraClip, hgClipSakura );
		}
	}

	/* 矩形選択を示すダミーデータ */
	if( bColumnSelect ){
		uFormat = ::RegisterClipboardFormat( _T("MSDEVColumnSelect") );
		if( 0 != uFormat ){
			hgClipMSDEVColumn = ::GlobalAlloc(
				GMEM_MOVEABLE | GMEM_DDESHARE,
				1
			);
			if( hgClipMSDEVColumn ){
				pszClip = (char*)::GlobalLock( hgClipMSDEVColumn );
				pszClip[0] = '\0';
				::GlobalUnlock( hgClipMSDEVColumn );
				::SetClipboardData( uFormat, hgClipMSDEVColumn );
			}
		}
	}

	/* 行選択を示すダミーデータ */
	if( bLineSelect ){
		uFormat = ::RegisterClipboardFormat( _T("MSDEVLineSelect") );
		if( 0 != uFormat ){
			hgClipMSDEVLine = ::GlobalAlloc(
				GMEM_MOVEABLE | GMEM_DDESHARE,
				1
			);
			if( hgClipMSDEVLine ){
				pszClip = (char*)::GlobalLock( hgClipMSDEVLine );
				pszClip[0] = (char)0x01;
				::GlobalUnlock( hgClipMSDEVLine );
				::SetClipboardData( uFormat, hgClipMSDEVLine );
			}
		}
		uFormat = ::RegisterClipboardFormat( _T("VisualStudioEditorOperationsLineCutCopyClipboardTag") );
		if( 0 != uFormat ){
			hgClipMSDEVLine2 = ::GlobalAlloc(
				GMEM_MOVEABLE | GMEM_DDESHARE,
				1
			);
			if( hgClipMSDEVLine2 ){
				pszClip = (char*)::GlobalLock( hgClipMSDEVLine2 );
				pszClip[0] = (char)0x01;	// ※ ClipSpy で調べるとデータはこれとは違うが内容には無関係に動くっぽい
				::GlobalUnlock( hgClipMSDEVLine2 );
				::SetClipboardData( uFormat, hgClipMSDEVLine2 );
			}
		}
	}
	::CloseClipboard();

	if( bColumnSelect && !hgClipMSDEVColumn ){
		return false;
	}
	if( bLineSelect && !(hgClipMSDEVLine && hgClipMSDEVLine2) ){
		return false;
	}
	if( !(hgClipText && hgClipSakura) ){
		return false;
	}
	return true;
}


/* 現在カーソル位置単語または選択範囲より検索等のキーを取得 */
void CEditView::GetCurrentTextForSearch( CMemory& cmemCurText )
{

	int				i;
	char			szTopic[_MAX_PATH];
	const char*		pLine;
	int				nLineLen;
	int				nIdx;
	CLayoutRange	sRange;

	cmemCurText.SetString( "" );
	szTopic[0] = '\0';
	if( IsTextSelected() ){	/* テキストが選択されているか */
		/* 選択範囲のデータを取得 */
		if( GetSelectedData( &cmemCurText, FALSE, NULL, FALSE, m_pShareData->m_Common.m_sEdit.m_bAddCRLFWhenCopy ) ){
			/* 検索文字列を現在位置の単語で初期化 */
			strncpy( szTopic, cmemCurText.GetStringPtr(), _MAX_PATH - 1 );
			szTopic[_MAX_PATH - 1] = '\0';
		}
	}else{
		const CLayout*	pcLayout;
		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( m_ptCaretPos.y, &nLineLen, &pcLayout );
		if( NULL != pLine ){
			/* 指定された桁に対応する行のデータ内の位置を調べる */
			nIdx = LineColumnToIndex( pcLayout, m_ptCaretPos.x );

			/* 現在位置の単語の範囲を調べる */
			if( m_pcEditDoc->m_cLayoutMgr.WhereCurrentWord(
				m_ptCaretPos.y, nIdx,
				&sRange, NULL, NULL )
			){
				/* 指定された行のデータ内の位置に対応する桁の位置を調べる */
				pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( sRange.m_ptFrom.y, &nLineLen, &pcLayout );
				sRange.m_ptFrom.x = LineIndexToColumn( pcLayout, sRange.m_ptFrom.x );
				pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( sRange.m_ptTo.y, &nLineLen, &pcLayout );
				sRange.m_ptTo.x = LineIndexToColumn( pcLayout, sRange.m_ptTo.x );
				/* 選択範囲の変更 */
				m_sSelectBgn = sRange;
				m_sSelect = sRange;

				/* 選択範囲のデータを取得 */
				if( GetSelectedData( &cmemCurText, FALSE, NULL, FALSE, m_pShareData->m_Common.m_sEdit.m_bAddCRLFWhenCopy ) ){
					/* 検索文字列を現在位置の単語で初期化 */
					strncpy( szTopic, cmemCurText.GetStringPtr(), MAX_PATH - 1 );
					szTopic[MAX_PATH - 1] = '\0';
				}
				/* 現在の選択範囲を非選択状態に戻す */
				DisableSelectArea( false );
			}
		}
	}

	/* 検索文字列は改行まで */
	int nLen = (int)lstrlen( szTopic );
	for( i = 0; i < nLen; ++i ){
		if( szTopic[i] == CR || szTopic[i] == LF ){
			szTopic[i] = '\0';
			break;
		}
	}
//	cmemCurText.SetData( szTopic, lstrlen( szTopic ) );
	cmemCurText.SetString( szTopic );
	return;

}


/*!	現在カーソル位置単語または選択範囲より検索等のキーを取得（ダイアログ用）
	@date 2006.08.23 ryoji 新規作成
*/
void CEditView::GetCurrentTextForSearchDlg( CMemory& cmemCurText )
{
	cmemCurText.SetString( "" );

	if( IsTextSelected() ){	// テキストが選択されている
		GetCurrentTextForSearch( cmemCurText );
	}
	else{	// テキストが選択されていない
		if( m_pShareData->m_Common.m_sSearch.m_bCaretTextForSearch ){
			GetCurrentTextForSearch( cmemCurText );	// カーソル位置単語を取得
		}
		else{
			cmemCurText.SetString( m_pShareData->m_sSearchKeywords.m_szSEARCHKEYArr[0] );	// 履歴からとってくる
		}
	}
}


/* カーソル行アンダーラインのON */
void CCaretUnderLine::CaretUnderLineON( bool bDraw )
{
	if( m_nLockCounter ) return;	//	ロックされていたら何もできない。
	m_pcEditView->CaretUnderLineON( bDraw );
}



/* カーソル行アンダーラインのOFF */
void CCaretUnderLine::CaretUnderLineOFF( bool bDraw )
{
	if( m_nLockCounter ) return;	//	ロックされていたら何もできない。
	m_pcEditView->CaretUnderLineOFF( bDraw );
}


/*! カーソル行アンダーラインのON
	@date 2007.09.09 Moca カーソル位置縦線処理追加
*/
void CEditView::CaretUnderLineON( bool bDraw )
{

	bool bUnderLine = m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_UNDERLINE].m_bDisp;
	bool bCursorVLine = m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_CURSORVLINE].m_bDisp;
	if( !bUnderLine && !bCursorVLine ){
		return;
	}

	if( IsTextSelected() ){	/* テキストが選択されているか */
		return;
	}
	// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
	if( bCursorVLine ){
		// カーソル位置縦線。-1してキャレットの左に来るように。
		m_nOldCursorLineX = m_nViewAlignLeft + (m_ptCaretPos.x - m_nViewLeftCol)
			* (m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace + m_nCharWidth ) - 1;
		if( -1 == m_nOldCursorLineX ){
			m_nOldCursorLineX = -2;
		}
	}else{
		m_nOldCursorLineX = -1;
	}

	if( bDraw
	 && m_bDrawSWITCH
	 && m_nViewAlignLeft - m_pShareData->m_Common.m_sWindow.m_nLineNumRightSpace < m_nOldCursorLineX
	 && m_nOldCursorLineX <= m_nViewAlignLeft + m_nViewCx
	 && m_bDoing_UndoRedo == false
	){
		// カーソル位置縦線の描画
		// アンダーラインと縦線の交点で、下線が上になるように先に縦線を引く。
		HDC		hdc;
		HPEN	hPen, hPenOld;
		hdc = ::GetDC( m_hWnd );
		hPen = ::CreatePen( PS_SOLID, 0, m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_CURSORVLINE].m_sColorAttr.m_cTEXT );
		hPenOld = (HPEN)::SelectObject( hdc, hPen );
		::MoveToEx( hdc, m_nOldCursorLineX, m_nViewAlignTop, NULL );
		::LineTo(   hdc, m_nOldCursorLineX, m_nViewCy + m_nViewAlignTop );
		// 「太字」のときは2dotの線にする。その際カーソルに掛からないように左側を太くする
		if( m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_CURSORVLINE].m_sFontAttr.m_bBoldFont &&
			m_nViewAlignLeft - m_pShareData->m_Common.m_sWindow.m_nLineNumRightSpace < m_nOldCursorLineX - 1 ){
			::MoveToEx( hdc, m_nOldCursorLineX - 1, m_nViewAlignTop, NULL );
			::LineTo(   hdc, m_nOldCursorLineX - 1, m_nViewCy + m_nViewAlignTop );
		}
		::SelectObject( hdc, hPenOld );
		::DeleteObject( hPen );
		::ReleaseDC( m_hWnd, hdc );
		hdc= NULL;
	}
	if( bUnderLine ){
		m_nOldUnderLineY = m_nViewAlignTop + (m_ptCaretPos.y - m_nViewTopLine)
			 * (m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight) + m_nCharHeight;
		if( -1 == m_nOldUnderLineY ){
			m_nOldUnderLineY = -2;
		}
	}else{
		m_nOldUnderLineY = -1;
	}
	// To Here 2007.09.09 Moca

	if( bDraw
	 && m_bDrawSWITCH
	 && m_nOldUnderLineY >=m_nViewAlignTop
	 && m_bDoing_UndoRedo == false	/* アンドゥ・リドゥの実行中か */
	){
//		MYTRACE( _T("★カーソル行アンダーラインの描画\n") );
		/* ★カーソル行アンダーラインの描画 */
		HDC		hdc;
		HPEN	hPen, hPenOld;
		hdc = ::GetDC( m_hWnd );
		hPen = ::CreatePen( PS_SOLID, 0, m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_UNDERLINE].m_sColorAttr.m_cTEXT );
		hPenOld = (HPEN)::SelectObject( hdc, hPen );
		::MoveToEx(
			hdc,
			m_nViewAlignLeft,
			m_nOldUnderLineY,
			NULL
		);
		::LineTo(
			hdc,
			m_nViewCx + m_nViewAlignLeft,
			m_nOldUnderLineY
		);
		::SelectObject( hdc, hPenOld );
		::DeleteObject( hPen );
		::ReleaseDC( m_hWnd, hdc );
		hdc= NULL;
	}
}

/*! カーソル行アンダーラインのOFF
	@date 2007.09.09 Moca カーソル位置縦線処理追加
*/
void CEditView::CaretUnderLineOFF( bool bDraw )
{
	if( !m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_UNDERLINE].m_bDisp &&
			!m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_CURSORVLINE].m_bDisp ){
		return;
	}

	if( -1 != m_nOldUnderLineY ){
		if( bDraw
		 && m_bDrawSWITCH
		 && m_nOldUnderLineY >=m_nViewAlignTop
		 && !m_bDoing_UndoRedo	/* アンドゥ・リドゥの実行中か */
		){
			/* カーソル行アンダーラインの消去（無理やり） */
			PAINTSTRUCT ps;
			ps.rcPaint.left = m_nViewAlignLeft;
			ps.rcPaint.right = m_nViewAlignLeft + m_nViewCx;
			ps.rcPaint.top = m_nOldUnderLineY;
			ps.rcPaint.bottom = m_nOldUnderLineY + 1; // 2007.09.09 Moca +1 するように
			HDC hdc = ::GetDC( m_hWnd );
			m_cUnderLine.Lock();
			//	不本意ながら選択情報をバックアップ。
			CLayoutRange sSelect = m_sSelect;
			m_sSelect.m_ptFrom.y = -1;
			m_sSelect.m_ptTo.y = -1;
			m_sSelect.m_ptFrom.x = -1;
			m_sSelect.m_ptTo.x = -1;
			// 可能なら互換BMPからコピーして再作画
			OnPaint( hdc, &ps, TRUE );
			//	選択情報を復元
			m_sSelect = sSelect;
			m_cUnderLine.UnLock();
			ReleaseDC( m_hWnd, hdc );
		}
		m_nOldUnderLineY = -1;
	}
	
	// From Here 2007.09.09 Moca 互換BMPによる画面バッファ
	// カーソル位置縦線
	if( -1 != m_nOldCursorLineX ){
		if( bDraw
		 && m_bDrawSWITCH
		 && m_nViewAlignLeft - m_pShareData->m_Common.m_sWindow.m_nLineNumRightSpace < m_nOldCursorLineX
		 && m_nOldCursorLineX <= m_nViewAlignLeft + m_nViewCx
		 && m_bDoing_UndoRedo == false
		){
			PAINTSTRUCT ps;
			ps.rcPaint.left = m_nOldCursorLineX;
			ps.rcPaint.right = m_nOldCursorLineX + 1;
			ps.rcPaint.top = m_nViewAlignTop;
			ps.rcPaint.bottom = m_nViewAlignTop + m_nViewCy;
			if( m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_CURSORVLINE].m_sFontAttr.m_bBoldFont ){
				ps.rcPaint.left += -1;
			}
			HDC hdc = ::GetDC( m_hWnd );
			m_cUnderLine.Lock();
			//	不本意ながら選択情報をバックアップ。
			CLayoutRange sSelect = m_sSelect;
			m_sSelect.m_ptFrom.y = -1;
			m_sSelect.m_ptTo.y = -1;
			m_sSelect.m_ptFrom.x = -1;
			m_sSelect.m_ptTo.x = -1;
			// 可能なら互換BMPからコピーして再作画
			OnPaint( hdc, &ps, TRUE );
			//	選択情報を復元
			m_sSelect = sSelect;
			m_cUnderLine.UnLock();
			ReleaseDC( m_hWnd, hdc );
		}
		m_nOldCursorLineX = -1;
	}
	// To Here 2007.09.09 Moca
	return;
}

/*!
	検索／置換／ブックマーク検索時の状態をステータスバーに表示する

	@date 2002.01.26 hor 新規作成
	@date 2002.12.04 genta 実体をCEditWndへ移動
*/
void CEditView::SendStatusMessage( const TCHAR* msg )
{
	m_pcEditWnd->SendStatusMessage( msg );
}


/*!
	@date 2003/02/18 ai
	@param flag [in] モード(true:登録, false:解除)
*/
void CEditView::SetBracketPairPos( bool flag )
{
	int	mode;

	// 03/03/06 ai すべて置換、すべて置換後のUndo&Redoがかなり遅い問題に対応
	if( m_bDoing_UndoRedo || !m_bDrawSWITCH ){
		return;
	}

	if( !m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_BRACKET_PAIR].m_bDisp ){
		return;
	}

	// 対括弧の検索&登録
	/*
	bit0(in)  : 表示領域外を調べるか？ 0:調べない  1:調べる
	bit1(in)  : 前方文字を調べるか？   0:調べない  1:調べる
	bit2(out) : 見つかった位置         0:後ろ      1:前
	*/
	mode = 2;

	CLayoutPoint ptColLine;

	if( ( flag == true ) && !IsTextSelected() && !m_bDrawSelectArea
		&& ( m_bBeginBoxSelect == false ) && SearchBracket( m_ptCaretPos, &ptColLine, &mode ) )
	{
		// 登録指定(flag=true)			&&
		// テキストが選択されていない	&&
		// 選択範囲を描画していない		&&
		// 矩形範囲選択中でない			&&
		// 対応する括弧が見つかった		場合
		if ( ( ptColLine.x >= m_nViewLeftCol ) && ( ptColLine.x <= m_nViewLeftCol + m_nViewColNum )
			&& ( ptColLine.y >= m_nViewTopLine ) && ( ptColLine.y <= m_nViewTopLine + m_nViewRowNum ) )
		{
			// 表示領域内の場合

			// レイアウト位置から物理位置へ変換(強調表示位置を登録)
			m_pcEditDoc->m_cLayoutMgr.LayoutToLogic( ptColLine.x, ptColLine.y, &m_ptBracketPairPos_PHY.x, &m_ptBracketPairPos_PHY.y );
			m_ptBracketCaretPos_PHY.y = m_ptCaretPos_PHY.y;
			if( 0 == ( mode & 4 ) ){
				// カーソルの後方文字位置
				m_ptBracketCaretPos_PHY.x = m_ptCaretPos_PHY.x;
			}else{
				// カーソルの前方文字位置
				m_ptBracketCaretPos_PHY.x = m_ptCaretPos_PHY.x - m_nCharSize;
			}
			return;
		}
	}

	// 括弧の強調表示位置情報初期化
	m_ptBracketPairPos_PHY.x  = -1;
	m_ptBracketPairPos_PHY.y  = -1;
	m_ptBracketCaretPos_PHY.x = -1;
	m_ptBracketCaretPos_PHY.y = -1;

	return;
}

/*!
	対括弧の強調表示
	@date 2002/09/18 ai
	@date 2003/02/18 ai 再描画対応の為大改造
*/
void CEditView::DrawBracketPair( bool bDraw )
{
	int			i;
	COLORREF	crBackOld;
	COLORREF	crTextOld;
	HFONT		hFontOld;

	// 03/03/06 ai すべて置換、すべて置換後のUndo&Redoがかなり遅い問題に対応
	if( m_bDoing_UndoRedo || !m_bDrawSWITCH ){
		return;
	}

	if( !m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_BRACKET_PAIR].m_bDisp ){
		return;
	}

	// 括弧の強調表示位置が未登録の場合は終了
	if( ( m_ptBracketPairPos_PHY.x  < 0 ) || ( m_ptBracketPairPos_PHY.y  < 0 )
	 || ( m_ptBracketCaretPos_PHY.x < 0 ) || ( m_ptBracketCaretPos_PHY.y < 0 ) ){
		return;
	}

	// 描画指定(bDraw=true)				かつ
	// ( テキストが選択されている		又は
	//   選択範囲を描画している			又は
	//   矩形範囲選択中					又は
	//   フォーカスを持っていない		又は
	//   アクティブなペインではない )	場合は終了
	if( bDraw
	 &&( IsTextSelected() || m_bDrawSelectArea || m_bBeginBoxSelect || !m_bDrawBracketPairFlag
	 || ( m_pcEditWnd->GetActivePane() != m_nMyIndex ) ) ){
		return;
	}

	HDC hdc = ::GetDC( m_hWnd );
	STypeConfig *TypeDataPtr = &( m_pcEditDoc->GetDocumentAttribute() );

	for( i = 0; i < 2; i++ )
	{
		// i=0:対括弧,i=1:カーソル位置の括弧
		// 2011.11.23 ryoji 対括弧 -> カーソル位置の括弧 の順に処理順序を変更
		//   ＃ { と } が異なる行にある場合に { を BS で消すと } の強調表示が解除されない問題（Wiki BugReport/89）の対策
		//   ＃ この順序変更によりカーソル位置が括弧でなくなっていても対括弧があれば対括弧側の強調表示は解除される

		CLayoutPoint	ptColLine;

		if( i == 0 ){
			m_pcEditDoc->m_cLayoutMgr.LogicToLayout( m_ptBracketPairPos_PHY.x,  m_ptBracketPairPos_PHY.y,  &ptColLine.x, &ptColLine.y );
		}else{
			m_pcEditDoc->m_cLayoutMgr.LogicToLayout( m_ptBracketCaretPos_PHY.x, m_ptBracketCaretPos_PHY.y, &ptColLine.x, &ptColLine.y );
		}

		if ( ( ptColLine.x >= m_nViewLeftCol ) && ( ptColLine.x <= m_nViewLeftCol + m_nViewColNum )
			&& ( ptColLine.y >= m_nViewTopLine ) && ( ptColLine.y <= m_nViewTopLine + m_nViewRowNum ) )
		{	// 表示領域内の場合
			if( !bDraw && m_bDrawSelectArea && ( 0 == IsCurrentPositionSelected( ptColLine ) ) )
			{	// 選択範囲描画済みで消去対象の括弧が選択範囲内の場合
				continue;
			}
			const CLayout* pcLayout;
			int			nLineLen;
			const char*	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( ptColLine.y, &nLineLen, &pcLayout );
			if( pLine )
			{
				EColorIndexType nColorIndex;
				int	OutputX = LineColumnToIndex( pcLayout, ptColLine.x );
				if( bDraw )	{
					nColorIndex = COLORIDX_BRACKET_PAIR;
				}
				else{
					if( IsBracket( pLine, OutputX, m_nCharSize ) ){
						// 03/10/24 ai 折り返し行のColorIndexが正しく取得できない問題に対応
						//nColorIndex = GetColorIndex( hdc, pcLayout, OutputX );
						if( i == 0 ){
							nColorIndex = GetColorIndex( hdc, pcLayout, m_ptBracketPairPos_PHY.x );
						}else{
							nColorIndex = GetColorIndex( hdc, pcLayout, m_ptBracketCaretPos_PHY.x );
						}
					}
					else{
						SetBracketPairPos( false );
						break;
					}
				}
				hFontOld = (HFONT)::SelectObject( hdc, m_pcViewFont->GetFontHan() );
				m_hFontOld = NULL;
				crBackOld = ::SetBkColor(	hdc, TypeDataPtr->m_ColorInfoArr[COLORIDX_TEXT].m_sColorAttr.m_cBACK );
				crTextOld = ::SetTextColor( hdc, TypeDataPtr->m_ColorInfoArr[COLORIDX_TEXT].m_sColorAttr.m_cTEXT );
				SetCurrentColor( hdc, nColorIndex );

				int nHeight = m_nCharHeight + m_pcEditDoc->GetDocumentAttribute().m_nLineSpace;
				int nLeft = (m_nViewAlignLeft - m_nViewLeftCol * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace )) + ptColLine.x * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
				int nTop  = ( ptColLine.y - m_nViewTopLine ) * nHeight + m_nViewAlignTop;

				// 03/03/03 ai カーソルの左に括弧があり括弧が強調表示されている状態でShift+←で選択開始すると
				//             選択範囲内に反転表示されない部分がある問題の修正
				if( ( ptColLine.x == m_ptCaretPos.x ) && ( m_bCaretShowFlag == true ) ){
					HideCaret_( m_hWnd );	// キャレットが一瞬消えるのを防止
					DispText( hdc, nLeft, nTop, &pLine[OutputX], m_nCharSize );
					// 2006.04.30 Moca 対括弧の縦線対応
					DispVerticalLines( hdc, nTop, nTop + nHeight, ptColLine.x, ptColLine.x + m_nCharSize );
					ShowCaret_( m_hWnd );	// キャレットが一瞬消えるのを防止
				}else{
					DispText( hdc, nLeft, nTop, &pLine[OutputX], m_nCharSize );
					// 2006.04.30 Moca 対括弧の縦線対応
					DispVerticalLines( hdc, nTop, nTop + nHeight, ptColLine.x, ptColLine.x + m_nCharSize );
				}

				if( NULL != m_hFontOld ){
					::SelectObject( hdc, m_hFontOld );
					m_hFontOld = NULL;
				}
				::SetTextColor( hdc, crTextOld );
				::SetBkColor( hdc, crBackOld );
				::SelectObject( hdc, hFontOld );

				if( ( m_pcEditWnd->GetActivePane() == m_nMyIndex )
					&& ( ( ptColLine.y == m_ptCaretPos.y ) || ( ptColLine.y - 1 == m_ptCaretPos.y ) ) ){	// 03/02/27 ai 行の間隔が"0"の時にアンダーラインが欠ける事がある為修正
					m_cUnderLine.CaretUnderLineON( true );
				}
			}
		}
	}

	::ReleaseDC( m_hWnd, hdc );

	return;
}

/*! 指定位置のColorIndexの取得
	CEditView::DispLineNewを元にしたためCEditView::DispLineNewに
	修正があった場合は、ここも修正が必要。
*/
EColorIndexType CEditView::GetColorIndex(
		HDC						hdc,
		const CLayout* const	pcLayout,
		int						nCol
)
{
	//	May 9, 2000 genta
	STypeConfig	*TypeDataPtr = &(m_pcEditDoc->GetDocumentAttribute());

	const char*				pLine;	//@@@ 2002.09.22 YAZAKI
	int						nLineLen;
	EColorIndexType			nCOMMENTMODE;
	EColorIndexType			nCOMMENTMODE_OLD;
	int						nCOMMENTEND;
	int						nCOMMENTEND_OLD;
	const CLayout*			pcLayout2;
	EColorIndexType			nColorIndex;

	/* 論理行データの取得 */
	if( NULL != pcLayout ){
		// 2002/2/10 aroka CMemory変更
		nLineLen = pcLayout->m_pCDocLine->m_cLine.GetStringLength();	// 03/10/24 ai 折り返し行のColorIndexが正しく取得できない問題に対応
		pLine = pcLayout->m_pCDocLine->m_cLine.GetStringPtr();			// 03/10/24 ai 折り返し行のColorIndexが正しく取得できない問題に対応

		// 2005.11.20 Moca 色が正しくないことがある問題に対処
		const CLayout* pcLayoutLineFirst = pcLayout;
		// 論理行の最初のレイアウト情報を取得する
		while( 0 != pcLayoutLineFirst->m_ptLogicPos.x ){
			pcLayoutLineFirst = pcLayoutLineFirst->m_pPrev;
		}
		nCOMMENTMODE = pcLayoutLineFirst->m_nTypePrev;
		nCOMMENTEND = 0;
		pcLayout2 = pcLayout;

	}else{
		pLine = NULL;
		nLineLen = 0;
		nCOMMENTMODE = COLORIDX_TEXT; // 2002/03/13 novice
		nCOMMENTEND = 0;
		pcLayout2 = NULL;
	}

	/* 現在の色を指定 */
	nColorIndex = nCOMMENTMODE;	// 02/12/18 ai

	if( NULL != pLine ){

		//@@@ 2001.11.17 add start MIK
		if( TypeDataPtr->m_bUseRegexKeyword )
		{
			m_cRegexKeyword->RegexKeyLineStart();
		}
		//@@@ 2001.11.17 add end MIK

		int			nBgn = 0;
		int			nPos = 0;
		int			nLineBgn =0;
		int			nCharChars = 0;
		BOOL		bSearchStringMode = FALSE;
		BOOL		bSearchFlg = TRUE;	// 2002.02.08 hor
		int			nSearchStart = -1;	// 2002.02.08 hor
		int			nSearchEnd	= -1;	// 2002.02.08 hor
		bool		bKeyWordTop	= true;	//	Keyword Top

		int			nNumLen;
		int			nUrlLen;

//@@@ 2001.11.17 add start MIK
		int			nMatchLen;
		int			nMatchColor;

		while( nPos <= nCol ){	// 03/10/24 ai 行頭のColorIndexが取得できない問題に対応

			nBgn = nPos;
			nLineBgn = nBgn;

			while( nPos - nLineBgn <= nCol ){	// 02/12/18 ai
				/* 検索文字列の色分け */
				if( m_bCurSrchKeyMark	/* 検索文字列のマーク */
				 && TypeDataPtr->m_ColorInfoArr[COLORIDX_SEARCH].m_bDisp ){
searchnext:;
				// 2002.02.08 hor 正規表現の検索文字列マークを少し高速化
					if(!bSearchStringMode && (!m_sCurSearchOption.bRegularExp || (bSearchFlg && nSearchStart < nPos))){
						bSearchFlg=IsSearchString( pLine, nLineLen, nPos, &nSearchStart, &nSearchEnd );
					}
					if( !bSearchStringMode && bSearchFlg && nSearchStart==nPos
					){
						nBgn = nPos;
						bSearchStringMode = TRUE;
						/* 現在の色を指定 */
						nColorIndex = COLORIDX_SEARCH;	// 02/12/18 ai
					}else
					if( bSearchStringMode
					 && nSearchEnd == nPos
					){
						nBgn = nPos;
						/* 現在の色を指定 */
						nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						bSearchStringMode = FALSE;
						goto searchnext;
					}
				}

				if( nPos >= nLineLen - pcLayout2->m_cEol.GetLen() ){
					goto end_of_line;
				}
				SEARCH_START:;
				switch( nCOMMENTMODE ){
				case COLORIDX_TEXT: // 2002/03/13 novice
//@@@ 2001.11.17 add start MIK
					//正規表現キーワード
					if( TypeDataPtr->m_bUseRegexKeyword
					 && m_cRegexKeyword->RegexIsKeyword( pLine, nPos, nLineLen, &nMatchLen, &nMatchColor )
					 /*&& TypeDataPtr->m_ColorInfoArr[nMatchColor].m_bDisp*/ )
					{
						/* 現在の色を指定 */
						nBgn = nPos;
						nCOMMENTMODE = (EColorIndexType)(COLORIDX_REGEX_FIRST + nMatchColor);	/* 色指定 */	//@@@ 2002.01.04 upd
						nCOMMENTEND = nPos + nMatchLen;  /* キーワード文字列の終端をセットする */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
					}
					else
//@@@ 2001.11.17 add end MIK
					//	Mar. 15, 2000 genta
					if( TypeDataPtr->m_ColorInfoArr[COLORIDX_COMMENT].m_bDisp &&
						TypeDataPtr->m_cLineComment.Match( nPos, nLineLen, pLine )	//@@@ 2002.09.22 YAZAKI
					){
						nBgn = nPos;

						nCOMMENTMODE = COLORIDX_COMMENT;	/* 行コメントである */ // 2002/03/13 novice

						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
					}else
					//	Mar. 15, 2000 genta
					if( TypeDataPtr->m_ColorInfoArr[COLORIDX_COMMENT].m_bDisp &&
						TypeDataPtr->m_cBlockComments[0].Match_CommentFrom(nPos, nLineLen, pLine )	//@@@ 2002.09.22 YAZAKI
					){
						nBgn = nPos;
						nCOMMENTMODE = COLORIDX_BLOCK1;	/* ブロックコメント1である */ // 2002/03/13 novice

						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
						/* この物理行にブロックコメントの終端があるか */
						nCOMMENTEND = TypeDataPtr->m_cBlockComments[0].Match_CommentTo(nPos + (int)lstrlen( TypeDataPtr->m_cBlockComments[0].getBlockCommentFrom() ), nLineLen, pLine );	//@@@ 2002.09.22 YAZAKI
					}else
					if( TypeDataPtr->m_ColorInfoArr[COLORIDX_COMMENT].m_bDisp &&
						TypeDataPtr->m_cBlockComments[1].Match_CommentFrom(nPos, nLineLen, pLine )	//@@@ 2002.09.22 YAZAKI
					){
						nBgn = nPos;
						nCOMMENTMODE = COLORIDX_BLOCK2;	/* ブロックコメント2である */ // 2002/03/13 novice
						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
						/* この物理行にブロックコメントの終端があるか */
						nCOMMENTEND = TypeDataPtr->m_cBlockComments[1].Match_CommentTo(nPos + (int)lstrlen( TypeDataPtr->m_cBlockComments[1].getBlockCommentFrom() ), nLineLen, pLine );	//@@@ 2002.09.22 YAZAKI
					}else
					if( pLine[nPos] == '\'' &&
						TypeDataPtr->m_ColorInfoArr[COLORIDX_SSTRING].m_bDisp  /* シングルクォーテーション文字列を表示する */
					){
						nBgn = nPos;
						nCOMMENTMODE = COLORIDX_SSTRING;	/* シングルクォーテーション文字列である */ // 2002/03/13 novice

						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
						/* シングルクォーテーション文字列の終端があるか */
						int i;
						nCOMMENTEND = nLineLen;
						for( i = nPos + 1; i <= nLineLen - 1; ++i ){
							// 2005-09-02 D.S.Koba GetSizeOfChar
							int nCharChars_2 = CMemory::GetSizeOfChar( pLine, nLineLen, i );
							if( 0 == nCharChars_2 ){
								nCharChars_2 = 1;
							}
							if( TypeDataPtr->m_nStringType == 0 ){	/* 文字列区切り記号エスケープ方法 0=[\"][\'] 1=[""][''] */
								if( 1 == nCharChars_2 && pLine[i] == '\\' ){
									++i;
								}else
								if( 1 == nCharChars_2 && pLine[i] == '\'' ){
									nCOMMENTEND = i + 1;
									break;
								}
							}else
							if( TypeDataPtr->m_nStringType == 1 ){	/* 文字列区切り記号エスケープ方法 0=[\"][\'] 1=[""][''] */
								if( 1 == nCharChars_2 && pLine[i] == '\'' ){
									if( i + 1 < nLineLen && pLine[i + 1] == '\'' ){
										++i;
									}else{
										nCOMMENTEND = i + 1;
										break;
									}
								}
							}
							if( 2 == nCharChars_2 ){
								++i;
							}
						}
					}else
					if( pLine[nPos] == '"' &&
						TypeDataPtr->m_ColorInfoArr[COLORIDX_WSTRING].m_bDisp	/* ダブルクォーテーション文字列を表示する */
					){
						nBgn = nPos;
						nCOMMENTMODE = COLORIDX_WSTRING;	/* ダブルクォーテーション文字列である */ // 2002/03/13 novice
						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
						/* ダブルクォーテーション文字列の終端があるか */
						int i;
						nCOMMENTEND = nLineLen;
						for( i = nPos + 1; i <= nLineLen - 1; ++i ){
							// 2005-09-02 D.S.Koba GetSizeOfChar
							int nCharChars_2 = CMemory::GetSizeOfChar( pLine, nLineLen, i );
							if( 0 == nCharChars_2 ){
								nCharChars_2 = 1;
							}
							if( TypeDataPtr->m_nStringType == 0 ){	/* 文字列区切り記号エスケープ方法 0=[\"][\'] 1=[""][''] */
								if( 1 == nCharChars_2 && pLine[i] == '\\' ){
									++i;
								}else
								if( 1 == nCharChars_2 && pLine[i] == '"' ){
									nCOMMENTEND = i + 1;
									break;
								}
							}else
							if( TypeDataPtr->m_nStringType == 1 ){	/* 文字列区切り記号エスケープ方法 0=[\"][\'] 1=[""][''] */
								if( 1 == nCharChars_2 && pLine[i] == '"' ){
									if( i + 1 < nLineLen && pLine[i + 1] == '"' ){
										++i;
									}else{
										nCOMMENTEND = i + 1;
										break;
									}
								}
							}
							if( 2 == nCharChars_2 ){
								++i;
							}
						}
					}else
					if( bKeyWordTop && TypeDataPtr->m_ColorInfoArr[COLORIDX_URL].m_bDisp			/* URLを表示する */
					 && ( FALSE != IsURL( &pLine[nPos], nLineLen - nPos, &nUrlLen ) )	/* 指定アドレスがURLの先頭ならばTRUEとその長さを返す */
					){
						nBgn = nPos;
						nCOMMENTMODE = COLORIDX_URL;	/* URLモード */ // 2002/03/13 novice
						nCOMMENTEND = nPos + nUrlLen;
						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
//@@@ 2001.02.17 Start by MIK: 半角数値を強調表示
					}else if( bKeyWordTop && TypeDataPtr->m_ColorInfoArr[COLORIDX_DIGIT].m_bDisp
						&& (nNumLen = IsNumber( pLine, nPos, nLineLen )) > 0 )		/* 半角数字を表示する */
					{
						/* キーワード文字列の終端をセットする */
						nNumLen = nPos + nNumLen;
						/* 現在の色を指定 */
						nBgn = nPos;
						nCOMMENTMODE = COLORIDX_DIGIT;	/* 半角数値である */ // 2002/03/13 novice
						nCOMMENTEND = nNumLen;
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
//@@@ 2001.02.17 End by MIK: 半角数値を強調表示
					}else
					if( bKeyWordTop && TypeDataPtr->m_nKeyWordSetIdx[0] != -1 && /* キーワードセット */
						TypeDataPtr->m_ColorInfoArr[COLORIDX_KEYWORD1].m_bDisp &&  /* 強調キーワードを表示する */ // 2002/03/13 novice
						IS_KEYWORD_CHAR( pLine[nPos] )
					){
						/* キーワード文字列の終端を探す */
						int nKeyEnd;
						for( nKeyEnd = nPos + 1; nKeyEnd <= nLineLen - 1; ++nKeyEnd ){
							if( !IS_KEYWORD_CHAR( pLine[nKeyEnd] ) ){
								break;
							}
						}
						int nKeyLen = nKeyEnd - nPos;

						/* キーワードが登録単語ならば、色を変える */
						// 2005.01.13 MIK 強調キーワード数追加に伴う配列化 //MIK 2000.12.01 second keyword & binary search
						for( int n = 0; n < MAX_KEYWORDSET_PER_TYPE; n++ )
						{
							// 強調キーワードは前詰めで設定されるので、未設定のIndexがあれば中断
							if(TypeDataPtr->m_nKeyWordSetIdx[n] == -1 ){
									break;
							}
							else if(TypeDataPtr->m_ColorInfoArr[COLORIDX_KEYWORD1 + n].m_bDisp)
							{
								/* ｎ番目のセットから指定キーワードをサーチ 無いときは-1を返す */
								int nIdx = m_pShareData->m_Common.m_sSpecialKeyword.m_CKeyWordSetMgr.SearchKeyWord2(
									TypeDataPtr->m_nKeyWordSetIdx[n],
									&pLine[nPos],
									nKeyLen
								);
								if( nIdx >= 0 ){
									/* 現在の色を指定 */
									nBgn = nPos;
									nCOMMENTMODE = (EColorIndexType)(COLORIDX_KEYWORD1 + n);
									nCOMMENTEND = nKeyEnd;
									if( !bSearchStringMode ){
										nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
									}
									break;
								}
							}
						}
						//MIK END
					}
					//	From Here Mar. 4, 2001 genta
					if( IS_KEYWORD_CHAR( pLine[nPos] ))	bKeyWordTop = false;
					else								bKeyWordTop = true;
					//	To Here
					break;
// 2002/03/13 novice
				case COLORIDX_URL:		/* URLモードである */
				case COLORIDX_KEYWORD1:	/* 強調キーワード1 */
				case COLORIDX_DIGIT:	/* 半角数値である */  //@@@ 2001.02.17 by MIK
				case COLORIDX_KEYWORD2:	/* 強調キーワード2 */	//MIK
				case COLORIDX_KEYWORD3:
				case COLORIDX_KEYWORD4:
				case COLORIDX_KEYWORD5:
				case COLORIDX_KEYWORD6:
				case COLORIDX_KEYWORD7:
				case COLORIDX_KEYWORD8:
				case COLORIDX_KEYWORD9:
				case COLORIDX_KEYWORD10:
					if( nPos == nCOMMENTEND ){
						nBgn = nPos;
						nCOMMENTMODE = COLORIDX_TEXT; // 2002/03/13 novice
						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
						goto SEARCH_START;
					}
					break;
				case COLORIDX_CTRLCODE:	/* コントロールコード */ // 2002/03/13 novice
					if( nPos == nCOMMENTEND ){
						nBgn = nPos;
						nCOMMENTMODE = nCOMMENTMODE_OLD;
						nCOMMENTEND = nCOMMENTEND_OLD;
						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
						goto SEARCH_START;
					}
					break;

				case COLORIDX_COMMENT:	/* 行コメントである */ // 2002/03/13 novice
					break;
				case COLORIDX_BLOCK1:	/* ブロックコメント1である */ // 2002/03/13 novice
					if( 0 == nCOMMENTEND ){
						/* この物理行にブロックコメントの終端があるか */
						nCOMMENTEND = TypeDataPtr->m_cBlockComments[0].Match_CommentTo(nPos, nLineLen, pLine );	//@@@ 2002.09.22 YAZAKI
					}else
					if( nPos == nCOMMENTEND ){
						nBgn = nPos;
						nCOMMENTMODE = COLORIDX_TEXT; // 2002/03/13 novice
						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
						goto SEARCH_START;
					}
					break;
				case COLORIDX_BLOCK2:	/* ブロックコメント2である */ // 2002/03/13 novice
					if( 0 == nCOMMENTEND ){
						/* この物理行にブロックコメントの終端があるか */
						nCOMMENTEND = TypeDataPtr->m_cBlockComments[1].Match_CommentTo(nPos, nLineLen, pLine );	//@@@ 2002.09.22 YAZAKI
					}else
					if( nPos == nCOMMENTEND ){
						nBgn = nPos;
						nCOMMENTMODE = COLORIDX_TEXT; // 2002/03/13 novice
						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
						goto SEARCH_START;
					}
					break;
				case COLORIDX_SSTRING:	/* シングルクォーテーション文字列である */ // 2002/03/13 novice
					if( 0 == nCOMMENTEND ){
						/* シングルクォーテーション文字列の終端があるか */
						int i;
						nCOMMENTEND = nLineLen;
						for( i = nPos/* + 1*/; i <= nLineLen - 1; ++i ){
							// 2005-09-02 D.S.Koba GetSizeOfChar
							int nCharChars_2 = CMemory::GetSizeOfChar( pLine, nLineLen, i );
							if( 0 == nCharChars_2 ){
								nCharChars_2 = 1;
							}
							if( TypeDataPtr->m_nStringType == 0 ){	/* 文字列区切り記号エスケープ方法 0=[\"][\'] 1=[""][''] */
								if( 1 == nCharChars_2 && pLine[i] == '\\' ){
									++i;
								}else
								if( 1 == nCharChars_2 && pLine[i] == '\'' ){
									nCOMMENTEND = i + 1;
									break;
								}
							}else
							if( TypeDataPtr->m_nStringType == 1 ){	/* 文字列区切り記号エスケープ方法 0=[\"][\'] 1=[""][''] */
								if( 1 == nCharChars_2 && pLine[i] == '\'' ){
									if( i + 1 < nLineLen && pLine[i + 1] == '\'' ){
										++i;
									}else{
										nCOMMENTEND = i + 1;
										break;
									}
								}
							}
							if( 2 == nCharChars_2 ){
								++i;
							}
						}
					}else
					if( nPos == nCOMMENTEND ){
						nBgn = nPos;
						nCOMMENTMODE = COLORIDX_TEXT; // 2002/03/13 novice
						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
						goto SEARCH_START;
					}
					break;
				case COLORIDX_WSTRING:	/* ダブルクォーテーション文字列である */ // 2002/03/13 novice
					if( 0 == nCOMMENTEND ){
						/* ダブルクォーテーション文字列の終端があるか */
						int i;
						nCOMMENTEND = nLineLen;
						for( i = nPos/* + 1*/; i <= nLineLen - 1; ++i ){
							// 2005-09-02 D.S.Koba GetSizeOfChar
							int nCharChars_2 = CMemory::GetSizeOfChar( pLine, nLineLen, i );
							if( 0 == nCharChars_2 ){
								nCharChars_2 = 1;
							}
							if( TypeDataPtr->m_nStringType == 0 ){	/* 文字列区切り記号エスケープ方法 0=[\"][\'] 1=[""][''] */
								if( 1 == nCharChars_2 && pLine[i] == '\\' ){
									++i;
								}else
								if( 1 == nCharChars_2 && pLine[i] == '"' ){
									nCOMMENTEND = i + 1;
									break;
								}
							}else
							if( TypeDataPtr->m_nStringType == 1 ){	/* 文字列区切り記号エスケープ方法 0=[\"][\'] 1=[""][''] */
								if( 1 == nCharChars_2 && pLine[i] == '"' ){
									if( i + 1 < nLineLen && pLine[i + 1] == '"' ){
										++i;
									}else{
										nCOMMENTEND = i + 1;
										break;
									}
								}
							}
							if( 2 == nCharChars_2 ){
								++i;
							}
						}
					}else
					if( nPos == nCOMMENTEND ){
						nBgn = nPos;
						nCOMMENTMODE = COLORIDX_TEXT; // 2002/03/13 novice
						/* 現在の色を指定 */
						if( !bSearchStringMode ){
							nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
						}
						goto SEARCH_START;
					}
					break;
				default:	//@@@ 2002.01.04 add start
					if( nCOMMENTMODE & COLORIDX_REGEX_BIT ){	//正規表現キーワード1〜10
						if( nPos == nCOMMENTEND ){
							nBgn = nPos;
							nCOMMENTMODE = COLORIDX_TEXT; // 2002/03/13 novice
							/* 現在の色を指定 */
							if( !bSearchStringMode ){
								nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
							}
							goto SEARCH_START;
						}
					}
					break;	//@@@ 2002.01.04 add end
				}
				if( pLine[nPos] == TAB ){
					nBgn = nPos + 1;
					nCharChars = 1;
				}else
				if( (unsigned char)pLine[nPos] == 0x81 && (unsigned char)pLine[nPos + 1] == 0x40	//@@@ 2001.11.17 upd MIK
				 && !(nCOMMENTMODE & COLORIDX_REGEX_BIT) )	//@@@ 2002.01.04
				{	//@@@ 2001.11.17 add MIK	//@@@ 2002.01.04
					nBgn = nPos + 2;
					nCharChars = 2;
				}
				//半角空白（半角スペース）を表示 2002.04.28 Add by KK 
				else if (pLine[nPos] == ' ' && TypeDataPtr->m_ColorInfoArr[COLORIDX_SPACE].m_bDisp 
				 && !(nCOMMENTMODE & COLORIDX_REGEX_BIT) )
				{
					nBgn = nPos + 1;
					nCharChars = 1;
				}
				else{
					// 2005-09-02 D.S.Koba GetSizeOfChar
					nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, nPos );
					if( 0 == nCharChars ){
						nCharChars = 1;
					}
					if( !bSearchStringMode
					 && 1 == nCharChars
					 && COLORIDX_CTRLCODE != nCOMMENTMODE // 2002/03/13 novice
					 && TypeDataPtr->m_ColorInfoArr[COLORIDX_CTRLCODE].m_bDisp	/* コントロールコードを色分け */
					 &&	(
								//	Jan. 23, 2002 genta 警告抑制
							( (unsigned char)pLine[nPos] <= (unsigned char)0x1F ) ||
							( (unsigned char)'~' < (unsigned char)pLine[nPos] && (unsigned char)pLine[nPos] < (unsigned char)'｡' ) ||
							( (unsigned char)'ﾟ' < (unsigned char)pLine[nPos] )
						)
					 && pLine[nPos] != TAB && pLine[nPos] != CR && pLine[nPos] != LF
					){
						nBgn = nPos;
						nCOMMENTMODE_OLD = nCOMMENTMODE;
						nCOMMENTEND_OLD = nCOMMENTEND;
						nCOMMENTMODE = COLORIDX_CTRLCODE;	/* コントロールコード モード */ // 2002/03/13 novice
						/* コントロールコード列の終端を探す */
						int nCtrlEnd;
						for( nCtrlEnd = nPos + 1; nCtrlEnd <= nLineLen - 1; ++nCtrlEnd ){
							// 2005-09-02 D.S.Koba GetSizeOfChar
							int nCharChars_2 = CMemory::GetSizeOfChar( pLine, nLineLen, nCtrlEnd );
							if( 0 == nCharChars_2 ){
								nCharChars_2 = 1;
							}
							if( nCharChars_2 != 1 ){
								break;
							}
							if( (
								//	Jan. 23, 2002 genta 警告抑制
								( (unsigned char)pLine[nCtrlEnd] <= (unsigned char)0x1F ) ||
									( (unsigned char)'~' < (unsigned char)pLine[nCtrlEnd] && (unsigned char)pLine[nCtrlEnd] < (unsigned char)'｡' ) ||
									( (unsigned char)'ﾟ' < (unsigned char)pLine[nCtrlEnd] )
								) &&
								pLine[nCtrlEnd] != TAB && pLine[nCtrlEnd] != CR && pLine[nCtrlEnd] != LF
							){
							}else{
								break;
							}
						}
						nCOMMENTEND = nCtrlEnd;
						/* 現在の色を指定 */
						nColorIndex = nCOMMENTMODE;	// 02/12/18 ai
					}
				}
				nPos+= nCharChars;
			} //end of while( nPos - nLineBgn < pcLayout2->m_nLength ){
			if( nPos > nCol ){	// 03/10/24 ai 行頭のColorIndexが取得できない問題に対応
				break;
			}
		}

end_of_line:;

	}

//@end_of_func:;
	return nColorIndex;
}

/*!	挿入モード取得

	@date 2005.10.02 genta 管理方法変更のため関数化
*/
bool CEditView::IsInsMode(void) const
{
	return m_pcEditDoc->IsInsMode();
}

void CEditView::SetInsMode(bool mode)
{
	m_pcEditDoc->SetInsMode( mode );
}

/*! アンドゥバッファの処理 */
void CEditView::SetUndoBuffer( bool bPaintLineNumber )
{
	if( NULL != m_pcOpeBlk && m_pcOpeBlk->Release() == 0 ){
		if( 0 < m_pcOpeBlk->GetNum() ){	/* 操作の数を返す */
			/* 操作の追加 */
			m_pcEditDoc->m_cOpeBuf.AppendOpeBlk( m_pcOpeBlk );

			if( bPaintLineNumber
			 && m_pcEditDoc->m_cOpeBuf.GetCurrentPointer() == 1 )	// 全Undo状態からの変更か？	// 2009.03.26 ryoji
				RedrawLineNumber();	// 自ペインの行番号（変更行）表示を更新 ← 変更行のみの表示更新で済ませている場合があるため

			if( !m_pcEditWnd->UpdateTextWrap() )	// 折り返し方法関連の更新	// 2008.06.10 ryoji
				m_pcEditWnd->RedrawAllViews( this );	//	他のペインの表示を更新
		}
		else{
			delete m_pcOpeBlk;
		}
		m_pcOpeBlk = NULL;
	}
}

/*[EOF]*/
