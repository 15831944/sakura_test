/*!	@file
	@brief プロセス間共有データへのアクセス

	@author Norio Nakatani
	@date 1998/05/26  新規作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, jepro, genta
	Copyright (C) 2001, jepro, genta, asa-o, MIK, YAZAKI, hor
	Copyright (C) 2002, genta, aroka, Moca, MIK, YAZAKI, hor
	Copyright (C) 2003, Moca, aroka, MIK, genta
	Copyright (C) 2004, Moca, novice, genta
	Copyright (C) 2005, MIK, genta, ryoji, aroka, Moca
	Copyright (C) 2006, aroka, ryoji, D.S.Koba, fon
	Copyright (C) 2007, ryoji, maru

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

class CShareData;

#ifndef _CSHAREDATA_H_
#define _CSHAREDATA_H_

#include <windows.h>
#include <commctrl.h>
#include "CKeyBind.h"
#include "CKeyWordSetMgr.h"
#include "CPrint.h"
#include "CDataProfile.h"
#include "basis/SakuraBasis.h"
#include "config/maxdata.h"


struct EditNode {
	int				m_nIndex;
	int				m_nGroup;					/*!< グループID */							//@@@ 2007.06.20 ryoji
	HWND			m_hWnd;
	WIN_CHAR		m_szTabCaption[_MAX_PATH];	/*!< タブウインドウ用：キャプション名 */	//@@@ 2003.05.31 MIK
	SFilePath		m_szFilePath;	/*!< タブウインドウ用：ファイル名 */		//@@@ 2006.01.28 ryoji
	BOOL			m_bIsGrep;					/*!< Grepのウィンドウか */					//@@@ 2006.01.28 ryoji
	UINT			m_showCmdRestore;			/*!< 元のサイズに戻すときのサイズ種別 */	//@@@ 2007.06.20 ryoji
	BOOL			m_bClosing;					/*!< 終了中か（「最後のファイルを閉じても(無題)を残す」用） */	//@@@ 2007.06.20 ryoji

	HWND GetHwnd() const{ return m_hWnd; }
};


//@@@ 2001.12.26 YAZAKI CMRU, CMRUFolder
#include "CMRU.h"
#include "CMRUFolder.h"

//20020129 aroka
#include "funccode.h"
#include "CMemory.h"

#include "CMutex.h"	// 2007.07.07 genta

#include "CLineComment.h"	//@@@ 2002.09.22 YAZAKI
#include "CBlockComment.h"	//@@@ 2002.09.22 YAZAKI

#include "FileInfo.h"


/*!	検索オプション
	20020118 aroka
*/
struct GrepInfo {
	CNativeW		cmGrepKey;			//!< 検索キー
	CNativeT		cmGrepFile;			//!< 検索対象ファイル
	CNativeT		cmGrepFolder;		//!< 検索対象フォルダ
	SSearchOption	sGrepSearchOption;	//!< 検索オプション
	bool			bGrepSubFolder;		//!< サブフォルダを検索する
	bool			bGrepOutputLine;	//!< 結果出力で該当行を出力する
	int				nGrepOutputStyle;	//!< 結果出力形式
	ECodeType		nGrepCharSet;		//!< 文字コードセット
};


//! 印刷設定
#define POS_LEFT	0
#define POS_CENTER	1
#define POS_RIGHT	2
#define HEADER_MAX	100
#define FOOTER_MAX	HEADER_MAX
struct PRINTSETTING {
	TCHAR			m_szPrintSettingName[32 + 1];		/*!< 印刷設定の名前 */
	TCHAR			m_szPrintFontFaceHan[LF_FACESIZE];	/*!< 印刷フォント */
	TCHAR			m_szPrintFontFaceZen[LF_FACESIZE];	/*!< 印刷フォント */
	int				m_nPrintFontWidth;					/*!< 印刷フォント幅(1/10mm単位単位) */
	int				m_nPrintFontHeight;					/*!< 印刷フォント高さ(1/10mm単位単位) */
	int				m_nPrintDansuu;						/*!< 段組の段数 */
	int				m_nPrintDanSpace;					/*!< 段と段の隙間(1/10mm単位) */
	int				m_nPrintLineSpacing;				/*!< 印刷フォント行間 文字の高さに対する割合(%) */
	int				m_nPrintMarginTY;					/*!< 印刷用紙マージン 上(mm単位) */
	int				m_nPrintMarginBY;					/*!< 印刷用紙マージン 下(mm単位) */
	int				m_nPrintMarginLX;					/*!< 印刷用紙マージン 左(mm単位) */
	int				m_nPrintMarginRX;					/*!< 印刷用紙マージン 右(mm単位) */
	int				m_nPrintPaperOrientation;			/*!< 用紙方向 DMORIENT_PORTRAIT (1) または DMORIENT_LANDSCAPE (2) */
	int				m_nPrintPaperSize;					/*!< 用紙サイズ */
	BOOL			m_bPrintWordWrap;					/*!< 英文ワードラップする */
	BOOL			m_bPrintKinsokuHead;				/*!< 行頭禁則する */	//@@@ 2002.04.09 MIK
	BOOL			m_bPrintKinsokuTail;				/*!< 行末禁則する */	//@@@ 2002.04.09 MIK
	BOOL			m_bPrintKinsokuRet;					/*!< 改行文字のぶら下げ */	//@@@ 2002.04.13 MIK
	BOOL			m_bPrintKinsokuKuto;				/*!< 句読点のぶらさげ */	//@@@ 2002.04.17 MIK
	BOOL			m_bPrintLineNumber;					/*!< 行番号を印刷する */


