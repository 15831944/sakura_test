/*!	@file
	@brief 印刷関連

	@author Norio Nakatani
	@date 1998/06/09 新規作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2003, かろと

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

class CPrint;

#ifndef _CPRINT_H_
#define _CPRINT_H_

#include <winspool.h>

struct	MYDEVMODE {
	BOOL	m_bPrinterNotFound;	/* プリンタがなかったフラグ */
	char	m_szPrinterDriverName[_MAX_PATH + 1];	/* プリンタドライバ名 */
	char	m_szPrinterDeviceName[_MAX_PATH + 1];				/* プリンタデバイス名 */
	char	m_szPrinterOutputName[_MAX_PATH + 1];	/* プリンタポート名 */
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
	int m_nId;	//!< 用紙ID
	int m_nAllWidth;	//!< 幅 (0.1mm単位)
	int m_nAllHeight;	//!< 高さ (0.1mm単位)
	const char* m_pszName; //!< 用紙名称
};

struct PRINTSETTING;


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
	static void SettingInitialize( PRINTSETTING&, const char* settingName );

	static char* GetPaperName( int , char* );	/* 用紙の名前を取得 */
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
		char*		pszErrMsg			/* エラーメッセージ格納場所 */
	);


	/* 印刷 ジョブ開始 */
	BOOL PrintOpen(
		char*		pszJobName,
		MYDEVMODE*	pMYDEVMODE,
		HDC*		phdc,
		char*		pszErrMsg		/* エラーメッセージ格納場所 */
	);
	void PrintStartPage( HDC );	/* 印刷 ページ開始 */
	void PrintEndPage( HDC );	/* 印刷 ページ終了 */
	void PrintClose( HDC );		/* 印刷 ジョブ終了 */ // 2003.05.02 かろと 不要なhPrinter削除

protected:
	/*
	||  実装ヘルパ関数
	*/
	// DC作成する(処理をまとめた) 2003.05.02 かろと
	HDC CreateDC( MYDEVMODE *pMYDEVMODE, char *pszErrMsg);
	
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
