/*!	@file
	@brief Entry Point

	@author Norio Nakatani
	@date	1998/03/13 作成
	@date	2001/06/26 genta ワード単位のGrepのためのコマンドライン処理追加
	@date	2002/01/08 aroka 処理の流れを整理、未使用コードを削除
	@date	2002/01/18 aroka 虫取り＆リリース
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001, genta
	Copyright (C) 2002, aroka

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "stdafx.h"
#include <windows.h>
#include "CProcessFactory.h"
#include "CProcess.h"
#include "debug/CRunningTimer.h"

/*!
	Windows Entry point

	1つ目のエディタプロセスの場合は、このプロセスはコントロールプロセスと
	なり、新しいエディタプロセスを起動する。そうでないときはエディタプロセス
	となる。

	コントロールプロセスはCControlProcessクラスのインスタンスを作り、
	エディタプロセスはCNormalProcessクラスのインスタンスを作る。
*/
int WINAPI _tWinMain(
	HINSTANCE	hInstance,		//!< handle to current instance
	HINSTANCE	hPrevInstance,	//!< handle to previous instance
	LPTSTR		lpCmdLine,		//!< pointer to command line
	int			nCmdShow		//!< show state of window
)
{
	MY_RUNNINGTIMER(cRunningTimer, "WinMain" );
	setlocale( LC_ALL, "Japanese" ); //2007.08.16 kobake 追加

	//開発情報
	DEBUG_TRACE(_T("-- -- WinMain -- --\n"));
	DEBUG_TRACE(_T("sizeof(DLLSHAREDATA) = %d\n"),sizeof(DLLSHAREDATA));

	//プロセスの生成とメッセージループ
	CProcessFactory aFactory;
	CProcess *process = 0;
	try{
		process = aFactory.Create( hInstance, lpCmdLine );
		MY_TRACETIME( cRunningTimer, "ProcessObject Created" );
	}
	catch(...){
	}
	if( 0 != process ){
		process->Run();
		delete process;
	}
	return 0;
}


