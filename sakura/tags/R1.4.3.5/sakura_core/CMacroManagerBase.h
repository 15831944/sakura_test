//	$Id$
/*!	@file
	@brief マクロエンジン

	@author genta
	$Revision$
*/
/*
	Copyright (C) 2002, genta

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose, 
	including commercial applications, and to alter it and redistribute it 
	freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented;
		   you must not claim that you wrote the original software.
		   If you use this software in a product, an acknowledgment
		   in the product documentation would be appreciated but is
		   not required.

		2. Altered source versions must be plainly marked as such, 
		   and must not be misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		   distribution.
*/

#ifndef __CMACROMGR_BASE_H_
#define __CMACROMGR_BASE_H_

#include <windows.h>
class CEditView;
/*!
	@brief マクロを処理するエンジン部分の基底クラス

*/
class CMacroManagerBase {
public:

	/*! キーボードマクロの実行
	
		@param pcEditView [in] マクロ実行対象の編集ウィンドウ
	*/
	virtual void ExecKeyMacro( class CEditView* pcEditView ) const = 0;
	
	/*! キーボードマクロを読み込む

		@param hInstance [in]
		@param pszPath [in] ファイル名
	*/
	virtual BOOL LoadKeyMacro( HINSTANCE hInstance, const char* pszPath) = 0;

	//static CMacroManagerBase* Creator( const char* str );
	//純粋仮想クラスは実体化できないのでFactoryは不要。
	//継承先クラスでは必要。
	
	//	デストラクタのvirtualを忘れずに
	virtual ~CMacroManagerBase();
	

protected:
	//!	Load済みかどうかを表すフラグ true...Load済み、false...未Load。
	bool m_nReady;

public:
	/*!	Load済みかどうか

		@retval true Load済み
		@retval false 未Load
	*/
	bool IsReady(){ return m_nReady; }

	// Constructor
	CMacroManagerBase();

};

#endif