	MYDEVMODE		m_mdmDevMode;						/*!< プリンタ設定 DEVMODE用 */
	BOOL			m_bHeaderUse[3];					/* ヘッダが使われているか？	*/
	EDIT_CHAR		m_szHeaderForm[3][HEADER_MAX];		/* 0:左寄せヘッダ。1:中央寄せヘッダ。2:右寄せヘッダ。*/
	BOOL			m_bFooterUse[3];					/* フッタが使われているか？	*/
	EDIT_CHAR		m_szFooterForm[3][FOOTER_MAX];		/* 0:左寄せフッタ。1:中央寄せフッタ。2:右寄せフッタ。*/
};


//! 色設定
struct ColorInfo {
	int			m_nColorIdx;
	BOOL		m_bDisp;			/* 色分け/表示 をする */
	BOOL		m_bFatFont;			/* 太字か */
	BOOL		m_bUnderLine;		/* アンダーラインか */
	COLORREF	m_colTEXT;			/* 前景色(文字色) */
	COLORREF	m_colBACK;			/* 背景色 */
	TCHAR		m_szName[32];		/* 名前 */
	wchar_t		m_cReserved[60];
};

//! 色設定(保存用)
struct ColorInfoIni {
	const TCHAR*	m_pszName;			/* 色名 */
	BOOL		m_bDisp;			/* 色分け/表示 をする */
	BOOL		m_bFatFont;			/* 太字か */
	BOOL		m_bUnderLine;		/* アンダーラインか */
	COLORREF	m_colTEXT;			/* 前景色(文字色) */
	COLORREF	m_colBACK;			/* 背景色 */
};

//@@@ 2001.11.17 add start MIK
struct RegexKeywordInfo {
	wchar_t	m_szKeyword[100];	//正規表現キーワード
	int	m_nColorIndex;		//色指定番号
};
//@@@ 2001.11.17 add end MIK

//@@@ 2006.04.10 fon ADD-start
const int DICT_ABOUT_LEN = 50; /*!< 辞書の説明の最大長 -1 */
struct KeyHelpInfo {
	int			m_nUse;						/*!< 辞書を 使用する/しない */
	TCHAR		m_szAbout[DICT_ABOUT_LEN];	/*!< 辞書の説明(辞書ファイルの1行目から生成) */
	SFilePath	m_szPath;					/*!< ファイルパス */
};
//@@@ 2006.04.10 fon ADD-end

//! タイプ別設定
struct Types {
	//2007.09.07 変数名変更: m_nMaxLineSize→m_nMaxLineKetas
	int					m_nIdx;
	TCHAR				m_szTypeName[64];				/*!< タイプ属性：名称 */
	TCHAR				m_szTypeExts[64];				/*!< タイプ属性：拡張子リスト */
	CLayoutInt			m_nMaxLineKetas;				/*!< 折り返し桁数 */
	int					m_nColmSpace;					/*!< 文字と文字の隙間 */
	int					m_nLineSpace;					/*!< 行間のすきま */
	CLayoutInt			m_nTabSpace;					/*!< TABの文字数 */
	int					m_bTabArrow;					/*!< タブ矢印表示 */	//@@@ 2003.03.26 MIK
	EDIT_CHAR			m_szTabViewString[17];			/*!< TAB表示文字列 */	// 2003.1.26 aroka サイズ拡張
	int					m_bInsSpace;					/* スペースの挿入 */	// 2001.12.03 hor
	// 2005.01.13 MIK 配列化
	int					m_nKeyWordSetIdx[MAX_KEYWORDSET_PER_TYPE];	/*!< キーワードセット */

	CLineComment		m_cLineComment;					/*!< 行コメントデリミタ */			//@@@ 2002.09.22 YAZAKI
	CBlockComment		m_cBlockComment;				/*!< ブロックコメントデリミタ */	//@@@ 2002.09.22 YAZAKI

