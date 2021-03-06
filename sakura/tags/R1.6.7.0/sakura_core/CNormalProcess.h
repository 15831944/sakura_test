/*!	@file
	@brief エディタプロセスクラスヘッダファイル

	@author aroka
	@date	2002/01/08 作成
*/
/*
	Copyright (C) 2002, aroka 新規作成

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#ifndef _CNORMALPROCESS_H_
#define _CNORMALPROCESS_H_

#include "global.h"
#include "CProcess.h"
#include "CMigemo.h"
class CEditWnd;

/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
/*!
	@brief エディタプロセスクラス
	
	エディタプロセスはCEditWndクラスのインスタンスを作る。
*/
class CNormalProcess : public CProcess {
public:
	//コンストラクタ・デストラクタ
	CNormalProcess( HINSTANCE hInstance, LPCTSTR lpCmdLine );
	virtual ~CNormalProcess();

protected:
	//プロセスハンドラ
	virtual bool InitializeProcess();
	virtual bool MainLoop();
	virtual void OnExitProcess();

protected:
	//実装補助
	HANDLE _GetInitializeMutex() const; // 2002/2/8 aroka

private:
	CEditWnd*	m_pcEditWnd;
};


///////////////////////////////////////////////////////////////////////
#endif /* _CNORMALPROCESS_H_ */

/*[EOF]*/
