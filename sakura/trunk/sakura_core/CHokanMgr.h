/*!	@file
	@brief キーワード補完

	@author Norio Nakatani
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001, asa-o
	Copyright (C) 2003, Moca

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

class CHokanMgr;

#ifndef _CHOKANMGR_H_
#define _CHOKANMGR_H_

#include "CDialog.h"
#include <windows.h>
#include "CMemory.h"

class CEditView;


/*! @brief キーワード補完

	@date 2003.06.25 Moca ファイル内からの補完機能を追加
*/
class CHokanMgr : public CDialog
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
	int Search(
		POINT*			ppoWin,
		int				nWinHeight,
		int				nColumnWidth,
		const char*		pszCurWord,
		const TCHAR*	pszHokanFile,
		bool			bHokanLoHiCase,			// 入力補完機能：英大文字小文字を同一視する 2001/06/19 asa-o
		bool			bHokanByFile,			// 編集中データから候補を探す。 2003.06.23 Moca
		CMemory*		pcmemHokanWord = NULL	// 補完候補が１つのときこれに格納 2001/06/19 asa-o
	);
//	void SetCurKouhoStr( void );
	BOOL DoHokan( int );
	void ChangeView( LPARAM );/* モードレス時：対象となるビューの変更 */


	BOOL OnInitDialog( HWND, WPARAM wParam, LPARAM lParam );
	BOOL OnDestroy( void );
	BOOL OnSize( WPARAM wParam, LPARAM lParam );
	BOOL OnBnClicked( int wID );
	BOOL OnKeyDown( WPARAM wParam, LPARAM lParam );
	BOOL OnLbnSelChange( HWND hwndCtl, int wID );
	BOOL OnLbnDblclk( int wID );
	BOOL OnKillFocus( WPARAM wParam, LPARAM lParam );
//	int OnVKeyToItem( WPARAM wParam, LPARAM lParam );
//	int OnCharToItem( WPARAM wParam, LPARAM lParam );

	int KeyProc( WPARAM, LPARAM );

//	2001/06/18 asa-o
	void ShowTip();	// 補完ウィンドウで選択中の単語にキーワードヘルプの表示


	CMemory			m_cmemCurWord;
	CMemory*		m_pcmemKouho;
	int				m_nKouhoNum;

	int				m_nCurKouhoIdx;

	POINT			m_poWin;
	int				m_nWinHeight;
	int				m_nColumnWidth;
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
