/*!	@file
	@brief 行コメントデリミタを管理する

	@author Yazaki
	@date 2002/09/17 新規作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2002, Yazaki, Moca

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/
/* LMP (Lucien Murray-Pitts) : 2011-02-26 Added Basic English Translation Resources */

#include "stdafx.h"
#include "CLineComment.h"
#include "my_icmp.h" // 2002/11/30 Moca 追加

CLineComment::CLineComment()
{
	int i;
	for ( i=0; i<COMMENT_DELIMITER_NUM; i++ ){
		m_pszLineComment[i][0] = '\0';
		m_nLineCommentPos[i] = -1;
	}
}

/*!
	行コメントデリミタをコピーする
	@param n [in]           コピー対象のコメント番号
	@param buffer [in]      コメント文字列
	@param nCommentPos [in] コメント位置．-1のときは指定無し．
*/
void CLineComment::CopyTo( const int n, const char* buffer, int nCommentPos )
{
	int nStrLen = lstrlen( buffer );
	if( 0 < nStrLen && nStrLen < COMMENT_DELIMITER_BUFFERSIZE ){
		strcpy( m_pszLineComment[n], buffer );
		m_nLineCommentPos[n] = nCommentPos;
		m_nLineCommentLen[n] = nStrLen;
	}
	else {
		m_pszLineComment[n][0] = '\0';
		m_nLineCommentPos[n] = -1;
		m_nLineCommentLen[n] = 0;
	}
}

bool CLineComment::Match( int nPos, int nLineLen, const char* pLine ) const
{
	int i;
	for ( i=0; i<COMMENT_DELIMITER_NUM; i++ ){
		if (
		  '\0' != m_pszLineComment[i][0] &&	/* 行コメントデリミタ */
		  ( m_nLineCommentPos[i] < 0 || nPos == m_nLineCommentPos[i] ) &&	//	位置指定ON.
		  nPos <= nLineLen - m_nLineCommentLen[i] &&	/* 行コメントデリミタ */
		  0 == memicmp( &pLine[nPos], m_pszLineComment[i], m_nLineCommentLen[i] )
		){
			return true;
		}
	}
	return false;
}
/*[EOF]*/
