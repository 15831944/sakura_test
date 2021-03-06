/*!	@file
	@brief キーボードマクロ

	@author Norio Nakatani
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001, aroka
	Copyright (C) 2002, YAZAKI, genta

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#ifndef _CKEYMACROMGR_H_
#define _CKEYMACROMGR_H_

#include <windows.h>
#include "CMacroManagerBase.h"

enum EFunctionCode;
class CMacro;

//#define MAX_STRLEN			70
//#define MAX_KEYMACRONUM		10000
/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
//! キーボードマクロ
/*!
	キーボードマクロクラス
*/
class CKeyMacroMgr : public CMacroManagerBase
{
public:
	/*
	||  Constructors
	*/
	CKeyMacroMgr();
	~CKeyMacroMgr();

	/*
	||  Attributes & Operations
	*/
	void ClearAll( void );				/* キーマクロのバッファをクリアする */
	void Append( EFunctionCode , LPARAM, class CEditView* pcEditView );		/* キーマクロのバッファにデータ追加 */
	void Append( class CMacro* macro );		/* キーマクロのバッファにデータ追加 */
	
	/* キーボードマクロをまとめて取り扱う */
	BOOL SaveKeyMacro( HINSTANCE hInstance, const TCHAR* pszPath) const;	/* CMacroの列を、キーボードマクロに保存 */
	//@@@2002.2.2 YAZAKI PPA.DLLアリ/ナシ共存のためvirtualに。
	virtual void ExecKeyMacro( class CEditView* pcEditView ) const;				/* キーボードマクロの実行 */
	virtual BOOL LoadKeyMacro( HINSTANCE hInstance, const TCHAR* pszPath);		/* キーボードマクロを読み込み、CMacroの列に変換 */
	
	// Apr. 29, 2002 genta
	static CMacroManagerBase* Creator(const TCHAR* ext);
	static void declare(void);

protected:
	CMacro*	m_pTop;	//	先頭と終端を保持
	CMacro*	m_pBot;
};



///////////////////////////////////////////////////////////////////////
#endif /* _CKEYMACROMGR_H_ */


/*[EOF]*/
