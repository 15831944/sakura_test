/*!	@file
	@brief 印刷関連

	@author Norio Nakatani
	@date 1998/06/09 新規作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2003, かろと
	Copyright (C) 2006, Moca

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

class CPrint;

#ifndef _CPRINT_H_
#define _CPRINT_H_

#include <winspool.h>
#include <commdlg.h> // PRINTDLG

struct	MYDEVMODE {
	BOOL	m_bPrinterNotFound;	/* プリンタがなかったフラグ */
	TCHAR	m_szPrinterDriverName[_MAX_PATH + 1];	// プリンタドライバ名
	TCHAR	m_szPrinterDeviceName[_MAX_PATH + 1];	// プリンタデバイス名
	TCHAR	m_szPrinterOutputName[_MAX_PATH + 1];	// プリンタポート名
	DWORD	dmFields;
	short	dmOrientation;
	short	dmPaperSize;
	short	dmPaperLength;
	short	dmPaperWidth;
	short	dmScale;
	short	dmCopies;
	short	dmDefaultSource;
	short	dmPrintQuality;
	short	dmColor;
	short	dmDuplex;
	short	dmYResolution;
	short	dmTTOption;
	short	dmCollate;
	BCHAR	dmFormName[CCHFORMNAME];
	WORD	dmLogPixels;
	DWORD	dmBitsPerPel;
	DWORD	dmPelsWidth;
	DWORD	dmPelsHeight;
	DWORD	dmDisplayFlags;
	DWORD	dmDisplayFrequency;
};

// 2006.08.14 Moca 用紙情報の統合 PAPER_INFO新設
//! 用紙情報
struct PAPER_INFO {
	int				m_nId;			//!< 用紙ID
	int				m_nAllWidth;	//!< 幅 (0.1mm単位)
	int				m_nAllHeight;	//!< 高さ (0.1mm単位)
	const TCHAR*	m_pszName;		//!< 用紙名称
};

struct PRINTSETTING;




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
	bool			m_bPrintWordWrap;					//!< 英文ワードラップする
	bool			m_bPrintKinsokuHead;				//!< 行頭禁則する		//@@@ 2002.04.09 MIK
	bool			m_bPrintKinsokuTail;				//!< 行末禁則する		//@@@ 2002.04.09 MIK
	bool			m_bPrintKinsokuRet;					//!< 改行文字のぶら下げ	//@@@ 2002.04.13 MIK
	bool			m_bPrintKinsokuKuto;				//!< 句読点のぶらさげ	//@@@ 2002.04.17 MIK
	BOOL			m_bPrintLineNumber;					/*!< 行番号を印刷する */


	MYDEVMODE		m_mdmDevMode;						/*!< プリンタ設定 DEVMODE用 */
	BOOL			m_bHeaderUse[3];					/* ヘッダが使われているか？	*/
	char			m_szHeaderForm[3][HEADER_MAX];		/* 0:左寄せヘッダ。1:中央寄せヘッダ。2:右寄せヘッダ。*/
	BOOL			m_bFooterUse[3];					/* フッタが使われているか？	*/
	char			m_szFooterForm[3][FOOTER_MAX];		/* 0:左寄せフッタ。1:中央寄せフッタ。2:右寄せフッタ。*/
};


/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
/*!
	@brief 印刷関連機能

	オブジェクト指向でないクラス
*/
class CPrint
{
public:
	static const PAPER_INFO m_paperInfoArr[];	//!< 用紙情報一覧
	static const int m_nPaperInfoArrNum; //!< 用紙情報一覧の要素数


	/*
	||	static関数群
	*/
	static void SettingInitialize( PRINTSETTING&, const TCHAR* settingName );

	static TCHAR* GetPaperName( int , TCHAR* );	/* 用紙の名前を取得 */
	/* 用紙の幅、高さ */
	static BOOL GetPaperSize(
		int*		pnPaperAllWidth,
		int*		pnPaperAllHeight,
		MYDEVMODE*	pDEVMODE
	);

public:
	/*
	||  Constructors
	*/
	CPrint();
	~CPrint();

	/*
	||  Attributes & Operations
	*/
	BOOL GetDefaultPrinter( MYDEVMODE *pMYDEVMODE );		/* デフォルトのプリンタ情報を取得 */
	BOOL PrintDlg( PRINTDLG *pd, MYDEVMODE *pMYDEVMODE );				/* プリンタ情報を取得 */
	/* 印刷/プレビューに必要な情報を取得 */
	BOOL GetPrintMetrics(
		MYDEVMODE*	pMYDEVMODE,
		int*		pnPaperAllWidth,	/* 用紙幅 */
		int*		pnPaperAllHeight,	/* 用紙高さ */
		int*		pnPaperWidth,		/* 用紙印刷可能幅 */
		int*		pnPaperHeight,		/* 用紙印刷可能高さ */
		int*		pnPaperOffsetLeft,	/* 用紙余白左端 */
		int*		pnPaperOffsetTop,	/* 用紙余白上端 */
		TCHAR*		pszErrMsg			/* エラーメッセージ格納場所 */
	);


	/* 印刷 ジョブ開始 */
	BOOL PrintOpen(
		TCHAR*		pszJobName,
		MYDEVMODE*	pMYDEVMODE,
		HDC*		phdc,
		TCHAR*		pszErrMsg		/* エラーメッセージ格納場所 */
	);
	void PrintStartPage( HDC );	/* 印刷 ページ開始 */
	void PrintEndPage( HDC );	/* 印刷 ページ終了 */
	void PrintClose( HDC );		/* 印刷 ジョブ終了 */ // 2003.05.02 かろと 不要なhPrinter削除

protected:
	/*
	||  実装ヘルパ関数
	*/
	// DC作成する(処理をまとめた) 2003.05.02 かろと
	HDC CreateDC( MYDEVMODE *pMYDEVMODE, TCHAR *pszErrMsg);
	
	static const PAPER_INFO* FindPaperInfo( int id );
private:
	/*
	||  メンバ変数
	*/
	HGLOBAL	m_hDevMode;							//!< 現在プリンタのDEVMODEへのメモリハンドル
	HGLOBAL	m_hDevNames;						//!< 現在プリンタのDEVNAMESへのメモリハンドル
};



///////////////////////////////////////////////////////////////////////
#endif /* _CPRINT_H_ */


/*[EOF]*/
