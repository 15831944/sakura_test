//	$Id$
/*!	@file
	@brief キーワード補完

	@author Norio Nakatani
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001, asa-o

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/
#include "CHokanMgr.h"
#include "debug.h"
#include "CKeyBind.h"
#include "Keycode.h"
#include "CDicMgr.h"
#include "CEditView.h"
#include "mymessage.h"
#include "sakura_rc.h"
#include "etc_uty.h"

	WNDPROC			gm_wpHokanListProc;


LRESULT APIENTRY HokanList_SubclassProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
//#ifdef _DEBUG
//	MYTRACE( "HokanList_SubclassProc() uMsg == %xh\n", uMsg );
//#endif
	// Modified by KEITA for WIN64 2003.9.6
	CDialog* pCDialog = ( CDialog* )::GetWindowLongPtr( ::GetParent( hwnd ), DWLP_USER );
	CHokanMgr* pCHokanMgr = (CHokanMgr*)::GetWindowLongPtr( ::GetParent( hwnd ), DWLP_USER );
//	WORD vkey;
//	WORD nCaretPos;
//	LPARAM hwndLB;
//	int i;
	MSG* pMsg;
	int nVKey;
//	LPARAM lKeyData;
	switch( uMsg ){
	case WM_KEYDOWN:
		nVKey = (int) wParam;	// virtual-key code
//		lKeyData = lParam;			// key data
//		MYTRACE( "WM_KEYDOWN nVKey = %xh\n", nVKey );
		/* キー操作を偽造しよう */
		if (nVKey == VK_SPACE){	//	Space
			int nIdx = 0;
			if( (SHORT)0x8000 & ::GetKeyState( VK_SHIFT	  ) ) nIdx |= _SHIFT;
			if( (SHORT)0x8000 & ::GetKeyState( VK_CONTROL ) ) nIdx |= _CTRL;
			if( (SHORT)0x8000 & ::GetKeyState( VK_MENU    ) ) nIdx |= _ALT;
			if (nIdx == _SHIFT){
				//	Shift + Spaceで↑を偽造
				wParam = VK_UP;
			}
			else if (nIdx == 0) {
				//	Spaceのみで↓を偽造
				wParam = VK_DOWN;
			}
		}
		/* 補完実行キーなら補完する */
		if( -1 != pCHokanMgr->KeyProc( wParam, lParam ) ){
			/* キーストロークを親に転送 */
			::EnableWindow( ::GetParent( ::GetParent( pCDialog->m_hwndParent ) ), TRUE );
//			::ShowWindow( pCDialog->m_hWnd, SW_HIDE );
//			::SetFocus( ::GetParent( ::GetParent( pCDialog->m_hwndParent ) ) );
			return ::PostMessage( ::GetParent( ::GetParent( pCDialog->m_hwndParent ) ), uMsg, wParam, lParam );
		}
		break;
	case WM_GETDLGCODE:
		pMsg = (MSG*) lParam; // pointer to an MSG structure
		if( NULL == pMsg ){
//			MYTRACE( "WM_GETDLGCODE  pMsg==NULL\n" );
			return 0;
		}
//		MYTRACE( "WM_GETDLGCODE  pMsg->message = %xh\n", pMsg->message );
		return DLGC_WANTALLKEYS;/* すべてのキーストロークを私に下さい */	//	Sept. 17, 2000 jepro 説明の「全て」を「すべて」に統一
	}
	return CallWindowProc( (WNDPROC)gm_wpHokanListProc, hwnd, uMsg, wParam, lParam);
}



CHokanMgr::CHokanMgr()
{
//	m_hFont = NULL;
//	m_hFontOld = NULL;
	m_cmemCurWord.SetDataSz( "" );

	m_pcmemKouho = NULL;
	m_nKouhoNum = 0;;
	m_nCurKouhoIdx = -1;
	m_pszCurKouho = NULL;
//	m_pcEditView = NULL;
	m_bTimerFlag = TRUE;
//	hm_wpHokanListProc = FALSE;
	return;
}

