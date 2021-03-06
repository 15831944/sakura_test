//	$Id$
/*!	@file
	@brief アウトライン解析ダイアログボックス

	@author Norio Nakatani
	@date 1998/06/23 新規作成
	@date 1998/12/04 再作成
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001, genta, hor
	Copyright (C) 2002, aroka

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

class CDlgFuncList;

#ifndef _CDLGFUNCLIST_H_
#define _CDLGFUNCLIST_H_

#include <windows.h>
#include "CDialog.h"
#include "CFuncInfoArr.h"
#include "CShareData.h"


//!	アウトライン解析ダイアログボックス
class SAKURA_CORE_API CDlgFuncList : public CDialog
{
public:
	/*
	||  Constructors
	*/
	CDlgFuncList();
	/*
	||  Attributes & Operations
	*/
	HWND DoModeless( HINSTANCE, HWND, LPARAM, CFuncInfoArr*, int, int, int );/* モードレスダイアログの表示 */
	void ChangeView( LPARAM );	/* モードレス時：検索対象となるビューの変更 */

	CFuncInfoArr*	m_pcFuncInfoArr;	/* 関数情報配列 */
	int				m_nCurLine;			/* 現在行 */
	int				m_nSortCol;			/* ソートする列番号 */
	int				m_nListType;		/* 一覧の種類 */
	CMemory			m_cmemClipText;		/* クリップボードコピー用テキスト */
	int				m_bLineNumIsCRLF;	/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
protected:
	BOOL OnInitDialog( HWND, WPARAM, LPARAM );
	BOOL OnBnClicked( int );
	BOOL OnNotify( WPARAM, LPARAM );
	BOOL OnSize( WPARAM, LPARAM );
	void SetData( void );	/* ダイアログデータの設定 */
	int GetData( void );	/* ダイアログデータの取得 */

	/*
	||  実装ヘルパ関数
	*/
	BOOL OnJump( bool bCheckAutoClose = true );	//	bCheckAutoClose：「このダイアログを自動的に閉じる」をチェックするかどうか
	void SetTreeCpp( HWND );	/* ツリーコントロールの初期化：C++メソッドツリー */
	void SetTreeJava( HWND, BOOL );	/* ツリーコントロールの初期化：Javaメソッドツリー */
	void SetTreeTxt( HWND );	/* ツリーコントロールの初期化：テキストトピックツリー */
	int SetTreeTxtNest( HWND, HTREEITEM, int, int, HTREEITEM*, int );
	void GetTreeTextNext( HWND, HTREEITEM, int );

	// 2001.12.03 hor
	void SetTreeBookMark( HWND );		/* ツリーコントロールの初期化：ブックマーク */
	LPVOID GetHelpIdTable(void);	//@@@ 2002.01.18 add
	void Key2Command( WORD );		//	キー操作→コマンド変換

private:
	//	May 18, 2001 genta
	/*!
		@brief アウトライン解析種別

		0: List, 1: Tree
	*/
	int	m_nViewType;

};



///////////////////////////////////////////////////////////////////////
#endif /* _CDLGFUNCLIST_H_ */


/*[EOF]*/
