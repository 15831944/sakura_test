/*!	@file
	@brief 共通関数群

	@author Norio Nakatani
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001-2002, genta
	Copyright (C) 2001, shoji masami, Stonee, MIK
	Copyright (C) 2002, aroka, hor, MIK, YAZAKI
	Copyright (C) 2003, genta, Moca
	Copyright (C) 2004, genta, novice
	Copyright (C) 2005, genta, aroka
	Copyright (C) 2006, ryoji, rastiv
	Copyright (C) 2007, ryoji
	Copyright (C) 2008, kobake

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#ifndef _ETC_UTY_H_
#define _ETC_UTY_H_

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include "global.h"
#include <shlobj.h>

// 2007.12.21 ryoji
// Windows SDK for Vista 以降には NewApis.h は含まれないので使用をあきらめる
// multimon.h もいずれは削除されてしまうかもしれないので使用しない
// Win95/NT4.0には存在しない API を使うようになるのでこれら古い OS への対応は不可です
//
// Note.
// 新しい Windows SDK を使用する場合、NewApis.h, multimon.h を除外するので
// WINVER に 0x0400 のような古い OS 対応の値を指定するとコンパイルエラーになります
// 明示指定がない場合のデフォルトは 0x0600 以上なので特に指定する必要はありません
// ちなみに通常の Windows.h 利用では昔から次のようにちょっと変なところがあります
// ・GetLongPathName() API は Win95(0x0400) 非対応なのに 0x0400 にしてもエラーになりません
// ・マルチモニタ関連 API は Win98(0x0410) 対応なのに 0x0500 以上にしないとエラーになります

#ifndef _INC_SDKDDKVER	// 新しい Windows SDK では windows.h が sdkddkver.h を include する
#define WANT_GETLONGPATHNAME_WRAPPER
#include <NewApis.h>
#include <multimon.h>
#endif

#if (_MSC_VER >= 1500) && (WINVER <= 0x0400)
#include <multimon.h>
#endif

/*!
	@brief 画面 DPI スケーリング
	@note 96 DPI ピクセルを想定しているデザインをどれだけスケーリングするか

	@date 2009.10.01 ryoji 高DPI対応用に作成
*/
class CDPI{
	static void Init()
	{
		if( !bInitialized )
		{
			HDC hDC = GetDC(NULL);
			nDpiX = GetDeviceCaps(hDC, LOGPIXELSX);
			nDpiY = GetDeviceCaps(hDC, LOGPIXELSY);
			ReleaseDC(NULL, hDC);
			bInitialized = true;
		}
	}
	static int nDpiX;
	static int nDpiY;
	static bool bInitialized;
public:
	static int ScaleX(int x){Init(); return ::MulDiv(x, nDpiX, 96);}
	static int ScaleY(int y){Init(); return ::MulDiv(y, nDpiY, 96);}
	static int UnscaleX(int x){Init(); return ::MulDiv(x, 96, nDpiX);}
	static int UnscaleY(int y){Init(); return ::MulDiv(y, 96, nDpiY);}
	static void ScaleRect(LPRECT lprc)
	{
		lprc->left = ScaleX(lprc->left);
		lprc->right = ScaleX(lprc->right);
		lprc->top = ScaleY(lprc->top);
		lprc->bottom = ScaleY(lprc->bottom);
	}
	static void UnscaleRect(LPRECT lprc)
	{
		lprc->left = UnscaleX(lprc->left);
		lprc->right = UnscaleX(lprc->right);
		lprc->top = UnscaleY(lprc->top);
		lprc->bottom = UnscaleY(lprc->bottom);
	}
	static int PointsToPixels(int pt, int ptMag = 1){Init(); return ::MulDiv(pt, nDpiY, 72 * ptMag);}	// ptMag: 引数のポイント数にかかっている倍率
	static int PixelsToPoints(int px, int ptMag = 1){Init(); return ::MulDiv(px * ptMag, 72, nDpiY);}	// ptMag: 戻り値のポイント数にかける倍率
};

