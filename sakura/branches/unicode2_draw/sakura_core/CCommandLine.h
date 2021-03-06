/*!	@file
	@brief コマンドラインパーサ ヘッダファイル

	@author aroka
	@date	2002/01/08 作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000-2001, genta
	Copyright (C) 2002, aroka CControlTrayより分離
	Copyright (C) 2002, genta
	Copyright (C) 2005, D.S.Koba
	Copyright (C) 2007, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#ifndef _CCOMMANDLINE_H_
#define _CCOMMANDLINE_H_

#include "global.h"
#include "EditInfo.h"
class CMemory;
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


/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/

/*!
	@brief コマンドラインパーサ クラス
*/
class SAKURA_CORE_API CCommandLine {
public:
	static CCommandLine* Instance(LPTSTR cmd=NULL);

private:
	// 2005-08-24 D.S.Koba 引数削除
	void ParseCommandLine( void );
	
	static int CheckCommandLine(
		LPTSTR	str,		//!< [in] 検証する文字列（先頭の-は含まない）
		int		quotelen,	//!< [in] オプション末尾の引用符の長さ．オプション全体が引用符で囲まれている場合の考慮．
		TCHAR**	arg			//!< [out] 引数がある場合はその先頭へのポインタ
	);
	
	// 外から作らせない。
	CCommandLine();
	CCommandLine(LPTSTR cmd);

	/*!
		引用符で囲まれている数値を認識するようにする
		@date 2002.12.05 genta
	*/
	static int AtoiOptionInt(const TCHAR* arg){
		return ( arg[0] == _T('"') || arg[0] == _T('\'') ) ?
			_ttoi( arg + 1 ) : _ttoi( arg );
	}

// member accessor method
public:
	bool IsNoWindow() const {return m_bNoWindow;};
	bool IsWriteQuit() const {return m_bWriteQuit;};	// 2007.05.19 ryoji sakuext用に追加
	bool IsGrepMode() const {return m_bGrepMode;};
	bool IsGrepDlg() const {return m_bGrepDlg;};
	bool IsDebugMode() const {return m_bDebugMode;};
	bool IsViewMode() const {return m_bViewMode;};
	bool GetEditInfo(EditInfo* fi) const { *fi = m_fi; return true; }
	bool GetGrepInfo(GrepInfo* gi) const { *gi = m_gi; return true; }
	int GetGroupId() const {return m_nGroup;};	// 2007.06.26 ryoji

// member valiables
private:
	static CCommandLine* _instance;
	LPCTSTR		m_pszCmdLineSrc;	//! [in]コマンドライン文字列
	bool		m_bGrepMode;		//! [out] TRUE: Grep Mode
	bool		m_bGrepDlg;			//  Grepダイアログ
	bool		m_bDebugMode;		
	bool		m_bNoWindow;		//! [out] TRUE: 編集Windowを開かない
	bool		m_bWriteQuit;		//! [out] TRUE: 設定を保存して終了	// 2007.05.19 ryoji sakuext用に追加
	EditInfo	m_fi;				//!
	GrepInfo	m_gi;				//!
	bool		m_bViewMode;	//! [out] TRUE: Read Only
	int			m_nGroup;			//! グループID	// 2007.06.26 ryoji
};

///////////////////////////////////////////////////////////////////////
#endif /* _CCOMMANDLINE_H_ */



