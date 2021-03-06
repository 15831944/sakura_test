//	$Id$
/************************************************************************

	COpe.h

	編集操作要素
	Copyright (C) 1998-2000, Norio Nakatani

    CREATE: 1998/6/9  新規作成

************************************************************************/

class COpe;

#ifndef _COPE_H_
#define _COPE_H_

#include "CMemory.h"




/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
/* 編集操作要素　COpe */
class COpe {
	public:
	   COpe();	/* COpeクラス構築 */
	   ~COpe();	/* COpeクラス消滅 */

		void DUMP( void );	/* 編集操作要素のダンプ */

		int		m_nOpe;			/* 操作種別 */

//- 1999.12.22 メモリ食う		
//-		int		m_nCaretPosX_Before;	/* 操作前のキャレット位置Ｘ */
//-		int		m_nCaretPosY_Before;	/* 操作前のキャレット位置Ｙ */
//-		int		m_nCaretPosX_To;	/* 操作前のキャレット位置Ｘ To */
//-		int		m_nCaretPosY_To;	/* 操作前のキャレット位置Ｙ To */
//-		int		m_nCaretPosX_After; 	/* 操作後のキャレット位置Ｘ */
//-		int		m_nCaretPosY_After; 	/* 操作後のキャレット位置Ｙ */

		int		m_nCaretPosX_PHY_Before;	/* カーソル位置　改行単位行先頭からのバイト数（０開始） */
		int		m_nCaretPosY_PHY_Before;	/* カーソル位置　改行単位行の行番号（０開始） */
		int		m_nCaretPosX_PHY_To;	/* 操作前のキャレット位置Ｘ To 改行単位行先頭からのバイト数（０開始）*/
		int		m_nCaretPosY_PHY_To;	/* 操作前のキャレット位置Ｙ To 改行単位行の行番号（０開始）*/
		int		m_nCaretPosX_PHY_After;		/* カーソル位置　改行単位行先頭からのバイト数（０開始） */
		int		m_nCaretPosY_PHY_After;		/* カーソル位置　改行単位行の行番号（０開始） */
	   

		
		int		m_nDataLen;				/* 操作に関連するデータのサイズ */
		CMemory*	m_pcmemData;			/* 操作に関連するデータ */

	public:
	private:
};



///////////////////////////////////////////////////////////////////////
#endif /* _COPE_H_ */

/*[EOF]*/
