//	$Id$
/*!	@file

	@author Norio Nakatani
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

class CDlgDebug;

#ifndef _CDLGDEBUG_H_
#define _CDLGDEBUG_H_

#include <windows.h>
#include "CMemory.h"

//#define MAX_SRCHTXT 1024
//#define MAX_TEXTARR 32

/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
class CDlgDebug
{
public:
	/*
	||  Constructors
	*/
	CDlgDebug();
	~CDlgDebug();

	/*
	||  Attributes & Operations
	*/
	BOOL DispatchEvent( HWND, UINT, WPARAM, LPARAM );	/* ダイアログのメッセージ処理 */
	int DoModal( HINSTANCE, HWND, CMemory& );	/* モーダルダイアログの表示 */

	HINSTANCE	m_hInstance;	/* アプリケーションインスタンスのハンドル */
	HWND		m_hwndParent;	/* オーナーウィンドウのハンドル */
	HWND		m_hWnd;		/* このダイアログのハンドル */

	CMemory		m_cmemDebugInfo;


protected:
	/*
	||  実装ヘルパ関数
	*/
};



///////////////////////////////////////////////////////////////////////
#endif /* _CDLGDEBUG_H_ */

/*[EOF]*/
