/*!	@file
	@brief アウトライン解析 データ配列

	@author Norio Nakatani
	@date	1998/06/23 作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2002, YAZAKI, aroka

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "stdafx.h"
#include <stdlib.h>
#include <malloc.h>
#include "CFuncInfoArr.h"
#include "debug/Debug.h"// 2002/2/10 aroka


/* CFuncInfoArrクラス構築 */
CFuncInfoArr::CFuncInfoArr()
{
	m_nFuncInfoArrNum = 0;	/* 配列要素数 */
	m_ppcFuncInfoArr = NULL;	/* 配列 */
	return;
}




/* CFuncInfoArrクラス消滅 */
CFuncInfoArr::~CFuncInfoArr()
{
	Empty();
	return;
}

void CFuncInfoArr::Empty( void )
{
	int i;
	if( m_nFuncInfoArrNum > 0 && NULL != m_ppcFuncInfoArr ){
		for( i = 0; i < m_nFuncInfoArrNum; ++i ){
			delete m_ppcFuncInfoArr[i];
			m_ppcFuncInfoArr[i] = NULL;
		}
		m_nFuncInfoArrNum = 0;
		free( m_ppcFuncInfoArr );
		m_ppcFuncInfoArr = NULL;
	}
	return;
}


/* 0<=の指定番号のデータを返す */
/* データがない場合はNULLを返す */
CFuncInfo* CFuncInfoArr::GetAt( int nIdx )
{
	if( nIdx >= m_nFuncInfoArrNum ){
		return NULL;
	}
	return m_ppcFuncInfoArr[nIdx];
}



/*! 配列の最後にデータを追加する */
void CFuncInfoArr::AppendData( CFuncInfo* pcFuncInfo )
{
	if( 0 == m_nFuncInfoArrNum){
		m_ppcFuncInfoArr = (CFuncInfo**)malloc( sizeof(CFuncInfo*) * (m_nFuncInfoArrNum + 1) );
	}else{
		m_ppcFuncInfoArr = (CFuncInfo**)realloc( m_ppcFuncInfoArr, sizeof(CFuncInfo*) * (m_nFuncInfoArrNum + 1) );
	}
	m_ppcFuncInfoArr[m_nFuncInfoArrNum] = pcFuncInfo;
	m_nFuncInfoArrNum++;
	return;
}



/*! 配列の最後にデータを追加する

	@date 2002.04.01 YAZAKI 深さ導入
*/
void CFuncInfoArr::AppendData(
	CLogicInt		nFuncLineCRLF,		//!< 関数のある行(CRLF単位)
	CLayoutInt		nFuncLineLAYOUT,	//!< 関数のある行(折り返し単位)
	const TCHAR*	pszFuncName,		//!< 関数名
	int				nInfo,				//!< 付加情報
	int				nDepth				//!< 深さ
)
{
	CFuncInfo* pcFuncInfo = new CFuncInfo( nFuncLineCRLF, nFuncLineLAYOUT, pszFuncName, nInfo );
	pcFuncInfo->m_nDepth = nDepth;
	AppendData( pcFuncInfo );
	return;
}

void CFuncInfoArr::AppendData(
	CLogicInt			nFuncLineCRLF,		//!< 関数のある行(CRLF単位)
	CLayoutInt			nFuncLineLAYOUT,	//!< 関数のある行(折り返し単位)
	const NOT_TCHAR*	pszFuncName,		//!< 関数名
	int					nInfo,				//!< 付加情報
	int					nDepth				//!< 深さ
)
{
	AppendData(nFuncLineCRLF,nFuncLineLAYOUT,to_tchar(pszFuncName),nInfo,nDepth);
}


void CFuncInfoArr::DUMP( void )
{
#ifdef _DEBUG
	int i;
	MYTRACE_A( "=============================\n" );
	for( i = 0; i < m_nFuncInfoArrNum; i++ ){
		MYTRACE_A( "[%d]------------------\n", i );
		MYTRACE_A( "m_nFuncLineCRLF	=%d\n", m_ppcFuncInfoArr[i]->m_nFuncLineCRLF );
		MYTRACE_A( "m_nFuncLineLAYOUT	=%d\n", m_ppcFuncInfoArr[i]->m_nFuncLineLAYOUT );
		MYTRACE_A( "m_cmemFuncName	=[%ls]\n", m_ppcFuncInfoArr[i]->m_cmemFuncName.GetStringPtr() );
		MYTRACE_A( "m_nInfo			=%d\n", m_ppcFuncInfoArr[i]->m_nInfo );
	}
	MYTRACE_A( "=============================\n" );
#endif
}



