/*!	@file
	@brief キー割り当てに関するクラス

	@author Norio Nakatani
	@date 1998/03/25 新規作成
	@date 1998/05/16 クラス内にデータを持たないように変更
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2007, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/
class CKeyBind;

#ifndef _CKEYBIND_H_
#define _CKEYBIND_H_

#include <windows.h>
class CMemory;// 2002/2/10 aroka

//! キー情報を保持する
struct KEYDATA {
	/*! キーコード	*/
	short			m_nKeyCode;
	
	/*!	キーの名前	*/
	TCHAR			m_szKeyName[64];
	
	/*!	対応する機能番号

		SHIFT, CTRL, ALTの３つのシフト状態のそれぞれに対して
		機能を割り当てるため、配列になっている。
	*/
	short	m_nFuncCodeArr[8]; // 2012.11.8 aroka KEYDATAINITとサイズを合わせる
};

class CFuncLookup;


/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
/*!
	@brief キー割り当て関連ルーチン
	
	すべての関数はstaticで保持するデータはない。
*/
class CKeyBind
{
public:
	/*
	||  Constructors
	*/
	CKeyBind();
	~CKeyBind();

	/*
	||  参照系メンバ関数
	*/
	static HACCEL CreateAccerelator( int, KEYDATA* );
	static int GetFuncCode( WORD nAccelCmd, int nKeyNameArrNum, KEYDATA* pKeyNameArr, BOOL bGetDefFuncCode = TRUE );
	static int GetFuncCodeAt( KEYDATA& KeyData, int nState, BOOL bGetDefFuncCode = TRUE );	/* 特定のキー情報から機能コードを取得する */	// 2007.02.24 ryoji
	static int GetDefFuncCode( int nKeyCode, int nState );	/* キーのデフォルト機能を取得する */	// 2007.02.22 ryoji

	//! キー割り当て一覧を作成する
	static int CreateKeyBindList( HINSTANCE hInstance, int nKeyNameArrNum, KEYDATA* pKeyNameArr, CMemory& cMemList, CFuncLookup* pcFuncLookup, BOOL bGetDefFuncCode = TRUE );
	static int GetKeyStr( HINSTANCE hInstance, int nKeyNameArrNum, KEYDATA* pKeyNameArr, CMemory& cMemList, int nFuncId, BOOL bGetDefFuncCode = TRUE );	/* 機能に対応するキー名の取得 */
	static int GetKeyStrList( HINSTANCE	hInstance, int nKeyNameArrNum,KEYDATA* pKeyNameArr, CMemory*** pppcMemList, int nFuncId, BOOL bGetDefFuncCode = TRUE );	/* 機能に対応するキー名の取得(複数) */
	static TCHAR* GetMenuLabel( HINSTANCE hInstance, int nKeyNameArrNum, KEYDATA* pKeyNameArr, int nFuncId, TCHAR* pszLabel, const TCHAR* pszKey, BOOL bKeyStr, BOOL bGetDefFuncCode = TRUE );	/* メニューラベルの作成 */

	static TCHAR* MakeMenuLabel(const TCHAR* sName, const TCHAR* sKey);

protected:
	/*
	||  実装ヘルパ関数
	*/
	static bool GetKeyStrSub(int& nKeyNameArrBegin, int nKeyNameArrEnd, KEYDATA* pKeyNameArr,
			int nShiftState, CMemory& cMemList, int nFuncId, BOOL bGetDefFuncCode );
};



///////////////////////////////////////////////////////////////////////
#endif /* _CKEYBIND_H_ */


/*[EOF]*/
