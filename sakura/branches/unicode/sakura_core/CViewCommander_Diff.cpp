#include "stdafx.h"
#include "CViewCommander.h"
#include "CEditView.h"
#include "CEditDoc.h"
#include "CDlgDiff.h"
#include "CEditWnd.h"

/*!	差分表示
	@note	HandleCommandからの呼び出し対応(ダイアログなし版)
	@author	maru
	@date	2005/10/28 これまでのCommand_Diffはm_pCommanderView->ViewDiffInfoに名称変更
*/
void CViewCommander::Command_Diff( const WCHAR* _szTmpFile2, int nFlgOpt )
{
	const TCHAR* szTmpFile2 = to_tchar(_szTmpFile2);

	bool	bTmpFile1 = false;
	TCHAR	szTmpFile1[_MAX_PATH * 2];

	if( -1 == ::GetFileAttributes( szTmpFile2 ) )
	{
		::MYMESSAGEBOX( m_pCommanderView->m_hWnd,	MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME,
			_T("差分コマンド実行は失敗しました。\n\n比較するファイルが見つかりません。") );
		return;
	}

	//自ファイル
	if (!GetDocument()->IsModified()) _tcscpy( szTmpFile1, GetDocument()->GetFilePath());
	else if (m_pCommanderView->MakeDiffTmpFile ( szTmpFile1, NULL )) bTmpFile1 = true;
	else return;

	//差分表示
	m_pCommanderView->ViewDiffInfo(szTmpFile1, szTmpFile2, nFlgOpt);

	//一時ファイルを削除する
	if( bTmpFile1 ) _tunlink( szTmpFile1 );

	return;

}

/*!	差分表示
	@note	HandleCommandからの呼び出し対応(ダイアログあり版)
	@author	MIK
	@date	2002/05/25
	@date	2002/11/09 編集中ファイルを許可
	@date	2005/10/29 maru 一時ファイル作成処理をm_pCommanderView->MakeDiffTmpFileへ移動
*/
void CViewCommander::Command_Diff_Dialog( void )
{
	CDlgDiff	cDlgDiff;
	bool	bTmpFile1 = false, bTmpFile2 = false;
	TCHAR	szTmpFile1[_MAX_PATH * 2];
	TCHAR	szTmpFile2[_MAX_PATH * 2];

	//DIFF差分表示ダイアログを表示する
	if( FALSE == cDlgDiff.DoModal( GetInstance(), m_pCommanderView->m_hWnd, (LPARAM)GetDocument(),
		GetDocument()->GetFilePath(),
		GetDocument()->IsModified() ) )
	{
		return;
	}
	
	//自ファイル
	if (!GetDocument()->IsModified()) _tcscpy( szTmpFile1, GetDocument()->GetFilePath());
	else if (m_pCommanderView->MakeDiffTmpFile ( szTmpFile1, NULL )) bTmpFile1 = true;
	else return;
		
	//相手ファイル
	if (!cDlgDiff.m_bIsModifiedDst ) _tcscpy( szTmpFile2, cDlgDiff.m_szFile2);
	else if (m_pCommanderView->MakeDiffTmpFile ( szTmpFile2, cDlgDiff.m_hWnd_Dst )) bTmpFile2 = true;
	else 
	{
		if( bTmpFile1 ) _tunlink( szTmpFile1 );
		return;
	}
	
	//差分表示
	m_pCommanderView->ViewDiffInfo(szTmpFile1, szTmpFile2, cDlgDiff.m_nDiffFlgOpt);
	
	
	//一時ファイルを削除する
	if( bTmpFile1 ) _tunlink( szTmpFile1 );
	if( bTmpFile2 ) _tunlink( szTmpFile2 );

	return;
}