CHokanMgr::~CHokanMgr()
{
	if( NULL != m_pcmemKouho ){
		delete m_pcmemKouho;
		m_pcmemKouho = NULL;
	}
	m_nKouhoNum = 0;;
	if( NULL != m_pszCurKouho ){
		delete [] m_pszCurKouho;
		m_pszCurKouho = NULL;
	}

}

/* モードレスダイアログの表示 */
HWND CHokanMgr::DoModeless( HINSTANCE hInstance , HWND hwndParent, LPARAM lParam )
{
	HWND hwndWork;
	hwndWork = CDialog::DoModeless( hInstance, hwndParent, IDD_HOKAN, lParam, SW_HIDE );
	OnSize( 0, 0 );
	/* リストをフック */
	// Modified by KEITA for WIN64 2003.9.6
	::gm_wpHokanListProc = (WNDPROC) ::SetWindowLongPtr( ::GetDlgItem( m_hWnd, IDC_LIST_WORDS ), GWLP_WNDPROC, (LONG_PTR)HokanList_SubclassProc  );

	::ShowWindow( m_hWnd, SW_HIDE );
	return hwndWork;
}

/* モードレス時：対象となるビューの変更 */
void CHokanMgr::ChangeView( LPARAM pcEditView )
{
	m_lParam = pcEditView;
	return;
}

void CHokanMgr::Hide( void )
{
	CEditView* pcEditView;
	::EnableWindow( ::GetParent( ::GetParent( m_hwndParent ) ), TRUE );
//	::DestroyWindow( hwnd );
	::ShowWindow( m_hWnd, SW_HIDE );
	m_nCurKouhoIdx = -1;
	/* 入力フォーカスを受け取ったときの処理 */
	pcEditView = (CEditView*)m_lParam;
	pcEditView->OnSetFocus();
//	m_nCurKouhoIdx = -1;
	return;

}

