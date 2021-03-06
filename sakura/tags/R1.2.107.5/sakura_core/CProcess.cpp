//	$Id$
/*!	@file
	@brief プロセス基底クラス

	@author aroka
	@date 2002/01/07 作成
	@date 2002/01/17 修正
	$Revision$
*/
/*
	Copyright (C) 2002, aroka 新規作成

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/


#include "CProcess.h"
#include "debug.h"
#include "etc_uty.h"

/*!
	@brief プロセス基底クラス
	
	@author aroka
	@date 2002/01/07
*/
CProcess::CProcess(
	HINSTANCE	hInstance,		//!< handle to process instance
	LPSTR		lpCmdLine		//!< pointer to command line
) :
	m_hInstance( hInstance ),
	m_CommandLine( lpCmdLine ),
	m_hWnd( 0 )
{
}

/*!
	@brief プロセスを初期化する

	共有メモリを初期化する
*/
bool CProcess::Initialize()
{
	/* 共有データ構造体のアドレスを返す */
	if( !m_cShareData.Init() ){
		//	適切なデータを得られなかった
		::MYMESSAGEBOX( NULL, MB_OK | MB_ICONERROR,
			GSTR_APPNAME, _T("異なるバージョンのエディタを同時に起動することはできません。") );
		return false;
	}
	m_pShareData = m_cShareData.GetShareData();

	/* リソースから製品バージョンの取得 */
	GetAppVersionInfo( m_hInstance, VS_VERSION_INFO,
		&m_pShareData->m_dwProductVersionMS, &m_pShareData->m_dwProductVersionLS );

	return true;
}

/*!
	@brief プロセス実行
	
	@author aroka
	@date 2002/01/16
*/
bool CProcess::Run(void)
{
	if( true == Initialize() ){
		MainLoop() ;
		Terminate();
		return true;
	}
	return false;
}
/*[EOF]*/