	int					m_nStringType;					/*!< 文字列区切り記号エスケープ方法  0=[\"][\'] 1=[""][''] */
	wchar_t				m_szIndentChars[64];			/*!< その他のインデント対象文字 */

	int					m_nColorInfoArrNum;				/*!< 色設定配列の有効数 */
	ColorInfo			m_ColorInfoArr[64];				/*!< 色設定配列 */

	int					m_bLineNumIsCRLF;				/*!< 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
	int					m_nLineTermType;				/*!< 行番号区切り  0=なし 1=縦線 2=任意 */
	wchar_t				m_cLineTermChar;				/*!< 行番号区切り文字 */
	CLayoutInt			m_nVertLineIdx[MAX_VERTLINES];	/*!< 指定桁縦線 */

	BOOL				m_bWordWrap;					/*!< 英文ワードラップをする */
	BOOL				m_bKinsokuHead;					/*!< 行頭禁則をする */	//@@@ 2002.04.08 MIK
	BOOL				m_bKinsokuTail;					/*!< 行末禁則をする */	//@@@ 2002.04.08 MIK
	BOOL				m_bKinsokuRet;					/*!< 改行文字のぶら下げ */	//@@@ 2002.04.13 MIK
	BOOL				m_bKinsokuKuto;					/*!< 句読点のぶらさげ */	//@@@ 2002.04.17 MIK
	wchar_t				m_szKinsokuHead[200];			/*!< 行頭禁則文字 */	//@@@ 2002.04.08 MIK
	wchar_t				m_szKinsokuTail[200];			/*!< 行頭禁則文字 */	//@@@ 2002.04.08 MIK

	int					m_nCurrentPrintSetting;			/*!< 現在選択している印刷設定 */

	EOutlineType		m_nDefaultOutline;				/*!< アウトライン解析方法 */
	SFilePath			m_szOutlineRuleFilename;		/*!< アウトライン解析ルールファイル */

	int					m_nSmartIndent;					/*!< スマートインデント種別 */
	int					m_nImeState;	//	Nov. 20, 2000 genta 初期IME状態

	//	2001/06/14 asa-o 補完のタイプ別設定
	SFilePath			m_szHokanFile;					/*!< 入力補完 単語ファイル */
	//	2003.06.23 Moca ファイル内からの入力補完機能
	int					m_bUseHokanByFile;				/*!< 入力補完 開いているファイル内から候補を探す */
	//	2001/06/19 asa-o
	int					m_bHokanLoHiCase;				/*!< 入力補完機能：英大文字小文字を同一視する */

	SFilePath			m_szExtHelp;					/* 外部ヘルプ１ */
	SFilePath			m_szExtHtmlHelp;				/* 外部HTMLヘルプ */
	BOOL				m_bHtmlHelpIsSingle;			/* HtmlHelpビューアはひとつ */
	
	
//@@@ 2001.11.17 add start MIK
	BOOL	m_bUseRegexKeyword;	/* 正規表現キーワードを使うか*/
	int	m_nRegexKeyMagicNumber;	/* 正規表現キーワード更新マジックナンバー */
	struct RegexKeywordInfo	m_RegexKeywordArr[MAX_REGEX_KEYWORD];	/* 正規表現キーワード */
//@@@ 2001.11.17 add end MIK

//@@@ 2006.04.10 fon ADD-start
	BOOL				m_bUseKeyWordHelp;			/* キーワード辞書セレクト機能を使うか */
	int					m_nKeyHelpNum;					/* キーワード辞書の冊数 */
	struct	KeyHelpInfo	m_KeyHelpArr[MAX_KEYHELP_FILE];	/* キーワード辞書ファイル */
	BOOL				m_bUseKeyHelpAllSearch;			/* ヒットした次の辞書も検索(&A) */
	BOOL				m_bUseKeyHelpKeyDisp;			/* 1行目にキーワードも表示する(&W) */
	BOOL				m_bUseKeyHelpPrefix;			/* 選択範囲で前方一致検索(&P) */
//@@@ 2006.04.10 fon ADD-end

	//	2002/04/30 YAZAKI Commonから移動。
	BOOL				m_bAutoIndent;					/* オートインデント */
	BOOL				m_bAutoIndent_ZENSPACE;			/* 日本語空白もインデント */
	BOOL				m_bRTrimPrevLine;				/* 2005.10.11 ryoji 改行時に末尾の空白を削除 */
	int					m_nIndentLayout;				/* 折り返しは2行目以降を字下げ表示 */
	
	//	Sep. 10, 2002 genta
	int					m_bUseDocumentIcon;	/*!< ファイルに関連づけられたアイコンを使う */

}; /* Types */

