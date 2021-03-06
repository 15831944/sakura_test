// stdafx.h : 標準のシステム インクルード ファイル、
//				または参照回数が多く、かつあまり変更されない
//				プロジェクト専用のインクルード ファイルを記述します。
//

#if !defined(AFX_STDAFX_H__11490042_E569_11D3_BCE2_444553540001__INCLUDED_)
#define AFX_STDAFX_H__11490042_E569_11D3_BCE2_444553540001__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// この位置にヘッダーを挿入してください
#define WIN32_LEAN_AND_MEAN		// Windows ヘッダーから殆ど使用されないスタッフを除外します

#if _MSC_VER >= 1400
#pragma warning( disable : 4996 )
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


//ビルドオプション的なヘッダ
#include "config/build_config.h"

//定数
#include "config/system_constants.h"	//システム定数
#include "config/app_constants.h"		//アプリケーション定数

//グローバル
#include "global.h"

//高頻度API等
#include <windows.h>
#include <stdlib.h>  // _MAX_PATH
#include <tchar.h>

//シンプルでよく使うもの
#include "basis/primitive.h"
#include "util/std_macro.h"

//MFC互換
#include "basis/CMyString.h"
#include "basis/CMyRect.h"
#include "basis/CMyPoint.h"
#include "basis/CMySize.h"

//サクラエディタ固有型
#include "basis/SakuraBasis.h"

//デバッグ
#include "debug/Debug2.h"
#include "debug/Debug3.h"

//よく使うヘッダ
#include "debug/Debug.h"
#include "mem/CNativeW.h"
#include "mem/CNativeA.h"
#include "mem/CNativeT.h"
#include <wchar.h>

#include "util/string_ex.h"

//その他
#define malloc_char (char*)malloc
#define GlobalLockChar  (char*)::GlobalLock
#define GlobalLockUChar (unsigned char*)::GlobalLock
#define GlobalLockWChar (wchar_t*)::GlobalLock
#define GlobalLockBYTE  (BYTE*)::GlobalLock

//APIラップ
#include "apiwrap/StdControl.h"
#include "apiwrap/CommonControl.h"
#include "apiwrap/StdApi.h"

//TCHARユーティリティ
#include "util/tchar_convert.h"


//###########超仮
#include "CGraphics.h"
#include "CNormalProcess.h"
#include "window/CEditWnd.h"
#include "CEditApp.h"
#include "docplus/CModifyManager.h"
#include "docplus/CDiffManager.h"
#include "docplus/CBookmarkManager.h"
#include "CReadManager.h"
#include "CWriteManager.h"
#include "CSearchAgent.h"
#include "doc/CDocReader.h"
#include "CAppMode.h"
#pragma warning( disable : 4355 ) //warning C4355: 'this' : ベース メンバ初期化リストで使用されました。

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_STDAFX_H__11490042_E569_11D3_BCE2_444553540001__INCLUDED_)



