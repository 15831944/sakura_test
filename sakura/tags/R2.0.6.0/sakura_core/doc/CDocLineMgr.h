/*!	@file
	@brief 行データの管理

	@author Norio Nakatani
	@date 1998/3/5  新規作成
	@date 2001/06/23 N.Nakatani WhereCurrentWord_2()追加 staticメンバ
	@date 2001/12/03 hor しおり(bookmark)機能追加に伴う関数追加
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, genta
	Copyright (C) 2001, hor, genta
	Copyright (C) 2002, aroka, MIK, hor
	Copyright (C) 2003, Moca, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#ifndef _CDOCLINEMGR_H_
#define _CDOCLINEMGR_H_

#include <Windows.h>
#include "_main/global.h" // 2002/2/10 aroka
#include "basis/SakuraBasis.h"
class CDocLine; // 2002/2/10 aroka
class CMemory; // 2002/2/10 aroka
class CBregexp; // 2002/2/10 aroka

struct DocLineReplaceArg {
	CLogicRange		sDelRange;			//!< 削除範囲。ロジック単位。
	CNativeW*		pcmemDeleted;		//!< 削除されたデータを保存
	CLogicInt		nDeletedLineNum;	//!< 削除した行の総数
	const wchar_t*	pInsData;			//!< 挿入するデータ
	int				nInsDataLen;		//!< 挿入するデータの長さ
	CLogicInt		nInsLineNum;		//!< 挿入によって増えた行の数
	CLogicPoint		ptNewPos;			//!< 挿入された部分の次の位置
};

/*-----------------------------------------------------------------------
クラスの宣言
-----------------------------------------------------------------------*/
//2007.09.30 kobake WhereCurrentWord_2 を CWordParse に移動
class SAKURA_CORE_API CDocLineMgr{
public:
	//コンストラクタ・デストラクタ
	CDocLineMgr();
	~CDocLineMgr();

	//状態
	CLogicInt GetLineCount() const{ return m_nLines; }	//!< 全行数を返す

	//行データへのアクセス
	CDocLine* GetLine( CLogicInt nLine ) const;						//!< 指定行を取得
	CDocLine* GetDocLineTop() const { return m_pDocLineTop; }		//!< 先頭行を取得
	CDocLine* GetDocLineBottom() const { return m_pDocLineBot; }	//!< 最終行を取得

	//行データの管理
	CDocLine* InsertNewLine(CDocLine* pPos);	//!< pPosの直前に新しい行を挿入
	CDocLine* AddNewLine();						//!< 最下部に新しい行を挿入
	void DeleteAllLine();						//!< 全ての行を削除する
	void DeleteLine( CDocLine* );				//!< 行の削除

	//デバッグ
	void DUMP();


	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
	//                         実装補助                            //
	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
protected:
	void _Init();
	// -- -- チェーン関数 -- -- // 2007.10.11 kobake 作成
	void _PushBottom(CDocLine* pDocLineNew);             //!< 最下部に挿入
	void _InsertBeforePos(CDocLine* pDocLineNew, CDocLine* pPos); //!< pPosの直前に挿入
	void _InsertAfterPos(CDocLine* pDocLineNew, CDocLine* pPos); //!< pPosの直後に挿入

	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
	//                        メンバ変数                           //
	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
private:
	CDocLine*	m_pDocLineTop;		//!< 最初の行
	CDocLine*	m_pDocLineBot;		//!< 最後の行(※1行しかない場合はm_pDocLineTopと等しくなる)
	CLogicInt	m_nLines;			//!< 全行数

public:
	//$$ kobake注: 以下、絶対に切り離したい（最低切り離せなくても、変数の意味をコメントで明確に記すべき）変数群
	mutable CDocLine*	m_pDocLineCurrent;	//!< 順アクセス時の現在位置
	mutable CLogicInt	m_nPrevReferLine;
	mutable CDocLine*	m_pCodePrevRefer;
};


///////////////////////////////////////////////////////////////////////
#endif /* _CDOCLINEMGR_H_ */