//! マクロ情報
struct MacroRec {
	TCHAR	m_szName[MACRONAME_MAX];	//<! 表示名
	TCHAR	m_szFile[_MAX_PATH+1];	//<! ファイル名(ディレクトリを含まない)
	BOOL	m_bReloadWhenExecute;	//	実行時に読み込みなおすか（デフォルトon）
	
	bool IsEnabled() const { return m_szFile[0] != '\0'; }
};
//	To Here Sep. 14, 2001 genta

// 2004/06/21 novice タグジャンプ機能追加
//! タグジャンプ情報
struct TagJump {
	HWND		hwndReferer;				//<! 参照元ウィンドウ
	CLogicPoint	point;						//<! ライン, カラム
};

//	Aug. 15, 2000 genta
//	Backup Flags
const int BKUP_YEAR		= 32;
const int BKUP_MONTH	= 16;
const int BKUP_DAY		= 8;
const int BKUP_HOUR		= 4;
const int BKUP_MIN		= 2;
const int BKUP_SEC		= 1;



//	2004.05.13 Moca
//! ウィンドウサイズ・位置の制御方法
enum eWINSIZEMODE{
	WINSIZEMODE_DEF = 0, //!< 指定なし
	WINSIZEMODE_SAVE = 1, //!< 継承(保存)
	WINSIZEMODE_SET = 2   //!< 直接指定(固定)
};

//2007.09.28 kobake Common構造体を
#include "share/CommonSetting.h"


//! iniフォルダ設定	// 2007.05.31 ryoji
struct IniFolder {
	bool m_bInit;							// 初期化済フラグ
	bool m_bReadPrivate;					// マルチユーザ用iniからの読み出しフラグ
	bool m_bWritePrivate;					// マルチユーザ用iniへの書き込みフラグ
	TCHAR m_szIniFile[_MAX_PATH];			// EXE基準のiniファイルパス
	TCHAR m_szPrivateIniFile[_MAX_PATH];	// マルチユーザ用のiniファイルパス
};	/* iniフォルダ設定 */


#include "StaticType.h"

//! 共有データ領域
//2007.09.23 kobake m_nSEARCHKEYArrNum,      m_szSEARCHKEYArr      を m_aSearchKeys      にまとめました
//2007.09.23 kobake m_nREPLACEKEYArrNum,     m_szREPLACEKEYArr     を m_aReplaceKeys     にまとめました
//2007.09.23 kobake m_nGREPFILEArrNum,       m_szGREPFILEArr       を m_aGrepFiles       にまとめました
//2007.09.23 kobake m_nGREPFOLDERArrNum,     m_szGREPFOLDERArr     を m_aGrepFolders     にまとめました
//2007.09.23 kobake m_szCmdArr,              m_nCmdArrNum          を m_aCommands        にまとめました
//2007.09.23 kobake m_nTagJumpKeywordArrNum, m_szTagJumpKeywordArr を m_aTagJumpKeywords にまとめました
struct DLLSHAREDATA {
	//	Oct. 27, 2000 genta
	//!	データ構造 Version
	/*	データ構造の異なるバージョンの同時起動を防ぐため
		必ず先頭になくてはならない．
	*/
	unsigned int		m_vStructureVersion;

	/* 共通作業域(保存しない) */
	//2007.09.16 kobake char型だと、常に文字列であるという誤解を招くので、BYTE型に変更。変数名も変更。
	//           UNICODE版では、余分に領域を使うことが予想されるため、ANSI版の2倍確保。
private:
	BYTE				m_pWork[32000*sizeof(TCHAR)];
public:
	template <class T>
	T* GetWorkBuffer(){ return reinterpret_cast<T*>(m_pWork); }

	template <class T>
	size_t GetWorkBufferCount(){ return sizeof(m_pWork)/sizeof(T); }

public:
	FileInfo			m_FileInfo_MYWM_GETFILEINFO;

	DWORD				m_dwProductVersionMS;
	DWORD				m_dwProductVersionLS;
	HWND				m_hwndTray;
	HWND				m_hwndDebug;
	HACCEL				m_hAccel;
	LONG				m_nSequences;	/* ウィンドウ連番 */
	LONG				m_nGroupSequences;	// タブグループ連番	// 2007.06.20 ryoji
	/**** 共通作業域(保存する) ****/
	int					m_nEditArrNum;	//short->intに修正	//@@@ 2003.05.31 MIK
	EditNode			m_pEditArr[MAX_EDITWINDOWS];	//最大値修正	@@@ 2003.05.31 MIK

