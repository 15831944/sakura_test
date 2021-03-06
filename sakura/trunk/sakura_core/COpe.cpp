/*!	@file
	@brief 編集操作要素

	@author Norio Nakatani
	@date 1998/06/09 新規作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "StdAfx.h"
#include <assert.h>
#include "COpe.h"
#include "CMemory.h"// 2002/2/10 aroka
#include "Debug.h"


// COpeクラス構築
COpe::COpe(EOpeCode eCode)
{
	assert( eCode != OPE_UNKNOWN );
	m_nOpe = eCode;					// 操作種別

	m_ptCaretPos_PHY_To.x = 0;		// 操作前のキャレット位置Ｘ To 改行単位行の行番号（０開始）
	m_ptCaretPos_PHY_To.y = 0;		// 操作前のキャレット位置Ｙ To 改行単位行先頭からのバイト数（０開始）
	m_ptCaretPos_PHY_Before.x = -1;	// カーソル位置 改行単位行先頭からのバイト数（０開始）
	m_ptCaretPos_PHY_Before.y = -1;	// カーソル位置 改行単位行の行番号（０開始）
	m_ptCaretPos_PHY_After.x = -1;	// カーソル位置 改行単位行先頭からのバイト数（０開始）
	m_ptCaretPos_PHY_After.y = -1;	// カーソル位置 改行単位行の行番号（０開始）
	m_pcmemData = NULL;				// 操作に関連するデータ 
}




/* COpeクラス消滅 */
COpe::~COpe()
{
	if( NULL != m_pcmemData ){	/* 操作に関連するデータ */
		delete m_pcmemData;
		m_pcmemData = NULL;
	}
}

/* 編集操作要素のダンプ */
void COpe::DUMP( void )
{
	DEBUG_TRACE( _T("\t\tm_nOpe                  = [%d]\n"), m_nOpe               );
	DEBUG_TRACE( _T("\t\tm_ptCaretPos_PHY_Before = [%d,%d]\n"), m_ptCaretPos_PHY_Before.x, m_ptCaretPos_PHY_Before.y   );
	DEBUG_TRACE( _T("\t\tm_ptCaretPos_PHY_After  = [%d,%d]\n"), m_ptCaretPos_PHY_After.x, m_ptCaretPos_PHY_After.y   );
	if( NULL == m_pcmemData ){
	DEBUG_TRACE( _T("\t\tm_nDataLen              = [%d]\n"), 0 );
		DEBUG_TRACE( _T("\t\tm_pcmemData         = [NULL]\n") );
	}else{
	DEBUG_TRACE( _T("\t\tm_nDataLen              = [%d]\n"), m_pcmemData->GetStringLength() );
		DEBUG_TRACE( _T("\t\tm_pcmemData         = [%s]\n"), m_pcmemData->GetStringPtr() );
	}
	return;
}


/*[EOF]*/