inline int DpiScaleX(int x){return CDPI::ScaleX(x);}
inline int DpiScaleY(int y){return CDPI::ScaleY(y);}
inline int DpiUnscaleX(int x){return CDPI::UnscaleX(x);}
inline int DpiUnscaleY(int y){return CDPI::UnscaleY(y);}
inline void DpiScaleRect(LPRECT lprc){CDPI::ScaleRect(lprc);}
inline void DpiUnscaleRect(LPRECT lprc){CDPI::UnscaleRect(lprc);}
inline int DpiPointsToPixels(int pt, int ptMag = 1){return CDPI::PointsToPixels(pt, ptMag);}
inline int DpiPixelsToPoints(int px, int ptMag = 1){return CDPI::PixelsToPoints(px, ptMag);}

#ifndef GA_PARENT
#define GA_PARENT		1
#define GA_ROOT			2
#define GA_ROOTOWNER	3
#endif
#define GA_ROOTOWNER2	100

#include "CHtmlHelp.h"	//	Jul.  6, 2001 genta
class CMemory;// 2002/2/3 aroka ヘッダ軽量化
class CEol;// 2002/2/3 aroka ヘッダ軽量化
class CBregexp;// 2002/2/3 aroka ヘッダ軽量化

BOOL MyWinHelp(HWND hWndMain, LPCTSTR lpszHelp, UINT uCommand, DWORD_PTR dwData);	/* WinHelp のかわりに HtmlHelp を呼び出す */	// 2006.07.22 ryoji

//!フォント選択ダイアログ
BOOL MySelectFont( LOGFONT* plf, INT* piPointSize, HWND hwndDlgOwner, bool );   // 2009.10.01 ryoji ポイントサイズ（1/10ポイント単位）引数追加

void CutLastYenFromDirectoryPath( TCHAR* );/* フォルダの最後が半角かつ'\\'の場合は、取り除く "c:\\"等のルートは取り除かない*/
void AddLastYenFromDirectoryPath( TCHAR* );/* フォルダの最後が半角かつ'\\'でない場合は、付加する */
int AddLastChar( TCHAR*, int, TCHAR );/* 2003.06.24 Moca 最後の文字が指定された文字でないときは付加する */
int LimitStringLengthB( const char*, int, int, CMemory& );/* データを指定バイト数以内に切り詰める */
const char* GetNextLimitedLengthText( const char*, int, int, int*, int* );/* 指定長以下のテキストに切り分ける */
const char* GetNextLine( const char*, int, int*, int*, CEol* );/* CR0LF0,CRLF,LFCR,LF,CRで区切られる「行」を返す。改行コードは行長に加えない */
void GetLineColm( const char*, int*, int* );
bool fexist(LPCTSTR pszPath); //!< ファイルまたはディレクトリが存在すればtrue
bool IsFilePath( const char*, int*, int*, bool = true );
bool IsFileExists(const TCHAR* path, bool bFileOnly = false);
BOOL IsURL( const char*, int, int* );/* 指定アドレスがURLの先頭ならばTRUEとその長さを返す */
BOOL IsMailAddress( const char*, int, int* );	/* 現在位置がメールアドレスならば、NULL以外と、その長さを返す */
//#ifdef COMPILE_COLOR_DIGIT
int IsNumber( const char*, int, int );/* 数値ならその長さを返す */	//@@@ 2001.02.17 by MIK
//#endif
void ActivateFrameWindow( HWND );	/* アクティブにする */
BOOL GetSystemResources( int*, int*, int* );	/* システムリソースを調べる */
BOOL CheckSystemResources( const TCHAR* );	/* システムリソースのチェック */
//BOOL CheckWindowsVersion( const char* pszAppName );	/* Windowsバージョンのチェック */
// Jul. 5, 2001 shoji masami
//bool CheckWindowsVersionNT( void );	/* NTプラットフォームかどうか */
void GetAppVersionInfo( HINSTANCE, int, DWORD*, DWORD* );	/* リソースから製品バージョンの取得 */
void SplitPath_FolderAndFile( const char*, char*, char* );	/* ファイルのフルパスを、フォルダとファイル名に分割 */
BOOL GetAbsolutePath( const char*, char*, BOOL );	/* 相対パス→絶対パス */
BOOL GetLongFileName( const TCHAR*, TCHAR* );	/* ロングファイル名を取得する */
BOOL CheckEXT( const TCHAR*, const TCHAR* );	/* 拡張子を調べる */
char* my_strtok( char*, int, int*, const char* );
/* Shell Interface系(?) */
BOOL SelectDir(HWND, const TCHAR*, const TCHAR*, TCHAR* );	/* フォルダ選択ダイアログ */
//不要＆実装間違いのため
//ITEMIDLIST* CreateItemIDList( const char* );	/* パス名に対するアイテムＩＤリストを取得する */
//BOOL DeleteItemIDList( ITEMIDLIST* );/* アイテムＩＤリストを削除する */
BOOL ResolveShortcutLink(HWND hwnd, LPCTSTR lpszLinkFile, LPTSTR lpszPath);/* ショートカット(.lnk)の解決 */
void ResolvePath(TCHAR* pszPath); //!< ショートカットの解決とロングファイル名へ変換を行う。

