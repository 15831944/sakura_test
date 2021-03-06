//	$Id$
/*!	@file
	@brief 処理所要時間の計測クラス

	デバッグ目的で用いる

	@author Norio Nakatani
	@date 1998/03/06  新規作成
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

class CRunningTimer;

#ifndef _CRUNNINGTIMER_H_
#define _CRUNNINGTIMER_H_

#include <windows.h>


/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
/*!
	@brief 処理所要時間の計測クラス
*/
class CRunningTimer
{
public:
	/*
	||  Constructors
	*/
//	CRunningTimer();
	CRunningTimer( const char* );
	~CRunningTimer();

	/*
	|| 関数
	*/
	void Reset();
	DWORD Read();

protected:
	DWORD	m_nStartTime;
	char	m_szText[100];
	static int m_nNestCount;
};




///////////////////////////////////////////////////////////////////////
#endif /* _CRUNNINGTIMER_H_ */


/*[EOF]*/