/*!	次の差分を探し，見つかったら移動する
*/
void CViewCommander::Command_Diff_Next( void )
{
	BOOL		bFound = FALSE;
	BOOL		bRedo = TRUE;

	CLogicPoint	ptXY(0, GetCaret().GetCaretLogicPos().y);
	int			nYOld_Logic = ptXY.y;
	CLogicInt tmp_y;

re_do:;	
	if( GetDocument()->m_cDocLineMgr.SearchDiffMark( ptXY.GetY2(), SEARCH_FORWARD, &tmp_y ) ){
		ptXY.y = tmp_y;
		bFound = TRUE;
		CLayoutPoint ptXY_Layout;
		GetDocument()->m_cLayoutMgr.LogicToLayout( ptXY, &ptXY_Layout );
		if( m_pCommanderView->GetSelectionInfo().m_bSelectingLock ){
			if( !m_pCommanderView->GetSelectionInfo().IsTextSelected() ) m_pCommanderView->GetSelectionInfo().BeginSelectArea();
		}
		else{
			if( m_pCommanderView->GetSelectionInfo().IsTextSelected() ) m_pCommanderView->GetSelectionInfo().DisableSelectArea( TRUE );
		}

		GetCaret().MoveCursor( ptXY_Layout, TRUE );
		if( m_pCommanderView->GetSelectionInfo().m_bSelectingLock ){
			m_pCommanderView->GetSelectionInfo().ChangeSelectAreaByCurrentCursor( ptXY_Layout );
		}
	}


	if( GetShareData()->m_Common.m_sSearch.m_bSearchAll ){
		// 見つからなかった。かつ、最初の検索
		if( !bFound	&& bRedo ){
			ptXY.y = 0 - 1;	// 1個手前を指定
			bRedo = FALSE;
			goto re_do;		// 先頭から再検索
		}
	}

	if( bFound ){
		if( nYOld_Logic >= ptXY.y ) m_pCommanderView->SendStatusMessage( _T("▼先頭から再検索しました") );
	}
	else{
		m_pCommanderView->SendStatusMessage( _T("▽見つかりませんでした") );
		if( GetShareData()->m_Common.m_sSearch.m_bNOTIFYNOTFOUND )	/* 見つからないときメッセージを表示 */
			InfoMessage( m_pCommanderView->m_hWnd, _T("後方(↓) に差分が見つかりません。") );
	}

	return;
}



/*!	前の差分を探し，見つかったら移動する
*/
void CViewCommander::Command_Diff_Prev( void )
{
	BOOL		bFound = FALSE;
	BOOL		bRedo = TRUE;

	CLogicPoint	ptXY(0,GetCaret().GetCaretLogicPos().y);
	int			nYOld_Logic = ptXY.y;
	CLogicInt tmp_y;

re_do:;
	if( GetDocument()->m_cDocLineMgr.SearchDiffMark( ptXY.GetY2(), SEARCH_BACKWARD, &tmp_y) ){
		ptXY.y = tmp_y;
		bFound = TRUE;
		CLayoutPoint ptXY_Layout;
		GetDocument()->m_cLayoutMgr.LogicToLayout( ptXY, &ptXY_Layout );
		if( m_pCommanderView->GetSelectionInfo().m_bSelectingLock ){
			if( !m_pCommanderView->GetSelectionInfo().IsTextSelected() ) m_pCommanderView->GetSelectionInfo().BeginSelectArea();
		}
		else{
			if( m_pCommanderView->GetSelectionInfo().IsTextSelected() ) m_pCommanderView->GetSelectionInfo().DisableSelectArea( TRUE );
		}

		GetCaret().MoveCursor( ptXY_Layout, TRUE );
		if( m_pCommanderView->GetSelectionInfo().m_bSelectingLock ){
			m_pCommanderView->GetSelectionInfo().ChangeSelectAreaByCurrentCursor( ptXY_Layout );
		}
	}

	if( GetShareData()->m_Common.m_sSearch.m_bSearchAll ){
		// 見つからなかった、かつ、最初の検索
		if( !bFound	&& bRedo ){
			ptXY.y = (CLogicInt)(Int)GetDocument()->m_cLayoutMgr.GetLineCount();	// 1個手前を指定 //$$ 単位混在
			bRedo = FALSE;
			goto re_do;	// 末尾から再検索
		}
	}

	if( bFound ){
		if( nYOld_Logic <= ptXY.y ) m_pCommanderView->SendStatusMessage( _T("▲末尾から再検索しました") );
	}
	else{
		m_pCommanderView->SendStatusMessage( _T("△見つかりませんでした") );
		if( GetShareData()->m_Common.m_sSearch.m_bNOTIFYNOTFOUND )	/* 見つからないときメッセージを表示 */
			InfoMessage( m_pCommanderView->m_hWnd, _T("前方(↑) に差分が見つかりません。") );
	}

	return;
}

/*!	差分表示の全解除
	@author	MIK
	@date	2002/05/26
*/
void CViewCommander::Command_Diff_Reset( void )
{
	GetDocument()->m_cDocLineMgr.ResetAllDiffMark();

	//分割したビューも更新
	for( int v = 0; v < 4; ++v )
		if( GetEditWindow()->m_nActivePaneIndex != v )
			GetEditWindow()->m_pcEditViewArr[v]->Redraw();
	m_pCommanderView->Redraw();
	return;
}
