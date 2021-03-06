//	$Id$
/************************************************************************

	CSplitBoxWnd.h

	分割ボックスウィンドウクラス
	Copyright (C) 1998-2000, Norio Nakatani

************************************************************************/

class CSplitBoxWnd;

#ifndef _CSPLITBOXWND_H_
#define _CSPLITBOXWND_H_

#include "CWnd.h"
//#include <windows.h>
#include "mymessage.h"
/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
class CSplitBoxWnd : public CWnd
{
public:
	/*
	||  Constructors
	*/
	CSplitBoxWnd();
	virtual ~CSplitBoxWnd();
	HWND Create( HINSTANCE , HWND , int );

	static void Draw3dRect( HDC , int , int , int , int , COLORREF , COLORREF );
	static void FillSolidRect( HDC , int , int , int , int , COLORREF );

//	LRESULT DispatchEvent( HWND, UINT, WPARAM, LPARAM );	/* メッセージディスパッチャ */



public:
	const char*	m_pszClassName;	/* クラス名 */
//	HINSTANCE	m_hInstance;	/* インスタンスハンドル */
//	HWND		m_hWnd;			/* ウィンドウハンドル */
//	HWND		m_hwndParent; 	/* 親ウィンドウハンドル */
	int			m_bVertical;	/* 垂直分割ボックスか */
	int			m_nDragPosY;
	int			m_nDragPosX;
protected:
	/* 仮想関数 */

	/* 仮想関数 メッセージ処理 詳しくは実装を参照 */
	LRESULT OnPaint( HWND, UINT, WPARAM, LPARAM );/* 描画処理 */
	LRESULT OnLButtonDown( HWND, UINT, WPARAM, LPARAM );// WM_LBUTTONDOWN
	LRESULT OnMouseMove( HWND, UINT, WPARAM, LPARAM );// WM_MOUSEMOVE
	LRESULT OnLButtonUp( HWND, UINT, WPARAM, LPARAM );//WM_LBUTTONUP
	LRESULT OnLButtonDblClk( HWND, UINT, WPARAM, LPARAM );//WM_LBUTTONDBLCLK


};


///////////////////////////////////////////////////////////////////////
#endif /* _CSPLITBOXWND_H_ */

/*[EOF]*/
