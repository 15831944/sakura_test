#include "stdafx.h"
#include "CColor_CtrlCode.h"
#include "parse/CWordParse.h"
#include "util/string_ex2.h"
#include "doc/CLayout.h"
#include "types/CTypeSupport.h"


// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                        ����R�[�h                           //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

EColorIndexType CColor_CtrlCode::BeginColor(SColorStrategyInfo* pInfo)
{
	if(!pInfo->pLine)return _COLORIDX_NOCHANGE;

	const CEditDoc* pcDoc = CEditDoc::GetInstance(0);
	const STypeConfig* TypeDataPtr = &pcDoc->m_cDocType.GetDocumentAttribute();

	if(TypeDataPtr->m_ColorInfoArr[COLORIDX_CTRLCODE].m_bDisp	/* �R���g���[���R�[�h��F���� */
		&&WCODE::IsControlCode(pInfo->pLine[pInfo->nPos]))
	{
		this->nCOMMENTMODE_OLD = pInfo->nCOMMENTMODE;
		this->nCOMMENTEND_OLD = pInfo->nCOMMENTEND;

		/* �R���g���[���R�[�h��̏I�[��T�� */
		int i;
		for( i = pInfo->nPos + 1; i <= pInfo->nLineLen - 1; ++i ){
			if(!WCODE::IsControlCode(pInfo->pLine[i])){
				break;
			}
		}
		pInfo->nCOMMENTEND = i;
		return COLORIDX_CTRLCODE;
	}

	return _COLORIDX_NOCHANGE;
}

bool CColor_CtrlCode::EndColor(SColorStrategyInfo* pInfo)
{
	if( pInfo->nPos == pInfo->nCOMMENTEND ){
		pInfo->nCOMMENTMODE = this->nCOMMENTMODE_OLD;
		pInfo->nCOMMENTEND = this->nCOMMENTEND_OLD;
		return true;
	}
	return false;
}