/*!	初期化
	pcmemHokanWord == NULLのとき、補完候補がひとつだったら、補完ウィンドウを表示しないで終了します。
	Search()呼び出し元で確定処理を進めてください。

	@date 2002.2.17 YAZAKI CShareDataのインスタンスは、CProcessにひとつあるのみ。
*/
int CHokanMgr::Search(
//	HFONT		hFont,
	POINT*		ppoWin,
	int			nWinHeight,
	int			nColmWidth,
	const char*	pszCurWord,
	const char* pszHokanFile,
	int			bHokanLoHiCase,	// 入力補完機能：英大文字小文字を同一視する 2001/06/19 asa-o
	BOOL		bHokanByFile,	// 編集中データから候補を探す 2003.06.23 Moca
	CMemory*	pcmemHokanWord	// 2001/06/19 asa-o
)
{
	CEditView* pcEditView = (CEditView*)m_lParam;
	/*
	||  補完キーワードの検索
	||
	||  ・見つかった候補をすべて返す(改行で区切って返す)
	||  ・指定された候補の最大数を超えると処理を中断する
	||  ・見つかった数を返す
	||
	*/
	if( NULL != m_pcmemKouho ){
		delete m_pcmemKouho;
		m_pcmemKouho = NULL;
	}
	m_nKouhoNum = CDicMgr::HokanSearch(
		pszCurWord,
//		m_pShareData->m_Common.m_bHokanLoHiCase,	/* 入力補完機能：英大文字小文字を同一視する */
		bHokanLoHiCase,								// 引数からに変更	2001/06/19 asa-o
		&m_pcmemKouho,
		0, //Max候補数
		pszHokanFile
	);

	// 2003.05.16 Moca 追加 編集中データ内から候補を探す
	if( bHokanByFile ){
		m_nKouhoNum = pcEditView->HokanSearchByFile(
			pszCurWord,
			bHokanLoHiCase,
			&m_pcmemKouho,
			m_nKouhoNum,
			1024 // 編集中データからなので数を制限しておく
		);
	}
	if( 0 == m_nKouhoNum ){
		m_nCurKouhoIdx = -1;
		return 0;
	}

//	2001/06/19 asa-o 候補が１つの場合補完ウィンドウは表示しない(逐次補完の場合は除く)
	if( 1 == m_nKouhoNum ){
		if(pcmemHokanWord != NULL){
			m_nCurKouhoIdx = -1;
			pcmemHokanWord->SetDataSz(m_pcmemKouho->GetPtr());
			return 1;
		}
	}

//	LOGFONT		lf;
//	HDC			hdc;
//	WNDCLASS	wc;
//	ATOM		atom;
	int			i;

	/* 共有データ構造体のアドレスを返す */
	m_pShareData = CShareData::getInstance()->GetShareData();

//	m_hFont = hFont;
	m_poWin.x = ppoWin->x;
	m_poWin.y = ppoWin->y;
	m_nWinHeight = nWinHeight;
	m_nColmWidth = nColmWidth;
//	m_cmemCurWord.SetData( pszCurWord, lstrlen( pszCurWord ) );
	m_cmemCurWord.SetDataSz( pszCurWord );


	m_nCurKouhoIdx = 0;
//	SetCurKouhoStr();



//	::ShowWindow( m_hWnd, SW_SHOWNA );







	HWND hwndList;
	hwndList = ::GetDlgItem( m_hWnd, IDC_LIST_WORDS );
	::SendMessage( hwndList, LB_RESETCONTENT, 0, 0 );
	char*	pszCR = "\n";
	char*	pszWork;
	char*	pszNext;
	char*	pszTest;
	pszWork = m_pcmemKouho->GetPtr();
	for( i = 0; i < m_nKouhoNum; ++i ){
		pszNext = strstr( pszWork, pszCR );
		if( NULL == pszNext ){
			break;
		}
		pszTest = new char[pszNext - pszWork + 1];
		memcpy( pszTest, pszWork, pszNext - pszWork );
		pszTest[pszNext - pszWork] = '\0';

		::SendMessage( hwndList, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) pszTest );

		delete [] pszTest;
		pszTest = NULL;
		pszWork = pszNext + lstrlen( pszCR );
	}
	::SendMessage( hwndList, LB_SETCURSEL, 0, 0 );


//? この処理はやらないほうが見やすいかも?
//?	/* リストの全アイテム数がリストに収まる時は、リストのサイズを小さくする */
//?	int nItemHeight;
//?	nItemHeight = ::SendMessage( hwndList, LB_GETITEMHEIGHT, 0, 0 );
//?	if( ( m_nKouhoNum + 1 ) * nItemHeight < m_nHeight ){
//?		m_nHeight = ( m_nKouhoNum + 1 ) * nItemHeight;
//?	}else{
//?		if( m_nHeight < 1000 ){
//?			m_nHeight = 1000;
//?		}
//?	}
//?	if( ( m_nKouhoNum + 1 ) * nItemHeight < m_nHeight ){
//?		m_nHeight = ( m_nKouhoNum + 1 ) * nItemHeight;
//?	}
//?


//@@	::EnableWindow( ::GetParent( ::GetParent( m_hwndParent ) ), FALSE );


	int nX;
	int nY;
	int nCX;
	int nCY;
	RECT	rcDesktop;
	//	May 01, 2004 genta マルチモニタ対応
	::GetMonitorWorkRect( m_hWnd, &rcDesktop );

	nX = m_poWin.x - m_nColmWidth;
	nY = m_poWin.y + m_nWinHeight + 4;
	nCX = m_nWidth;
	nCY = m_nHeight;

	/* 下に入るなら */
	if( nY + nCY < rcDesktop.bottom ){
		/* 何もしない */
	}else
	/* 上に入るなら */
	if( rcDesktop.top < m_poWin.y - m_nHeight - 4 ){
		/* 上に出す */
		nY = m_poWin.y - m_nHeight - 4;
	}else
	/* 上に出すか下に出すか(広いほうに出す) */
	if(	rcDesktop.bottom - nY > m_poWin.y ){
		/* 下に出す */
//		m_nHeight = rcDesktop.bottom - nY;
		nCY = rcDesktop.bottom - nY;
	}else{
		/* 上に出す */
		nY = rcDesktop.top;
		nCY = m_poWin.y - 4 - rcDesktop.top;
	}

