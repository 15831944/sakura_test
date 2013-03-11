#include "StdAfx.h"
#include "CViewCalc.h"
#include "mem/CMemoryIterator.h"
#include "view/CEditView.h"
#include "doc/CEditDoc.h"
#include "doc/CLayout.h"

//外部依存
CLayoutInt CViewCalc::GetTabSpace() const
{
	return m_pOwner->m_pcEditDoc->m_cLayoutMgr.GetTabSpace();
}


/* 指定された桁に対応する行のデータ内の位置を調べる Ver1
	
	@@@ 2002.09.28 YAZAKI CDocLine版
*/
CLogicInt CViewCalc::LineColmnToIndex( const CDocLine* pcDocLine, CLayoutInt nColumn ) const
{
	CLogicInt i2 = CLogicInt(0);
	CMemoryIterator it( pcDocLine, GetTabSpace() );
	while( !it.end() ){
		it.scanNext();
		if ( it.getColumn() + it.getColumnDelta() > nColumn ){
			break;
		}
		it.addDelta();
	}
	i2 += it.getIndex();
	return i2;
}


/* 指定された桁に対応する行のデータ内の位置を調べる Ver1
	
	@@@ 2002.09.28 YAZAKI CLayoutが必要になりました。
*/
CLogicInt CViewCalc::LineColmnToIndex( const CLayout* pcLayout, CLayoutInt nColumn ) const
{
	CLogicInt i2 = CLogicInt(0);
	CMemoryIterator it( pcLayout, GetTabSpace() );
	while( !it.end() ){
		it.scanNext();
		if ( it.getColumn() + it.getColumnDelta() > nColumn ){
			break;
		}
		it.addDelta();
	}
	i2 += it.getIndex();
	return i2;
}



/* 指定された桁に対応する行のデータ内の位置を調べる Ver0 */
/* 指定された桁より、行が短い場合はpnLineAllColLenに行全体の表示桁数を返す */
/* それ以外の場合はpnLineAllColLenに０をセットする
	
	@@@ 2002.09.28 YAZAKI CLayoutが必要になりました。
*/
CLogicInt CViewCalc::LineColmnToIndex2( const CLayout* pcLayout, CLayoutInt nColumn, CLayoutInt* pnLineAllColLen ) const
{
	*pnLineAllColLen = CLayoutInt(0);

	CLogicInt i2 = CLogicInt(0);
	CLayoutInt nPosX2 = CLayoutInt(0);
	CMemoryIterator it( pcLayout, GetTabSpace() );
	while( !it.end() ){
		it.scanNext();
		if ( it.getColumn() + it.getColumnDelta() > nColumn ){
			break;
		}
		it.addDelta();
	}
	i2 += it.getIndex();
	if( i2 >= pcLayout->GetLengthWithEOL() ){
		nPosX2 += it.getColumn();
		*pnLineAllColLen = nPosX2;
	}
	return i2;
}





/*
||	指定された行のデータ内の位置に対応する桁の位置を調べる
||
||	@@@ 2002.09.28 YAZAKI CLayoutが必要になりました。
*/
CLayoutInt CViewCalc::LineIndexToColmn( const CLayout* pcLayout, CLogicInt nIndex ) const
{
	//	以下、iterator版
	CLayoutInt nPosX2 = CLayoutInt(0);
	CMemoryIterator it( pcLayout, GetTabSpace() );
	while( !it.end() ){
		it.scanNext();
		if ( it.getIndex() + it.getIndexDelta() > nIndex ){
			break;
		}
		it.addDelta();
	}
	nPosX2 += it.getColumn();
	return nPosX2;
}


/*
||	指定された行のデータ内の位置に対応する桁の位置を調べる
||
||	@@@ 2002.09.28 YAZAKI CDocLine版
*/
CLayoutInt CViewCalc::LineIndexToColmn( const CDocLine* pcDocLine, CLogicInt nIndex ) const
{
	CLayoutInt nPosX2 = CLayoutInt(0);
	CMemoryIterator it( pcDocLine, GetTabSpace() );
	while( !it.end() ){
		it.scanNext();
		if ( it.getIndex() + it.getIndexDelta() > nIndex ){
			break;
		}
		it.addDelta();
	}
	nPosX2 += it.getColumn();
	return nPosX2;
}



