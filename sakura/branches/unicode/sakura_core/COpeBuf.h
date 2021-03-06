/*!	@file
	@brief アンドゥ・リドゥバッファ

	@author Norio Nakatani
	@date 1998/06/09 新規作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

class COpeBuf;

#ifndef _COPEBUF_H_
#define _COPEBUF_H_


#include "global.h"
class COpeBlk;/// 2002/2/10 aroka




/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
/*!
	@brief アンドゥ・リドゥバッファ
*/
class SAKURA_CORE_API COpeBuf {
public:
	COpeBuf();	//!< COpeBufクラス構築
	~COpeBuf();	//!< COpeBufクラス消滅

	//状態
	bool IsEnableUndo( void );		//!< Undo可能な状態か
	bool IsEnableRedo( void );		//!< Redo可能な状態か

	//操作
	void ClearAll( void );			//!< 全要素のクリア
	int AppendOpeBlk( COpeBlk* );	//!< 操作ブロックの追加
	COpeBlk* DoUndo( bool* );		//!< 現在のUndo対象の操作ブロックを返す
	COpeBlk* DoRedo( bool* );		//!< 現在のRedo対象の操作ブロックを返す
	void SetNoModified( void );		//!< 現在位置で無変更な状態になったことを通知

	//デバッグ
	void DUMP( void );				//!< 編集操作要素ブロックのダンプ

private:
	int			m_nCOpeBlkArrNum;	//!< 操作ブロックの数
	COpeBlk**	m_ppCOpeBlkArr;		//!< 操作ブロックの配列
	int			m_nCurrentPointer;	//!< 現在位置
	int			m_nNoModifiedIndex;	//!< 無変更な状態になった位置
};



///////////////////////////////////////////////////////////////////////
#endif /* _COPEBUF_H_ */


/*[EOF]*/