//	2001/06/19 Start by asa-o: 表示位置補正

	// 右に入る
	if(nX + nCX < rcDesktop.right ){
		// そのまま
	}else
	// 左に入る
	if(rcDesktop.left < nX - nCX + 8){
		// 左に表示
		nX -= nCX - 8;
	}else{
		// サイズを調整して右に表示
		nCX = max((int)(rcDesktop.right - nX) , 100);	// 最低サイズを100くらいに
	}

//	2001/06/19 End

//	2001/06/18 Start by asa-o: 補正後の位置・サイズを保存
	m_poWin.x = nX;
	m_poWin.y = nY;
	m_nHeight = nCY;
	m_nWidth = nCX;
//	2001/06/18 End

	/* はみ出すなら小さくする */
//	if( rcDesktop.bottom < nY + nCY ){
//		/* 下にはみ出す */
//		if( m_poWin.y - 4 - nCY < 0 ){
//			/* 上にはみ出す */
//			/* →高さだけ調節 */
//			nCY = rcDesktop.bottom - nY - 4;
//		}else{
//
//		}
//
//	}
	::MoveWindow( m_hWnd, nX, nY, nCX, nCY, TRUE );
	::ShowWindow( m_hWnd, SW_SHOW );
//	::ShowWindow( m_hWnd, SW_SHOWNA );
	::SetFocus( m_hWnd );
//	::SetFocus( ::GetDlgItem( m_hWnd, IDC_LIST_WORDS ) );
//	::SetFocus( ::GetParent( ::GetParent( m_hwndParent ) ) );


//	2001/06/18 asa-o:
	ShowTip();	// 補完ウィンドウで選択中の単語にキーワードヘルプを表示

//	2003.06.25 Moca 他のメソッドで使っていないので、とりあえず削除しておく
	if( NULL != m_pcmemKouho ){
		delete m_pcmemKouho;
		m_pcmemKouho = NULL;
	}
	return m_nKouhoNum;
}


#if 0
///// 2003.06.25　未使用のようだ。Moca
void CHokanMgr::SetCurKouhoStr( void )
{
	char*	pszCR = "\n";
	int		i;
	char*	pszWork;
	char*	pszNext;
	char	szAdd[64];

	if( NULL != m_pszCurKouho ){
		delete [] m_pszCurKouho;
		m_pszCurKouho = NULL;
	}
	if( m_nCurKouhoIdx >= m_nKouhoNum ){
		return;
	}
	strcpy( szAdd, "" );
//	sprintf( szAdd, " (%d/%d)", m_nCurKouhoIdx + 1, m_nKouhoNum );
	pszWork = m_pcmemKouho->GetPtr();
	for( i = 0; i <= m_nCurKouhoIdx; ++i ){
		pszNext = strstr( pszWork, pszCR );
		if( NULL == pszNext ){
			return;
		}
		if( i == m_nCurKouhoIdx ){
			pszWork += m_cmemCurWord.GetLength();
			m_pszCurKouho = new char[pszNext - pszWork + 1 + lstrlen( szAdd )];
			memcpy( m_pszCurKouho, pszWork, pszNext - pszWork );
			m_pszCurKouho[pszNext - pszWork] = '\0';
			strcat( m_pszCurKouho, szAdd );
			::MoveWindow( m_hWnd,
				m_poWin.x - m_nColmWidth/*+ m_cmemCurWord.GetLength() * m_nColmWidth*/,
				m_poWin.y + m_nWinHeight,
//				m_nColmWidth * lstrlen(m_pszCurKouho) + 2,
//				m_nWinHeight + 2 + 8,
				220,
				180,
				TRUE
			);
			OnSize( 0, 0 );

			return;
		}
		pszWork = pszNext + lstrlen( pszCR );
	}
	return;

}
#endif


