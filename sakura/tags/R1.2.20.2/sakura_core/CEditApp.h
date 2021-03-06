//	$Id$
/************************************************************************
*
*	CEditApp.h
*
*	MRU、キー割り当て、共通設定、編集ウィンドウの管理
*	Copyright (C) 1998-2000, Norio Nakatani
*
*
*    CREATE: 1998/5/13
*
************************************************************************/

class CEditApp;

#ifndef _CEDITAPP_H_
#define _CEDITAPP_H_





#include <windows.h>
#include "CEditWnd.h"
#include "CKeyBind.h"
#include "CShareData.h"
#include "CMenuDrawer.h"







/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
class SAKURA_CORE_API CEditApp
{
public:
	/*
	||  Constructors
	*/
	CEditApp();
	~CEditApp();

	/*
	|| メンバ関数
	*/
	HWND Create( HINSTANCE );	/* 作成 */
	LRESULT DispatchEvent( HWND, UINT, WPARAM, LPARAM );	/* メッセージ処理 */
	void MessageLoop( void );	/* メッセージループ */
	int	CreatePopUpMenu_L( void );	/* ポップアップメニュー(トレイ左ボタン) */
	int	CreatePopUpMenu_R( void );	/* ポップアップメニュー(トレイ右ボタン) */

	static bool OpenNewEditor( HINSTANCE, HWND, char*, int, BOOL, bool sync = false );		/* 新規編集ウィンドウの追加 ver 0 */
	static bool OpenNewEditor2( HINSTANCE, HWND , FileInfo*, BOOL, bool sync = false );	/* 新規編集ウィンドウの追加 ver 1 */
//シングルプロセス版用
//	static HWND OpenNewEditor3( HINSTANCE, HWND , const char*, BOOL );	/* 新規編集ウィンドウの追加 ver 2 */

	static BOOL CloseAllEditor( void );	/* すべてのウィンドウを閉じる */	//Oct. 7, 2000 jepro 「編集ウィンドウの全終了」という説明を左記のように変更
	static void TerminateApplication( void );	/* テキストエディタの終了 */
	/* コマンドラインの解析 */
	static void CEditApp::ParseCommandLine( 
		const char*	pszCmdLineSrc,
		BOOL*		pbGrepMode,
		CMemory*	pcmGrepKey,
		CMemory*	pcmGrepFile,
		CMemory*	pcmGrepFolder,
		BOOL*		pbGrepSubFolder,
		BOOL*		pbGrepLoHiCase,
		BOOL*		pbGrepRegularExp,
		BOOL*		pbGrepKanjiCode_AutoDetect,
		BOOL*		pbGrepOutputLine,
		int	*		pnGrepOutputStyle,
		BOOL*		pbDebugMode,
		BOOL*		pbNoWindow,
		FileInfo*	pfi,
		BOOL*		pbReadOnly
	);

	/*
	|| メンバ変数
	*/
//	CKeyBind		m_CKeyBind;
//	HACCEL			m_hAccel;

private:
	CMenuDrawer		m_CMenuDrawer;
	HINSTANCE		m_hInstance;
	HWND			m_hWnd;
	char*			m_pszAppName;
	BOOL			m_bCreatedTrayIcon;	/* トレイにアイコンを作った  */

	CShareData		m_cShareData;
	DLLSHAREDATA*	m_pShareData;
	int				m_nSettingType;

	CImageListMgr	m_hIcons;


	/*
	|| 実装ヘルパ系
	*/
protected:
	BOOL TrayMessage(HWND , DWORD , UINT , HICON , const char* );	/* タスクトレイのアイコンに関する処理 */
	void OnCommand( WORD , WORD  , HWND );	/* WM_COMMANDメッセージ処理 */


};


///////////////////////////////////////////////////////////////////////
#endif /* _CEDITAPP_H_ */

/*[EOF]*/