	//From Here 2003.05.31 MIK
	//WINDOWPLACEMENT		m_TabWndWndpl;					//タブウインドウ時のウインドウ情報
	//To Here 2003.05.31 MIK
	BOOL				m_bEditWndChanging;				// 編集ウィンドウ切替中	// 2007.04.03 ryoji

//@@@ 2001.12.26 YAZAKI	以下の2つは、直接アクセスしないでください。CMRUを経由してください。
	int					m_nMRUArrNum;
	FileInfo			m_fiMRUArr[MAX_MRU];
	bool				m_bMRUArrFavorite[MAX_MRU];	//お気に入り	//@@@ 2003.04.08 MIK

//@@@ 2001.12.26 YAZAKI	以下の2つは、直接アクセスしないでください。CMRUFolderを経由してください。
	int						m_nOPENFOLDERArrNum;
	StaticString<TCHAR,_MAX_PATH>	m_szOPENFOLDERArr[MAX_OPENFOLDER];
	bool					m_bOPENFOLDERArrFavorite[MAX_OPENFOLDER];	//お気に入り	//@@@ 2003.04.08 MIK

	int					m_nTransformFileNameArrNum;
	TCHAR				m_szTransformFileNameFrom[MAX_TRANSFORM_FILENAME][_MAX_PATH];
	TCHAR				m_szTransformFileNameTo[MAX_TRANSFORM_FILENAME][_MAX_PATH];	//お気に入り	//@@@ 2003.04.08 MIK

	StaticVector< StaticString<WCHAR, _MAX_PATH>, MAX_SEARCHKEY,  const WCHAR*>	m_aSearchKeys;
	StaticVector< StaticString<WCHAR, _MAX_PATH>, MAX_REPLACEKEY, const WCHAR*>	m_aReplaceKeys;
	StaticVector< StaticString<TCHAR, _MAX_PATH>, MAX_GREPFILE,   const TCHAR*>	m_aGrepFiles;
	StaticVector< StaticString<TCHAR, _MAX_PATH>, MAX_GREPFOLDER, const TCHAR*>	m_aGrepFolders;

	SFilePath			m_szMACROFOLDER;		/* マクロ用フォルダ */
	SFilePath			m_szIMPORTFOLDER;	// 設定インポート用フォルダ
	
	//	Sep. 14, 2001 genta
	MacroRec			m_MacroTable[MAX_CUSTMACRO];	//!< キー割り当て用マクロテーブル

	// 2004/06/21 タグジャンプ機能追加
	int					m_TagJumpNum;					//!< タグジャンプ情報の有効データ数
	int					m_TagJumpTop;					//!< スタックの一番上の位置
	TagJump				m_TagJump[MAX_TAGJUMPNUM];		//!< タグジャンプ情報


	StaticVector< StaticString<TCHAR, MAX_CMDLEN>, MAX_CMDARR > m_aCommands;

	/**** iniフォルダ設定 ****/
	IniFolder			m_IniFolder;

	/**** 共通設定 ****/
	CommonSetting		m_Common;

	/* キー割り当て */
	int					m_nKeyNameArrNum;			/* キー割り当て表の有効データ数 */
	KEYDATA				m_pKeyNameArr[100];			/* キー割り当て表 */

	/**** 印刷ページ設定 ****/
	PRINTSETTING		m_PrintSettingArr[MAX_PRINTSETTINGARR];

	/* 強調キーワード設定 */
	CKeyWordSetMgr		m_CKeyWordSetMgr;					/* 強調キーワード */
	char				m_szKeyWordSetDir[MAX_PATH];		/* 強調キーワードファイルのディレクトリ */

	/* **** タイプ別設定 **** */
	Types				m_Types[MAX_TYPES];

	/*	@@@ 2002.1.24 YAZAKI
		キーボードマクロは、記録終了した時点でファイル「m_szKeyMacroFileName」に書き出すことにする。
		m_bRecordingKeyMacroがTRUEのときは、キーボードマクロの記録中なので、m_szKeyMacroFileNameにアクセスしてはならない。
	*/
	BOOL				m_bRecordingKeyMacro;		/* キーボードマクロの記録中 */
	HWND				m_hwndRecordingKeyMacro;	/* キーボードマクロを記録中のウィンドウ */
	TCHAR				m_szKeyMacroFileName[MAX_PATH];	/* キーボードマクロのファイル名 */

//@@@ 2002.01.08 YAZAKI 設定を保存するためにShareDataに移動
	/* **** その他のダイアログ **** */
//	BOOL				m_bGetStdout;		/* 外部コマンド実行の「標準出力を得る」 */
	int					m_nExecFlgOpt;		/* 外部コマンド実行オプション */	//	2006.12.03 maru オプションの拡張のため
	BOOL				m_bLineNumIsCRLF;	/* 指定行へジャンプの「改行単位の行番号」か「折り返し単位の行番号」か */

