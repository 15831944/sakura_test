/*!	@file
	@brief タイプ別設定ダイアログボックス

	@author Norio Nakatani
	@date 1998/05/08  新規作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, jepro
	Copyright (C) 2001, MIK, asa-o
	Copyright (C) 2002, YAZAKI
	Copyright (C) 2003, genta
	Copyright (C) 2005, MIK, aroka, genta
	Copyright (C) 2006, fon

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/

#ifndef SAKURA_TYPES_CPROPTYPES_H_
#define SAKURA_TYPES_CPROPTYPES_H_

class CPropTypes;


#include <windows.h>
#include "CShareData.h"

/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
/*!
	@brief タイプ別設定ダイアログボックス

	@date 2002.2.17 YAZAKI CShareDataのインスタンスは、CProcessにひとつあるのみ。
*/
class SAKURA_CORE_API CPropTypes{

public:
	/*
	||  Constructors
	*/
	CPropTypes();
	~CPropTypes();
	void Create( HINSTANCE, HWND );	//!< 初期化

	/*
	||  Attributes & Operations
	*/
	int DoPropertySheet( int );	/* プロパティシートの作成 */
	INT_PTR DispatchEvent_p1( HWND, UINT, WPARAM, LPARAM );	/* p1 メッセージ処理 */
	INT_PTR DispatchEvent_p2( HWND, UINT, WPARAM, LPARAM );	/* p2 メッセージ処理 支援タブ */ // 2001/06/14 asa-o
	INT_PTR DispatchEvent_p3( HWND, UINT, WPARAM, LPARAM );	/* p3 メッセージ処理 */
	INT_PTR DispatchEvent_p3_new( HWND, UINT, WPARAM, LPARAM );	/* p3 メッセージ処理 */
	static BOOL SelectColor( HWND , COLORREF*, DWORD* );	/* 色選択ダイアログ */
	INT_PTR DispatchEvent_Regex( HWND, UINT, WPARAM, LPARAM );	/* メッセージ処理 正規表現キーワード */	//@@@ 2001.11.17 add MIK
	INT_PTR DispatchEvent_KeyHelp( HWND, UINT, WPARAM, LPARAM );	/* メッセージ処理 キーワード辞書選択 */	//@@@ 2006.04.10 fon

private:
	HINSTANCE	m_hInstance;	/* アプリケーションインスタンスのハンドル */
	HWND		m_hwndParent;	/* オーナーウィンドウのハンドル */
	HWND		m_hwndThis;		/* このダイアログのハンドル */
	int			m_nPageNum;

	/*
	|| ダイアログデータ
	*/
	char			m_szHelpFile[_MAX_PATH + 1];
	STypeConfig		m_Types;
	CKeyWordSetMgr*	m_pCKeyWordSetMgr;	// Mar. 31, 2003 genta メモリ削減のためポインタに
	int				m_nCurrentColorType;		/* 現在選択されている色タイプ */
	DLLSHAREDATA*	m_pShareData;
	int		m_nSet[ MAX_KEYWORDSET_PER_TYPE ];	//	2005.01.13 MIK keyword set index

	// フォントDialogカスタムパレット
	DWORD			m_dwCustColors[16];

protected:
	/*
	||  実装ヘルパ関数
	*/
	void OnHelp( HWND , int );	/* ヘルプ */
	//void DrawToolBarItemList( DRAWITEMSTRUCT* );	/* ツールバーボタンリストのアイテム描画 */// 20050809 aroka 未使用
	void DrawColorButton( DRAWITEMSTRUCT* , COLORREF );	/* 色ボタンの描画 */
	void SetData_p1( HWND );	/* ダイアログデータの設定 p1 */
	int  GetData_p1( HWND );	/* ダイアログデータの取得 p1 */

	// 2001/06/14 asa-o
	void SetData_p2( HWND );	/* ダイアログデータの設定 p2 支援タブ */
	int  GetData_p2( HWND );	/* ダイアログデータの取得 p2 支援タブ */

	void SetData_p3( HWND );	/* ダイアログデータの設定 p3 */
	int  GetData_p3( HWND );	/* ダイアログデータの取得 p3 */
	void SetData_p3_new( HWND );	/* ダイアログデータの設定 p3 */
	int  GetData_p3_new( HWND );	/* ダイアログデータの取得 p3 */
	void p3_Import_Colors( HWND );	/* 色の設定をインポート */
	void p3_Export_Colors( HWND );	/* 色の設定をエクスポート */
	void DrawColorListItem( DRAWITEMSTRUCT*);	/* 色種別リスト オーナー描画 */

	//	Sept. 10, 2000 JEPRO 次行を追加
	void EnableTypesPropInput( HWND hwndDlg );	//	タイプ別設定のカラー設定のON/OFF

	void SetData_Regex( HWND );	/* ダイアログデータの設定 正規表現キーワード */	//@@@ 2001.11.17 add MIK
	int  GetData_Regex( HWND );	/* ダイアログデータの取得 正規表現キーワード */	//@@@ 2001.11.17 add MIK
	BOOL Import_Regex( HWND );	//@@@ 2001.11.17 add MIK
	BOOL Export_Regex( HWND );	//@@@ 2001.11.17 add MIK
	static INT_PTR CALLBACK PropTypesRegex( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );	//@@@ 2001.11.17 add MIK
	BOOL RegexKakomiCheck(const char *s);	//@@@ 2001.11.17 add MIK

	void RearrangeKeywordSet( HWND );	// Jan. 23, 2005 genta キーワードセット再配置

	// 2006.04.10 fon
	void SetData_KeyHelp( HWND );	/* ダイアログデータの設定 キーワード辞書選択 */
	int  GetData_KeyHelp( HWND );	/* ダイアログデータの取得 キーワード辞書選択 */
	BOOL Import_KeyHelp( HWND );	//@@@ 2006.04.10 fon
	BOOL Export_KeyHelp( HWND );	//@@@ 2006.04.10 fon
	static INT_PTR CALLBACK PropTypesKeyHelp( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

public:
	//	Jan. 23, 2005 genta
	//	タイプ別設定データの設定・取得
	void SetTypeData( const STypeConfig& t ){ m_Types = t; }
	void GetTypeData( STypeConfig& t ) const { t = m_Types; }
};



///////////////////////////////////////////////////////////////////////
#endif /* SAKURA_TYPES_CPROPTYPES_H_ */


/*[EOF]*/