BOOL CHokanMgr::OnInitDialog( HWND hwndDlg, WPARAM wParam, LPARAM lParam )
{
	m_hWnd = hwndDlg;
	/* 基底クラスメンバ */
//-	CreateSizeBox();
	return CDialog::OnInitDialog( hwndDlg, wParam, lParam );

}

BOOL CHokanMgr::OnDestroy( void )
{
	/* 基底クラスメンバ */
	CreateSizeBox();
	return CDialog::OnDestroy();


}


BOOL CHokanMgr::OnSize( WPARAM wParam, LPARAM lParam )
{
	/* 基底クラスメンバ */
	CDialog::OnSize( wParam, lParam );

	int	Controls[] = {
		IDC_LIST_WORDS
	};
	int		nControls = sizeof( Controls ) / sizeof( Controls[0] );
	int		fwSizeType;
	int		nWidth;
	int		nHeight;
	int		i;
	int		nWork;
	RECT	rc;
	HWND	hwndCtrl;
	POINT	po;
	RECT	rcDlg;

	fwSizeType = wParam;	// resizing flag

	::GetClientRect( m_hWnd, &rcDlg );
	nWidth = rcDlg.right - rcDlg.left;  // width of client area
	nHeight = rcDlg.bottom - rcDlg.top; // height of client area

//	2001/06/18 Start by asa-o: サイズ変更後の位置を保存
	m_poWin.x = rcDlg.left - 4;
	m_poWin.y = rcDlg.top - 3;
	::ClientToScreen(m_hWnd,&m_poWin);
//	2001/06/18 End

	nWork = 48;
	for ( i = 0; i < nControls; ++i ){
		hwndCtrl = ::GetDlgItem( m_hWnd, Controls[i] );
		::GetWindowRect( hwndCtrl, &rc );
		po.x = rc.left;
		po.y = rc.top;
		::ScreenToClient( m_hWnd, &po );
		rc.left = po.x;
		rc.top  = po.y;
		po.x = rc.right;
		po.y = rc.bottom;
		::ScreenToClient( m_hWnd, &po );
		rc.right = po.x;
		rc.bottom  = po.y;
		if( Controls[i] == IDC_LIST_WORDS ){
			::SetWindowPos(
				hwndCtrl,
				NULL,
				rc.left,
				rc.top,
				nWidth - rc.left * 2,
				nHeight - rc.top * 2/* - 20*/,
				SWP_NOOWNERZORDER | SWP_NOZORDER
			);
		}
	}

//	2001/06/18 asa-o:
	ShowTip();	// 補完ウィンドウで選択中の単語にキーワードヘルプを表示

	return TRUE;

}

BOOL CHokanMgr::OnBnClicked( int wID )
{
	switch( wID ){
	case IDCANCEL:
//		CloseDialog( 0 );
		Hide();
		return TRUE;
	case IDOK:
//		CloseDialog( 0 );
		/* 補完実行 */
		DoHokan( VK_RETURN );
		return TRUE;
	}
	return FALSE;
	return 0;
}



BOOL CHokanMgr::OnKeyDown( WPARAM wParam, LPARAM lParam )
{
	int nVKey;
	nVKey = (int) wParam;	// virtual-key code
//	lKeyData = lParam;			// key data
	switch( nVKey ){
	case VK_HOME:
	case VK_END:
	case VK_UP:
	case VK_DOWN:
	case VK_PRIOR:
	case VK_NEXT:
		return 1;
	}
 	return 0;
}


BOOL CHokanMgr::OnCbnSelChange( HWND hwndCtl, int wID )
{
//	2001/06/18 asa-o:
	ShowTip();	// 補完ウィンドウで選択中の単語にキーワードヘルプを表示
	return TRUE;
}

BOOL CHokanMgr::OnLbnDblclk( int wID )
{
	/* 補完実行 */
	DoHokan( 0 );
	return TRUE;

}


