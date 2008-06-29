#include "stdafx.h"
#include "CFigure_HanSpace.h"
#include "types/CTypeSupport.h"

void _DispHankakuSpace( CGraphics& gr, DispPos* pDispPos, bool bSearchStringMode, CEditView* pcView );

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                      CFigure_HanSpace                         //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

bool CFigure_HanSpace::DrawImp(SColorStrategyInfo* pInfo)
{
	if(!pInfo->pLine)return false;

	const CEditDoc* pcDoc = CEditDoc::GetInstance(0);
	const STypeConfig* TypeDataPtr = &pcDoc->m_cDocType.GetDocumentAttribute();

	if(pInfo->pLine[pInfo->nPos] == L' ' && TypeDataPtr->m_ColorInfoArr[COLORIDX_SPACE].m_bDisp){
		_DispHankakuSpace(pInfo->gr,pInfo->pDispPos,pInfo->bSearchStringMode,pInfo->pcView);
		return true;
	}
	return false;
}


// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                         �`�����                            //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

void _DispHankakuSpace( CGraphics& gr, DispPos* pDispPos, bool bSearchStringMode, CEditView* pcView )
{
	//�N���b�s���O��`���v�Z�B��ʊO�Ȃ�`�悵�Ȃ�
	CMyRect rcClip;
	if(pcView->GetTextArea().GenerateClipRect(&rcClip,*pDispPos,1))
	{
		// �F����
		CTypeSupport cSupport(pcView,pcView->GetTextDrawer()._GetColorIdx(COLORIDX_SPACE,bSearchStringMode));
		cSupport.SetGraphicsState_WhileThisObj(gr);
		
		//������"o"�̉��������o��
		CMyRect rcClipBottom=rcClip;
		rcClipBottom.top=rcClip.top+rcClip.Height()/2;
		::ExtTextOutW_AnyBuild(
			gr,
			pDispPos->GetDrawPos().x,
			pDispPos->GetDrawPos().y,
			ExtTextOutOption(),
			&rcClipBottom,
			L"o",
			1,
			pcView->GetTextMetrics().GetDxArray_AllHankaku()
		);
		
		//�㔼���͕��ʂ̋󔒂ŏo�́i"o"�̏㔼���������j
		CMyRect rcClipTop=rcClip;
		rcClipTop.bottom=rcClip.top+rcClip.Height()/2;
		::ExtTextOutW_AnyBuild(
			gr,
			pDispPos->GetDrawPos().x,
			pDispPos->GetDrawPos().y,
			ExtTextOutOption(),
			&rcClipTop,
			L" ",
			1,
			pcView->GetTextMetrics().GetDxArray_AllHankaku()
		);
	}

	//�ʒu�i�߂�
	pDispPos->ForwardDrawCol(1);
}