/*
||	処理中のユーザー操作を可能にする
||	ブロッキングフック(?)(メッセージ配送)
*/
BOOL BlockingHook( HWND hwndDlgCancel );

//	Jun. 26, 2001 genta
//!	正規表現ライブラリのバージョン取得
bool CheckRegexpVersion( HWND hWnd, int nCmpId, bool bShowMsg = false );
bool CheckRegexpSyntax( const char* szPattern, HWND hWnd, bool bShowMessage, int nOption = -1 );// 2002/2/1 hor追加
bool InitRegexp( HWND hWnd, CBregexp& rRegexp, bool bShowMessage );

HWND OpenHtmlHelp( HWND hWnd, LPCTSTR szFile, UINT uCmd, DWORD_PTR data, bool msgflag = true);
DWORD NetConnect ( const char strNetWorkPass[] );

int cescape(const TCHAR* org, TCHAR* buf, TCHAR cesc, TCHAR cwith);
int cescape_j(const char* org, char* out, char cesc, char cwith);

/* ヘルプの目次を表示 */
void ShowWinHelpContents( HWND hwnd, LPCTSTR lpszHelp );

bool SetClipboardText( HWND, const char*, int );	//!クリープボードにText形式でコピーする

/*!	&の二重化
	メニューに含まれる&を&&に置き換える
	@author genta
	@date 2002/01/30 cescapeに拡張し，
	@date 2004/06/19 genta Generic mapping
*/
inline void dupamp(const TCHAR* org, TCHAR* out)
{	cescape( org, out, _T('&'), _T('&') ); }
///////////////////////////////////////////////////////////////////////

/* カラー名＜＞インデックス番号の変換 */	//@@@ 2002.04.30
int GetColorIndexByName( const char *name );
const char* GetColorNameByIndex( int index );

//	Sep. 10, 2002 genta CWSH.cppからの移動に伴う追加
bool ReadRegistry(HKEY Hive, const TCHAR *Path, const TCHAR* Item, TCHAR* Buffer, unsigned BufferSize);

//	Dec. 2, 2002 genta
void GetExedir( LPTSTR pDir, LPCTSTR szFile = NULL );
void GetInidir( LPTSTR pDir, LPCTSTR szFile = NULL ); // 2007.05.19 ryoji
void GetInidirOrExedir( LPTSTR pDir, LPCTSTR szFile = NULL, BOOL bRetExedirIfFileEmpty = FALSE ); // 2007.05.22 ryoji
HICON GetAppIcon( HINSTANCE hInst, int nResource, const TCHAR* szFile, bool bSmall = false);

//	Apr. 03, 2003 genta
char *strncpy_ex(char *dst, size_t dst_count, const char* src, size_t src_count);

FILE *_tfopen_absexe(LPCTSTR fname, LPCTSTR mode); // 2003.06.23 Moca
FILE *_tfopen_absini(LPCTSTR fname, LPCTSTR mode, BOOL bOrExedir = TRUE); // 2007.05.19 ryoji

//	Apr. 30, 2003 genta
//	ディレクトリの深さを調べる
int CalcDirectoryDepth(const TCHAR* path);

//	May 01, 2004 genta マルチモニタ対応のデスクトップ領域取得
bool GetMonitorWorkRect(HWND hWnd, LPRECT prcWork, LPRECT prcMonitor = NULL);		// 2006.04.21 ryoji パラメータ prcMonitor を追加
bool GetMonitorWorkRect(LPCRECT prc, LPRECT prcWork, LPRECT prcMonitor = NULL);		// 2006.04.21 ryoji
bool GetMonitorWorkRect(POINT pt, LPRECT prcWork, LPRECT prcMonitor = NULL);		// 2006.04.21 ryoji
bool GetMonitorWorkRect(HMONITOR hMon, LPRECT prcWork, LPRECT prcMonitor = NULL);	// 2006.04.21 ryoji