BOOL CHokanMgr::OnKillFocus( WPARAM wParam, LPARAM lParam )
{
//	Hide();
	return TRUE;
}



/* 補完実行 */
BOOL CHokanMgr::DoHokan( int nVKey )
{
#ifdef _DEBUG
	MYTRACE( "CHokanMgr::DoHokan( nVKey==%xh )\n", nVKey );
#endif
	/* 補完候補決定キー */
	if( VK_RETURN	== nVKey && FALSE == m_pShareData->m_Common.m_bHokanKey_RETURN )	return FALSE;/* VK_RETURN 補完決定キーが有効/無効 */
	if( VK_TAB		== nVKey && FALSE == m_pShareData->m_Common.m_bHokanKey_TAB ) 		return FALSE;/* VK_TAB    補完決定キーが有効/無効 */
	if( VK_RIGHT	== nVKey && FALSE == m_pShareData->m_Common.m_bHokanKey_RIGHT )		return FALSE;/* VK_RIGHT  補完決定キーが有効/無効 */
#if 0
	if( VK_SPACE	== nVKey && FALSE == m_pShareData->m_Common.m_bHokanKey_SPACE )		return FALSE;/* VK_SPACE  補完決定キーが有効/無効 */
#endif

	HWND hwndList;
	int nItem;
	char szLabel[1024];
	CEditView* pcEditView;
	hwndList = ::GetDlgItem( m_hWnd, IDC_LIST_WORDS );
	nItem = ::SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
	if( LB_ERR == nItem ){
		return FALSE;
	}
	::SendMessage( hwndList, LB_GETTEXT, nItem, (LPARAM)szLabel );
#if 0
	/* スペースで候補決定の場合はスペースをつける */
	if( VK_SPACE == nVKey ){
		strcat( szLabel, " " );
	}
#endif
//	pszWork += m_cmemCurWord.GetLength();

 	/* テキストを貼り付け */
	pcEditView = (CEditView*)m_lParam;
	//	Apr. 28, 2000 genta
	pcEditView->HandleCommand( F_WordDeleteToStart, FALSE, 0, 0, 0, 0 );
	pcEditView->HandleCommand( F_INSTEXT, TRUE, (LPARAM)(szLabel), TRUE, 0, 0 );

	// Until here
//	pcEditView->HandleCommand( F_INSTEXT, TRUE, (LPARAM)(szLabel + m_cmemCurWord.GetLength()), TRUE, 0, 0 );
	Hide();

	m_pShareData->m_Common.m_bUseHokan = FALSE;	//	補完したら
	return TRUE;
}

/*
戻り値が -2 の場合は、アプリケーションは項目の選択を完了し、
リスト ボックスでそれ以上の動作が必要でないことを示します。

戻り値が -1 の場合は、リスト ボックスがキーストロークに応じて
デフォルトの動作を実行することを示します。

 戻り値が 0 以上の場合は、その値はリスト ボックスの項目の 0 を
基準としたインデックスを意味し、リスト ボックスがその項目での
キーストロークに応じてデフォルトの動作を実行することを示します。

*/
//	int CHokanMgr::OnVKeyToItem( WPARAM wParam, LPARAM lParam )
//	{
//		return KeyProc( wParam, lParam );
//	}

/*
戻り値が -2 の場合は、アプリケーションは項目の選択を完了し、
リスト ボックスでそれ以上の動作が必要でないことを示します。

戻り値が -1 の場合は、リスト ボックスがキーストロークに応じて
デフォルトの動作を実行することを示します。

 戻り値が 0 以上の場合は、その値はリスト ボックスの項目の 0 を
基準としたインデックスを意味し、リスト ボックスがその項目での
キーストロークに応じてデフォルトの動作を実行することを示します。

*/
//	int CHokanMgr::OnCharToItem( WPARAM wParam, LPARAM lParam )
//	{
//		WORD vkey;
//		WORD nCaretPos;
//		LPARAM hwndLB;
//		vkey = LOWORD(wParam);		// virtual-key code
//		nCaretPos = HIWORD(wParam);	// caret position
//		hwndLB = lParam;			// handle to list box
//	//	switch( vkey ){
//	//	}
//
//		MYTRACE( "CHokanMgr::OnCharToItem vkey=%xh\n", vkey );
//		return -1;
//	}

