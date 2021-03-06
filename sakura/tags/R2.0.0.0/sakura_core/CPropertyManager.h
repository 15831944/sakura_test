#pragma once

#include "prop/CPropCommon.h"
#include "typeprop/CPropTypes.h"

class CPropertyManager{
public:
	CPropertyManager();
	/*
	|| その他
	*/
	BOOL OpenPropertySheet( int nPageNum );	/* 共通設定 */
	BOOL OpenPropertySheetTypes( int nPageNum, CTypeConfig nSettingType );	/* タイプ別設定 */

public:
	CPropCommon			m_cPropCommon;
	CPropTypes			m_cPropTypes;
};