HWND MyGetAncestor( HWND hWnd, UINT gaFlags );	// 指定したウィンドウの祖先のハンドルを取得する	// 2007.07.01 ryoji

//ファイル時刻
class CFileTime{
public:
	CFileTime(){ ClearFILETIME(); }
	CFileTime(const FILETIME& ftime){ SetFILETIME(ftime); }
	//設定
	void ClearFILETIME(){ m_ftime.dwLowDateTime = m_ftime.dwHighDateTime = 0; m_bModified = true; }
	void SetFILETIME(const FILETIME& ftime){ m_ftime = ftime; m_bModified = true; }
	//取得
	const FILETIME& GetFILETIME() const{ return m_ftime; }
	const SYSTEMTIME& GetSYSTEMTIME() const
	{
		//キャッシュ更新 -> m_systime, m_bModified
		if(m_bModified){
			m_bModified = false;
			FILETIME ftimeLocal;
			if(!::FileTimeToLocalFileTime( &m_ftime, &ftimeLocal ) || !::FileTimeToSystemTime( &ftimeLocal, &m_systime )){
				memset(&m_systime,0,sizeof(m_systime)); //失敗時ゼロクリア
			}
		}
		return m_systime;
	}
	const SYSTEMTIME* operator->() const{ return &GetSYSTEMTIME(); }
	//判定
	bool IsZero() const
	{
		return m_ftime.dwLowDateTime == 0 && m_ftime.dwHighDateTime == 0;
	}
protected:
private:
	FILETIME m_ftime;
	//キャッシュ
	mutable SYSTEMTIME	m_systime;
	mutable bool		m_bModified;
};
bool GetLastWriteTimestamp( const TCHAR* filename, CFileTime* pcFileTime ); //	Oct. 22, 2005 genta

// 20051121 aroka
bool GetDateTimeFormat( TCHAR* szResult, int size, const TCHAR* format, const SYSTEMTIME& systime );

// novice 2004/10/10
int getCtrlKeyState();

/*! 相対パスか判定する
	@author Moca
	@date 2003.06.23
*/
inline bool _IS_REL_PATH(const char* path)
{
	bool ret = true;
	if( ( 'A' <= path[0] && path[0] <= 'Z' || 'a' <= path[0] && path[0] <= 'z' )
		&& path[1] == ':' && path[2] == '\\'
		|| path[0] == '\\' && path[1] == '\\'
		 ){
		ret = false;
	}
	return ret;
}

// 2005.11.26 aroka
bool IsLocalDrive( const TCHAR* pszDrive );

// 2006.06.17 ryoji
#define PACKVERSION( major, minor ) MAKELONG( minor, major )
DWORD GetDllVersion( LPCTSTR lpszDllName );	// シェルやコモンコントロール DLL のバージョン番号を取得	// 2006.06.17 ryoji
DWORD GetComctl32Version();					// Comctl32.dll のバージョン番号を取得						// 2006.06.17 ryoji
BOOL IsVisualStyle();						// 自分が現在ビジュアルスタイル表示状態かどうかを示す		// 2006.06.17 ryoji
void PreventVisualStyle( HWND hWnd );		// 指定ウィンドウでビジュアルスタイルを使わないようにする	// 2006.06.23 ryoji
void MyInitCommonControls();				// コモンコントロールを初期化する							// 2006.06.21 ryoji

//カレントディレクトリユーティリティ。
//コンストラクタでカレントディレクトリを保存し、デストラクタでカレントディレクトリを復元するモノ。
//2008.03.01 kobake 作成
class CCurrentDirectoryBackupPoint{
public:
	CCurrentDirectoryBackupPoint();
	~CCurrentDirectoryBackupPoint();
private:
	TCHAR m_szCurDir[_MAX_PATH];
};

void ChangeCurrentDirectoryToExeDir();
HMODULE LoadLibraryExedir(LPCTSTR pszDll);

BOOL GetSpecialFolderPath( int nFolder, LPTSTR pszPath );	// 特殊フォルダのパスを取得する	// 2007.05.19 ryoji
int MyPropertySheet( LPPROPSHEETHEADER lppsph );	// 独自拡張プロパティシート	// 2007.05.24 ryoji

#endif /* _ETC_UTY_H_ */


/*[EOF]*/