int CHokanMgr::KeyProc( WPARAM wParam, LPARAM lParam )
{
	WORD vkey;
	WORD nCaretPos;
	LPARAM hwndLB;
	vkey = LOWORD(wParam);		// virtual-key code
	nCaretPos = HIWORD(wParam);	// caret position
	hwndLB = lParam;			// handle to list box
//	MYTRACE( "CHokanMgr::OnVKeyToItem vkey=%xh\n", vkey );
	switch( vkey ){
	case VK_HOME:
	case VK_END:
	case VK_UP:
	case VK_DOWN:
	case VK_PRIOR:
	case VK_NEXT:
		/* リストボックスのデフォルトの動作をさせる */
		return -1;
	case VK_RETURN:
	case VK_TAB:
	case VK_RIGHT:
#if 0
	case VK_SPACE:
#endif
		/* 補完実行 */
		if( DoHokan( vkey ) ){
			return -1;
		}else{
			return -2;
		}
	case VK_ESCAPE:
	case VK_LEFT:
		m_pShareData->m_Common.m_bUseHokan = FALSE;
		return -2;
	}
	return -2;
}

//BOOL CHokanMgr::OnNextDlgCtl( WPARAM wParam, LPARAM lParam )
//{
//	/* 補完実行 */
//	DoHokan();
//	return 0;
//}


//	2001/06/18 Start by asa-o: 補完ウィンドウで選択中の単語にキーワードヘルプを表示
void CHokanMgr::ShowTip()
{
	INT			nItem,
				nTopItem,
				nItemHeight;
	CHAR		szLabel[1024];
	POINT		point;
	CEditView*	pcEditView;
	HWND		hwndCtrl;
	RECT		rcHokanWin;

	hwndCtrl = ::GetDlgItem( m_hWnd, IDC_LIST_WORDS );

	nItem = ::SendMessage( hwndCtrl, LB_GETCURSEL, 0, 0 );
	if( LB_ERR == nItem )	return ;

	::SendMessage( hwndCtrl, LB_GETTEXT, nItem, (LPARAM)szLabel );	// 選択中の単語を取得

	pcEditView = (CEditView*)m_lParam;

	// すでに辞書Tipが表示されていたら
	if( pcEditView->m_dwTipTimer == 0 )
	{
		// 辞書Tipを消す
		pcEditView -> m_cTipWnd.Hide();
		pcEditView -> m_dwTipTimer = ::GetTickCount();
	}

	// 表示する位置を決定
	nTopItem = ::SendMessage(hwndCtrl,LB_GETTOPINDEX,0,0);
	nItemHeight = ::SendMessage( hwndCtrl, LB_GETITEMHEIGHT, 0, 0 );
	point.x = m_poWin.x + m_nWidth;
	point.y = m_poWin.y + 4 + (nItem - nTopItem) * nItemHeight;
	// 2001/06/19 asa-o 選択中の単語が補完ウィンドウに表示されているなら辞書Tipを表示
	if( point.y > m_poWin.y && point.y < m_poWin.y + m_nHeight )
	{
		::SetRect( &rcHokanWin , m_poWin.x, m_poWin.y, m_poWin.x + m_nWidth, m_poWin.y + m_nHeight );
		if( !pcEditView -> ShowKeywordHelp( point,szLabel, &rcHokanWin ) )
			pcEditView -> m_dwTipTimer = ::GetTickCount();	// 表示するべきキーワードヘルプが無い
	}
}
//	2001/06/18 End

//@@@ 2002.01.18 add start
const DWORD p_helpids[] = {
	0, 0
};

LPVOID CHokanMgr::GetHelpIdTable(void)
{
	return (LPVOID)p_helpids;
}
//@@@ 2002.01.18 add end

/*[EOF]*/
