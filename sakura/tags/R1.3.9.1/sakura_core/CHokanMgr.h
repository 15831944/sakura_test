//	$Id$
/*!	@file
	@brief キーワード補完

	@author Norio Nakatani
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

class CHokanMgr;

#ifndef _CHOKANMGR_H_
#define _CHOKANMGR_H_

#include "CDialog.h"
#include <windows.h>
#include "CMemory.h"

//! キーワード補完
class SAKURA_CORE_API CHokanMgr : public CDialog
{
public:
	/*
	||  Constructors
	*/
	CHokanMgr();
	~CHokanMgr();

	HWND DoModeless( HINSTANCE, HWND, LPARAM );/* モードレスダイアログの表示 */
	void Hide( void );
	/* 初期化 */
	int CHokanMgr::Search(
	//	HFONT		hFont,
		POINT*		ppoWin,
		int			nWinHeight,
		int			nColmWidth,
		const char*	pszCurWord,
	//	void*		pcEditView,
		const char* pszHokanFile,
		int			bHokanLoHiCase,			// 入力補完機能：英大文字小文字を同一視する 2001/06/19 asa-o
		CMemory*	pcmemHokanWord = NULL	// 補完候補が１つのときこれに格納 2001/06/19 asa-o
	);
	void SetCurKouhoStr( void );
	BOOL DoHokan( int );
	void ChangeView( LPARAM );/* モードレス時：対象となるビューの変更 */


	BOOL OnInitDialog( HWND, WPARAM wParam, LPARAM lParam );
	BOOL OnDestroy( void );
	BOOL OnSize( WPARAM wParam, LPARAM lParam );
	BOOL OnBnClicked( int wID );
	BOOL OnKeyDown( WPARAM wParam, LPARAM lParam );
	BOOL OnCbnSelChange( HWND hwndCtl, int wID );
	BOOL OnLbnDblclk( int wID );
	BOOL OnKillFocus( WPARAM wParam, LPARAM lParam );
//	int OnVKeyToItem( WPARAM wParam, LPARAM lParam );
//	int OnCharToItem( WPARAM wParam, LPARAM lParam );
//	BOOL OnNextDlgCtl( WPARAM, LPARAM );

	int KeyProc( WPARAM, LPARAM );

//	2001/06/18 asa-o
	void ShowTip();	// 補完ウィンドウで選択中の単語にキーワードヘルプの表示


//	HFONT			m_hFont;
//	HFONT			m_hFontOld;
	CMemory			m_cmemCurWord;
	CMemory*		m_pcmemKouho;
	int				m_nKouhoNum;

	int				m_nCurKouhoIdx;
	char*			m_pszCurKouho;

	POINT			m_poWin;
	int				m_nWinHeight;
	int				m_nColmWidth;
	int				m_pnDx[256 + 1];		/* 文字列描画用文字幅配列 */
//	void*			m_pcEditView;
	int				m_bTimerFlag;

protected:
	/*
	||  実装ヘルパ関数
	*/
	LPVOID GetHelpIdTable(void);	//@@@ 2002.01.18 add

};



///////////////////////////////////////////////////////////////////////
#endif /* _CHOKANMGR_H_ */


/*[EOF]*/
