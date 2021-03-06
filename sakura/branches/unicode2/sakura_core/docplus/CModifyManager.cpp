#include "stdafx.h"
#include "docplus/CModifyManager.h"


void CModifyManager::OnAfterSave(const SSaveInfo& sSaveInfo)
{
	CEditDoc* pcDoc = GetListeningDoc();

	// 行変更状態をすべてリセット
	CModifyVisitor().ResetAllModifyFlag(&pcDoc->m_cDocLineMgr);
}



bool CModifyVisitor::IsLineModified(const CDocLine* pcDocLine) const
{
	return pcDocLine->m_sMark.m_cModified;
}
void CModifyVisitor::SetLineModified(CDocLine* pcDocLine, bool bModified)
{
	pcDocLine->m_sMark.m_cModified = bModified;
}

/* 行変更状態をすべてリセット */
/*
  ・変更フラグCDocLineオブジェクト作成時にはTRUEである
  ・変更回数はCDocLineオブジェクト作成時には1である

  ファイルを読み込んだときは変更フラグを FALSEにする
  ファイルを読み込んだときは変更回数を 0にする

  ファイルを上書きした時は変更フラグを FALSEにする
  ファイルを上書きした時は変更回数は変えない

  変更回数はUndoしたときに-1される
  変更回数が0になった場合は変更フラグをFALSEにする
*/
void CModifyVisitor::ResetAllModifyFlag(CDocLineMgr* pcDocLineMgr)
{
	CDocLine* pDocLine = pcDocLineMgr->GetDocLineTop();
	while( pDocLine ){
		CDocLine* pDocLineNext = pDocLine->GetNextLine();
		SetLineModified(pDocLine, false);
		pDocLine = pDocLineNext;
	}
}


