//	$Id$
/*!	@file
	テキストのレイアウト情報管理

	@author Norio Nakatani
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "CLayoutMgr.h"
#include "charcode.h"
#include "etc_uty.h"
#include "debug.h"
#include <commctrl.h>
#include "CRunningTimer.h"
#include <stdlib.h>



/* 文字列置換 */
void CLayoutMgr::ReplaceData_CLayoutMgr(
		LayoutReplaceArg*	pArg
#if 0
		int			nDelLineFrom,			/* 削除範囲行  From レイアウト行番号 */
		int			nDelColmFrom,			/* 削除範囲位置From レイアウト行桁位置 */
		int			nDelLineTo,				/* 削除範囲行  To   レイアウト行番号 */
		int			nDelColmTo,				/* 削除範囲位置To   レイアウト行桁位置 */
		CMemory*	pcmemDeleted,			/* 削除されたデータ */
		const char*	pInsData,				/* 挿入するデータ */
		int			nInsDataLen,			/* 挿入するデータの長さ */

		int*		pnAddLineNum,			/* 再描画ヒント レイアウト行の増減 */
		int*		pnModLineFrom,			/* 再描画ヒント 変更されたレイアウト行From(レイアウト行の増減が0のとき使う) */
		int*		pnModLineTo,			/* 再描画ヒント 変更されたレイアウト行From(レイアウト行の増減が0のとき使う) */

		int*		pnNewLine,				/* 挿入された部分の次の位置の行(レイアウト行) */
		int*		pnNewPos,				/* 挿入された部分の次の位置のデータ位置(レイアウト桁位置) */

		BOOL		bDispSSTRING,			/* シングルクォーテーション文字列を表示する */
		BOOL		bDispWSTRING,			/* ダブルクォーテーション文字列を表示する */
		BOOL		bUndo					/* Undo操作かどうか */
#endif
)
{
//	int nDeletedLineNum;	/* 削除した行の総数 */
//	int nInsLineNum;		/* 挿入によって増えた行の数 */
//	int nNewLine;			/* 挿入された部分の次の位置の行 */
//	int nNewPos;			/* 挿入された部分の次の位置のデータ位置 */
	int	nxFrom;
	int	nyFrom;
	int	nxTo;
	int	nyTo;
	CLayout* pLayout;
	CLayout* pLayoutWork;
	int nCurrentLineType;
	int nLineWork;

	/* 置換先頭位置のレイアウト情報 */
	pLayout = (CLayout*)Search( pArg->nDelLineFrom );
	nCurrentLineType = 0;
	pLayoutWork = pLayout;
	nLineWork = pArg->nDelLineFrom;

	if( NULL != pLayoutWork ){
		while( 0 != pLayoutWork->m_nOffset ){
			pLayoutWork = pLayoutWork->m_pPrev;
			nLineWork--;
		}
		nCurrentLineType = pLayoutWork->m_nTypePrev;
	}


	/*
	||  カーソル位置変換
	||  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置) →
	||  物理位置(行頭からのバイト数、折り返し無し行位置)
	*/
	CaretPos_Log2Phys( pArg->nDelColmFrom, pArg->nDelLineFrom, &nxFrom, &nyFrom );
	CaretPos_Log2Phys( pArg->nDelColmTo, pArg->nDelLineTo, &nxTo, &nyTo );

	/* 指定範囲のデータを置換(削除 & データを挿入)
	  Fromを含む位置からToの直前を含むデータを削除する
	  Fromの位置へテキストを挿入する
	*/
	DocLineReplaceArg DLRArg;
	DLRArg.nDelLineFrom = nyFrom;			/* 削除範囲行  From 改行単位の行番号 0開始) */
	DLRArg.nDelPosFrom = nxFrom;			/* 削除範囲位置From 改行単位の行頭からのバイト位置 0開始) */
	DLRArg.nDelLineTo = nyTo;				/* 削除範囲行  To   改行単位の行番号 0開始) */
	DLRArg.nDelPosTo = nxTo;				/* 削除範囲位置To   改行単位の行頭からのバイト位置 0開始) */
	DLRArg.pcmemDeleted = pArg->pcmemDeleted;	/* 削除されたデータを保存 */
//	DLRArg.nDeletedLineNum = 0;					/* 削除した行の総数 */
	DLRArg.pInsData = pArg->pInsData;			/* 挿入するデータ */
	DLRArg.nInsDataLen = pArg->nInsDataLen;		/* 挿入するデータの長さ */
//	DLRArg.nInsLineNum = 0;						/* 挿入によって増えた行の数 */
//	DLRArg.nNewLine = 0;						/* 挿入された部分の次の位置の行 */
//	DLRArg.nNewPos = 0;							/* 挿入された部分の次の位置のデータ位置 */
	m_pcDocLineMgr->ReplaceData(
		&DLRArg
#if 0
		nyFrom,					/* 削除範囲行  From 改行単位の行番号 0開始) */
		nxFrom,					/* 削除範囲位置From 改行単位の行頭からのバイト位置 0開始) */
		nyTo,					/* 削除範囲行  To   改行単位の行番号 0開始) */
		nxTo,					/* 削除範囲位置To   改行単位の行頭からのバイト位置 0開始) */
		pArg->pcmemDeleted,		/* 削除されたデータを保存 */
		&nDeletedLineNum,		/* 削除した行の総数 */

		pArg->pInsData,			/* 挿入するデータ */
		pArg->nInsDataLen,		/* 挿入するデータの長さ */
		&nInsLineNum,			/* 挿入によって増えた行の数 */
		&pArg->nNewLine,		/* 挿入された部分の次の位置の行 */
		&pArg->nNewPos			/* 挿入された部分の次の位置のデータ位置 */
#endif
	);
//	nDeletedLineNum = DLRArg.nDeletedLineNum;	/* 削除した行の総数 */
//	pArg->nInsLineNum = DLRArg.nInsLineNum;		/* 挿入によって増えた行の数 */
	pArg->nNewLine = DLRArg.nNewLine;			/* 挿入された部分の次の位置の行 */
	pArg->nNewPos = DLRArg.nNewPos;				/* 挿入された部分の次の位置のデータ位置 */



//	DUMP();

	/*--- 変更された行のレイアウト情報を再生成 ---*/
	/* 論理行の指定範囲に該当するレイアウト情報を削除して */
	/* 削除した範囲の直前のレイアウト情報のポインタを返す */

	int nAllLinesOld = m_nLines;
	int	nModifyLayoutLinesOld;
	CLayout* pLayoutPrev;
	int nWork;
	nWork = __max( DLRArg.nDeletedLineNum, DLRArg.nInsLineNum );
//	if( 0 < nWork ){
//		--nWork;
//	}


//	DUMP();

	if( NULL != pLayoutWork ){
		pLayoutPrev = DeleteLayoutAsLogical(
			pLayoutWork,
			nLineWork,

			nyFrom,
			nyFrom + nWork,
			nyFrom, nxFrom,
			&nModifyLayoutLinesOld
		);

//		DUMP();

		/* 指定行より後の行のレイアウト情報について、論理行番号を指定行数だけシフトする */
		/* 論理行が削除された場合は０より小さい行数 */
		/* 論理行が挿入された場合は０より大きい行数 */
		if( 0 != DLRArg.nInsLineNum - DLRArg.nDeletedLineNum ){
			ShiftLogicalLineNum( pLayoutPrev,
				DLRArg.nInsLineNum - DLRArg.nDeletedLineNum
			);
		}
	}else{
		pLayoutPrev = m_pLayoutBot;
	}
//	DUMP();

	/* 指定レイアウト行に対応する論理行の次の論理行から指定論理行数だけ再レイアウトする */
	int	nRowNum;
	if( NULL == pLayoutPrev ){
		if( NULL == m_pLayoutTop ){
			nRowNum = m_pcDocLineMgr->GetLineCount();
		}else{
			nRowNum = m_pLayoutTop->m_nLinePhysical;
		}
	}else{
		if( NULL == pLayoutPrev->m_pNext ){
			nRowNum =
				m_pcDocLineMgr->GetLineCount() -
				pLayoutPrev->m_nLinePhysical - 1;
		}else{
			nRowNum =
				pLayoutPrev->m_pNext->m_nLinePhysical -
				pLayoutPrev->m_nLinePhysical - 1;
		}
	}

//	if( NULL != pLayoutPrev ){
//		pLayoutNext	= pLayoutPrev->m_pNext;
//	}else{
//		pLayoutNext	= m_pLayoutBot;
//	}

	/* 指定レイアウト行に対応する論理行の次の論理行から指定論理行数だけ再レイアウトする */
//	*pnModifyLayoutLinesNew = DoLayout3(
//		pLayoutPrev, nRowNum,
//		nDelStartLogicalLine, nDelStartLogicalPos
//	);

//	MYTRACE( "pLayoutPrev=[%xh]\n", pLayoutPrev );
//	MYTRACE( "pLayoutNext=[%xh]\n", pLayoutNext );
	int nAddInsLineNum;
	pArg->nModLineTo = DoLayout3_New(
		pLayoutPrev,
//		pLayoutNext,
		nRowNum,
		nyFrom, nxFrom,
		nCurrentLineType,
		&nAddInsLineNum,
		pArg->bDispSSTRING,	/* シングルクォーテーション文字列を表示する */
		pArg->bDispWSTRING	/* ダブルクォーテーション文字列を表示する */
	);


//	DUMP();

	pArg->nAddLineNum = nModifyLayoutLinesOld - pArg->nModLineTo;/* nAddInsLineNum;*/	/* 再描画ヒント レイアウト行の増減 */
	pArg->nModLineFrom = pArg->nDelLineFrom;	/* 再描画ヒント 変更されたレイアウト行From */
	pArg->nModLineTo += ( pArg->nModLineFrom - 1 ) ;	/* 再描画ヒント 変更されたレイアウト行To */

#ifdef _DEBUG
//	MYTRACE( "削除した行の総数( DLRArg.nDeletedLineNum ) == %d \n" , DLRArg.nDeletedLineNum );/* 削除した行の総数 */
//	MYTRACE( "挿入によって増えた行の数( DLRArg.nInsLineNum ) == %d \n" , DLRArg.nInsLineNum );/* 挿入によって増えた行の数 */
//	MYTRACE( "挿入された部分の次の位置の行( nNewLine ) == %d \n" , nNewLine );/* 挿入された部分の次の位置の行 */
//	MYTRACE( "挿入された部分の次の位置のデータ位置( nNewPos ) == %d \n" , nNewPos );/* 挿入された部分の次の位置のデータ位置 */
//	MYTRACE( "nModifyLayoutLinesOld == %d \n" , nModifyLayoutLinesOld );

//	MYTRACE( "\n\n■■■■■■■■■■■■■■\n" );
//	MYTRACE( "再描画ヒント レイアウト行の増減(pArg->nAddLineNum) == %d \n" , pArg->nAddLineNum );
//	MYTRACE( "再描画ヒント 変更されたレイアウト行From(pArg->nModLineFrom) == %d \n" , pArg->nModLineFrom );
//	MYTRACE( "再描画ヒント 変更されたレイアウト行To(pArg->nModLineTo) == %d \n" , pArg->nModLineTo );
#endif

//	DoLayout( NULL, bDispSSTRING, bDispWSTRING );

	/* レイアウト位置への変換 */
	CaretPos_Phys2Log( pArg->nNewPos, pArg->nNewLine, &pArg->nNewPos, &pArg->nNewLine );

	return;
}


/*[EOF]*/