	int					m_nDiffFlgOpt;		/* DIFF差分表示 */	//@@@ 2002.05.27 MIK
	
	TCHAR				m_szTagsCmdLine[_MAX_PATH];	/* TAGSコマンドラインオプション */	//@@@ 2003.05.12 MIK
	int					m_nTagsOpt;			/* TAGSオプション(チェック) */	//@@@ 2003.05.12 MIK

	//From Here 2005.04.03 MIK キーワード指定タグジャンプ
	StaticVector< StaticString<WCHAR, _MAX_PATH>, MAX_TAGJUMP_KEYWORD >		m_aTagJumpKeywords;

	BOOL				m_bTagJumpICase;	//!< 大文字小文字を同一視
	BOOL				m_bTagJumpAnyWhere;	//!< 文字列の途中にマッチ
	//To Here 2005.04.03 MIK
};



/*!	@brief 共有データの管理

	CShareDataはCProcessのメンバであるため，両者の寿命は同一です．
	本来はCProcessオブジェクトを通じてアクセスするべきですが，
	CProcess内のデータ領域へのポインタをstatic変数に保存することで
	Singletonのようにどこからでもアクセスできる構造になっています．

	共有メモリへのポインタをm_pShareDataに保持します．このメンバは
	公開されていますが，CShareDataによってMap/Unmapされるために
	ChareDataの消滅によってポインタm_pShareDataも無効になることに
	注意してください．

	@date 2002.01.03 YAZAKI m_tbMyButtonなどをCShareDataからCMenuDrawerへ移動したことによる修正。
*/
class SAKURA_CORE_API CShareData
{
public:
	/*
	||	Singleton風
	*/
	static CShareData* getInstance();

protected:
	static CShareData* _instance;
	static CMutex g_cEditArrMutex;

public:
	/*
	||  Constructors
	*/
	CShareData();
	~CShareData();

	/*
	||  Attributes & Operations
	*/
	bool Init(void);	/* CShareDataクラスの初期化処理 */
	DLLSHAREDATA* GetShareData(){ return m_pShareData; }		/* 共有データ構造体のアドレスを返す */
	int GetDocumentType( const TCHAR* pszFilePath );			/* ファイルパスを渡して、ドキュメントタイプ（数値）を取得する */
	int GetDocumentTypeExt( const TCHAR* pszExt );				/* 拡張子を渡して、ドキュメントタイプ（数値）を取得する */
	
	BOOL AddEditWndList( HWND, int nGroup = 0 );				/* 編集ウィンドウの登録 */	// 2007.06.26 ryoji nGroup引数追加
	void DeleteEditWndList( HWND );								/* 編集ウィンドウリストからの削除 */
	
	void ResetGroupId( void );									/* グループをIDリセットする */
	EditNode* GetEditNode( HWND hWnd );							/* 編集ウィンドウ情報を取得する */
	int GetGroupId( HWND hWnd );								/* グループIDを取得する */
	bool IsSameGroup( HWND hWnd1, HWND hWnd2 );					/* 同一グループかどうかを調べる */
	bool ReorderTab( HWND hSrcTab, HWND hDstTab );				/* タブ移動に伴うウィンドウの並び替え 2007.07.07 genta */
	HWND SeparateGroup( HWND hwndSrc, HWND hwndDst, bool bSrcIsTop, int notifygroups[] );/* タブ分離に伴うウィンドウ処理 2007.07.07 genta */
	EditNode* GetEditNodeAt( int nGroup, int nIndex );			/* 指定位置の編集ウィンドウ情報を取得する */
	EditNode* GetTopEditNode( HWND hWnd );						/* 先頭の編集ウィンドウ情報を取得する */
	HWND GetTopEditWnd( HWND hWnd );							/* 先頭の編集ウィンドウを取得する */
	bool IsTopEditWnd( HWND hWnd ){ return (GetTopEditWnd( hWnd ) == hWnd); }	/* 先頭の編集ウィンドウかどうかを調べる */

