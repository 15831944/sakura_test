/*!	@file
	@brief プロセス基底クラス

	@author aroka
	@date 2002/01/07 作成
	@date 2002/01/17 修正
*/
/*
	Copyright (C) 2002, aroka 新規作成
	Copyright (C) 2004, Moca
	Copyright (C) 2009, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/


#include "stdafx.h"
#include "CProcess.h"
#include "debug/Debug.h"
#include "util/module.h"

/*!
	@brief プロセス基底クラス
	
	@author aroka
	@date 2002/01/07
*/
CProcess::CProcess(
	HINSTANCE	hInstance,		//!< handle to process instance
	LPTSTR		lpCmdLine		//!< pointer to command line
)
: m_hInstance( hInstance )
, m_CommandLine( lpCmdLine )
, m_hWnd( 0 )
, m_pfnMiniDumpWriteDump(NULL)
{
}

/*!
	@brief プロセスを初期化する

	共有メモリを初期化する
*/
bool CProcess::InitializeProcess()
{
	/* 共有データ構造体のアドレスを返す */
	if( !m_cShareData.InitShareData() ){
		//	適切なデータを得られなかった
		::MYMESSAGEBOX( NULL, MB_OK | MB_ICONERROR,
			GSTR_APPNAME, _T("異なるバージョンのエディタを同時に起動することはできません。") );
		return false;
	}

	/* リソースから製品バージョンの取得 */
	//	2004.05.13 Moca 共有データのバージョン情報はコントロールプロセスだけが
	//	ShareDataで設定するように変更したのでここからは削除

	return true;
}

/*!
	@brief プロセス実行
	
	@author aroka
	@date 2002/01/16
*/
bool CProcess::Run()
{
	if( InitializeProcess() )
	{
		HMODULE hDllDbgHelp = LoadLibraryExedir( _T("dbghelp.dll") );
		m_pfnMiniDumpWriteDump = NULL;
		if( hDllDbgHelp ){
			*(FARPROC*)&m_pfnMiniDumpWriteDump = ::GetProcAddress( hDllDbgHelp, "MiniDumpWriteDump" );
		}

		__try {
			MainLoop() ;
			OnExitProcess();
		}
		__except( WriteDump( GetExceptionInformation() ) ){
		}

		if( hDllDbgHelp ){
			::FreeLibrary( hDllDbgHelp );
			m_pfnMiniDumpWriteDump = NULL;
		}
		return true;
	}
	return false;
}

/*!
	@brief クラッシュダンプ
	
	@author ryoji
	@date 2009.01.21
*/
int CProcess::WriteDump( PEXCEPTION_POINTERS pExceptPtrs )
{
	if( !m_pfnMiniDumpWriteDump )
		return EXCEPTION_CONTINUE_SEARCH;

	static TCHAR szFile[MAX_PATH];
	GetInidirOrExedir( szFile, _APP_NAME_(_T) _T(".dmp") );	// 出力先はiniと同じ（InitializeProcess()後に確定）

	HANDLE hFile = ::CreateFile(
		szFile,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
		NULL);

	if( hFile != INVALID_HANDLE_VALUE ){
		MINIDUMP_EXCEPTION_INFORMATION eInfo;
		eInfo.ThreadId = GetCurrentThreadId();
		eInfo.ExceptionPointers = pExceptPtrs;
		eInfo.ClientPointers = FALSE;

		m_pfnMiniDumpWriteDump(
			::GetCurrentProcess(),
			::GetCurrentProcessId(),
			hFile,
			MiniDumpNormal,
			pExceptPtrs ? &eInfo : NULL,
			NULL,
			NULL);

		::CloseHandle(hFile);
	}

	return EXCEPTION_CONTINUE_SEARCH;
}
