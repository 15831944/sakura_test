/*!	@file
	@brief メニュー管理＆表示

	@author Norio Nakatani
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, genta
	Copyright (C) 2002, YAZAKI, MIK, aroka
	Copyright (C) 2003, MIK
	Copyright (C) 2005, aroka, genta
	Copyright (C) 2007, ryoji
	Copyright (C) 2010, syat

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/


#ifndef _CMENUDRAWER_H_
#define _CMENUDRAWER_H_

#include "CShareData.h" // MAX_TOOLBARBUTTONS

class CMenuDrawer;

class CImageListMgr;// 2002/2/10 aroka

//#define MAX_MENUPOS	10
//	Jul. 2, 2005 genta : マクロをたくさん登録すると上限を超えてしまうので
//	初期値の300から増やす
#define MAX_MENUITEMS	400

//ツールバーの拡張	//@@@ 2002.06.15 MIK
#define TBSTYLE_COMBOBOX	((BYTE)0x40)	//ツールバーにコンボボックス
#ifndef TBSTYLE_DROPDOWN	//IE3以上
	#define TBSTYLE_DROPDOWN	0x0008
#endif
#ifndef TB_SETEXTENDEDSTYLE	//IE4以上
	#define TB_SETEXTENDEDSTYLE     (WM_USER + 84)  // For TBSTYLE_EX_*
#endif
#ifndef TBSTYLE_EX_DRAWDDARROWS	//IE4以上
	#define TBSTYLE_EX_DRAWDDARROWS 0x00000001
#endif

/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
/*!
	@brief メニュー表示＆管理

	@date 2002.2.17 YAZAKI CShareDataのインスタンスは、CProcessにひとつあるのみ。
	@date 20050809 aroka クラス外部からアクセスされないメンバはprivateにした。
*/
class CMenuDrawer
{
public:
	/*
	||  Constructors
	*/
	CMenuDrawer();
	~CMenuDrawer();
	void Create( HINSTANCE, HWND, CImageListMgr* );


	/*
	||  Attributes & Operations
	*/
	void ResetContents( void );
	//void MyAppendMenu( HMENU , int , int , const char*, BOOL = TRUE );	/* メニュー項目を追加 */
	void MyAppendMenu( HMENU hMenu, int nFlag, int nFuncId, const TCHAR*, const TCHAR* pszKey, BOOL bAddKeyStr = TRUE, int nForceIconId = -1 );	/* メニュー項目を追加 */	//お気に入り	//@@@ 2003.04.08 MIK
	int MeasureItem( int, int* );	/* メニューアイテムの描画サイズを計算 */
	void DrawItem( DRAWITEMSTRUCT* );	/* メニューアイテム描画 */
	LRESULT OnMenuChar( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	int FindToolbarNoFromCommandId( int idCommand, bool bOnlyFunc = true )const; // ツールバーIndexの取得 // 20050809 aroka

	TBBUTTON getButton( int index ) const; // 20050809 aroka

	// 古いコンパイラはstatic const int未サポート
	enum {
		TOOLBAR_BUTTON_F_TOOLBARWRAP = 384,	//ツールバー折返しアイコン（ダミー）
	};

private:
	int Find( int nFuncID );
	const TCHAR* GetLabel( int nFuncID );
	TCHAR GetAccelCharFromLabel( const TCHAR* pszLabel );
	int GetIconId( int nIndex ){ return ( 0 <= nIndex && nIndex < m_nMyButtonNum )? m_tbMyButton[nIndex].iBitmap: -1; }	// 2007.11.02 ryoji 範囲外チェック

	int MyDrawText( int nMode, HDC hDC, LPCTSTR lpItemStr, LPRECT rc, UINT uFormat );

private:
	DLLSHAREDATA*	m_pShareData;

	HINSTANCE		m_hInstance;
	HWND			m_hWndOwner;
	int				(WINAPI *m_pfnDrawTextW)( HDC, LPCWSTR, int, LPRECT, UINT );

	int				m_nMenuItemNum;
//@@@ 2002.01.03 YAZAKI m_tbMyButtonなどをCShareDataからCMenuDrawerへ移動したことによる修正。
	TBBUTTON		m_tbMyButton[MAX_TOOLBAR_ICON_COUNT+1];	/* ツールバーのボタン +1はセパレータ */
	int				m_nMyButtonNum;

	int				m_nMenuItemBitmapIdxArr[MAX_MENUITEMS];
	int				m_nMenuItemFuncArr[MAX_MENUITEMS];
	CMemory			m_cmemMenuItemStrArr[MAX_MENUITEMS];
	int				m_nMenuHeight;
	HFONT			m_hFontMenu;
	HFONT			m_hFontMenuUndelLine;

public:
	// 2010.01.30 syat アイコンイメージリストをprivate->public
	//	Oct. 16, 2000 genta
	CImageListMgr	*m_pcIcons;	//	Image List

protected:
	/*
	||  実装ヘルパ関数
	*/
	int GetData( void );	/* ダイアログデータの取得 */

//@@@ 2002.01.03 YAZAKI m_tbMyButtonなどをCShareDataからCMenuDrawerへ移動したことによる修正。
	void SetTBBUTTONVal( TBBUTTON*, int, int, BYTE, BYTE, DWORD_PTR, INT_PTR ) const;	/* TBBUTTON構造体にデータをセット */
};



///////////////////////////////////////////////////////////////////////
#endif /* _CMENUDRAWER_H_ */


/*[EOF]*/