	BOOL RequestCloseAllEditor( BOOL bExit, int nGroup );		/* 全編集ウィンドウへ終了要求を出す */	// 2007.02.13 ryoji 「編集の全終了」を示す引数(bExit)を追加	// 2007.06.20 ryoji nGroup引数追加
	BOOL IsPathOpened( const TCHAR* pszPath, HWND* phwndOwner ); /* 指定ファイルが開かれているか調べる */
	BOOL IsPathOpened( const TCHAR* pszPath, HWND* phwndOwner, ECodeType nCharCode );/* 指定ファイルが開かれているか調べつつ、多重オープン時の文字コード衝突も確認 */	// 2007.03.16
	int GetEditorWindowsNum( int nGroup );						/* 現在の編集ウィンドウの数を調べる */	// 2007.06.20 ryoji nGroup引数追加
	BOOL PostMessageToAllEditors( UINT uMsg, WPARAM wParam, LPARAM lParam, HWND hWndLast, int nGroup = 0 );	/* 全編集ウィンドウへメッセージをポストする */	// 2007.06.20 ryoji nGroup引数追加
	BOOL SendMessageToAllEditors( UINT uMsg, WPARAM wParam, LPARAM lParam, HWND hWndLast, int nGroup = 0 );	/* 全編集ウィンドウへメッセージを送るする */	// 2007.06.20 ryoji nGroup引数追加
	int GetOpenedWindowArr( EditNode** , BOOL, BOOL bGSort = FALSE );				/* 現在開いている編集ウィンドウの配列を返す */
	static BOOL IsEditWnd( HWND );								/* 指定ウィンドウが、編集ウィンドウのフレームウィンドウかどうか調べる */
	static void SetKeyNameArrVal(
		DLLSHAREDATA*, int, short, TCHAR*,
		EFunctionCode, EFunctionCode, EFunctionCode, EFunctionCode,
		EFunctionCode, EFunctionCode, EFunctionCode, EFunctionCode );									/* KEYDATA配列にデータをセット */
//	static void SetKeyNameArrVal( DLLSHAREDATA*, int, short, char* );	/* KEYDATA配列にデータをセット */ // 20050818 aroka 未使用なので削除
	static LONG MY_RegSetVal(
		HKEY hKey,				// handle of key to set value for
		LPCTSTR lpValueName,	// address of value to set
		CONST BYTE *lpData,		// address of value data
		DWORD cbData 			// size of value data
	);
	static LONG MY_RegQuerVal(
		HKEY hKey,				// handle of key to set value for
		LPCTSTR lpValueName,	// address of value to set
		BYTE *lpData,			// address of value data
		DWORD cbData 			// size of value data
	);
	void TraceOut( LPCTSTR lpFmt, ...);	/* デバッグモニタに出力 */
	void SetTraceOutSource( HWND hwnd ){ m_hwndTraceOutSource = hwnd; }	/* TraceOut起動元ウィンドウの設定 */
	BOOL LoadShareData( void );	/* 共有データのロード */
	void SaveShareData( void );	/* 共有データの保存 */
	static void GetIniFileNameDirect( LPTSTR pszPrivateIniFile, LPTSTR pszIniFile );	/* 構成設定ファイルからiniファイル名を取得する */	// 2007.09.04 ryoji
	void GetIniFileName( LPTSTR pszIniFileName, BOOL bRead = FALSE );	/* iniファイル名の取得 */	// 2007.05.19 ryoji
	BOOL IsPrivateSettings( void ){ return m_pShareData->m_IniFolder.m_bWritePrivate; }			/* iniファイルの保存先がユーザ別設定フォルダかどうか */	// 2007.05.25 ryoji
	BOOL ShareData_IO_2( bool );	/* 共有データの保存 */
	static void IO_ColorSet( CDataProfile* , const WCHAR* , ColorInfo* );	/* 色設定 I/O */ // Feb. 12, 2006 D.S.Koba

//	int			m_nStdToolBarButtons; 2004.03.30 Moca 未使用

	//@@@ 2002.2.2 YAZAKI
	//	Jun. 14, 2003 genta 引数追加．書式変更
	int		GetMacroFilename( int idx, TCHAR* pszPath, int nBufLen ); // idxで指定したマクロファイル名（フルパス）を取得する
	bool		BeReloadWhenExecuteMacro( int idx );	//	idxで指定したマクロは、実行するたびにファイルを読み込む設定か？
	void		AddToSearchKeyArr( const wchar_t* pszSearchKey );	//	m_aSearchKeysにpszSearchKeyを追加する
	void		AddToReplaceKeyArr( const wchar_t* pszReplaceKey );	//	m_aReplaceKeysにpszReplaceKeyを追加する
	void		AddToGrepFileArr( const TCHAR* pszGrepFile );		//	m_aGrepFilesにpszGrepFileを追加する
	void		AddToGrepFolderArr( const TCHAR* pszGrepFolder );	//	m_aGrepFolders.size()にpszGrepFolderを追加する

	//@@@ 2002.2.3 YAZAKI
	bool		ExtWinHelpIsSet( int nType = -1 );	//	タイプがnTypeのときに、外部ヘルプが設定されているか。
	const TCHAR*	GetExtWinHelp( int nType = -1 );	//	タイプがnTypeのときの、外部ヘルプファイル名を取得。
	bool		ExtHTMLHelpIsSet( int nType = -1 );	//	タイプがnTypeのときに、外部HTMLヘルプが設定されているか。
	const TCHAR*	GetExtHTMLHelp( int nType = -1 );	//	タイプがnTypeのときの、外部HTMLヘルプファイル名を取得。
	bool		HTMLHelpIsSingle( int nType = -1 );	//	タイプがnTypeのときの、外部HTMLヘルプ「ビューアを複数起動しない」がONかを取得。
	
	//@@@ 2002.2.9 YAZAKI
	const TCHAR* MyGetDateFormat( const SYSTEMTIME& systime, TCHAR* pszDest, int nDestLen );
	const TCHAR* MyGetTimeFormat( const SYSTEMTIME& systime, TCHAR* pszDest, int nDestLen );
	const TCHAR* MyGetDateFormat( const SYSTEMTIME& systime, TCHAR* pszDest, int nDestLen, int nDateFormatType, const TCHAR* szDateFormat );
	const TCHAR* MyGetTimeFormat( const SYSTEMTIME& systime, TCHAR* pszDest, int nDestLen, int nTimeFormatType, const TCHAR* szTimeFormat );
	
	// 2002.11.24 Moca Add
//	LPTSTR GetTransformFileList( LPCTSTR*, LPTSTR*, int );
//	LPTSTR GetTransformFileName( LPCTSTR, LPTSTR, int );
	LPTSTR GetTransformFileNameFast( LPCTSTR, LPTSTR, int );
	int TransformFileName_MakeCache( void );
	static LPCTSTR GetFilePathFormat( LPCTSTR, LPTSTR, int, LPCTSTR, LPCTSTR );
	static bool ExpandMetaToFolder( LPCTSTR, LPTSTR, int );

	// 2004/06/21 novice タグジャンプ機能追加
	void PushTagJump(const TagJump *);		//!< タグジャンプ情報の保存
	bool PopTagJump(TagJump *);				//!< タグジャンプ情報の参照

protected:
	/*
	||  実装ヘルパ関数
	*/
	HANDLE			m_hFileMap;
	DLLSHAREDATA*	m_pShareData;
	HWND			m_hwndTraceOutSource;	// TraceOutA()起動元ウィンドウ（いちいち起動元を指定しなくてすむように）

//	long GetModuleDir(char* , long );	/* この実行ファイルのあるディレクトリを返します */
	/* MRUとOPENFOLDERリストの存在チェックなど
	存在しないファイルやフォルダはMRUやOPENFOLDERリストから削除する
	 */
//@@@ 2002.01.03 YAZAKI CMRU、CMRUFolderに移動した。
//	void CheckMRUandOPENFOLDERList( void );

	// ファイル名簡易表示用キャッシュ
	int m_nTransformFileNameCount; // 有効数
	TCHAR m_szTransformFileNameFromExp[MAX_TRANSFORM_FILENAME][_MAX_PATH];
	int m_nTransformFileNameOrgId[MAX_TRANSFORM_FILENAME];

	//	Jan. 30, 2005 genta 初期化関数の分割
	void InitKeyword(DLLSHAREDATA*);
	void InitKeyAssign(DLLSHAREDATA*);
	void InitToolButtons(DLLSHAREDATA*);
	void InitTypeConfig(DLLSHAREDATA*);
	void InitPopupMenu(DLLSHAREDATA*);
	
	// Feb. 12, 2006 D.S.Koba
	void ShareData_IO_Mru( CDataProfile& );
	void ShareData_IO_Keys( CDataProfile& );
	void ShareData_IO_Grep( CDataProfile& );
	void ShareData_IO_Folders( CDataProfile& );
	void ShareData_IO_Cmd( CDataProfile& );
	void ShareData_IO_Nickname( CDataProfile& );
	void ShareData_IO_Common( CDataProfile& );
	void ShareData_IO_Toolbar( CDataProfile& );
	void ShareData_IO_CustMenu( CDataProfile& );
	void ShareData_IO_Font( CDataProfile& );
	void ShareData_IO_KeyBind( CDataProfile& );
	void ShareData_IO_Print( CDataProfile& );
	void ShareData_IO_Types( CDataProfile& );
	void ShareData_IO_KeyWords( CDataProfile& );
	void ShareData_IO_Macro( CDataProfile& );
	void ShareData_IO_Other( CDataProfile& );

	int GetOpenedWindowArrCore( EditNode** , BOOL, BOOL bGSort = FALSE );			/* 現在開いている編集ウィンドウの配列を返す（コア処理部） */
};



//! どこからでもアクセスできる、共有データアクセサ。2007.10.30 kobake
DLLSHAREDATA& GetDllShareData();

///////////////////////////////////////////////////////////////////////
#endif /* _CSHAREDATA_H_ */


/*[EOF]*/
